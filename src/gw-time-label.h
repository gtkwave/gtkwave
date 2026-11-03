#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GW_TYPE_TIME_LABEL (gw_time_label_get_type())
G_DECLARE_FINAL_TYPE(GwTimeLabel, gw_time_label, GW, TIME_LABEL, GtkLabel)

G_END_DECLS
