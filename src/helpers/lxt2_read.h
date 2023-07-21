/*
 * Copyright (c) 2003-2012 Tony Bybell.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef DEFS_LXTR_H
#define DEFS_LXTR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifndef _MSC_VER
#include <unistd.h>
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#else
typedef long off_t;
#include <windows.h>
#include <io.h>
#endif

#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif

#include <zlib.h>

#ifdef __GNUC__
#if __STDC_VERSION__ >= 199901L
#define _LXT2_RD_INLINE inline __attribute__((__gnu_inline__))
#else
#define _LXT2_RD_INLINE inline
#endif
#else
#define _LXT2_RD_INLINE
#endif

#define LXT2_RDLOAD "LXTLOAD | "

#define LXT2_RD_HDRID (0x1380)
#define LXT2_RD_VERSION (0x0001)

#define LXT2_RD_GRANULE_SIZE (64)
#define LXT2_RD_PARTIAL_SIZE (2048)

#define LXT2_RD_GRAN_SECT_TIME 0
#define LXT2_RD_GRAN_SECT_DICT 1
#define LXT2_RD_GRAN_SECT_TIME_PARTIAL 2

#define LXT2_RD_MAX_BLOCK_MEM_USAGE (64*1024*1024)	/* 64MB */

#ifndef _MSC_VER
typedef uint8_t 		lxtint8_t;
typedef uint16_t 		lxtint16_t;
typedef uint32_t		lxtint32_t;
typedef uint64_t	 	lxtint64_t;
typedef int32_t			lxtsint32_t;
typedef int64_t			lxtsint64_t;
#ifndef __MINGW32__
#define LXT2_RD_LLD "%"PRId64
#define LXT2_RD_LD "%"PRId32
#else
#define LXT2_RD_LLD "%I64d"
#define LXT2_RD_LD "%d"
#endif
#define LXT2_RD_LLDESC(x) x##LL
#define LXT2_RD_ULLDESC(x) x##ULL
#else
typedef unsigned __int8		lxtint8_t;
typedef unsigned __int16	lxtint16_t;
typedef unsigned __int32	lxtint32_t;
typedef unsigned __int64	lxtint64_t;
typedef          __int32	lxtsint32_t;
typedef          __int64	lxtsint64_t;
#define LXT2_RD_LLD "%I64d"
#define LXT2_RD_LD "%d"
#define LXT2_RD_LLDESC(x) x##i64
#define LXT2_RD_ULLDESC(x) x##i64
#endif

#if LXT2_RD_GRANULE_SIZE > 32
typedef lxtint64_t		granmsk_t;
typedef lxtint32_t		granmsk_smaller_t;
#define LXT2_RD_GRAN_0VAL 	(LXT2_RD_ULLDESC(0))
#define LXT2_RD_GRAN_1VAL 	(LXT2_RD_ULLDESC(1))
#define get_fac_msk 		lxt2_rd_get_64
#define get_fac_msk_smaller 	lxt2_rd_get_32
#else
typedef lxtint32_t		granmsk_t;
#define LXT2_RD_GRAN_0VAL 	(0)
#define LXT2_RD_GRAN_1VAL 	(1)
#define get_fac_msk 		lxt2_rd_get_32
#endif


#define LXT2_RD_SYM_F_BITS      (0)
#define LXT2_RD_SYM_F_INTEGER   (1<<0)
#define LXT2_RD_SYM_F_DOUBLE    (1<<1)
#define LXT2_RD_SYM_F_STRING    (1<<2)
#define LXT2_RD_SYM_F_TIME	(LXT2_RD_SYM_F_STRING)	/* user must correctly format this as a string */
#define LXT2_RD_SYM_F_ALIAS     (1<<3)

#define LXT2_RD_SYM_F_SIGNED	(1<<4)
#define LXT2_RD_SYM_F_BOOLEAN	(1<<5)
#define LXT2_RD_SYM_F_NATURAL	((1<<6)|(LXT2_RD_SYM_F_INTEGER))
#define LXT2_RD_SYM_F_POSITIVE	((1<<7)|(LXT2_RD_SYM_F_INTEGER))
#define LXT2_RD_SYM_F_CHARACTER	(1<<8)
#define LXT2_RD_SYM_F_CONSTANT	(1<<9)
#define LXT2_RD_SYM_F_VARIABLE	(1<<10)
#define LXT2_RD_SYM_F_SIGNAL	(1<<11)

#define LXT2_RD_SYM_F_IN	(1<<12)
#define LXT2_RD_SYM_F_OUT	(1<<13)
#define LXT2_RD_SYM_F_INOUT	(1<<14)

#define LXT2_RD_SYM_F_WIRE	(1<<15)
#define LXT2_RD_SYM_F_REG	(1<<16)


enum LXT2_RD_Encodings {
        LXT2_RD_ENC_0,
        LXT2_RD_ENC_1,
        LXT2_RD_ENC_INV,
        LXT2_RD_ENC_LSH0,
        LXT2_RD_ENC_LSH1,
        LXT2_RD_ENC_RSH0,
        LXT2_RD_ENC_RSH1,

        LXT2_RD_ENC_ADD1,
        LXT2_RD_ENC_ADD2,
        LXT2_RD_ENC_ADD3,
        LXT2_RD_ENC_ADD4,

        LXT2_RD_ENC_SUB1,
        LXT2_RD_ENC_SUB2,
        LXT2_RD_ENC_SUB3,
        LXT2_RD_ENC_SUB4,

        LXT2_RD_ENC_X,
        LXT2_RD_ENC_Z,

	LXT2_RD_ENC_BLACKOUT,

        LXT2_RD_DICT_START
        };


struct lxt2_rd_block
{
char *mem;
struct lxt2_rd_block *next;

lxtint32_t uncompressed_siz, compressed_siz;
lxtint64_t start, end;

lxtint32_t num_map_entries, num_dict_entries;
char *map_start;
char *dict_start;

char **string_pointers; /* based inside dict_start */
unsigned int *string_lens;

off_t filepos; /* where block starts in file if we have to reload */

unsigned short_read_ignore : 1;	 /* tried to read once and it was corrupt so ignore next time */
unsigned exclude_block : 1;	 /* user marked this block off to be ignored */
};


struct lxt2_rd_geometry
{
lxtint32_t rows;
lxtsint32_t msb, lsb;
lxtint32_t flags, len;
};


struct lxt2_rd_facname_cache
{
char *n;
char *bufprev, *bufcurr;

lxtint32_t old_facidx;
};


struct lxt2_rd_trace
{
lxtint32_t *rows;
lxtsint32_t *msb, *lsb;
lxtint32_t *flags, *len;
char **value;

granmsk_t *fac_map;
char **fac_curpos;
char *process_mask;
char *process_mask_compressed;

void **radix_sort[LXT2_RD_GRANULE_SIZE+1];
void **next_radix;

void (*value_change_callback)(struct lxt2_rd_trace **lt, lxtint64_t *time, lxtint32_t *facidx, char **value);
void *user_callback_data_pointer;

unsigned char fac_map_index_width;
unsigned char fac_curpos_width;
lxtint8_t granule_size;

lxtint32_t numfacs, numrealfacs, numfacbytes, longestname, zfacnamesize, zfacname_predec_size, zfacgeometrysize;
lxtint8_t timescale;

lxtsint64_t timezero;
lxtint64_t prev_time;
unsigned char num_time_table_entries;
lxtint64_t time_table[LXT2_RD_GRANULE_SIZE];

char *zfacnames;

unsigned int numblocks;
struct lxt2_rd_block *block_head, *block_curr;

lxtint64_t start, end;
struct lxt2_rd_geometry geometry;

struct lxt2_rd_facname_cache *faccache;

FILE *handle;
gzFile zhandle;

lxtint64_t block_mem_consumed, block_mem_max;

unsigned process_mask_dirty : 1; /* only used on partial block reads */
};


/*
 * LXT2 Reader API functions...
 */
struct lxt2_rd_trace *       	lxt2_rd_init(const char *name);
void                    	lxt2_rd_close(struct lxt2_rd_trace *lt);

lxtint64_t			lxt2_rd_set_max_block_mem_usage(struct lxt2_rd_trace *lt, lxtint64_t block_mem_max);
lxtint64_t			lxt2_rd_get_block_mem_usage(struct lxt2_rd_trace *lt);
unsigned int			lxt2_rd_get_num_blocks(struct lxt2_rd_trace *lt);
unsigned int			lxt2_rd_get_num_active_blocks(struct lxt2_rd_trace *lt);

lxtint32_t			lxt2_rd_get_num_facs(struct lxt2_rd_trace *lt);
char *				lxt2_rd_get_facname(struct lxt2_rd_trace *lt, lxtint32_t facidx);
struct lxt2_rd_geometry *	lxt2_rd_get_fac_geometry(struct lxt2_rd_trace *lt, lxtint32_t facidx);
lxtint32_t			lxt2_rd_get_fac_rows(struct lxt2_rd_trace *lt, lxtint32_t facidx);
lxtsint32_t			lxt2_rd_get_fac_msb(struct lxt2_rd_trace *lt, lxtint32_t facidx);
lxtsint32_t			lxt2_rd_get_fac_lsb(struct lxt2_rd_trace *lt, lxtint32_t facidx);
lxtint32_t			lxt2_rd_get_fac_flags(struct lxt2_rd_trace *lt, lxtint32_t facidx);
lxtint32_t			lxt2_rd_get_fac_len(struct lxt2_rd_trace *lt, lxtint32_t facidx);
lxtint32_t			lxt2_rd_get_alias_root(struct lxt2_rd_trace *lt, lxtint32_t facidx);

char				lxt2_rd_get_timescale(struct lxt2_rd_trace *lt);
lxtsint64_t 			lxt2_rd_get_timezero(struct lxt2_rd_trace *lt);
lxtint64_t			lxt2_rd_get_start_time(struct lxt2_rd_trace *lt);
lxtint64_t			lxt2_rd_get_end_time(struct lxt2_rd_trace *lt);

int				lxt2_rd_get_fac_process_mask(struct lxt2_rd_trace *lt, lxtint32_t facidx);
int				lxt2_rd_set_fac_process_mask(struct lxt2_rd_trace *lt, lxtint32_t facidx);
int				lxt2_rd_clr_fac_process_mask(struct lxt2_rd_trace *lt, lxtint32_t facidx);
int				lxt2_rd_set_fac_process_mask_all(struct lxt2_rd_trace *lt);
int				lxt2_rd_clr_fac_process_mask_all(struct lxt2_rd_trace *lt);

				/* null value_change_callback calls an empty dummy function */
int 				lxt2_rd_iter_blocks(struct lxt2_rd_trace *lt,
				void (*value_change_callback)(struct lxt2_rd_trace **lt, lxtint64_t *time, lxtint32_t *facidx, char **value),
				void *user_callback_data_pointer);
void *				lxt2_rd_get_user_callback_data_pointer(struct lxt2_rd_trace *lt);

				/* time (un)/restricted read ops */
unsigned int			lxt2_rd_limit_time_range(struct lxt2_rd_trace *lt, lxtint64_t strt_time, lxtint64_t end_time);
unsigned int			lxt2_rd_unlimit_time_range(struct lxt2_rd_trace *lt);

#ifdef __cplusplus
}
#endif

#endif

