#pragma once

#include <glib-object.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_VCD_PARTIAL_LOADER (gw_vcd_partial_loader_get_type())
G_DECLARE_FINAL_TYPE(GwVcdPartialLoader, gw_vcd_partial_loader, GW, VCD_PARTIAL_LOADER, GwLoader)

GwLoader *gw_vcd_partial_loader_new(void);

void gw_vcd_partial_loader_set_vlist_prepack(GwVcdPartialLoader *self, gboolean vlist_prepack);
gboolean gw_vcd_partial_loader_is_vlist_prepack(GwVcdPartialLoader *self);
void gw_vcd_partial_loader_set_vlist_compression_level(GwVcdPartialLoader *self, gint level);
gint gw_vcd_partial_loader_get_vlist_compression_level(GwVcdPartialLoader *self);
void gw_vcd_partial_loader_set_warning_filesize(GwVcdPartialLoader *self, guint warning_filesize);
guint gw_vcd_partial_loader_get_warning_filesize(GwVcdPartialLoader *self);

G_END_DECLS
