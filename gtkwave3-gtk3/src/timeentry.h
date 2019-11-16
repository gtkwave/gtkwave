/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_TIMEENTRY_H
#define WAVE_TIMEENTRY_H

void time_update(void);
void from_entry_callback(GtkWidget *widget, GtkWidget *entry);
void to_entry_callback(GtkWidget *widget, GtkWidget *entry);

#endif

