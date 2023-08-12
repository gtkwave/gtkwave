/*
 * Copyright (c) Tony Bybell 1999.
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

void service_left_shift(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    GtkAdjustment *hadj;
    gfloat inc;
    GwTime ntinc;

    if (GLOBALS->nspx > 1.0)
        inc = GLOBALS->nspx;
    else
        inc = 1.0;


    hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
    if ((gtk_adjustment_get_value(hadj) - inc) > GLOBALS->tims.first)
        gtk_adjustment_set_value(hadj, gtk_adjustment_get_value(hadj) - inc);
    else
        gtk_adjustment_set_value(hadj, GLOBALS->tims.first);

    ntinc = (GwTime)inc;
    if ((GLOBALS->tims.start - ntinc) > GLOBALS->tims.first)
        GLOBALS->tims.timecache = GLOBALS->tims.start - ntinc;
    else
        GLOBALS->tims.timecache = GLOBALS->tims.first;

    time_update();

    DEBUG(printf("Left Shift\n"));
}

void service_right_shift(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    GtkAdjustment *hadj;
    gfloat inc;
    GwTime ntinc, pageinc;

    if (GLOBALS->nspx > 1.0)
        inc = GLOBALS->nspx;
    else
        inc = 1.0;
    ntinc = (GwTime)inc;

    hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
    if ((gtk_adjustment_get_value(hadj) + inc) < GLOBALS->tims.last)
        gtk_adjustment_set_value(hadj, gtk_adjustment_get_value(hadj) + inc);
    else
        gtk_adjustment_set_value(hadj, GLOBALS->tims.last - inc);

    pageinc = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);

    if ((GLOBALS->tims.start + ntinc) < (GLOBALS->tims.last - pageinc + 1))
        GLOBALS->tims.timecache = GLOBALS->tims.start + ntinc;
    else {
        GLOBALS->tims.timecache = GLOBALS->tims.last - pageinc + 1;
        if (GLOBALS->tims.timecache < GLOBALS->tims.first)
            GLOBALS->tims.timecache = GLOBALS->tims.first;
    }

    time_update();

    DEBUG(printf("Right Shift\n"));
}
