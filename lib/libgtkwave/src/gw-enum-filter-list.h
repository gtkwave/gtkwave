#pragma once

#include <glib-object.h>
#include "gw-enum-filter.h"

G_BEGIN_DECLS

#define GW_TYPE_ENUM_FILTER_LIST (gw_enum_filter_list_get_type())
G_DECLARE_FINAL_TYPE(GwEnumFilterList, gw_enum_filter_list, GW, ENUM_FILTER_LIST, GObject)

GwEnumFilterList *gw_enum_filter_list_new(void);

guint gw_enum_filter_list_add(GwEnumFilterList *self, GwEnumFilter *filter);
GwEnumFilter *gw_enum_filter_list_get(GwEnumFilterList *self, guint index);

G_END_DECLS
