#pragma once

#include <gtk/gtk.h>
#include "analyzer.h"

G_BEGIN_DECLS

#define GW_TYPE_MOUSEOVER (gw_mouseover_get_type())
G_DECLARE_FINAL_TYPE(GwMouseover, gw_mouseover, GW, MOUSEOVER, GtkWindow)

GtkWidget *gw_mouseover_new(void);
void gw_mouseover_update(GwMouseover *self, GwTrace *trace, GwTime t);
void gw_mouseover_update_signal_list(GwMouseover *self, GwTrace *trace, GwTime t);

G_END_DECLS
