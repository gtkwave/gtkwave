/*
 * Copyright (c) Tony Bybell 2003-2004.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_VZTRDR_H
#define WAVE_VZTRDR_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "vcd.h"


TimeType 	vzt_main(char *fname, char *skip_start, char *skip_end);
void 		import_vzt_trace(nptr np);
void 		vzt_set_fac_process_mask(nptr np);
void 		vzt_import_masked(void);

#endif

