#include "gw-enum-filter.h"

static gint strcmp_wrapper(gconstpointer a, gconstpointer b, gpointer data)
{
    (void)data;
    return g_ascii_strcasecmp(a, b);
}

struct _GwEnumFilter
{
    GObject parent_instance;

    GTree *values;
};

G_DEFINE_TYPE(GwEnumFilter, gw_enum_filter, G_TYPE_OBJECT)

static void gw_enum_filter_finalize(GObject *object)
{
    GwEnumFilter *self = GW_ENUM_FILTER(object);

    g_tree_destroy(self->values);

    G_OBJECT_CLASS(gw_enum_filter_parent_class)->finalize(object);
}

static void gw_enum_filter_class_init(GwEnumFilterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_enum_filter_finalize;
}

static void gw_enum_filter_init(GwEnumFilter *self)
{
    self->values = g_tree_new_full(strcmp_wrapper, NULL, g_free, g_free);
}

GwEnumFilter *gw_enum_filter_new(void)
{
    return g_object_new(GW_TYPE_ENUM_FILTER, NULL);
}

void gw_enum_filter_insert(GwEnumFilter *self, const char *key, const char *value)
{
    g_return_if_fail(GW_IS_ENUM_FILTER(self));
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);

    // Don't allow redefinition of existing value.
    g_return_if_fail(gw_enum_filter_lookup(self, key) == NULL);

    g_tree_insert(self->values, g_strdup(key), g_strdup(value));
}

const char *gw_enum_filter_lookup(GwEnumFilter *self, const char *value)
{
    g_return_val_if_fail(GW_IS_ENUM_FILTER(self), NULL);
    g_return_val_if_fail(value != NULL, NULL);

    return g_tree_lookup(self->values, value);
}
