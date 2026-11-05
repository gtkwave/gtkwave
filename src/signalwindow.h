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

/* for dnd
 * MIME-style names are required on macOS: plain names get extra pasteboard
 * types without a MIME type, which makes GTK's Quartz backend log
 * "gdk_atom_intern: assertion 'atom_name != NULL' failed" on every drag motion.
 * Other platforms accept any name: X11 uses it as an atom, Wayland expects MIME
 * types anyway, and Windows registers it as a clipboard format.
 */
#define WAVE_DRAG_TARGET_SIGNAL_LIST "application/x-gtkwave-signal-list"
#define WAVE_DRAG_TARGET_TCL "application/x-gtkwave-tcl"

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
