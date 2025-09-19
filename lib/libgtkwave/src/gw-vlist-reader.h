#pragma once

typedef struct _GwVlistWriter GwVlistWriter;

#include <glib-object.h>
#include "gw-vlist.h"

G_BEGIN_DECLS

#define GW_TYPE_VLIST_READER (gw_vlist_reader_get_type())
G_DECLARE_FINAL_TYPE(GwVlistReader, gw_vlist_reader, GW, VLIST_READER, GObject)

GwVlistReader *gw_vlist_reader_new(GwVlist *vlist, gboolean prepacked);

gboolean gw_vlist_reader_is_done(GwVlistReader *self);
gint gw_vlist_reader_next(GwVlistReader *self);
guint32 gw_vlist_reader_read_uv32(GwVlistReader *self);
const gchar *gw_vlist_reader_read_string(GwVlistReader *self);

void gw_vlist_reader_set_position(GwVlistReader *self, guint position);

GwVlistReader *gw_vlist_reader_new_from_writer(GwVlistWriter *writer);

// void gw_vlist_writer_append_string(GwVlistWriter *self, const gchar *str);
// void gw_vlist_writer_append_mvl9_string(GwVlistWriter *self, const char *str);

// GwVlist *gw_vlist_writer_finish(GwVlistWriter *self);

G_END_DECLS
