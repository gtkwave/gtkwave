#include "gw-loader.h"

typedef struct
{
    gboolean preserve_glitches;
    gboolean preserve_glitches_real;

    gboolean already_used;
} GwLoaderPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GwLoader, gw_loader, G_TYPE_OBJECT)

enum
{
    PROP_PRESERVE_GLITCHES = 1,
    PROP_PRESERVE_GLITCHES_REAL,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_loader_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
    GwLoader *self = GW_LOADER(object);

    switch (property_id) {
        case PROP_PRESERVE_GLITCHES:
            gw_loader_set_preserve_glitches(self, g_value_get_boolean(value));
            break;

        case PROP_PRESERVE_GLITCHES_REAL:
            gw_loader_set_preserve_glitches_real(self, g_value_get_boolean(value));
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

    switch (property_id) {
        case PROP_PRESERVE_GLITCHES:
            g_value_set_boolean(value, gw_loader_is_preserve_glitches(self));
            break;

        case PROP_PRESERVE_GLITCHES_REAL:
            g_value_set_boolean(value, gw_loader_is_preserve_glitches_real(self));
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

    properties[PROP_PRESERVE_GLITCHES] =
        g_param_spec_boolean("preserve-glitches",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_PRESERVE_GLITCHES_REAL] =
        g_param_spec_boolean("preserve-glitches-real",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

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

void gw_loader_set_preserve_glitches(GwLoader *self, gboolean preserve_glitches)
{
    g_return_if_fail(GW_IS_LOADER(self));

    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    preserve_glitches = !!preserve_glitches;

    if (priv->preserve_glitches != preserve_glitches) {
        priv->preserve_glitches = preserve_glitches;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PRESERVE_GLITCHES]);
    }
}

gboolean gw_loader_is_preserve_glitches(GwLoader *self)
{
    g_return_val_if_fail(GW_IS_LOADER(self), FALSE);

    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    return priv->preserve_glitches;
}

void gw_loader_set_preserve_glitches_real(GwLoader *self, gboolean preserve_glitches_real)
{
    g_return_if_fail(GW_IS_LOADER(self));

    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    preserve_glitches_real = !!preserve_glitches_real;

    if (priv->preserve_glitches_real != preserve_glitches_real) {
        priv->preserve_glitches_real = preserve_glitches_real;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_PRESERVE_GLITCHES_REAL]);
    }
}

gboolean gw_loader_is_preserve_glitches_real(GwLoader *self)
{
    g_return_val_if_fail(GW_IS_LOADER(self), FALSE);

    GwLoaderPrivate *priv = gw_loader_get_instance_private(self);

    return priv->preserve_glitches_real;
}