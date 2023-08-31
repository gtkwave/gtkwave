#include <config.h>
#include <gtkwave.h>
#include "gw-named-marker-dialog.h"
#include "currenttime.h"

struct _GwNamedMarkerDialog
{
    GtkDialog parent_instance;

    GwNamedMarkers *markers;
    GtkWidget *grid;
    GtkWidget *scrolled_window;

    GPtrArray *time_entries;
    GPtrArray *alias_entries;
};

G_DEFINE_TYPE(GwNamedMarkerDialog, gw_named_marker_dialog, GTK_TYPE_DIALOG)

enum
{
    PROP_MARKERS = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_named_marker_dialog_constructed(GObject *object);

static void gw_named_marker_dialog_dispose(GObject *object)
{
    GwNamedMarkerDialog *self = GW_NAMED_MARKER_DIALOG(object);

    g_clear_object(&self->markers);

    G_OBJECT_CLASS(gw_named_marker_dialog_parent_class)->dispose(object);
}

static void gw_named_marker_dialog_finalize(GObject *object)
{
    GwNamedMarkerDialog *self = GW_NAMED_MARKER_DIALOG(object);

    g_ptr_array_free(self->time_entries, TRUE);
    g_ptr_array_free(self->alias_entries, TRUE);

    G_OBJECT_CLASS(gw_named_marker_dialog_parent_class)->finalize(object);
}

static void gw_named_marker_dialog_set_property(GObject *object,
                                                guint property_id,
                                                const GValue *value,
                                                GParamSpec *pspec)
{
    GwNamedMarkerDialog *self = GW_NAMED_MARKER_DIALOG(object);

    switch (property_id) {
        case PROP_MARKERS:
            self->markers = g_value_dup_object(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static gboolean parse_time_entry(GtkEntry *entry, GwNamedMarkerDialog *dialog, GwTime *time_out)
{
    g_return_val_if_fail(time_out != NULL, FALSE);

    gchar *text = g_strdup(gtk_entry_get_text(entry));
    g_strstrip(text);

    if (text[0] == '\0') {
        g_free(text);
        return FALSE;
    }

    GwTime temp = unformat_time(text, GLOBALS->time_dimension);
    temp -= GLOBALS->global_time_offset;
    g_free(text);

    if (temp < GLOBALS->tims.start || temp > GLOBALS->tims.last) {
        return FALSE;
    }

    *time_out = temp + GLOBALS->global_time_offset;

    return TRUE;
}

static void gw_named_marker_dialog_response(GtkDialog *dialog, gint response_id)
{
    GwNamedMarkerDialog *self = GW_NAMED_MARKER_DIALOG(dialog);

    if (response_id == GTK_RESPONSE_OK) {
        guint num_markers = gw_named_markers_get_number_of_markers(self->markers);
        for (guint i = 0; i < num_markers; i++) {
            GwMarker *marker = gw_named_markers_get(self->markers, i);
            gw_marker_set_enabled(marker, FALSE);
            gw_marker_set_alias(marker, NULL);
        }

        for (guint i = 0; i < num_markers; i++) {
            GwMarker *marker = gw_named_markers_get(self->markers, i);

            GtkEntry *time_entry = g_ptr_array_index(self->time_entries, i);
            GtkEntry *alias_entry = g_ptr_array_index(self->alias_entries, i);

            const gchar *time_str = gtk_entry_get_text(time_entry);
            const gchar *alias_str = gtk_entry_get_text(alias_entry);

            GwTime temp = unformat_time(time_str, GLOBALS->time_dimension);
            temp -= GLOBALS->global_time_offset;

            if (temp >= GLOBALS->tims.start && temp <= GLOBALS->tims.last) {
                if (gw_named_markers_find(self->markers, temp) == NULL) {
                    gw_marker_set_enabled(marker, TRUE);
                    gw_marker_set_position(marker, temp);
                    gw_marker_set_alias(marker, alias_str);
                }
            }
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void gw_named_marker_dialog_class_init(GwNamedMarkerDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->dispose = gw_named_marker_dialog_dispose;
    object_class->finalize = gw_named_marker_dialog_finalize;
    object_class->constructed = gw_named_marker_dialog_constructed;
    object_class->set_property = gw_named_marker_dialog_set_property;

    dialog_class->response = gw_named_marker_dialog_response;

    properties[PROP_MARKERS] =
        g_param_spec_object("markers",
                            "Markers",
                            "The named markers",
                            GW_TYPE_NAMED_MARKERS,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);

    gtk_widget_class_set_template_from_resource(
        widget_class,
        "/io/github/gtkwave/GTKWave/ui/gw-named-marker-dialog.ui");
    gtk_widget_class_bind_template_child(widget_class, GwNamedMarkerDialog, grid);
    gtk_widget_class_bind_template_child(widget_class, GwNamedMarkerDialog, scrolled_window);
}

gboolean on_time_entry_focus_out(GtkWidget *widget, GdkEventFocus event, gpointer user_data)
{
    GtkEntry *entry = GTK_ENTRY(widget);
    GwNamedMarkerDialog *dialog = user_data;

    GwTime time = 0;
    if (parse_time_entry(entry, dialog, &time)) {
        gchar buf[128];
        reformat_time_simple(buf, time, GLOBALS->time_dimension);
        gtk_entry_set_text(entry, buf);
    } else {
        gtk_entry_set_text(entry, "");
    }

    return GDK_EVENT_PROPAGATE;
}

static void gw_named_marker_dialog_init(GwNamedMarkerDialog *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    self->time_entries = g_ptr_array_new();
    self->alias_entries = g_ptr_array_new();
}

static void gw_named_marker_dialog_constructed(GObject *object)
{
    GwNamedMarkerDialog *self = GW_NAMED_MARKER_DIALOG(object);

    G_OBJECT_CLASS(gw_named_marker_dialog_parent_class)->constructed(object);

    gchar buf[128];

    guint num_markers = gw_named_markers_get_number_of_markers(self->markers);
    for (guint i = 0; i < num_markers; i++) {
        GwMarker *marker = gw_named_markers_get(self->markers, i);

        if (i != 0) {
            GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
            gtk_grid_attach_next_to(GTK_GRID(self->grid), separator, NULL, GTK_POS_BOTTOM, 3, 1);
        }

        const gchar *name = gw_marker_get_name(marker);

        GtkWidget *label = gtk_label_new(name);
        gtk_widget_set_margin_start(label, 6);
        gtk_grid_attach_next_to(GTK_GRID(self->grid), label, NULL, GTK_POS_BOTTOM, 1, 1);

        GtkWidget *time_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(time_entry), "disabled");
        gtk_entry_set_max_length(GTK_ENTRY(time_entry), 48);
        gtk_entry_set_activates_default(GTK_ENTRY(time_entry), TRUE);
        gtk_widget_set_hexpand(time_entry, TRUE);
        gtk_grid_attach_next_to(GTK_GRID(self->grid), time_entry, label, GTK_POS_RIGHT, 1, 1);

        GtkWidget *alias_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(alias_entry), name);
        gtk_entry_set_max_length(GTK_ENTRY(alias_entry), 48);
        gtk_entry_set_activates_default(GTK_ENTRY(alias_entry), TRUE);
        gtk_widget_set_margin_end(alias_entry, 6);
        gtk_widget_set_hexpand(alias_entry, TRUE);
        gtk_grid_attach_next_to(GTK_GRID(self->grid), alias_entry, time_entry, GTK_POS_RIGHT, 1, 1);

        g_signal_connect(time_entry, "focus-out-event", G_CALLBACK(on_time_entry_focus_out), NULL);

        g_ptr_array_add(self->time_entries, time_entry);
        g_ptr_array_add(self->alias_entries, alias_entry);

        if (gw_marker_is_enabled(marker)) {
            reformat_time_simple(buf, gw_marker_get_position(marker), GLOBALS->time_dimension);
            gtk_entry_set_text(GTK_ENTRY(time_entry), buf);
            gtk_entry_set_text(GTK_ENTRY(alias_entry), gw_marker_get_alias(marker));
        }
    }

    gboolean use_header_bar = FALSE;
    g_object_get(object, "use-header-bar", &use_header_bar, NULL);
    if (!use_header_bar) {
        gtk_widget_set_margin_bottom(self->scrolled_window, 12);
    }

    gtk_widget_show_all(self->grid);
}

GtkWidget *gw_named_marker_dialog_new(GwNamedMarkers *markers)
{
    gboolean use_header = FALSE;
    g_object_get(gtk_settings_get_default(), "gtk-dialogs-use-header", &use_header, NULL);

    return g_object_new(GW_TYPE_NAMED_MARKER_DIALOG,
                        "markers",
                        markers,
                        "use-header-bar",
                        use_header,
                        NULL);
}
