/*
 * Copyright (c) Tony Bybell 2006-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* this code implements generic vlists.  (see the original paper
   from Phil Bagwell in 2002.)  the original idea was to
   clean up histents by using vlist_alloc() to create a growable
   array that doesn't require next pointers per-element, however
   that doesn't seem necessary given the space savings that
   gzipped dormant vlist entries buys you.

   the vlists have been modified since the original version in
   two ways: (1) only half as many bytes are allocated as needed
   and when vlist_alloc() reaches the halfway point the struct
   is finally reallocated with the rest, (2) if vlist_spill is
   set to "on" in the rc file, vlist entries spill to a tempfile
   which can reduce memory usage dramatically.
 */

#include <config.h>
#include "globals.h"
#include "vlist.h"
#include <zlib.h>
#include <string.h>

/* create / destroy
 */
struct vlist_t *vlist_create(unsigned int elem_siz)
{
    struct vlist_t *v;

    v = calloc_2(1, sizeof(struct vlist_t) + elem_siz);
    v->siz = 1;
    v->elem_siz = elem_siz;

    return (v);
}

void vlist_destroy(struct vlist_t *v)
{
    struct vlist_t *vt;

    while (v) {
        vt = v->next;
        free_2(v);
        v = vt;
    }
}

/* realtime compression/decompression of bytewise vlists
 * this can obviously be extended if elem_siz > 1, but
 * the viewer doesn't need that feature
 */
struct vlist_t *vlist_compress_block(struct vlist_t *v, unsigned int *rsiz)
{
    if (v->siz > 32) {
        struct vlist_t *vz;
        unsigned int *ipnt;
        char *dmem = malloc_2(compressBound(v->siz));
        unsigned long destlen = v->siz;
        int rc;

        rc = compress2((unsigned char *)dmem,
                       &destlen,
                       (unsigned char *)(v + 1),
                       v->siz,
                       GLOBALS->vlist_compression_depth);
        if ((rc == Z_OK) && ((destlen + sizeof(int)) < v->siz)) {
            /* printf("siz: %d, dest: %d rc: %d\n", v->siz, (int)destlen, rc); */

            vz = malloc_2(*rsiz = sizeof(struct vlist_t) + sizeof(int) + destlen);
            memcpy(vz, v, sizeof(struct vlist_t));

            ipnt = (unsigned int *)(vz + 1);
            ipnt[0] = destlen;
            memcpy(&ipnt[1], dmem, destlen);
            vz->offs = (unsigned int)(-(int)v->offs); /* neg value signified compression */
            free_2(v);
            v = vz;
        }

        free_2(dmem);
    }

    return (v);
}

void vlist_uncompress(struct vlist_t **v)
{
    struct vlist_t *vl = *v;
    struct vlist_t *vprev = NULL;

    while (vl) {
        if ((int)vl->offs < 0) {
            struct vlist_t *vz = malloc_2(sizeof(struct vlist_t) + vl->siz);
            unsigned int *ipnt;
            unsigned long sourcelen, destlen;
            int rc;

            memcpy(vz, vl, sizeof(struct vlist_t));
            vz->offs = (unsigned int)(-(int)vl->offs);

            ipnt = (unsigned int *)(vl + 1);
            sourcelen = (unsigned long)ipnt[0];
            destlen = (unsigned long)vl->siz;

            rc = uncompress((unsigned char *)(vz + 1),
                            &destlen,
                            (unsigned char *)&ipnt[1],
                            sourcelen);
            if (rc != Z_OK) {
                fprintf(stderr,
                        "Error in vlist uncompress(), rc=%d/destlen=%d exiting!\n",
                        rc,
                        (int)destlen);
                exit(255);
            }

            free_2(vl);
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
void *vlist_alloc(struct vlist_t **v, int compressable)
{
    struct vlist_t *vl = *v;
    char *px;
    struct vlist_t *v2;

    if (vl->offs == vl->siz) {
        unsigned int siz, rsiz;

        /* 2 times versions are the growable, indexable vlists */
        siz = 2 * vl->siz;

        rsiz = sizeof(struct vlist_t) + (vl->siz * vl->elem_siz);

        if ((compressable) && (vl->elem_siz == 1)) {
            if (GLOBALS->vlist_compression_depth >= 0) {
                vl = vlist_compress_block(vl, &rsiz);
            }
        }

        v2 = calloc_2(1, sizeof(struct vlist_t) + (vl->siz * vl->elem_siz));
        v2->siz = siz;
        v2->elem_siz = vl->elem_siz;
        v2->next = vl;
        *v = v2;
        vl = *v;
    } else if (vl->offs * 2 == vl->siz) {
        v2 = calloc_2(1, sizeof(struct vlist_t) + (vl->siz * vl->elem_siz));
        memcpy(v2, vl, sizeof(struct vlist_t) + (vl->siz / 2 * vl->elem_siz));
        free_2(vl);

        *v = v2;
        vl = *v;
    }

    px = (((char *)(vl)) + sizeof(struct vlist_t) + ((vl->offs++) * vl->elem_siz));
    return ((void *)px);
}

/* vlist_size() and vlist_locate() do not work properly on
   compressed lists...you'll have to call vlist_uncompress() first!
 */
unsigned int vlist_size(struct vlist_t *v)
{
    return (v->siz - 1 + v->offs);
}

void *vlist_locate(struct vlist_t *v, unsigned int idx)
{
    unsigned int here = v->siz - 1;
    unsigned int siz = here + v->offs; /* siz is the same as vlist_size() */

    if ((!siz) || (idx >= siz))
        return (NULL);

    while (idx < here) {
        v = v->next;
        here = v->siz - 1;
    }

    idx -= here;

    return ((void *)(((char *)(v)) + sizeof(struct vlist_t) + (idx * v->elem_siz)));
}

/* calling this if you don't plan on adding any more elements will free
   up unused space as well as compress final blocks (if enabled)
 */
void vlist_freeze(struct vlist_t **v)
{
    struct vlist_t *vl = *v;
    unsigned int siz = vl->offs;
    unsigned int rsiz = sizeof(struct vlist_t) + (siz * vl->elem_siz);

    if ((vl->elem_siz == 1) && (siz)) {
        struct vlist_t *w, *v2;

        if (vl->offs * 2 <= vl->siz) /* Electric Fence, change < to <= */
        {
            v2 = calloc_2(1,
                          sizeof(struct vlist_t) + (vl->siz /* * vl->elem_siz */)); /* scan-build */
            memcpy(v2,
                   vl,
                   sizeof(struct vlist_t) + (vl->siz / 2 /* * vl->elem_siz */)); /* scan-build */
            free_2(vl);

            *v = v2;
            vl = *v;
        }

        w = vlist_compress_block(vl, &rsiz);
        *v = w;
    } else if (siz != vl->siz) {
        struct vlist_t *w = malloc_2(rsiz);
        memcpy(w, vl, rsiz);
        free_2(vl);
        *v = w;
    }
}

/* this code implements an LZ-based filter that can sit on top
   of the vlists.  it uses a generic escape value of 0xff as
   that is one that statistically occurs infrequently in value
   change data fed into vlists.
 */

void vlist_packer_emit_out(struct vlist_packer_t *p, unsigned char byt)
{
    char *pnt;

#ifdef WAVE_VLIST_PACKER_STATS
    p->packed_bytes++;
#endif

    pnt = vlist_alloc(&p->v, 1);
    *pnt = byt;
}

void vlist_packer_emit_uv32(struct vlist_packer_t *p, unsigned int v)
{
    unsigned int nxt;

    while ((nxt = v >> 7)) {
        vlist_packer_emit_out(p, v & 0x7f);
        v = nxt;
    }

    vlist_packer_emit_out(p, (v & 0x7f) | 0x80);
}

void vlist_packer_emit_uv32rvs(struct vlist_packer_t *p, unsigned int v)
{
    unsigned int nxt;
    unsigned char buf[2 * sizeof(int)];
    unsigned int idx = 0;
    int i;

    while ((nxt = v >> 7)) {
        buf[idx++] = v & 0x7f;
        v = nxt;
    }

    buf[idx] = (v & 0x7f) | 0x80;

    for (i = idx; i >= 0; i--) {
        vlist_packer_emit_out(p, buf[i]);
    }
}

void vlist_packer_alloc(struct vlist_packer_t *p, unsigned char byt)
{
    int i, j, k, l;

    p->unpacked_bytes++;

    if (!p->repcnt) {
    top:
        for (i = 0; i < WAVE_ZIVSRCH; i++) {
            if (p->buf[(p->bufpnt - i) & WAVE_ZIVMASK] == byt) {
                p->repdist = i;
                p->repcnt = 1;

                p->repdist2 = p->repdist3 = p->repdist4 = 0;

                for (j = i + WAVE_ZIVSKIP; j < WAVE_ZIVSRCH; j++) {
                    if (p->buf[(p->bufpnt - j) & WAVE_ZIVMASK] == byt) {
                        p->repdist2 = j;
                        p->repcnt2 = 1;

                        for (k = j + WAVE_ZIVSKIP; k < WAVE_ZIVSRCH; k++) {
                            if (p->buf[(p->bufpnt - k) & WAVE_ZIVMASK] == byt) {
                                p->repdist3 = k;
                                p->repcnt3 = 1;

                                for (l = k + WAVE_ZIVSKIP; l < WAVE_ZIVSRCH; l++) {
                                    if (p->buf[(p->bufpnt - l) & WAVE_ZIVMASK] == byt) {
                                        p->repdist4 = l;
                                        p->repcnt4 = 1;
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }

                p->bufpnt++;
                p->bufpnt &= WAVE_ZIVMASK;
                p->buf[p->bufpnt] = byt;

                return;
            }
        }

        p->bufpnt++;
        p->bufpnt &= WAVE_ZIVMASK;
        p->buf[p->bufpnt] = byt;
        vlist_packer_emit_out(p, byt);
        if (byt == WAVE_ZIVFLAG) {
            vlist_packer_emit_uv32(p, 0);
        }
    } else {
    attempt2:
        if (p->buf[(p->bufpnt - p->repdist) & WAVE_ZIVMASK] == byt) {
            p->repcnt++;

            if (p->repcnt2) {
                p->repcnt2 = ((p->buf[(p->bufpnt - p->repdist2) & WAVE_ZIVMASK] == byt))
                                 ? p->repcnt2 + 1
                                 : 0;
            }
            if (p->repcnt3) {
                p->repcnt3 = ((p->buf[(p->bufpnt - p->repdist3) & WAVE_ZIVMASK] == byt))
                                 ? p->repcnt3 + 1
                                 : 0;
            }
            if (p->repcnt4) {
                p->repcnt4 = ((p->buf[(p->bufpnt - p->repdist4) & WAVE_ZIVMASK] == byt))
                                 ? p->repcnt4 + 1
                                 : 0;
            }

            p->bufpnt++;
            p->bufpnt &= WAVE_ZIVMASK;
            p->buf[p->bufpnt] = byt;
        } else {
            if (p->repcnt2) {
                p->repcnt = p->repcnt2;
                p->repdist = p->repdist2;

                p->repcnt2 = p->repcnt3;
                p->repdist2 = p->repdist3;

                p->repcnt3 = p->repcnt4;
                p->repdist3 = p->repdist4;

                p->repcnt4 = 0;
                p->repdist4 = 0;

                goto attempt2;
            }

            if (p->repcnt3) {
                p->repcnt = p->repcnt3;
                p->repdist = p->repdist3;

                p->repcnt2 = p->repcnt4;
                p->repdist2 = p->repdist4;

                p->repcnt3 = 0;
                p->repdist3 = 0;
                p->repcnt4 = 0;
                p->repdist4 = 0;

                goto attempt2;
            }

            if (p->repcnt4) {
                p->repcnt = p->repcnt4;
                p->repdist = p->repdist4;

                p->repcnt4 = 0;
                p->repdist4 = 0;

                goto attempt2;
            }

            if (p->repcnt > 2) {
                vlist_packer_emit_out(p, WAVE_ZIVFLAG);
                vlist_packer_emit_uv32(p, p->repcnt);
                p->repcnt = 0;
                vlist_packer_emit_uv32(p, p->repdist);
            } else {
                if (p->repcnt == 2) {
                    vlist_packer_emit_out(p, p->buf[(p->bufpnt - 1) & WAVE_ZIVMASK]);
                    if (p->buf[(p->bufpnt - 1) & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                        vlist_packer_emit_uv32(p, 0);
                    }
                }

                vlist_packer_emit_out(p, p->buf[p->bufpnt & WAVE_ZIVMASK]);
                p->repcnt = 0;
                if (p->buf[p->bufpnt & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                    vlist_packer_emit_uv32(p, 0);
                }
            }
            goto top;
        }
    }
}

void vlist_packer_finalize(struct vlist_packer_t *p)
{
#ifdef WAVE_VLIST_PACKER_STATS
    static guint64 pp = 0, upp = 0;
#endif

    if (p->repcnt) {
        if (p->repcnt > 2) {
            vlist_packer_emit_out(p, WAVE_ZIVFLAG);
            vlist_packer_emit_uv32(p, p->repcnt);
            p->repcnt = 0;
            vlist_packer_emit_uv32(p, p->repdist);
        } else {
            if (p->repcnt == 2) {
                vlist_packer_emit_out(p, p->buf[(p->bufpnt - 1) & WAVE_ZIVMASK]);
                if (p->buf[(p->bufpnt - 1) & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                    vlist_packer_emit_uv32(p, 0);
                }
            }

            vlist_packer_emit_out(p, p->buf[p->bufpnt & WAVE_ZIVMASK]);
            p->repcnt = 0;
            if (p->buf[p->bufpnt & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                vlist_packer_emit_uv32(p, 0);
            }
        }
    }

    vlist_packer_emit_uv32rvs(p, p->unpacked_bytes); /* for malloc later during decompress */

#ifdef WAVE_VLIST_PACKER_STATS
    pp += p->packed_bytes;
    upp += p->unpacked_bytes;

    printf("pack:%d orig:%d (%lld %lld %f)\n",
           p->packed_bytes,
           p->unpacked_bytes,
           pp,
           upp,
           (float)pp / (float)upp);
#endif
}

struct vlist_packer_t *vlist_packer_create(void)
{
    struct vlist_packer_t *vp = calloc_2(1, sizeof(struct vlist_packer_t));
    vp->v = vlist_create(sizeof(char));

    return (vp);
}

unsigned char *vlist_packer_decompress(struct vlist_t *v, unsigned int *declen)
{
    unsigned int list_size = vlist_size(v);
    unsigned int top_of_packed_size = list_size - 1;
    unsigned char *chp;
    unsigned int dec_size = 0;
    unsigned int dec_size_cmp;
    unsigned int shamt = 0;
    unsigned char *mem, *dpnt;
    unsigned int i, j, repcnt, dist;

    for (;;) {
        chp = vlist_locate(v, top_of_packed_size);

        dec_size |= ((unsigned int)(*chp & 0x7f)) << shamt;

        if (*chp & 0x80) {
            break;
        }

        shamt += 7;
        top_of_packed_size--;
    }

    mem = calloc_2(1, WAVE_ZIVWRAP + dec_size);
    dpnt = mem + WAVE_ZIVWRAP;
    for (i = 0; i < top_of_packed_size; i++) {
        chp = vlist_locate(v, i);
        if (*chp != WAVE_ZIVFLAG) {
            *(dpnt++) = *chp;
            continue;
        }

        i++;
        repcnt = shamt = 0;
        for (;;) {
            chp = vlist_locate(v, i);

            repcnt |= ((unsigned int)(*chp & 0x7f)) << shamt;

            if (*chp & 0x80) {
                break;
            }
            i++;

            shamt += 7;
        }
        if (repcnt == 0) {
            *(dpnt++) = WAVE_ZIVFLAG;
            continue;
        }

        i++;
        dist = shamt = 0;
        for (;;) {
            chp = vlist_locate(v, i);

            dist |= ((unsigned int)(*chp & 0x7f)) << shamt;

            if (*chp & 0x80) {
                break;
            }
            i++;

            shamt += 7;
        }

        for (j = 0; j < repcnt; j++) {
            *dpnt = *(dpnt - dist - 1);
            dpnt++;
        }
    }

    *declen = dec_size;

    dec_size_cmp = dpnt - mem - WAVE_ZIVWRAP;
    if (dec_size != dec_size_cmp) {
        fprintf(stderr,
                "miscompare: decompressed vlist_packer length: %d vs %d bytes, exiting.\n",
                dec_size,
                dec_size_cmp);
        exit(255);
    }

    return (mem + WAVE_ZIVWRAP);
}

void vlist_packer_decompress_destroy(char *mem)
{
    free_2(mem - WAVE_ZIVWRAP);
}
