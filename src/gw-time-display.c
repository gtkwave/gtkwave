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

    if (dim != 0 && dim != ' ' && dim != 's') {
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

struct _GwTimeDisplay
{
    GtkBox parent_instance;

    GwProject *project;

    GtkWidget *marker_label;
    GtkWidget *marker_value;
    GtkWidget *cursor_label;
    GtkWidget *cursor_value;
};

G_DEFINE_TYPE(GwTimeDisplay, gw_time_display, GTK_TYPE_BOX)

enum
{
    PROP_PROJECT = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_time_display_update(GwTimeDisplay *self)
{
    GwMarker *primary_marker = NULL;
    GwMarker *baseline_marker = NULL;
    GwMarker *ghost_marker = NULL;
    GwMarker *cursor = NULL;
    if (self->project != NULL) {
        primary_marker = gw_project_get_primary_marker(self->project);
        baseline_marker = gw_project_get_baseline_marker(self->project);
        ghost_marker = gw_project_get_ghost_marker(self->project);
        cursor = gw_project_get_cursor(self->project);
    }

    if (GLOBALS->use_maxtime_display) {
        gchar *text = reformat_time_2(GLOBALS->max_time + GLOBALS->global_time_offset,
                                      GLOBALS->time_dimension,
                                      FALSE);
        gtk_label_set_text(GTK_LABEL(self->marker_label), "Max");
        gtk_label_set_text(GTK_LABEL(self->marker_value), text);
        g_free(text);
    } else {
        gtk_label_set_text(GTK_LABEL(self->marker_label), "Marker");

        if (primary_marker != NULL && gw_marker_is_enabled(primary_marker)) {
            gchar *text = NULL;

            if (gw_marker_is_enabled(baseline_marker) || gw_marker_is_enabled(ghost_marker)) {
                GwMarker *base =
                    gw_marker_is_enabled(baseline_marker) ? baseline_marker : ghost_marker;
                GwTime delta =
                    gw_marker_get_position(primary_marker) - gw_marker_get_position(base);

                if (GLOBALS->use_frequency_delta) {
                    text = reformat_time_as_frequency(delta, GLOBALS->time_dimension);
                } else {
                    text = reformat_time_2(delta, GLOBALS->time_dimension, TRUE);
                }

                if (gw_marker_is_enabled(baseline_marker)) {
                    gchar *t = g_strconcat("B", text, NULL);
                    g_free(text);
                    text = t;
                }
            } else {
                text = reformat_time_2(gw_marker_get_position(primary_marker) +
                                           GLOBALS->global_time_offset,
                                       GLOBALS->time_dimension,
                                       FALSE);
            }

            gtk_label_set_text(GTK_LABEL(self->marker_value), text);
            g_free(text);
        } else {
            gtk_label_set_text(GTK_LABEL(self->marker_value), EM_DASH);
        }
    }

    if (baseline_marker != NULL && gw_marker_is_enabled(baseline_marker)) {
        gtk_label_set_text(GTK_LABEL(self->cursor_label), "Base");
        gchar *text =
            reformat_time_2(gw_marker_get_position(baseline_marker) + GLOBALS->global_time_offset,
                            GLOBALS->time_dimension,
                            FALSE);
        gtk_label_set_text(GTK_LABEL(self->cursor_value), text);
        g_free(text);
    } else {
        gtk_label_set_text(GTK_LABEL(self->cursor_label), "Cursor");

        GwTime cursor_pos = cursor != NULL ? gw_marker_get_position(cursor) : 0;
        cursor_pos += GLOBALS->global_time_offset;

        gchar *text = reformat_time_2(cursor_pos, GLOBALS->time_dimension, FALSE);

        if (GLOBALS->blackout_regions != NULL &&
            gw_blackout_regions_contains(GLOBALS->blackout_regions, cursor_pos)) {
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

static void gw_time_display_dispose(GObject *object)
{
    GwTimeDisplay *self = GW_TIME_DISPLAY(object);

    g_clear_object(&self->project);

    G_OBJECT_CLASS(gw_time_display_parent_class)->dispose(object);
}

static void gw_time_display_set_property(GObject *object,
                                         guint property_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
    GwTimeDisplay *self = GW_TIME_DISPLAY(object);

    switch (property_id) {
        case PROP_PROJECT:
            gw_time_display_set_project(self, g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_time_display_get_property(GObject *object,
                                         guint property_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
    GwTimeDisplay *self = GW_TIME_DISPLAY(object);

    switch (property_id) {
        case PROP_PROJECT:
            g_value_set_object(value, gw_time_display_get_project(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_time_display_class_init(GwTimeDisplayClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->dispose = gw_time_display_dispose;
    object_class->set_property = gw_time_display_set_property;
    object_class->get_property = gw_time_display_get_property;

    properties[PROP_PROJECT] = g_param_spec_object("project",
                                                   "Project",
                                                   "The project",
                                                   GW_TYPE_PROJECT,
                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);

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

    gw_time_display_update(self);
}

GtkWidget *gw_time_display_new(void)
{
    return g_object_new(GW_TYPE_TIME_DISPLAY, NULL);
}

static void on_unnamed_marker_changed(GwProject *project, GwTimeDisplay *time_display)
{
    (void)project;

    gw_time_display_update(time_display);
}

static void on_cursor_changed(GwMarker *cursor, GParamSpec *pspec, GwTimeDisplay *time_display)
{
    (void)cursor;
    (void)pspec;

    gw_time_display_update(time_display);
}

void gw_time_display_set_project(GwTimeDisplay *self, GwProject *project)
{
    g_return_if_fail(GW_IS_TIME_DISPLAY(self));
    g_return_if_fail(project == NULL || GW_IS_PROJECT(project));

    if (self->project == project) {
        return;
    }

    if (self->project != NULL) {
        g_signal_handlers_disconnect_by_data(self->project, self);
        g_signal_handlers_disconnect_by_data(gw_project_get_cursor(self->project), self);

        g_object_unref(self->project);
    }

    if (project != NULL) {
        self->project = g_object_ref(project);

        g_signal_connect(self->project,
                         "unnamed-marker-changed",
                         G_CALLBACK(on_unnamed_marker_changed),
                         self);
        g_signal_connect(gw_project_get_cursor(self->project),
                         "notify",
                         G_CALLBACK(on_cursor_changed),
                         self);
    }

    gw_time_display_update(self);
}

GwProject *gw_time_display_get_project(GwTimeDisplay *self)
{
    g_return_val_if_fail(GW_IS_TIME_DISPLAY(self), NULL);

    return self->project;
}
