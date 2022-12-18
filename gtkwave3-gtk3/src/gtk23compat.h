#ifndef WAVE_GTK23COMPAT_H
#define WAVE_GTK23COMPAT_H

/* this is for any future gtk2/gtk3 compatibility */

#ifdef MAC_INTEGRATION
/* #undef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND */
#endif

#if GTK_CHECK_VERSION(3,22,26)
#if !defined(__MINGW32__) && !defined(MAC_INTEGRATION)
#include <gdk/gdkwayland.h>
#endif
#endif

#if GTK_CHECK_VERSION(3,0,0)

#ifndef MAC_INTEGRATION
#define WAVE_ALLOW_GTK3_HEADER_BAR
#endif

/* workaround for wave_vslider not rendering properly on startup */
#define WAVE_ALLOW_GTK3_VSLIDER_WORKAROUND

/* workarounds for gtk warnings "How does the code know the size to allocate?", etc. */
#define WAVE_GTK3_GLIB_WARNING_SUPPRESS
/* #define WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_TREESEARCH */
/* #define WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER */
/* #define WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_DEPRECATED_API */

/* seat vs pointer enable */
#define WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB

#define WAVE_GTK3_HIERSEARCH_DEBOUNCE
#define WAVE_GTK3_MENU_SEPARATOR
#define WAVE_ALLOW_GTK3_GRID
#define WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX

/* gestures enable */
#define WAVE_ALLOW_GTK3_GESTURE_EVENT

#define WAVE_GTK3_SWIPE_TIME_CONSTANT (325000.0)
#define WAVE_GTK3_SWIPE_MICROSECONDS (8 * WAVE_GTK3_SWIPE_TIME_CONSTANT)
#define WAVE_GTK3_SWIPE_VEL_VS_DIST_FACTOR (WAVE_GTK3_SWIPE_TIME_CONSTANT / 1000000.0)

#define WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON

#define WAVE_CTRL_SCROLL_ZOOM_FACTOR 0.3

/* tested as working on thinkpad25 */
#define WAVE_GTK3_GESTURE_ZOOM_USES_GTK_PHASE_CAPTURE
#define WAVE_GTK3_GESTURE_ZOOM_IS_1D
#endif

#define WAVE_GTKIFE(a,b,c,d,e) {a,b,c,d,e,NULL}

#if GTK_CHECK_VERSION(3,0,0)
#define WAVE_GDK_GET_POINTER(a,b,c,bi,ci,d)  gdk_window_get_device_position(a, gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default())), bi, ci, d)
#else
#define WAVE_GDK_GET_POINTER(a,b,c,bi,ci,d)  gdk_window_get_pointer(a,bi,ci,d)
#endif

#define WAVE_GDK_GET_POINTER_COPY x=xi; y=yi;
#define WAVE_GDK_GET_POINTER_COPY_XONLY x=xi;

#define WAVE_GTK_SFUNCAST(x) ((void (*)(GtkWidget *, gpointer))(x))

/* doesn't work in gtk 2 or 3 */
#undef WAVE_ALLOW_SLIDER_ZOOM

/* gtk3->4 deprecated changes */

#if GTK_CHECK_VERSION(3,0,0)

#define XXX_GTK_TREE_VIEW GTK_SCROLLABLE
#define XXX_gtk_tree_view_get_vadjustment gtk_scrollable_get_vadjustment
#define XXX_gtk_tree_view_get_hadjustment gtk_scrollable_get_hadjustment
#define XXX_gtk_tree_view_set_vadjustment gtk_scrollable_set_vadjustment
#define XXX_gtk_tree_view_set_hadjustment gtk_scrollable_set_hadjustment
#define XXX_GTK_TEXT_VIEW GTK_SCROLLABLE
#define XXX_gtk_text_view_get_vadjustment gtk_scrollable_get_vadjustment
GtkWidget *XXX_gtk_hbox_new (gboolean homogeneous, gint spacing);
GtkWidget *XXX_gtk_vbox_new (gboolean homogeneous, gint spacing);
#define XXX_gtk_hpaned_new(a) gtk_paned_new(GTK_ORIENTATION_HORIZONTAL)
#define XXX_gtk_vpaned_new(a) gtk_paned_new(GTK_ORIENTATION_VERTICAL)
GtkWidget *XXX_gtk_hseparator_new (void);
#define XXX_gtk_hscrollbar_new(a) gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, a)
#define XXX_gtk_vscrollbar_new(a) gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, a)
#define XXX_GTK_STOCK_CANCEL "_Cancel"
#define XXX_GTK_STOCK_OPEN "_Open"
#define XXX_GTK_STOCK_SAVE "_Save"
#define XXX_GTK_STOCK_CUT "edit-cut"
#define XXX_GTK_STOCK_COPY "edit-copy"
#define XXX_GTK_STOCK_PASTE "edit-paste"
#define XXX_GTK_STOCK_ZOOM_FIT "zoom-fit-best"
#define XXX_GTK_STOCK_ZOOM_IN "zoom-in"
#define XXX_GTK_STOCK_ZOOM_OUT "zoom-out"
#define XXX_GTK_STOCK_UNDO "edit-undo"
#define XXX_GTK_STOCK_GOTO_FIRST "go-first"
#define XXX_GTK_STOCK_GOTO_LAST "go-last"
#define XXX_GTK_STOCK_GO_BACK "go-previous"
#define XXX_GTK_STOCK_GO_FORWARD "go-next"
#define XXX_GTK_STOCK_REFRESH "view-refresh"

#else

#define XXX_GTK_TREE_VIEW GTK_TREE_VIEW
#define XXX_gtk_tree_view_get_vadjustment gtk_tree_view_get_vadjustment
#define XXX_gtk_tree_view_get_hadjustment gtk_tree_view_get_hadjustment
#define XXX_gtk_tree_view_set_vadjustment gtk_tree_view_set_vadjustment
#define XXX_gtk_tree_view_set_hadjustment gtk_tree_view_set_hadjustment
#define XXX_GTK_TEXT_VIEW GTK_TEXT_VIEW
#define XXX_gtk_text_view_get_vadjustment gtk_text_view_get_vadjustment
#define XXX_gtk_hbox_new(a, b) gtk_hbox_new((a), (b))
#define XXX_gtk_vbox_new(a, b) gtk_vbox_new((a), (b))
#define XXX_gtk_hpaned_new(a) gtk_hpaned_new()
#define XXX_gtk_vpaned_new(a) gtk_vpaned_new()
#define XXX_gtk_hseparator_new gtk_hseparator_new
#define XXX_gtk_hscrollbar_new(a) gtk_hscrollbar_new(a)
#define XXX_gtk_vscrollbar_new(a) gtk_vscrollbar_new(a)
#define XXX_GTK_STOCK_CANCEL GTK_STOCK_CANCEL
#define XXX_GTK_STOCK_OPEN GTK_STOCK_OPEN
#define XXX_GTK_STOCK_SAVE GTK_STOCK_SAVE
#define XXX_GTK_STOCK_CUT GTK_STOCK_CUT
#define XXX_GTK_STOCK_COPY GTK_STOCK_COPY
#define XXX_GTK_STOCK_PASTE GTK_STOCK_PASTE
#define XXX_GTK_STOCK_ZOOM_FIT GTK_STOCK_ZOOM_FIT
#define XXX_GTK_STOCK_ZOOM_IN GTK_STOCK_ZOOM_IN
#define XXX_GTK_STOCK_ZOOM_OUT GTK_STOCK_ZOOM_OUT
#define XXX_GTK_STOCK_UNDO GTK_STOCK_UNDO
#define XXX_GTK_STOCK_GOTO_FIRST GTK_STOCK_GOTO_FIRST
#define XXX_GTK_STOCK_GOTO_LAST GTK_STOCK_GOTO_LAST
#define XXX_GTK_STOCK_GO_BACK GTK_STOCK_GO_BACK
#define XXX_GTK_STOCK_GO_FORWARD GTK_STOCK_GO_FORWARD
#define XXX_GTK_STOCK_REFRESH GTK_STOCK_REFRESH

#endif


#ifdef WAVE_ALLOW_GTK3_GRID
GtkWidget *XXX_gtk_table_new (guint rows,
               guint columns,
               gboolean homogeneous);
void XXX_gtk_table_attach (GtkGrid *table,
                  GtkWidget *child,
                  guint left_attach,
                  guint right_attach,
                  guint top_attach,
                  guint bottom_attach,
                  GtkAttachOptions xoptions,
                  GtkAttachOptions yoptions,
                  guint xpadding,
                  guint ypadding);
#define XXX_GTK_TABLE GTK_GRID
#else
#define XXX_gtk_table_new gtk_table_new
#define XXX_gtk_table_attach gtk_table_attach
#define XXX_GTK_TABLE GTK_TABLE
#endif


#ifdef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
void XXX_gdk_pointer_ungrab (guint32 time_);
#else
#define XXX_gdk_pointer_ungrab gdk_pointer_ungrab
#endif


#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
cairo_t *XXX_gdk_cairo_create (GdkWindow *window, GdkDrawingContext **gdc);
#else
#define XXX_gdk_cairo_create(a, b) gdk_cairo_create(a)
#endif


/* useful for multiple GTK versions */
GtkWidget *
XXX_gtk_toolbar_insert_stock (GtkToolbar *toolbar,
                          const gchar *stock_id,
                          const char *tooltip_text,
                          const char *tooltip_private_text,
                          GCallback callback,
                          gpointer user_data,
                          gint position);

void
XXX_gtk_toolbar_insert_space (GtkToolbar *toolbar,
                          gint position);

void
XXX_gtk_toolbar_insert_widget (GtkToolbar *toolbar,
                           GtkWidget *widget,
                           const char *tooltip_text,
                           const char *tooltip_private_text,
                           gint position);

gint 
XXX_gtk_widget_get_scale_factor (GtkWidget *widget);

#endif
