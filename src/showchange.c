/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "analyzer.h"
#include "symbol.h"
#include "debug.h"

typedef struct {
    const char* label;
    uint64_t value;
} LabeledUint64;

typedef struct {
    const char *label;
    int64_t value;
} LabeledInt64;

static const LabeledUint64 ATTRIBUTES[] = {
    { "Right Justify", TR_RJUSTIFY },
    { "Invert", TR_INVERT },
    { "Reverse", TR_REVERSE },
    { "Exclude", TR_EXCLUDE },
    { "Popcnt", TR_POPCNT },
    { "Find First One", TR_FFO },
    {}
};

static const LabeledUint64 BASES[] = {
    { "Hex", TR_HEX },
    { "Decimal", TR_DEC },
    { "Signed Decimal", TR_SIGNED },
    { "Binary", TR_BIN },
    { "Octal", TR_OCT },
    { "ASCII", TR_ASCII },
    { "Real", TR_REAL },
    { "Time", TR_TIME | TR_DEC },
    { "Enum", TR_ENUM | TR_BIN },
    {}
};

static const LabeledInt64 COLORS[] = {
    { "Normal", WAVE_COLOR_NORMAL },
    { "Red", WAVE_COLOR_RED },
    { "Orange", WAVE_COLOR_ORANGE },
    { "Yellow", WAVE_COLOR_YELLOW },
    { "Green", WAVE_COLOR_GREEN },
    { "Blue", WAVE_COLOR_BLUE },
    { "Indigo", WAVE_COLOR_INDIGO },
    { "Violet", WAVE_COLOR_VIOLET },
    // { "Cycle", WAVE_COLOR_CYCLE },
    {}
};

static const LabeledUint64 ANALOG_FORMATS[] = {
    { "Off", 0 },
    { "Step", TR_ANALOG_STEP },
    { "Interpolated", TR_ANALOG_INTERPOLATED },
    { "Interpolated Annotated", TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP },
    {}
};

static const LabeledUint64 ANALOG_RESIZING[] = {
    { "Screen Data", 0 },
    { "All Data", TR_ANALOG_FULLSCALE },
    {}
};

enum {
    COLUMN_LABEL,
    COLUMN_VALUE,
    COLUMN_COLORED_DOT // only used for color combo box
};

static GtkListStore *flags_list_store_new(const LabeledUint64 *flags) {
    GtkListStore *list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_UINT64);

    GtkTreeIter iter;
    const LabeledUint64 *flag;
    for (flag = flags; flag->label; flag++) {
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
            COLUMN_LABEL, flag->label,
            COLUMN_VALUE, flag->value,
            -1
        );
    }

    return list_store;
}

static GtkWidget *flags_combo_box_new(const LabeledUint64 *flags, TraceFlagsType active) {
    GtkListStore *list_store = flags_list_store_new(flags);

    GtkWidget *combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));

    GtkCellRenderer *cell_renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), cell_renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), cell_renderer,
        "text", COLUMN_LABEL,
        NULL
    );

    g_object_unref(list_store);

    // Set active entry
    int i = 0;
    const LabeledUint64 *flag;
    for (flag = flags; flag->label; flag++) {
        if (flag->value == active) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), i);
            break;
        }

        i++;
    }

    return combo_box;
}

static GtkListStore *color_list_store_new(void) {
    GtkListStore *list_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT64, G_TYPE_STRING);

    GtkTreeIter iter;
    const LabeledInt64 *color;
    for (color = COLORS; color->label; color++) {
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
            COLUMN_LABEL, color->label,
            COLUMN_VALUE, color->value,
            -1
        );

        if (color->value != WAVE_COLOR_NORMAL && color->value != WAVE_COLOR_CYCLE) {
            const uint32_t RAINBOW_COLORS[] = WAVE_RAINBOW_RGB;

            gchar *dot = g_strdup_printf(
                "<span foreground=\"#%06X\">&#x2B24;</span>",
                RAINBOW_COLORS[color->value - 1]
            );
            gtk_list_store_set(list_store, &iter, COLUMN_COLORED_DOT, dot, -1);
            g_free(dot);
        }
    }

    return list_store;
}

static GtkWidget *color_combo_box_new(void) {
    GtkListStore *color_list_store = color_list_store_new();

    GtkWidget *combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(color_list_store));

    GtkCellRenderer *cell_renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), cell_renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), cell_renderer,
        "text", COLUMN_LABEL,
        NULL
    );

    cell_renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), cell_renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), cell_renderer,
        "markup", COLUMN_COLORED_DOT,
        NULL
    );

    g_object_unref(color_list_store);

    return combo_box;
}

static gboolean combo_box_get_active_value(GtkWidget *combo_box, void *value) {
    GtkTreeIter iter;
    gboolean has_active;

    has_active = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo_box), &iter);

    if (has_active) {
        GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box));

        gtk_tree_model_get(model, &iter, COLUMN_VALUE, value, -1);
    }

    return has_active;
}

#if GTK_CHECK_VERSION(3,0,0)
static void add_header(GtkWidget *grid, const gchar *text) {
    gchar *markup = g_markup_printf_escaped("<b>%s</b>", text);;

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_widget_set_margin_top(label, 6);

    gtk_grid_attach_next_to(GTK_GRID(grid), label, NULL, GTK_POS_BOTTOM, 2, 1);

    g_free(markup);
}

static void add_labeled_widget(GtkWidget *grid, const gchar *text, GtkWidget *widget) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);

    gtk_grid_attach_next_to(GTK_GRID(grid), label, NULL, GTK_POS_BOTTOM, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid), widget, label, GTK_POS_RIGHT, 1, 1);
}
#else
static guint current_row;

static void add_header(GtkWidget *table, const gchar *text) {
    gchar *markup = g_markup_printf_escaped("<b>%s</b>", text);;

    GtkWidget *label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, current_row, current_row + 1);

    current_row++;

    g_free(markup);
}

static void add_labeled_widget(GtkWidget *table, const gchar *text, GtkWidget *widget) {
    GtkWidget *label = gtk_label_new(text);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, current_row, current_row + 1);
    gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, current_row, current_row + 1);

    current_row++;
}
#endif

void showchange(char *title, Trptr t, GCallback func)
{
    TraceFlagsType old_flags = t->flags;
    unsigned int old_color = t->t_color;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Trace Properties",
        GTK_WINDOW(GLOBALS->mainwindow),
#if GTK_CHECK_VERSION(3,0,0)
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
#else
        GTK_DIALOG_DESTROY_WITH_PARENT,
#endif
        "Cancel",
        GTK_RESPONSE_CANCEL,
        "Ok",
        GTK_RESPONSE_OK,
        NULL
    );
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
#else
    current_row = 0;
    GtkWidget *grid = gtk_table_new(1, 2, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(grid), 6);
    gtk_table_set_row_spacings(GTK_TABLE(grid), 6);
#endif

    GtkWidget *base_combo_box = flags_combo_box_new(BASES, old_flags & TR_NUMMASK);
    add_labeled_widget(grid, "Base", base_combo_box);

    GtkWidget *color_combo_box = color_combo_box_new();
    gtk_combo_box_set_active(GTK_COMBO_BOX(color_combo_box), old_color);
    add_labeled_widget(grid, "Color", color_combo_box);

    add_header(grid, "Analog");

    GtkWidget *analog_format_combo_box = flags_combo_box_new(ANALOG_FORMATS, old_flags & TR_ANALOGMASK);
    add_labeled_widget(grid, "Format", analog_format_combo_box);

    GtkWidget *analog_resizing_combo_box = flags_combo_box_new(ANALOG_RESIZING, old_flags & TR_ANALOG_FULLSCALE);
    add_labeled_widget(grid, "Resizing", analog_resizing_combo_box);

    add_header(grid, "Attributes");

    GSList *attribute_check_buttons = NULL;
    const LabeledUint64 *attribute;
    for (attribute = ATTRIBUTES; attribute->label; attribute++) {
        GtkWidget *check_button = gtk_check_button_new_with_label(attribute->label);
#if GTK_CHECK_VERSION(3,0,0)
        gtk_widget_set_hexpand(check_button, TRUE);
        gtk_widget_set_margin_start(check_button, 12);
        gtk_grid_attach_next_to(GTK_GRID(grid), check_button, NULL, GTK_POS_BOTTOM, 2, 1);
#else
        gtk_table_attach_defaults(GTK_TABLE(grid), check_button, 0, 2, current_row, current_row + 1);
        current_row++;
#endif

        gboolean active = old_flags & attribute->value;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), active);

        attribute_check_buttons = g_slist_prepend(attribute_check_buttons, check_button);
    }
    attribute_check_buttons = g_slist_reverse(attribute_check_buttons);

    gtk_widget_show_all(grid);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        TraceFlagsType new_flags = old_flags;
        unsigned int new_color = old_color;

        uint64_t base;
        if (combo_box_get_active_value(base_combo_box, &base)) {
            new_flags = new_flags & ~TR_NUMMASK | base;
        }

        int64_t color;
        if (combo_box_get_active_value(color_combo_box, &color)) {
            new_color = color;
        }

        uint64_t analog_format;
        if (combo_box_get_active_value(analog_format_combo_box, &analog_format)) {
            new_flags = new_flags & ~TR_ANALOGMASK | analog_format;
        }

        uint64_t analog_resizing;
        if (combo_box_get_active_value(analog_resizing_combo_box, &analog_resizing)) {
            new_flags = new_flags & ~TR_ANALOG_FULLSCALE | analog_resizing;
        }

        int i = 0;
	GSList *list;
        for (list = attribute_check_buttons; list != NULL; list = list->next) {
            GtkWidget *attribute_check_button = list->data;

            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(attribute_check_button))) {
                new_flags |= ATTRIBUTES[i].value;
            } else {
                new_flags &= ~ATTRIBUTES[i].value;
            }

            i++;
        }

        if (t->flags != new_flags || t->t_color != new_color) {
            t->flags = new_flags;
            t->t_color = new_color;
            t->minmax_valid = 0; // force analog traces to regenerate if necessary

            func();
        }
    }

    g_slist_free(attribute_check_buttons);
    gtk_widget_destroy(dialog);
}
