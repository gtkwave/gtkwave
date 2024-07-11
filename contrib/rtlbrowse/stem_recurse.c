/*
 * Copyright (c) Tony Bybell 2006-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "splay.h"
#include "wavelink.h"
#include "gwr-model.h"

extern GtkWidget *notebook;
void treebox(char *title, GCallback func, GtkWidget *old_window, GtkApplication *app);
gboolean update_ctx_when_idle(gpointer dummy);

int verilog_2005 = 0; /* currently 1364-2005 keywords are disabled */

int mod_cnt;
ds_Tree **mod_list;
ds_Tree *flattened_mod_list_root = NULL;

struct gtkwave_annotate_ipc_t *anno_ctx = NULL;
struct vzt_rd_trace *vzt = NULL;
struct lxt2_rd_trace *lx2 = NULL;
void *fst = NULL;
int64_t timezero = 0; /* only currently used for FST */

static int bwsigcmp(char *s1, char *s2)
{
    unsigned char c1, c2;
    int u1, u2;

    for (;;) {
        c1 = (unsigned char)*(s1++);
        c2 = (unsigned char)*(s2++);

        if ((!c1) && (!c2))
            return (0);
        if ((c1 <= '9') && (c2 <= '9') && (c2 >= '0') && (c1 >= '0')) {
            u1 = (int)(c1 & 15);
            u2 = (int)(c2 & 15);

            while (((c2 = (unsigned char)*s2) >= '0') && (c2 <= '9')) {
                u2 *= 10;
                u2 += (unsigned int)(c2 & 15);
                s2++;
            }

            while (((c2 = (unsigned char)*s1) >= '0') && (c2 <= '9')) {
                u1 *= 10;
                u1 += (unsigned int)(c2 & 15);
                s1++;
            }

            if (u1 == u2)
                continue;
            else
                return ((int)u1 - (int)u2);
        } else {
            if (c1 != c2)
                return ((int)c1 - (int)c2);
        }
    }
}

static int compar_comp_array_bsearch(const void *s1, const void *s2)
{
    char *key, *obj;

    key = (*((struct ds_component **)s1))->compname;
    obj = (*((struct ds_component **)s2))->compname;

    return (bwsigcmp(key, obj));
}

void recurse_into_modules(char *comp_path_name,
                          char *comp_name,
                          ds_Tree *tree,
                          GListStore *parent)
{
    struct ds_component *comp; // Current node's comp
    char *comp_name_full; // e.g. top.adder.add
    ds_Tree *tree_dup = malloc(sizeof(ds_Tree));
    char *colon;
    char *compname_colon = NULL;

    if(comp_name) {
        compname_colon = strchr(comp_name, ':');
        if (compname_colon) {
            *compname_colon = 0;
        }
    }

    memcpy(tree_dup, tree, sizeof(ds_Tree));
    tree = tree_dup;
    colon = strchr(tree->item, ':');
    if(colon) {
        *colon = 0; /* remove generate hack */
    }

    tree->next_flat = flattened_mod_list_root;
    flattened_mod_list_root = tree;

    if (comp_path_name) {
        int path_len = strlen(comp_path_name);

        comp_name_full = malloc(path_len + 1 + strlen(comp_name) + 1);
        sprintf(comp_name_full, "%s.%s", comp_path_name, comp_name);
    } else {
        comp_name_full = strdup(tree->item);
    }

    tree->fullname = comp_name_full;

    GwrModule *node;
    node = GWR_MODULE(g_object_new(GWR_TYPE_MODULE,
    "name", comp_name ? strdup(comp_name) : tree->item,
    "tree", tree,
    NULL));

    g_list_store_append(parent, node);

    if(colon) {
        *colon = ':';
    }

    comp = tree->comp;
    if(comp) {
        int comp_size = 0;
        struct ds_component **comp_array; // array of every elements in comp linklist
        while(comp) {
            comp_size ++;
            comp = comp->next;
        }

        comp_array = calloc(comp_size, sizeof(struct  ds_component*));

        comp = tree->comp;
        for(int i = 0; i < comp_size; i++) {
            comp_array[i] = comp;
            comp = comp->next;
        }
        qsort(comp_array, comp_size, sizeof(struct ds_component*), compar_comp_array_bsearch);

        gwr_model_set_children_model(node, G_LIST_MODEL(g_list_store_new(GWR_TYPE_MODULE)));
        for(int i = 0; i < comp_size; i++) {
            comp = comp_array[i];
            recurse_into_modules(comp_name_full, comp->compname, comp->module, G_LIST_STORE(gwr_model_get_children_model(node)));
        }
        free(comp_array);
    }
}

/* Recursively counts the total number of nodes in ds_Tree */
void rec_tree(ds_Tree *t, int *cnt)
{
    if (!t)
        return;

    if (t->left) {
        rec_tree(t->left, cnt);
    }

    (*cnt)++;

    if (t->right) {
        rec_tree(t->right, cnt);
    }
}

/* Recursively populates an array with ds_tree_node in inorder traversal sequence */
void rec_tree_populate(ds_Tree *t, int *cnt, ds_Tree **list_root)
{
    if (!t)
        return;

    if (t->left) {
        rec_tree_populate(t->left, cnt, list_root);
    }

    list_root[*cnt] = t;
    (*cnt)++;

    if (t->right) {
        rec_tree_populate(t->right, cnt, list_root);
    }
}

ds_Tree *load_stems_file(FILE *f)
{
    ds_Tree *modules = NULL;
    while (!feof(f)) {
        char *ln = fgetmalloc(f);

        if (fgetmalloc_len > 4) {
            if ((ln[0] == '+') && (ln[1] == '+') && (ln[2] == ' ')) {
                if (ln[3] == 'c') {
                    char cname[1024], mname[1024], pname[1024], scratch[128];
                    ds_Tree *which_module;
                    struct ds_component *dc;

                    sscanf(ln + 8, "%s %s %s %s %s", cname, scratch, mname, scratch, pname);
                    /* printf("comp: %s module: %s, parent: %s\n", cname, mname, pname); */

                    modules = ds_splay(mname, modules);
                    if ((!modules) || (strcmp(modules->item, mname))) {
                        modules = ds_insert(strdup(mname), modules);
                    }
                    which_module = modules;
                    which_module->refcnt++;

                    modules = ds_splay(pname, modules);
                    if (strcmp(modules->item, pname)) {
                        modules = ds_insert(strdup(pname), modules);
                    }

                    dc = calloc(1, sizeof(struct ds_component));
                    dc->compname = strdup(cname);
                    dc->module = which_module;
                    dc->next = modules->comp;
                    modules->comp = dc;
                } else if ((ln[3] == 'm') || (ln[3] == 'u')) {
                    char scratch[128], mname[1024], fname[1024];
                    int s_line, e_line;

                    sscanf(ln + 3,
                           "%s %s %s %s %s %d %s %d",
                           scratch,
                           mname,
                           scratch,
                           fname,
                           scratch,
                           &s_line,
                           scratch,
                           &e_line);
                    /* printf("mod: %s from %s %d-%d\n", mname, fname, s_line, e_line); */

                    modules = ds_insert(strdup(mname), modules);
                    modules->filename = strdup(fname);
                    modules->s_line = s_line;
                    modules->e_line = e_line;
                    modules->resolved = 1;
                } else if (ln[3] == 'v') {
                }
            }
        }

        free(ln);
    }
    return modules;
}

// Create a suitable data module for tree list from mod_list
GListModel * create_module(void)
{
    GListStore *root = g_list_store_new(GWR_TYPE_MODULE);
    for (int i = 0; i < mod_cnt; i++) {
        ds_Tree *t = mod_list[i];

        if (!t->refcnt) {
            /* printf("TOP: %s\n", t->item); */
            // Find the top module
            recurse_into_modules(NULL, NULL, t, root);
        } else if (!t->resolved) {
            /* printf("MISSING: %s\n", t->item); */
        }
    }
    return G_LIST_MODEL(root);
}