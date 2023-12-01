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
#include "debug.h"
#include "busy.h"
#include "hierpack.h"
#include "fst.h"

/*
 * actually import an lx2 trace but don't do it if it's already been imported
 */
void import_lx2_trace(GwNode *np)
{
    switch (GLOBALS->is_lx2) {
        case LXT2_IS_VLIST:
            import_vcd_trace(GLOBALS->vcd_file, np);
            return;
        case LXT2_IS_FST:
            import_fst_trace(GLOBALS->fst_file, np);
            return;
        case LXT2_IS_FSDB:
            import_extload_trace(np);
            return;
        default:
            g_return_if_reached();
            break;
    }
}

/*
 * pre-import many traces at once so function above doesn't have to iterate...
 */
void lx2_set_fac_process_mask(GwNode *np)
{
    switch (GLOBALS->is_lx2) {
        case LXT2_IS_VLIST:
            vcd_set_fac_process_mask(GLOBALS->vcd_file, np);
            return;
        case LXT2_IS_FST:
            fst_set_fac_process_mask(GLOBALS->fst_file, np);
            return;
        case LXT2_IS_FSDB:
            fsdb_set_fac_process_mask(np);
            return;
        default:
            g_return_if_reached();
            break;
    }
}

void lx2_import_masked(void)
{
    switch (GLOBALS->is_lx2) {
        case LXT2_IS_VLIST:
            vcd_import_masked(GLOBALS->vcd_file);
            return;
        case LXT2_IS_FST:
            fst_import_masked(GLOBALS->fst_file);
            return;
        case LXT2_IS_FSDB:
            fsdb_import_masked();
            return;
        default:
            g_return_if_reached();
            break;
    }
}
