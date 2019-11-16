/*
 * Copyright (c) Tony Bybell 2005-2009
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_PIPEIO_H
#define WAVE_PIPEIO_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "debug.h"

#if defined _MSC_VER || defined __MINGW32__
#include <windows.h>
#endif


struct pipe_ctx
{
#if defined _MSC_VER || defined __MINGW32__

HANDLE g_hChildStd_IN_Rd;
HANDLE g_hChildStd_IN_Wr;  /* handle for gtkwave to write to */
HANDLE g_hChildStd_OUT_Rd; /* handle for gtkwave to read from */
HANDLE g_hChildStd_OUT_Wr;
PROCESS_INFORMATION piProcInfo;

#else

FILE *sin, *sout;
int fd0, fd1;
pid_t pid;

#endif
};


struct pipe_ctx *pipeio_create(char *execappname, char *arg);
void pipeio_destroy(struct pipe_ctx *p);

#endif

