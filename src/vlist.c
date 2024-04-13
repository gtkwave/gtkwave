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

    pnt = gw_vlist_alloc(&p->v, TRUE, GLOBALS->vlist_compression_depth);
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
    vp->v = gw_vlist_create(sizeof(char));

    return (vp);
}

unsigned char *vlist_packer_decompress(GwVlist *v, unsigned int *declen)
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

    mem = calloc_2(1, WAVE_ZIVWRAP + dec_size);
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
