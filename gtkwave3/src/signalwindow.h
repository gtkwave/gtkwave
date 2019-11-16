/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_SIGNALWINDOW_H
#define WAVE_SIGNALWINDOW_H

/* for dnd */
#define WAVE_DRAG_TAR_NAME_0         "text/plain"
#define WAVE_DRAG_TAR_INFO_0         0

#define WAVE_DRAG_TAR_NAME_1         "text/uri-list"         /* not url-list */
#define WAVE_DRAG_TAR_INFO_1         1

#define WAVE_DRAG_TAR_NAME_2         "STRING"
#define WAVE_DRAG_TAR_INFO_2         2


void draw_signalarea_focus(void);
gint signalarea_configure_event(GtkWidget *widget, GdkEventConfigure *event);
void dnd_error(void);
gint install_keypress_handler(void);
void remove_keypress_handler(gint id);

#endif

