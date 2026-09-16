#pragma once

#include <glib-object.h>
#include "gw-time.h"

G_BEGIN_DECLS

#define GW_TYPE_TIME_RANGE (gw_time_range_get_type())
G_DECLARE_FINAL_TYPE(GwTimeRange, gw_time_range, GW, TIME_RANGE, GObject)

GwTimeRange *gw_time_range_new(GwTime start, GwTime end);

GwTime gw_time_range_get_start(GwTimeRange *self);
GwTime gw_time_range_get_end(GwTimeRange *self);

gboolean gw_time_range_contains(GwTimeRange *self, GwTime time);

G_END_DECLS
