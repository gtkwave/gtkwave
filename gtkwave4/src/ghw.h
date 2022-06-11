/*
 * Copyright (c) Tony Bybell 2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef GHW_H
#define GHW_H

#include <limits.h>
#include "tree.h"
#include "vcd.h"

#define WAVE_GHW_DUMMYFACNAME "!!__(dummy)__!!"

TimeType ghw_main(char *fname);
int strand_pnt(char *s);

#endif

