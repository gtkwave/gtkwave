#include "gwr-model.h"
#include "splay.h"

struct _GwrModule
{
    GObject parent_instance;

    char *name;
    ds_Tree *tree;
    GListModel *children_model;
}; // Comp Tree module

G_DEFINE_TYPE(GwrModule, gwr_module, G_TYPE_OBJECT);

enum
{
    PROP_NAME = 1,
    PROP_TREE,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES] = {
    NULL,
};

static void gwr_module_set_property(GObject *object,
                                    guint property_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
    GwrModule *self = GWR_MODULE(object);

    switch (property_id) {
        case PROP_NAME:
            self->name = g_value_dup_string(value);
            break;
        case PROP_TREE:
            self->tree = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void gwr_module_get_property(GObject *object,
                                    guint property_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
    GwrModule *self = GWR_MODULE(object);

    switch (property_id) {
        case PROP_NAME:
            g_value_set_string(value, self->name);
            break;
        case PROP_TREE:
            g_value_set_pointer(value, self->tree);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void gwr_model_finalize(GObject *object)
{
    GwrModule *module = GWR_MODULE(object);

    g_free(module->name);

    G_OBJECT_CLASS(gwr_module_parent_class)->finalize(object);
}

static void gwr_module_class_init(GwrModuleClass *class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(class);

    gobject_class->set_property = gwr_module_set_property;
    gobject_class->get_property = gwr_module_get_property;
    gobject_class->finalize = gwr_model_finalize;

    properties[PROP_NAME] =
        g_param_spec_string("name",
                            NULL,
                            NULL,
                            NULL,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    properties[PROP_TREE] =
        g_param_spec_pointer("tree",
                             NULL,
                             NULL,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(gobject_class, N_PROPERTIES, properties);
}

static void gwr_module_init(GwrModule *object)
{
    (void)object;
}

GListModel *gwr_model_get_children_model(GwrModule *self)
{
    g_return_val_if_fail(GWR_IS_MODULE(self), NULL);

    return self->children_model;
}

void gwr_model_set_children_model(GwrModule *self, GListModel *child)
{
    g_return_if_fail(GWR_IS_MODULE(self));
    g_return_if_fail(child != NULL);

    self->children_model = child;
}
