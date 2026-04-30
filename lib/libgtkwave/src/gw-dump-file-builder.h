#pragma once

#include <glib-object.h>
#include "gw-dump-file.h"
#include "gw-time.h"

G_BEGIN_DECLS

#define GW_TYPE_DUMP_FILE_BUILDER (gw_dump_file_builder_get_type())
G_DECLARE_FINAL_TYPE(GwDumpFileBuilder, gw_dump_file_builder, GW, DUMP_FILE_BUILDER, GObject)

GwDumpFileBuilder *gw_dump_file_builder_new(void);
GwDumpFile *gw_dump_file_builder_build(GwDumpFileBuilder *self);

void gw_dump_file_builder_set_time_scale(GwDumpFileBuilder *self, GwTime time_scale);
void gw_dump_file_builder_set_time_dimension(GwDumpFileBuilder *self, GwTimeDimension time_dimension);
GwTime gw_dump_file_builder_get_time_scale(GwDumpFileBuilder *self);
GwTimeDimension gw_dump_file_builder_get_time_dimension(GwDumpFileBuilder *self);

G_END_DECLS