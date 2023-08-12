#pragma once

#include <gtk/gtk.h>
#include "analyzer.h"

G_BEGIN_DECLS

#define GW_TYPE_TIME_DISPLAY (gw_time_display_get_type())
G_DECLARE_FINAL_TYPE(GwTimeDisplay, gw_time_display, GW, TIME_DISPLAY, GtkBox)

GtkWidget *gw_time_display_new(void);
void gw_time_display_update(GwTimeDisplay *self, Times *times);

G_END_DECLS
