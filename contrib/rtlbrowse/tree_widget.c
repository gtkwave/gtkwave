/*
 * Copyright (c) Tony Bybell 1999-2008.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "splay.h"
#include "wavelink.h"
#include "gwr-model.h"
#include "stem_recurse.h"
void create_toolbar(GtkWidget *grid);
void switch_page_cb(GtkNotebook* self,GtkWidget* page,guint page_num);

GtkWidget *notebook = NULL;

static GtkWidget* create_tree(void);
void bwlogbox(char *title, int width, ds_Tree *t, int display_mode);

ds_Tree *selectedtree = NULL;
static GtkWidget *window;

/*
 * mainline..
 */
void treebox(const char *title, GtkApplication *app)
{
    GtkWidget *scrolled_tree_view;
    GtkWidget *paned;
    GtkWidget *grid;
    GtkWidget *tree;

    window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 600);

    grid = gtk_grid_new();

    paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_paned_set_end_child(GTK_PANED(paned), notebook);
    g_signal_connect(notebook, "switch_page", G_CALLBACK(switch_page_cb),NULL);


    gtk_grid_attach(GTK_GRID(grid), paned, 0, 0, 1, 1);
    gtk_widget_set_hexpand(GTK_WIDGET(paned), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(paned), TRUE);

    tree = create_tree();
    selectedtree = NULL;

    scrolled_tree_view = gtk_scrolled_window_new();
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_tree_view), 150, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_tree_view),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_tree_view), GTK_WIDGET(tree));
    gtk_paned_set_start_child(GTK_PANED(paned), scrolled_tree_view);

    create_toolbar(grid);

    gtk_window_set_child(GTK_WINDOW(window), grid);

    gtk_window_present(GTK_WINDOW(window));

}

static GListModel *
gwr_model_get_child_model (gpointer item, gpointer user_data)
{
    (void)user_data;
    GwrModule *self = item;

    if (gwr_model_get_children_model(self))
        return g_object_ref (G_LIST_MODEL (gwr_model_get_children_model(self)));

    return NULL;
}


static void
setup_listitem (GtkListItemFactory *factory,
                   GtkListItem        *item)
{
    (void)factory;

    GtkWidget *label;
    GtkWidget *expander;

    expander = gtk_tree_expander_new();
    label = gtk_label_new (NULL);

    gtk_label_set_xalign (GTK_LABEL (label), 0.);
#if GTK_CHECK_VERSION(4,12,0)
    gtk_list_item_set_focusable(GTK_LIST_ITEM(item), FALSE);
#endif
    gtk_tree_expander_set_child (GTK_TREE_EXPANDER (expander), label);

    gtk_list_item_set_child (item, expander);
}

static void
bind_listitem (GtkListItemFactory *factory,
                  GtkListItem        *item)
{
    (void)factory;

    GtkWidget *expander, *label;
    GtkTreeListRow *row;
    GwrModule *module;
    gchar *name;

    row = (gtk_list_item_get_item(item));
    module = GWR_MODULE(gtk_tree_list_row_get_item(row));

    expander = gtk_list_item_get_child (item);
    gtk_tree_expander_set_list_row (GTK_TREE_EXPANDER (expander), row);
    label = gtk_tree_expander_get_child (GTK_TREE_EXPANDER (expander));
    g_object_get(module, "name", &name, NULL);
    gtk_label_set_label(GTK_LABEL(label), name);
    g_free(name);

}
static void select_row_callback(GtkSingleSelection *sel,
              GParamSpec         *pspec,
              gpointer            user_data)
{
    (void)pspec; (void)user_data;

    GwrModule *t;
    ds_Tree *tree;
    GtkTreeListRow *row = gtk_single_selection_get_selected_item (sel);

    if(row == NULL) {
        return; // a non-existing row - should not happen
    }

    t = GWR_MODULE(gtk_tree_list_row_get_item(row));
    if(t == NULL) {
        return;
    }
    g_object_get(t, "tree", &tree, NULL);
    if(selectedtree != tree) {
        if(tree->filename) {
            bwlogbox(tree->fullname ? tree->fullname : "*", 640 + 8 * 8, tree, 0);
        }
    }

}


static GtkWidget* create_tree(void)
{
    GListModel *listmodel;
    GtkTreeListModel *treemodel;
    GtkWidget *listview;
    GtkSingleSelection *selection;
    GtkListItemFactory* factory;

    listmodel = create_module();
    treemodel = gtk_tree_list_model_new (G_LIST_MODEL (listmodel),
                                     FALSE,
                                     FALSE,
                                     gwr_model_get_child_model,
                                     NULL,
                                     NULL);

    factory = gtk_signal_list_item_factory_new ();
    g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem), NULL);
    g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem), NULL);

    selection = gtk_single_selection_new (G_LIST_MODEL (treemodel));
    g_signal_connect (selection, "notify::selected-item", G_CALLBACK (select_row_callback), NULL);

    listview = gtk_list_view_new(GTK_SELECTION_MODEL(selection), factory);
    gtk_list_view_set_model (GTK_LIST_VIEW (listview), GTK_SELECTION_MODEL (selection));

    select_row_callback (selection, NULL, NULL);
    return listview;
}