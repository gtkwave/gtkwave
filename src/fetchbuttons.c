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

void fetch_left(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    DEBUG(printf("Fetch Left\n"));

    GwTime newlo = (GLOBALS->tims.first) - GLOBALS->fetchwindow;

    GwTimeRange *time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);
    newlo = MAX(newlo, gw_time_range_get_start(time_range));

    if (newlo < (GLOBALS->tims.last)) {
        GLOBALS->tims.first = newlo;
        if (GLOBALS->tims.start < GLOBALS->tims.first)
            GLOBALS->tims.start = GLOBALS->tims.first;

        time_update();
    }
}

void fetch_right(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    DEBUG(printf("Fetch Right\n"));

    GwTime newhi = (GLOBALS->tims.last) + GLOBALS->fetchwindow;

    GwTimeRange *time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);
    newhi = MIN(newhi, gw_time_range_get_end(time_range));

    if (newhi > (GLOBALS->tims.first)) {
        GLOBALS->tims.last = newhi;
        time_update();
    }
}

void discard_left(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    DEBUG(printf("Discard Left\n"));

    GwTime newlo = (GLOBALS->tims.first) + GLOBALS->fetchwindow;

    if (newlo < (GLOBALS->tims.last)) {
        GLOBALS->tims.first = newlo;
        time_update();
    }
}

void discard_right(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    DEBUG(printf("Discard Right\n"));

    GwTime newhi = (GLOBALS->tims.last) - GLOBALS->fetchwindow;

    if (newhi > (GLOBALS->tims.first)) {
        GLOBALS->tims.last = newhi;
        time_update();
    }
}
