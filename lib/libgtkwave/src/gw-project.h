#pragma once

#include <glib-object.h>
#include "gw-named-markers.h"

G_BEGIN_DECLS

#define GW_TYPE_PROJECT (gw_project_get_type())
G_DECLARE_FINAL_TYPE(GwProject, gw_project, GW, PROJECT, GObject)

GwProject *gw_project_new(void);

GwNamedMarkers *gw_project_get_named_markers(GwProject *self);

G_END_DECLS
