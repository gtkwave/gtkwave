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
#include "tree_widget.h"
#include "stem_recurse.h"

void create_toolbar(GtkWidget *grid);
void switch_page_cb(GtkNotebook* self,GtkWidget* page,guint page_num);

GtkWidget *notebook = NULL;

static GtkWidget* create_tree(void);
void bwlogbox(char *title, int width, ds_Tree *t, int display_mode);
void setup_dnd(GtkWidget *wid);

ds_Tree *selectedtree = NULL;
static GtkWidget *window;

struct _GwrModule
{
    GObject parent_instance;

    const char *name;
    ds_Tree* tree;
    GListModel *children_model;
};

G_DEFINE_TYPE(GwrModule, gwr_module, G_TYPE_OBJECT);

static GParamSpec *module_properties[GWR_MODULE_N_PROPS] = { NULL, };

static void
gwr_module_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
    GwrModule *self = GWR_MODULE (object);

    switch(property_id) {
        case GWR_MODULE_PROP_NAME:
            self->name = g_value_get_string(value);
        break;
        case GWR_MODULE_PROP_TREE:
            self->tree = g_value_get_pointer(value);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void
gwr_module_class_init(GwrModuleClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (class);

    gobject_class->set_property = gwr_module_set_property;

    module_properties[GWR_MODULE_PROP_NAME] =
        g_param_spec_string("name",
                            "name",
                            "name",
                            NULL,
                            G_PARAM_WRITABLE);
    module_properties[GWR_MODULE_PROP_TREE] =
        g_param_spec_pointer("tree",
                             "tree",
                             "tree",
                             G_PARAM_WRITABLE);

    g_object_class_install_properties(gobject_class, GWR_MODULE_N_PROPS, module_properties);
}

static void
gwr_module_init(GwrModule *object)
{
    (void) object;
}

GListModel *
gwr_model_get_children_model (GwrModule *self)
{
    g_return_val_if_fail (GWR_IS_MODULE (self), NULL);

    return self->children_model;
}

void
gwr_model_set_children_model (GwrModule *self,
                             GListModel       *child)
{
    g_return_if_fail (GWR_IS_MODULE (self));

    if(child == NULL) {
        return;
    }

    if (self->children_model == child)
        return;

    self->children_model = child;

}
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

    //setup_dnd(window);
}

static GListModel *
gwr_model_get_child_model (gpointer item, gpointer user_data)
{
    (void)user_data;
    GwrModule *self = item;

    if (self->children_model)
        return g_object_ref (G_LIST_MODEL (self->children_model));

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

    row = (gtk_list_item_get_item(item));
    module = GWR_MODULE(gtk_tree_list_row_get_item(row));

    expander = gtk_list_item_get_child (item);
    gtk_tree_expander_set_list_row (GTK_TREE_EXPANDER (expander), row);
    label = gtk_tree_expander_get_child (GTK_TREE_EXPANDER (expander));
    gtk_label_set_label (GTK_LABEL (label), module->name);

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
    tree = t->tree;
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