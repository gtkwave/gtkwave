/*
 * Copyright (c) Tony Bybell 1999-2013.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <config.h>
#include "globals.h"
#include <ctype.h>
#include "gtk23compat.h"
#include "strace.h"
#include "currenttime.h"
#include "hierpack.h"

#define WV_STRACE_CTX "strace_ctx"

/* need to do this every time you connect a signal */
#define WV_STRACE_CURWIN(x) \
    g_object_set_data(G_OBJECT(x), WV_STRACE_CTX, (gpointer)GLOBALS->strace_ctx)

/* need to do this at top of every entry point function where a signal is connected */
#define GET_WV_STRACE_CURWIN(x) GLOBALS->strace_ctx = g_object_get_data(G_OBJECT(x), WV_STRACE_CTX)

static const char *logical[] = {"AND", "OR", "XOR", "NAND", "NOR", "XNOR"};

static const char *stype[WAVE_STYPE_COUNT] = {"Don't Care",
                                              "High",
                                              "Z (Mid)",
                                              "X",
                                              "Low",
                                              "String",
                                              "Rising Edge",
                                              "Falling Edge",
                                              "Any Edge"};

/*
 * naive nonoptimized case insensitive strstr
 */
char *strstr_i(char *hay, char *needle)
{
    char *ht, *nt;

    while (*hay) {
        int offs = 0;
        ht = hay;
        nt = needle;
        while (*nt) {
            if (toupper((int)(unsigned char)*(ht + offs)) != toupper((int)(unsigned char)*nt))
                break;
            offs++;
            nt++;
        }

        if (!*nt)
            return (hay);
        hay++;
    }

    return *hay ? hay : NULL;
}

/*
 * trap timescale overflows
 */
GwTime strace_adjust(GwTime a, GwTime b)
{
    GwTime res = a + b;

    if (b > GW_TIME_CONSTANT(0) && res < a) {
        return MAX_HISTENT_TIME;
    }

    return res;
}

/*
 * free the straces...
 */
static int count_active_straces(void)
{
    int s_ctx_iter;
    int activ = 0;

    WAVE_STRACE_ITERATOR_FWD(s_ctx_iter)
    {
        struct strace_ctx_t *strace_ctx = &GLOBALS->strace_windows[s_ctx_iter];

        if (strace_ctx->window_strace_c_10)
            activ++;
    }

    return activ;
}

/*
 * free the straces...
 */
static void free_straces(void)
{
    struct strace *s, *skill;
    struct strace_defer_free *sd, *sd2;

    s = GLOBALS->strace_ctx->straces;

    while (s) {
        if (s->string) {
            free_2(s->string);
        }
        skill = s;
        s = s->next;
        free_2(skill);
    }

    GLOBALS->strace_ctx->straces = NULL;

    /* only free up traces if there is only one pattern search active. */
    /* we could splice across multiple strace_ctx but it's not worth the effort here */
    if (count_active_straces() <= 1) {
        sd = GLOBALS->strace_ctx->strace_defer_free_head;

        while (sd) {
            FreeTrace(sd->defer);
            sd2 = sd->next;
            free_2(sd);
            sd = sd2;
        }

        /* moved inside if() so it frees eventually and doesn't stay around until context cleanup */
        GLOBALS->strace_ctx->strace_defer_free_head = NULL;
    }
}

/*
 * button/menu/entry activations..
 */
static void logical_clicked(GtkComboBox *widget, gpointer user_data)
{
    (void)user_data;

    GtkComboBox *combo_box = widget;
    int which = gtk_combo_box_get_active(combo_box);
    int i;

    GET_WV_STRACE_CURWIN(widget);

    for (i = 0; i < 6; i++) {
        GLOBALS->strace_ctx->logical_mutex[i] = 0;
    }
    GLOBALS->strace_ctx->logical_mutex[which] = 1; /* mark our choice */
}

static void stype_clicked(GtkComboBox *widget, gpointer user_data)
{
    GtkComboBox *combo_box = widget;
    int which = gtk_combo_box_get_active(combo_box);
    struct strace *s = (struct strace *)user_data;
    GET_WV_STRACE_CURWIN(widget);

    s->value = which;

    DEBUG(printf("Trace %s Search Type: %s\n", s->trace->name, stype[(int)s->value]));
}

static void start_clicked(GtkComboBox *widget, gpointer user_data)
{
    (void)user_data;

    GtkComboBox *combo_box = widget;
    int which = gtk_combo_box_get_active(combo_box);

    GET_WV_STRACE_CURWIN(widget);

    GLOBALS->strace_ctx->mark_idx_start = which;
}

static void end_clicked(GtkComboBox *widget, gpointer user_data)
{
    (void)user_data;

    GtkComboBox *combo_box = widget;
    int which = gtk_combo_box_get_active(combo_box);

    GET_WV_STRACE_CURWIN(widget);

    GLOBALS->strace_ctx->mark_idx_end = which;
}

static void enter_callback(GtkWidget *widget, gpointer strace_tmp)
{
    const gchar *entry_text;
    struct strace *s;
    int i, len;

    GET_WV_STRACE_CURWIN(widget);

    s = (struct strace *)strace_tmp;
    if (s->string) {
        free_2(s->string);
        s->string = NULL;
    }

    entry_text = gtk_entry_get_text(GTK_ENTRY(widget));
    entry_text = entry_text ? entry_text : "";
    DEBUG(printf("Trace %s Entry contents: %s\n", s->trace->name, entry_text));

    len = strlen(entry_text);
    if (len == 0) {
        return;
    }

    s->string = malloc_2(len + 1);
    strcpy(s->string, entry_text);
    for (i = 0; i < len; i++) {
        char ch;
        ch = s->string[i];
        if (ch >= 'a' && ch <= 'z') {
            s->string[i] = ch - ('a' - 'A');
        }
    }
}

static void forwards_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    GET_WV_STRACE_CURWIN(widget);

    /* no cleanup necessary, but do real search */
    DEBUG(printf("Searching Forward..\n"));
    strace_search(STRACE_FORWARD);
}

static void backwards_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    GET_WV_STRACE_CURWIN(widget);

    /* no cleanup necessary, but do real search */
    DEBUG(printf("Searching Backward..\n"));
    strace_search(STRACE_BACKWARD);
}

static void mark_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    GET_WV_STRACE_CURWIN(widget);

    DEBUG(printf("Marking..\n"));
    if (GLOBALS->strace_ctx->shadow_straces) {
        delete_strace_context();
    }

    strace_maketimetrace(1);
    cache_actual_pattern_mark_traces();

    redraw_signals_and_waves();
}

static void clear_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    GET_WV_STRACE_CURWIN(widget);

    DEBUG(printf("Clearing..\n"));
    if (GLOBALS->strace_ctx->shadow_straces) {
        delete_strace_context();
    }
    strace_maketimetrace(0);

    redraw_signals_and_waves();
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    GET_WV_STRACE_CURWIN(widget);

    free_straces();
    GLOBALS->strace_ctx->ptr_mark_count_label_strace_c_1 = NULL;
    gtk_widget_destroy(GLOBALS->strace_ctx->window_strace_c_10);
    GLOBALS->strace_ctx->window_strace_c_10 = NULL;
}

/* update mark count label on pattern search dialog */

static void update_mark_count_label(void)
{
    char mark_count_buf[64];

    if (GLOBALS->strace_ctx->ptr_mark_count_label_strace_c_1 == NULL) {
        return;
    }

    sprintf(mark_count_buf, "%d", GLOBALS->strace_ctx->timearray_size);
    gtk_label_set_text(GTK_LABEL(GLOBALS->strace_ctx->ptr_mark_count_label_strace_c_1),
                       mark_count_buf);
}

static GtkWidget *build_marker_combo_box(gboolean is_end)
{
    char c;
    GtkWidget *combo_box;

    combo_box = gtk_combo_box_text_new();
    if (is_end) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "End of Time");
    } else {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), "Start of Time");
    }

    for (c = 'A'; c <= 'Z'; c++) {
        gchar *text = g_strdup_printf("Named Marker %c", c);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), text);
        g_free(text);
    }

    return combo_box;
}

void tracesearchbox(const char *title, GCallback func, gpointer data)
{
    GtkWidget *entry;
    GtkWidget *vbox, *hbox, *small_hbox, *vbox_g, *label;
    GtkWidget *button1, *button1a, *button1b, *button1c, *button2, *scrolled_win, *frame,
        *separator;
    GtkSizeGroup *label_group;
    GwTrace *t;
    int i;
    int numtraces;

    GLOBALS->strace_current_window = (int)(intptr_t)data; /* arg for which search box going in */
    GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window];

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    if (GLOBALS->strace_ctx->straces) {
        gdk_window_raise(gtk_widget_get_window(GLOBALS->strace_ctx->window_strace_c_10));
        return; /* is already active */
    }

    GLOBALS->strace_ctx->cleanup_strace_c_7 = func;
    numtraces = 0;

    /* create a new window */
    GLOBALS->strace_ctx->window_strace_c_10 =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);

    GLOBALS->strace_windows[GLOBALS->strace_current_window].window_strace_c_10 =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(
        GLOBALS->strace_windows[GLOBALS->strace_current_window].window_strace_c_10,
        ((char *)&GLOBALS->strace_windows[GLOBALS->strace_current_window].window_strace_c_10) -
            ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW(GLOBALS->strace_ctx->window_strace_c_10), title);
    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->strace_ctx->window_strace_c_10), 420, -1);
    gtkwave_signal_connect(GLOBALS->strace_ctx->window_strace_c_10,
                           "delete_event",
                           (GCallback)destroy_callback,
                           NULL);
    WV_STRACE_CURWIN(GLOBALS->strace_ctx->window_strace_c_10);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(GLOBALS->strace_ctx->window_strace_c_10), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(GLOBALS->strace_ctx->window_strace_c_10), 12);
    gtk_widget_show(vbox);

    vbox_g = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(vbox_g);

    frame = gtk_frame_new(NULL);
    gtk_widget_show(frame);

    small_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(small_hbox);

    label = gtk_label_new("Logical Operation");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(small_hbox), label, TRUE, TRUE, 0);

    GtkWidget *combo_box = gtk_combo_box_text_new();

    for (i = 0; i < 6; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), logical[i]);
        GLOBALS->strace_ctx->logical_mutex[i] = 0;
    }

    GLOBALS->strace_ctx->logical_mutex[0] = 1; /* "and" */
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);

    WV_STRACE_CURWIN(combo_box);
    g_signal_connect(combo_box, "changed", G_CALLBACK(logical_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(small_hbox), combo_box, FALSE, FALSE, 0);
    gtk_widget_show(combo_box);

    gtk_box_pack_start(GTK_BOX(vbox), small_hbox, FALSE, FALSE, 0);

    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), -1, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_win);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    for (t = GLOBALS->traces.first; t; t = t->t_next) {
        struct strace *s;

        if ((t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) || (!(t->flags & TR_HIGHLIGHT)) ||
            (!(t->name))) {
            continue;
        }

        if (numtraces > 0) {
            separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
            gtk_widget_show(separator);
            gtk_box_pack_start(GTK_BOX(vbox_g), separator, FALSE, FALSE, 0);
        }

        numtraces++;
        if (numtraces == 500) {
            status_text("Limiting waveform display search to 500 traces.\n");
            break;
        }

        s = (struct strace *)calloc_2(1, sizeof(struct strace));
        s->next = GLOBALS->strace_ctx->straces;
        GLOBALS->strace_ctx->straces = s;
        s->trace = t;

        small_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_show(small_hbox);
        gtk_container_set_border_width(GTK_CONTAINER(small_hbox), 6);

        label = gtk_label_new(t->name);
        gtk_widget_show(label);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_widget_set_margin_top(label, 6);
        gtk_widget_set_margin_start(label, 6);
        gtk_widget_set_margin_end(label, 6);
        gtk_box_pack_start(GTK_BOX(vbox_g), label, FALSE, FALSE, 0);

        combo_box = gtk_combo_box_text_new();
        for (i = 0; i < WAVE_STYPE_COUNT; i++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), stype[i]);
        }

        WV_STRACE_CURWIN(combo_box);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
        g_signal_connect(combo_box, "changed", G_CALLBACK(stype_clicked), s);
        gtk_widget_show(combo_box);
        gtk_box_pack_start(GTK_BOX(small_hbox), combo_box, FALSE, TRUE, 0);

        entry = X_gtk_entry_new_with_max_length(257); /* %+256ch */
        gtkwave_signal_connect(entry, "activate", G_CALLBACK(enter_callback), s);
        gtkwave_signal_connect(entry, "changed", G_CALLBACK(enter_callback), s);
        WV_STRACE_CURWIN(entry);

        gtk_box_pack_start(GTK_BOX(small_hbox), entry, TRUE, TRUE, 0);
        gtk_widget_show(entry);
        gtk_box_pack_start(GTK_BOX(vbox_g), small_hbox, FALSE, FALSE, 0);
    }

    gtk_container_add(GTK_CONTAINER(scrolled_win), vbox_g); /* removes gtk3 deprecated warning */

    label_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    do /* add GUI elements for displaying mark count and mark count start/end */
    {
        GtkWidget *time_range_hbox;

        time_range_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

        label = gtk_label_new("Time range");
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(time_range_hbox), label, TRUE, TRUE, 0);
        gtk_size_group_add_widget(label_group, label);

        combo_box = build_marker_combo_box(FALSE);
        WV_STRACE_CURWIN(combo_box);
        g_signal_connect(combo_box, "changed", G_CALLBACK(start_clicked), NULL);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), GLOBALS->strace_ctx->mark_idx_start);
        gtk_box_pack_start(GTK_BOX(time_range_hbox), combo_box, FALSE, FALSE, 0);

        combo_box = build_marker_combo_box(TRUE);
        WV_STRACE_CURWIN(combo_box);
        g_signal_connect(combo_box, "changed", G_CALLBACK(end_clicked), NULL);
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), GLOBALS->strace_ctx->mark_idx_end);
        gtk_box_pack_start(GTK_BOX(time_range_hbox), combo_box, FALSE, FALSE, 0);

        gtk_widget_show_all(time_range_hbox);

        gtk_box_pack_start(GTK_BOX(vbox), time_range_hbox, FALSE, FALSE, 0);
    } while (0);

    /* add mark count GUI element */
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    label = gtk_label_new("Beg/End Marks");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_size_group_add_widget(label_group, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    GLOBALS->strace_ctx->ptr_mark_count_label_strace_c_1 = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(GLOBALS->strace_ctx->ptr_mark_count_label_strace_c_1), 0.0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       GLOBALS->strace_ctx->ptr_mark_count_label_strace_c_1,
                       TRUE,
                       TRUE,
                       0);
    update_mark_count_label();

    gtk_widget_show_all(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    button1 = gtk_button_new_with_label("Fwd");
    gtk_widget_set_size_request(button1, 75, -1);
    gtkwave_signal_connect(button1, "clicked", G_CALLBACK(forwards_callback), NULL);
    WV_STRACE_CURWIN(button1);
    gtk_widget_show(button1);
    gtk_box_pack_start(GTK_BOX(hbox), button1, TRUE, TRUE, 0);
    gtk_widget_set_can_default(button1, TRUE);
    gtkwave_signal_connect_object(button1, "realize", (GCallback)gtk_widget_grab_default, button1);

    button1a = gtk_button_new_with_label("Bkwd");
    gtk_widget_set_size_request(button1a, 75, -1);
    gtkwave_signal_connect(button1a, "clicked", G_CALLBACK(backwards_callback), NULL);
    WV_STRACE_CURWIN(button1a);
    gtk_widget_show(button1a);
    gtk_box_pack_start(GTK_BOX(hbox), button1a, TRUE, TRUE, 0);
    gtk_widget_set_can_default(button1a, TRUE);

    button1b = gtk_button_new_with_label("Mark");
    gtk_widget_set_size_request(button1b, 75, -1);
    gtkwave_signal_connect(button1b, "clicked", G_CALLBACK(mark_callback), NULL);
    WV_STRACE_CURWIN(button1b);
    gtk_widget_show(button1b);
    gtk_box_pack_start(GTK_BOX(hbox), button1b, TRUE, TRUE, 0);
    gtk_widget_set_can_default(button1b, TRUE);

    button1c = gtk_button_new_with_label("Clear");
    gtk_widget_set_size_request(button1c, 75, -1);
    gtkwave_signal_connect(button1c, "clicked", G_CALLBACK(clear_callback), NULL);
    WV_STRACE_CURWIN(button1c);
    gtk_widget_show(button1c);
    gtk_box_pack_start(GTK_BOX(hbox), button1c, TRUE, TRUE, 0);
    gtk_widget_set_can_default(button1c, TRUE);

    button2 = gtk_button_new_with_label("Exit");
    gtk_widget_set_size_request(button2, 75, -1);
    gtkwave_signal_connect(button2, "clicked", G_CALLBACK(destroy_callback), NULL);
    WV_STRACE_CURWIN(button2);
    gtk_widget_show(button2);
    gtk_box_pack_start(GTK_BOX(hbox), button2, TRUE, TRUE, 0);
    gtk_widget_set_can_default(button2, TRUE);

    gtk_widget_show(GLOBALS->strace_ctx->window_strace_c_10);
}

/*
 * strace backward or forward..
 */
static void strace_search_2(int direction, int is_last_iteration)
{
    struct strace *s;
    GwTime basetime, maxbase, sttim, fintim;
    GwTrace *t;
    int totaltraces, passcount;
    int whichpass;
    GwTime middle = 0, width;

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (gw_marker_is_enabled(primary_marker)) {
        basetime = gw_marker_get_position(primary_marker);
    } else {
        if (direction == STRACE_BACKWARD) {
            basetime = MAX_HISTENT_TIME;
        } else {
            basetime = GLOBALS->tims.first;
        }
    }

    sttim = GLOBALS->tims.first;
    fintim = GLOBALS->tims.last;

    for (whichpass = 0;; whichpass++) {
        if (direction == STRACE_BACKWARD) /* backwards */
        {
            maxbase = -1;
            s = GLOBALS->strace_ctx->straces;
            while (s) {
                t = s->trace;
                GLOBALS->shift_timebase = t->shift;
                if (!(t->vector)) {
                    GwHistEnt *h;
                    GwHistEnt **hp;
                    GwUTime utt;
                    GwTime tt;

                    /* h= */ bsearch_node(t->n.nd, basetime - t->shift); /* scan-build */
                    hp = GLOBALS->max_compare_index;
                    if ((hp == &(t->n.nd->harray[1])) || (hp == &(t->n.nd->harray[0])))
                        return;
                    if (basetime == ((*hp)->time + GLOBALS->shift_timebase))
                        hp--;
                    h = *hp;
                    s->his.h = h;
                    utt = strace_adjust(h->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt > maxbase)
                        maxbase = tt;
                } else {
                    GwVectorEnt *v;
                    GwVectorEnt **vp;
                    GwUTime utt;
                    GwTime tt;

                    /* v= */ bsearch_vector(t->n.vec, basetime - t->shift); /* scan-build */
                    vp = GLOBALS->vmax_compare_index;
                    if ((vp == &(t->n.vec->vectors[1])) || (vp == &(t->n.vec->vectors[0])))
                        return;
                    if (basetime == ((*vp)->time + GLOBALS->shift_timebase))
                        vp--;
                    v = *vp;
                    s->his.v = v;
                    utt = strace_adjust(v->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt > maxbase)
                        maxbase = tt;
                }

                s = s->next;
            }
        } else /* go forward */
        {
            maxbase = MAX_HISTENT_TIME;
            s = GLOBALS->strace_ctx->straces;
            while (s) {
                t = s->trace;
                GLOBALS->shift_timebase = t->shift;
                if (!(t->vector)) {
                    GwHistEnt *h;
                    GwUTime utt;
                    GwTime tt;

                    h = bsearch_node(t->n.nd, basetime - t->shift);
                    while (h->next && h->time == h->next->time)
                        h = h->next;
                    if ((whichpass) || gw_marker_is_enabled(primary_marker))
                        h = h->next;
                    if (!h)
                        return;
                    s->his.h = h;
                    utt = strace_adjust(h->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt < maxbase)
                        maxbase = tt;
                } else {
                    GwVectorEnt *v;
                    GwUTime utt;
                    GwTime tt;

                    v = bsearch_vector(t->n.vec, basetime - t->shift);
                    while (v->next && v->time == v->next->time)
                        v = v->next;
                    if ((whichpass) || gw_marker_is_enabled(primary_marker))
                        v = v->next;
                    if (!v)
                        return;
                    s->his.v = v;
                    utt = strace_adjust(v->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt < maxbase)
                        maxbase = tt;
                }

                s = s->next;
            }
        }

        s = GLOBALS->strace_ctx->straces;
        totaltraces = 0; /* increment when not don't care */
        while (s) {
            char str[2];
            t = s->trace;
            s->search_result = 0; /* explicitly must set this */
            GLOBALS->shift_timebase = t->shift;

            if ((!t->vector) && (!(t->n.nd->extvals))) {
                if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) != maxbase) {
                    s->his.h = bsearch_node(t->n.nd, maxbase - t->shift);
                    while (s->his.h->next && s->his.h->time == s->his.h->next->time)
                        s->his.h = s->his.h->next;
                }
                if (t->flags & TR_INVERT) {
                    str[0] = AN_STR_INV[s->his.h->v.h_val];
                } else {
                    str[0] = AN_STR[s->his.h->v.h_val];
                }
                str[1] = 0x00;

                switch (s->value) {
                    case ST_DC:
                        break;

                    case ST_HIGH:
                        totaltraces++;
                        if (str[0] == '1' || str[0] == 'h' || str[0] == 'H') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_RISE:
                        totaltraces++;
                        if (str[0] == '1' || str[0] == 'h' || str[0] == 'H') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_LOW:
                        totaltraces++;
                        if (str[0] == '0' || str[0] == 'l' || str[0] == 'L') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_FALL:
                        totaltraces++;
                        if (str[0] == '0' || str[0] == 'l' || str[0] == 'L') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_MID:
                        totaltraces++;
                        if (str[0] == 'z' || str[0] == 'Z') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_X:
                        totaltraces++;
                        if (str[0] == 'x' || str[0] == 'X') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_ANY:
                        totaltraces++;
                        s->search_result = 1;
                        break;

                    case ST_STRING:
                        totaltraces++;
                        if (s->string != NULL && strstr_i(s->string, str) != NULL) {
                            s->search_result = 1;
                        }
                        break;

                    default:
                        fprintf(stderr, "Internal error: st_type of %d\n", s->value);
                        exit(255);
                }

            } else {
                char *chval, *chval2;
                char ch;

                if (t->vector) {
                    if (strace_adjust(s->his.v->time, GLOBALS->shift_timebase) != maxbase) {
                        s->his.v = bsearch_vector(t->n.vec, maxbase - t->shift);
                        while (s->his.v->next && s->his.v->time == s->his.v->next->time) {
                            s->his.v = s->his.v->next;
                        }
                    }
                    chval = convert_ascii(t, s->his.v);
                } else {
                    if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) != maxbase) {
                        s->his.h = bsearch_node(t->n.nd, maxbase - t->shift);
                        while (s->his.h->next && s->his.h->time == s->his.h->next->time) {
                            s->his.h = s->his.h->next;
                        }
                    }
                    if (s->his.h->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(s->his.h->flags & GW_HIST_ENT_FLAG_STRING)) {
                            chval = convert_ascii_real(t, &s->his.h->v.h_double);
                        } else {
                            chval = convert_ascii_string((char *)s->his.h->v.h_vector);
                            chval2 = chval;
                            while ((ch = *chval2)) { /* toupper() the string */
                                if ((ch >= 'a') && (ch <= 'z')) {
                                    *chval2 = ch - ('a' - 'A');
                                }
                                chval2++;
                            }
                        }
                    } else {
                        chval = convert_ascii_vec(t, s->his.h->v.h_vector);
                    }
                }

                switch (s->value) {
                    case ST_DC:
                        break;

                    case ST_RISE:
                    case ST_FALL:
                        totaltraces++;
                        break;

                    case ST_HIGH:
                        totaltraces++;
                        if ((chval2 = chval))
                            while ((ch = *(chval2++))) {
                                if (((ch >= '1') && (ch <= '9')) || (ch == 'h') || (ch == 'H') ||
                                    ((ch >= 'A') && (ch <= 'F'))) {
                                    s->search_result = 1;
                                    break;
                                }
                            }
                        break;

                    case ST_LOW:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            s->search_result = 1;
                            while ((ch = *(chval2++))) {
                                if ((ch != '0') && (ch != 'l') && (ch != 'L')) {
                                    s->search_result = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_MID:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            s->search_result = 1;
                            while ((ch = *(chval2++))) {
                                if ((ch != 'z') && (ch != 'Z')) {
                                    s->search_result = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_X:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            s->search_result = 1;
                            while ((ch = *(chval2++))) {
                                if ((ch != 'x') && (ch != 'w') && (ch != 'X') && (ch != 'W')) {
                                    s->search_result = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_ANY:
                        totaltraces++;
                        s->search_result = 1;
                        break;

                    case ST_STRING:
                        totaltraces++;
                        if (s->string)
                            if (strstr_i(chval, s->string))
                                s->search_result = 1;
                        break;

                    default:
                        fprintf(stderr, "Internal error: st_type of %d\n", s->value);
                        exit(255);
                }

                free_2(chval);
            }
            s = s->next;
        }

        if ((maxbase < sttim) || (maxbase > fintim))
            return;

        DEBUG(printf("Maxbase: %" GW_TIME_FORMAT ", total traces: %d\n", maxbase, totaltraces));
        s = GLOBALS->strace_ctx->straces;
        passcount = 0;
        while (s) {
            DEBUG(printf("\tPass: %d, Name: %s\n", s->search_result, s->trace->name));
            if (s->search_result)
                passcount++;
            s = s->next;
        }

        if (totaltraces) {
            if (GLOBALS->strace_ctx->logical_mutex[0]) { /* and */
                if (totaltraces == passcount)
                    break;
            } else if (GLOBALS->strace_ctx->logical_mutex[1]) { /* or */
                if (passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[2]) { /* xor */
                if (passcount & 1) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[3]) { /* nand */
                if (totaltraces != passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[4]) { /* nor */
                if (!passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[5]) { /* xnor */
                if (!(passcount & 1)) {
                    break;
                }
            }
        }

        basetime = maxbase;
    }

    // TODO: don't use sentinel values for disabled values
    gw_marker_set_position(primary_marker, maxbase);
    gw_marker_set_enabled(primary_marker, maxbase >= 0);

    if (is_last_iteration) {
        update_time_box();

        width = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
        if ((maxbase < GLOBALS->tims.start) || (maxbase >= GLOBALS->tims.start + width)) {
            if ((maxbase < 0) || (maxbase < GLOBALS->tims.first) ||
                (maxbase > GLOBALS->tims.last)) {
                if (GLOBALS->tims.end > GLOBALS->tims.last)
                    GLOBALS->tims.end = GLOBALS->tims.last;
                middle = (GLOBALS->tims.start / 2) + (GLOBALS->tims.end / 2);
                if ((GLOBALS->tims.start & 1) && (GLOBALS->tims.end & 1)) {
                    middle++;
                }
            } else {
                middle = maxbase;
            }

            GLOBALS->tims.start = time_trunc(middle - (width / 2));
            if (GLOBALS->tims.start + width > GLOBALS->tims.last) {
                GLOBALS->tims.start = GLOBALS->tims.last - width;
            }
            if (GLOBALS->tims.start < GLOBALS->tims.first) {
                GLOBALS->tims.start = GLOBALS->tims.first;
            }
            gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                     GLOBALS->tims.timecache = GLOBALS->tims.start);
        }

        redraw_signals_and_waves();
    }
}

void strace_search(int direction)
{
    int i;
    int i_high_cnt = ((GLOBALS->strace_repeat_count > 0) ? GLOBALS->strace_repeat_count : 1) - 1;

    for (i = 0; i <= i_high_cnt; i++) {
        strace_search_2(direction, (i == i_high_cnt));
    }
}

/*********************************************/

/*
 * strace forward to make the timetrace
 */
GwTime strace_timetrace(GwTime basetime, int notfirst)
{
    struct strace *s;
    GwTime maxbase, fintim;
    GwTrace *t;
    int totaltraces, passcount;
    int whichpass;

    fintim = GLOBALS->tims.last;

    for (whichpass = 0;; whichpass++) {
        maxbase = MAX_HISTENT_TIME;
        s = GLOBALS->strace_ctx->straces;
        while (s) {
            t = s->trace;
            GLOBALS->shift_timebase = t->shift;
            if (!(t->vector)) {
                GwHistEnt *h;
                GwUTime utt;
                GwTime tt;

                h = bsearch_node(t->n.nd, basetime - t->shift);
                s->his.h = h;
                while (h->time == h->next->time) {
                    h = h->next;
                }
                if ((whichpass) || (notfirst)) {
                    h = h->next;
                }
                if (h == NULL) {
                    return MAX_HISTENT_TIME;
                }
                utt = strace_adjust(h->time, GLOBALS->shift_timebase);
                tt = utt;
                if (tt < maxbase)
                    maxbase = tt;
            } else {
                GwVectorEnt *v;
                GwUTime utt;
                GwTime tt;

                v = bsearch_vector(t->n.vec, basetime - t->shift);
                if ((whichpass) || (notfirst)) {
                    v = v->next;
                }
                if (v == NULL) {
                    return MAX_HISTENT_TIME;
                }
                s->his.v = v;
                utt = strace_adjust(v->time, GLOBALS->shift_timebase);
                tt = utt;
                if (tt < maxbase)
                    maxbase = tt;
            }

            s = s->next;
        }

        s = GLOBALS->strace_ctx->straces;
        totaltraces = 0; /* increment when not don't care */
        while (s) {
            char str[2];
            t = s->trace;
            s->search_result = 0; /* explicitly must set this */
            GLOBALS->shift_timebase = t->shift;

            if ((!t->vector) && (!(t->n.nd->extvals))) {
                if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) != maxbase) {
                    s->his.h = bsearch_node(t->n.nd, maxbase - t->shift);
                    while (s->his.h->next && s->his.h->time == s->his.h->next->time) {
                        s->his.h = s->his.h->next;
                    }
                }
                if (t->flags & TR_INVERT) {
                    str[0] = AN_STR_INV[s->his.h->v.h_val];
                } else {
                    str[0] = AN_STR[s->his.h->v.h_val];
                }
                str[1] = 0x00;

                switch (s->value) {
                    case ST_DC:
                        break;

                    case ST_HIGH:
                        totaltraces++;
                        if (str[0] == '1' || str[0] == 'h' || str[0] == 'H') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_RISE:
                        totaltraces++;
                        if ((str[0] == '1' || str[0] == 'h' || str[0] == 'H') &&
                            strace_adjust(s->his.h->time, GLOBALS->shift_timebase) == maxbase) {
                            s->search_result = 1;
                        }
                        break;

                    case ST_LOW:
                        totaltraces++;
                        if (str[0] == '0' || str[0] == 'l' || str[0] == 'L') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_FALL:
                        totaltraces++;
                        if ((str[0] == '0' || str[0] == 'l' || str[0] == 'L') &&
                            strace_adjust(s->his.h->time, GLOBALS->shift_timebase) == maxbase) {
                            s->search_result = 1;
                        }
                        break;

                    case ST_MID:
                        totaltraces++;
                        if ((str[0] == 'z') || (str[0] == 'Z'))
                            s->search_result = 1;
                        break;

                    case ST_X:
                        totaltraces++;
                        if (str[0] == 'x' || str[0] == 'X') {
                            s->search_result = 1;
                        }
                        break;

                    case ST_ANY:
                        totaltraces++;
                        if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) == maxbase) {
                            s->search_result = 1;
                        }
                        break;

                    case ST_STRING:
                        totaltraces++;
                        if (s->string != NULL && strstr_i(s->string, str) != NULL) {
                            s->search_result = 1;
                        }
                        break;

                    default:
                        fprintf(stderr, "Internal error: st_type of %d\n", s->value);
                        exit(255);
                }

            } else {
                char *chval, *chval2;
                char ch;

                if (t->vector) {
                    if (strace_adjust(s->his.v->time, GLOBALS->shift_timebase) != maxbase) {
                        s->his.v = bsearch_vector(t->n.vec, maxbase - t->shift);
                        while (s->his.v->next && s->his.v->time == s->his.v->next->time) {
                            s->his.v = s->his.v->next;
                        }
                    }
                    chval = convert_ascii(t, s->his.v);
                } else {
                    if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) != maxbase) {
                        s->his.h = bsearch_node(t->n.nd, maxbase - t->shift);
                        while (s->his.h->next && s->his.h->time == s->his.h->next->time) {
                            s->his.h = s->his.h->next;
                        }
                    }
                    if (s->his.h->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(s->his.h->flags & GW_HIST_ENT_FLAG_STRING)) {
                            chval = convert_ascii_real(t, &s->his.h->v.h_double);
                        } else {
                            chval = convert_ascii_string((char *)s->his.h->v.h_vector);
                            chval2 = chval;
                            while ((ch = *chval2)) { /* toupper() the string */
                                if ((ch >= 'a') && (ch <= 'z')) {
                                    *chval2 = ch - ('a' - 'A');
                                }
                                chval2++;
                            }
                        }
                    } else {
                        chval = convert_ascii_vec(t, s->his.h->v.h_vector);
                    }
                }

                switch (s->value) {
                    case ST_DC:
                        break;

                    case ST_RISE:
                    case ST_FALL:
                        totaltraces++;
                        break;

                    case ST_HIGH:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            while ((ch = *(chval2++))) {
                                if ((ch >= '1' && ch <= '9') || ch == 'h' || ch == 'H' ||
                                    (ch >= 'A' && ch <= 'F')) {
                                    s->search_result = 1;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_LOW:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            s->search_result = 1;
                            while ((ch = *(chval2++))) {
                                if (ch != '0' && ch != 'l' && ch != 'L') {
                                    s->search_result = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_MID:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            s->search_result = 1;
                            while ((ch = *(chval2++))) {
                                if (ch != 'z' && ch != 'Z') {
                                    s->search_result = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_X:
                        totaltraces++;
                        if ((chval2 = chval)) {
                            s->search_result = 1;
                            while ((ch = *(chval2++))) {
                                if (ch != 'x' && ch != 'w' && ch != 'X' && ch != 'W') {
                                    s->search_result = 0;
                                    break;
                                }
                            }
                        }
                        break;

                    case ST_ANY:
                        totaltraces++;
                        if (strace_adjust(s->his.v->time, GLOBALS->shift_timebase) == maxbase) {
                            s->search_result = 1;
                        }
                        break;

                    case ST_STRING:
                        totaltraces++;
                        if (s->string != NULL && strstr_i(chval, s->string) != NULL) {
                            s->search_result = 1;
                        }
                        break;

                    default:
                        fprintf(stderr, "Internal error: st_type of %d\n", s->value);
                        exit(255);
                }

                free_2(chval);
            }
            s = s->next;
        }

        if (maxbase > fintim)
            return (MAX_HISTENT_TIME);

        DEBUG(printf("Maxbase: %" GW_TIME_FORMAT ", total traces: %d\n", maxbase, totaltraces));
        s = GLOBALS->strace_ctx->straces;
        passcount = 0;
        while (s) {
            DEBUG(printf("\tPass: %d, Name: %s\n", s->search_result, s->trace->name));
            if (s->search_result) {
                passcount++;
            }
            s = s->next;
        }

        if (totaltraces) {
            if (GLOBALS->strace_ctx->logical_mutex[0]) { /* and */
                if (totaltraces == passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[1]) { /* or */
                if (passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[2]) { /* xor */
                if (passcount & 1) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[3]) { /* nand */
                if (totaltraces != passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[4]) { /* nor */
                if (!passcount) {
                    break;
                }
            } else if (GLOBALS->strace_ctx->logical_mutex[5]) { /* xnor */
                if (!(passcount & 1)) {
                    break;
                }
            }
        }

        basetime = maxbase;
    }

    return (maxbase);
}

void strace_maketimetrace(int mode)
{
    GwTime basetime = GLOBALS->tims.first;
    GwTime endtime = MAX_HISTENT_TIME;
    int notfirst = 0;
    GwTime *t;
    int t_allocated;
    GwTime orig_basetime;

    if (GLOBALS->strace_ctx->timearray) {
        free_2(GLOBALS->strace_ctx->timearray);
        GLOBALS->strace_ctx->timearray = NULL;
    }

    GLOBALS->strace_ctx->timearray_size = 0;

    if ((!mode) && (!GLOBALS->strace_ctx->shadow_active)) {
        update_mark_count_label();
        delete_mprintf();
        return; /* merely free stuff up */
    }

    GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);

    if (GLOBALS->strace_ctx->mark_idx_start > 0) {
        GwMarker *start_marker =
            gw_named_markers_get(markers, GLOBALS->strace_ctx->mark_idx_start - 1);
        if (gw_marker_is_enabled(start_marker)) {
            basetime = gw_marker_get_position(start_marker);
        } else {
            gchar *text =
                g_strdup_printf("Named Marker %s not in use.\n", gw_marker_get_name(start_marker));
            status_text(text);
            g_free(text);
        }
    }

    if (GLOBALS->strace_ctx->mark_idx_end > 0) {
        GwMarker *end_marker = gw_named_markers_get(markers, GLOBALS->strace_ctx->mark_idx_end - 1);
        if (gw_marker_is_enabled(end_marker)) {
            endtime = gw_marker_get_position(end_marker);
        } else {
            gchar *text =
                g_strdup_printf("Named Marker %s not in use.\n", gw_marker_get_name(end_marker));
            status_text(text);
            g_free(text);
        }
    }

    if (basetime > endtime) {
        GwTime tmp = basetime;
        basetime = endtime;
        endtime = tmp;
    }

    t_allocated = 1;
    t = malloc_2(sizeof(GwTime) * t_allocated);

    orig_basetime = basetime;
    while (1) {
        basetime = strace_timetrace(basetime, notfirst);
        notfirst = 1;

        if (endtime == MAX_HISTENT_TIME) {
            if (basetime == MAX_HISTENT_TIME)
                break;
        } else {
            if (basetime > endtime)
                break; /* formerly was >= which didn't mark the endpoint if true which is incorrect
                        */
        } /* i.e., if start is markable, end should be also */

        if (basetime >= orig_basetime) {
            t[GLOBALS->strace_ctx->timearray_size] = basetime;
            GLOBALS->strace_ctx->timearray_size++;
            if (GLOBALS->strace_ctx->timearray_size == t_allocated) {
                t_allocated *= 2;
                t = realloc_2(t, sizeof(GwTime) * t_allocated);
            }
        }
    }

    if (GLOBALS->strace_ctx->timearray_size) {
        GLOBALS->strace_ctx->timearray =
            realloc_2(t, sizeof(GwTime) * GLOBALS->strace_ctx->timearray_size);
    } else {
        free_2(t);
        GLOBALS->strace_ctx->timearray = NULL;
    }

    if (!GLOBALS->strace_ctx->shadow_active)
        update_mark_count_label();
}

/*
 * swap context for mark during trace load...
 */
void swap_strace_contexts(void)
{
    struct strace *stemp;
    char logical_mutex_temp[6];
    char mark_idx_start_temp, mark_idx_end_temp;

    stemp = GLOBALS->strace_ctx->straces;
    GLOBALS->strace_ctx->straces = GLOBALS->strace_ctx->shadow_straces;
    GLOBALS->strace_ctx->shadow_straces = stemp;

    memcpy(logical_mutex_temp, GLOBALS->strace_ctx->logical_mutex, 6);
    memcpy(GLOBALS->strace_ctx->logical_mutex, GLOBALS->strace_ctx->shadow_logical_mutex, 6);
    memcpy(GLOBALS->strace_ctx->shadow_logical_mutex, logical_mutex_temp, 6);

    mark_idx_start_temp = GLOBALS->strace_ctx->mark_idx_start;
    GLOBALS->strace_ctx->mark_idx_start = GLOBALS->strace_ctx->shadow_mark_idx_start;
    GLOBALS->strace_ctx->shadow_mark_idx_start = mark_idx_start_temp;

    mark_idx_end_temp = GLOBALS->strace_ctx->mark_idx_end;
    GLOBALS->strace_ctx->mark_idx_end = GLOBALS->strace_ctx->shadow_mark_idx_end;
    GLOBALS->strace_ctx->shadow_mark_idx_end = mark_idx_end_temp;
}

/*
 * delete context
 */
void delete_strace_context(void)
{
    int i;
    struct strace *stemp;
    struct strace *strace_cache;

    for (i = 0; i < 6; i++) {
        GLOBALS->strace_ctx->shadow_logical_mutex[i] = 0;
    }

    GLOBALS->strace_ctx->shadow_mark_idx_start = 0;
    GLOBALS->strace_ctx->shadow_mark_idx_end = 0;

    strace_cache = GLOBALS->strace_ctx->straces; /* so the trace actually deletes */
    GLOBALS->strace_ctx->straces = NULL;

    stemp = GLOBALS->strace_ctx->shadow_straces;
    while (stemp) {
        GLOBALS->strace_ctx->shadow_straces = stemp->next;
        if (stemp->string)
            free_2(stemp->string);

        FreeTrace(stemp->trace);
        free_2(stemp);
        stemp = GLOBALS->strace_ctx->shadow_straces;
    }

    if (GLOBALS->strace_ctx->shadow_string) {
        free_2(GLOBALS->strace_ctx->shadow_string);
        GLOBALS->strace_ctx->shadow_string = NULL;
    }

    GLOBALS->strace_ctx->straces = strace_cache;
}

/*************************************************************************/

/*
 * printf to memory..
 */

int mprintf(const char *fmt, ...) G_GNUC_PRINTF(1, 2);

int mprintf(const char *fmt, ...)
{
    int len;
    int rc;
    va_list args;
    struct mprintf_buff_t *bt = (struct mprintf_buff_t *)calloc_2(1, sizeof(struct mprintf_buff_t));
    char buff[65537];

    va_start(args, fmt);
    rc = vsprintf(buff, fmt, args);
    len = strlen(buff);
    bt->str = malloc_2(len + 1);
    strcpy(bt->str, buff);

    if (!GLOBALS->strace_ctx->mprintf_buff_current) {
        GLOBALS->strace_ctx->mprintf_buff_head = GLOBALS->strace_ctx->mprintf_buff_current = bt;
    } else {
        GLOBALS->strace_ctx->mprintf_buff_current->next = bt;
        GLOBALS->strace_ctx->mprintf_buff_current = bt;
    }

    va_end(args);
    return (rc);
}

/*
 * kill mprint buffer
 */
void delete_mprintf(void)
{
    if (GLOBALS->strace_ctx->mprintf_buff_head) {
        struct mprintf_buff_t *mb = GLOBALS->strace_ctx->mprintf_buff_head;
        struct mprintf_buff_t *mbt;

        while (mb) {
            free_2(mb->str);
            mbt = mb->next;
            free_2(mb);
            mb = mbt;
        }

        GLOBALS->strace_ctx->mprintf_buff_head = GLOBALS->strace_ctx->mprintf_buff_current = NULL;
    }
}

/*
 * so we can (later) write out the traces which are actually marked...
 */
void cache_actual_pattern_mark_traces(void)
{
    GwTrace *t;
    TraceFlagsType def = 0;
    GwTime prevshift = GW_TIME_CONSTANT(0);
    struct strace *st;

    delete_mprintf();

    if (GLOBALS->strace_ctx->timearray) {
        mprintf("!%d%d%d%d%d%d%c%c\n",
                GLOBALS->strace_ctx->logical_mutex[0],
                GLOBALS->strace_ctx->logical_mutex[1],
                GLOBALS->strace_ctx->logical_mutex[2],
                GLOBALS->strace_ctx->logical_mutex[3],
                GLOBALS->strace_ctx->logical_mutex[4],
                GLOBALS->strace_ctx->logical_mutex[5],
                '@' + GLOBALS->strace_ctx->mark_idx_start,
                '@' + GLOBALS->strace_ctx->mark_idx_end);
        st = GLOBALS->strace_ctx->straces;

        while (st) {
            if (st->value == ST_STRING) {
                mprintf("?\"%s\n",
                        st->string ? st->string : ""); /* search type for this trace is string.. */
            } else {
                mprintf("?%02x\n",
                        (unsigned char)st->value); /* else search type for this trace.. */
            }

            t = st->trace;

            if ((t->flags != def) || (st == GLOBALS->strace_ctx->straces)) {
                mprintf("@%" TRACEFLAGSPRIFMT "\n", def = t->flags);
            }

            if ((t->shift) || ((prevshift) && (!t->shift))) {
                mprintf(">%" GW_TIME_FORMAT "\n", t->shift);
            }
            prevshift = t->shift;

            if (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                if (t->vector) {
                    int i;
                    GwNode **nodes;

                    if (HasAlias(t)) {
                        mprintf("+{%s} ", t->name_full);
                    }

                    mprintf("#{%s}", t->name);

                    nodes = t->n.vec->bits->nodes;
                    for (i = 0; i < t->n.vec->bits->nnbits; i++) {
                        int was_packed = HIER_DEPACK_STATIC;
                        char *namex;
                        if (nodes[i]->expansion) {
                            namex = hier_decompress_flagged(nodes[i]->expansion->parent->nname,
                                                            &was_packed);
                            mprintf(" (%d)%s", nodes[i]->expansion->parentbit, namex);
                            /* if(was_packed) free_2(namex); ...not needed for HIER_DEPACK_STATIC */
                        } else {
                            /* namex = */ hier_decompress_flagged(nodes[i]->nname,
                                                                  &was_packed); /* scan-build */
                            mprintf(" %s", nodes[i]->nname);
                            /* if(was_packed) free_2(namex); ...not needed for HIER_DEPACK_STATIC */
                        }
                    }
                    mprintf("\n");
                } else {
                    int was_packed = HIER_DEPACK_STATIC;
                    char *namex;

                    if (HasAlias(t)) {
                        if (t->n.nd->expansion) {
                            namex = hier_decompress_flagged(t->n.nd->expansion->parent->nname,
                                                            &was_packed);
                            mprintf("+{%s} (%d)%s\n",
                                    t->name + 2,
                                    t->n.nd->expansion->parentbit,
                                    namex);
                            /* if(was_packed) free_2(namex); ...not needed for HIER_DEPACK_STATIC */
                        } else {
                            namex = hier_decompress_flagged(t->n.nd->nname, &was_packed);
                            mprintf("+{%s} %s\n", t->name + 2, namex);
                            /* if(was_packed) free_2(namex); ...not needed for HIER_DEPACK_STATIC */
                        }
                    } else {
                        if (t->n.nd->expansion) {
                            namex = hier_decompress_flagged(t->n.nd->expansion->parent->nname,
                                                            &was_packed);
                            mprintf("(%d)%s\n", t->n.nd->expansion->parentbit, namex);
                            /* if(was_packed) free_2(namex); ...not needed for HIER_DEPACK_STATIC */
                        } else {
                            namex = hier_decompress_flagged(t->n.nd->nname, &was_packed);
                            mprintf("%s\n", namex);
                            /* if(was_packed) free_2(namex); ...not needed for HIER_DEPACK_STATIC*/
                        }
                    }
                }
            }

            st = st->next;
        } /* while(st)... */

        mprintf("!!\n"); /* mark end of strace region */
    }
}
