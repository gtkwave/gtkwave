/*
 * Copyright (c) Tony Bybell 2010-2017
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_TREESEARCH_H
#define WAVE_TREESEARCH_H

GtkWidget *treeboxframe(const char *title);
void dump_open_tree_nodes(FILE *wave, xl_Tree *t);
int force_open_tree_node(char *name, int keep_path_nodes_open, GwTree **t_pnt);
void select_tree_node(char *name);
void dnd_setup(GtkWidget *src, gboolean search); /* dnd from gtk2 tree to signalwindow */
void treeview_select_all_callback(void); /* gtk2 */
void treeview_unselect_all_callback(void); /* gtk2 */
int treebox_is_active(void);

void DND_helper_quartz(char *data);

void recurse_import(GtkWidget *widget, guint callback_action);
#define WV_RECURSE_IMPORT_WARN (0)

enum sst_cb_action
{
    SST_ACTION_INSERT,
    SST_ACTION_REPLACE,
    SST_ACTION_APPEND,
    SST_ACTION_PREPEND,
    SST_ACTION_NONE
};

enum
{
    VIEW_DRAG_INACTIVE,
    TREE_TO_VIEW_DRAG_ACTIVE,
    SEARCH_TO_VIEW_DRAG_ACTIVE
};

void action_callback(enum sst_cb_action action);

#endif
