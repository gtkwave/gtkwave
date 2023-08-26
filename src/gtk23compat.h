#ifndef WAVE_GTK23COMPAT_H
#define WAVE_GTK23COMPAT_H

#include <gtk/gtk.h>

/* this is for any future gtk2/gtk3 compatibility */

#ifdef MAC_INTEGRATION
/* #undef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND */
#endif

#if !defined(__MINGW32__) && !defined(MAC_INTEGRATION) && defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif

/* workaround for wave_vslider not rendering properly on startup */
#define WAVE_ALLOW_GTK3_VSLIDER_WORKAROUND

/* workarounds for gtk warnings "How does the code know the size to allocate?", etc. */
/* #define WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_TREESEARCH */
/* #define WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER */
/* #define WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_DEPRECATED_API */

/* seat vs pointer enable */
#define WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB

#define WAVE_GTK3_MENU_SEPARATOR
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

#define WAVE_GTKIFE(a, b, c, d, e) \
    { \
        a, b, c, d, e, NULL \
    }

#define WAVE_GDK_GET_POINTER(a, b, c, bi, ci, d) \
    gdk_window_get_device_position( \
        a, \
        gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default())), \
        bi, \
        ci, \
        d)

#define WAVE_GDK_GET_POINTER_COPY \
    x = xi; \
    y = yi;
#define WAVE_GDK_GET_POINTER_COPY_XONLY x = xi;

#define WAVE_GTK_SFUNCAST(x) ((void (*)(GtkWidget *, gpointer))(x))

/* gtk3->4 deprecated changes */

#define XXX_GTK_STOCK_CANCEL "_Cancel"
#define XXX_GTK_STOCK_OPEN "_Open"
#define XXX_GTK_STOCK_SAVE "_Save"

#ifdef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
void XXX_gdk_pointer_ungrab(guint32 time_);
#else
#define XXX_gdk_pointer_ungrab gdk_pointer_ungrab
#endif

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
cairo_t *XXX_gdk_cairo_create(GdkWindow *window, GdkDrawingContext **gdc);
#else
#define XXX_gdk_cairo_create(a, b) gdk_cairo_create(a)
#endif

#endif
