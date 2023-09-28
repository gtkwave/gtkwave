#pragma  once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GW_TYPE_WAVE_VIEW (gw_wave_view_get_type())
G_DECLARE_FINAL_TYPE(GwWaveView, gw_wave_view, GW, WAVE_VIEW, GtkDrawingArea)

GtkWidget *gw_wave_view_new(void);
void gw_wave_view_force_redraw(GwWaveView *self);

G_END_DECLS