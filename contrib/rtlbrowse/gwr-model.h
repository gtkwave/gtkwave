#pragma once

#include <gtk/gtk.h>

#define GWR_TYPE_MODULE (gwr_module_get_type ())

G_DECLARE_FINAL_TYPE(GwrModule, gwr_module, GWR, MODULE, GObject);


GListModel *
gwr_model_get_children_model (GwrModule *self);
void
gwr_model_set_children_model (GwrModule *self,
                             GListModel *child);