/*
 * Copyright (c) Tony Bybell 1999-2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef VCD_H
#define VCD_H

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif

#include <setjmp.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include "symbol.h"
#include "debug.h"
#include "tree.h"


#define vcd_exit(x) \
    if (GLOBALS->vcd_jmp_buf) { \
        splash_finalize(); \
        longjmp(*(GLOBALS->vcd_jmp_buf), x); \
    } else { \
        exit(x); \
    }

enum VCDName_ByteSubstitutions
{
    VCDNAM_NULL = 0,
    VCDNAM_ESCAPE
};

void strcpy_vcdalt(char *too, char *from, char delim);

#endif
