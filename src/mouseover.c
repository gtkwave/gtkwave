/*
 * Copyright (c) Tony Bybell 2006-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gw-mouseover.h>

void move_mouseover(GwTrace *t, gint xin, gint yin, GwTime tim)
{
    if (t == NULL || GLOBALS->disable_mouseover || xin <= 0 || yin <= 0 ||
        yin > GLOBALS->waveheight) {
        g_clear_pointer(&GLOBALS->mouseover_mouseover_c_1, gtk_widget_destroy);
        return;
    }

    if (GLOBALS->mouseover_mouseover_c_1 == NULL) {
        GLOBALS->mouseover_mouseover_c_1 = gw_mouseover_new();
        gtk_window_set_transient_for(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1),
                                     GTK_WINDOW(GLOBALS->mainwindow));

        gtk_widget_show(GLOBALS->mouseover_mouseover_c_1);
    }

    gw_mouseover_update(GW_MOUSEOVER(GLOBALS->mouseover_mouseover_c_1), t, tim);

    gint xd = 0;
    gint yd = 0;
    gdk_window_get_origin(gtk_widget_get_window(GLOBALS->wavearea), &xd, &yd);
    gtk_window_move(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1), xin + xd + 8, yin + yd + 20);
}

void move_mouseover_sigs(GwTrace *t, gint xin, gint yin, GwTime tim)
{
    if (t == NULL || GLOBALS->disable_mouseover) {
        g_clear_pointer(&GLOBALS->mouseover_mouseover_c_1, gtk_widget_destroy);
        return;
    }

    if (GLOBALS->mouseover_mouseover_c_1 == NULL) {
        GLOBALS->mouseover_mouseover_c_1 = gw_mouseover_new();
        gtk_window_set_transient_for(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1),
                                     GTK_WINDOW(GLOBALS->mainwindow));

        gtk_widget_show(GLOBALS->mouseover_mouseover_c_1);
    }

    gw_mouseover_update_signal_list(GW_MOUSEOVER(GLOBALS->mouseover_mouseover_c_1), t, tim);

    gint xd = 0;
    gint yd = 0;
    gdk_window_get_origin(gtk_widget_get_window(GLOBALS->signalwindow), &xd, &yd);
    gtk_window_move(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1), xin + xd + 8, yin + yd + 20);
}
