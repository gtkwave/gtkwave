#ifndef WAVE_GTK12COMPAT_H
#define WAVE_GTK12COMPAT_H

#ifdef MAC_INTEGRATION
/* #undef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND */
#endif

#if WAVE_USE_GTK2

#define WAVE_GTKIFE(a,b,c,d,e) {a,b,c,d,e,NULL}
#define WAVE_GDK_GET_POINTER(a,b,c,bi,ci,d)  gdk_window_get_pointer(a,bi,ci,d)
#define WAVE_GDK_GET_POINTER_COPY x=xi; y=yi;

#define WAVE_GTK_SFUNCAST(x) ((void (*)(GtkWidget *, gpointer))(x))

#else

#if !defined __MINGW32__ && !defined _MSC_VER
#ifndef G_CONST_RETURN
#define G_CONST_RETURN
#endif
#endif

#define WAVE_GTKIFE(a,b,c,d,e) {a,b,c,d,e}
#define WAVE_GDK_GET_POINTER(a,b,c,bi,ci,d)  gdk_input_window_get_pointer(a, event->deviceid, b, c, NULL, NULL, NULL, d)
#define WAVE_GDK_GET_POINTER_COPY
#define WAVE_GTK_SIGFONT wavearea->style->font
#define WAVE_GTK_WAVEFONT wavearea->style->font
#define WAVE_GTK_SFUNCAST(x) ((GtkSignalFunc)(x))

#define gtk_notebook_set_current_page(n,p) gtk_notebook_set_page((n),(p))


#endif

#endif

