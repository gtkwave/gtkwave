/*
 * Copyright (c) Tony Bybell 2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_FSTRDR_H
#define WAVE_FSTRDR_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "vcd.h"
#include "ae2.h"
#include "tree_component.h"

TimeType fst_main(char *fname, char *skip_start, char *skip_end);
void import_fst_trace(nptr np);
void fst_set_fac_process_mask(nptr np);
void fst_import_masked(void);

#endif

