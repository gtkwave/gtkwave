/*
 * Copyright (c) Tony Bybell 1999-2004.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef REGEX_WAVE_H
#define REGEX_WAVE_H

enum WaveRegexTypes { WAVE_REGEX_SEARCH, WAVE_REGEX_TREE, WAVE_REGEX_WILD, WAVE_REGEX_DND, WAVE_REGEX_TOTAL };

int wave_regex_compile(char *regex, int which);
int wave_regex_match(char *str, int which);

void *wave_regex_alloc_compile(char *regex);
int wave_regex_alloc_match(void *mreg, char *str);
void wave_regex_alloc_free(void *pnt);

#endif

