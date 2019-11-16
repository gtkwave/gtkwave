/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef BSEARCH_NODES_VECTORS_H
#define BSEARCH_NODES_VECTORS_H

int bsearch_timechain(TimeType key);
int bsearch_aetinfo_timechain(TimeType key);
hptr bsearch_node(nptr n, TimeType key);
vptr bsearch_vector(bvptr b, TimeType key);
char *bsearch_trunc(char *ascii, int maxlen);
char *bsearch_trunc_print(char *ascii, int maxlen);
struct symbol *bsearch_facs(char *ascii, unsigned int *rows_return);

#endif

