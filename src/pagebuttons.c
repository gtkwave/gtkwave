/*
 * Copyright (c) Tony Bybell 1999-2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "currenttime.h"
#include "pixmaps.h"
#include "debug.h"

void service_left_page(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    GwTime ntinc, ntfrac;

    ntinc = (GwTime)(((gdouble)GLOBALS->wavewidth) *
                     GLOBALS->nspx); /* really don't need this var but the speed of ui code is human
                                        dependent.. */
    ntfrac = ntinc * GLOBALS->page_divisor;
    if ((ntfrac < 1) || (ntinc < 1))
        ntfrac = /*ntinc=*/1; /* scan-build */

    if ((GLOBALS->tims.start - ntfrac) > GLOBALS->tims.first)
        GLOBALS->tims.timecache = GLOBALS->tims.start - ntfrac;
    else
        GLOBALS->tims.timecache = GLOBALS->tims.first;

    gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache);
    time_update();

    DEBUG(printf("Left Page\n"));
}

void service_right_page(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    GwTime ntinc, ntfrac;

    ntinc = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
    ntfrac = ntinc * GLOBALS->page_divisor;

    if ((ntfrac < 1) || (ntinc < 1))
        ntfrac = ntinc = 1;

    if ((GLOBALS->tims.start + ntfrac) < (GLOBALS->tims.last - ntinc + 1)) {
        GLOBALS->tims.timecache = GLOBALS->tims.start + ntfrac;
    } else {
        GLOBALS->tims.timecache = GLOBALS->tims.last - ntinc + 1;
        if (GLOBALS->tims.timecache < GLOBALS->tims.first)
            GLOBALS->tims.timecache = GLOBALS->tims.first;
    }

    gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache);
    time_update();

    DEBUG(printf("Right Page\n"));
}
