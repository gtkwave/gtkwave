#ifndef WAVE_SIGNAL_LIST_H
#define WAVE_SIGNAL_LIST_H

#include "config.h"
#include <gtk/gtk.h>
#include "analyzer.h"

G_BEGIN_DECLS

#define GW_TYPE_SIGNAL_LIST gw_signal_list_get_type()

#if GTK_CHECK_VERSION(3, 0, 0)
G_DECLARE_FINAL_TYPE(GwSignalList, gw_signal_list, GW, SIGNAL_LIST, GtkDrawingArea)
#else
typedef struct _GwSignalList GwSignalList;
typedef struct {
    GtkDrawingAreaClass parent_class;
} GwSignalListClass;

GType gw_signal_list_get_type(void);

#define GW_SIGNAL_LIST(obj)    (G_TYPE_CHECK_INSTANCE_CAST((obj), GW_TYPE_SIGNAL_LIST, GwSignalList))
#define GW_IS_SIGNAL_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GW_TYPE_SIGNAL_LIST))
#endif

GtkWidget *gw_signal_list_new(void);
GtkAdjustment *gw_signal_list_get_hadjustment(GwSignalList *signal_list);
GtkAdjustment *gw_signal_list_get_vadjustment(GwSignalList *signal_list);
void gw_signal_list_force_redraw(GwSignalList *signal_list);
Trptr gw_signal_list_get_trace(GwSignalList *signal_list, guint i);
void gw_signal_list_scroll(GwSignalList *signal_list, int index);
void gw_signal_list_scroll_to_trace(GwSignalList *signal_list, Trptr trace);
int gw_signal_list_get_num_traces_displayable(GwSignalList *signal_list);

G_END_DECLS

#endif