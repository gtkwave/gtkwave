#pragma once

#include <gtk/gtk.h>
#include "tree_widget.h"

extern ds_Tree *flattened_mod_list_root;
extern int verilog_2005;
extern int mod_cnt;
extern ds_Tree **mod_list;
extern struct gtkwave_annotate_ipc_t *anno_ctx;

GListModel * create_module(void);