#include "gw-plugin.h"

G_DEFINE_INTERFACE(GwPlugin, gw_plugin, G_TYPE_OBJECT)

static void gw_plugin_default_init(GwPluginInterface *self)
{
    (void)self;
}

void gw_plugin_activate(GwPlugin *self, GwProject *project)
{
    g_return_if_fail(GW_IS_PLUGIN(self));

    if (GW_PLUGIN_GET_IFACE(self)->activate == NULL) {
        return;
    }

    return GW_PLUGIN_GET_IFACE(self)->activate(self, project);
}