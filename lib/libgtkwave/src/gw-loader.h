#pragma once

#include <glib-object.h>
#include "gw-dump-file.h"

G_BEGIN_DECLS

#define GW_TYPE_LOADER (gw_loader_get_type())
G_DECLARE_DERIVABLE_TYPE(GwLoader, gw_loader, GW, LOADER, GObject)

struct _GwLoaderClass
{
    GObjectClass parent_class;

    GwDumpFile *(*load)(GwLoader *self, const gchar *path, GError **error);
};

GwDumpFile *gw_loader_load(GwLoader *self, const gchar *path, GError **error);

void gw_loader_set_preserve_glitches(GwLoader *self, gboolean preserve_glitches);
gboolean gw_loader_is_preserve_glitches(GwLoader *self);
void gw_loader_set_preserve_glitches_real(GwLoader *self, gboolean preserve_glitches_real);
gboolean gw_loader_is_preserve_glitches_real(GwLoader *self);

G_END_DECLS
