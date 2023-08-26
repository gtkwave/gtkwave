#include "gw-project.h"

struct _GwProject
{
    GObject parent_instance;

    GwNamedMarkers *named_markers;
};

G_DEFINE_TYPE(GwProject, gw_project, G_TYPE_OBJECT)

enum
{
    PROP_NAMED_MARKERS = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_project_dispose(GObject *object)
{
    GwProject *self = GW_PROJECT(object);

    g_clear_object(&self->named_markers);

    G_OBJECT_CLASS(gw_project_parent_class)->dispose(object);
}

static void gw_project_get_property(GObject *object,
                                    guint property_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
    GwProject *self = GW_PROJECT(object);

    switch (property_id) {
        case PROP_NAMED_MARKERS:
            g_value_set_object(value, gw_project_get_named_markers(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_project_class_init(GwProjectClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->dispose = gw_project_dispose;
    object_class->get_property = gw_project_get_property;

    properties[PROP_NAMED_MARKERS] = g_param_spec_object("named-markers",
                                                         "Named markers",
                                                         "The named markers",
                                                         GW_TYPE_NAMED_MARKERS,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_project_init(GwProject *self)
{
    self->named_markers = gw_named_markers_new(26); // TODO: support manymarkers
}

GwProject *gw_project_new(void)
{
    return g_object_new(GW_TYPE_PROJECT, NULL);
}

GwNamedMarkers *gw_project_get_named_markers(GwProject *self)
{
    g_return_val_if_fail(GW_IS_PROJECT(self), NULL);

    return self->named_markers;
}
