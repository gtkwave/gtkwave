#include "gw-facs.h"
#include "gw-util.h"

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

    if (self->facs != NULL) {
        g_ptr_array_free(self->facs, TRUE);
    }

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

void gw_facs_set(GwFacs *self, guint index, GwSymbol *symbol)
{
    g_return_if_fail(GW_IS_FACS(self));
    g_return_if_fail(index < self->facs->len);

    g_ptr_array_index(self->facs, index) = symbol;
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

// TODO: remove
GwSymbol **gw_facs_get_array(GwFacs *self)
{
    g_return_val_if_fail(GW_IS_FACS(self), NULL);

    return (GwSymbol **)self->facs->pdata;
}

void gw_facs_order_from_tree_rec(GwFacs *facs, GwFacs *sorted_facs, gint *pos, GwTreeNode *t)
{
    for (; t != NULL; t = t->next) {
        if (t->child != NULL) {
            gw_facs_order_from_tree_rec(facs, sorted_facs, pos, t->child);
        }

        /* for when valid netnames like A.B.C, A.B.C.D exist (not legal excluding texsim) */
        /* otherwise this would be an 'else' */
        if (t->t_which >= 0) {
            gw_facs_set(sorted_facs, *pos, gw_facs_get(facs, t->t_which));
            t->t_which = *pos;
            (*pos)--;
        }
    }
}

void gw_facs_order_from_tree(GwFacs *self, GwTree *tree)
{
    g_return_if_fail(GW_IS_FACS(self));
    g_return_if_fail(GW_IS_TREE(tree));

    // TODO: check for empty

    GwFacs *sorted_facs = gw_facs_new(gw_facs_get_length(self));
    gint pos = gw_facs_get_length(self) - 1;

    gw_facs_order_from_tree_rec(self, sorted_facs, &pos, gw_tree_get_root(tree));
    g_assert_cmpint(pos, <, 0);

    g_ptr_array_free(self->facs, TRUE);
    self->facs = g_steal_pointer(&sorted_facs->facs);

    g_object_unref(sorted_facs);
}

static int sigcmp(const void *v1, const void *v2)
{
    GwSymbol *a1 = *((GwSymbol **)v1);
    GwSymbol *a2 = *((GwSymbol **)v2);
    return gw_signal_name_compare(a1->name, a2->name);
}

void gw_facs_sort(GwFacs *self)
{
    g_return_if_fail(GW_IS_FACS(self));

    // TODO: Check which sorting algorithm is used by Glib. The original code
    // used a custom heapsort for some platforms, because quicksort can be very
    // slow if the facs are already sorted.
    g_ptr_array_sort(self->facs, sigcmp);
}
