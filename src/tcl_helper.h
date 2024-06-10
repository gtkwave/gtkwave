/*
 * Copyright (c) Tony Bybell and Concept Engineering GmbH 2008-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_TCLHELPER_H
#define WAVE_TCLHELPER_H

#include <config.h>

#define WAVE_OE_ME \
    if (one_entry) { \
        if (!mult_entry) { \
            mult_entry = one_entry; \
            mult_len = strlen(mult_entry); \
        } else { \
            int sing_len = strlen(one_entry); \
            mult_entry = realloc_2(mult_entry, mult_len + sing_len + 1); \
            strcpy(mult_entry + mult_len, one_entry); \
            mult_len += sing_len; \
        } \
    }

struct iter_dnd_strings
{
    char *one_entry;
    char *mult_entry;
    int mult_len;
};

typedef enum
{
    LL_NONE,
    LL_INT,
    LL_UINT,
    LL_CHAR,
    LL_SHORT,
    LL_STR,
    LL_VOID_P,
    LL_TIMETYPE
} ll_elem_type;

typedef union llist_payload
{
    int i;
    unsigned int u;
    char c;
    short s;
    char *str;
    void *p;
    GwTime tt;
} llist_u;

typedef struct llist_s
{
    llist_u u;
    struct llist_s *prev;
    struct llist_s *next;
} llist_p;

int process_url_file(char *s);
int process_url_list(char *s);
int process_tcl_list(const char *s, gboolean prepend);
char *add_dnd_from_searchbox(void);
char *add_dnd_from_signal_window(void);
char *add_traces_from_signal_window(gboolean is_from_tcl_command);
char *add_dnd_from_tree_window(void);
char *emit_gtkwave_savefile_formatted_entries_in_tcl_list(GwTrace *trhead, gboolean use_tcl_mode);

char *zMergeTclList(int argc, const char **argv);
char **zSplitTclList(const char *list, int *argcPtr);
char *make_single_tcl_list_name(char *s, char *opt_value, int promote_to_bus, int preserve_range);

char *rpc_script_execute(const char *nam);

#endif