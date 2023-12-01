/*
 * Copyright (c) Tristan Gingold and Tony Bybell 2006-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <config.h>
#include "globals.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <ctype.h>
#include "gtk23compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "busy.h"
#include "debug.h"
#include "hierpack.h"
#include "tcl_helper.h"
#include "tcl_support_commands.h"
#include "signal_list.h"

/* Treesearch is a pop-up window used to select signals.
   It is composed of two main areas:
   * A tree area to select the hierarchy [tree area]
   * The (filtered) list of signals contained in the hierarchy [signal area].
*/

/* SIG_ROOT is the branch currently selected.
   Signals of SIG_ROOT are displayed in the signals window.  */

/* Only signals which match the filter are displayed in the signal area.  */

/* The signal area is based on a tree view which requires a store model.
   This store model contains the list of signals to be displayed.
*/
enum
{
    NAME_COLUMN,
    TREE_COLUMN,
    TYPE_COLUMN,
    DIR_COLUMN,
    DTYPE_COLUMN,
    N_COLUMNS
};

/* list of autocoalesced (synthesized) filter names that need to be freed at some point) */

void free_afl(void)
{
    struct autocoalesce_free_list *at;

    while (GLOBALS->afl_treesearch_gtk2_c_1) {
        if (GLOBALS->afl_treesearch_gtk2_c_1->name)
            free_2(GLOBALS->afl_treesearch_gtk2_c_1->name);
        at = GLOBALS->afl_treesearch_gtk2_c_1->next;
        free_2(GLOBALS->afl_treesearch_gtk2_c_1);
        GLOBALS->afl_treesearch_gtk2_c_1 = at;
    }
}

/* point to pure signame (remove hierarchy) for fill_sig_store() */
static char *prune_hierarchy(char *nam)
{
    char cmpchar = GLOBALS->alt_hier_delimeter ? GLOBALS->alt_hier_delimeter : '.';
    char *t = nam;
    char *lastmatch = NULL;

    while (t && *t) {
        if (*t == cmpchar) {
            lastmatch = t + 1;
        }
        t++;
    }

    return (lastmatch ? lastmatch : nam);
}

/* fix escaped signal names */
static void *fix_escaped_names(char *s, int do_free)
{
    char *s2 = s;
    int found = 0;

    while (*s2) {
        if ((*s2) == VCDNAM_ESCAPE) {
            found = 1;
            break;
        }
        s2++;
    }

    if (found) {
        s2 = strdup_2(s);
        if (do_free)
            free_2(s);
        s = s2;

        while (*s2) {
            if (*s2 == VCDNAM_ESCAPE) {
                *s2 = GLOBALS->hier_delimeter;
            } /* restore back to normal */
            s2++;
        }
    }

    return (s);
}

/* truncate VHDL types to string directly after final '.' */
char *varxt_fix(char *s)
{
    char *pnt = strrchr(s, '.');
    return (pnt ? (pnt + 1) : s);
}

/* Fill the store model using current SIG_ROOT and FILTER_STR.  */
void fill_sig_store(void)
{
    GwTree *t;
    GwTree *t_prev = NULL;
    GtkTreeIter iter;

    if (GLOBALS->selected_sig_name) {
        free_2(GLOBALS->selected_sig_name);
        GLOBALS->selected_sig_name = NULL;
    }

    free_afl();
    gtk_list_store_clear(GLOBALS->sig_store_treesearch_gtk2_c_1);

    for (t = GLOBALS->sig_root_treesearch_gtk2_c_1; t != NULL; t = t->next) {
        int i = t->t_which;
        char *s, *tmp2;
        int vartype;
        int vardir;
        int is_tname = 0;
        int wrexm;
        int vardt;
        unsigned int varxt;
        char *varxt_pnt;

        if (i < 0) {
            t_prev = NULL;
            continue;
        }

        if (t_prev) /* duplicates removal for faulty dumpers */
        {
            if (!strcmp(t_prev->name, t->name)) {
                continue;
            }
        }
        t_prev = t;

        varxt = GLOBALS->facs[i]->n->varxt;
        varxt_pnt = varxt ? varxt_fix(fst_file_get_subvar(GLOBALS->fst_file, varxt)) : NULL;

        vartype = GLOBALS->facs[i]->n->vartype;
        if ((vartype < 0) || (vartype > GW_VAR_TYPE_MAX)) {
            vartype = 0;
        }

        vardir = GLOBALS->facs[i]
                     ->n->vardir; /* two bit already chops down to 0..3, but this doesn't hurt */
        if ((vardir < 0) || (vardir > GW_VAR_DIR_MAX)) {
            vardir = 0;
        }

        vardt = GLOBALS->facs[i]->n->vardt;
        if ((vardt < 0) || (vardt > GW_VAR_DATA_TYPE_MAX)) {
            vardt = 0;
        }

        if (!GLOBALS->facs[i]->vec_root) {
            is_tname = 1;
            s = t->name;
            s = fix_escaped_names(s, 0);
        } else {
            if (GLOBALS->autocoalesce) {
                char *p;
                if (GLOBALS->facs[i]->vec_root != GLOBALS->facs[i])
                    continue;

                tmp2 = makename_chain(GLOBALS->facs[i]);
                p = prune_hierarchy(tmp2);
                s = (char *)malloc_2(strlen(p) + 4);
                strcpy(s, "[] ");
                strcpy(s + 3, p);
                s = fix_escaped_names(s, 1);
                free_2(tmp2);
            } else {
                char *p = prune_hierarchy(GLOBALS->facs[i]->name);
                s = (char *)malloc_2(strlen(p) + 4);
                strcpy(s, "[] ");
                strcpy(s + 3, p);
                s = fix_escaped_names(s, 1);
            }
        }

        wrexm = 0;
        if ((GLOBALS->filter_str_treesearch_gtk2_c_1 == NULL) ||
            ((!GLOBALS->filter_noregex_treesearch_gtk2_c_1) &&
             (wrexm = wave_regex_match(t->name, WAVE_REGEX_TREE)) &&
             (!GLOBALS->filter_matlen_treesearch_gtk2_c_1)) ||
            (GLOBALS->filter_matlen_treesearch_gtk2_c_1 &&
             ((GLOBALS->filter_typ_treesearch_gtk2_c_1 == vardir) ^
              GLOBALS->filter_typ_polarity_treesearch_gtk2_c_1) &&
             (wrexm || (wrexm = wave_regex_match(t->name, WAVE_REGEX_TREE))))) {
            gtk_list_store_prepend(GLOBALS->sig_store_treesearch_gtk2_c_1, &iter);
            if (is_tname) {
                gtk_list_store_set(GLOBALS->sig_store_treesearch_gtk2_c_1,
                                   &iter,
                                   NAME_COLUMN,
                                   s,
                                   TREE_COLUMN,
                                   t,
                                   TYPE_COLUMN,
                                   (((GLOBALS->supplemental_datatypes_encountered) &&
                                     (!GLOBALS->supplemental_vartypes_encountered))
                                        ? (varxt ? varxt_pnt : gw_var_data_type_to_string(vardt))
                                        : gw_var_type_to_string(vartype)),
                                   DIR_COLUMN,
                                   gw_var_dir_to_string(vardir),
                                   DTYPE_COLUMN,
                                   varxt ? varxt_pnt : gw_var_data_type_to_string(vardt),
                                   -1);

                if (s != t->name) {
                    free_2(s);
                }
            } else {
                struct autocoalesce_free_list *a =
                    calloc_2(1, sizeof(struct autocoalesce_free_list));
                a->name = s;
                a->next = GLOBALS->afl_treesearch_gtk2_c_1;
                GLOBALS->afl_treesearch_gtk2_c_1 = a;

                gtk_list_store_set(GLOBALS->sig_store_treesearch_gtk2_c_1,
                                   &iter,
                                   NAME_COLUMN,
                                   s,
                                   TREE_COLUMN,
                                   t,
                                   TYPE_COLUMN,
                                   (((GLOBALS->supplemental_datatypes_encountered) &&
                                     (!GLOBALS->supplemental_vartypes_encountered))
                                        ? (varxt ? varxt_pnt : gw_var_data_type_to_string(vardt))
                                        : gw_var_type_to_string(vartype)),
                                   DIR_COLUMN,
                                   gw_var_dir_to_string(vardir),
                                   DTYPE_COLUMN,
                                   varxt ? varxt_pnt : gw_var_data_type_to_string(vardt),
                                   -1);
            }
        } else {
            if (s != t->name) {
                free_2(s);
            }
        }
    }
}

/*
 * tree open/close handling
 */
static void XXX_create_sst_nodes_if_necessary(GtkTreeModel *model,
                                              GtkTreeIter *iter,
                                              GtkTreePath *path)
{
#ifndef WAVE_DISABLE_FAST_TREE

    GwTree *t;

    gtk_tree_model_get(model, iter, XXX_TREE_COLUMN, &t, -1);

    if (t->child) {
        GtkTreePath *path2 = gtk_tree_path_copy(path);
        GtkTreeIter iter2;

        gtk_tree_path_down(path2);

        while (path2) {
            if (!gtk_tree_model_get_iter(model, &iter2, path2)) {
                break;
            }

            gtk_tree_model_get(model, &iter2, XXX_TREE_COLUMN, &t, -1);

            if (t->t_which < 0) {
                if (!t->children_in_gui) {
                    GtkTreeIter iter2_copy = iter2;

                    t->children_in_gui = 1;

                    if (t->child)
                        XXX_maketree2(&iter2_copy, t->child, 0);
                }
            }

            gtk_tree_path_next(path2);
        }

        gtk_tree_path_free(path2);
    }

#endif
}

int force_open_tree_node(char *name, int keep_path_nodes_open, GwTree **t_pnt)
{
    GtkTreeModel *model = GTK_TREE_MODEL(GLOBALS->treestore_main);
    GtkTreeIter iter;
    int rv = SST_NODE_NOT_EXIST; /* can possibly open */

    if (model && gtk_tree_model_get_iter_first(model, &iter)) {
        if (1) {
            int namlen = strlen(name);
            char *namecache = g_alloca(namlen + 1);
            char *name_end = name + namlen - 1;
            char *zap = name;
            int depth = 1;

            strcpy(namecache, name);
            for (;;) {
                GwTree *t;
                gtk_tree_model_get(model, &iter, XXX_TREE_COLUMN, &t, -1);
                if (t_pnt) {
                    *t_pnt = t;
                }
                while (*zap) {
                    if (*zap != GLOBALS->hier_delimeter) {
                        zap++;
                    } else {
                        *zap = 0;
                        break;
                    }
                }

                if (!strcmp(t->name, name)) {
                    if (zap == name_end) {
                        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                        GtkTreePath *path2 = gtk_tree_model_get_path(model, &iter);
                        gboolean *exphist = g_alloca(depth * sizeof(gboolean));
                        GtkTreePath **pathhist = g_alloca(depth * sizeof(GtkTreePath *));
                        int i = depth - 1;

                        memset(exphist, 0, depth * sizeof(gboolean)); /* scan-build */
                        memset(pathhist, 0, depth * sizeof(GtkTreePath *)); /* scan-build */

                        while (gtk_tree_path_up(path2)) {
                            exphist[i] =
                                gtk_tree_view_row_expanded(GTK_TREE_VIEW(GLOBALS->treeview_main),
                                                           path2);
                            pathhist[i] = gtk_tree_path_copy(path2);
                            i--;
                        }
                        gtk_tree_path_free(path2);

                        for (i = 0; i < depth; i++) /* fully expand down */
                        {
                            gtk_tree_view_expand_row(GTK_TREE_VIEW(GLOBALS->treeview_main),
                                                     pathhist[i],
                                                     0);
                        }

                        gtk_tree_view_expand_row(GTK_TREE_VIEW(GLOBALS->treeview_main), path, 0);

                        for (i = depth - 1; i >= 0; i--) /* collapse back up */
                        {
                            if (!keep_path_nodes_open && !exphist[i]) {
                                gtk_tree_view_collapse_row(GTK_TREE_VIEW(GLOBALS->treeview_main),
                                                           pathhist[i]);
                            }

                            gtk_tree_path_free(pathhist[i]);
                        }

                        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(GLOBALS->treeview_main),
                                                     path,
                                                     NULL,
                                                     TRUE,
                                                     0.5,
                                                     0.5);

                        gtk_tree_path_free(path);
                        rv = SST_NODE_FOUND; /* opened */
                        GLOBALS->open_tree_nodes =
                            xl_insert(namecache, GLOBALS->open_tree_nodes, NULL);
                        return rv;
                        /* break; */
                    } else {
                        depth++;
                        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                        XXX_create_sst_nodes_if_necessary(model, &iter, path);
                        gtk_tree_path_down(path);
                        gboolean child_exists = gtk_tree_model_get_iter(model, &iter, path);
                        gtk_tree_path_free(path);
                        if (!child_exists) {
                            break;
                        }
                        name = ++zap;
                        continue;
                    }
                }

                if (!gtk_tree_model_iter_next(model, &iter)) {
                    return (rv);
                }
            }
        }
    }

    return (SST_TREE_NOT_EXIST);
}

void dump_open_tree_nodes(FILE *wave, xl_Tree *t)
{
    if (t->left) {
        dump_open_tree_nodes(wave, t->left);
    }

    fprintf(wave, "[treeopen] %s\n", t->item);

    if (t->right) {
        dump_open_tree_nodes(wave, t->right);
    }
}

void select_tree_node(char *name)
{
    GtkTreeModel *model = GTK_TREE_MODEL(GLOBALS->treestore_main);
    GtkTreeIter iter;

    if (model && gtk_tree_model_get_iter_first(model, &iter)) {
        if (1) {
            int namlen = strlen(name);
            char *namecache = g_alloca(namlen + 1);
            char *name_end = name + namlen - 1;
            char *zap = name;
            int depth = 1;

            strcpy(namecache, name);
            for (;;) {
                GwTree *t;
                gtk_tree_model_get(model, &iter, XXX_TREE_COLUMN, &t, -1);
                while (*zap) {
                    if (*zap != GLOBALS->hier_delimeter) {
                        zap++;
                    } else {
                        *zap = 0;
                        break;
                    }
                }

                if (!strcmp(t->name, name)) {
                    if (zap == name_end) {
                        /* printf("[treeselectnode] '%s' ok\n", name); */
                        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                        gtk_tree_view_set_cursor(GTK_TREE_VIEW(GLOBALS->treeview_main),
                                                 path,
                                                 NULL,
                                                 FALSE);
                        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(GLOBALS->treeview_main),
                                                     path,
                                                     NULL,
                                                     TRUE,
                                                     0.5,
                                                     0.5);
                        gtk_tree_path_free(path);

                        GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = t;
                        GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
                        fill_sig_store();

                        return;
                    } else {
                        depth++;
                        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                        XXX_create_sst_nodes_if_necessary(model, &iter, path);
                        gtk_tree_path_down(path);
                        gboolean child_exists = gtk_tree_model_get_iter(model, &iter, path);
                        gtk_tree_path_free(path);
                        if (!child_exists) {
                            break;
                        }
                        name = ++zap;
                        continue;
                    }
                }

                if (!gtk_tree_model_iter_next(model, &iter)) {
                    return;
                }
            }
        }
    }
}

/* Callbacks for tree area when a row is selected/deselected.  */
static void XXX_select_row_callback(GtkTreeModel *model, GtkTreePath *path)
{
    GtkTreeIter iter;
    GwTree *t;
    GwTree **gctr;
    int depth, i;
    int len = 1;
    char *tstring;
    char hier_suffix[2];
    GtkTreePath *path2;

    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return; /* path describes a non-existing row - should not happen */
    }

    hier_suffix[0] = GLOBALS->hier_delimeter;
    hier_suffix[1] = 0;

    depth = gtk_tree_store_iter_depth(GLOBALS->treestore_main, &iter);
    depth++;

    path2 = gtk_tree_path_copy(path);

    gctr = g_alloca(depth * sizeof(GwTree *));
    for (i = depth - 1; i >= 0; i--) {
        gtk_tree_model_get_iter(model, &iter, path2);
        gtk_tree_model_get(model, &iter, XXX_TREE_COLUMN, &gctr[i], -1);
        t = gctr[i];

        len += (strlen(t->name) + 1);
        gtk_tree_path_up(path2);
    }

    gtk_tree_path_free(path2);

    tstring = g_alloca(len);
    memset(tstring, 0, len);

    for (i = 0; i < depth; i++) {
        t = gctr[i];
        strcat(tstring, t->name);
        strcat(tstring, hier_suffix);
    }

    if (GLOBALS->selected_hierarchy_name) {
        free_2(GLOBALS->selected_hierarchy_name);
    }
    GLOBALS->selected_hierarchy_name = strdup_2(tstring);

    t = gctr[depth - 1];
    DEBUG(printf("TS: %08x %s\n", t, t->name));
    GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = t;
    GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
    fill_sig_store();
    gtkwavetcl_setvar(WAVE_TCLCB_TREE_SELECT,
                      GLOBALS->selected_hierarchy_name,
                      WAVE_TCLCB_TREE_SELECT_FLAGS);
}

static void XXX_unselect_row_callback(GtkTreeModel *model, GtkTreePath *path)
{
    GtkTreeIter iter;
    GwTree *t;

    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return; /* path describes a non-existing row - should not happen */
    }

    if (GLOBALS->selected_hierarchy_name) {
        gtkwavetcl_setvar(WAVE_TCLCB_TREE_UNSELECT,
                          GLOBALS->selected_hierarchy_name,
                          WAVE_TCLCB_TREE_UNSELECT_FLAGS);
        free_2(GLOBALS->selected_hierarchy_name);
        GLOBALS->selected_hierarchy_name = NULL;
    }

    gtk_tree_model_get(model, &iter, XXX_TREE_COLUMN, &t, -1);
    if (t) {
        /* unused */
    }

    DEBUG(printf("TU: %08x %s\n", t, t->name));
    GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = NULL;
    GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
    fill_sig_store();
}

/* Signal callback for the filter widget.
   This catch the return key to update the signal area.  */
static gboolean filter_edit_cb(GtkWidget *widget, GdkEventKey *ev, gpointer *data)
{
    (void)data;

    /* Maybe this test is too strong ?  */
    if (ev->keyval == GDK_KEY_Return) {
        const char *t;

        /* Get the filter string, save it and change the store.  */
        if (GLOBALS->filter_str_treesearch_gtk2_c_1) {
            free_2((char *)GLOBALS->filter_str_treesearch_gtk2_c_1);
            GLOBALS->filter_str_treesearch_gtk2_c_1 = NULL;
        }
        t = gtk_entry_get_text(GTK_ENTRY(widget));
        if (t == NULL || *t == 0)
            GLOBALS->filter_str_treesearch_gtk2_c_1 = NULL;
        else {
            int i;

            GLOBALS->filter_str_treesearch_gtk2_c_1 = malloc_2(strlen(t) + 1);
            strcpy(GLOBALS->filter_str_treesearch_gtk2_c_1, t);

            GLOBALS->filter_typ_treesearch_gtk2_c_1 = GW_VAR_DIR_UNSPECIFIED;
            GLOBALS->filter_typ_polarity_treesearch_gtk2_c_1 = 0;
            GLOBALS->filter_matlen_treesearch_gtk2_c_1 = 0;
            GLOBALS->filter_noregex_treesearch_gtk2_c_1 = 0;

            if (GLOBALS->filter_str_treesearch_gtk2_c_1[0] == '+') {
                for (i = 0; i <= GW_VAR_DIR_MAX; i++) {
                    int tlen = strlen(gw_var_dir_to_string(i));
                    if (!strncasecmp(gw_var_dir_to_string(i),
                                     GLOBALS->filter_str_treesearch_gtk2_c_1 + 1,
                                     tlen)) {
                        if (GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 1] == '+') {
                            GLOBALS->filter_matlen_treesearch_gtk2_c_1 = tlen + 2;
                            GLOBALS->filter_typ_treesearch_gtk2_c_1 = i;
                            if (GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 2] == 0) {
                                GLOBALS->filter_noregex_treesearch_gtk2_c_1 = 1;
                            }
                        }
                    }
                }
            } else if (GLOBALS->filter_str_treesearch_gtk2_c_1[0] == '-') {
                for (i = 0; i <= GW_VAR_DIR_MAX; i++) {
                    int tlen = strlen(gw_var_dir_to_string(i));
                    if (!strncasecmp(gw_var_dir_to_string(i),
                                     GLOBALS->filter_str_treesearch_gtk2_c_1 + 1,
                                     tlen)) {
                        if (GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 1] == '-') {
                            GLOBALS->filter_matlen_treesearch_gtk2_c_1 = tlen + 2;
                            GLOBALS->filter_typ_treesearch_gtk2_c_1 = i;
                            GLOBALS->filter_typ_polarity_treesearch_gtk2_c_1 =
                                1; /* invert via XOR with 1 */
                            if (GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 2] == 0) {
                                GLOBALS->filter_noregex_treesearch_gtk2_c_1 = 1;
                            }
                        }
                    }
                }
            }

            wave_regex_compile(GLOBALS->filter_str_treesearch_gtk2_c_1 +
                                   GLOBALS->filter_matlen_treesearch_gtk2_c_1,
                               WAVE_REGEX_TREE);
        }
        fill_sig_store();
    }
    return FALSE;
}

static gboolean XXX_view_selection_func(GtkTreeSelection *selection,
                                        GtkTreeModel *model,
                                        GtkTreePath *path,
                                        gboolean path_currently_selected,
                                        gpointer userdata)
{
    (void)selection;
    (void)userdata;

    if (!path_currently_selected) {
        XXX_select_row_callback(model, path);
    } else {
        XXX_unselect_row_callback(model, path);
    }

    return (TRUE);
}

static void XXX_generic_tree_expand_collapse_callback(int is_expand,
                                                      GtkTreeModel *model,
                                                      GtkTreeIter *iter,
                                                      GtkTreePath *path)
{
    GwTree *t;
    GwTree **gctr;
    int depth, i;
    int len = 1;
    char *tstring;
    char hier_suffix[2];
    GtkTreePath *path2;
    int found;

    if (!gtk_tree_model_get_iter(model, iter, path)) {
        return; /* path describes a non-existing row - should not happen */
    }

    hier_suffix[0] = GLOBALS->hier_delimeter;
    hier_suffix[1] = 0;

    depth = gtk_tree_store_iter_depth(GLOBALS->treestore_main, iter);
    depth++;

    path2 = gtk_tree_path_copy(path);

    gctr = g_alloca(depth * sizeof(GwTree *));
    for (i = depth - 1; i >= 0; i--) {
        gtk_tree_model_get_iter(model, iter, path2);
        gtk_tree_model_get(model, iter, XXX_TREE_COLUMN, &gctr[i], -1);
        t = gctr[i];

        len += (strlen(t->name) + 1);
        gtk_tree_path_up(path2);
    }

    gtk_tree_path_free(path2);

    tstring = g_alloca(len);
    memset(tstring, 0, len);

    for (i = 0; i < depth; i++) {
        t = gctr[i];
        strcat(tstring, t->name);
        strcat(tstring, hier_suffix);
    }

    if (GLOBALS->open_tree_nodes) /* cut down on chatter to Tcl clients */
    {
        GLOBALS->open_tree_nodes = xl_splay(tstring, GLOBALS->open_tree_nodes);
        if (!strcmp(GLOBALS->open_tree_nodes->item, tstring)) {
            found = 1;
        } else {
            found = 0;
        }
    } else {
        found = 0;
    }

    if (is_expand) {
        GLOBALS->open_tree_nodes = xl_insert(tstring, GLOBALS->open_tree_nodes, NULL);
        if (!found) {
            gtkwavetcl_setvar(WAVE_TCLCB_TREE_EXPAND, tstring, WAVE_TCLCB_TREE_EXPAND_FLAGS);
        }
    } else {
        GLOBALS->open_tree_nodes = xl_delete(tstring, GLOBALS->open_tree_nodes);
        if (found) {
            gtkwavetcl_setvar(WAVE_TCLCB_TREE_COLLAPSE, tstring, WAVE_TCLCB_TREE_COLLAPSE_FLAGS);
        }
    }
}

static void XXX_tree_expand_callback(GtkTreeView *tree_view,
                                     GtkTreeIter *iter,
                                     GtkTreePath *path,
                                     gpointer user_data)
{
    (void)tree_view;
    (void)user_data;

    XXX_create_sst_nodes_if_necessary(
        gtk_tree_view_get_model(GTK_TREE_VIEW(GLOBALS->treeview_main)),
        iter,
        path);

    XXX_generic_tree_expand_collapse_callback(
        1,
        gtk_tree_view_get_model(GTK_TREE_VIEW(GLOBALS->treeview_main)),
        iter,
        path);

#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_TREESEARCH
    gtk_widget_queue_resize(GTK_WIDGET(GLOBALS->treeview_main));
#endif

#ifdef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND
#ifdef MAC_INTEGRATION
    /* workaround for ctree not rendering properly in OSX */
    gtk_widget_hide(GTK_WIDGET(GLOBALS->treeview_main));
    gtk_widget_show(GTK_WIDGET(GLOBALS->treeview_main));
#endif
#endif
}

static void XXX_tree_collapse_callback(GtkTreeView *tree_view,
                                       GtkTreeIter *iter,
                                       GtkTreePath *path,
                                       gpointer user_data)
{
    (void)tree_view;
    (void)user_data;

    XXX_generic_tree_expand_collapse_callback(
        0,
        gtk_tree_view_get_model(GTK_TREE_VIEW(GLOBALS->treeview_main)),
        iter,
        path);

#ifdef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND
#ifdef MAC_INTEGRATION
    /* workaround for ctree not rendering properly in OSX */
    gtk_widget_hide(GTK_WIDGET(GLOBALS->treeview_main));
    gtk_widget_show(GTK_WIDGET(GLOBALS->treeview_main));
#endif
#endif
}

/*
 * for dynamic updates, simply fake the return key to the function above
 */
static void press_callback(GtkWidget *widget, gpointer *data)
{
    GdkEventKey ev;

    ev.keyval = GDK_KEY_Return;

    filter_edit_cb(widget, &ev, data);
}

/*
 * select/unselect all in treeview
 */
void treeview_select_all_callback(void)
{
    GtkTreeSelection *ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
    gtk_tree_selection_select_all(ts);
}

void treeview_unselect_all_callback(void)
{
    GtkTreeSelection *ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
    gtk_tree_selection_unselect_all(ts);
}

int treebox_is_active(void)
{
    return (GLOBALS->is_active_treesearch_gtk2_c_6);
}

/***************************************************************************/

/* Callback for insert/replace/append buttions.
   This call-back is called for every signal selected.  */

static void sig_selection_foreach(GtkTreeModel *model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer data)
{
    (void)path;
    (void)data;

    GwTree *sel;
    /* const enum sst_cb_action action = (enum sst_cb_action)data; */
    int i;
    int low, high;

    /* Get the tree.  */
    gtk_tree_model_get(model, iter, TREE_COLUMN, &sel, -1);

    if (!sel)
        return;

    low = fetchlow(sel)->t_which;
    high = fetchhigh(sel)->t_which;

    /* Add signals and vectors.  */
    for (i = low; i <= high; i++) {
        int len;
        struct symbol *s, *t;
        s = GLOBALS->facs[i];
        t = s->vec_root;
        if ((t) && (GLOBALS->autocoalesce)) {
            if (get_s_selected(t)) {
                set_s_selected(t, 0);
                len = 0;
                while (t) {
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        } else {
            AddNodeUnroll(s->n, NULL);
        }
    }
}

static void sig_selection_foreach_finalize(gpointer data)
{
    const enum sst_cb_action action = GPOINTER_TO_INT(data);

    if (action == SST_ACTION_REPLACE || action == SST_ACTION_INSERT ||
        action == SST_ACTION_PREPEND) {
        GwTrace *tfirst = NULL;
        GwTrace *tlast = NULL;
        GwTrace *t;
        GwTrace **tp = NULL;
        int numhigh = 0;
        int it;

        if (action == SST_ACTION_REPLACE) {
            tfirst = GLOBALS->traces.first;
            tlast = GLOBALS->traces.last; /* cache for highlighting */
        }

        GLOBALS->traces.buffercount = GLOBALS->traces.total;
        GLOBALS->traces.buffer = GLOBALS->traces.first;
        GLOBALS->traces.bufferlast = GLOBALS->traces.last;
        GLOBALS->traces.first = GLOBALS->tcache_treesearch_gtk2_c_2.first;
        GLOBALS->traces.last = GLOBALS->tcache_treesearch_gtk2_c_2.last;
        GLOBALS->traces.total = GLOBALS->tcache_treesearch_gtk2_c_2.total;

        if (action == SST_ACTION_REPLACE) {
            t = GLOBALS->traces.first;
            while (t) {
                if (t->flags & TR_HIGHLIGHT) {
                    numhigh++;
                }
                t = t->t_next;
            }
            if (numhigh) {
                tp = calloc_2(numhigh, sizeof(GwTrace *));
                t = GLOBALS->traces.first;
                it = 0;
                while (t) {
                    if (t->flags & TR_HIGHLIGHT) {
                        tp[it++] = t;
                    }
                    t = t->t_next;
                }
            }
        }

        if (action == SST_ACTION_PREPEND) {
            PrependBuffer();
        } else {
            PasteBuffer();
        }

        GLOBALS->traces.buffercount = GLOBALS->tcache_treesearch_gtk2_c_2.buffercount;
        GLOBALS->traces.buffer = GLOBALS->tcache_treesearch_gtk2_c_2.buffer;
        GLOBALS->traces.bufferlast = GLOBALS->tcache_treesearch_gtk2_c_2.bufferlast;

        if (action == SST_ACTION_REPLACE) {
            for (it = 0; it < numhigh; it++) {
                tp[it]->flags |= TR_HIGHLIGHT;
            }

            t = tfirst;
            while (t) {
                t->flags &= ~TR_HIGHLIGHT;
                if (t == tlast)
                    break;
                t = t->t_next;
            }

            CutBuffer();

            while (tfirst) {
                tfirst->flags |= TR_HIGHLIGHT;
                if (tfirst == tlast)
                    break;
                tfirst = tfirst->t_next;
            }

            if (tp) {
                free_2(tp);
            }
        }
    }
}

static void sig_selection_foreach_preload_lx2(GtkTreeModel *model,
                                              GtkTreePath *path,
                                              GtkTreeIter *iter,
                                              gpointer data)
{
    (void)path;
    (void)data;

    GwTree *sel;
    /* const enum sst_cb_action action = (enum sst_cb_action)data; */
    int i;
    int low, high;

    /* Get the tree.  */
    gtk_tree_model_get(model, iter, TREE_COLUMN, &sel, -1);

    if (!sel)
        return;

    low = fetchlow(sel)->t_which;
    high = fetchhigh(sel)->t_which;

    /* If signals are vectors, coalesces vectors if so.  */
    for (i = low; i <= high; i++) {
        struct symbol *s;
        s = GLOBALS->facs[i];
        if (s->vec_root) {
            set_s_selected(s->vec_root, GLOBALS->autocoalesce);
        }
    }

    /* LX2 */
    if (GLOBALS->is_lx2) {
        for (i = low; i <= high; i++) {
            struct symbol *s, *t;
            s = GLOBALS->facs[i];
            t = s->vec_root;
            if ((t) && (GLOBALS->autocoalesce)) {
                if (get_s_selected(t)) {
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            GLOBALS->pre_import_treesearch_gtk2_c_1++;
                        }
                        t = t->vec_chain;
                    }
                }
            } else {
                if (s->n->mv.mvlfac) {
                    lx2_set_fac_process_mask(s->n);
                    GLOBALS->pre_import_treesearch_gtk2_c_1++;
                }
            }
        }
    }
    /* LX2 */
}

void action_callback(enum sst_cb_action action)
{
    if (action == SST_ACTION_NONE)
        return; /* only used for double-click in signals pane of SST */

    GLOBALS->pre_import_treesearch_gtk2_c_1 = 0;

    /* once through to mass gather lx2 traces... */
    gtk_tree_selection_selected_foreach(GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                        &sig_selection_foreach_preload_lx2,
                                        GINT_TO_POINTER(action));
    if (GLOBALS->pre_import_treesearch_gtk2_c_1) {
        lx2_import_masked();
    }

    /* then do */
    if (action == SST_ACTION_INSERT || action == SST_ACTION_REPLACE ||
        action == SST_ACTION_PREPEND) {
        /* Save and clear current traces.  */
        memcpy(&GLOBALS->tcache_treesearch_gtk2_c_2, &GLOBALS->traces, sizeof(Traces));
        GLOBALS->traces.total = 0;
        GLOBALS->traces.first = GLOBALS->traces.last = NULL;
    }

    gtk_tree_selection_selected_foreach(GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                        &sig_selection_foreach,
                                        GINT_TO_POINTER(action));

    sig_selection_foreach_finalize(GINT_TO_POINTER(action));

    if (action == SST_ACTION_APPEND) {
        gw_signal_list_scroll_to_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), GLOBALS->traces.last);
    }
    redraw_signals_and_waves();
}

static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    set_window_busy(widget);
    action_callback(SST_ACTION_INSERT);
    set_window_idle(widget);
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    set_window_busy(widget);
    action_callback(SST_ACTION_REPLACE);
    set_window_idle(widget);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    set_window_busy(widget);
    action_callback(SST_ACTION_APPEND);
    set_window_idle(widget);
}

/**********************************************************************/

static gboolean view_selection_func(GtkTreeSelection *selection,
                                    GtkTreeModel *model,
                                    GtkTreePath *path,
                                    gboolean path_currently_selected,
                                    gpointer userdata)
{
    (void)selection;
    (void)userdata;

    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar *name;

        gtk_tree_model_get(model, &iter, 0, &name, -1);

        if (GLOBALS->selected_sig_name) {
            free_2(GLOBALS->selected_sig_name);
            GLOBALS->selected_sig_name = NULL;
        }

        if (!path_currently_selected) {
            GLOBALS->selected_sig_name = strdup_2(name);
            gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_SELECT, name, WAVE_TCLCB_TREE_SIG_SELECT_FLAGS);
        } else {
            gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_UNSELECT,
                              name,
                              WAVE_TCLCB_TREE_SIG_UNSELECT_FLAGS);
        }

        g_free(name);
    }
    return TRUE; /* allow selection state to change */
}

/**********************************************************************/

static gint button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
    (void)widget;

    if (event->type == GDK_2BUTTON_PRESS) {
        if (GLOBALS->selected_hierarchy_name && GLOBALS->selected_sig_name) {
            char *sstr = g_alloca(strlen(GLOBALS->selected_hierarchy_name) +
                                  strlen(GLOBALS->selected_sig_name) + 1);
            strcpy(sstr, GLOBALS->selected_hierarchy_name);
            strcat(sstr, GLOBALS->selected_sig_name);

            gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_DOUBLE_CLICK,
                              sstr,
                              WAVE_TCLCB_TREE_SIG_DOUBLE_CLICK_FLAGS);
            action_callback(GLOBALS->sst_dbl_action_type);
        }
    }

    return (FALSE);
}

static gint hier_top_button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
    if ((event->button == 3) && (event->type == GDK_BUTTON_PRESS)) {
        if (GLOBALS->sst_sig_root_treesearch_gtk2_c_1) {
            do_sst_popup_menu(widget, event);
            return (TRUE);
        }
    }

    return (FALSE);
}

/**********************************************************************/

/*
 * for use with expander in gtk2.4 and higher...
 */
GtkWidget *treeboxframe(const char *title)
{
    (void)title;

    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button4;
    GtkWidget *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *sig_view;

    GLOBALS->is_active_treesearch_gtk2_c_6 = 1;

    /* create a new modal window */

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_widget_show(vbox);

    vpan = gtk_paned_new(GTK_ORIENTATION_VERTICAL); /* GLOBALS->sst_vpaned is to be used to clone
                                                       position over during reload */
    GLOBALS->sst_vpaned = (GtkPaned *)vpan;
    if (GLOBALS->vpanedwindow_size_cache) {
        gtk_paned_set_position(GTK_PANED(GLOBALS->sst_vpaned), GLOBALS->vpanedwindow_size_cache);
        GLOBALS->vpanedwindow_size_cache = 0;
    }
    gtk_widget_show(vpan);
    gtk_box_pack_start(GTK_BOX(vbox), vpan, TRUE, TRUE, 1);

    gtk_widget_set_vexpand(vpan,
                           TRUE); /* needed for gtk3, otherwise box does not grow vertically */

    /* Hierarchy.  */
    GLOBALS->gtk2_tree_frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(GLOBALS->gtk2_tree_frame), 3);
    gtk_widget_show(GLOBALS->gtk2_tree_frame);

    gtk_paned_pack1(GTK_PANED(vpan), GLOBALS->gtk2_tree_frame, TRUE, FALSE);

    decorated_module_cleanup();
    XXX_maketree(NULL, GLOBALS->treeroot);
    gtk_tree_selection_set_select_function(
        gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->treeview_main)),
        XXX_view_selection_func,
        NULL,
        NULL);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->treeview_main)),
                                GTK_SELECTION_SINGLE);
    gtkwave_signal_connect_object(GLOBALS->treeview_main,
                                  "row-expanded",
                                  G_CALLBACK(XXX_tree_expand_callback),
                                  NULL);
    gtkwave_signal_connect_object(GLOBALS->treeview_main,
                                  "row-collapsed",
                                  G_CALLBACK(XXX_tree_collapse_callback),
                                  NULL);

    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add(GTK_CONTAINER(scrolled_win), GTK_WIDGET(GLOBALS->treeview_main));
    gtkwave_signal_connect(GLOBALS->treeview_main,
                           "button_press_event",
                           G_CALLBACK(hier_top_button_press_event_std),
                           NULL);

    gtk_container_add(GTK_CONTAINER(GLOBALS->gtk2_tree_frame), scrolled_win);

    /* Signal names.  */
    GLOBALS->sig_store_treesearch_gtk2_c_1 = gtk_list_store_new(N_COLUMNS,
                                                                G_TYPE_STRING,
                                                                G_TYPE_POINTER,
                                                                G_TYPE_STRING,
                                                                G_TYPE_STRING,
                                                                G_TYPE_STRING);
    GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = NULL;
    GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
    fill_sig_store();

    sig_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(GLOBALS->sig_store_treesearch_gtk2_c_1));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref(G_OBJECT(GLOBALS->sig_store_treesearch_gtk2_c_1));

    {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        renderer = gtk_cell_renderer_text_new();

        switch (GLOBALS->loaded_file_type) {
#ifdef EXTLOAD_SUFFIX
            case EXTLOAD_FILE:
#endif
            case FST_FILE:
                /* fallthrough for Dir is deliberate for extload and FST */
                if (GLOBALS->nonimplicit_direction_encountered) {
                    column = gtk_tree_view_column_new_with_attributes("Dir",
                                                                      renderer,
                                                                      "text",
                                                                      DIR_COLUMN,
                                                                      NULL);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(sig_view), column);
                }
                /* fallthrough */
            case VCD_RECODER_FILE:
            case DUMPLESS_FILE:
                column = gtk_tree_view_column_new_with_attributes(
                    ((GLOBALS->supplemental_datatypes_encountered) &&
                     (GLOBALS->supplemental_vartypes_encountered))
                        ? "VType"
                        : "Type",
                    renderer,
                    "text",
                    TYPE_COLUMN,
                    NULL);
                gtk_tree_view_append_column(GTK_TREE_VIEW(sig_view), column);
                if ((GLOBALS->supplemental_datatypes_encountered) &&
                    (GLOBALS->supplemental_vartypes_encountered)) {
                    column = gtk_tree_view_column_new_with_attributes("DType",
                                                                      renderer,
                                                                      "text",
                                                                      DTYPE_COLUMN,
                                                                      NULL);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(sig_view), column);
                }
                break;
            case GHW_FILE:
            case MISSING_FILE:
            default:
                break;
        }

        column = gtk_tree_view_column_new_with_attributes("Signals",
                                                          renderer,
                                                          "text",
                                                          NAME_COLUMN,
                                                          NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sig_view), column);

        /* Setup the selection handler */
        GLOBALS->sig_selection_treesearch_gtk2_c_1 =
            gtk_tree_view_get_selection(GTK_TREE_VIEW(sig_view));
        gtk_tree_selection_set_mode(GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                    GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function(GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                               view_selection_func,
                                               NULL,
                                               NULL);

        gtkwave_signal_connect(sig_view,
                               "button_press_event",
                               G_CALLBACK(button_press_event_std),
                               NULL);
    }

    GLOBALS->dnd_sigview = sig_view;

    sig_frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(sig_frame), 3);
    gtk_widget_show(sig_frame);

    gtk_paned_pack2(GTK_PANED(vpan), sig_frame, TRUE, FALSE);

    sig_scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(GTK_WIDGET(sig_scroll_win), 80, 100);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sig_scroll_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sig_scroll_win);
    gtk_container_add(GTK_CONTAINER(sig_frame), sig_scroll_win);
    gtk_container_add(GTK_CONTAINER(sig_scroll_win), sig_view);
    gtk_widget_show(sig_view);

    /* Filter.  */
    filter_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_container_set_border_width(GTK_CONTAINER(filter_hbox), 2);
    gtk_widget_show(filter_hbox);

    GLOBALS->filter_entry = gtk_search_entry_new();
    if (GLOBALS->filter_str_treesearch_gtk2_c_1) {
        gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry),
                           GLOBALS->filter_str_treesearch_gtk2_c_1);
    }
    gtk_widget_show(GLOBALS->filter_entry);

    gtkwave_signal_connect(GLOBALS->filter_entry, "activate", G_CALLBACK(press_callback), NULL);
    if (!GLOBALS->do_dynamic_treefilter) {
        gtkwave_signal_connect(GLOBALS->filter_entry,
                               "key_press_event",
                               (GCallback)filter_edit_cb,
                               NULL);
    } else {
        gtkwave_signal_connect(GLOBALS->filter_entry, "changed", G_CALLBACK(press_callback), NULL);
    }

    gtk_tooltips_set_tip_2(GLOBALS->filter_entry,
                           "Add a POSIX filter. "
                           "'.*' matches any number of characters,"
                           " '.' matches any character.  Hit Return to apply."
                           " The filter may be preceded with the port direction if it exists such "
                           "as ++ (show only non-port), +I+, +O+, +IO+, etc."
                           " Use -- to exclude all non-ports (i.e., show only all ports), -I- to "
                           "exclude all input ports, etc.");
    gtk_box_pack_start(GTK_BOX(filter_hbox), GLOBALS->filter_entry, TRUE, TRUE, 1);

    gtk_box_pack_start(GTK_BOX(vbox), filter_hbox, FALSE, FALSE, 1);

    /* Buttons.  */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_widget_show(hbox);

    button1 = gtk_button_new_with_label("Append");
    gtk_container_set_border_width(GTK_CONTAINER(button1), 3);
    gtkwave_signal_connect_object(button1,
                                  "clicked",
                                  G_CALLBACK(ok_callback),
                                  GLOBALS->gtk2_tree_frame);
    gtk_widget_show(button1);
    gtk_tooltips_set_tip_2(
        button1,
        "Add selected signal hierarchy to end of the display on the main window.");

    gtk_box_pack_start(GTK_BOX(hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label(" Insert ");
    gtk_container_set_border_width(GTK_CONTAINER(button2), 3);
    gtkwave_signal_connect_object(button2,
                                  "clicked",
                                  G_CALLBACK(insert_callback),
                                  GLOBALS->gtk2_tree_frame);
    gtk_widget_show(button2);
    gtk_tooltips_set_tip_2(
        button2,
        "Add selected signal hierarchy after last highlighted signal on the main window.");
    gtk_box_pack_start(GTK_BOX(hbox), button2, TRUE, FALSE, 0);

    button4 = gtk_button_new_with_label(" Replace ");
    gtk_container_set_border_width(GTK_CONTAINER(button4), 3);
    gtkwave_signal_connect_object(button4,
                                  "clicked",
                                  G_CALLBACK(replace_callback),
                                  GLOBALS->gtk2_tree_frame);
    gtk_widget_show(button4);
    gtk_tooltips_set_tip_2(
        button4,
        "Replace highlighted signals on the main window with signals selected above.");
    gtk_box_pack_start(GTK_BOX(hbox), button4, TRUE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 1);
    return vbox;
}

/****************************************************************
 **
 ** dnd
 **
 ****************************************************************/

/*
 *	DND "drag_begin" handler, this is called whenever a drag starts.
 */
static void DNDBeginCB(GtkWidget *widget, GdkDragContext *dc, gpointer data)
{
    (void)widget;
    (void)data;

    // Reset the hotspot to make sure the icon is shown at the correct position in
    // wayland.
    gdk_drag_context_set_hotspot(dc, 0, 0);

    // GtkTreeView sets the drag icon to an image of the dragged row, which can
    // hide a large part of the drag destination. By overriding the icon to the
    // default the destination highlight in the signal list is more visible.
    gtk_drag_set_icon_default(dc);

    GLOBALS->dnd_state = 1;
}

static void DNDEndCB(GtkWidget *widget, GdkDragContext *dc, gpointer data)
{
    (void)widget;
    (void)dc;
    (void)data;

    GLOBALS->dnd_state = 0;

    redraw_signals_and_waves();
}

/*
 *	DND "drag_data_get" handler, for handling requests for DND
 *	data on the specified widget. This function is called when
 *	there is need for DND data on the source, so this function is
 *	responsable for setting up the dynamic data exchange buffer
 *	(DDE as sometimes it is called) and sending it out.
 */
static void DNDDataRequestCBTree(GtkWidget *widget,
                                 GdkDragContext *dc,
                                 GtkSelectionData *selection_data,
                                 guint info,
                                 guint t,
                                 gpointer data)
{
    (void)widget;
    (void)dc;
    (void)info;
    (void)t;
    (void)data;

    int upd = 0;

    char *text = add_dnd_from_tree_window();
    if (text) {
        gtk_selection_data_set(selection_data,
                               GDK_SELECTION_TYPE_STRING,
                               8,
                               (guchar *)text,
                               strlen(text));
        free_2(text);
        upd = 1;
    }
    if (upd) {
        redraw_signals_and_waves();
    }
}

static void DNDDataRequestCBSearch(GtkWidget *widget,
                                   GdkDragContext *dc,
                                   GtkSelectionData *selection_data,
                                   guint info,
                                   guint t,
                                   gpointer data)
{
    (void)widget;
    (void)dc;
    (void)info;
    (void)t;
    (void)data;

    int upd = 0;

    char *text = add_dnd_from_searchbox();
    if (text) {
        gtk_selection_data_set(selection_data,
                               GDK_SELECTION_TYPE_STRING,
                               8,
                               (guchar *)text,
                               strlen(text));
        free_2(text);
        upd = 1;
    }

    if (upd) {
        redraw_signals_and_waves();
    }
}

/***********************/

void dnd_setup(GtkWidget *src, gboolean search)
{
    GtkTargetEntry target_entry[1];

    /* Set up the list of data format types that our DND
     * callbacks will accept.
     */
    target_entry[0].target = (char *)WAVE_DRAG_TARGET_TCL;
    target_entry[0].flags = 0;
    target_entry[0].info = WAVE_DRAG_INFO_TCL;

    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(src),
                                           GDK_BUTTON1_MASK,
                                           target_entry,
                                           G_N_ELEMENTS(target_entry),
                                           GDK_ACTION_COPY);

    // connect signal after the default handler to override the default icon
    g_signal_connect_after(src, "drag_begin", G_CALLBACK(DNDBeginCB), NULL);

    if (search) {
        gtkwave_signal_connect(src, "drag_data_get", G_CALLBACK(DNDDataRequestCBSearch), NULL);
    } else {
        gtkwave_signal_connect(src, "drag_data_get", G_CALLBACK(DNDDataRequestCBTree), NULL);
    }

    g_signal_connect(src, "drag_end", G_CALLBACK(DNDEndCB), NULL);
}

/***************************************************************************/

static void recurse_append_callback(GtkWidget *widget, gpointer data)
{
    int i;

    if (!GLOBALS->sst_sig_root_treesearch_gtk2_c_1 || !data)
        return;

    set_window_busy(widget);

    for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
        struct symbol *s;
        if (i < 0)
            break; /* GHW */
        s = GLOBALS->facs[i];
        if (s->vec_root) {
            set_s_selected(s->vec_root, GLOBALS->autocoalesce);
        }
    }

    /* LX2 */
    if (GLOBALS->is_lx2) {
        int pre_import = 0;

        for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
            struct symbol *s, *t;
            if (i < 0)
                break; /* GHW */
            s = GLOBALS->facs[i];
            t = s->vec_root;
            if ((t) && (GLOBALS->autocoalesce)) {
                if (get_s_selected(t)) {
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            pre_import++;
                        }
                        t = t->vec_chain;
                    }
                }
            } else {
                if (s->n->mv.mvlfac) {
                    lx2_set_fac_process_mask(s->n);
                    pre_import++;
                }
            }
        }

        if (pre_import) {
            lx2_import_masked();
        }
    }
    /* LX2 */

    for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
        int len;
        struct symbol *s, *t;
        if (i < 0)
            break; /* GHW */
        s = GLOBALS->facs[i];
        t = s->vec_root;
        if ((t) && (GLOBALS->autocoalesce)) {
            if (get_s_selected(t)) {
                set_s_selected(t, 0);
                len = 0;
                while (t) {
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        } else {
            AddNodeUnroll(s->n, NULL);
        }
    }

    set_window_idle(widget);

    gw_signal_list_scroll_to_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), GLOBALS->traces.last);
    redraw_signals_and_waves();
}

static void recurse_insert_callback(GtkWidget *widget, gpointer data)
{
    Traces tcache;
    int i;

    if (!GLOBALS->sst_sig_root_treesearch_gtk2_c_1 || !data)
        return;

    memcpy(&tcache, &GLOBALS->traces, sizeof(Traces));
    GLOBALS->traces.total = 0;
    GLOBALS->traces.first = GLOBALS->traces.last = NULL;

    set_window_busy(widget);

    for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
        struct symbol *s;
        if (i < 0)
            break; /* GHW */
        s = GLOBALS->facs[i];
        if (s->vec_root) {
            set_s_selected(s->vec_root, GLOBALS->autocoalesce);
        }
    }

    /* LX2 */
    if (GLOBALS->is_lx2) {
        int pre_import = 0;

        for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
            struct symbol *s, *t;
            if (i < 0)
                break; /* GHW */
            s = GLOBALS->facs[i];
            t = s->vec_root;
            if ((t) && (GLOBALS->autocoalesce)) {
                if (get_s_selected(t)) {
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            pre_import++;
                        }
                        t = t->vec_chain;
                    }
                }
            } else {
                if (s->n->mv.mvlfac) {
                    lx2_set_fac_process_mask(s->n);
                    pre_import++;
                }
            }
        }

        if (pre_import) {
            lx2_import_masked();
        }
    }
    /* LX2 */

    for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
        int len;
        struct symbol *s, *t;
        if (i < 0)
            break; /* GHW */
        s = GLOBALS->facs[i];
        t = s->vec_root;
        if ((t) && (GLOBALS->autocoalesce)) {
            if (get_s_selected(t)) {
                set_s_selected(t, 0);
                len = 0;
                while (t) {
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        } else {
            AddNodeUnroll(s->n, NULL);
        }
    }

    set_window_idle(widget);

    GLOBALS->traces.buffercount = GLOBALS->traces.total;
    GLOBALS->traces.buffer = GLOBALS->traces.first;
    GLOBALS->traces.bufferlast = GLOBALS->traces.last;
    GLOBALS->traces.first = tcache.first;
    GLOBALS->traces.last = tcache.last;
    GLOBALS->traces.total = tcache.total;

    PasteBuffer();

    GLOBALS->traces.buffercount = tcache.buffercount;
    GLOBALS->traces.buffer = tcache.buffer;
    GLOBALS->traces.bufferlast = tcache.bufferlast;

    redraw_signals_and_waves();
}

static void recurse_replace_callback(GtkWidget *widget, gpointer data)
{
    Traces tcache;
    int i;
    GwTrace *tfirst = NULL;
    GwTrace *tlast = NULL;

    if (!GLOBALS->sst_sig_root_treesearch_gtk2_c_1 || !data)
        return;

    memcpy(&tcache, &GLOBALS->traces, sizeof(Traces));
    GLOBALS->traces.total = 0;
    GLOBALS->traces.first = GLOBALS->traces.last = NULL;

    set_window_busy(widget);

    for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
        struct symbol *s;
        if (i < 0)
            break; /* GHW */
        s = GLOBALS->facs[i];
        if (s->vec_root) {
            set_s_selected(s->vec_root, GLOBALS->autocoalesce);
        }
    }

    /* LX2 */
    if (GLOBALS->is_lx2) {
        int pre_import = 0;

        for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
            struct symbol *s, *t;
            if (i < 0)
                break; /* GHW */
            s = GLOBALS->facs[i];
            t = s->vec_root;
            if ((t) && (GLOBALS->autocoalesce)) {
                if (get_s_selected(t)) {
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            pre_import++;
                        }
                        t = t->vec_chain;
                    }
                }
            } else {
                if (s->n->mv.mvlfac) {
                    lx2_set_fac_process_mask(s->n);
                    pre_import++;
                }
            }
        }

        if (pre_import) {
            lx2_import_masked();
        }
    }
    /* LX2 */

    for (i = GLOBALS->fetchlow; i <= GLOBALS->fetchhigh; i++) {
        int len;
        struct symbol *s, *t;
        if (i < 0)
            break; /* GHW */
        s = GLOBALS->facs[i];
        t = s->vec_root;
        if ((t) && (GLOBALS->autocoalesce)) {
            if (get_s_selected(t)) {
                set_s_selected(t, 0);
                len = 0;
                while (t) {
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        } else {
            AddNodeUnroll(s->n, NULL);
        }
    }

    set_window_idle(widget);

    tfirst = GLOBALS->traces.first;
    tlast = GLOBALS->traces.last; /* cache for highlighting */

    GLOBALS->traces.buffercount = GLOBALS->traces.total;
    GLOBALS->traces.buffer = GLOBALS->traces.first;
    GLOBALS->traces.bufferlast = GLOBALS->traces.last;
    GLOBALS->traces.first = tcache.first;
    GLOBALS->traces.last = tcache.last;
    GLOBALS->traces.total = tcache.total;

    {
        GwTrace *t = GLOBALS->traces.first;
        GwTrace **tp = NULL;
        int numhigh = 0;
        int it;

        while (t) {
            if (t->flags & TR_HIGHLIGHT) {
                numhigh++;
            }
            t = t->t_next;
        }
        if (numhigh) {
            tp = calloc_2(numhigh, sizeof(GwTrace *));
            t = GLOBALS->traces.first;
            it = 0;
            while (t) {
                if (t->flags & TR_HIGHLIGHT) {
                    tp[it++] = t;
                }
                t = t->t_next;
            }
        }

        PasteBuffer();

        GLOBALS->traces.buffercount = tcache.buffercount;
        GLOBALS->traces.buffer = tcache.buffer;
        GLOBALS->traces.bufferlast = tcache.bufferlast;

        for (i = 0; i < numhigh; i++) {
            tp[i]->flags |= TR_HIGHLIGHT;
        }

        t = tfirst;
        while (t) {
            t->flags &= ~TR_HIGHLIGHT;
            if (t == tlast)
                break;
            t = t->t_next;
        }

        CutBuffer();

        while (tfirst) {
            tfirst->flags |= TR_HIGHLIGHT;
            if (tfirst == tlast)
                break;
            tfirst = tfirst->t_next;
        }

        if (tp) {
            free_2(tp);
        }
    }

    redraw_signals_and_waves();
}

void recurse_import(GtkWidget *widget, guint callback_action)
{
    if (GLOBALS->sst_sig_root_treesearch_gtk2_c_1) {
        int fz;

        GLOBALS->fetchlow = GLOBALS->fetchhigh = -1;
        if (GLOBALS->sst_sig_root_treesearch_gtk2_c_1->child)
            recurse_fetch_high_low(GLOBALS->sst_sig_root_treesearch_gtk2_c_1->child);
        fz = GLOBALS->fetchhigh - GLOBALS->fetchlow + 1;
        void (*func)(GtkWidget *, gpointer);

        switch (callback_action) {
            case WV_RECURSE_INSERT:
                func = recurse_insert_callback;
                break;
            case WV_RECURSE_REPLACE:
                func = recurse_replace_callback;
                break;

            case WV_RECURSE_APPEND:
            default:
                func = recurse_append_callback;
                break;
        }

        if ((GLOBALS->fetchlow >= 0) && (GLOBALS->fetchhigh >= 0)) {
            widget = GLOBALS->mainwindow; /* otherwise using widget passed from the menu item
                                             crashes on OSX */

            if (fz > WV_RECURSE_IMPORT_WARN) {
                char recwarn[128];
                sprintf(recwarn, "Really import %d facilit%s?", fz, (fz == 1) ? "y" : "ies");

                simplereqbox("Recurse Warning", 300, recwarn, "Yes", "No", G_CALLBACK(func), 1);
            } else {
                func(widget, (gpointer)1);
            }
        }
    }
}

/***************************************************************************/
