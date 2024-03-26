#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GW_TYPE_ENUM_FILTER (gw_enum_filter_get_type())
G_DECLARE_FINAL_TYPE(GwEnumFilter, gw_enum_filter, GW, ENUM_FILTER, GObject)

GwEnumFilter *gw_enum_filter_new(void);

void gw_enum_filter_insert(GwEnumFilter *self, const gchar *key, const gchar *value);
const gchar *gw_enum_filter_lookup(GwEnumFilter *self, const gchar *value);

G_END_DECLS
