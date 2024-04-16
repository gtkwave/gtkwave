#pragma once

#include <glib-object.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_FST_LOADER (gw_fst_loader_get_type())
G_DECLARE_FINAL_TYPE(GwFstLoader, gw_fst_loader, GW, FST_LOADER, GwLoader)

GwLoader *gw_fst_loader_new(void);

void gw_fst_loader_set_start_time(GwFstLoader *self, const gchar *start_time);
void gw_fst_loader_set_end_time(GwFstLoader *self, const gchar *end_time);

G_END_DECLS
