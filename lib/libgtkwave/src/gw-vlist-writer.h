#pragma once

#include <glib-object.h>
#include "gw-vlist.h"

G_BEGIN_DECLS

#define GW_TYPE_VLIST_WRITER (gw_vlist_writer_get_type())
G_DECLARE_FINAL_TYPE(GwVlistWriter, gw_vlist_writer, GW, VLIST_WRITER, GObject)

GwVlistWriter *gw_vlist_writer_new(gint compression_level, gboolean prepack);

void gw_vlist_writer_append_uv32(GwVlistWriter *self, guint32 value);
void gw_vlist_writer_append_string(GwVlistWriter *self, const gchar *str);
void gw_vlist_writer_append_mvl9_string(GwVlistWriter *self, const char *str);

GwVlist *gw_vlist_writer_finish(GwVlistWriter *self);

void gw_vlist_writer_set_live_mode(GwVlistWriter *self, gboolean is_live);

GwVlist *gw_vlist_writer_get_vlist(GwVlistWriter *self);
gboolean gw_vlist_writer_get_is_live(GwVlistWriter *self);
gboolean gw_vlist_writer_get_prepack(GwVlistWriter *self);

G_END_DECLS
