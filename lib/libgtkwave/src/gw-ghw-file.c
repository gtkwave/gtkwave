#include <config.h>
#include <gtkwave.h>
#include "gw-ghw-file.h"
#include "gw-ghw-file-private.h"

G_DEFINE_TYPE(GwGhwFile, gw_ghw_file, GW_TYPE_DUMP_FILE)

static void gw_ghw_file_dispose(GObject *object)
{
    GwGhwFile *self = GW_GHW_FILE(object);

    g_clear_object(&self->hist_ent_factory);

    G_OBJECT_CLASS(gw_ghw_file_parent_class)->dispose(object);
}

static void gw_ghw_file_class_init(GwGhwFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->dispose = gw_ghw_file_dispose;
}

static void gw_ghw_file_init(GwGhwFile *self)
{
    (void)self;
}
