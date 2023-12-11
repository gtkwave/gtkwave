/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/*
 * vcd.c 			23jan99ajb
 * evcd parts 			29jun99ajb
 * profiler optimizations 	15jul99ajb
 * more profiler optimizations	25jan00ajb
 * finsim parameter fix		26jan00ajb
 * vector rechaining code	03apr00ajb
 * multiple var section code	06apr00ajb
 * fix for duplicate nets	19dec00ajb
 * support for alt hier seps	23dec00ajb
 * fix for rcs identifiers	16jan01ajb
 * coredump fix for bad VCD	04apr02ajb
 * min/maxid speedup            27feb03ajb
 * bugfix on min/maxid speedup  06jul03ajb
 * escaped hier modification    20feb06ajb
 * added real_parameter vartype 04aug06ajb
 * added in/out port vartype    31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 * Backtracking fix             16oct18ajb
 */

/* AIX may need this for alloca to work */
#if defined _AIX
#pragma alloca
#endif

#include <config.h>
#include <gtkwave.h>
#include "globals.h"
#include "vcd.h"
#include "gw-vcd-loader.h"

void strcpy_vcdalt(char *too, char *from, char delim)
{
    char ch;

    do {
        ch = *(from++);
        if (ch == delim) {
            ch = GLOBALS->hier_delimeter;
        }
    } while ((*(too++) = ch));
}

// TODO: remove
GwDumpFile *vcd_recoder_main(char *fname)
{
    const Settings *global_settings = &GLOBALS->settings;

    GwLoader *loader = gw_vcd_loader_new();
    gw_loader_set_preserve_glitches(loader, global_settings->preserve_glitches);
    gw_loader_set_preserve_glitches_real(loader, global_settings->preserve_glitches_real);
    gw_vcd_loader_set_vlist_prepack(GW_VCD_LOADER(loader), global_settings->vlist_prepack);

    GwDumpFile *file = gw_loader_load(loader, fname, NULL); // TODO: use error

    g_object_unref(loader);

    return file;
}
