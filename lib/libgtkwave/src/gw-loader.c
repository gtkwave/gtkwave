#include "gw-loader.h"

G_DEFINE_BOXED_TYPE(GwLoaderSettings,
                    gw_loader_settings,
                    gw_loader_settings_copy,
                    gw_loader_settings_free)

GwLoaderSettings *gw_loader_settings_new(void)
{
    GwLoaderSettings *self = g_new0(GwLoaderSettings, 1);

    return self;
}

GwLoaderSettings *gw_loader_settings_copy(const GwLoaderSettings *self)
{
    GwLoaderSettings *copy = g_new(GwLoaderSettings, 1);
    *copy = *self;

    return copy;
}

void gw_loader_settings_free(GwLoaderSettings *self)
{
    g_free(self);
}

typedef struct
{
    gboolean already_used;
    GwLoaderSettings *settings;
} GwLoaderPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GwLoader, gw_loader, G_TYPE_OBJECT)

enum
{
    PROP_SETTINGS = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_loader_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
    GwLoader *self = GW_LOADER(object);
    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    switch (property_id) {
        case PROP_SETTINGS:
            g_assert_null(priv->settings);
            priv->settings = g_value_get_boxed(value);
            if (priv->settings == NULL) {
                priv->settings = gw_loader_settings_new();
            }
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_loader_get_property(GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
    GwLoader *self = GW_LOADER(object);
    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    switch (property_id) {
        case PROP_SETTINGS:
            g_value_set_boxed(value, priv->settings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_loader_class_init(GwLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = gw_loader_set_property;
    object_class->get_property = gw_loader_get_property;

    properties[PROP_SETTINGS] =
        g_param_spec_boxed("settings",
                           NULL,
                           NULL,
                           GW_TYPE_LOADER_SETTINGS,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_BLURB);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_loader_init(GwLoader *self)
{
    (void)self;
}

/**
 * gw_loader_load:
 * @self: A #GwLoader.
 * @path: The dump file path.
 * @error: The return location for a #GError or %NULL.
 *
 * Loads a dump file.
 *
 * Returns: (transfer full): The loaded #GwDumpFile.
 */
GwDumpFile *gw_loader_load(GwLoader *self, const gchar *path, GError **error)
{
    g_return_val_if_fail(GW_IS_LOADER(self), NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    // Loaders must only be used once.
    g_return_val_if_fail(!priv->already_used, NULL);

    g_return_val_if_fail(GW_LOADER_GET_CLASS(self)->load != NULL, NULL);
    GwDumpFile *file = GW_LOADER_GET_CLASS(self)->load(self, path, error);

    priv->already_used = TRUE;

    return file;
}

const GwLoaderSettings *gw_loader_get_settings(GwLoader *self)
{
    g_return_val_if_fail(GW_IS_LOADER(self), NULL);

    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    return priv->settings;
}