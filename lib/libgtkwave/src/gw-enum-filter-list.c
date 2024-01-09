#include "gw-enum-filter-list.h"

struct _GwEnumFilterList
{
    GObject parent_instance;

    GPtrArray *filters;
};

G_DEFINE_TYPE(GwEnumFilterList, gw_enum_filter_list, G_TYPE_OBJECT)

static void gw_enum_filter_list_finalize(GObject *object)
{
    GwEnumFilterList *self = GW_ENUM_FILTER_LIST(object);

    g_ptr_array_free(self->filters, TRUE);

    G_OBJECT_CLASS(gw_enum_filter_list_parent_class)->finalize(object);
}

static void gw_enum_filter_list_class_init(GwEnumFilterListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_enum_filter_list_finalize;
}

static void gw_enum_filter_list_init(GwEnumFilterList *self)
{
    self->filters = g_ptr_array_new_with_free_func(g_object_unref);
}

GwEnumFilterList *gw_enum_filter_list_new(void)
{
    return g_object_new(GW_TYPE_ENUM_FILTER_LIST, NULL);
}

guint gw_enum_filter_list_add(GwEnumFilterList *self, GwEnumFilter *filter)
{
    g_return_val_if_fail(GW_IS_ENUM_FILTER_LIST(self), 0);
    g_return_val_if_fail(GW_IS_ENUM_FILTER(filter), 0);

    guint index = self->filters->len;

    g_ptr_array_add(self->filters, filter);

    return index;
}

GwEnumFilter *gw_enum_filter_list_get(GwEnumFilterList *self, guint index)
{
    g_return_val_if_fail(GW_IS_ENUM_FILTER_LIST(self), 0);

    if (index >= self->filters->len) {
        return NULL;
    }

    return g_ptr_array_index(self->filters, index);
}
