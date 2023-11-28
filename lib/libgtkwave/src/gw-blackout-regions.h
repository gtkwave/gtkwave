#pragma once

#include <glib-object.h>
#include "gw-time.h"

G_BEGIN_DECLS

typedef void (*GwBlackoutRegionsForEach)(GwTime start, GwTime end, gpointer user_data);

#define GW_TYPE_BLACKOUT_REGIONS (gw_blackout_regions_get_type())
G_DECLARE_FINAL_TYPE(GwBlackoutRegions, gw_blackout_regions, GW, BLACKOUT_REGIONS, GObject)

GwBlackoutRegions *gw_blackout_regions_new(void);

void gw_blackout_regions_add(GwBlackoutRegions *self, GwTime start, GwTime end);
void gw_blackout_regions_add_dumpoff(GwBlackoutRegions *self, GwTime t);
void gw_blackout_regions_add_dumpon(GwBlackoutRegions *self, GwTime t);

gboolean gw_blackout_regions_contains(GwBlackoutRegions *self, GwTime t);
void gw_blackout_regions_scale(GwBlackoutRegions *self, GwTime scale);
void gw_blackout_regions_foreach(GwBlackoutRegions *self,
                                 GwBlackoutRegionsForEach function,
                                 gpointer user_data);
guint gw_blackout_regions_length(GwBlackoutRegions *self);

G_END_DECLS