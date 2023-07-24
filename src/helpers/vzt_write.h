/*
 * Copyright (c) 2004-2010 Tony Bybell.
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

#ifndef DEFS_VZTW_H
#define DEFS_VZTW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <zlib.h>
#include <bzlib.h>
#include <LzmaLib.h>

#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif


#define VZT_WR_HDRID (('V' << 8) + ('Z'))
#define VZT_WR_VERSION (0x0001)

#define VZT_WR_GRANULE_SIZE (32)
#define VZT_WR_GRANULE_NUM (8)

#define VZT_WR_GZWRITE_BUFFER 4096
#define VZT_WR_SYMPRIME 500009

#ifndef _MSC_VER
typedef uint8_t                 vztint8_t;
typedef uint16_t                vztint16_t;
typedef uint32_t                vztint32_t;
typedef uint64_t                vztint64_t;

typedef int32_t                 vztsint32_t;
typedef int64_t                 vztsint64_t;
typedef uint64_t 		vzttime_t;

#ifndef __MINGW32__
#define VZT_WR_LLD "%lld"
#else
#define VZT_WR_LLD "%I64d"
#endif
#define VZT_WR_LLDESC(x) x##LL
#define VZT_WR_ULLDESC(x) x##ULL
#else
typedef unsigned __int8         vztint8_t;
typedef unsigned __int16        vztint16_t;
typedef unsigned __int32        vztint32_t;
typedef unsigned __int64        vztint64_t;

typedef          __int32        vztsint32_t;
typedef          __int64        vztsint64_t;
typedef unsigned __int64        vzttime_t;
#define VZT_WR_LLD "%I64d"
#define VZT_WR_LLDESC(x) x##i64
#define VZT_WR_ULLDESC(x) x##i64
#endif

#ifdef __GNUC__
#if __STDC_VERSION__ >= 199901L
#define _VZT_WR_INLINE inline __attribute__((__gnu_inline__))
#else
#define _VZT_WR_INLINE inline
#endif
#else
#define _VZT_WR_INLINE
#endif



/*
 * integer splay
 */
typedef struct vzt_wr_dsvzt_tree_node vzt_wr_dsvzt_Tree;
struct vzt_wr_dsvzt_tree_node {
    vzt_wr_dsvzt_Tree * left, * right;
    vzt_wr_dsvzt_Tree * child;
    vztint32_t item;
    vztint32_t val;
};

/*
 * string splay
 */
typedef struct vzt2_wr_dsvzt_tree_node vzt2_wr_dsvzt_Tree;
struct vzt2_wr_dsvzt_tree_node {
    vzt2_wr_dsvzt_Tree * left, * right;
    char *item;
    unsigned int val;
    vzt2_wr_dsvzt_Tree * next;
};


struct vzt_wr_trace
{
FILE *handle;
void *zhandle;

vzt_wr_dsvzt_Tree *dict;
vzt_wr_dsvzt_Tree *dict_cache;	/* for fast malloc/free */

int numstrings;
vzt2_wr_dsvzt_Tree *str_head, *str_curr, *str; /* for potential string vchgs */

off_t position;
off_t zfacname_predec_size, zfacname_size, zfacgeometry_size;
off_t zpackcount, zpackcount_cumulative;
off_t current_chunk, current_chunkz;

struct vzt_wr_symbol **sorted_facs;
struct vzt_wr_symbol *symchain;
int numfacs, numalias;
int numfacbytes;
int longestname;

int numsections, numblock;
off_t facname_offset, facgeometry_offset;

vzttime_t mintime, maxtime;
unsigned int timegranule;
int timescale;
int timepos;
vzttime_t *timetable;
unsigned int maxgranule;
vzttime_t firsttime, lasttime;

char *compress_fac_str;
int compress_fac_len;

vztsint64_t timezero;
vzttime_t flushtime;
unsigned flush_valid : 1;

unsigned do_strip_brackets : 1;
unsigned emitted : 1;			/* gate off change field zmode changes when set */
unsigned timeset : 1;			/* time has been modified from 0..0 */
unsigned bumptime : 1;			/* says that must go to next time position in granule as value change exists for current time */
unsigned granule_dirty : 1;		/* for flushing out final block */
unsigned blackout : 1;			/* blackout on/off */
unsigned multi_state : 1;		/* 2 or 4 state marker */
unsigned use_multi_state : 1;		/* if zero, can shortcut to 2 state */
unsigned ztype : 2;			/* 0: gzip (default), 1: bzip2, 2: lzma */
unsigned ztype_cfg : 2;			/* 0: gzip (default), 1: bzip2, 2: lzma */
unsigned rle : 1;			/* emit rle packed value section */
unsigned rle_start : 1;			/* initial/previous rle value */

char initial_value;

char zmode[4];				/* fills in with "wb0".."wb9" */
unsigned int gzbufpnt;

char *vztname;
off_t break_size;
off_t break_header_size;
unsigned int break_number;

/* larger datasets at end */
unsigned char gzdest[VZT_WR_GZWRITE_BUFFER + 10];	/* enough for zlib buffering */
struct vzt_wr_symbol *sym[VZT_WR_SYMPRIME];
};


struct vzt_wr_symbol
{
struct vzt_wr_symbol *next;
struct vzt_wr_symbol *symchain;
char *name;
int namlen;

int facnum;
struct vzt_wr_symbol *aliased_to;

unsigned int rows;
int msb, lsb;
int len;
int flags;

vzt_wr_dsvzt_Tree **prev;		/* previous  chain (for len bits) */
vztint32_t *chg;			/* for len  bits */

vzt_wr_dsvzt_Tree **prevx;		/* previous xchain (for len bits) */
vztint32_t *chgx;			/* for len xbits */
};


#define VZT_WR_IS_GZ		  (0)
#define VZT_WR_IS_BZ2		  (1)
#define VZT_WR_IS_LZMA		  (2)

#define VZT_WR_SYM_F_BITS         (0)
#define VZT_WR_SYM_F_INTEGER      (1<<0)
#define VZT_WR_SYM_F_DOUBLE       (1<<1)
#define VZT_WR_SYM_F_STRING       (1<<2)
#define VZT_WR_SYM_F_TIME         (VZT_WR_SYM_F_STRING)      /* user must correctly format this as a string */
#define VZT_WR_SYM_F_ALIAS        (1<<3)

#define VZT_WR_SYM_F_SIGNED       (1<<4)
#define VZT_WR_SYM_F_BOOLEAN      (1<<5)
#define VZT_WR_SYM_F_NATURAL      ((1<<6)|(VZT_WR_SYM_F_INTEGER))
#define VZT_WR_SYM_F_POSITIVE     ((1<<7)|(VZT_WR_SYM_F_INTEGER))
#define VZT_WR_SYM_F_CHARACTER    (1<<8)

#define VZT_WR_SYM_F_CONSTANT     (1<<9)
#define VZT_WR_SYM_F_VARIABLE     (1<<10)
#define VZT_WR_SYM_F_SIGNAL       (1<<11)

#define VZT_WR_SYM_F_IN           (1<<12)
#define VZT_WR_SYM_F_OUT          (1<<13)
#define VZT_WR_SYM_F_INOUT        (1<<14)

#define VZT_WR_SYM_F_WIRE         (1<<15)
#define VZT_WR_SYM_F_REG          (1<<16)

#define VZT_WR_SYM_MASK           (VZT_WR_SYM_F_BITS|VZT_WR_SYM_F_INTEGER|VZT_WR_SYM_F_DOUBLE|VZT_WR_SYM_F_STRING|VZT_WR_SYM_F_TIME| \
                                  VZT_WR_SYM_F_ALIAS|VZT_WR_SYM_F_SIGNED|VZT_WR_SYM_F_BOOLEAN|VZT_WR_SYM_F_NATURAL| \
                                  VZT_WR_SYM_F_POSITIVE|VZT_WR_SYM_F_CHARACTER|VZT_WR_SYM_F_CONSTANT|VZT_WR_SYM_F_VARIABLE| \
                                  VZT_WR_SYM_F_SIGNAL|VZT_WR_SYM_F_IN|VZT_WR_SYM_F_OUT|VZT_WR_SYM_F_INOUT|VZT_WR_SYM_F_WIRE| \
                                  VZT_WR_SYM_F_REG)

#define VZT_WR_SYM_F_SYNVEC       (1<<17) /* reader synthesized vector in alias sec'n from non-adjacent vectorizing */


			/* file I/O */
struct vzt_wr_trace *	vzt_wr_init(const char *name);
void 			vzt_wr_flush(struct vzt_wr_trace *lt);
void 			vzt_wr_close(struct vzt_wr_trace *lt);

			/* for dealing with very large traces, split into multiple files approximately "siz" in length */
void 			vzt_wr_set_break_size(struct vzt_wr_trace *lt, off_t siz);

			/* 0 = gzip, 1 = bzip2 */
void			vzt_wr_set_compression_type(struct vzt_wr_trace *lt, unsigned int type);

			/* 0 = no compression, 9 = best compression, 4 = default */
void			vzt_wr_set_compression_depth(struct vzt_wr_trace *lt, unsigned int depth);

			/* 0 = pure value changes, 1 = rle packed */
void			vzt_wr_set_rle(struct vzt_wr_trace *lt, unsigned int mode);

			/* bitplane depth: must call before adding any facilities */
void 			vzt_wr_force_twostate(struct vzt_wr_trace *lt);

			/* facility creation */
void                    vzt_wr_set_initial_value(struct vzt_wr_trace *lt, char value);
struct vzt_wr_symbol *	vzt_wr_symbol_find(struct vzt_wr_trace *lt, const char *name);
struct vzt_wr_symbol *	vzt_wr_symbol_add(struct vzt_wr_trace *lt, const char *name, unsigned int rows, int msb, int lsb, int flags);
struct vzt_wr_symbol *	vzt_wr_symbol_alias(struct vzt_wr_trace *lt, const char *existing_name, const char *alias, int msb, int lsb);
void			vzt_wr_symbol_bracket_stripping(struct vzt_wr_trace *lt, int doit);

			/* each granule is VZT_WR_GRANULE_SIZE (32) timesteps, default is 8 per section */
void 			vzt_wr_set_maxgranule(struct vzt_wr_trace *lt, unsigned int maxgranule);

			/* time ops */
void 			vzt_wr_set_timescale(struct vzt_wr_trace *lt, int timescale);
void 			vzt_wr_set_timezero(struct vzt_wr_trace *lt, vztsint64_t timeval);
int 			vzt_wr_set_time(struct vzt_wr_trace *lt, unsigned int timeval);
int 			vzt_wr_inc_time_by_delta(struct vzt_wr_trace *lt, unsigned int timeval);
int 			vzt_wr_set_time64(struct vzt_wr_trace *lt, vzttime_t timeval);
int 			vzt_wr_inc_time_by_delta64(struct vzt_wr_trace *lt, vzttime_t timeval);

                        /* allows blackout regions in VZT files */
void                    vzt_wr_set_dumpoff(struct vzt_wr_trace *lt);
void                    vzt_wr_set_dumpon(struct vzt_wr_trace *lt);

			/* left fill on bit_string uses vcd semantics (left fill with value[0] unless value[0]=='1', then use '0') */
int 			vzt_wr_emit_value_int(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, int value);
int 			vzt_wr_emit_value_double(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, double value);
int 			vzt_wr_emit_value_string(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, char *value);
int 			vzt_wr_emit_value_bit_string(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, char *value);

#ifdef __cplusplus
}
#endif

#endif

