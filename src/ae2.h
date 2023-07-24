/*
 * Copyright (c) Tony Bybell 2004-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_AE2RDR_H
#define WAVE_AE2RDR_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "vcd.h"
#include "lx2.h"

#ifdef AET2_IS_PRESENT
#include <ae2rw.h>
#endif
#ifdef AET2_ALIASDB_IS_PRESENT
#include <aliasdb.h>
#endif

#define AET2_RDLOAD "AE2LOAD | "

#define AE2_MAX_NAME_LENGTH 		2048
#define	AE2_MAXFACLEN 	 		65536
#define AE2_MAX_ROWS			16384
#define WAVE_ADB_ALLOC_POOL_SIZE 	(1024 * 1024)
#define WAVE_ADB_ALLOC_ALTREQ_SIZE 	(4 * 1024)


#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct ae2_ncycle_autosort
{
struct ae2_ncycle_autosort *next;
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


TimeType 	ae2_main(char *fname, char *skip_start, char *skip_end);
void 		import_ae2_trace(nptr np);
void 		ae2_set_fac_process_mask(nptr np);
void 		ae2_import_masked(void);

#endif
