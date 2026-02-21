/*
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_SYMBOL_H
#define WAVE_SYMBOL_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "analyzer.h"
#include "currenttime.h"
#include "tree.h"
#include "debug.h"

#define WAVE_DECOMPRESSOR "gzip -cd " /* zcat alone doesn't cut it for AIX */

#include <unistd.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

struct string_chain_t
{
    struct string_chain_t *next;
    char *str;
};

void facsplit(char *, int *, int *);
int sigcmp(char *, char *);
void quicksort(GwSymbol **, int, int);

GwBits *makevec(char *, char *);
GwBits *makevec_annotated(char *, char *);
int maketraces(char *, char *, int);

/* additions to bitvec.c because of search.c/menu.c ==> formerly in analyzer.h */
GwBitVector *bits2vector(GwBits *b);
GwBits *makevec_chain(char *vec, GwSymbol *sym, int len);
int add_vector_chain(GwSymbol *s, int len);
char *makename_chain(GwSymbol *sym);

/* accessor functions for sym->selected moved (potentially) to sparse array */
char get_s_selected(GwSymbol *s);
char set_s_selected(GwSymbol *s, char value);
void destroy_s_selected(void);

#endif
