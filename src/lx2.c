/*
 * Copyright (c) Tony Bybell 2003-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <stdio.h>
#include "lx2.h"

#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "vcd.h"
#include "busy.h"

// TODO: remove
static GPtrArray *import_nodes;

/*
 * actually import an lx2 trace but don't do it if it's already been imported
 */
void import_lx2_trace(GwNode *np)
{
    GwNode *nodes[2] = {np, NULL};

    // TODO: report errors
    g_assert_true(gw_dump_file_import_traces(GLOBALS->dump_file, nodes, NULL));
}

/*
 * pre-import many traces at once so function above doesn't have to iterate...
 */
void lx2_set_fac_process_mask(GwNode *np)
{
    if (import_nodes == NULL) {
        import_nodes = g_ptr_array_new();
    }

    g_ptr_array_add(import_nodes, np);
}

void lx2_import_masked(void)
{
    if (import_nodes == NULL) {
        return;
    }

    g_ptr_array_add(import_nodes, NULL);

    // TODO: report errors
    g_assert_true(
        gw_dump_file_import_traces(GLOBALS->dump_file, (GwNode **)import_nodes->pdata, NULL));

    g_ptr_array_set_size(import_nodes, 0);
}
