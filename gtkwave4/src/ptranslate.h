/*
 * Copyright (c) Tony Bybell 2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_PTRANSLATE_H
#define WAVE_PTRANSLATE_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "debug.h"

#define PROC_FILTER_MAX (128)


void ptrans_searchbox(char *title);
void init_proctrans_data(void);
int install_proc_filter(int which);
void set_current_translate_proc(char *name);
void remove_all_proc_filters(void);

#endif

