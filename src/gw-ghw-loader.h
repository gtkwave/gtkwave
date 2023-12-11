#pragma once

#include <glib-object.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_GHW_LOADER (gw_ghw_loader_get_type())
G_DECLARE_FINAL_TYPE(GwGhwLoader, gw_ghw_loader, GW, GHW_LOADER, GwLoader)

GwLoader *gw_ghw_loader_new(void);

G_END_DECLS
