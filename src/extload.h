/*
 * Copyright (c) Tony Bybell 2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_EXTRDR_H
#define WAVE_EXTRDR_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "vcd.h"

#define EXTLOAD "EXTLOAD | "

TimeType 	extload_main(char *fname, char *skip_start, char *skip_end);
void 		import_extload_trace(nptr np);

/* FsdbReader adds */
void 		fsdb_import_masked(void);
void 		fsdb_set_fac_process_mask(nptr np);

#endif

