#pragma once

#include <glib-object.h>
#include "gw-project.h"

G_BEGIN_DECLS

#define GW_TYPE_PLUGIN (gw_plugin_get_type())
G_DECLARE_INTERFACE(GwPlugin, gw_plugin, GW, PLUGIN, GObject)

/**
 * GwPluginInterface:
 * @activate: Activates the plugin.
 */
struct _GwPluginInterface
{
    GTypeInterface parent_iface;

    void (*activate)(GwPlugin *self, GwProject *project);
};

void gw_plugin_activate(GwPlugin *self, GwProject *project);

G_END_DECLS
