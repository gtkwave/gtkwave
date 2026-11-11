#include "gw-vcd-file.h"
#include "gw-vcd-file-private.h"
#include "gw-vlist-reader.h"
#include <stdio.h>

G_DEFINE_TYPE(GwVcdFile, gw_vcd_file, GW_TYPE_DUMP_FILE)

static void gw_vcd_file_import_trace(GwVcdFile *self, GwNode *np);

static gboolean gw_vcd_file_import_traces(GwDumpFile *dump_file, GwNode **nodes, GError **error)
{
    GwVcdFile *self = GW_VCD_FILE(dump_file);
    (void)error;

    for (GwNode **iter = nodes; *iter != NULL; iter++) {
        GwNode *node = *iter;

        if (node->mv.mvlfac_vlist != NULL) {
            gw_vcd_file_import_trace(self, node);
        }
    }

    return TRUE;
}

static void gw_vcd_file_dispose(GObject *object)
{
    GwVcdFile *self = GW_VCD_FILE(object);

    g_clear_object(&self->hist_ent_factory);

    G_OBJECT_CLASS(gw_vcd_file_parent_class)->dispose(object);
}

static void gw_vcd_file_class_init(GwVcdFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GwDumpFileClass *dump_file_class = GW_DUMP_FILE_CLASS(klass);

    object_class->dispose = gw_vcd_file_dispose;

    dump_file_class->import_traces = gw_vcd_file_import_traces;
}

static void gw_vcd_file_init(GwVcdFile *self)
{
    self->hist_ent_factory = gw_hist_ent_factory_new();
}

static void add_histent_string(GwVcdFile *self, GwTime tim, GwNode *n, const char *str)
{
    if (!n->curr) {
        GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        he->flags = (GW_HIST_ENT_FLAG_STRING | GW_HIST_ENT_FLAG_REAL);
        he->time = -1;
        he->v.h_vector = NULL;

        n->curr = he;
        n->head.next = he;
    }

    if (n->curr->time == tim) {
        // TODO: add warning
        // DEBUG(printf("Warning: String Glitch at time [%" GW_TIME_FORMAT
        //              "] Signal [%p].\n",
        //              tim,
        //              n));
        g_free(n->curr->v.h_vector);
        n->curr->v.h_vector = g_strdup(str); /* we have a glitch! */

        if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
            n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
        }
    } else {
        GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        he->flags = (GW_HIST_ENT_FLAG_STRING | GW_HIST_ENT_FLAG_REAL);
        he->time = tim;
        he->v.h_vector = g_strdup(str);

        n->curr->next = he;
        n->curr = he;
    }
}

static void add_histent_real(GwVcdFile *self, GwTime tim, GwNode *n, gdouble value)
{
    if (!n->curr) {
        GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        he->flags = GW_HIST_ENT_FLAG_REAL;
        he->time = -1;
        he->v.h_double = strtod("NaN", NULL);

        n->curr = he;
        n->head.next = he;
    }

    if ((n->curr->v.h_double != value) || (tim == self->start_time) || (self->preserve_glitches) ||
        (self->preserve_glitches_real)) /* same region == go skip */
    {
        if (n->curr->time == tim) {
            // TODO: add warning
            // DEBUG(printf("Warning: Real number Glitch at time [%" GW_TIME_FORMAT
            //              "] Signal [%p].\n",
            //              tim,
            //              n));
            n->curr->v.h_double = value;
            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
            }
        } else {
            GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            he->flags = GW_HIST_ENT_FLAG_REAL;
            he->time = tim;
            he->v.h_double = value;
            n->curr->next = he;
            n->curr = he;
        }
    }
}

static void add_histent_vector(GwVcdFile *self, GwTime tim, GwNode *n, guint8 *vector, guint len)
{
    if (!n->curr) {
        GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        he->time = -1;
        he->v.h_vector = NULL;

        n->curr = he;
        n->head.next = he;
    }

    if ((n->curr->v.h_vector && vector && memcmp(n->curr->v.h_vector, vector, len) != 0) ||
        (tim == self->start_time) || (!n->curr->v.h_vector) ||
        (self->preserve_glitches)) /* same region == go skip */
    {
        if (n->curr->time == tim) {
            // TODO: add warning
            // DEBUG(printf("Warning: Glitch at time [%" GW_TIME_FORMAT
            //              "] Signal [%p], Value [%c->%c].\n",
            //              tim,
            //              n,
            //              gw_bit_to_char(n->curr->v.h_val),
            //              ch));
            g_free(n->curr->v.h_vector);
            n->curr->v.h_vector = vector; /* we have a glitch! */

            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
            }
        } else {
            GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            he->time = tim;
            he->v.h_vector = vector;

            n->curr->next = he;
            n->curr = he;
        }
    } else {
        g_free(vector);
    }
}

static void add_histent_scalar(GwVcdFile *self, GwTime tim, GwNode *n, GwBit bit)
{
    if (!n->curr) {
        GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        he->time = -1;
        he->v.h_val = GW_BIT_X;

        n->curr = he;
        n->head.next = he;
    }

    if ((n->curr->v.h_val != bit) || (tim == self->start_time) ||
        (n->vartype == GW_VAR_TYPE_VCD_EVENT) ||
        (self->preserve_glitches)) /* same region == go skip */
    {
        if (n->curr->time == tim) {
            // TODO: add warning
            // DEBUG(printf("Warning: Glitch at time [%" GW_TIME_FORMAT
            //              "] Signal [%p], Value [%c->%c].\n",
            //              tim,
            //              n,
            //              gw_bit_to_char(n->curr->v.h_val),
            //              ch));
            n->curr->v.h_val = bit; /* we have a glitch! */

            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
            }
        } else {
            GwHistEnt *he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            he->time = tim;
            he->v.h_val = bit;

            n->curr->next = he;
            if (n->curr->v.h_val == bit) {
                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
            }
            n->curr = he;
        }
    }
}

static void gw_vcd_file_import_trace_scalar(GwVcdFile *self, GwNode *np, GwVlistReader *reader)
{
    GwTime time_scale = gw_dump_file_get_time_scale(GW_DUMP_FILE(self));
    unsigned int time_idx = 0;

    static const GwBit EXTRA_VALUES[] =
        {GW_BIT_X, GW_BIT_Z, GW_BIT_H, GW_BIT_U, GW_BIT_W, GW_BIT_L, GW_BIT_DASH, GW_BIT_X};

    while (!gw_vlist_reader_is_done(reader)) {
        guint32 accum = gw_vlist_reader_read_uv32(reader);

        GwBit bit;
        guint delta;
        if (!(accum & 1)) {
            delta = accum >> 2;
            bit = accum & 2 ? GW_BIT_1 : GW_BIT_0;
        } else {
            delta = accum >> 4;
            guint index = (accum >> 1) & 7;
            bit = EXTRA_VALUES[index];
        }
        time_idx += delta;

        GwTime *curtime_pnt = gw_vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
        if (!curtime_pnt) {
            g_error("malformed bitwise signal data for '%s' after time_idx = %d",
                    np->nname,
                    time_idx - delta);
        }

        GwTime t = *curtime_pnt * time_scale;
        add_histent_scalar(self, t, np, bit);
    }

    add_histent_scalar(self, GW_TIME_MAX - 1, np, GW_BIT_X);
    add_histent_scalar(self, GW_TIME_MAX, np, GW_BIT_Z);
}

static void gw_vcd_file_import_trace_vector(GwVcdFile *self,
                                            GwNode *np,
                                            GwVlistReader *reader,
                                            guint32 len)
{
    GwTime time_scale = gw_dump_file_get_time_scale(GW_DUMP_FILE(self));
    unsigned int time_idx = 0;
    guint8 *sbuf = g_malloc(len + 1);

    while (!gw_vlist_reader_is_done(reader)) {
        guint delta = gw_vlist_reader_read_uv32(reader);
        time_idx += delta;

        GwTime *curtime_pnt = gw_vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
        if (!curtime_pnt) {
            g_error("malformed 'b' signal data for '%s' after time_idx = %d",
                    np->nname,
                    time_idx - delta);
        }
        GwTime t = *curtime_pnt * time_scale;

        guint32 dst_len = 0;
        for (;;) {
            gint c = gw_vlist_reader_next(reader);
            if (c < 0) {
                break;
            }
            if ((c >> 4) == GW_BIT_MASK) {
                break;
            }
            if (dst_len == len) {
                if (len != 1)
                    memmove(sbuf, sbuf + 1, dst_len - 1);
                dst_len--;
            }
            sbuf[dst_len++] = c >> 4;
            if ((c & GW_BIT_MASK) == GW_BIT_MASK)
                break;
            if (dst_len == len) {
                if (len != 1)
                    memmove(sbuf, sbuf + 1, dst_len - 1);
                dst_len--;
            }
            sbuf[dst_len++] = c & GW_BIT_MASK;
        }

        if (len == 1) {
            add_histent_scalar(self, t, np, sbuf[0]);
        } else {
            guint8 *vector = g_malloc(len + 1);
            if (dst_len < len) {
                GwBit extend = (sbuf[0] == GW_BIT_1) ? GW_BIT_0 : sbuf[0];
                memset(vector, extend, len - dst_len);
                memcpy(vector + (len - dst_len), sbuf, dst_len);
            } else {
                memcpy(vector, sbuf, len);
            }

            vector[len] = 0;
            add_histent_vector(self, t, np, vector, len);
        }
    }

    if (len == 1) {
        add_histent_scalar(self, GW_TIME_MAX - 1, np, GW_BIT_X);
        add_histent_scalar(self, GW_TIME_MAX, np, GW_BIT_Z);
    } else {
        guint8 *x = g_malloc0(len);
        memset(x, GW_BIT_X, len);

        guint8 *z = g_malloc0(len);
        memset(z, GW_BIT_Z, len);

        add_histent_vector(self, GW_TIME_MAX - 1, np, x, len);
        add_histent_vector(self, GW_TIME_MAX, np, z, len);
    }

    g_free(sbuf);
}

static void gw_vcd_file_import_trace_real(GwVcdFile *self, GwNode *np, GwVlistReader *reader)
{
    GwTime time_scale = gw_dump_file_get_time_scale(GW_DUMP_FILE(self));
    unsigned int time_idx = 0;

    while (!gw_vlist_reader_is_done(reader)) {
        unsigned int delta;

        delta = gw_vlist_reader_read_uv32(reader);
        time_idx += delta;

        GwTime *curtime_pnt = gw_vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
        if (!curtime_pnt) {
            g_error("malformed 'r' signal data for '%s' after time_idx = %d\n",
                    np->nname,
                    time_idx - delta);
        }
        GwTime t = *curtime_pnt * time_scale;

        const gchar *str = gw_vlist_reader_read_string(reader);

        gdouble value = 0.0;
        sscanf(str, "%lg", &value);

        add_histent_real(self, t, np, value);
    }

    add_histent_real(self, GW_TIME_MAX - 1, np, 1.0);
    add_histent_real(self, GW_TIME_MAX, np, 0.0);
}

static void gw_vcd_file_import_trace_string(GwVcdFile *self, GwNode *np, GwVlistReader *reader)
{
    GwTime time_scale = gw_dump_file_get_time_scale(GW_DUMP_FILE(self));
    unsigned int time_idx = 0;

    while (!gw_vlist_reader_is_done(reader)) {
        unsigned int delta = gw_vlist_reader_read_uv32(reader);
        time_idx += delta;

        GwTime *curtime_pnt = gw_vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
        if (!curtime_pnt) {
            g_error("malformed 's' signal data for '%s' after time_idx = %d",
                    np->nname,
                    time_idx - delta);
        }
        GwTime t = *curtime_pnt * time_scale;

        const gchar *str = gw_vlist_reader_read_string(reader);
        add_histent_string(self, t, np, str);
    }

    add_histent_string(self, GW_TIME_MAX - 1, np, "UNDEF");
    add_histent_string(self, GW_TIME_MAX, np, "");
}

static void gw_vcd_file_import_trace(GwVcdFile *self, GwNode *np)
{
    guint32 len = 1;
    guint32 vlist_type;

    if (np->mv.mvlfac_vlist == NULL && np->mv.mvlfac_vlist_reader == NULL) {
        return;
    }

    gw_vlist_uncompress(&np->mv.mvlfac_vlist);

    GwVlistReader *reader = NULL;
    if (np->mv.mvlfac_vlist != NULL) {
        reader = gw_vlist_reader_new(g_steal_pointer(&np->mv.mvlfac_vlist), self->is_prepacked);
    } else if (np->mv.mvlfac_vlist_reader != NULL) {
        reader = g_object_ref(np->mv.mvlfac_vlist_reader);
    }

    if (gw_vlist_reader_is_done(reader)) {
        len = 1;
        vlist_type = '!'; /* possible alias */
    } else {
        vlist_type = gw_vlist_reader_read_uv32(reader);
        switch (vlist_type) {
            case '0': {
                len = 1;
                gint c = gw_vlist_reader_next(reader);
                if (c < 0) {
                    g_error("Internal error file '%s' line %d", __FILE__, __LINE__);
                }
                /* vartype = (unsigned int)(*chp & 0x7f); */ /*scan-build */
                break;
            }

            case 'B':
            case 'R':
            case 'S': {
                gint c = gw_vlist_reader_next(reader);
                if (c < 0) {
                    g_error("Internal error file '%s' line %d", __FILE__, __LINE__);
                }
                /* vartype = (unsigned int)(*chp & 0x7f); */ /* scan-build */

                len = gw_vlist_reader_read_uv32(reader);

                break;
            }

            default:
                g_error("Unsupported vlist type '%c'", vlist_type);
                break;
        }
    }

    if (vlist_type == '0') {
        gw_vcd_file_import_trace_scalar(self, np, reader);
    } else if (vlist_type == 'B') {
        gw_vcd_file_import_trace_vector(self, np, reader, len);
    } else if (vlist_type == 'R') {
        gw_vcd_file_import_trace_real(self, np, reader);
    } else if (vlist_type == 'S') {
        gw_vcd_file_import_trace_string(self, np, reader);
    } else if (vlist_type == '!') /* error in loading */
    {
        GwNode *n2 = (GwNode *)np->curr;

        if ((n2) &&
            (n2 != np)) /* keep out any possible infinite recursion from corrupt pointer bugs */
        {
            gw_vcd_file_import_trace(self, n2);

            g_clear_object(&reader);

            np->head = n2->head;
            np->curr = n2->curr;
            return;
        }

        g_error("Error in decompressing vlist for '%s'", np->nname);
    }

    g_clear_object(&reader);
}
