#pragma once

#include <gtk/gtk.h>

#define GWR_TYPE_MODULE (gwr_module_get_type ())
typedef struct _GwrModule GwrModule; // Comp Tree module

enum
{
    GWR_MODULE_PROP_0,
    GWR_MODULE_PROP_NAME,
    GWR_MODULE_PROP_TREE,

    GWR_MODULE_N_PROPS
  };

G_DECLARE_FINAL_TYPE(GwrModule, gwr_module, GWR, MODULE, GObject);

extern GtkWidget *notebook;

GListModel *
gwr_model_get_children_model (GwrModule *self);
void
gwr_model_set_children_model (GwrModule *self,
                             GListModel       *child);