#include <config.h>
#include "debug.h"
#include "gw-time-display.h"

#define EM_DASH "\xe2\x80\x94"
static const char *TIME_PREFIX = WAVE_SI_UNITS;

static int dim_to_exponent(char dim)
{
    char *pnt = strchr(TIME_PREFIX, (int)dim);
    if (pnt == NULL) {
        return 0;
    }

    return (pnt - TIME_PREFIX) * 3;
}

static gchar *reformat_time_2_scale_to_dimension(GwTime val,
                                                 char dim,
                                                 char target_dim,
                                                 gboolean show_plus_sign)
{
    int value_exponent = dim_to_exponent(dim);
    int target_exponent = dim_to_exponent(target_dim);

    double v = val;
    while (value_exponent > target_exponent) {
        v /= 1000.0;
        value_exponent -= 3;
    }
    while (value_exponent < target_exponent) {
        v *= 1000.0;
        value_exponent += 3;
    }

    gchar *str;
    if (GLOBALS->scale_to_time_dimension == 's') {
        str = g_strdup_printf("%.9g sec", v);
    } else {
        str = g_strdup_printf("%.9g %cs", v, target_dim);
    }

    if (show_plus_sign && val >= 0) {
        gchar *t = g_strconcat("+", str, NULL);
        g_free(str);
        return t;
    } else {
        return str;
    }
}

static gchar *reformat_time_2(GwTime val, char dim, gboolean show_plus_sign)
{
    static const gunichar THIN_SPACE = 0x2009;

    if (GLOBALS->scale_to_time_dimension) {
        return reformat_time_2_scale_to_dimension(val,
                                                  dim,
                                                  GLOBALS->scale_to_time_dimension,
                                                  show_plus_sign);
    }

    gboolean negative = val < 0;
    val = ABS(val);

    gchar *value_str = g_strdup_printf("%" GW_TIME_FORMAT, val);
    gsize value_len = strlen(value_str);

    GString *str = g_string_new(NULL);
    if (negative) {
        g_string_append_c(str, '-');
    } else if (show_plus_sign) {
        g_string_append_c(str, '+');
    }

    gsize first_group_len = value_len % 3;
    if (first_group_len > 0) {
        g_string_append_len(str, value_str, first_group_len);
    }
    for (gsize group = 0; group < value_len / 3; group++) {
        if (group > 0 || first_group_len != 0) {
            g_string_append_unichar(str, THIN_SPACE);
        }
        g_string_append_len(str, &value_str[first_group_len + group * 3], 3);
    }

    g_string_append_c(str, ' ');

    if (dim != 0 && dim != ' ') {
        g_string_append_c(str, dim);
        g_string_append_c(str, 's');
    } else {
        g_string_append(str, "sec");
    }

    return g_string_free(str, FALSE);
}

static gchar *reformat_time_as_frequency(GwTime val, char dim)
{
    double k;

    static const double NEG_POW[] =
        {1.0, 1.0e-3, 1.0e-6, 1.0e-9, 1.0e-12, 1.0e-15, 1.0e-18, 1.0e-21};

    char *pnt = strchr(TIME_PREFIX, (int)dim);
    int offset;
    if (pnt) {
        offset = pnt - TIME_PREFIX;
    } else
        offset = 0;

    if (val != 0) {
        k = 1 / ((double)val * NEG_POW[offset]);

        return g_strdup_printf("%e Hz", k);
    } else {
        return g_strdup(EM_DASH " Hz");
    }
}

static gboolean is_in_blackout_region(GwTime val)
{
    for (struct blackout_region_t *bt = GLOBALS->blackout_regions; bt != NULL; bt = bt->next) {
        if (val >= bt->bstart && val < bt->bend) {
            return TRUE;
        }
    }

    return FALSE;
}

struct _GwTimeDisplay
{
    GtkBox parent_instance;

    GtkWidget *marker_label;
    GtkWidget *marker_value;
    GtkWidget *cursor_label;
    GtkWidget *cursor_value;
};

G_DEFINE_TYPE(GwTimeDisplay, gw_time_display, GTK_TYPE_BOX)

static void gw_time_display_class_init(GwTimeDisplayClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class,
                                                "/io/github/gtkwave/GTKWave/ui/gw-time-display.ui");
    gtk_widget_class_bind_template_child(widget_class, GwTimeDisplay, marker_label);
    gtk_widget_class_bind_template_child(widget_class, GwTimeDisplay, marker_value);
    gtk_widget_class_bind_template_child(widget_class, GwTimeDisplay, cursor_label);
    gtk_widget_class_bind_template_child(widget_class, GwTimeDisplay, cursor_value);
}

static void gw_time_display_init(GwTimeDisplay *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));
}

GtkWidget *gw_time_display_new(void)
{
    return g_object_new(GW_TYPE_TIME_DISPLAY, NULL);
}

void gw_time_display_update(GwTimeDisplay *self, Times *times)
{
    if (GLOBALS->use_maxtime_display) {
        gchar *text = reformat_time_2(GLOBALS->max_time + GLOBALS->global_time_offset,
                                      GLOBALS->time_dimension,
                                      FALSE);
        gtk_label_set_text(GTK_LABEL(self->marker_label), "Max");
        gtk_label_set_text(GTK_LABEL(self->marker_value), text);
        g_free(text);
    } else {
        gtk_label_set_text(GTK_LABEL(self->marker_label), "Marker");

        if (times->marker >= 0) {
            gchar *text = NULL;

            if (times->baseline >= 0 || times->lmbcache >= 0) {
                GwTime delta =
                    times->marker - (times->baseline >= 0 ? times->baseline : times->lmbcache);

                if (GLOBALS->use_frequency_delta) {
                    text = reformat_time_as_frequency(delta, GLOBALS->time_dimension);
                } else {
                    text = reformat_time_2(delta, GLOBALS->time_dimension, TRUE);
                }

                if (times->baseline >= 0) {
                    gchar *t = g_strconcat("B", text, NULL);
                    g_free(text);
                    text = t;
                }
            } else {
                text = reformat_time_2(times->marker + GLOBALS->global_time_offset,
                                       GLOBALS->time_dimension,
                                       FALSE);
            }

            gtk_label_set_text(GTK_LABEL(self->marker_value), text);
            g_free(text);
        } else {
            gtk_label_set_text(GTK_LABEL(self->marker_value), EM_DASH);
        }
    }

    if (times->baseline >= 0) {
        gtk_label_set_text(GTK_LABEL(self->cursor_label), "Base");
        gchar *text = reformat_time_2(times->baseline + GLOBALS->global_time_offset,
                                      GLOBALS->time_dimension,
                                      FALSE);
        gtk_label_set_text(GTK_LABEL(self->cursor_value), text);
        g_free(text);
    } else {
        gtk_label_set_text(GTK_LABEL(self->cursor_label), "Cursor");

        gchar *text = reformat_time_2(GLOBALS->currenttime + GLOBALS->global_time_offset,
                                      GLOBALS->time_dimension,
                                      FALSE);

        if (is_in_blackout_region(GLOBALS->currenttime + GLOBALS->global_time_offset)) {
            gchar *last_space = g_strrstr(text, " ");
            if (last_space != NULL) {
                *last_space = '*';
            } else {
                g_assert_not_reached();
            }
        }

        gtk_label_set_text(GTK_LABEL(self->cursor_value), text);
        g_free(text);
    }
}
