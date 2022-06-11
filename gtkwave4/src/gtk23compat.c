/*
 * Copyright (c) Tony Bybell 2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "gtk23compat.h"


#if GTK_CHECK_VERSION(3,0,0)

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
cairo_t *XXX_gdk_cairo_create (GdkWindow *window, GdkDrawingContext **gdc)
{
cairo_region_t *region = gdk_window_get_visible_region (window);
GdkDrawingContext *context = gdk_window_begin_draw_frame (window, region);
*gdc = context;
cairo_region_destroy (region);

cairo_t *cr = gdk_drawing_context_get_cairo_context (context);

return(cr);
}
#endif


#ifdef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
void
XXX_gdk_pointer_ungrab (guint32 time_)
{
(void) time_;

GdkDisplay *display = gdk_display_get_default();
GdkSeat *seat = gdk_display_get_default_seat (display);
gdk_seat_ungrab(seat);
}
#endif


#ifdef WAVE_ALLOW_GTK3_GRID
GtkWidget *
XXX_gtk_table_new (guint rows,
               guint columns,
               gboolean homogeneous)
{
(void) rows;
(void) columns;

GtkWidget *grid = gtk_grid_new ();
gtk_grid_set_row_homogeneous (GTK_GRID(grid), homogeneous);
gtk_grid_set_column_homogeneous (GTK_GRID(grid), homogeneous);

return(grid);
}

void
XXX_gtk_table_attach (GtkGrid *table,
                  GtkWidget *child,
                  guint left_attach,
                  guint right_attach,
                  guint top_attach,
                  guint bottom_attach,
                  GtkAttachOptions xoptions,
                  GtkAttachOptions yoptions,
                  guint xpadding,
                  guint ypadding)
{
(void) xpadding;
(void) ypadding;

gtk_grid_attach (table, child, left_attach, top_attach, right_attach - left_attach, bottom_attach - top_attach);

gtk_widget_set_hexpand(child, (xoptions & (GTK_EXPAND | GTK_FILL)) != 0);
gtk_widget_set_vexpand(child, (yoptions & (GTK_EXPAND | GTK_FILL)) != 0);
}
#endif

GtkWidget *
XXX_gtk_hseparator_new (void)
{
return(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
}

GtkWidget *
XXX_gtk_hbox_new(gboolean homogeneous, gint spacing)
{
GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
gtk_box_set_homogeneous (GTK_BOX(hbox), homogeneous);

return(hbox);
}

GtkWidget *
XXX_gtk_vbox_new(gboolean homogeneous, gint spacing)
{
GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
gtk_box_set_homogeneous (GTK_BOX(vbox), homogeneous);

return(vbox);
}
#endif


GtkWidget *
XXX_gtk_toolbar_insert_stock (GtkToolbar *toolbar,
                          const gchar *stock_id,
                          const char *tooltip_text,
                          const char *tooltip_private_text,
                          GCallback callback,
                          gpointer user_data,
                          gint position)
{
 (void) tooltip_private_text;

  GtkToolItem *button;
#if GTK_CHECK_VERSION(3,0,0)
  GtkWidget *icon_widget = gtk_image_new_from_icon_name(stock_id, GTK_ICON_SIZE_BUTTON);

  gtk_widget_show(icon_widget);
  button = gtk_tool_button_new(icon_widget, NULL);
#else
  button = gtk_tool_button_new_from_stock (stock_id);
#endif
  gtk_tool_item_set_tooltip_text (button, tooltip_text);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                      button,
                      position);

  g_signal_connect(XXX_GTK_OBJECT(button), "clicked", G_CALLBACK(callback), user_data);
  return(GTK_WIDGET(button));
}

void
XXX_gtk_toolbar_insert_space (GtkToolbar *toolbar,
                          gint position)
{
  GtkToolItem *button;

  button = gtk_separator_tool_item_new();
  gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(button), TRUE);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                      button,
                      position);
}

void
XXX_gtk_toolbar_insert_widget (GtkToolbar *toolbar,
                           GtkWidget *widget,
                           const char *tooltip_text,
                           const char *tooltip_private_text,
                           gint position)
{
(void) tooltip_text;
(void) tooltip_private_text;

GtkToolItem *ti = gtk_tool_item_new ();

gtk_container_add(GTK_CONTAINER(ti), widget);
gtk_widget_show(GTK_WIDGET(ti));

gtk_toolbar_insert (GTK_TOOLBAR (toolbar), ti, position);
}


gint XXX_gtk_widget_get_scale_factor (GtkWidget *widget)
{
#if GTK_CHECK_VERSION(3,0,0)
gint rc = widget ? gtk_widget_get_scale_factor(widget) : 1;
return(rc);
#else
return(1);
#endif
}

