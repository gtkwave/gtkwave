#pragma once

#include <glib-object.h>
#include "gw-blackout-regions.h"
#include "gw-stems.h"
#include "gw-hist-ent-factory.h"

G_BEGIN_DECLS

#define GW_TYPE_DUMP_FILE (gw_dump_file_get_type())
G_DECLARE_FINAL_TYPE(GwDumpFile, gw_dump_file, GW, DUMP_FILE, GObject)

GwBlackoutRegions *gw_dump_file_get_blackout_regions(GwDumpFile *self);
GwStems *gw_dump_file_get_stems(GwDumpFile *self);
GwHistEntFactory *gw_dump_file_get_hist_ent_factory(GwDumpFile *self);

GwTime gw_dump_file_get_global_time_offset(GwDumpFile *self);

G_END_DECLS