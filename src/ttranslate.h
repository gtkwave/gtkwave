/*
 * Copyright (c) Tony Bybell 2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_TTRANSLATE_H
#define WAVE_TTRANSLATE_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "debug.h"

#define TTRANS_FILTER_MAX (128)


void ttrans_searchbox(char *title);
void init_ttrans_data(void);
int install_ttrans_filter(int which);
void set_current_translate_ttrans(char *name);
void remove_all_ttrans_filters(void);

#endif

