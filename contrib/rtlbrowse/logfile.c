/*
 * Copyright (c) Tony Bybell 1999-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>

#include <string.h>
#include "splay.h"
#include "vlex.h"
#include "vlex.yy.h"
#include "jrb.h"
#include "wavelink.h"
#include "stem_recurse.h"

extern GtkWidget *notebook;
#define XXX_GTK_STOCK_GO_BACK "go-previous"
#define XXX_GTK_STOCK_GO_FORWARD "go-next"
#define XXX_GTK_STOCK_CLOSE "window-close"

GwTime old_marker = 0;
unsigned old_marker_set = 0;

/* only for use locally */
struct wave_logfile_lines_t
{
    struct wave_logfile_lines_t *next;
    int line_no, tok;
    char *text;
};

struct logfile_context_t
{
    ds_Tree *which;
    char *title;
    int display_mode;
    int width;

    JRB varnames;
    int resolved;
};

struct text_find_t
{
    struct text_find_t *next;
    GtkWidget *text, *window, *button;
    struct logfile_context_t *ctx;

    gint line, offs; /* of insert marker */
    gint srch_line, srch_offs; /* for search, to avoid duplication */

    GtkTextTag *bold_tag;
    GtkTextTag *dgray_tag, *lgray_tag;
    GtkTextTag *blue_tag, *fwht_tag;
    GtkTextTag *mono_tag;
    GtkTextTag *size_tag;
};

void bwlogbox(char *title, int width, ds_Tree *t, int display_mode);
void bwlogbox_2(struct logfile_context_t *ctx,
                GtkWidget *window,
                GtkWidget *button,
                GtkWidget *text);
gboolean update_ctx_when_idle(gpointer textview_or_dummy);

static struct text_find_t *text_root = NULL;
static struct text_find_t *selected_text_via_tab = NULL;
static GtkWidget *matchcase_checkbutton = NULL;
static gboolean matchcase_active = FALSE;

/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static gpointer fontx = NULL;

static GtkTextIter iterx;
static GtkTextTag *bold_tag = NULL;
static GtkTextTag *dgray_tag = NULL, *lgray_tag = NULL;
static GtkTextTag *blue_tag = NULL, *fwht_tag = NULL;
static GtkTextTag *mono_tag = NULL;
static GtkTextTag *size_tag = NULL;

void
switch_page_cb (
  GtkNotebook* self,
  GtkWidget* widget,
  guint page_num)
{
    (void)self;
    (void)page_num;
    struct text_find_t *tr = text_root;

    while (tr) {
        if (tr->window == widget) {
            if (selected_text_via_tab != tr) {
                selected_text_via_tab = tr;
                //printf("Expose: %p '%s'\n", widget, tr->ctx->title);
            }
            return;
        }
        tr = tr->next;
    }
    selected_text_via_tab = NULL;
}


/*
 * Create a button inside a box with given
 * stock icon id and tooltip text
 */
void create_button_in_box(GtkWidget *box,
                          const gchar *stock_id,
                          const char *tooltip_text,
                          GCallback callback,
                          gpointer user_data)
{
    GtkWidget *button = gtk_button_new_from_icon_name(stock_id);

    gtk_button_set_has_frame(GTK_BUTTON(button), FALSE);
    gtk_widget_set_tooltip_text(button, tooltip_text);
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);

    gtk_box_append(GTK_BOX(box), button);
    g_signal_connect(button, "clicked", G_CALLBACK(callback), user_data);
}

void read_insert_position(struct text_find_t *tr)
{
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
    GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
    GtkTextIter iter;

    gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);

    tr->line = gtk_text_iter_get_line(&iter);
    tr->offs = gtk_text_iter_get_line_offset(&iter);
}

void set_insert_position(struct text_find_t *tr)
{
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
    GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
    GtkTextIter iter;
    gint llen;

    gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);
    gtk_text_iter_set_line(&iter, tr->line);
    llen = gtk_text_iter_get_chars_in_line(&iter);
    tr->offs = (tr->offs > llen) ? llen : tr->offs;
    gtk_text_iter_set_line_offset(&iter, tr->offs);

    gtk_text_buffer_place_cursor(tb, &iter);
}

static void forward_chars_with_skipping(GtkTextIter *iter, gint count)
{
    gint i;

    g_return_if_fail(count >= 0);

    i = count;

    while (i > 0) {
        gboolean ignored = FALSE;

        if (gtk_text_iter_get_char(iter) == 0xFFFC)
            ignored = TRUE;

        gtk_text_iter_forward_char(iter);

        if (!ignored)
            --i;
    }
}

static gboolean iter_forward_search_caseins(const GtkTextIter *iter,
                                            gchar *str,
                                            GtkTextIter *match_start,
                                            GtkTextIter *match_end)
{
    GtkTextIter start = *iter;
    GtkTextIter next;
    gchar *line_text, *found, *pnt;
    gint offset;
    gchar *strcaseins;

    if (!str)
        return (FALSE);

    pnt = strcaseins = strdup(str);
    while (*pnt) {
        *pnt = toupper((int)(unsigned char)*pnt);
        pnt++;
    }

    for (;;) {
        next = start;
        gtk_text_iter_forward_line(&next);

        /* No more text in buffer */
        if (gtk_text_iter_equal(&start, &next)) {
            free(strcaseins);
            return (FALSE);
        }

        pnt = line_text = gtk_text_iter_get_visible_text(&start, &next);
        while (*pnt) {
            *pnt = toupper((int)(unsigned char)*pnt);
            pnt++;
        }
        found = strstr(line_text, strcaseins);
        if (found) {
            gchar cached = *found;
            *found = 0;
            offset = g_utf8_strlen(line_text, -1);
            *found = cached;
            break;
        }
        g_free(line_text);

        start = next;
    }

    *match_start = start;
    forward_chars_with_skipping(match_start, offset);

    offset = g_utf8_strlen(str, -1);
    *match_end = *match_start;
    forward_chars_with_skipping(match_end, offset);

    free(strcaseins);
    return (TRUE);
}

void tr_search_forward(char *str, gboolean noskip)
{
    struct text_find_t *tr = selected_text_via_tab;
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
    GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
    GtkTextIter iter;
    gboolean found = FALSE;
    GtkTextIter match_start;
    GtkTextIter match_end;

    if (tr == NULL || tr->text == NULL) {
        return;
    }

    gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);

    tr->line = gtk_text_iter_get_line(&iter);
    tr->offs = gtk_text_iter_get_line_offset(&iter);

    if (!noskip)
        if (tr->line == tr->srch_line && tr->offs == tr->srch_offs) {
            // Ensure search starts from the next character
            // Otherwise "Search Forward" or [ENTER] can never jumps to the
            // next place
            gtk_text_iter_forward_char(&iter);
        }

    if (str) {
        if (!matchcase_active) {
            found = iter_forward_search_caseins(&iter, str, &match_start, &match_end);
        } else {
            found = gtk_text_iter_forward_search(&iter,
                                                 str,
                                                 GTK_TEXT_SEARCH_TEXT_ONLY,
                                                 &match_start,
                                                 &match_end,
                                                 NULL);
        }
        if (!found) {
            gtk_text_buffer_get_start_iter(tb, &iter);
            if (!matchcase_active) {
                found = iter_forward_search_caseins(&iter, str, &match_start, &match_end);
            } else {
                found = gtk_text_iter_forward_search(&iter,
                                                     str,
                                                     GTK_TEXT_SEARCH_TEXT_ONLY,
                                                     &match_start,
                                                     &match_end,
                                                     NULL);
            }
        }
    }

    if (found) {
        gtk_text_buffer_select_range(tb, &match_start, &match_end);
        read_insert_position(tr);
        tr->srch_line = tr->line;
        tr->srch_offs = tr->offs;

        /* tm = gtk_text_buffer_get_insert(tb); */ /* scan-build : never read */

        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(tr->text),
                                     &match_start,
                                     0.0,
                                     TRUE,
                                     0.0,
                                     0.5);
    } else {
        gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);
        gtk_text_buffer_select_range(tb, &iter, &iter);
    }
}

static gboolean iter_backward_search_caseins(const GtkTextIter *iter,
                                             gchar *str,
                                             GtkTextIter *match_start,
                                             GtkTextIter *match_end)
{
    GtkTextIter start = *iter;
    GtkTextIter next;
    gchar *line_text;
    int offset;
    int cmpval;

    if (!str)
        return (FALSE);
    offset = g_utf8_strlen(str, -1);

    for (;;) {
        if (gtk_text_iter_is_start(&start)) {
            break;
        }

        next = start;
        forward_chars_with_skipping(&next, offset);
        line_text = gtk_text_iter_get_visible_text(&start, &next);
        cmpval = strcasecmp(str, line_text);
        g_free(line_text);
        if (!cmpval) {
            *match_start = start;
            *match_end = next;
            return (TRUE);
        }

        gtk_text_iter_backward_char(&start);
    }

    return (FALSE);
}

void tr_search_backward(char *str)
{
    struct text_find_t *tr = selected_text_via_tab;

    if (tr == NULL || tr->text == NULL) {
        return;
    }

    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
    GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
    GtkTextIter iter;
    gboolean found = FALSE;
    GtkTextIter match_start;
    GtkTextIter match_end;

    gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);

    tr->line = gtk_text_iter_get_line(&iter);
    tr->offs = gtk_text_iter_get_line_offset(&iter);

    if (tr->line == tr->srch_line && tr->offs == tr->srch_offs) {
        gtk_text_iter_backward_char(&iter);
    }

    if (str) {
        if (!matchcase_active) {
            found = iter_backward_search_caseins(&iter, str, &match_start, &match_end);
        } else {
            found = gtk_text_iter_backward_search(&iter,
                                                  str,
                                                  GTK_TEXT_SEARCH_TEXT_ONLY,
                                                  &match_start,
                                                  &match_end,
                                                  NULL);
        }
        if (!found) {
            gtk_text_buffer_get_end_iter(tb, &iter);
            if (!matchcase_active) {
                found = iter_backward_search_caseins(&iter, str, &match_start, &match_end);
            } else {
                found = gtk_text_iter_backward_search(&iter,
                                                      str,
                                                      GTK_TEXT_SEARCH_TEXT_ONLY,
                                                      &match_start,
                                                      &match_end,
                                                      NULL);
            }
        }
    }

    if (found) {
        gtk_text_buffer_select_range(tb, &match_start, &match_end);
        read_insert_position(tr);
        tr->srch_line = tr->line;
        tr->srch_offs = tr->offs;

        /* tm = gtk_text_buffer_get_insert(tb); */ /* scan-build : never read */
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(tr->text),
                                     &match_start,
                                     0.0,
                                     TRUE,
                                     0.0,
                                     0.5);
    } else {
        gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);
        gtk_text_buffer_select_range(tb, &iter, &iter);
    }
}

static char *search_string = NULL;

static void search_backward(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    tr_search_backward(search_string);
}


static void search_forward(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    tr_search_forward(search_string, FALSE);
}

/* Signal callback for the filter widget.
   This also catch the return key to update the signal area.  */
static void find_edit_cb(GtkWidget *self, gpointer no_skip)
{
    const char *t = gtk_editable_get_text(GTK_EDITABLE(self));
    /* Maybe this test is too strong ?  */
    if (search_string) {
        free(search_string);
        search_string = NULL;
    }
    if (t == NULL || *t == 0) {
    } else {
        search_string = strdup(t);
    }
    tr_search_forward(search_string, (gboolean)no_skip);
}

static void toggle_callback(GtkWidget *widget)
{

    matchcase_active = gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)) != 0;
    tr_search_forward(search_string, TRUE);
}

void create_toolbar(GtkWidget *grid)
{
    GtkWidget *find_label;
    GtkWidget *find_entry;
    GtkWidget *box;
    GtkWidget *hbox;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
    gtk_widget_set_size_request(hbox, -1 ,40);

    gtk_grid_attach(GTK_GRID(grid), hbox, 0, 1, 1, 1);
    gtk_widget_set_hexpand(GTK_WIDGET(hbox), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(hbox), FALSE); /* otherwise the bottom part stretches */

    find_label = gtk_label_new("Find:");
    gtk_box_append(GTK_BOX(hbox), find_label);

    find_entry = gtk_search_entry_new();
    g_signal_connect(find_entry, "search_changed", G_CALLBACK(find_edit_cb), (void*)TRUE);
    g_signal_connect (find_entry, "activate", G_CALLBACK (find_edit_cb), FALSE);

    gtk_box_append(GTK_BOX(hbox), find_entry);

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    create_button_in_box(GTK_WIDGET(box),
                         XXX_GTK_STOCK_GO_BACK,
                         "Search Back",
                         G_CALLBACK(search_backward),
                         NULL);

    create_button_in_box(GTK_WIDGET(box),
                         XXX_GTK_STOCK_GO_FORWARD,
                         "Search Forward",
                         G_CALLBACK(search_forward),
                         NULL);

    gtk_box_append(GTK_BOX(hbox), box);

    matchcase_checkbutton = gtk_check_button_new_with_label("Match case");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(matchcase_checkbutton), matchcase_active);
    g_signal_connect(matchcase_checkbutton, "toggled", G_CALLBACK(toggle_callback), NULL);

    gtk_box_append(GTK_BOX(hbox), matchcase_checkbutton);
}

static int is_identifier(char ch)
{
    int rc = ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ||
             ((ch >= '0') && (ch <= '9')) || (ch == '_') || (ch == '$');

    return (rc);
}

static char *hexify(char *s)
{
    int len = strlen(s);

    if (len < 4) {
        char *s2 = malloc(len + 1 + 1);
        int idx;

        s2[0] = 'b';
        for (idx = 0; idx < len; idx++) {
            s2[idx + 1] = toupper((int)(unsigned char)s[idx]);
        }
        s2[idx + 1] = 0;

        free(s);
        return (s2);
    } else {
        int skip = len & 3;
        int siz = ((len + 3) / 4) + 1;
        char *sorig = s;
        char *s2 = calloc(1, siz);
        int idx;
        char *pnt = s2;
        char arr[5] = {0, 0, 0, 0, 0}; /* scan-build : but arr[4] should work ok */

        while (*s) {
            char isx, isz;
            int val;

            if (!skip) {
                arr[0] = toupper((int)(unsigned char)*(s++));
                arr[1] = toupper((int)(unsigned char)*(s++));
                arr[2] = toupper((int)(unsigned char)*(s++));
                arr[3] = toupper((int)(unsigned char)*(s++));
            } else {
                int j = 3;
                for (idx = skip - 1; idx >= 0; idx--) {
                    arr[j] = toupper((int)(unsigned char)s[idx]);
                    j--;
                }
                for (idx = j; idx >= 0; idx--) {
                    arr[idx] = ((arr[j + 1] == 'X') || (arr[j + 1] == 'Z')) ? arr[j + 1] : '0';
                }

                s += skip;
                skip = 0;
            }

            isx = isz = 0;
            val = 0;
            for (idx = 0; idx < 4; idx++) {
                val <<= 1;
                if (arr[idx] == '0')
                    continue;
                if (arr[idx] == '1') {
                    val |= 1;
                    continue;
                }
                if (arr[idx] == 'Z') {
                    isz++;
                    continue;
                }
                isx++;
            }

            if (isx) {
                *(pnt++) = (isx == 4) ? 'x' : 'X';
            } else if (isz) {
                *(pnt++) = (isz == 4) ? 'z' : 'Z';
            } else {
                *(pnt++) = "0123456789ABCDEF"[val];
            }
        }

        free(sorig);
        return (s2);
    }
}

int fst_alpha_strcmpeq(const char *s1, const char *s2)
{
    for (;;) {
        char c1 = *s1;
        char c2 = *s2;

        switch (c1) {
            case ' ':
            case '\t':
            case '[':
                c1 = 0;

            default:
                break;
        }

        switch (c2) {
            case ' ':
            case '\t':
            case '[':
                c2 = 0;

            default:
                break;
        }

        if (c1 != c2) {
            return (1);
        }
        if (!c1)
            break;

        s1++;
        s2++;
    }

    return (0);
}

void log_text(GtkWidget *text, gpointer font, char *str)
{
    (void)font;

    gtk_text_buffer_insert_with_tags(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                     &iterx,
                                     str,
                                     -1,
                                     mono_tag,
                                     size_tag,
                                     NULL);
}

void log_text_bold(GtkWidget *text, gpointer font, char *str)
{
    (void)font;

    gtk_text_buffer_insert_with_tags(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                     &iterx,
                                     str,
                                     -1,
                                     bold_tag,
                                     mono_tag,
                                     size_tag,
                                     fwht_tag,
                                     blue_tag,
                                     NULL);
}

void log_text_active(GtkWidget *text, gpointer font, char *str)
{
    (void)font;

    gtk_text_buffer_insert_with_tags(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                     &iterx,
                                     str,
                                     -1,
                                     dgray_tag,
                                     mono_tag,
                                     size_tag,
                                     NULL);
}

void log_text_prelight(GtkWidget *text, gpointer font, char *str)
{
    (void)font;

    gtk_text_buffer_insert_with_tags(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                     &iterx,
                                     str,
                                     -1,
                                     lgray_tag,
                                     mono_tag,
                                     size_tag,
                                     NULL);
}

/* lxt2 iteration handling... (lxt2 doesn't have a direct "value at" function) */
static JRB lx2vals = NULL;

static void lx2_iter_fn(struct lxt2_rd_trace **lt,
                        lxtint64_t *pnt_time,
                        lxtint32_t *pnt_facidx,
                        char **pnt_value)
{
    (void)lt;

    if (*pnt_time <= (lxtint64_t)anno_ctx->marker) {
        JRB node = jrb_find_int(lx2vals, *pnt_facidx);
        Jval jv;

        if (node) {
            free(node->val.s);
            node->val.s = strdup(*pnt_value);
        } else {
            jv.s = strdup(*pnt_value);
            jrb_insert_int(lx2vals, *pnt_facidx, jv);
        }
    }
}

// When double-clicking an identifier in logfile.
// Try to interpert it as a sub-module of the current module.
// If such module exist. Open it in a new tab.
static void import_doubleclick(GtkWidget *text, char *s)
{
    struct text_find_t *t = text_root;
    char *s2;
    ds_Tree *ft = flattened_mod_list_root;

    while (t) {
        if (text == t->text) {
            break;
        }
        t = t->next;
    }

    if (!t)
        return;

    s2 = g_strdup_printf("%s.%s", t->ctx->which->fullname, s);

    while (ft) {
        if (!g_strcmp0(s2, ft->fullname)) {
            bwlogbox(ft->fullname, 640 + 8 * 8, ft, 0);
            break;
        }

        ft = ft->next_flat;
    }

    g_free(s2);
}

// Decide whether the text being double-clicked is a part of an valid
// verilog identifier. If so, call `import_doubleclick`
static void button_release_event(GtkGestureClick *gesture,
            guint            n_pressed,
            double           x,
            double           y,
            gpointer         text)
{
    (void)gesture;
    (void)n_pressed;
    (void)x; (void)y;
    gchar *sel;

    GtkTextIter start;
    GtkTextIter end;

    if(!gtk_text_buffer_get_selection_bounds(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                             &start,
                                             &end)) {
        return; // no text is selected
    }

    if (gtk_text_iter_compare(&start, &end) >= 0) { // start must less than end
        return;
    }

    sel = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                   &start,
                                   &end,
                                   FALSE);

    if (sel == NULL || strlen(sel) == 0) {
        return;
    }

    int len = strlen(sel);
    char *sel2;
    char ch;

    for (int i = 0; i < len; i++) {
        if (!is_identifier(sel[i]))
            goto bail;
    }

    while (gtk_text_iter_backward_char(&start)) {
        sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                        &start,
                                        &end,
                                        FALSE);
        if (sel2 == NULL)
            break;
        ch = *sel2;
        g_free(sel2);
        if (!is_identifier(ch)) {
            gtk_text_iter_forward_char(&start);
            break;
        }
    }

    gtk_text_iter_backward_char(&end);
    for (;;) {
        gtk_text_iter_forward_char(&end);
        sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                        &start,
                                        &end,
                                        FALSE);
        if (sel2 == NULL)
            break;
        ch = *(sel2 + strlen(sel2) - 1);
        g_free(sel2);
        if (!is_identifier(ch)) {
            gtk_text_iter_backward_char(&end);
            break;
        }
    }

    sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                    &start,
                                    &end,
                                    FALSE);

    import_doubleclick(text, sel2);
    g_free(sel2);
bail:
    g_free(sel);
}

/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_log_text(GtkWidget **textpnt)
{
    GtkWidget *text;
    GtkWidget *scrolled_window;
    GtkEventController *controller;

    text = gtk_text_view_new();
    gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)), &iterx);
    bold_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "bold",
                                          "weight",
                                          PANGO_WEIGHT_BOLD,
                                          NULL);
    dgray_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                           "dk_gray_background",
                                           "background",
                                           "dark gray",
                                           NULL);
    lgray_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                           "lt_gray_background",
                                           "background",
                                           "light gray",
                                           NULL);
    fwht_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "white_foreground",
                                          "foreground",
                                          "white",
                                          NULL);
    blue_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "blue_background",
                                          "background",
                                          "blue",
                                          NULL);
#ifdef __APPLE__
    mono_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "monospace",
                                          "family",
                                          "monospace",
                                          NULL);
    size_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "fsiz",
                                          "size",
                                          10 * PANGO_SCALE,
                                          NULL);
#else
    mono_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "monospace",
                                          "family",
                                          "monospace",
                                          NULL);
    size_tag = gtk_text_buffer_create_tag(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                          "fsiz",
                                          "size",
                                          8 * PANGO_SCALE,
                                          NULL);
#endif
    *textpnt = text;

    gtk_widget_set_size_request(GTK_WIDGET(text), 100, 100);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);


    scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), text);

    gtk_widget_set_margin_start (text, 5);
    gtk_widget_set_margin_end   (text, 5);
    gtk_widget_set_margin_top   (text, 5);
    gtk_widget_set_margin_bottom (text, 5);

    controller = GTK_EVENT_CONTROLLER (gtk_gesture_click_new());
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (controller), 0);
    g_signal_connect (controller, "released", G_CALLBACK (button_release_event), text);
    gtk_event_controller_set_propagation_phase (controller, GTK_PHASE_BUBBLE);
    gtk_widget_add_controller (GTK_WIDGET (text), controller);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);

    return scrolled_window;
}

/***********************************************************************************/

static void ok_callback(GtkWidget *widget, struct logfile_context_t *ctx)
{
    (void)widget;

    bwlogbox(ctx->which->fullname, ctx->width, ctx->which, (ctx->display_mode == 0));
}

gboolean update_ctx_when_idle(gpointer textview_or_dummy)
{
    struct text_find_t *t;

    if (anno_ctx && anno_ctx->cygwin_remote_kill) {
        anno_ctx->cygwin_remote_kill = 0;
        exit(0); /* remote kill command from gtkwave */
    }

    if (textview_or_dummy == NULL) {
        if (anno_ctx == NULL) {
            return TRUE;
        }
        if (anno_ctx->marker_set != old_marker_set || old_marker != anno_ctx->marker) {
            old_marker_set = anno_ctx->marker_set;
            old_marker = anno_ctx->marker;
        } else {
            return TRUE;
        }
    }

    t = text_root;
    while (t) {
        if (textview_or_dummy) {
            if (textview_or_dummy != (gpointer)(t->text)) {
                t = t->next;
                continue;
            }
        }

        if (t->window) {
            if ((!t->ctx->display_mode) || (textview_or_dummy)) {
                GtkTextIter st_iter, en_iter;

                GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(t->text));
                gdouble vvalue = gtk_adjustment_get_value(vadj);

                read_insert_position(t);
                gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->text)),
                                               &st_iter);
                gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->text)),
                                             &en_iter);
                gtk_text_buffer_delete(gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->text)),
                                       &st_iter,
                                       &en_iter);

                gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(t->text)),
                                               &iterx);

                bold_tag = t->bold_tag;
                dgray_tag = t->dgray_tag;
                lgray_tag = t->lgray_tag;
                blue_tag = t->blue_tag;
                fwht_tag = t->fwht_tag;
                mono_tag = t->mono_tag;
                size_tag = t->size_tag;

                bwlogbox_2(t->ctx, NULL, t->button, t->text);
                set_insert_position(t);

                gtk_adjustment_set_value(vadj, vvalue);
                g_signal_emit_by_name(vadj, "changed");
                g_signal_emit_by_name(vadj, "value_changed");
            }
        }
        t = t->next;
    }

    return TRUE;
}

static void destroy_callback(GtkButton* widget)
{
    struct text_find_t *t = text_root, *tprev = NULL;
    struct logfile_context_t *ctx = NULL;
    GtkWidget *matched = NULL;

    while (t) {
        if (GTK_BUTTON(t->button) == widget) {
            matched = t->window;
            if (t == selected_text_via_tab)
                selected_text_via_tab = NULL;
            if (tprev) /* prune out struct text_find_t */
            {
                tprev->next = t->next;
                ctx = t->ctx;
                free(t);
                break;
            } else {
                text_root = t->next;
                ctx = t->ctx;
                free(t);
                break;
            }
        }

        tprev = t;
        t = t->next;
    }

    if (matched)
        gtk_notebook_detach_tab(GTK_NOTEBOOK(notebook), matched);

    if (ctx) {
        JRB node;
        JRB varnames = ctx->varnames;

        if (ctx->title)
            free(ctx->title);

        /* Avoid dereferencing null pointers. */
        if (varnames == NULL)
            return;

        /* free up variables list */
        jrb_traverse(node, varnames)
        {
            if (node->val2.v)
                free(node->val2.v);
            free(node->key.s);
        }

        jrb_traverse(node, varnames)
        {
            struct jrb_chain *jvc = node->jval_chain;
            while (jvc) {
                struct jrb_chain *jvcn = jvc->next;
                free(jvc);
                jvc = jvcn;
            }
        }

        jrb_free_tree(varnames);

        free(ctx);
    }
}

void bwlogbox(char *title, int width, ds_Tree *t, int display_mode)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox, *button1;
    GtkWidget *label, *separator;
    GtkWidget *ctext;
    GtkWidget *text;
    GtkWidget *tbox;
    GtkWidget *l1;
    GtkWidget *close_button;
    gint pagenum = 0;
    FILE *handle;
    struct logfile_context_t *ctx;
    char *default_text = t->filename;

    handle = fopen(default_text, "rb");
    if (!handle) {
        if (strcmp(default_text, "(VerilatorTop)")) {
            fprintf(stderr, "Could not open sourcefile '%s'\n", default_text);
        }
        return;
    }
    fclose(handle);

    /* nothing */

    if (!notebook) { /* Keep this just in case. Can be removed in furute version */
        fprintf(stderr,
                "Internal error: notebook should be constructed at this point. Please report this "
                "as a bug\n");
    }

    window = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    tbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    l1 = gtk_label_new(title);

    /* setup close button */
    close_button = gtk_button_new_from_icon_name(XXX_GTK_STOCK_CLOSE);
    gtk_button_set_has_frame(GTK_BUTTON(close_button), FALSE);

    /* don't allow focus on the close button */
    gtk_widget_set_focus_on_click(GTK_WIDGET(close_button), FALSE);

    gtk_box_append(GTK_BOX(tbox), l1);
    gtk_box_append(GTK_BOX(tbox), close_button);

    g_signal_connect (close_button, "clicked", G_CALLBACK (destroy_callback), close_button);
    /* this will destroy the tab by destroying the parent container */

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_paned_set_start_child(GTK_PANED(window), vbox);

    label = gtk_label_new(default_text);
    gtk_box_append(GTK_BOX(vbox), label);

    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(vbox), separator);

    ctext = create_log_text(&text);
    gtk_widget_set_vexpand(ctext, TRUE);
    gtk_box_append(GTK_BOX(vbox), ctext);

    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(vbox), separator);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);

    gtk_box_append(GTK_BOX(vbox), hbox);

    ctx = (struct logfile_context_t *)calloc(1, sizeof(struct logfile_context_t));
    ctx->which = t;
    ctx->display_mode = display_mode;
    ctx->width = width;
    ctx->title = strdup(title);

    button1 = gtk_button_new_with_label(display_mode ? "View Design Unit Only" : "View Full File");
    gtk_widget_set_hexpand(button1, TRUE);
    g_signal_connect(button1, "clicked", G_CALLBACK(ok_callback), ctx);
    gtk_box_append(GTK_BOX(hbox), button1);
    // gtk_window_set_default_widget(GTK_WINDOW(hbox),button1);

    bwlogbox_2(ctx, window, close_button, text);

    // Page must be appent to the notebook after `bwlogbox_2`
    // Since for the first page Gtk.Notebook::switch-page will be emitted
    // inside `gtk_notebook_append_page_menu` rather than `gtk_notebook_set_current_page`
    // and text_root must be set when the signal is emitted. Otherwise user can not use search
    // function in first page.
    pagenum =
        gtk_notebook_append_page_menu(GTK_NOTEBOOK(notebook), window, tbox, gtk_label_new(title));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), pagenum);

}

void bwlogbox_2(struct logfile_context_t *ctx,
                GtkWidget *window,
                GtkWidget *button,
                GtkWidget *text)
{
    ds_Tree *t = ctx->which;
    int display_mode = ctx->display_mode;
    char *title = ctx->title;

    FILE *handle;
    int lx;
    int lx_module_line = 0;
    int lx_module_line_locked = 0;
    int lx_endmodule_line_locked = 0;

    struct wave_logfile_lines_t *wlog_head = NULL, *wlog_curr = NULL;
    int wlog_size = 0;
    int line_no;
    int s_line_find = -1, e_line_find = -1;
    struct text_find_t *text_curr;

    char *default_text = t->filename;
    char *design_unit = t->item;
    int s_line = t->s_line;
    int e_line = t->e_line;

    handle = fopen(default_text, "rb");
    if (!handle) {
        if (strcmp(default_text, "(VerilatorTop)")) {
            fprintf(stderr, "Could not open sourcefile '%s'\n", default_text);
        }
        return;
    }
    fclose(handle);

    v_preproc_name = default_text;
    while ((lx = yylex())) {
        char *pnt = yytext;
        struct wave_logfile_lines_t *w;
        line_no = my_yylineno;

        if (!verilog_2005) {
            if (lx == V_KW_2005)
                lx = V_ID;
        }

        if (!lx_module_line_locked) {
            if (lx == V_MODULE) {
                lx_module_line = line_no;
            } else if ((lx == V_ID) && (lx_module_line)) {
                if (!strcmp(pnt, design_unit)) {
                    lx_module_line_locked = 1;
                    s_line_find = lx_module_line;
                }
            } else if ((lx != V_WS) && (lx_module_line)) {
                lx_module_line = 0;
            }
        } else {
            if ((lx == V_ENDMODULE) && (!lx_endmodule_line_locked)) {
                e_line_find = line_no;
                lx_endmodule_line_locked = 1;
            }
        }

        w = calloc(1, sizeof(struct wave_logfile_lines_t));
        w->line_no = line_no;
        w->tok = lx;
        wlog_size += yyleng;
        w->text = malloc(yyleng + 1);
        strcpy(w->text, pnt);
        if (!wlog_curr) {
            wlog_head = wlog_curr = w;
        } else {
            wlog_curr->next = w;
            wlog_curr = w;
        }
    }

    log_text(text, NULL, "Design unit ");
    log_text_bold(text, NULL, design_unit);
    {
        char buf[8192];

        s_line = s_line_find > 0 ? s_line_find : s_line;
        e_line = e_line_find > 0 ? e_line_find : e_line;

        sprintf(buf, " occupies lines %d - %d.\n", s_line, e_line);
        log_text(text, NULL, buf);
        if (anno_ctx) {
            sprintf(buf,
                    "Marker time for '%s' is %s.\n",
                    anno_ctx->aet_name,
                    anno_ctx->marker_set ? anno_ctx->time_string : "not set");
            log_text(text, NULL, buf);
        }

        log_text(text, NULL, "\n");
    }

    if (wlog_curr) {
        struct wave_logfile_lines_t *w = wlog_head;
        struct wave_logfile_lines_t *wt;
        char *pnt = malloc(wlog_size + 1);
        char *pnt2 = pnt;
        JRB varnames = NULL;
        JRB node;
        int numvars = 0;

        /* build up list of potential variables in this module */
        if (!display_mode && !ctx->varnames) {
            varnames = make_jrb();
            while (w) {
                if (w->tok == V_ID) {
                    if ((w->line_no >= s_line) && (w->line_no <= e_line)) {
                        if (strcmp(w->text, design_unit)) /* filter out design unit name */
                        {
                            node = jrb_find_str(varnames, w->text);

                            if (!node) {
                                Jval dummy;
                                dummy.v = NULL;
                                jrb_insert_str(varnames, strdup(w->text), dummy);
                                numvars++;
                            }
                        }
                    }
                }
                w = w->next;
            }
        }

        if ((anno_ctx) && (anno_ctx->marker_set) &&
            (!display_mode)) /* !display_mode somehow was removed earlier causing full view crashes
                              */
        {
            int resolved = 0;

            /*************************/
            if (fst) {
                int numfacs = fstReaderGetVarCount(fst);
                int i;
                const char *scp_nam = NULL;
                fstHandle fh = 0;
                int new_scope_encountered = 1;
                int good_scope = 0;

                if (ctx->varnames)
                    goto skip_resolved_fst;

                jrb_traverse(node, varnames)
                {
                    node->val.i = -1;
                }
                fstReaderIterateHierRewind(fst);
                fstReaderResetScope(fst);

                for (i = 0; i < numfacs; i++) {
                    struct fstHier *h;

                    new_scope_encountered = 0;
                    while ((h = fstReaderIterateHier(fst))) {
                        int do_brk = 0;
                        switch (h->htyp) {
                            case FST_HT_SCOPE:
                                scp_nam = fstReaderPushScope(fst, h->u.scope.name, NULL);
                                new_scope_encountered = 1;
                                break;
                            case FST_HT_UPSCOPE:
                                scp_nam = fstReaderPopScope(fst);
                                new_scope_encountered = 1;
                                break;
                            case FST_HT_VAR:
                                scp_nam = fstReaderGetCurrentFlatScope(fst);
                                if (!h->u.var.is_alias)
                                    fh++;
                                do_brk = 1;
                                break;
                            default:
                                break;
                        }
                        if (do_brk)
                            break;
                    }
                    if (!h)
                        break;

                    if (!new_scope_encountered) {
                        if (!good_scope) {
                            continue;
                        }
                    } else {
                        good_scope = !strcmp(scp_nam, title);
                    }

                    if (!good_scope) {
                        if (resolved == numvars) {
                            break;
                        }
                    } else {
                        jrb_traverse(node, varnames)
                        {
                            if (node->val.i < 0) {
                                if (!fst_alpha_strcmpeq(h->u.var.name, node->key.s)) {
                                    resolved++;
                                    if (h->u.var.is_alias) {
                                        node->val.i = h->u.var.handle;
                                    } else {
                                        node->val.i = fh;
                                    }
                                }
                            } else /* bitblasted */
                            {
                                if (!fst_alpha_strcmpeq(h->u.var.name, node->key.s)) {
                                    struct jrb_chain *jvc = node->jval_chain;
                                    if (jvc) {
                                        while (jvc->next)
                                            jvc = jvc->next;
                                        jvc->next = calloc(1, sizeof(struct jrb_chain));
                                        jvc = jvc->next;
                                    } else {
                                        jvc = calloc(1, sizeof(struct jrb_chain));
                                        node->jval_chain = jvc;
                                    }

                                    if (h->u.var.is_alias) {
                                        jvc->val.i = h->u.var.is_alias;
                                    } else {
                                        jvc->val.i = fh;
                                    }
                                }
                            }
                        }
                    }
                }
                /* resolved_fst: */
                ctx->varnames = varnames;
                ctx->resolved = resolved;
            skip_resolved_fst:
                varnames = ctx->varnames;
                resolved = ctx->resolved;

                jrb_traverse(node, varnames)
                {
                    if (node->val.i >= 0) {
                        char rcb[65537];
                        char *rc;
                        struct jrb_chain *jvc = node->jval_chain;
                        char first_char;

                        rc = fstReaderGetValueFromHandleAtTime(fst,
                                                               anno_ctx->marker,
                                                               node->val.i,
                                                               rcb);
                        first_char = rc ? rc[0] : '?';

                        if (!jvc) {
                            if (rc) {
                                node->val2.v = hexify(strdup(rc));
                            } else {
                                node->val2.v = NULL;
                            }
                        } else {
                            char *rc2;
                            int len = rc ? strlen(rc) : 0;
                            int iter = 1;

                            while (jvc) {
                                fstReaderGetValueFromHandleAtTime(fst,
                                                                  anno_ctx->marker,
                                                                  jvc->val.i,
                                                                  rc);
                                len +=
                                    (rc ? strlen(rc) : 0); /* scan-build : possible null pointer */
                                iter++;
                                jvc = jvc->next;
                            }

                            if (iter == len) {
                                int pos = 1;
                                jvc = node->jval_chain;
                                rc2 = calloc(1, len + 1);
                                rc2[0] = first_char;

                                while (jvc) {
                                    char rcv[65537];
                                    fstReaderGetValueFromHandleAtTime(fst,
                                                                      anno_ctx->marker,
                                                                      jvc->val.i,
                                                                      rcv);
                                    rc2[pos++] = *rcv;
                                    jvc = jvc->next;
                                }

                                node->val2.v = hexify(strdup(rc2));
                                free(rc2);
                            } else {
                                node->val2.v = NULL;
                            }
                        }
                    } else {
                        node->val2.v = NULL;
                    }
                }
            }
            /*************************/
            else if (vzt) {
                int numfacs = vzt_rd_get_num_facs(vzt);
                int i;
                int tlen;
                char *pfx = NULL;

                if (ctx->varnames)
                    goto skip_resolved_vzt;

                pfx = malloc((tlen = strlen(title)) + 1 + 1);
                strcpy(pfx, title);
                strcat(pfx + tlen, ".");
                tlen++;

                jrb_traverse(node, varnames)
                {
                    node->val.i = -1;
                }

                for (i = 0; i < numfacs; i++) {
                    char *fnam = vzt_rd_get_facname(vzt, i);

                    if (!strncmp(fnam, pfx, tlen)) {
                        if (!strchr(fnam + tlen, '.')) {
                            jrb_traverse(node, varnames)
                            {
                                int mat = 0;

                                if (node->val.i < 0) {
                                    if (!strcmp(fnam + tlen, node->key.s)) {
                                        mat = 1;
                                        if (vzt->flags[i] & VZT_RD_SYM_F_ALIAS) {
                                            node->val.i = vzt_rd_get_alias_root(vzt, i);
                                        } else {
                                            node->val.i = i;
                                        }
                                    }
                                } else /* bitblasted */
                                {
                                    if (!strcmp(fnam + tlen, node->key.s)) {
                                        struct jrb_chain *jvc = node->jval_chain;

                                        mat = 1;

                                        if (jvc) {
                                            while (jvc->next)
                                                jvc = jvc->next;
                                            jvc->next = calloc(1, sizeof(struct jrb_chain));
                                            jvc = jvc->next;
                                        } else {
                                            jvc = calloc(1, sizeof(struct jrb_chain));
                                            node->jval_chain = jvc;
                                        }

                                        if (vzt->flags[i] & VZT_RD_SYM_F_ALIAS) {
                                            jvc->val.i = vzt_rd_get_alias_root(vzt, i);
                                        } else {
                                            jvc->val.i = i;
                                        }
                                    }
                                }

                                if (mat) {
                                    if (i == (numfacs - 1)) {
                                        resolved++;
                                    } else {
                                        char *fnam2 = vzt_rd_get_facname(vzt, i + 1);
                                        if (strcmp(fnam, fnam2)) {
                                            resolved++;
                                            if (resolved == numvars)
                                                goto resolved_vzt;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            resolved_vzt:
                free(pfx);
                ctx->varnames = varnames;
                ctx->resolved = resolved;
            skip_resolved_vzt:
                varnames = ctx->varnames;
                resolved = ctx->resolved;

                jrb_traverse(node, varnames)
                {
                    if (node->val.i >= 0) {
                        char *rc = vzt_rd_value(vzt, anno_ctx->marker, node->val.i);
                        struct jrb_chain *jvc = node->jval_chain;
                        char first_char = rc ? rc[0] : '?';

                        if (!jvc) {
                            if (rc) {
                                node->val2.v = hexify(strdup(rc));
                            } else {
                                node->val2.v = NULL;
                            }
                        } else {
                            char *rc2;
                            int len = rc ? strlen(rc) : 0;
                            int iter = 1;

                            while (jvc) {
                                rc = vzt_rd_value(vzt, anno_ctx->marker, jvc->val.i);
                                len += (rc ? strlen(rc) : 0);
                                iter++;
                                jvc = jvc->next;
                            }

                            if (iter == len) {
                                int pos = 1;
                                jvc = node->jval_chain;
                                rc2 = calloc(1, len + 1);
                                rc2[0] = first_char;

                                while (jvc) {
                                    char *rcv = vzt_rd_value(vzt, anno_ctx->marker, jvc->val.i);
                                    rc2[pos++] = *rcv;
                                    jvc = jvc->next;
                                }

                                node->val2.v = hexify(strdup(rc2));
                                free(rc2);
                            } else {
                                node->val2.v = NULL;
                            }
                        }
                    } else {
                        node->val2.v = NULL;
                    }
                }
            }
            /******/
            else if (lx2) {
                int numfacs = lxt2_rd_get_num_facs(lx2);
                int i;
                int tlen;
                char *pfx = NULL;

                if (ctx->varnames)
                    goto skip_resolved_lxt2;

                pfx = malloc((tlen = strlen(title)) + 1 + 1);
                strcpy(pfx, title);
                strcat(pfx + tlen, ".");
                tlen++;

                lxt2_rd_clr_fac_process_mask_all(lx2);
                jrb_traverse(node, varnames)
                {
                    node->val.i = -1;
                }

                for (i = 0; i < numfacs; i++) {
                    char *fnam = lxt2_rd_get_facname(lx2, i);

                    if (!strncmp(fnam, pfx, tlen)) {
                        if (!strchr(fnam + tlen, '.')) {
                            jrb_traverse(node, varnames)
                            {
                                int mat = 0;

                                if (node->val.i < 0) {
                                    if (!strcmp(fnam + tlen, node->key.s)) {
                                        mat = 1;

                                        if (lx2->flags[i] & LXT2_RD_SYM_F_ALIAS) {
                                            node->val.i = lxt2_rd_get_alias_root(lx2, i);
                                        } else {
                                            node->val.i = i;
                                        }

                                        lxt2_rd_set_fac_process_mask(lx2, node->val.i);
                                    }
                                } else /* bitblasted */
                                {
                                    if (!strcmp(fnam + tlen, node->key.s)) {
                                        struct jrb_chain *jvc = node->jval_chain;

                                        mat = 1;

                                        if (jvc) {
                                            while (jvc->next)
                                                jvc = jvc->next;
                                            jvc->next = calloc(1, sizeof(struct jrb_chain));
                                            jvc = jvc->next;
                                        } else {
                                            jvc = calloc(1, sizeof(struct jrb_chain));
                                            node->jval_chain = jvc;
                                        }

                                        if (lx2->flags[i] & LXT2_RD_SYM_F_ALIAS) {
                                            jvc->val.i = lxt2_rd_get_alias_root(lx2, i);
                                        } else {
                                            jvc->val.i = i;
                                        }

                                        lxt2_rd_set_fac_process_mask(lx2, jvc->val.i);
                                    }
                                }

                                if (mat) {
                                    if (i == (numfacs - 1)) {
                                        resolved++;
                                    } else {
                                        char *fnam2 = lxt2_rd_get_facname(lx2, i + 1);
                                        if (strcmp(fnam, fnam2)) {
                                            resolved++;
                                            if (resolved == numvars)
                                                goto resolved_lxt2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            resolved_lxt2:
                free(pfx);
                ctx->varnames = varnames;
                ctx->resolved = resolved;
            skip_resolved_lxt2:
                varnames = ctx->varnames;
                resolved = ctx->resolved;

                lx2vals = make_jrb();
                lxt2_rd_unlimit_time_range(lx2);
                lxt2_rd_limit_time_range(lx2, anno_ctx->marker, anno_ctx->marker);
                lxt2_rd_iter_blocks(lx2, lx2_iter_fn, NULL);

                jrb_traverse(node, varnames)
                {
                    struct jrb_chain *jvc = node->jval_chain;

                    if (node->val.i >= 0) {
                        JRB srch = jrb_find_int(lx2vals, node->val.i);
                        char *rc = srch ? srch->val.s : NULL;
                        char first_char = rc ? rc[0] : '?';

                        if (!jvc) {
                            if (rc) {
                                node->val2.v = hexify(strdup(rc));
                            } else {
                                node->val2.v = NULL;
                            }
                        } else {
                            char *rc2;
                            int len = rc ? strlen(rc) : 0;
                            int iter = 1;

                            while (jvc) {
                                srch = jrb_find_int(lx2vals, jvc->val.i);
                                rc = srch ? srch->val.s : NULL;
                                len += (rc ? strlen(rc) : 0);
                                iter++;
                                jvc = jvc->next;
                            }

                            if (iter == len) {
                                int pos = 1;
                                jvc = node->jval_chain;
                                rc2 = calloc(1, len + 1);
                                rc2[0] = first_char;

                                while (jvc) {
                                    srch = jrb_find_int(lx2vals, jvc->val.i);
                                    rc = srch->val.s;
                                    rc2[pos++] = *rc;
                                    jvc = jvc->next;
                                }

                                node->val2.v = hexify(strdup(rc2));
                                free(rc2);
                            } else {
                                node->val2.v = NULL;
                            }
                        }
                    } else {
                        node->val2.v = NULL;
                    }
                }

                jrb_traverse(node, lx2vals)
                {
                    if (node->val.s)
                        free(node->val.s);
                }
                jrb_free_tree(lx2vals);
                lx2vals = NULL;
            }
            if (resolved > 0) {
                w = wlog_head;
                while (w) {
                    if ((w->line_no >= s_line) && (w->line_no <= e_line)) {
                        if (w->tok == V_ID) {
                            if (strcmp(w->text, design_unit)) /* filter out design unit name */
                            {
                                node = jrb_find_str(varnames, w->text);
                                if ((node) && (node->val2.v)) {
                                    log_text(text, fontx, w->text);
                                    log_text_bold(text, fontx, "[");
                                    log_text_bold(text, fontx, node->val2.v);
                                    log_text_bold(text, fontx, "]");
                                    goto iter_free;
                                }
                            }
                        }

                        switch (w->tok) {
                            case V_CMT:
                                log_text_active(text, fontx, w->text);
                                break;

                            case V_STRING:
                            case V_PREPROC:
                            case V_PREPROC_WS:
                            case V_MACRO:
                                log_text_prelight(text, fontx, w->text);
                                break;

                            default:
                                log_text(text, fontx, w->text);
                                break;
                        }
                    }

                iter_free:
                    free(w->text);
                    wt = w;
                    w = w->next;
                    free(wt);
                }
                free(pnt);
                /* wlog_head = */ wlog_curr = NULL;
                goto free_vars;
            }
        }

        w = wlog_head;
        while (w) {
            if ((display_mode) || ((w->line_no >= s_line) && (w->line_no <= e_line))) {
                int len = strlen(w->text);
                memcpy(pnt2, w->text, len);
                pnt2 += len;
            }

            free(w->text);
            wt = w;
            w = w->next;
            free(wt);
        }
        /* wlog_head = */ wlog_curr = NULL;
        *pnt2 = 0;
        log_text(text, fontx, pnt);
        free(pnt);

    free_vars:
#if 0
	/* free up variables list */
	if(!display_mode)
		{
		jrb_traverse(node, varnames)
			{
			if(node->val2.v) free(node->val2.v);
			free(node->key.s);
			}
		}

	jrb_traverse(node, varnames)
		{
		struct jrb_chain *jvc = node->jval_chain;
		while(jvc)
			{
			struct jrb_chain *jvcn = jvc->next;
			free(jvc);
			jvc = jvcn;
			}
		}

	jrb_free_tree(varnames);
	varnames = NULL;
#endif

        /* insert context for destroy */
        if (window) {
            text_curr = (struct text_find_t *)calloc(1, sizeof(struct text_find_t));
            text_curr->window = window;
            text_curr->button = button;
            text_curr->text = text;
            text_curr->ctx = ctx;
            text_curr->next = text_root;
            text_root = text_curr;

            text_curr->bold_tag = bold_tag;
            text_curr->dgray_tag = dgray_tag;
            text_curr->lgray_tag = lgray_tag;
            text_curr->blue_tag = blue_tag;
            text_curr->fwht_tag = fwht_tag;
            text_curr->mono_tag = mono_tag;
            text_curr->size_tag = size_tag;
        }
    }
}
