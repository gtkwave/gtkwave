#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GW_TYPE_STRING_TABLE (gw_string_table_get_type())
G_DECLARE_FINAL_TYPE(GwStringTable, gw_string_table, GW, STRING_TABLE, GObject)

GwStringTable *gw_string_table_new(void);

guint gw_string_table_add(GwStringTable *self, const gchar *str);
const gchar *gw_string_table_get(GwStringTable *self, guint index);

void gw_string_table_freeze(GwStringTable *self);

G_END_DECLS
