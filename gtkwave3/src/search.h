/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_SEARCHBOX_H
#define WAVE_SEARCHBOX_H

void searchbox(char *title, GtkSignalFunc func);
void search_enter_callback(GtkWidget *widget, GtkWidget *do_warning);
void search_insert_callback(GtkWidget *widget, char is_prepend);
int searchbox_is_active(void);

#endif

