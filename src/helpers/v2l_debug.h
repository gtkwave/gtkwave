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
#include <glib.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

struct memchunk
{
struct memchunk *next;
void *ptr;
size_t size;
};

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

gint64 atoi_64(char *str);

/*
 * if your system really doesn't have alloca() at all,
 * you can force functionality by using malloc
 * instead.  but note that you're going to have some
 * memory leaks because of it.  you have been warned.
 */

#include <stdlib.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#elif defined(__GNUC__)
#ifndef alloca
#define alloca __builtin_alloca
#endif
#elif defined(_MSC_VER)
#include <malloc.h>
#define alloca _alloca
#endif
#define wave_alloca alloca
#endif

