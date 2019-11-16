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
#include "wavealloca.h"
#include "analyzer.h"
#include "currenttime.h"
#include "tree.h"
#include "debug.h"

#define SYMPRIME 500009
#define WAVE_DECOMPRESSOR "gzip -cd "	/* zcat alone doesn't cut it for AIX */

#ifndef _MSC_VER
#include <unistd.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#else
typedef long off_t;
#include <windows.h>
#include <io.h>
#endif

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct fac
{
struct Node *working_node;
int node_alias;
int len;
unsigned int flags;
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


struct symbol
{
#ifndef _WAVE_HAVE_JUDY
struct symbol *sym_next;  /* for hash chain, judy uses sym_judy in globals */
#endif

struct symbol *vec_root, *vec_chain;
char *name;
struct Node *n;

#ifndef _WAVE_HAVE_JUDY
char s_selected;		/* for the clist object */
#endif
};


struct symchain		/* for restoring state of ->selected in signal regex search */
{
struct symchain *next;
struct symbol *symbol;
};


struct string_chain_t
{
struct string_chain_t *next;
char *str;
};


/* hash create/destroy */
void sym_hash_initialize(void *g);
void sym_hash_destroy(void *g);


struct symbol *symfind(char *, unsigned int *);
struct symbol *symadd(char *, int);
struct symbol *symadd_name_exists(char *name, int hv);
int hash(char *s);

/* typically use zero for hashval as it doesn't matter if facs are sorted as symfind will bsearch... */
#define symadd_name_exists_sym_exists(s, nam, hv) \
(s)->name = (nam); /* (s)->sym_next=GLOBALS->sym_hash[(hv)]; GLOBALS->sym_hash[(hv)]=(s); (obsolete) */

void facsplit(char *, int *, int *);
int sigcmp(char *, char *);
void quicksort(struct symbol **, int, int);

void wave_heapsort(struct symbol **a, int num);

struct Bits *makevec(char *, char *);
struct Bits *makevec_annotated(char *, char *);
int maketraces(char *, char *, int);


/* additions to bitvec.c because of search.c/menu.c ==> formerly in analyzer.h */
bvptr bits2vector(struct Bits *b);
struct Bits *makevec_selected(char *vec, int numrows, char direction);
int add_vector_selected(char *alias, int numrows, char direction);
struct Bits *makevec_range(char *vec, int lo, int hi, char direction);
int add_vector_range(char *alias, int lo, int hi, char direction);
struct Bits *makevec_chain(char *vec, struct symbol *sym, int len);
int add_vector_chain(struct symbol *s, int len);
char *makename_chain(struct symbol *sym);

/* splash screen activation (version >= GTK2 only) */
void splash_create(void);
void splash_sync(off_t current, off_t total);
void splash_finalize(void);
gint splash_button_press_event(GtkWidget *widget, GdkEventExpose *event);

/* accessor functions for sym->selected moved (potentially) to sparse array */
char get_s_selected(struct symbol *s);
char set_s_selected(struct symbol *s, char value);
void destroy_s_selected(void);

#endif

