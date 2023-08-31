#include "gw-marker.h"
#include "gw-util.h"

/**
 * GwMarker:
 *
 * A named or unnamed markers.
 */

struct _GwMarker
{
    GObject parent_instance;

    GwTime position;
    gboolean enabled;
    gchar *name;
    gchar *alias;
};

G_DEFINE_TYPE(GwMarker, gw_marker, G_TYPE_OBJECT)

enum
{
    PROP_POSITION = 1,
    PROP_ENABLED,
    PROP_NAME,
    PROP_ALIAS,
    PROP_DISPLAY_NAME,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_marker_finalize(GObject *object)
{
    GwMarker *self = GW_MARKER(object);

    g_clear_pointer(&self->name, g_free);

    G_OBJECT_CLASS(gw_marker_parent_class)->finalize(object);
}

static void gw_marker_set_property(GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
    GwMarker *self = GW_MARKER(object);

    switch (property_id) {
        case PROP_POSITION:
            gw_marker_set_position(self, g_value_get_int64(value));
            break;

        case PROP_ENABLED:
            gw_marker_set_enabled(self, g_value_get_boolean(value));
            break;

        case PROP_NAME:
            self->name = g_value_dup_string(value);
            if (self->name == NULL) {
                self->name = g_strdup("");
            }
            break;

        case PROP_ALIAS:
            gw_marker_set_alias(self, g_value_get_string(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_marker_get_property(GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
    GwMarker *self = GW_MARKER(object);

    switch (property_id) {
        case PROP_POSITION:
            g_value_set_int64(value, gw_marker_get_position(self));
            break;

        case PROP_ENABLED:
            g_value_set_boolean(value, gw_marker_is_enabled(self));
            break;

        case PROP_NAME:
            g_value_set_string(value, gw_marker_get_name(self));
            break;

        case PROP_ALIAS:
            g_value_set_string(value, gw_marker_get_alias(self));
            break;

        case PROP_DISPLAY_NAME:
            g_value_set_string(value, gw_marker_get_display_name(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_marker_class_init(GwMarkerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_marker_finalize;
    object_class->set_property = gw_marker_set_property;
    object_class->get_property = gw_marker_get_property;

    properties[PROP_POSITION] =
        gw_param_spec_time("position",
                           "Position",
                           "The time position",
                           G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_ENABLED] =
        g_param_spec_boolean("enabled",
                             "Enabled",
                             "Wether the marker is enabled",
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_NAME] =
        g_param_spec_string("name",
                            "Name",
                            "The name",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    properties[PROP_ALIAS] =
        g_param_spec_string("alias",
                            "Alias",
                            "The alias",
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_DISPLAY_NAME] =
        g_param_spec_string("display-name",
                            "Display name",
                            "The name or alias, if the alias is not NULL",
                            FALSE,
                            G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_marker_init(GwMarker *self)
{
    (void)self;
}

/**
 * gw_marker_new:
 * @name: The marker name.
 *
 * Creates a marker.
 *
 * Returns: (transfer full): A new instance of #GwMarker.
 */
GwMarker *gw_marker_new(const gchar *name)
{
    g_return_val_if_fail(name != NULL, NULL);

    return g_object_new(GW_TYPE_MARKER, "name", name, NULL);
}

/**
 * gw_marker_set_position:
 * @self: A #GwMarker.
 * @position: The marker position.
 *
 * Moves the marker to the given position.
 */
void gw_marker_set_position(GwMarker *self, GwTime position)
{
    g_return_if_fail(GW_IS_MARKER(self));

    if (self->position != position) {
        self->position = position;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_POSITION]);
    }
}

/**
 * gw_marker_get_position:
 * @self: A #GwMarker.
 *
 * Returns the marker position.
 *
 * Returns: The marker position.
 */
GwTime gw_marker_get_position(GwMarker *self)
{
    g_return_val_if_fail(GW_IS_MARKER(self), 0);

    return self->position;
}

/**
 * gw_marker_set_enabled:
 * @self: A #GwMarker.
 * @enabled: Whether the marker should be enabled.
 *
 * Enables or disables the marker.
 */
void gw_marker_set_enabled(GwMarker *self, gboolean enabled)
{
    g_return_if_fail(GW_IS_MARKER(self));

    if (self->enabled != enabled) {
        self->enabled = enabled;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ENABLED]);
    }
}

/**
 * gw_marker_get_enabled:
 * @self: A #GwMarker.
 *
 * Returns wether the marker is enabled.
 *
 * Returns: %TRUE if the marker is enabled.
 */
gboolean gw_marker_is_enabled(GwMarker *self)
{
    g_return_val_if_fail(GW_IS_MARKER(self), FALSE);

    return self->enabled;
}

/**
 * gw_marker_set_alias:
 * @self: A #GwMarker.
 * @alias: (nullable): The maker alias.
 *
 * Sets the marker alias.
 *
 * If @alias is %NULL the current alias will be removed.
 */
void gw_marker_set_alias(GwMarker *self, const gchar *alias)
{
    g_return_if_fail(GW_IS_MARKER(self));

    if (alias != NULL && alias[0] == '\0') {
        alias = NULL;
    }

    if (g_strcmp0(self->alias, alias) != 0) {
        g_clear_pointer(&self->alias, g_free);
        self->alias = g_strdup(alias);

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_ALIAS]);
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_DISPLAY_NAME]);
    }
}

/**
 * gw_marker_get_alias:
 * @self: A #GwMarker.
 *
 * Returns the alias.
 *
 * Returns: (nullable): The alias or %NULL if no alias is set .
 */
const gchar *gw_marker_get_alias(GwMarker *self)
{
    g_return_val_if_fail(GW_IS_MARKER(self), FALSE);

    return self->alias;
}

/**
 * gw_marker_get_name:
 * @self: A #GwMarker.
 *
 * Returns the name.
 *
 * Returns: (not nullable): The name.
 */
const gchar *gw_marker_get_name(GwMarker *self)
{
    g_return_val_if_fail(GW_IS_MARKER(self), FALSE);

    return self->name;
}

/**
 * gw_marker_get_display_name:
 * @self: A #GwMarker.
 *
 * Returns the display name.
 *
 * Returns: (not nullable): The name or the alias if the alias is not %NULL.
 */
const gchar *gw_marker_get_display_name(GwMarker *self)
{
    g_return_val_if_fail(GW_IS_MARKER(self), FALSE);

    return self->alias != NULL ? self->alias : self->name;
}
