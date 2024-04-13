/*
 * Copyright (c) Tony Bybell 2006-8.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <gtkwave.h>

#ifndef WAVE_VLIST_H
#define WAVE_VLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

/* experimentation shows that 255 is one of the least common
   bytes found in recoded value change streams */
#define WAVE_ZIVFLAG (0xff)

#define WAVE_ZIVWRAP (1 << 7) /* must be power of two because of AND mask */
#define WAVE_ZIVSRCH (WAVE_ZIVWRAP) /* search depth in bytes */
#define WAVE_ZIVSKIP (1) /* number of bytes to skip for alternate rollover searches */
#define WAVE_ZIVMASK ((WAVE_ZIVWRAP)-1) /* then this becomes an AND mask for wrapping */

struct vlist_packer_t
{
    GwVlist *v;

    unsigned char buf[WAVE_ZIVWRAP];

#ifdef WAVE_VLIST_PACKER_STATS
    unsigned int packed_bytes;
#endif
    unsigned int unpacked_bytes;
    unsigned int repcnt, repcnt2, repcnt3, repcnt4;

    unsigned char bufpnt;
    unsigned char repdist, repdist2, repdist3, repdist4;
};

struct vlist_packer_t *vlist_packer_create(void);
void vlist_packer_alloc(struct vlist_packer_t *v, unsigned char ch);
void vlist_packer_finalize(struct vlist_packer_t *v);
unsigned char *vlist_packer_decompress(GwVlist *vl, unsigned int *declen);
void vlist_packer_decompress_destroy(char *mem);

#endif
