/*
 * Copyright (c) Tony Bybell 1999-2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "debug.h"
#include "symbol.h"
#include "currenttime.h"
#include "fgetdynamic.h"

/* only for use locally */
struct wave_logfile_lines_t
{
    struct wave_logfile_lines_t *next;
    char *text;
};

struct logfile_instance_t
{
    struct logfile_instance_t *next;
    GtkWidget *window;
    GtkWidget *text;

    GtkTextTag *bold_tag;
    GtkTextTag *mono_tag;
    GtkTextTag *size_tag;

    char default_text[1];
};

#define log_collection (*((struct logfile_instance_t **)GLOBALS->logfiles))

/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static void log_text(GtkTextBuffer *buffer, void *font, const char *str)
{
    (void)font;

    gtk_text_buffer_insert_with_tags(buffer,
                                     &GLOBALS->iter_logfile_c_2,
                                     str,
                                     -1,
                                     GLOBALS->mono_tag_logfile_c_1,
                                     GLOBALS->size_tag_logfile_c_1,
                                     NULL);
}

static void log_text_bold(GtkTextBuffer *buffer, void *font, const char *str)
{
    (void)font;

    gtk_text_buffer_insert_with_tags(buffer,
                                     &GLOBALS->iter_logfile_c_2,
                                     str,
                                     -1,
                                     GLOBALS->bold_tag_logfile_c_2,
                                     GLOBALS->mono_tag_logfile_c_1,
                                     GLOBALS->size_tag_logfile_c_1,
                                     NULL);
}

static void log_text_number(GtkTextBuffer *buffer, void *font, const char *str)
{
    (void)font;

    GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *number_tag = gtk_text_tag_table_lookup(tags, "number");

    gtk_text_buffer_insert_with_tags(buffer,
                                     &GLOBALS->iter_logfile_c_2,
                                     str,
                                     -1,
                                     GLOBALS->bold_tag_logfile_c_2,
                                     GLOBALS->mono_tag_logfile_c_1,
                                     GLOBALS->size_tag_logfile_c_1,
                                     number_tag,
                                     NULL);
}

static void log_realize_text(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    /* nothing for now */
}

static void center_op(void)
{
    GwTime middle = 0, width;

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    GwTime primary_pos = gw_marker_get_position(primary_marker);

    if (!gw_marker_is_enabled(primary_marker) || primary_pos < GLOBALS->tims.first ||
        primary_pos > GLOBALS->tims.last) {
        if (GLOBALS->tims.end > GLOBALS->tims.last)
            GLOBALS->tims.end = GLOBALS->tims.last;
        middle = (GLOBALS->tims.start / 2) + (GLOBALS->tims.end / 2);
        if ((GLOBALS->tims.start & 1) && (GLOBALS->tims.end & 1))
            middle++;
    } else {
        middle = primary_pos;
    }

    width = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
    GLOBALS->tims.start = time_trunc(middle - (width / 2));
    if (GLOBALS->tims.start + width > GLOBALS->tims.last)
        GLOBALS->tims.start = time_trunc(GLOBALS->tims.last - width);
    if (GLOBALS->tims.start < GLOBALS->tims.first)
        GLOBALS->tims.start = GLOBALS->tims.first;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                             GLOBALS->tims.timecache = GLOBALS->tims.start);

    fix_wavehadj();

    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed"); /* force zoom update */
    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                          "value_changed"); /* force zoom update */
}

static void handle_number(const char *number)
{
    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);

    GwTime tm = unformat_time(number, time_dimension);

    // g_printerr("Time: '%s' (%" GW_TIME_FORMAT ")\n", number, tm);

    if ((tm >= GLOBALS->tims.first) && (tm <= GLOBALS->tims.last)) {
        GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
        gw_marker_set_position(primary_marker, tm);
        gw_marker_set_enabled(primary_marker, TRUE);

        GwMarker *ghost_marker = gw_project_get_ghost_marker(GLOBALS->project);
        gw_marker_set_enabled(ghost_marker, FALSE);

        center_op();
        redraw_signals_and_waves();
        update_time_box(); /* centering problem in GTK2 */
    }
}

static char *get_tagged_text(GtkTextBuffer *buffer, const GtkTextIter *position, GtkTextTag *tag)
{
    GtkTextIter start = *position;
    if (!gtk_text_iter_starts_tag(&start, tag)) {
        gtk_text_iter_backward_to_tag_toggle(&start, tag);
    }

    GtkTextIter end = *position;
    if (!gtk_text_iter_ends_tag(&end, tag)) {
        gtk_text_iter_forward_to_tag_toggle(&end, tag);
    }

    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static gboolean button_release_event(GtkWidget *text, GdkEventButton *event)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *number_tag = gtk_text_tag_table_lookup(tags, "number");

    int x = 0;
    int y = 0;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text),
                                          GTK_TEXT_WINDOW_WIDGET,
                                          event->x,
                                          event->y,
                                          &x,
                                          &y);

    GtkTextIter position;
    if (gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(text), &position, NULL, x, y)) {
        if (gtk_text_iter_has_tag(&position, number_tag)) {
            char *str = get_tagged_text(buffer, &position, number_tag);

            handle_number(str);

            g_free(str);
        }
    }

    return FALSE; /* call remaining handlers... */
}

static gboolean motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *number_tag = gtk_text_tag_table_lookup(tags, "number");

    static GdkCursor *cursor_default;
    static GdkCursor *cursor_pointer;
    if (cursor_default == NULL || cursor_pointer == NULL) {
        GdkDisplay *display = gtk_widget_get_display(widget);
        cursor_default = gdk_cursor_new_from_name(display, "default");
        cursor_pointer = gdk_cursor_new_from_name(display, "pointer");
    }

    int x = 0;
    int y = 0;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget),
                                          GTK_TEXT_WINDOW_WIDGET,
                                          event->x,
                                          event->y,
                                          &x,
                                          &y);

    static gboolean hovering = FALSE;

    GtkTextIter position;
    if (gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(widget), &position, NULL, x, y)) {
        GdkWindow *window = gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT);

        gboolean has_tag = gtk_text_iter_has_tag(&position, number_tag);
        if (!hovering && has_tag) {
            gdk_window_set_cursor(window, cursor_pointer);
        } else if (hovering && !has_tag) {
            gdk_window_set_cursor(window, cursor_default);
        }

        hovering = has_tag;
    }

    return FALSE;
}

/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_log_text(GtkWidget **textpnt)
{
    GtkWidget *text;
    GtkWidget *scrolled_window;

    text = gtk_text_view_new();
    gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                   &GLOBALS->iter_logfile_c_2);
    GLOBALS->bold_tag_logfile_c_2 =
        gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                   "bold",
                                   "weight",
                                   PANGO_WEIGHT_BOLD,
                                   NULL);
    GLOBALS->mono_tag_logfile_c_1 =
        gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                   "monospace",
                                   "family",
                                   "monospace",
                                   NULL);

    GLOBALS->size_tag_logfile_c_1 =
        gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                   "fsiz",
                                   "size",
                                   (GLOBALS->use_big_fonts ? 12 : 8) * PANGO_SCALE,
                                   NULL);

    gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                               "number",
                               "foreground",
                               "blue",
                               NULL);

    *textpnt = text;
    gtk_widget_set_size_request(GTK_WIDGET(text), 100, 100);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text), FALSE);
    gtk_widget_show(text);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 5);
    gtk_widget_show(scrolled_window);

    /* Add a handler to put a message in the text widget when it is realized */
    g_signal_connect(text, "realize", G_CALLBACK(log_realize_text), NULL);

    g_signal_connect(text, "button_release_event", G_CALLBACK(button_release_event), NULL);
    g_signal_connect(text, "motion_notify_event", G_CALLBACK(motion_notify_event), NULL);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);
    return (scrolled_window);
}

/***********************************************************************************/

static void ok_callback(GtkWidget *widget, GtkWidget *cached_window)
{
    (void)widget;

    struct logfile_instance_t *l = log_collection;
    struct logfile_instance_t *lprev = NULL;

    while (l) {
        if (l->window == cached_window) {
            if (lprev) {
                lprev->next = l->next;
            } else {
                log_collection = l->next;
            }

            free(l); /* deliberately not free_2 */
            break;
        }

        lprev = l;
        l = l->next;
    }

    DEBUG(printf("OK\n"));
    gtk_widget_destroy(cached_window);
}

static void destroy_callback(GtkWidget *widget, GtkWidget *cached_window)
{
    (void)cached_window;

    ok_callback(widget, widget);
}

static void load_log(GtkTextBuffer *text_buffer, FILE *handle)
{
    gtk_text_buffer_set_text(text_buffer, "", 0);
    gtk_text_buffer_get_start_iter(text_buffer, &GLOBALS->iter_logfile_c_2);

    log_text_bold(text_buffer, NULL, "Click");
    log_text(text_buffer, NULL, " on numbers to jump to that time value in the wave viewer.\n");
    log_text(text_buffer, NULL, " \n");

    GRegex *number_regex =
        g_regex_new("(\\d+(?:\\.\\d+)?(?:\\s*[munpfaz]?s(?=\\W|$))?)", 0, 0, NULL);
    g_assert(number_regex != NULL);

    while (!feof(handle)) {
        char *line = fgetmalloc(handle);
        if (line == NULL) {
            break;
        }

        char **parts = g_regex_split(number_regex, line, 0);

        if (parts != NULL) {
            gboolean is_number = FALSE;
            for (char **iter = parts; *iter != NULL; iter++) {
                if (is_number) {
                    log_text_number(text_buffer, NULL, *iter);
                } else {
                    log_text(text_buffer, NULL, *iter);
                }

                is_number = !is_number;
            }

            g_strfreev(parts);
        }

        log_text(text_buffer, NULL, "\n");

        free(line);
    }

    g_regex_unref(number_regex);
}

void logbox(const char *title, int width, const char *default_text)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox, *button1;
    GtkWidget *label, *separator;
    GtkWidget *ctext;
    GtkWidget *text;
    struct logfile_instance_t *log_c;

    FILE *handle = fopen(default_text, "rb");
    if (!handle) {
        char *buf = malloc_2(strlen(default_text) + 128);
        sprintf(buf, "Could not open logfile '%s'\n", default_text);
        status_text(buf);
        free_2(buf);
        return;
    }

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    /* nothing */

    /* create a new nonmodal window */
    window =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    if (GLOBALS->use_big_fonts || GLOBALS->fontname_logfile) {
        gtk_widget_set_size_request(GTK_WIDGET(window), width * 1.8, 600);
    } else {
        gtk_widget_set_size_request(GTK_WIDGET(window), width, 400);
    }
    gtk_window_set_title(GTK_WINDOW(window), title);

    g_signal_connect(window, "delete_event", (GCallback)destroy_callback, window);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    label = gtk_label_new(default_text);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);

    ctext = create_log_text(&text);
    gtk_box_pack_start(GTK_BOX(vbox), ctext, TRUE, TRUE, 0);
    gtk_widget_show(ctext);

    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show(separator);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    button1 = gtk_button_new_with_label("Close Logfile");
    gtk_widget_set_size_request(button1, 100, -1);
    g_signal_connect(button1, "clicked", G_CALLBACK(ok_callback), window);
    gtk_widget_show(button1);
    gtk_box_pack_start(GTK_BOX(hbox), button1, TRUE, TRUE, 0);
    gtk_widget_set_can_default(button1, TRUE);
    g_signal_connect_swapped(button1, "realize", (GCallback)gtk_widget_grab_default, button1);

    gtk_widget_show(window);

    load_log(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)), handle);

    fclose(handle);

    // deliberately not calloc_2, needs to be persistent!
    log_c = calloc(1, sizeof(struct logfile_instance_t) + strlen(default_text));
    strcpy(log_c->default_text, default_text);
    log_c->window = window;
    log_c->text = text;
    log_c->next = log_collection;
    log_c->bold_tag = GLOBALS->bold_tag_logfile_c_2;
    log_c->mono_tag = GLOBALS->mono_tag_logfile_c_1;
    log_c->size_tag = GLOBALS->size_tag_logfile_c_1;
    log_collection = log_c;
}

void logbox_reload(void)
{
    struct logfile_instance_t *l = log_collection;

    while (l) {
        GLOBALS->bold_tag_logfile_c_2 = l->bold_tag;
        GLOBALS->mono_tag_logfile_c_1 = l->mono_tag;
        GLOBALS->size_tag_logfile_c_1 = l->size_tag;

        FILE *handle = fopen(l->default_text, "rb");
        if (!handle) {
            char *buf = malloc_2(strlen(l->default_text) + 128);
            sprintf(buf, "Could not open logfile '%s'\n", l->default_text);
            status_text(buf);
            free_2(buf);
            return;
        }

        load_log(gtk_text_view_get_buffer(GTK_TEXT_VIEW(l->text)), handle);

        fclose(handle);

        l = l->next;
    }
}
