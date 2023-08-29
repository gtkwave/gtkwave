#pragma once

#include <glib-object.h>
#include "gw-named-markers.h"
#include "gw-marker.h"

G_BEGIN_DECLS

#define GW_TYPE_PROJECT (gw_project_get_type())
G_DECLARE_FINAL_TYPE(GwProject, gw_project, GW, PROJECT, GObject)

GwProject *gw_project_new(void);

GwMarker *gw_project_get_cursor(GwProject *self);
GwMarker *gw_project_get_primary_marker(GwProject *self);
GwMarker *gw_project_get_baseline_marker(GwProject *self);
GwMarker *gw_project_get_ghost_marker(GwProject *self);

GwNamedMarkers *gw_project_get_named_markers(GwProject *self);

G_END_DECLS
