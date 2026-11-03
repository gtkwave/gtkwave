#include "gw-time-label.h"

struct _GwTimeLabel
{
    GtkLabel parent_instance;
};

G_DEFINE_TYPE(GwTimeLabel, gw_time_label, GTK_TYPE_LABEL)

static void gw_time_label_get_preferred_width(GtkWidget *widget, int *minimum, int *natural)
{
    GTK_WIDGET_CLASS(gw_time_label_parent_class)->get_preferred_width(widget, minimum, natural);
    *minimum = 50;
}

static void gw_time_label_class_init(GwTimeLabelClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    widget_class->get_preferred_width = gw_time_label_get_preferred_width;
}

static void gw_time_label_init(GwTimeLabel *self)
{
    (void)self;
}
