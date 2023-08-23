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
#define WAVE_DRAG_TARGET_SIGNAL_LIST "GTKWAVE_TRACE_SIGNAL_LIST"
#define WAVE_DRAG_TARGET_TCL "GTKWAVE_TRACE_TCL"

enum
{
    WAVE_DRAG_INFO_SIGNAL_LIST = 0,
    WAVE_DRAG_INFO_TCL = 1,
};

void draw_signalarea_focus(cairo_t *cr);
void dnd_error(void);
gint install_keypress_handler(void);
void remove_keypress_handler(gint id);

void redraw_signals_and_waves(void);

#endif
