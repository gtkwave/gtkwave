#pragma once

#include <gtk/gtk.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_NAMED_MARKER_DIALOG (gw_named_marker_dialog_get_type())
G_DECLARE_FINAL_TYPE(GwNamedMarkerDialog,
                     gw_named_marker_dialog,
                     GW,
                     NAMED_MARKER_DIALOG,
                     GtkDialog)

GtkWidget *gw_named_marker_dialog_new(GwNamedMarkers *markers);

G_END_DECLS
