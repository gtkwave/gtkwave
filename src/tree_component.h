/*
 * Copyright (c) Tony Bybell 2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "debug.h"

#ifdef _WAVE_HAVE_JUDY
#include <Judy.h>
#else
#include "jrb.h"
#endif

#ifndef WAVE_TREE_COMP_H
#define WAVE_TREE_COMP_H

void iter_through_comp_name_table(void);
int add_to_comp_name_table(const char *s, int slen);

#endif

