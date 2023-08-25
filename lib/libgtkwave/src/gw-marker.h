#pragma once

#include <glib-object.h>
#include "gw-time.h"

G_BEGIN_DECLS

#define GW_TYPE_MARKER (gw_marker_get_type())
G_DECLARE_FINAL_TYPE(GwMarker, gw_marker, GW, MARKER, GObject)

GwMarker *gw_marker_new(const char *name);

void gw_marker_set_position(GwMarker *self, GwTime position);
GwTime gw_marker_get_position(GwMarker *self);

void gw_marker_set_enabled(GwMarker *self, gboolean enabled);
gboolean gw_marker_is_enabled(GwMarker *self);

void gw_marker_set_alias(GwMarker *self, const gchar *alias);
const gchar *gw_marker_get_alias(GwMarker *self);

const gchar *gw_marker_get_name(GwMarker *self);
const gchar *gw_marker_get_display_name(GwMarker *self);

G_END_DECLS
