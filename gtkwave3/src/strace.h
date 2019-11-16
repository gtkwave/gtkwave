/*
 * Copyright (c) Tony Bybell 1999-2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef GTKWAVE_STRACE_H
#define GTKWAVE_STRACE_H

#include <gtk/gtk.h>
#include <string.h>
#include <stdarg.h>
#include "debug.h"
#include "analyzer.h"
#include "currenttime.h"
#include "bsearch.h"

#if WAVE_USE_GTK2
#define WAVE_NUM_STRACE_WINDOWS (2)
#else
#define WAVE_NUM_STRACE_WINDOWS (1)
#endif

#define WAVE_STRACE_ITERATOR(x) for((x)=((WAVE_NUM_STRACE_WINDOWS)-1); (x)>=0 ; (x)--)
#define WAVE_STRACE_ITERATOR_FWD(x) for((x)=0;(x)<(WAVE_NUM_STRACE_WINDOWS);(x)++)

enum strace_directions
{ STRACE_BACKWARD, STRACE_FORWARD };

enum st_stype
        {ST_DC, ST_HIGH, ST_MID, ST_X, ST_LOW, ST_STRING,
         ST_RISE, ST_FALL, ST_ANY, WAVE_STYPE_COUNT};

struct strace_defer_free
{
struct strace_defer_free *next;
Trptr defer;
};

struct strace_back
{
struct strace *parent;
int which;
};

struct strace
{
struct strace *next;
char *string;           /* unmalloc this when all's done! */
Trptr trace;
char value;
char search_result;

union
	{
        hptr    h;             /* what makes up this trace */
        vptr    v;
      	} his;

struct strace_back *back[WAVE_STYPE_COUNT];    /* dealloc these too! */
};


struct timechain
{
struct timechain *next;
TimeType t;
};


struct mprintf_buff_t
{
struct mprintf_buff_t *next;
char *str;
};

struct item_mark_string {
   char *str;
   unsigned char idx;
};


/* for being able to handle multiple strace sessions at once, context is moved here */
struct strace_ctx_t
{
GtkWidget *ptr_mark_count_label_strace_c_1;
struct strace *straces;
struct strace *shadow_straces;
struct strace_defer_free *strace_defer_free_head;
GtkWidget *window_strace_c_10;
void (*cleanup_strace_c_7)(void);

struct mprintf_buff_t *mprintf_buff_head;
struct mprintf_buff_t *mprintf_buff_current;
char *shadow_string;

TimeType *timearray;
int timearray_size;

char logical_mutex[6];
char shadow_logical_mutex[6];
char shadow_active;
char shadow_encountered_parsewavline;
char shadow_type;
signed char mark_idx_start;
signed char mark_idx_end;
signed char shadow_mark_idx_start;
signed char shadow_mark_idx_end;
};


void strace_search(int direction);
void strace_maketimetrace(int mode); /* 1=create, zero=delete */
TimeType strace_adjust(TimeType a, TimeType b);

void swap_strace_contexts(void);
void delete_strace_context(void);
void cache_actual_pattern_mark_traces(void);

void edge_search(int direction); /* from edgebuttons.c */

int mprintf(const char *fmt, ... );
void delete_mprintf(void);

void tracesearchbox(const char *title, GtkSignalFunc func, gpointer data);

#endif

