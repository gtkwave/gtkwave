/*
 * Copyright (c) Tony Bybell 2010-2014
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_MARKER_DIALOG_H
#define WAVE_MARKER_DIALOG_H

#ifdef WAVE_MANYMARKERS_MODE

/* 702 will go from A-Z, AA-AZ, ... , ZA-ZZ */
/* this is a bijective name similar to the columns on spreadsheets */
/* the upper count is (practically) unbounded */
#define WAVE_NUM_NAMED_MARKERS (702)

#else

/* do not go less than 26! */
#define WAVE_NUM_NAMED_MARKERS (26)

#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GW_TYPE_MARKER_DIALOG gw_marker_dialog_get_type()
G_DECLARE_FINAL_TYPE(GwMarkerDialog, gw_marker_dialog, GW, MARKER_DIALOG, GtkDialog)

GwMarkerDialog *gw_marker_dialog_new(void);

void gw_show_marker_dialog(void);

TimeType gw_marker_dialog_get_time(GwMarkerDialog *marker_dialog, int index);
const gchar *gw_marker_dialog_get_name(GwMarkerDialog *marker_dialog, int index);

char *make_bijective_marker_id_string(char *buf, unsigned int value);
unsigned int bijective_marker_id_string_hash(const char *so);
unsigned int bijective_marker_id_string_len(const char *s);

G_END_DECLS

#endif
