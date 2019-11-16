/*
 * Copyright (c) Tony Bybell 1999-2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

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

struct blackout_region_t
{
struct blackout_region_t *next;
TimeType bstart, bend;
};


/* currenttime.c protos */

void fractional_timescale_fix(char *);
void update_markertime(TimeType val);
void update_maxtime(TimeType val);
void update_basetime(TimeType val);
void update_currenttime(TimeType val);
void update_maxmarker_labels(void);
void reformat_time(char *buf, TimeType val, char dim);
void reformat_time_simple(char *buf, TimeType val, char dim);
TimeType unformat_time(const char *buf, char dim);
void time_trunc_set(void);
TimeType time_trunc(TimeType t);
void exponent_to_time_scale(signed char scale);

/* other protos / definitions */

#include "baseconvert.h"
#include "edgebuttons.h"
#include "entry.h"
#include "fetchbuttons.h"
#include "file.h"
#include "fonts.h"
#include "help.h"
#include "interp.h"
#include "logfile.h"
#include "markerbox.h"
#include "menu.h"
#include "mouseover.h"
#include "mouseover_sigs.h"
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
#include "vcd_partial.h"
#include "wavewindow.h"
#include "zoombuttons.h"
#include "hiersearch.h"

#endif

