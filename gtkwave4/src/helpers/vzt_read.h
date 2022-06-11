/*
 * Copyright (c) 2004-2012 Tony Bybell.
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

#ifndef DEFS_VZTR_H
#define DEFS_VZTR_H

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
#ifdef HAVE_INTTYPES_H
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
#if defined _MSC_VER || defined __MINGW32__
typedef int pthread_t;
typedef int pthread_attr_t;
typedef int pthread_mutex_t;
typedef int pthread_mutexattr_t;
#else
#include <pthread.h>
#endif

#include <zlib.h>
#include <bzlib.h>
#include <LzmaLib.h>

#ifdef __GNUC__
#if __STDC_VERSION__ >= 199901L
#define _VZT_RD_INLINE inline __attribute__((__gnu_inline__))
#else
#define _VZT_RD_INLINE inline
#endif
#else
#define _VZT_RD_INLINE
#endif

#define VZT_RDLOAD "VZTLOAD | "

#define VZT_RD_HDRID (('V' << 8) + ('Z'))
#define VZT_RD_VERSION (0x0001)

#define VZT_RD_GRANULE_SIZE (32)

#define VZT_RD_MAX_BLOCK_MEM_USAGE (64*1024*1024)	/* 64MB */

#ifndef _MSC_VER
typedef uint8_t 		vztint8_t;
typedef uint16_t 		vztint16_t;
typedef uint32_t		vztint32_t;
typedef uint64_t	 	vztint64_t;
typedef int64_t			vztsint64_t;
typedef int32_t			vztsint32_t;
#ifndef __MINGW32__
#define VZT_RD_LLD "%"PRId64
#define VZT_RD_LD "%"PRId32
#else
#define VZT_RD_LLD "%I64d"
#define VZT_RD_LD "%d"
#endif
#define VZT_RD_LLDESC(x) x##LL
#define VZT_RD_ULLDESC(x) x##ULL
#else
typedef unsigned __int8		vztint8_t;
typedef unsigned __int16	vztint16_t;
typedef unsigned __int32	vztint32_t;
typedef unsigned __int64	vztint64_t;
typedef          __int64	vztsint64_t;
typedef          __int32	vztsint32_t;
#define VZT_RD_LLD "%I64d"
#define VZT_RD_LD "%d"
#define VZT_RD_LLDESC(x) x##i64
#define VZT_RD_ULLDESC(x) x##i64
#endif


#define VZT_RD_IS_GZ              (0)
#define VZT_RD_IS_BZ2             (1)
#define VZT_RD_IS_LZMA            (2)

#define VZT_RD_SYM_F_BITS      	(0)
#define VZT_RD_SYM_F_INTEGER   	(1<<0)
#define VZT_RD_SYM_F_DOUBLE    	(1<<1)
#define VZT_RD_SYM_F_STRING    	(1<<2)
#define VZT_RD_SYM_F_TIME	(VZT_RD_SYM_F_STRING)	/* user must correctly format this as a string */
#define VZT_RD_SYM_F_ALIAS     	(1<<3)

#define VZT_RD_SYM_F_SIGNED	(1<<4)
#define VZT_RD_SYM_F_BOOLEAN	(1<<5)
#define VZT_RD_SYM_F_NATURAL	((1<<6)|(VZT_RD_SYM_F_INTEGER))
#define VZT_RD_SYM_F_POSITIVE	((1<<7)|(VZT_RD_SYM_F_INTEGER))
#define VZT_RD_SYM_F_CHARACTER	(1<<8)
#define VZT_RD_SYM_F_CONSTANT	(1<<9)
#define VZT_RD_SYM_F_VARIABLE	(1<<10)
#define VZT_RD_SYM_F_SIGNAL	(1<<11)

#define VZT_RD_SYM_F_IN		(1<<12)
#define VZT_RD_SYM_F_OUT	(1<<13)
#define VZT_RD_SYM_F_INOUT	(1<<14)

#define VZT_RD_SYM_F_WIRE	(1<<15)
#define VZT_RD_SYM_F_REG	(1<<16)

#define VZT_RD_SYM_MASK		(VZT_RD_SYM_F_BITS|VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING|VZT_RD_SYM_F_TIME| \
				VZT_RD_SYM_F_ALIAS|VZT_RD_SYM_F_SIGNED|VZT_RD_SYM_F_BOOLEAN|VZT_RD_SYM_F_NATURAL| \
				VZT_RD_SYM_F_POSITIVE|VZT_RD_SYM_F_CHARACTER|VZT_RD_SYM_F_CONSTANT|VZT_RD_SYM_F_VARIABLE| \
				VZT_RD_SYM_F_SIGNAL|VZT_RD_SYM_F_IN|VZT_RD_SYM_F_OUT|VZT_RD_SYM_F_INOUT|VZT_RD_SYM_F_WIRE| \
				VZT_RD_SYM_F_REG)

#define VZT_RD_SYM_F_SYNVEC	(1<<17)	/* reader synthesized vector in alias sec'n from non-adjacent vectorizing */

struct vzt_rd_block
{
char *mem;
struct vzt_rd_block *next;
struct vzt_rd_block *prev;

vztint32_t uncompressed_siz, compressed_siz, num_rle_bytes;
vztint64_t start, end;

vztint32_t *vindex;
vztint64_t *times;
vztint32_t *change_dict;
vztint32_t *val_dict;
char **sindex;
unsigned int num_time_ticks, num_sections, num_dict_entries, num_str_entries;

off_t filepos; /* where block starts in file if we have to reload */

unsigned short_read_ignore : 1;	/* tried to read once and it was corrupt so ignore next time */
unsigned exclude_block : 1;	/* user marked this block off to be ignored */
unsigned multi_state : 1;	/* not just two state value changes */
unsigned killed : 1;		/* we're in vzt_close(), don't grab anymore blocks */
unsigned ztype : 2;		/* 1: gzip, 0: bzip2, 2: lzma */
unsigned rle : 1;		/* set when end < start which says that an rle depack is necessary */

pthread_t pth;
pthread_attr_t pth_attr;
pthread_mutex_t mutex;

vztint64_t last_rd_value_simtime;
vztint32_t last_rd_value_idx;
};


struct vzt_rd_geometry
{
vztint32_t rows;
vztsint32_t msb, lsb;
vztint32_t flags, len;
};


struct vzt_rd_facname_cache
{
char *n;
char *bufprev, *bufcurr;

vztint32_t old_facidx;
};


struct vzt_rd_trace
{
vztint32_t *rows;
vztsint32_t *msb, *lsb;
vztint32_t *flags, *len, *vindex_offset;
vztsint64_t timezero;

char *value_current_sector;
char *value_previous_sector;
vztint32_t longest_len;

vztint32_t total_values; /* number of value index entries in table */

char *process_mask;

void (*value_change_callback)(struct vzt_rd_trace **lt, vztint64_t *time, vztint32_t *facidx, char **value);
void *user_callback_data_pointer;

vztint8_t granule_size;

vztint32_t numfacs, numrealfacs, numfacbytes, longestname, zfacnamesize, zfacname_predec_size, zfacgeometrysize;
vztint8_t timescale;

char *zfacnames;

unsigned int numblocks;
struct vzt_rd_block *block_head, *block_curr;

vztint64_t start, end;
struct vzt_rd_geometry geometry;

struct vzt_rd_facname_cache *faccache;

vztint64_t last_rd_value_simtime;		/* for single value reads w/o using the callback mechanism */
struct vzt_rd_block *last_rd_value_block;

char *filename;			/* for multithread */
FILE *handle;
void *zhandle;

vztint64_t block_mem_consumed, block_mem_max;
pthread_mutex_t mutex;	/* for these */

unsigned int pthreads;			/* pthreads are enabled, set to max processor # (starting at zero for a uni) */
unsigned process_linear : 1;	/* set by gtkwave for read optimization */
unsigned vectorize : 1;		/* set when coalescing blasted bitvectors */
};


/*
 * VZT Reader API functions...
 */
struct vzt_rd_trace *       	vzt_rd_init(const char *name);
struct vzt_rd_trace *       	vzt_rd_init_smp(const char *name, unsigned int num_cpus);
void                    	vzt_rd_close(struct vzt_rd_trace *lt);

vztint64_t			vzt_rd_set_max_block_mem_usage(struct vzt_rd_trace *lt, vztint64_t block_mem_max);
vztint64_t			vzt_rd_get_block_mem_usage(struct vzt_rd_trace *lt);
unsigned int			vzt_rd_get_num_blocks(struct vzt_rd_trace *lt);
unsigned int			vzt_rd_get_num_active_blocks(struct vzt_rd_trace *lt);

vztint32_t			vzt_rd_get_num_facs(struct vzt_rd_trace *lt);
char *				vzt_rd_get_facname(struct vzt_rd_trace *lt, vztint32_t facidx);
struct vzt_rd_geometry *	vzt_rd_get_fac_geometry(struct vzt_rd_trace *lt, vztint32_t facidx);
vztint32_t			vzt_rd_get_fac_rows(struct vzt_rd_trace *lt, vztint32_t facidx);
vztsint32_t			vzt_rd_get_fac_msb(struct vzt_rd_trace *lt, vztint32_t facidx);
vztsint32_t			vzt_rd_get_fac_lsb(struct vzt_rd_trace *lt, vztint32_t facidx);
vztint32_t			vzt_rd_get_fac_flags(struct vzt_rd_trace *lt, vztint32_t facidx);
vztint32_t			vzt_rd_get_fac_len(struct vzt_rd_trace *lt, vztint32_t facidx);
vztint32_t			vzt_rd_get_alias_root(struct vzt_rd_trace *lt, vztint32_t facidx);

char				vzt_rd_get_timescale(struct vzt_rd_trace *lt);
vztint64_t			vzt_rd_get_start_time(struct vzt_rd_trace *lt);
vztint64_t			vzt_rd_get_end_time(struct vzt_rd_trace *lt);
vztsint64_t		 	vzt_rd_get_timezero(struct vzt_rd_trace *lt);

int				vzt_rd_get_fac_process_mask(struct vzt_rd_trace *lt, vztint32_t facidx);
int				vzt_rd_set_fac_process_mask(struct vzt_rd_trace *lt, vztint32_t facidx);
int				vzt_rd_clr_fac_process_mask(struct vzt_rd_trace *lt, vztint32_t facidx);
int				vzt_rd_set_fac_process_mask_all(struct vzt_rd_trace *lt);
int				vzt_rd_clr_fac_process_mask_all(struct vzt_rd_trace *lt);

				/* null value_change_callback calls an empty dummy function */
int 				vzt_rd_iter_blocks(struct vzt_rd_trace *lt,
				void (*value_change_callback)(struct vzt_rd_trace **lt, vztint64_t *time, vztint32_t *facidx, char **value),
				void *user_callback_data_pointer);
void *				vzt_rd_get_user_callback_data_pointer(struct vzt_rd_trace *lt);
void 				vzt_rd_process_blocks_linearly(struct vzt_rd_trace *lt, int doit);

				/* time (un)/restricted read ops */
unsigned int			vzt_rd_limit_time_range(struct vzt_rd_trace *lt, vztint64_t strt_time, vztint64_t end_time);
unsigned int			vzt_rd_unlimit_time_range(struct vzt_rd_trace *lt);

				/* naive read on time/facidx */
char *				vzt_rd_value(struct vzt_rd_trace *lt, vztint64_t simtime, vztint32_t facidx);

				/* experimental function for reconstituting bitblasted nets */
				void vzt_rd_vectorize(struct vzt_rd_trace *lt);

#ifdef __cplusplus
}
#endif

#endif

