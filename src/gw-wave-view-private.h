#pragma once

#include <gtk/gtk.h>
#include <gtkwave.h>

G_BEGIN_DECLS

struct _GwWaveView
{
    GtkDrawingArea parent_instance;

    cairo_surface_t *traces_surface;

    gboolean dirty;
};

G_END_DECLS