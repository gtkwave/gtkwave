#pragma once

#include <glib-object.h>
#include "gw-dump-file.h"

G_BEGIN_DECLS

typedef struct
{
    gboolean preserve_glitches;
    gboolean preserve_glitches_real;
} GwLoaderSettings;

#define GW_TYPE_LOADER_SETTINGS (gw_loader_settings_get_type())
GType gw_loader_settings_get_type(void);

GwLoaderSettings *gw_loader_settings_new(void);
GwLoaderSettings *gw_loader_settings_copy(const GwLoaderSettings *self);
void gw_loader_settings_free(GwLoaderSettings *self);

#define GW_TYPE_LOADER (gw_loader_get_type())
G_DECLARE_DERIVABLE_TYPE(GwLoader, gw_loader, GW, LOADER, GObject)

struct _GwLoaderClass
{
    GObjectClass parent_class;

    GwDumpFile *(*load)(GwLoader *self, const gchar *path, GError **error);
};

GwDumpFile *gw_loader_load(GwLoader *self, const gchar *path, GError **error);
const GwLoaderSettings *gw_loader_get_settings(GwLoader *self);

G_END_DECLS
