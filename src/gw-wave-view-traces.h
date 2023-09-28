#pragma once

#include <gtk/gtk.h>
#include "gw-wave-view.h"

G_BEGIN_DECLS

void gw_wave_view_render_traces(GwWaveView *self, cairo_t *cr);

G_END_DECLS
