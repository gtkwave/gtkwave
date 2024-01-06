#include <config.h>
#include "gw-vcd-file.h"
#include "gw-vcd-file-private.h"
#include "analyzer.h"

G_DEFINE_TYPE(GwVcdFile, gw_vcd_file, GW_TYPE_DUMP_FILE)

static void gw_vcd_file_dispose(GObject *object)
{
    GwVcdFile *self = GW_VCD_FILE(object);

    g_clear_object(&self->hist_ent_factory);

    G_OBJECT_CLASS(gw_vcd_file_parent_class)->dispose(object);
}

static void gw_vcd_file_class_init(GwVcdFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->dispose = gw_vcd_file_dispose;
}

static void gw_vcd_file_init(GwVcdFile *self)
{
    self->hist_ent_factory = gw_hist_ent_factory_new();
}

void gw_vcd_file_import_masked(GwVcdFile *self)
{
    /* nothing */
    (void)self;
}

void gw_vcd_file_set_fac_process_mask(GwVcdFile *self, GwNode *np)
{
    if (np && np->mv.mvlfac_vlist) {
        gw_vcd_file_import_trace(self, np);
    }
}

static void add_histent(GwVcdFile *self, GwTime tim, GwNode *n, char ch, int regadd, char *vector)
{
    GwHistEnt *he;
    char heval;

    GwTime time_scale = gw_dump_file_get_time_scale(GW_DUMP_FILE(self));

    if (!vector) {
        if (!n->curr) {
            he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            he->time = -1;
            he->v.h_val = GW_BIT_X;

            n->curr = he;
            n->head.next = he;

            add_histent(self, tim, n, ch, regadd, vector);
        } else {
            if (regadd) {
                tim *= time_scale;
            }

            if (ch == '0')
                heval = GW_BIT_0;
            else if (ch == '1')
                heval = GW_BIT_1;
            else if ((ch == 'x') || (ch == 'X'))
                heval = GW_BIT_X;
            else if ((ch == 'z') || (ch == 'Z'))
                heval = GW_BIT_Z;
            else if ((ch == 'h') || (ch == 'H'))
                heval = GW_BIT_H;
            else if ((ch == 'u') || (ch == 'U'))
                heval = GW_BIT_U;
            else if ((ch == 'w') || (ch == 'W'))
                heval = GW_BIT_W;
            else if ((ch == 'l') || (ch == 'L'))
                heval = GW_BIT_L;
            else
                /* if(ch=='-') */ heval = GW_BIT_DASH; /* default */

            if ((n->curr->v.h_val != heval) || (tim == self->start_time) ||
                (n->vartype == GW_VAR_TYPE_VCD_EVENT) ||
                (self->preserve_glitches)) /* same region == go skip */
            {
                if (n->curr->time == tim) {
                    DEBUG(printf("Warning: Glitch at time [%" GW_TIME_FORMAT
                                 "] Signal [%p], Value [%c->%c].\n",
                                 tim,
                                 n,
                                 gw_bit_to_char(n->curr->v.h_val),
                                 ch));
                    n->curr->v.h_val = heval; /* we have a glitch! */

                    if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                        n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                    }
                } else {
                    he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                    he->time = tim;
                    he->v.h_val = heval;

                    n->curr->next = he;
                    if (n->curr->v.h_val == heval) {
                        n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                    }
                    n->curr = he;
                }
            }
        }
    } else {
        switch (ch) {
            case 's': /* string */
            {
                if (!n->curr) {
                    he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                    he->flags = (GW_HIST_ENT_FLAG_STRING | GW_HIST_ENT_FLAG_REAL);
                    he->time = -1;
                    he->v.h_vector = NULL;

                    n->curr = he;
                    n->head.next = he;

                    add_histent(self, tim, n, ch, regadd, vector);
                } else {
                    if (regadd) {
                        tim *= time_scale;
                    }

                    if (n->curr->time == tim) {
                        DEBUG(printf("Warning: String Glitch at time [%" GW_TIME_FORMAT
                                     "] Signal [%p].\n",
                                     tim,
                                     n));
                        if (n->curr->v.h_vector)
                            free_2(n->curr->v.h_vector);
                        n->curr->v.h_vector = vector; /* we have a glitch! */

                        if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                            n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                        }
                    } else {
                        he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                        he->flags = (GW_HIST_ENT_FLAG_STRING | GW_HIST_ENT_FLAG_REAL);
                        he->time = tim;
                        he->v.h_vector = vector;

                        n->curr->next = he;
                        n->curr = he;
                    }
                }
                break;
            }
            case 'g': /* real number */
            {
                if (!n->curr) {
                    he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                    he->flags = GW_HIST_ENT_FLAG_REAL;
                    he->time = -1;
                    he->v.h_double = strtod("NaN", NULL);

                    n->curr = he;
                    n->head.next = he;

                    add_histent(self, tim, n, ch, regadd, vector);
                } else {
                    if (regadd) {
                        tim *= time_scale;
                    }

                    if ((vector && (n->curr->v.h_double != *(double *)vector)) ||
                        (tim == self->start_time) || (self->preserve_glitches) ||
                        (self->preserve_glitches_real)) /* same region == go skip */
                    {
                        if (n->curr->time == tim) {
                            DEBUG(printf("Warning: Real number Glitch at time [%" GW_TIME_FORMAT
                                         "] Signal [%p].\n",
                                         tim,
                                         n));
                            n->curr->v.h_double = *((double *)vector);
                            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                            }
                        } else {
                            he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                            he->flags = GW_HIST_ENT_FLAG_REAL;
                            he->time = tim;
                            he->v.h_double = *((double *)vector);
                            n->curr->next = he;
                            n->curr = he;
                        }
                    } else {
                    }
                    free_2(vector);
                }
                break;
            }
            default: {
                if (!n->curr) {
                    he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                    he->time = -1;
                    he->v.h_vector = NULL;

                    n->curr = he;
                    n->head.next = he;

                    add_histent(self, tim, n, ch, regadd, vector);
                } else {
                    if (regadd) {
                        tim *= time_scale;
                    }

                    if ((n->curr->v.h_vector && vector && (strcmp(n->curr->v.h_vector, vector))) ||
                        (tim == self->start_time) || (!n->curr->v.h_vector) ||
                        (self->preserve_glitches)) /* same region == go skip */
                    {
                        if (n->curr->time == tim) {
                            DEBUG(printf("Warning: Glitch at time [%" GW_TIME_FORMAT
                                         "] Signal [%p], Value [%c->%c].\n",
                                         tim,
                                         n,
                                         gw_bit_to_char(n->curr->v.h_val),
                                         ch));
                            if (n->curr->v.h_vector)
                                free_2(n->curr->v.h_vector);
                            n->curr->v.h_vector = vector; /* we have a glitch! */

                            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                            }
                        } else {
                            he = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                            he->time = tim;
                            he->v.h_vector = vector;

                            n->curr->next = he;
                            n->curr = he;
                        }
                    } else {
                        free_2(vector);
                    }
                }
                break;
            }
        }
    }
}

#define vlist_locate_import(self, x, y) \
    ((self->is_prepacked) ? ((depacked) + (y)) : vlist_locate((x), (y)))

void gw_vcd_file_import_trace(GwVcdFile *self, GwNode *np)
{
    struct vlist_t *v = np->mv.mvlfac_vlist;
    int len = 1;
    unsigned int list_size;
    unsigned char vlist_type;
    /* unsigned int vartype = 0; */ /* scan-build */
    unsigned int vlist_pos = 0;
    unsigned char *chp;
    unsigned int time_idx = 0;
    GwTime *curtime_pnt;
    unsigned char arr[5];
    int arr_pos;
    unsigned int accum;
    unsigned char ch;
    double *d;
    unsigned char *depacked = NULL;

    if (!v)
        return;
    vlist_uncompress(&v);

    if (self->is_prepacked) {
        depacked = vlist_packer_decompress(v, &list_size);
        vlist_destroy(v);
    } else {
        list_size = vlist_size(v);
    }

    if (!list_size) {
        len = 1;
        vlist_type = '!'; /* possible alias */
    } else {
        chp = vlist_locate_import(self, v, vlist_pos++);
        if (chp) {
            switch ((vlist_type = (*chp & 0x7f))) {
                case '0':
                    len = 1;
                    chp = vlist_locate_import(self, v, vlist_pos++);
                    if (!chp) {
                        fprintf(stderr,
                                "Internal error file '%s' line %d, exiting.\n",
                                __FILE__,
                                __LINE__);
                        exit(255);
                    }
                    /* vartype = (unsigned int)(*chp & 0x7f); */ /*scan-build */
                    break;

                case 'B':
                case 'R':
                case 'S':
                    chp = vlist_locate_import(self, v, vlist_pos++);
                    if (!chp) {
                        fprintf(stderr,
                                "Internal error file '%s' line %d, exiting.\n",
                                __FILE__,
                                __LINE__);
                        exit(255);
                    }
                    /* vartype = (unsigned int)(*chp & 0x7f); */ /* scan-build */

                    arr_pos = accum = 0;

                    do {
                        chp = vlist_locate_import(self, v, vlist_pos++);
                        if (!chp)
                            break;
                        ch = *chp;
                        arr[arr_pos++] = ch;
                    } while (!(ch & 0x80));

                    for (--arr_pos; arr_pos >= 0; arr_pos--) {
                        ch = arr[arr_pos];
                        accum <<= 7;
                        accum |= (unsigned int)(ch & 0x7f);
                    }

                    len = accum;

                    break;

                default:
                    fprintf(stderr, "Unsupported vlist type '%c', exiting.", vlist_type);
                    vcd_exit(255);
                    break;
            }
        } else {
            len = 1;
            vlist_type = '!'; /* possible alias */
        }
    }

    if (vlist_type == '0') /* single bit */
    {
        while (vlist_pos < list_size) {
            unsigned int delta, bitval;
            char ascval;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            if (!(accum & 1)) {
                delta = accum >> 2;
                bitval = (accum >> 1) & 1;
                ascval = '0' + bitval;
            } else {
                delta = accum >> 4;
                bitval = (accum >> 1) & 7;
                ascval = RCV_STR[bitval];
            }
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed bitwise signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            add_histent(self, *curtime_pnt, np, ascval, 1, NULL);
        }

        add_histent(self, MAX_HISTENT_TIME - 1, np, 'x', 0, NULL);
        add_histent(self, MAX_HISTENT_TIME, np, 'z', 0, NULL);
    } else if (vlist_type == 'B') /* bit vector, port type was converted to bit vector already */
    {
        char *sbuf = malloc_2(len + 1);
        int dst_len;
        char *vector;

        while (vlist_pos < list_size) {
            unsigned int delta;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            delta = accum;
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed 'b' signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            dst_len = 0;
            for (;;) {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                if ((ch >> 4) == GW_BIT_MASK)
                    break;
                if (dst_len == len) {
                    if (len != 1)
                        memmove(sbuf, sbuf + 1, dst_len - 1);
                    dst_len--;
                }
                sbuf[dst_len++] = gw_bit_to_char(ch >> 4);
                if ((ch & GW_BIT_MASK) == GW_BIT_MASK)
                    break;
                if (dst_len == len) {
                    if (len != 1)
                        memmove(sbuf, sbuf + 1, dst_len - 1);
                    dst_len--;
                }
                sbuf[dst_len++] = gw_bit_to_char(ch & GW_BIT_MASK);
            }

            if (len == 1) {
                add_histent(self, *curtime_pnt, np, sbuf[0], 1, NULL);
            } else {
                vector = malloc_2(len + 1);
                if (dst_len < len) {
                    unsigned char extend = (sbuf[0] == '1') ? '0' : sbuf[0];
                    memset(vector, extend, len - dst_len);
                    memcpy(vector + (len - dst_len), sbuf, dst_len);
                } else {
                    memcpy(vector, sbuf, len);
                }

                vector[len] = 0;
                add_histent(self, *curtime_pnt, np, 0, 1, vector);
            }
        }

        if (len == 1) {
            add_histent(self, MAX_HISTENT_TIME - 1, np, 'x', 0, NULL);
            add_histent(self, MAX_HISTENT_TIME, np, 'z', 0, NULL);
        } else {
            add_histent(self, MAX_HISTENT_TIME - 1, np, 'x', 0, (char *)calloc_2(1, sizeof(char)));
            add_histent(self, MAX_HISTENT_TIME, np, 'z', 0, (char *)calloc_2(1, sizeof(char)));
        }

        free_2(sbuf);
    } else if (vlist_type == 'R') /* real */
    {
        char *sbuf = malloc_2(64);
        int dst_len;
        char *vector;

        while (vlist_pos < list_size) {
            unsigned int delta;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            delta = accum;
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed 'r' signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            dst_len = 0;
            do {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                sbuf[dst_len++] = ch;
            } while (ch);

            vector = malloc_2(sizeof(double));
            sscanf(sbuf, "%lg", (double *)vector);
            add_histent(self, *curtime_pnt, np, 'g', 1, (char *)vector);
        }

        d = malloc_2(sizeof(double));
        *d = 1.0;
        add_histent(self, MAX_HISTENT_TIME - 1, np, 'g', 0, (char *)d);

        d = malloc_2(sizeof(double));
        *d = 0.0;
        add_histent(self, MAX_HISTENT_TIME, np, 'g', 0, (char *)d);

        free_2(sbuf);
    } else if (vlist_type == 'S') /* string */
    {
        char *sbuf = malloc_2(list_size); /* being conservative */
        int dst_len;
        char *vector;

        while (vlist_pos < list_size) {
            unsigned int delta;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            delta = accum;
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed 's' signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            dst_len = 0;
            do {
                chp = vlist_locate_import(self, v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                sbuf[dst_len++] = ch;
            } while (ch);

            vector = malloc_2(dst_len + 1);
            strcpy(vector, sbuf);
            add_histent(self, *curtime_pnt, np, 's', 1, (char *)vector);
        }

        d = malloc_2(sizeof(double));
        *d = 1.0;
        add_histent(self, MAX_HISTENT_TIME - 1, np, 'g', 0, (char *)d);

        d = malloc_2(sizeof(double));
        *d = 0.0;
        add_histent(self, MAX_HISTENT_TIME, np, 'g', 0, (char *)d);

        free_2(sbuf);
    } else if (vlist_type == '!') /* error in loading */
    {
        GwNode *n2 = (GwNode *)np->curr;

        if ((n2) &&
            (n2 != np)) /* keep out any possible infinite recursion from corrupt pointer bugs */
        {
            gw_vcd_file_import_trace(self, n2);

            if (self->is_prepacked) {
                vlist_packer_decompress_destroy((char *)depacked);
            } else {
                vlist_destroy(v);
            }
            np->mv.mvlfac_vlist = NULL;

            np->head = n2->head;
            np->curr = n2->curr;
            return;
        }

        fprintf(stderr, "Error in decompressing vlist for '%s', exiting.\n", np->nname);
        vcd_exit(255);
    }

    if (self->is_prepacked) {
        vlist_packer_decompress_destroy((char *)depacked);
    } else {
        vlist_destroy(v);
    }
    np->mv.mvlfac_vlist = NULL;
}
