#include "gw-vlist-packer.h"

/* experimentation shows that 255 is one of the least common
   bytes found in recoded value change streams */
#define WAVE_ZIVFLAG (0xff)

#define WAVE_ZIVWRAP (1 << 7) /* must be power of two because of AND mask */
#define WAVE_ZIVSRCH (WAVE_ZIVWRAP) /* search depth in bytes */
#define WAVE_ZIVSKIP (1) /* number of bytes to skip for alternate rollover searches */
#define WAVE_ZIVMASK ((WAVE_ZIVWRAP)-1) /* then this becomes an AND mask for wrapping */

struct _GwVlistPacker
{
    GwVlist *v;

    gint compression_level;

    unsigned char buf[WAVE_ZIVWRAP];

#ifdef WAVE_VLIST_PACKER_STATS
    guint packed_bytes;
#endif
    guint unpacked_bytes;
    guint repcnt, repcnt2, repcnt3, repcnt4;

    guchar bufpnt;
    guchar repdist, repdist2, repdist3, repdist4;
};

/* this code implements an LZ-based filter that can sit on top
   of the vlists.  it uses a generic escape value of 0xff as
   that is one that statistically occurs infrequently in value
   change data fed into vlists.
 */

static void gw_vlist_packer_emit_out(GwVlistPacker *self, unsigned char byt)
{
    char *pnt;

#ifdef WAVE_VLIST_PACKER_STATS
    p->packed_bytes++;
#endif

    pnt = gw_vlist_alloc(&self->v, TRUE, self->compression_level);
    *pnt = byt;
}

void gw_vlist_packer_emit_uv32(GwVlistPacker *self, unsigned int v)
{
    unsigned int nxt;

    while ((nxt = v >> 7)) {
        gw_vlist_packer_emit_out(self, v & 0x7f);
        v = nxt;
    }

    gw_vlist_packer_emit_out(self, (v & 0x7f) | 0x80);
}

static void gw_vlist_packer_emit_uv32rvs(GwVlistPacker *self, unsigned int v)
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
        gw_vlist_packer_emit_out(self, buf[i]);
    }
}

void gw_vlist_packer_alloc(GwVlistPacker *self, unsigned char byt)
{
    int i, j, k, l;

    self->unpacked_bytes++;

    if (!self->repcnt) {
    top:
        for (i = 0; i < WAVE_ZIVSRCH; i++) {
            if (self->buf[(self->bufpnt - i) & WAVE_ZIVMASK] == byt) {
                self->repdist = i;
                self->repcnt = 1;

                self->repdist2 = self->repdist3 = self->repdist4 = 0;

                for (j = i + WAVE_ZIVSKIP; j < WAVE_ZIVSRCH; j++) {
                    if (self->buf[(self->bufpnt - j) & WAVE_ZIVMASK] == byt) {
                        self->repdist2 = j;
                        self->repcnt2 = 1;

                        for (k = j + WAVE_ZIVSKIP; k < WAVE_ZIVSRCH; k++) {
                            if (self->buf[(self->bufpnt - k) & WAVE_ZIVMASK] == byt) {
                                self->repdist3 = k;
                                self->repcnt3 = 1;

                                for (l = k + WAVE_ZIVSKIP; l < WAVE_ZIVSRCH; l++) {
                                    if (self->buf[(self->bufpnt - l) & WAVE_ZIVMASK] == byt) {
                                        self->repdist4 = l;
                                        self->repcnt4 = 1;
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }

                self->bufpnt++;
                self->bufpnt &= WAVE_ZIVMASK;
                self->buf[self->bufpnt] = byt;

                return;
            }
        }

        self->bufpnt++;
        self->bufpnt &= WAVE_ZIVMASK;
        self->buf[self->bufpnt] = byt;
        gw_vlist_packer_emit_out(self, byt);
        if (byt == WAVE_ZIVFLAG) {
            gw_vlist_packer_emit_uv32(self, 0);
        }
    } else {
    attempt2:
        if (self->buf[(self->bufpnt - self->repdist) & WAVE_ZIVMASK] == byt) {
            self->repcnt++;

            if (self->repcnt2) {
                self->repcnt2 = ((self->buf[(self->bufpnt - self->repdist2) & WAVE_ZIVMASK] == byt))
                                    ? self->repcnt2 + 1
                                    : 0;
            }
            if (self->repcnt3) {
                self->repcnt3 = ((self->buf[(self->bufpnt - self->repdist3) & WAVE_ZIVMASK] == byt))
                                    ? self->repcnt3 + 1
                                    : 0;
            }
            if (self->repcnt4) {
                self->repcnt4 = ((self->buf[(self->bufpnt - self->repdist4) & WAVE_ZIVMASK] == byt))
                                    ? self->repcnt4 + 1
                                    : 0;
            }

            self->bufpnt++;
            self->bufpnt &= WAVE_ZIVMASK;
            self->buf[self->bufpnt] = byt;
        } else {
            if (self->repcnt2) {
                self->repcnt = self->repcnt2;
                self->repdist = self->repdist2;

                self->repcnt2 = self->repcnt3;
                self->repdist2 = self->repdist3;

                self->repcnt3 = self->repcnt4;
                self->repdist3 = self->repdist4;

                self->repcnt4 = 0;
                self->repdist4 = 0;

                goto attempt2;
            }

            if (self->repcnt3) {
                self->repcnt = self->repcnt3;
                self->repdist = self->repdist3;

                self->repcnt2 = self->repcnt4;
                self->repdist2 = self->repdist4;

                self->repcnt3 = 0;
                self->repdist3 = 0;
                self->repcnt4 = 0;
                self->repdist4 = 0;

                goto attempt2;
            }

            if (self->repcnt4) {
                self->repcnt = self->repcnt4;
                self->repdist = self->repdist4;

                self->repcnt4 = 0;
                self->repdist4 = 0;

                goto attempt2;
            }

            if (self->repcnt > 2) {
                gw_vlist_packer_emit_out(self, WAVE_ZIVFLAG);
                gw_vlist_packer_emit_uv32(self, self->repcnt);
                self->repcnt = 0;
                gw_vlist_packer_emit_uv32(self, self->repdist);
            } else {
                if (self->repcnt == 2) {
                    gw_vlist_packer_emit_out(self, self->buf[(self->bufpnt - 1) & WAVE_ZIVMASK]);
                    if (self->buf[(self->bufpnt - 1) & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                        gw_vlist_packer_emit_uv32(self, 0);
                    }
                }

                gw_vlist_packer_emit_out(self, self->buf[self->bufpnt & WAVE_ZIVMASK]);
                self->repcnt = 0;
                if (self->buf[self->bufpnt & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                    gw_vlist_packer_emit_uv32(self, 0);
                }
            }
            goto top;
        }
    }
}

GwVlist *gw_vlist_packer_finalize_and_free(GwVlistPacker *self)
{
#ifdef WAVE_VLIST_PACKER_STATS
    static guint64 pp = 0, upp = 0;
#endif

    if (self->repcnt) {
        if (self->repcnt > 2) {
            gw_vlist_packer_emit_out(self, WAVE_ZIVFLAG);
            gw_vlist_packer_emit_uv32(self, self->repcnt);
            self->repcnt = 0;
            gw_vlist_packer_emit_uv32(self, self->repdist);
        } else {
            if (self->repcnt == 2) {
                gw_vlist_packer_emit_out(self, self->buf[(self->bufpnt - 1) & WAVE_ZIVMASK]);
                if (self->buf[(self->bufpnt - 1) & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                    gw_vlist_packer_emit_uv32(self, 0);
                }
            }

            gw_vlist_packer_emit_out(self, self->buf[self->bufpnt & WAVE_ZIVMASK]);
            self->repcnt = 0;
            if (self->buf[self->bufpnt & WAVE_ZIVMASK] == WAVE_ZIVFLAG) {
                gw_vlist_packer_emit_uv32(self, 0);
            }
        }
    }

    gw_vlist_packer_emit_uv32rvs(self,
                                 self->unpacked_bytes); /* for malloc later during decompress */

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

    GwVlist *ret = self->v;
    g_free(self);
    return ret;
}

GwVlistPacker *gw_vlist_packer_new(gint compression_level)
{
    GwVlistPacker *self = g_new0(GwVlistPacker, 1);
    self->v = gw_vlist_create(sizeof(char));
    self->compression_level = compression_level;

    return self;
}

unsigned char *gw_vlist_packer_decompress(GwVlist *v, unsigned int *declen)
{
    unsigned int list_size = gw_vlist_size(v);
    unsigned int top_of_packed_size = list_size - 1;
    unsigned char *chp;
    unsigned int dec_size = 0;
    unsigned int dec_size_cmp;
    unsigned int shamt = 0;
    unsigned char *mem, *dpnt;
    unsigned int i, j, repcnt, dist;

    for (;;) {
        chp = gw_vlist_locate(v, top_of_packed_size);

        dec_size |= ((unsigned int)(*chp & 0x7f)) << shamt;

        if (*chp & 0x80) {
            break;
        }

        shamt += 7;
        top_of_packed_size--;
    }

    mem = g_malloc0(WAVE_ZIVWRAP + dec_size);
    dpnt = mem + WAVE_ZIVWRAP;
    for (i = 0; i < top_of_packed_size; i++) {
        chp = gw_vlist_locate(v, i);
        if (*chp != WAVE_ZIVFLAG) {
            *(dpnt++) = *chp;
            continue;
        }

        i++;
        repcnt = shamt = 0;
        for (;;) {
            chp = gw_vlist_locate(v, i);

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
            chp = gw_vlist_locate(v, i);

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
        g_error("miscompare: decompressed vlist_packer length: %d vs %d bytes",
                dec_size,
                dec_size_cmp);
    }

    return (mem + WAVE_ZIVWRAP);
}

void gw_vlist_packer_decompress_destroy(char *mem)
{
    g_free(mem - WAVE_ZIVWRAP);
}
