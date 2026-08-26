#include "gw-dump-file-builder.h"
#include "gw-dump-file.h"
#include "gw-time.h"

struct _GwDumpFileBuilder
{
    GObject parent_instance;

    GwTime time_scale;
    GwTimeDimension time_dimension;
};

G_DEFINE_TYPE(GwDumpFileBuilder, gw_dump_file_builder, G_TYPE_OBJECT)

static void gw_dump_file_builder_class_init(GwDumpFileBuilderClass *klass)
{
    (void)klass;
}

static void gw_dump_file_builder_init(GwDumpFileBuilder *self)
{
    self->time_scale = 1;
    self->time_dimension = GW_TIME_DIMENSION_NANO;
}

GwDumpFileBuilder *gw_dump_file_builder_new(void)
{
    return g_object_new(GW_TYPE_DUMP_FILE_BUILDER, NULL);
}

void gw_dump_file_builder_set_time_scale(GwDumpFileBuilder *self, GwTime time_scale)
{
    g_return_if_fail(GW_IS_DUMP_FILE_BUILDER(self));
    self->time_scale = time_scale;
}

void gw_dump_file_builder_set_time_dimension(GwDumpFileBuilder *self, GwTimeDimension time_dimension)
{
    g_return_if_fail(GW_IS_DUMP_FILE_BUILDER(self));
    self->time_dimension = time_dimension;
}

GwTime gw_dump_file_builder_get_time_scale(GwDumpFileBuilder *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE_BUILDER(self), 1);
    return self->time_scale;
}

GwTimeDimension gw_dump_file_builder_get_time_dimension(GwDumpFileBuilder *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE_BUILDER(self), GW_TIME_DIMENSION_NANO);
    return self->time_dimension;
}

/**
 * gw_dump_file_builder_build:
 * @self: A #GwDumpFileBuilds.
 *
 * Returns the built dump file.
 *
 * Returns: (transfer full): The dump file.
 */
GwDumpFile *gw_dump_file_builder_build(GwDumpFileBuilder *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE_BUILDER(self), NULL);

    return g_object_new(GW_TYPE_DUMP_FILE,
                       "time-scale", self->time_scale,
                       "time-dimension", self->time_dimension,
                       NULL);
}