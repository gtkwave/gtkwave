/*
 * Copyright (c) Tony Bybell 1999-2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "debug.h"
#include "globals.h"

#ifndef CURRENTTIME_H
#define CURRENTTIME_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "analyzer.h"
#include "regex_wave.h"
#include "translate.h"

#define WAVE_INF_SCALING (0.5)
#define WAVE_SI_UNITS " munpfaz"

/* currenttime.c protos */

void fractional_timescale_fix(char *);
void update_time_box(void);
void update_currenttime(GwTime val);
void reformat_time(char *buf, GwTime val, char dim);
void reformat_time_simple(char *buf, GwTime val, char dim);
GwTime unformat_time(const char *buf, char dim);
void time_trunc_set(void);
GwTime time_trunc(GwTime t);
void exponent_to_time_scale(signed char scale);

/* other protos / definitions */

#include "baseconvert.h"
#include "edgebuttons.h"
#include "entry.h"
#include "fetchbuttons.h"
#include "file.h"
#include "fonts.h"
#include "logfile.h"
#include "markerbox.h"
#include "menu.h"
#include "mouseover.h"
#include "pagebuttons.h"
#include "renderopt.h"
#include "search.h"
#include "shiftbuttons.h"
#include "showchange.h"
#include "signalwindow.h"
#include "simplereq.h"
#include "status.h"
#include "strace.h"
#include "timeentry.h"
#include "tree.h"
#include "treesearch.h"
#include "wavewindow.h"
#include "zoombuttons.h"

#endif
