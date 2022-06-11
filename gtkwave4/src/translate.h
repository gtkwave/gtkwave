/*
 * Copyright (c) Tony Bybell 2005.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_TRANSLATE_H
#define WAVE_TRANSLATE_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "debug.h"

/*
 * char splay
 */
typedef struct xl_tree_node xl_Tree;
struct xl_tree_node {
    xl_Tree *left, *right;
    char *item;
    char *trans;
};


#define FILE_FILTER_MAX (128)
#define WAVE_TCL_INSTALLED_FILTER "\"TCL_Installed_Filter\""


xl_Tree * xl_splay (char *i, xl_Tree * t);
xl_Tree * xl_insert(char *i, xl_Tree * t, char *trans);
xl_Tree * xl_delete(char *i, xl_Tree * t);


void trans_searchbox(char *title);
void init_filetrans_data(void);
int install_file_filter(int which);

void set_current_translate_enums(char *lst);
void set_current_translate_file(char *name);

#endif

