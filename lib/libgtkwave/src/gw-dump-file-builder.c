#include "gw-dump-file-builder.h"

struct _GwDumpFileBuilder
{
    GObject parent_instance;
};

G_DEFINE_TYPE(GwDumpFileBuilder, gw_dump_file_builder, G_TYPE_OBJECT)

static void gw_dump_file_builder_class_init(GwDumpFileBuilderClass *klass)
{
    (void)klass;
}

static void gw_dump_file_builder_init(GwDumpFileBuilder *self)
{
    (void)self;
}

GwDumpFileBuilder *gw_dump_file_builder_new(void)
{
    return g_object_new(GW_TYPE_DUMP_FILE_BUILDER, NULL);
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

    return g_object_new(GW_TYPE_DUMP_FILE, NULL);
}