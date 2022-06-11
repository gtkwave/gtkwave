/*
 * Copyright (c) 2001-2014 Tony Bybell.
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

#ifndef WAVE_DEBUG_H
#define WAVE_DEBUG_H

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <wavealloca.h>

struct memchunk
{
struct memchunk *next;
void *ptr;
size_t size;
};


/*
 * If you have problems viewing traces (mangled timevalues),
 * make sure that you use longs rather than the glib 64-bit
 * types...
 */
#define G_HAVE_GINT64
#define gint64 int64_t
#define guint64 uint64_t

#ifdef G_HAVE_GINT64
typedef gint64          TimeType;
typedef guint64         UTimeType;

#ifndef _MSC_VER
#define LLDescriptor(x) x##LL
#define ULLDescriptor(x) x##ULL
#ifndef __MINGW32__
#if __WORDSIZE == 64
#define TTFormat "%ld"
#else
#define TTFormat "%lld"
#endif
#else
#define TTFormat "%I64d"
#endif
#else
#define LLDescriptor(x) x##i64
#define ULLDescriptor(x) x##i64
#define TTFormat "%I64d"
#endif

#else
typedef long            TimeType;
typedef unsigned long   UTimeType;

#define TTFormat "%d"
#define LLDescriptor(x) x
#define ULLDescriptor(x) x
#endif


#ifdef DEBUG_PRINTF
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#ifdef DEBUG_MALLOC
#define DEBUG_M(x) x
#else
#define DEBUG_M(x)
#endif

void *malloc_2(size_t size);
void *realloc_2(void *ptr, size_t size);
void *calloc_2(size_t nmemb, size_t size);
void free_2(void *ptr);

TimeType atoi_64(char *str);
#endif

