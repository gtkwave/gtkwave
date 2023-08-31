#pragma once

#include <glib-object.h>
#include "gw-plugin.h"

G_BEGIN_DECLS

#define GW_TYPE_PLUGIN_MANAGER (gw_plugin_manager_get_type())
G_DECLARE_FINAL_TYPE(GwPluginManager, gw_plugin_manager, GW, PLUGIN_MANAGER, GObject)

GwPluginManager *gw_plugin_manager_new(void);
GSList *gw_plugin_manager_list_plugins(GwPluginManager *self);
GwPlugin *gw_plugin_manager_build(GwPluginManager *self, guint index);

G_END_DECLS
