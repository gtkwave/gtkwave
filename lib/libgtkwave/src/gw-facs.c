#include "gw-facs.h"

struct _GwFacs
{
    GObject parent_instance;

    GPtrArray *facs;
};

G_DEFINE_TYPE(GwFacs, gw_facs, G_TYPE_OBJECT)

enum
{
    PROP_LENGTH = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_facs_finalize(GObject *object)
{
    GwFacs *self = GW_FACS(object);

    g_ptr_array_free(self->facs, TRUE);

    G_OBJECT_CLASS(gw_facs_parent_class)->finalize(object);
}

static void gw_facs_set_property(GObject *object,
                                 guint property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    GwFacs *self = GW_FACS(object);

    switch (property_id) {
        case PROP_LENGTH:
            g_ptr_array_set_size(self->facs, g_value_get_uint(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_facs_get_property(GObject *object,
                                 guint property_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
    GwFacs *self = GW_FACS(object);

    switch (property_id) {
        case PROP_LENGTH:
            g_value_set_uint(value, gw_facs_get_length(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}
static void gw_facs_class_init(GwFacsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_facs_finalize;
    object_class->set_property = gw_facs_set_property;
    object_class->get_property = gw_facs_get_property;

    properties[PROP_LENGTH] =
        g_param_spec_uint("length",
                          NULL,
                          NULL,
                          0,
                          G_MAXUINT,
                          0,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_facs_init(GwFacs *self)
{
    self->facs = g_ptr_array_new();
}

GwFacs *gw_facs_new(guint length)
{
    return g_object_new(GW_TYPE_FACS, "length", length, NULL);
}

GwSymbol *gw_facs_get(GwFacs *self, guint index)
{
    g_return_val_if_fail(GW_IS_FACS(self), NULL);
    g_return_val_if_fail(index < self->facs->len, NULL);

    return g_ptr_array_index(self->facs, index);
}

guint gw_facs_get_length(GwFacs *self)
{
    g_return_val_if_fail(GW_IS_FACS(self), 0);

    return self->facs->len;
}
