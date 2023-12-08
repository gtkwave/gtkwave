#pragma once

#include <glib-object.h>
#include "gw-blackout-regions.h"
#include "gw-stems.h"
#include "gw-time.h"

G_BEGIN_DECLS

#define GW_TYPE_DUMP_FILE (gw_dump_file_get_type())
G_DECLARE_DERIVABLE_TYPE(GwDumpFile, gw_dump_file, GW, DUMP_FILE, GObject)

struct _GwDumpFileClass
{
    GObjectClass parent_class;
};

GwBlackoutRegions *gw_dump_file_get_blackout_regions(GwDumpFile *self);
GwStems *gw_dump_file_get_stems(GwDumpFile *self);

GwTimeDimension gw_dump_file_get_time_dimension(GwDumpFile *self);
GwTime gw_dump_file_get_time_scale(GwDumpFile *self);
GwTime gw_dump_file_get_global_time_offset(GwDumpFile *self);

G_END_DECLS