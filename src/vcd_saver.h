/*
 * Copyright (c) Tony Bybell 2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef VCD_SAVER_H
#define VCD_SAVER_H

#include "vcd.h"
#include "strace.h"

enum vcd_export_typ 		{ WAVE_EXPORT_VCD, WAVE_EXPORT_LXT, WAVE_EXPORT_TIM, WAVE_EXPORT_TRANS };
enum vcd_saver_rc 		{ VCDSAV_OK, VCDSAV_EMPTY, VCDSAV_FILE_ERROR };
enum vcd_saver_tr_datatype	{ VCDSAV_IS_BIN, VCDSAV_IS_HEX,  VCDSAV_IS_TEXT };

int save_nodes_to_export(const char *fname, int export_typ);
int do_timfile_save(const char *fname);
int save_nodes_to_trans(FILE *trans, Trptr t);

/* from helpers/scopenav.c */
extern void free_hier(void);
extern char *output_hier(int is_trans, char *name);

#endif

