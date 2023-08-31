#pragma once

#include <glib-object.h>
#include "gw-time.h"
#include "gw-marker.h"

G_BEGIN_DECLS

#define GW_TYPE_NAMED_MARKERS (gw_named_markers_get_type())
G_DECLARE_FINAL_TYPE(GwNamedMarkers, gw_named_markers, GW, NAMED_MARKERS, GObject)

GwNamedMarkers *gw_named_markers_new(gsize number_of_markers);

guint gw_named_markers_get_number_of_markers(GwNamedMarkers *self);

GwMarker *gw_named_markers_get(GwNamedMarkers *self, guint index);
GwMarker *gw_named_markers_find(GwNamedMarkers *self, GwTime time);
GwMarker *gw_named_markers_find_first_disabled(GwNamedMarkers *self);
GwMarker *gw_named_markers_find_closest(GwNamedMarkers *self, GwTime time, GwTime *delta);

void gw_named_markers_foreach(GwNamedMarkers *self, GFunc func, gpointer user_data);

G_END_DECLS
