/*
 * Copyright (c) Tony Bybell 1999-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */
#if defined _AIX
#pragma alloca
#endif

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "gtk23compat.h"
#include "analyzer.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "ghw.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"
#include "signal_list.h"

enum
{
    NAME_COLUMN,
    PTR_COLUMN,
    N_COLUMNS
};

static gboolean XXX_view_selection_func(GtkTreeSelection *selection,
                                        GtkTreeModel *model,
                                        GtkTreePath *path,
                                        gboolean path_currently_selected,
                                        gpointer userdata)
{
    (void)selection;
    (void)userdata;

    GtkTreeIter iter;
    char *nam = NULL;
    struct symbol *s;

    if (path) {
        if (gtk_tree_model_get_iter(model, &iter, path)) /* null return should not happen */
        {
            gtk_tree_model_get(model, &iter, NAME_COLUMN, &nam, PTR_COLUMN, &s, -1);

            if (!path_currently_selected) {
                set_s_selected(s, 1);
                GLOBALS->selected_rows_search_c_2++;
            } else {
                set_s_selected(s, 0);
                GLOBALS->selected_rows_search_c_2--;
            }
        }
    }

    return (TRUE);
}

int searchbox_is_active(void)
{
    return (GLOBALS->is_active_search_c_4);
}

static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    const gchar *entry_text;
    int len;
    const char *vname = "<Vector>";
    entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_a_search_c_1));
    entry_text = entry_text ? entry_text : "";
    DEBUG(printf("Entry contents: %s\n", entry_text));
    if (!(len = strlen(entry_text)))
        strcpy((GLOBALS->entrybox_text_local_search_c_2 = (char *)malloc_2(strlen(vname) + 1)),
               vname); /* make consistent with other widgets rather than producing NULL */
    else
        strcpy((GLOBALS->entrybox_text_local_search_c_2 = (char *)malloc_2(len + 1)), entry_text);

    wave_gtk_grab_remove(GLOBALS->window1_search_c_2);
    gtk_widget_destroy(GLOBALS->window1_search_c_2);
    GLOBALS->window1_search_c_2 = NULL;

    GLOBALS->cleanup_e_search_c_2();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    DEBUG(printf("Entry Cancel\n"));
    GLOBALS->entrybox_text_local_search_c_2 = NULL;
    wave_gtk_grab_remove(GLOBALS->window1_search_c_2);
    gtk_widget_destroy(GLOBALS->window1_search_c_2);
    GLOBALS->window1_search_c_2 = NULL;
}

static void entrybox_local(const char *title,
                           int width,
                           const char *default_text,
                           int maxch,
                           GCallback func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    GLOBALS->cleanup_e_search_c_2 = func;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    /* create a new modal window */
    GLOBALS->window1_search_c_2 =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window1_search_c_2,
                     ((char *)&GLOBALS->window1_search_c_2) - ((char *)GLOBALS));

    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->window1_search_c_2), width, 60);
    gtk_window_set_title(GTK_WINDOW(GLOBALS->window1_search_c_2), title);
    gtkwave_signal_connect(GLOBALS->window1_search_c_2,
                           "delete_event",
                           (GCallback)destroy_callback_e,
                           NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(GLOBALS->window1_search_c_2), vbox);
    gtk_widget_show(vbox);

    GLOBALS->entry_a_search_c_1 = X_gtk_entry_new_with_max_length(maxch);
    gtkwave_signal_connect(GLOBALS->entry_a_search_c_1,
                           "activate",
                           G_CALLBACK(enter_callback_e),
                           GLOBALS->entry_a_search_c_1);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->entry_a_search_c_1), default_text);
    gtk_editable_select_region(GTK_EDITABLE(GLOBALS->entry_a_search_c_1),
                               0,
                               gtk_entry_get_text_length(GTK_ENTRY(GLOBALS->entry_a_search_c_1)));
    gtk_box_pack_start(GTK_BOX(vbox), GLOBALS->entry_a_search_c_1, TRUE, TRUE, 0);
    gtk_widget_show(GLOBALS->entry_a_search_c_1);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    button1 = gtk_button_new_with_label("OK");
    gtk_widget_set_size_request(button1, 100, -1);
    gtkwave_signal_connect(button1, "clicked", G_CALLBACK(enter_callback_e), NULL);
    gtk_widget_show(button1);
    gtk_container_add(GTK_CONTAINER(hbox), button1);
    gtk_widget_set_can_default(button1, TRUE);
    gtkwave_signal_connect_object(button1, "realize", (GCallback)gtk_widget_grab_default, button1);

    button2 = gtk_button_new_with_label("Cancel");
    gtk_widget_set_size_request(button2, 100, -1);
    gtkwave_signal_connect(button2, "clicked", G_CALLBACK(destroy_callback_e), NULL);
    gtk_widget_set_can_default(button2, TRUE);
    gtk_widget_show(button2);
    gtk_container_add(GTK_CONTAINER(hbox), button2);

    gtk_widget_show(GLOBALS->window1_search_c_2);
    wave_gtk_grab_add(GLOBALS->window1_search_c_2);
}

/***************************************************************************/

static const char *regex_type[] = {"\\(\\[.*\\]\\)*$",
                                   "\\>.\\([0-9]\\)*$",
                                   "\\(\\[.*\\]\\)*$",
                                   "\\>.\\([0-9]\\)*$",
                                   ""};
static const char *regex_name[] = {"WRange", "WStrand", "Range", "Strand", "None"};

static void on_changed(GtkComboBox *widget, gpointer user_data)
{
    (void)user_data;

    GtkComboBox *combo_box = widget;
    int which = gtk_combo_box_get_active(combo_box);
    int i;

    for (i = 0; i < 5; i++)
        GLOBALS->regex_mutex_search_c_1[i] = 0;

    GLOBALS->regex_which_search_c_1 = which;
    GLOBALS->regex_mutex_search_c_1[GLOBALS->regex_which_search_c_1] = 1; /* mark our choice */

    DEBUG(printf("picked: %s\n", regex_name[GLOBALS->regex_which_search_c_1]));
}

/***************************************************************************/

/* call cleanup() on ok/insert functions */

static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    search_insert_callback(widget, 0); /* native to search */
}

struct symchain /* for restoring state of ->selected in signal regex search */
{
    struct symchain *next;
    struct symbol *symbol;
};

void search_insert_callback(GtkWidget *widget, char is_prepend)
{
    Traces tcache;
    struct symchain *symc, *symc_current;
    int i;

    gfloat interval;

    if (GLOBALS->is_insert_running_search_c_1)
        return;
    GLOBALS->is_insert_running_search_c_1 = ~0;
    wave_gtk_grab_add(widget);
    set_window_busy(widget);

    symc = NULL;

    memcpy(&tcache, &GLOBALS->traces, sizeof(Traces));
    GLOBALS->traces.total = 0;
    GLOBALS->traces.first = GLOBALS->traces.last = NULL;

    interval = (gfloat)(GLOBALS->num_rows_search_c_2 / 100.0);

    /* LX2 */
    GtkTreeIter iter;
    if (GLOBALS->is_lx2) {
        int pre_import = 0;

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);
        for (i = 0; i < GLOBALS->num_rows_search_c_2; i++) {
            struct symbol *s, *t;

            gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->sig_store_search),
                               &iter,
                               PTR_COLUMN,
                               &s,
                               -1);
            gtk_tree_model_iter_next(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);

            if (get_s_selected(s)) {
                if ((!s->vec_root) || (!GLOBALS->autocoalesce)) {
                    if (s->n->mv.mvlfac) {
                        lx2_set_fac_process_mask(s->n);
                        pre_import++;
                    }
                } else {
                    t = s->vec_root;
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            pre_import++;
                        }
                        t = t->vec_chain;
                    }
                }
            }
        }

        if (pre_import) {
            lx2_import_masked();
        }
    }
    /* LX2 */

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);
    for (i = 0; i < GLOBALS->num_rows_search_c_2; i++) {
        int len;
        struct symbol *s, *t;

        gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter, PTR_COLUMN, &s, -1);
        gtk_tree_model_iter_next(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);

        if (get_s_selected(s)) {
            GLOBALS->pdata->value = i;
            if (((int)(GLOBALS->pdata->value / interval)) !=
                ((int)(GLOBALS->pdata->oldvalue / interval))) {
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar),
                                              i / (gfloat)((GLOBALS->num_rows_search_c_2 > 1)
                                                               ? GLOBALS->num_rows_search_c_2 - 1
                                                               : 1));
                gtkwave_main_iteration();
            }
            GLOBALS->pdata->oldvalue = i;

            if ((!s->vec_root) || (!GLOBALS->autocoalesce)) {
                AddNodeUnroll(s->n, NULL);
            } else {
                len = 0;
                t = s->vec_root;
                set_s_selected(t, 1); /* move selected to head */
                while (t) {
                    if (get_s_selected(t)) {
                        if (len)
                            set_s_selected(t, 0);
                        symc_current = (struct symchain *)calloc_2(1, sizeof(struct symchain));
                        symc_current->next = symc;
                        symc_current->symbol = t;
                        symc = symc_current;
                    }
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        }
    }

    while (symc) {
        set_s_selected(symc->symbol, 1);
        symc_current = symc;
        symc = symc->next;
        free_2(symc_current);
    }

    GLOBALS->traces.buffercount = GLOBALS->traces.total;
    GLOBALS->traces.buffer = GLOBALS->traces.first;
    GLOBALS->traces.bufferlast = GLOBALS->traces.last;
    GLOBALS->traces.first = tcache.first;
    GLOBALS->traces.last = tcache.last;
    GLOBALS->traces.total = tcache.total;

    if (is_prepend) {
        PrependBuffer();
    } else {
        PasteBuffer();
    }

    GLOBALS->traces.buffercount = tcache.buffercount;
    GLOBALS->traces.buffer = tcache.buffer;
    GLOBALS->traces.bufferlast = tcache.bufferlast;

    redraw_signals_and_waves();

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar), 0.0);
    GLOBALS->pdata->oldvalue = -1.0;

    set_window_idle(widget);
    wave_gtk_grab_remove(widget);
    GLOBALS->is_insert_running_search_c_1 = 0;
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    Traces tcache;
    int i;
    GwTrace *tfirst;
    GwTrace *tlast;
    struct symchain *symc, *symc_current;

    gfloat interval;

    if (GLOBALS->is_replace_running_search_c_1)
        return;
    GLOBALS->is_replace_running_search_c_1 = ~0;
    wave_gtk_grab_add(widget);
    set_window_busy(widget);

    tfirst = NULL;
    tlast = NULL;
    symc = NULL;
    memcpy(&tcache, &GLOBALS->traces, sizeof(Traces));
    GLOBALS->traces.total = 0;
    GLOBALS->traces.first = GLOBALS->traces.last = NULL;

    interval = (gfloat)(GLOBALS->num_rows_search_c_2 / 100.0);
    GLOBALS->pdata->oldvalue = -1.0;

    /* LX2 */
    GtkTreeIter iter;
    if (GLOBALS->is_lx2) {
        int pre_import = 0;

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);
        for (i = 0; i < GLOBALS->num_rows_search_c_2; i++) {
            struct symbol *s, *t;

            gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->sig_store_search),
                               &iter,
                               PTR_COLUMN,
                               &s,
                               -1);
            gtk_tree_model_iter_next(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);

            if (get_s_selected(s)) {
                if ((!s->vec_root) || (!GLOBALS->autocoalesce)) {
                    if (s->n->mv.mvlfac) {
                        lx2_set_fac_process_mask(s->n);
                        pre_import++;
                    }
                } else {
                    t = s->vec_root;
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            pre_import++;
                        }
                        t = t->vec_chain;
                    }
                }
            }
        }

        if (pre_import) {
            lx2_import_masked();
        }
    }
    /* LX2 */

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);
    for (i = 0; i < GLOBALS->num_rows_search_c_2; i++) {
        int len;
        struct symbol *s, *t;

        gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter, PTR_COLUMN, &s, -1);
        gtk_tree_model_iter_next(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);

        if (get_s_selected(s)) {
            GLOBALS->pdata->value = i;
            if (((int)(GLOBALS->pdata->value / interval)) !=
                ((int)(GLOBALS->pdata->oldvalue / interval))) {
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar),
                                              i / (gfloat)((GLOBALS->num_rows_search_c_2 > 1)
                                                               ? GLOBALS->num_rows_search_c_2 - 1
                                                               : 1));
                gtkwave_main_iteration();
            }
            GLOBALS->pdata->oldvalue = i;

            if ((!s->vec_root) || (!GLOBALS->autocoalesce)) {
                AddNodeUnroll(s->n, NULL);
            } else {
                len = 0;
                t = s->vec_root;
                while (t) {
                    if (get_s_selected(t)) {
                        if (len)
                            set_s_selected(t, 0);
                        symc_current = (struct symchain *)calloc_2(1, sizeof(struct symchain));
                        symc_current->next = symc;
                        symc_current->symbol = t;
                        symc = symc_current;
                    }
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        }
    }

    while (symc) {
        set_s_selected(symc->symbol, 1);
        symc_current = symc;
        symc = symc->next;
        free_2(symc_current);
    }

    tfirst = GLOBALS->traces.first;
    tlast = GLOBALS->traces.last; /* cache for highlighting */

    GLOBALS->traces.buffercount = GLOBALS->traces.total;
    GLOBALS->traces.buffer = GLOBALS->traces.first;
    GLOBALS->traces.bufferlast = GLOBALS->traces.last;
    GLOBALS->traces.first = tcache.first;
    GLOBALS->traces.last = tcache.last;
    GLOBALS->traces.total = tcache.total;

    {
        GwTrace *t = GLOBALS->traces.first;
        GwTrace **tp = NULL;
        int numhigh = 0;
        int it;

        while (t) {
            if (t->flags & TR_HIGHLIGHT) {
                numhigh++;
            }
            t = t->t_next;
        }
        if (numhigh) {
            tp = calloc_2(numhigh, sizeof(GwTrace *));
            t = GLOBALS->traces.first;
            it = 0;
            while (t) {
                if (t->flags & TR_HIGHLIGHT) {
                    tp[it++] = t;
                }
                t = t->t_next;
            }
        }

        PasteBuffer();

        GLOBALS->traces.buffercount = tcache.buffercount;
        GLOBALS->traces.buffer = tcache.buffer;
        GLOBALS->traces.bufferlast = tcache.bufferlast;

        for (it = 0; it < numhigh; it++) {
            tp[it]->flags |= TR_HIGHLIGHT;
        }

        t = tfirst;
        while (t) {
            t->flags &= ~TR_HIGHLIGHT;
            if (t == tlast)
                break;
            t = t->t_next;
        }

        CutBuffer();

        while (tfirst) {
            tfirst->flags |= TR_HIGHLIGHT;
            if (tfirst == tlast)
                break;
            tfirst = tfirst->t_next;
        }

        if (tp) {
            free_2(tp);
        }
    }

    redraw_signals_and_waves();

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar), 0.0);
    GLOBALS->pdata->oldvalue = -1.0;

    set_window_idle(widget);
    wave_gtk_grab_remove(widget);
    GLOBALS->is_replace_running_search_c_1 = 0;
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)nothing;

    int i;
    struct symchain *symc, *symc_current;

    gfloat interval;

    if (GLOBALS->is_append_running_search_c_1)
        return;
    GLOBALS->is_append_running_search_c_1 = ~0;
    wave_gtk_grab_add(widget);
    set_window_busy(widget);

    symc = NULL;

    interval = (gfloat)(GLOBALS->num_rows_search_c_2 / 100.0);
    GLOBALS->pdata->oldvalue = -1.0;

    /* LX2 */
    GtkTreeIter iter;
    if (GLOBALS->is_lx2) {
        int pre_import = 0;

        gtk_tree_model_get_iter_first(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);
        for (i = 0; i < GLOBALS->num_rows_search_c_2; i++) {
            struct symbol *s, *t;

            gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->sig_store_search),
                               &iter,
                               PTR_COLUMN,
                               &s,
                               -1);
            gtk_tree_model_iter_next(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);

            if (get_s_selected(s)) {
                if ((!s->vec_root) || (!GLOBALS->autocoalesce)) {
                    if (s->n->mv.mvlfac) {
                        lx2_set_fac_process_mask(s->n);
                        pre_import++;
                    }
                } else {
                    t = s->vec_root;
                    while (t) {
                        if (t->n->mv.mvlfac) {
                            lx2_set_fac_process_mask(t->n);
                            pre_import++;
                        }
                        t = t->vec_chain;
                    }
                }
            }
        }

        if (pre_import) {
            lx2_import_masked();
        }
    }
    /* LX2 */

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);
    for (i = 0; i < GLOBALS->num_rows_search_c_2; i++) {
        int len;
        struct symbol *s, *t;

        gtk_tree_model_get(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter, PTR_COLUMN, &s, -1);
        gtk_tree_model_iter_next(GTK_TREE_MODEL(GLOBALS->sig_store_search), &iter);

        if (get_s_selected(s)) {
            GLOBALS->pdata->value = i;
            if (((int)(GLOBALS->pdata->value / interval)) !=
                ((int)(GLOBALS->pdata->oldvalue / interval))) {
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar),
                                              i / (gfloat)((GLOBALS->num_rows_search_c_2 > 1)
                                                               ? GLOBALS->num_rows_search_c_2 - 1
                                                               : 1));
                gtkwave_main_iteration();
            }
            GLOBALS->pdata->oldvalue = i;

            if ((!s->vec_root) || (!GLOBALS->autocoalesce)) {
                AddNodeUnroll(s->n, NULL);
            } else {
                len = 0;
                t = s->vec_root;
                while (t) {
                    if (get_s_selected(t)) {
                        if (len)
                            set_s_selected(t, 0);
                        symc_current = (struct symchain *)calloc_2(1, sizeof(struct symchain));
                        symc_current->next = symc;
                        symc_current->symbol = t;
                        symc = symc_current;
                    }
                    len++;
                    t = t->vec_chain;
                }
                if (len)
                    add_vector_chain(s->vec_root, len);
            }
        }
    }

    while (symc) {
        set_s_selected(symc->symbol, 1);
        symc_current = symc;
        symc = symc->next;
        free_2(symc_current);
    }

    gw_signal_list_scroll_to_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), GLOBALS->traces.last);
    redraw_signals_and_waves();

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar), 0.0);
    GLOBALS->pdata->oldvalue = -1.0;

    set_window_idle(widget);
    wave_gtk_grab_remove(widget);
    GLOBALS->is_append_running_search_c_1 = 0;
}

void search_enter_callback(GtkWidget *widget, GtkWidget *do_warning)
{
    const gchar *entry_text;
    char *entry_suffixed;
    int i;
    char *s, *tmp2;
    gfloat interval;
    char *duplicate_row_buffer = NULL;

    if (GLOBALS->is_searching_running_search_c_1)
        return;
    GLOBALS->is_searching_running_search_c_1 = ~0;
    wave_gtk_grab_add(widget);

    entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_search_c_3));
    entry_text = entry_text ? entry_text : "";
    DEBUG(printf("Entry contents: %s\n", entry_text));

    free_2(GLOBALS->searchbox_text_search_c_1);

    if (strlen(entry_text)) {
        GLOBALS->searchbox_text_search_c_1 = strdup_2(entry_text);
    } else {
        GLOBALS->searchbox_text_search_c_1 = strdup_2("");
    }

    GLOBALS->num_rows_search_c_2 = 0;

    entry_suffixed =
        g_alloca(strlen(GLOBALS->searchbox_text_search_c_1 /* scan-build, was entry_text */) +
                 strlen(regex_type[GLOBALS->regex_which_search_c_1]) + 1 +
                 ((GLOBALS->regex_which_search_c_1 < 2) ? 2 : 0));
    *entry_suffixed = 0x00;
    if (GLOBALS->regex_which_search_c_1 < 2)
        strcpy(entry_suffixed, "\\<"); /* match on word boundary */
    strcat(entry_suffixed, GLOBALS->searchbox_text_search_c_1); /* scan-build */
    strcat(entry_suffixed, regex_type[GLOBALS->regex_which_search_c_1]);
    wave_regex_compile(entry_suffixed, WAVE_REGEX_SEARCH);
    for (i = 0; i < GLOBALS->numfacs; i++) {
        set_s_selected(GLOBALS->facs[i], 0);
    }

    GLOBALS->pdata->oldvalue = -1.0;
    interval = (gfloat)(GLOBALS->numfacs / 100.0);

    duplicate_row_buffer = (char *)calloc_2(1, GLOBALS->longestname + 1);

    gtk_list_store_clear(GTK_LIST_STORE(GLOBALS->sig_store_search));

    for (i = 0; i < GLOBALS->numfacs; i++) {
        int was_packed = HIER_DEPACK_STATIC;
        char *hfacname = NULL;
        int skiprow;

        GLOBALS->pdata->value = i;
        if (((int)(GLOBALS->pdata->value / interval)) !=
            ((int)(GLOBALS->pdata->oldvalue / interval))) {
            gtk_progress_bar_set_fraction(
                GTK_PROGRESS_BAR(GLOBALS->pdata->pbar),
                i / (gfloat)((GLOBALS->numfacs > 1) ? GLOBALS->numfacs - 1 : 1));
            gtkwave_main_iteration();
        }
        GLOBALS->pdata->oldvalue = i;

        hfacname = hier_decompress_flagged(GLOBALS->facs[i]->name, &was_packed);
        if (!strcmp(hfacname, duplicate_row_buffer)) {
            skiprow = 1;
        } else {
            skiprow = 0;
            strcpy(duplicate_row_buffer, hfacname);
        }

        if ((!skiprow) && wave_regex_match(hfacname, WAVE_REGEX_SEARCH))
            if ((!GLOBALS->is_ghw) || (strcmp(WAVE_GHW_DUMMYFACNAME, hfacname))) {
                GtkTreeIter iter;

                if (!GLOBALS->facs[i]->vec_root) {
                    gtk_list_store_append(GTK_LIST_STORE(GLOBALS->sig_store_search), &iter);
                    gtk_list_store_set(GTK_LIST_STORE(GLOBALS->sig_store_search),
                                       &iter,
                                       NAME_COLUMN,
                                       hfacname,
                                       PTR_COLUMN,
                                       GLOBALS->facs[i],
                                       -1);
                } else {
                    if (GLOBALS->autocoalesce) {
                        if (GLOBALS->facs[i]->vec_root != GLOBALS->facs[i])
                            continue;

                        tmp2 = makename_chain(GLOBALS->facs[i]);
                        s = (char *)malloc_2(strlen(tmp2) + 4);
                        strcpy(s, "[] ");
                        strcpy(s + 3, tmp2);
                        free_2(tmp2);
                    } else {
                        s = (char *)malloc_2(strlen(hfacname) + 4);
                        strcpy(s, "[] ");
                        strcpy(s + 3, hfacname);
                    }

                    gtk_list_store_append(GTK_LIST_STORE(GLOBALS->sig_store_search), &iter);
                    gtk_list_store_set(GTK_LIST_STORE(GLOBALS->sig_store_search),
                                       &iter,
                                       NAME_COLUMN,
                                       s,
                                       PTR_COLUMN,
                                       GLOBALS->facs[i],
                                       -1);
                    free_2(s);
                }

                GLOBALS->num_rows_search_c_2++;
                if (GLOBALS->num_rows_search_c_2 == WAVE_MAX_CLIST_LENGTH) {
                    /* if(was_packed) { free_2(hfacname); } ...not needed with HIER_DEPACK_STATIC */
                    break;
                }
            }

        /* if(was_packed) { free_2(hfacname); } ...not needed with HIER_DEPACK_STATIC */
    }

    free_2(duplicate_row_buffer);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar), 0.0);
    GLOBALS->pdata->oldvalue = -1.0;
    wave_gtk_grab_remove(widget);
    GLOBALS->is_searching_running_search_c_1 = 0;

    if (do_warning)
        if (GLOBALS->num_rows_search_c_2 >= WAVE_MAX_CLIST_LENGTH) {
            char buf[256];
            sprintf(buf, "Limiting results to first %d entries.", GLOBALS->num_rows_search_c_2);
            simplereqbox("Regex Search Warning", 300, buf, "OK", NULL, NULL, 1);
        }
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    if ((!GLOBALS->is_insert_running_search_c_1) && (!GLOBALS->is_replace_running_search_c_1) &&
        (!GLOBALS->is_append_running_search_c_1) && (!GLOBALS->is_searching_running_search_c_1)) {
        GLOBALS->is_active_search_c_4 = 0;
        gtk_widget_destroy(GLOBALS->window_search_c_7);
        GLOBALS->window_search_c_7 = NULL;
        GLOBALS->sig_store_search = NULL;
        GLOBALS->sig_selection_search = NULL;
        GLOBALS->sig_view_search = NULL;
    }
}

static void select_all_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    gtk_tree_selection_select_all(GLOBALS->sig_selection_search);
}

static void unselect_all_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    gtk_tree_selection_unselect_all(GLOBALS->sig_selection_search);
}

/*
 * mainline..
 */
void searchbox(const char *title, GCallback func)
{
    int i;
    const gchar *titles[] = {"Matches"};
    int cached_which = GLOBALS->regex_which_search_c_1;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    if (GLOBALS->is_active_search_c_4) {
        gdk_window_raise(gtk_widget_get_window(GLOBALS->window_search_c_7));
        return;
    }

    if (!GLOBALS->searchbox_text_search_c_1)
        GLOBALS->searchbox_text_search_c_1 = strdup_2("");

    GLOBALS->is_active_search_c_4 = 1;
    GLOBALS->cleanup_search_c_5 = func;
    GLOBALS->num_rows_search_c_2 = GLOBALS->selected_rows_search_c_2 = 0;

    /* create a new modal window */
    GLOBALS->window_search_c_7 =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_search_c_7,
                     ((char *)&GLOBALS->window_search_c_7) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW(GLOBALS->window_search_c_7), title);
    gtkwave_signal_connect(GLOBALS->window_search_c_7,
                           "delete_event",
                           (GCallback)destroy_callback,
                           NULL);
    gtk_container_set_border_width(GTK_CONTAINER(GLOBALS->window_search_c_7), 12);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(GLOBALS->window_search_c_7), main_vbox);

    GtkWidget *label = gtk_label_new("Signal Search Expression");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(main_vbox), label, FALSE, FALSE, 0);

    GLOBALS->entry_search_c_3 = X_gtk_entry_new_with_max_length(256);
    gtkwave_signal_connect(GLOBALS->entry_search_c_3,
                           "activate",
                           G_CALLBACK(search_enter_callback),
                           GLOBALS->entry_search_c_3);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->entry_search_c_3), GLOBALS->searchbox_text_search_c_1);
    gtk_editable_select_region(GTK_EDITABLE(GLOBALS->entry_search_c_3),
                               0,
                               gtk_entry_get_text_length(GTK_ENTRY(GLOBALS->entry_search_c_3)));
    gtk_tooltips_set_tip_2(
        GLOBALS->entry_search_c_3,
        "Enter search expression here.  POSIX Wildcards are allowed.  Note that you may also "
        "modify the search criteria by selecting ``[W]Range'', ``[W]Strand'', or ``None'' for "
        "suffix "
        "matching.");
    gtk_box_pack_start(GTK_BOX(main_vbox), GLOBALS->entry_search_c_3, FALSE, FALSE, 0);

    /* Allocate memory for the data that is used later */
    GLOBALS->pdata = calloc_2(1, sizeof(SearchProgressData));
    GLOBALS->pdata->value = GLOBALS->pdata->oldvalue = 0.0;

    /* Create the GtkProgressBar */
    GLOBALS->pdata->pbar = gtk_progress_bar_new();
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar), " ");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(GLOBALS->pdata->pbar), 0.0);
    gtk_widget_show(GLOBALS->pdata->pbar);
    gtk_box_pack_start(GTK_BOX(main_vbox), GLOBALS->pdata->pbar, FALSE, FALSE, 0);

    GLOBALS->sig_store_search = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
    GLOBALS->sig_view_search =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(GLOBALS->sig_store_search));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref(G_OBJECT(GLOBALS->sig_store_search));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes(titles[0], renderer, "text", NAME_COLUMN, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(GLOBALS->sig_view_search), column);

    /* Setup the selection handler */
    GLOBALS->sig_selection_search =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->sig_view_search));
    gtk_tree_selection_set_mode(GLOBALS->sig_selection_search, GTK_SELECTION_MULTIPLE);
    gtk_tree_selection_set_select_function(GLOBALS->sig_selection_search,
                                           XXX_view_selection_func,
                                           NULL,
                                           NULL);

    gtk_list_store_clear(GTK_LIST_STORE(GLOBALS->sig_store_search));

    gtk_widget_show(GLOBALS->sig_view_search);

    dnd_setup(GLOBALS->sig_view_search, TRUE);

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_win), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), -1, 300);
    gtk_container_add(GTK_CONTAINER(scrolled_win), GLOBALS->sig_view_search);

    gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_win, TRUE, TRUE, 0);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *select_all_button = gtk_button_new_with_label("Select All");
    gtkwave_signal_connect_object(select_all_button,
                                  "clicked",
                                  G_CALLBACK(select_all_callback),
                                  GLOBALS->window_search_c_7);
    gtk_tooltips_set_tip_2(select_all_button, "Highlight all signals listed in the match window.");
    gtk_box_pack_start(GTK_BOX(button_box), select_all_button, TRUE, FALSE, 0);

    GtkWidget *combo_box = gtk_combo_box_text_new();
    for (i = 0; i < 5; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), regex_name[i]);
        GLOBALS->regex_mutex_search_c_1[i] = 0;
    }

    GLOBALS->regex_which_search_c_1 = cached_which;
    if ((GLOBALS->regex_which_search_c_1 < 0) || (GLOBALS->regex_which_search_c_1 > 4))
        GLOBALS->regex_which_search_c_1 = 0;
    GLOBALS->regex_mutex_search_c_1[GLOBALS->regex_which_search_c_1] =
        1; /* configure for "range", etc */
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), GLOBALS->regex_which_search_c_1);

    gtk_tooltips_set_tip_2(
        combo_box,
        "You may "
        "modify the search criteria by selecting ``Range'', ``Strand'', or ``None'' for suffix "
        "matching.  This optionally matches the string you enter in the search string above with a "
        "Verilog "
        "format range (signal[7:0]), a strand (signal.1, signal.0), or with no suffix.  "
        "The ``W'' modifier for ``Range'' and ``Strand'' explicitly matches on word boundaries.  "
        "(addr matches unit.freezeaddr[63:0] for ``Range'' but only unit.addr[63:0] for ``WRange'' "
        "since addr has to be on a word boundary.  "
        "Note that when ``None'' "
        "is selected, the search string may be located anywhere in the signal name.");

    g_signal_connect(combo_box, "changed", G_CALLBACK(on_changed), NULL);

    gtk_box_pack_start(GTK_BOX(button_box), combo_box, TRUE, TRUE, 0);

    GtkWidget *unselect_all_button = gtk_button_new_with_label("Unselect All");
    gtkwave_signal_connect_object(unselect_all_button,
                                  "clicked",
                                  G_CALLBACK(unselect_all_callback),
                                  GLOBALS->window_search_c_7);
    gtk_tooltips_set_tip_2(unselect_all_button,
                           "Unhighlight all signals listed in the match window.");
    gtk_box_pack_start(GTK_BOX(button_box), unselect_all_button, TRUE, TRUE, 0);

    GtkWidget *button_box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box2, FALSE, FALSE, 0);

    GtkWidget *append_button = gtk_button_new_with_label("Append");
    gtkwave_signal_connect_object(append_button,
                                  "clicked",
                                  G_CALLBACK(ok_callback),
                                  GLOBALS->window_search_c_7);
    gtk_tooltips_set_tip_2(append_button,
                           "Add selected signals to end of the display on the main window.");
    gtk_box_pack_start(GTK_BOX(button_box2), append_button, TRUE, TRUE, 0);

    GtkWidget *insert_button = gtk_button_new_with_label("Insert");
    gtkwave_signal_connect_object(insert_button,
                                  "clicked",
                                  G_CALLBACK(insert_callback),
                                  GLOBALS->window_search_c_7);
    gtk_tooltips_set_tip_2(
        insert_button,
        "Add selected signals after last highlighted signal on the main window.");
    gtk_box_pack_start(GTK_BOX(button_box2), insert_button, TRUE, TRUE, 0);

    GtkWidget *replace_button = gtk_button_new_with_label("Replace");
    gtkwave_signal_connect_object(replace_button,
                                  "clicked",
                                  G_CALLBACK(replace_callback),
                                  GLOBALS->window_search_c_7);
    gtk_tooltips_set_tip_2(
        replace_button,
        "Replace highlighted signals on the main window with signals selected above.");
    gtk_box_pack_start(GTK_BOX(button_box2), replace_button, TRUE, TRUE, 0);

    GtkWidget *exit_button = gtk_button_new_with_label("Exit");
    gtkwave_signal_connect_object(exit_button,
                                  "clicked",
                                  G_CALLBACK(destroy_callback),
                                  GLOBALS->window_search_c_7);
    gtk_tooltips_set_tip_2(exit_button, "Do nothing and return to the main window.");
    gtk_box_pack_start(GTK_BOX(button_box2), exit_button, TRUE, TRUE, 0);

    gtk_widget_show_all(GLOBALS->window_search_c_7);

    if (strlen(GLOBALS->searchbox_text_search_c_1) > 0)
        search_enter_callback(GLOBALS->entry_search_c_3, NULL);
}
