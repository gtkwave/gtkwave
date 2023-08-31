#include <libpeas.h>
#include "gw-plugin-manager.h"
#include "gw-plugin.h"

struct _GwPluginManager
{
    GObject parent_instance;
};

G_DEFINE_TYPE(GwPluginManager, gw_plugin_manager, G_TYPE_OBJECT)

static void gw_plugin_manager_class_init(GwPluginManagerClass *klass)
{
}

static void gw_plugin_manager_init(GwPluginManager *self)
{
    PeasEngine *engine = peas_engine_get_default();
    peas_engine_enable_loader(engine, "python");
    peas_engine_enable_loader(engine, "gjs");

    gchar *path = g_build_filename(g_get_user_data_dir(), "gtkwave", "plugins", NULL);
    peas_engine_add_search_path(engine, path, NULL);

    g_printerr("path: %s\n", path);
}

GwPluginManager *gw_plugin_manager_new(void)
{
    return g_object_new(GW_TYPE_PLUGIN_MANAGER, NULL);
}

GwPlugin *gw_plugin_manager_build(GwPluginManager *self, guint index)
{
    PeasEngine *engine = peas_engine_get_default();
    PeasPluginInfo *info = g_list_model_get_item(G_LIST_MODEL(engine), index);
    g_assert_true(peas_engine_load_plugin(engine, info));

    GwPlugin *plugin = GW_PLUGIN(peas_engine_create_extension(engine, info, GW_TYPE_PLUGIN, FALSE));
    g_assert_true(G_IS_OBJECT(info));

    g_object_unref(info);

    return plugin;
}
