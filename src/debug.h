/*
 * Copyright (c) Tony Bybell 1999-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_DEBUG_H
#define WAVE_DEBUG_H

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtkwave.h>
#include "gtk23compat.h"

#ifndef __MINGW32__
#include <stdint.h>
#else
#include <windows.h>
#endif

#define WAVE_MAX_CLIST_LENGTH 15000

#define WAVE_MINZOOM (GW_TIME_CONSTANT(-4000000000))

#ifdef DEBUG_PRINTF
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#ifdef DEBUG_MALLOC_LINES
void free_2(void *ptr, char *filename, int lineno);
#define free_2(x) free_2((x), __FILE__, __LINE__)

void *malloc_2(size_t size, char *filename, int lineno);
#define malloc_2(x) malloc_2((x), __FILE__, __LINE__)

void *realloc_2(void *ptr, size_t size, char *filename, int lineno);
#define realloc_2(x, y) realloc_2((x), (y), __FILE__, __LINE__)

void *calloc_2(size_t nmemb, size_t size, char *filename, int lineno);
#define calloc_2(x, y) calloc_2((x), (y), __FILE__, __LINE__)

#else
void free_2(void *ptr);
void *malloc_2(size_t size);
void *realloc_2(void *ptr, size_t size);
void *calloc_2(size_t nmemb, size_t size);
#endif

void free_outstanding(void);

char *strdup_2(const char *s);
char *strdup_2s(const char *s);

char *tmpnam_2(char *s, int *fd); /* mimic functionality of tmpnam() */

GwTime atoi_64(const char *str);

void gtk_tooltips_set_tip_2(GtkWidget *widget, const gchar *tip_text);

char *realpath_2(const char *path, char *resolved_path);

enum WaveLoadingTitleType
{
    WAVE_SET_TITLE_NONE,
    WAVE_SET_TITLE_MODIFIED,
    WAVE_SET_TITLE_LOADING
};

#undef WAVE_USE_SIGCMP_INFINITE_PRECISION /* define this for slow sigcmp with infinite digit \
                                             accuracy */
#define WAVE_OPT_SKIP 1 /* make larger for more accel on traces */

/* for source code annotation helper app */

#ifndef PATH_MAX
#define PATH_MAX (4096)
#endif

#define WAVE_MATCHWORD "WAVE"
enum AnnotateAetType
{
    WAVE_ANNO_NONE,
    WAVE_ANNO_FST,
    WAVE_ANNO_MAX
};

#if !defined __MINGW32__

#include <sys/ipc.h>
#include <sys/shm.h>

struct gtkwave_annotate_ipc_t
{
    char matchword[4]; /* match against WAVE_MATCHWORD string */
    char time_string[40]; /* formatted marker time */

    pid_t gtkwave_process;
    pid_t browser_process;

    GwTime marker;
    unsigned marker_set : 1;
    unsigned cygwin_remote_kill : 1;

    int aet_type;
    char aet_name[PATH_MAX + 1];
    char stems_name[PATH_MAX + 1];
};

#else

struct gtkwave_annotate_ipc_t
{
    char matchword[4]; /* match against WAVE_MATCHWORD string */
    char time_string[40]; /* formatted marker time */

#ifdef __MINGW32__
    HANDLE browser_process;
#endif

    GwTime marker;
    unsigned marker_set : 1;
    unsigned cygwin_remote_kill : 1;

    int aet_type;
    char aet_name[PATH_MAX + 1];
    char stems_name[PATH_MAX + 1];
};

#endif

#define DUAL_MATCHWORD "DUAL"

struct gtkwave_dual_ipc_t
{
    char matchword[4]; /* match against DUAL_MATCHWORD string */

    GwTime left_margin_time;
    GwTime marker, baseline;
    gdouble zoom;
    unsigned use_new_times : 1;
    unsigned viewer_is_initialized : 1;
};

enum GtkwaveFileTypes
{
    G_FT_UNKNOWN,
    G_FT_FST
};

int determine_gtkwave_filetype(const char *path);

GtkWidget *X_gtk_entry_new_with_max_length(gint max);

#endif
