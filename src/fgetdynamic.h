/*
 * Copyright (c) Tony Bybell 1999-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef FGET_DYNAMIC_H
#define FGET_DYNAMIC_H

#include "vlist.h"

/* using alloca avoids having to preserve across contexts */
struct wave_script_args {
  struct wave_script_args *curr;
  struct wave_script_args *next;
  char payload[]; /* C99 */
};

char *fgetmalloc(FILE *handle);
char *fgetmalloc_stripspaces(FILE *handle);

char *wave_script_args_fgetmalloc(struct wave_script_args *wave_script_args);
char *wave_script_args_fgetmalloc_stripspaces(struct wave_script_args *wave_script_args);

#endif

