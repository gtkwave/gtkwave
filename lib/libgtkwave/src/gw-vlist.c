#include "gw-vlist.h"
#include <zlib.h>

/* create / destroy */
GwVlist *gw_vlist_create(unsigned int element_size)
{
    GwVlist *v = g_malloc0(sizeof(GwVlist) + element_size);
    v->size = 1;
    v->element_size = element_size;

    return v;
}

void gw_vlist_destroy(GwVlist *self)
{
    while (self != NULL) {
        GwVlist *vt = self->next;
        g_free(self);
        self = vt;
    }
}

/* realtime compression/decompression of bytewise vlists
 * this can obviously be extended if elem_siz > 1, but
 * the viewer doesn't need that feature
 */
static GwVlist *gw_vlist_compress_block(GwVlist *v, guint *rsize, gint compression_level)
{
    if (v->size <= 32) {
        return v;
    }

    GwVlist *vz;
    unsigned int *ipnt;
    char *dmem = g_malloc(compressBound(v->size));
    unsigned long destlen = v->size;
    int rc;

    rc = compress2((unsigned char *)dmem,
                   &destlen,
                   (unsigned char *)(v + 1),
                   v->size,
                   compression_level);
    if ((rc == Z_OK) && ((destlen + sizeof(int)) < v->size)) {
        /* printf("siz: %d, dest: %d rc: %d\n", v->siz, (int)destlen, rc); */

        vz = g_malloc(*rsize = sizeof(GwVlist) + sizeof(int) + destlen);
        memcpy(vz, v, sizeof(GwVlist));

        ipnt = (unsigned int *)(vz + 1);
        ipnt[0] = destlen;
        memcpy(&ipnt[1], dmem, destlen);
        vz->offset = (unsigned int)(-(int)v->offset); /* neg value signified compression */
        g_free(v);
        v = vz;
    }

    g_free(dmem);

    return (v);
}

void gw_vlist_uncompress(GwVlist **v)
{
    GwVlist *vl = *v;
    GwVlist *vprev = NULL;

    while (vl != NULL) {
        if ((int)vl->offset < 0) {
            GwVlist *vz = g_malloc(sizeof(GwVlist) + vl->size);
            unsigned int *ipnt;
            unsigned long sourcelen, destlen;
            int rc;

            memcpy(vz, vl, sizeof(GwVlist));
            vz->offset = (unsigned int)(-(int)vl->offset);

            ipnt = (unsigned int *)(vl + 1);
            sourcelen = (unsigned long)ipnt[0];
            destlen = (unsigned long)vl->size;

            rc = uncompress((unsigned char *)(vz + 1),
                            &destlen,
                            (unsigned char *)&ipnt[1],
                            sourcelen);
            if (rc != Z_OK) {
                g_error("Error in vlist uncompress(), rc=%d/destlen=%d exiting!", rc, (int)destlen);
            }

            g_free(vl);
            vl = vz;

            if (vprev) {
                vprev->next = vz;
            } else {
                *v = vz;
            }
        }

        vprev = vl;
        vl = vl->next;
    }
}

/* get pointer to one unit of space
 */
void *gw_vlist_alloc(GwVlist **v, gboolean compressable, gint compression_level)
{
    GwVlist *vl = *v;
    char *px;
    GwVlist *v2;

    if (vl->offset == vl->size) {
        unsigned int siz, rsiz;

        /* 2 times versions are the growable, indexable vlists */
        siz = 2 * vl->size;

        rsiz = sizeof(GwVlist) + (vl->size * vl->element_size);

        if (compressable && vl->element_size == 1) {
            if (compression_level >= 0) {
                vl = gw_vlist_compress_block(vl, &rsiz, compression_level);
            }
        }

        v2 = g_malloc0(sizeof(GwVlist) + (vl->size * vl->element_size));
        v2->size = siz;
        v2->element_size = vl->element_size;
        v2->next = vl;
        *v = v2;
        vl = *v;
    } else if (vl->offset * 2 == vl->size) {
        v2 = g_malloc0(sizeof(GwVlist) + (vl->size * vl->element_size));
        memcpy(v2, vl, sizeof(GwVlist) + (vl->size / 2 * vl->element_size));
        g_free(vl);

        *v = v2;
        vl = *v;
    }

    px = (((char *)(vl)) + sizeof(GwVlist) + ((vl->offset++) * vl->element_size));
    return ((void *)px);
}

/* vlist_size() and vlist_locate() do not work properly on
   compressed lists...you'll have to call vlist_uncompress() first!
 */
guint gw_vlist_size(GwVlist *self)
{
    return self->size - 1 + self->offset;
}

void *gw_vlist_locate(GwVlist *self, unsigned int idx)
{
    unsigned int here = self->size - 1;
    unsigned int siz = here + self->offset; /* siz is the same as vlist_size() */

    if ((!siz) || (idx >= siz))
        return (NULL);

    while (idx < here) {
        self = self->next;
        here = self->size - 1;
    }

    idx -= here;

    return ((void *)(((char *)(self)) + sizeof(GwVlist) + (idx * self->element_size)));
}

/* calling this if you don't plan on adding any more elements will free
   up unused space as well as compress final blocks (if enabled)
 */
void gw_vlist_freeze(GwVlist **v, gint compression_level)
{
    GwVlist *vl = *v;
    unsigned int siz = vl->offset;
    unsigned int rsiz = sizeof(GwVlist) + (siz * vl->element_size);

    if ((vl->element_size == 1) && (siz)) {
        GwVlist *w, *v2;

        if (vl->offset * 2 <= vl->size) /* Electric Fence, change < to <= */
        {
            v2 = g_malloc0(sizeof(GwVlist) + (vl->size /* * vl->elem_siz */)); /* scan-build */
            memcpy(v2, vl, sizeof(GwVlist) + (vl->size / 2 /* * vl->elem_siz */)); /* scan-build */
            g_free(vl);

            *v = v2;
            vl = *v;
        }

        w = gw_vlist_compress_block(vl, &rsiz, compression_level);
        *v = w;
    } else if (siz != vl->size) {
        GwVlist *w = g_malloc(rsiz);
        memcpy(w, vl, rsiz);
        g_free(vl);
        *v = w;
    }
}
