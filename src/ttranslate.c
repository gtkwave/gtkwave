/*
 * Copyright (c) Tony Bybell 2010-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "gtk23compat.h"
#include "symbol.h"
#include "ttranslate.h"
#include "pipeio.h"
#include "debug.h"

enum
{
    NAME_COLUMN,
    N_COLUMNS
};

static gboolean XXX_view_selection_func(GtkTreeSelection *selection,
                                        GtkTreeModel *model,
                                        GtkTreePath *path,
                                        gboolean path_currently_selected,
                                        gpointer userdata)
{
    (void)selection;
    (void)model;
    (void)userdata;

    gint *idx = NULL;

    if (path) {
        idx = gtk_tree_path_get_indices(path);

        if (idx) {
            if (!path_currently_selected) {
                GLOBALS->current_filter_ttranslate_c_1 = idx[0] + 1;
            } else {
                GLOBALS->current_filter_ttranslate_c_1 = 0; /* none */
            }
        }
    }

    return (TRUE);
}

static void args_entry_callback(GtkWidget *widget, GtkWidget *entry)
{
    (void)widget;

    const gchar *entry_text;

    entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
    entry_text = entry_text ? entry_text : "";

    if (GLOBALS->ttranslate_args) {
        free_2(GLOBALS->ttranslate_args);
    }
    GLOBALS->ttranslate_args = strdup_2(entry_text);

    DEBUG(printf("Args Entry contents: %s\n", entry_text));
}

void init_ttrans_data(void)
{
    int i;

    if (!GLOBALS->ttranssel_filter) {
        GLOBALS->ttranssel_filter = calloc_2(TTRANS_FILTER_MAX + 1, sizeof(char *));
    }
    if (!GLOBALS->ttrans_filter) {
        GLOBALS->ttrans_filter = calloc_2(TTRANS_FILTER_MAX + 1, sizeof(struct pipe_ctx *));
    }

    for (i = 0; i < TTRANS_FILTER_MAX + 1; i++) {
        GLOBALS->ttranssel_filter[i] = NULL;
        GLOBALS->ttrans_filter[i] = NULL;
    }
}

void remove_all_ttrans_filters(void)
{
    struct Global *GLOBALS_cache = GLOBALS;
    unsigned int i, j;

    for (j = 0; j < GLOBALS->num_notebook_pages; j++) {
        GLOBALS = (*GLOBALS->contexts)[j];

        for (i = 1; i < TTRANS_FILTER_MAX + 1; i++) {
            if (GLOBALS->ttrans_filter[i]) {
                pipeio_destroy(GLOBALS->ttrans_filter[i]);
                GLOBALS->ttrans_filter[i] = NULL;
            }

            if (GLOBALS->ttranssel_filter[i]) {
                free_2(GLOBALS->ttranssel_filter[i]);
                GLOBALS->ttranssel_filter[i] = NULL;
            }
        }

        GLOBALS = GLOBALS_cache;
    }
}

int traverse_vector_nodes(GwTrace *t);

/*
 * this is likely obsolete
 */
#if 0
static void remove_ttrans_filter(int which, int regen)
{
if(GLOBALS->ttrans_filter[which])
	{
	pipeio_destroy(GLOBALS->ttrans_filter[which]);
	GLOBALS->ttrans_filter[which] = NULL;

	if(regen)
	        {
		GLOBALS->signalwindow_width_dirty=1;
		redraw_signals_and_waves();
	        }
	}
}
#endif

static void load_ttrans_filter(int which, char *name)
{
    FILE *stream;

    char *cmd;
    char exec_name[1025];
    char abs_path[1025];
    char *arg, end;
    int result;

    exec_name[0] = 0;
    abs_path[0] = 0;

    /* if name has arguments grab only the first word (the name of the executable)*/
    sscanf(name, "%s ", exec_name);

    arg = name + strlen(exec_name);

    /* remove leading spaces from argument */
    while (isspace((int)(unsigned char)arg[0])) {
        arg++;
    }

    /* remove trailing spaces from argument */
    if (strlen(arg) > 0) {
        end = strlen(arg) - 1;

        while (arg[(int)end] == ' ') {
            arg[(int)end] = 0;
            end--;
        }
    }

    /* turn the exec_name into an absolute path */
#if !defined __MINGW32__
    cmd = (char *)malloc_2(strlen(exec_name) + 6 + 1);
    sprintf(cmd, "which %s", exec_name);
    stream = popen(cmd, "r");

    result = fscanf(stream, "%s", abs_path);

    if ((strlen(abs_path) == 0) || (!result)) {
        status_text("Could not find transaction filter process!\n");
        pclose(stream); /* cppcheck */
        return;
    }

    pclose(stream);
    free_2(cmd);
#else
    strcpy(abs_path, exec_name);
#endif

    /* remove_ttrans_filter(which, 0); ... should never happen from GUI, but perhaps possible from
     * save files or other weirdness */
    if (!GLOBALS->ttrans_filter[which]) {
        GLOBALS->ttrans_filter[which] = pipeio_create(abs_path, arg);
    }
}

int install_ttrans_filter(int which)
{
    int found = 0;

    if ((which < 0) || (which >= (PROC_FILTER_MAX + 1))) {
        which = 0;
    }

    if (GLOBALS->traces.first) {
        GwTrace *t = GLOBALS->traces.first;
        while (t) {
            if (t->flags & TR_HIGHLIGHT) {
                if ((!t->vector) && (which)) {
                    GwBitVector *v = combine_traces(1, t); /* down: make single signal a vector */
                    if (v) {
                        v->transaction_nd = t->n.nd; /* cache for savefile, disable */
                        t->vector = 1;
                        t->n.vec = v; /* splice in */
                    }
                }

                if ((t->vector) && (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)))) {
                    t->t_filter = which;
                    t->t_filter_converted = 0;

                    /* back out allocation to revert (if any) */
                    if (t->n.vec->transaction_cache) {
                        int i;
                        GwBitVector *bv = t->n.vec;
                        GwBitVector *bv2;
                        GwNode *ndcache = NULL;

                        t->n.vec = bv->transaction_cache;
                        if ((t->n.vec->transaction_nd) && (!which)) {
                            ndcache = t->n.vec->transaction_nd;
                        }

                        while (bv) {
                            bv2 = bv->transaction_chain;

                            if (bv->bvname) {
                                free_2(bv->bvname);
                            }

                            for (i = 0; i < bv->numregions; i++) {
                                free_2(bv->vectors[i]);
                            }

                            free_2(bv);
                            bv = bv2;
                        }

                        t->name = t->n.vec->bvname;
                        if (GLOBALS->hier_max_level)
                            t->name = hier_extract(t->name, GLOBALS->hier_max_level);

                        if (ndcache) {
                            t->n.nd = ndcache;
                            t->vector = 0;
                            /* still need to deallocate old t->n.vec! */
                        }
                    }

                    if (!which) {
                        t->flags &= (~(TR_TTRANSLATED | TR_ANALOGMASK));
                    } else {
                        t->flags &= (~(TR_ANALOGMASK));
                        t->flags |= TR_TTRANSLATED;
                        if (t->transaction_args)
                            free_2(t->transaction_args);
                        if (GLOBALS->ttranslate_args) {
                            t->transaction_args = strdup_2(GLOBALS->ttranslate_args);
                        } else {
                            t->transaction_args = NULL;
                        }
                        traverse_vector_nodes(t);
                    }
                    found++;

                    if (t->t_match) {
                        GwTrace *curr_trace = t;
                        t = t->t_next;
                        while (t && (t->t_match != curr_trace)) {
                            t = t->t_next;
                        }
                    }
                }
            }
            t = GiveNextTrace(t);
        }
    }

    if (found) {
        GLOBALS->signalwindow_width_dirty = 1;
        redraw_signals_and_waves();
    }

    return (found);
}

/************************************************************************/

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    GLOBALS->is_active_ttranslate_c_2 = 0;
    gtk_widget_destroy(GLOBALS->window_ttranslate_c_5);
    GLOBALS->window_ttranslate_c_5 = NULL;
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
    install_ttrans_filter(GLOBALS->current_filter_ttranslate_c_1);
    destroy_callback(widget, nothing);
}

static void add_filter_callback_2(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    int i;

    if (!GLOBALS->filesel_ok) {
        return;
    }

    if (*GLOBALS->fileselbox_text) {
        for (i = 0; i < GLOBALS->num_ttrans_filters; i++) {
            if (GLOBALS->ttranssel_filter[i]) {
                if (!strcmp(GLOBALS->ttranssel_filter[i], *GLOBALS->fileselbox_text)) {
                    status_text("Filter already imported.\n");
                    if (GLOBALS->is_active_ttranslate_c_2)
                        gdk_window_raise(gtk_widget_get_window(GLOBALS->window_ttranslate_c_5));
                    return;
                }
            }
        }
    }

    GLOBALS->num_ttrans_filters++;
    load_ttrans_filter(GLOBALS->num_ttrans_filters, *GLOBALS->fileselbox_text);
    if (GLOBALS->ttrans_filter[GLOBALS->num_ttrans_filters]) {
        if (GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters])
            free_2(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]);
        GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters] =
            malloc_2(strlen(*GLOBALS->fileselbox_text) + 1);
        strcpy(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters], *GLOBALS->fileselbox_text);

        GtkTreeIter iter;
        gtk_list_store_append(GTK_LIST_STORE(GLOBALS->sig_store_ttranslate), &iter);
        gtk_list_store_set(GTK_LIST_STORE(GLOBALS->sig_store_ttranslate),
                           &iter,
                           NAME_COLUMN,
                           GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters],
                           -1);
    } else {
        GLOBALS->num_ttrans_filters--;
    }

    if (GLOBALS->is_active_ttranslate_c_2)
        gdk_window_raise(gtk_widget_get_window(GLOBALS->window_ttranslate_c_5));
}

static void add_filter_callback(GtkWidget *widget, GtkWidget *nothing)
{
    (void)widget;
    (void)nothing;

    if (GLOBALS->num_ttrans_filters == TTRANS_FILTER_MAX) {
        status_text("Max number of transaction filters processes installed already.\n");
        return;
    }

    fileselbox("Select Transaction Filter Process",
               &GLOBALS->fcurr_ttranslate_c_1,
               G_CALLBACK(add_filter_callback_2),
               G_CALLBACK(NULL),
               "*",
               0);
}

/*
 * mainline..
 */
void ttrans_searchbox(const char *title)
{
    int i;

    const gchar *titles[] = {"Transaction Process Filter Select"};

    if (GLOBALS->is_active_ttranslate_c_2) {
        gdk_window_raise(gtk_widget_get_window(GLOBALS->window_ttranslate_c_5));
        return;
    }

    GLOBALS->is_active_ttranslate_c_2 = 1;
    GLOBALS->current_filter_ttranslate_c_1 = 0;

    /* create a new modal window */
    GLOBALS->window_ttranslate_c_5 =
        gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_ttranslate_c_5,
                     ((char *)&GLOBALS->window_ttranslate_c_5) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW(GLOBALS->window_ttranslate_c_5), title);
    gtkwave_signal_connect(GLOBALS->window_ttranslate_c_5,
                           "delete_event",
                           (GCallback)destroy_callback,
                           NULL);
    gtk_container_set_border_width(GTK_CONTAINER(GLOBALS->window_ttranslate_c_5), 12);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(GLOBALS->window_ttranslate_c_5), main_vbox);

    GLOBALS->sig_store_ttranslate = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
    GtkWidget *sig_view =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(GLOBALS->sig_store_ttranslate));

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref(G_OBJECT(GLOBALS->sig_store_ttranslate));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes(titles[0], renderer, "text", NAME_COLUMN, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sig_view), column);

    /* Setup the selection handler */
    GLOBALS->sig_selection_ttranslate = gtk_tree_view_get_selection(GTK_TREE_VIEW(sig_view));
    gtk_tree_selection_set_mode(GLOBALS->sig_selection_ttranslate, GTK_SELECTION_SINGLE);
    gtk_tree_selection_set_select_function(GLOBALS->sig_selection_ttranslate,
                                           XXX_view_selection_func,
                                           NULL,
                                           NULL);

    gtk_list_store_clear(GTK_LIST_STORE(GLOBALS->sig_store_ttranslate));
    for (i = 0; i < GLOBALS->num_ttrans_filters; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(GTK_LIST_STORE(GLOBALS->sig_store_ttranslate), &iter);
        gtk_list_store_set(GTK_LIST_STORE(GLOBALS->sig_store_ttranslate),
                           &iter,
                           NAME_COLUMN,
                           GLOBALS->ttranssel_filter[i + 1],
                           -1);
    }

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_win), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), -1, 300);
    gtk_container_add(GTK_CONTAINER(scrolled_win), sig_view);

    gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_win, TRUE, TRUE, 0);

    GtkWidget *add_button = gtk_button_new_with_label("Add Trans Filter to List");
    gtkwave_signal_connect_object(add_button,
                                  "clicked",
                                  G_CALLBACK(add_filter_callback),
                                  GLOBALS->window_ttranslate_c_5);
    gtk_tooltips_set_tip_2(add_button,
                           "Bring up a file requester to add a transaction process filter to the "
                           "filter select window.");
    gtk_box_pack_start(GTK_BOX(main_vbox), add_button, FALSE, FALSE, 0);

    /* args entry box */
    {
        GwTrace *t = GLOBALS->traces.first;
        while (t) {
            if (t->flags & TR_HIGHLIGHT) {
                if (t->transaction_args) {
                    if (GLOBALS->ttranslate_args)
                        free_2(GLOBALS->ttranslate_args);
                    GLOBALS->ttranslate_args = strdup_2(t->transaction_args);
                    break;
                }
            }

            t = t->t_next;
        }

        GtkWidget *label = gtk_label_new("Args:");
        GtkWidget *entry = X_gtk_entry_new_with_max_length(1025);

        gtk_entry_set_text(GTK_ENTRY(entry),
                           GLOBALS->ttranslate_args ? GLOBALS->ttranslate_args : "");
        g_signal_connect(entry, "activate", G_CALLBACK(args_entry_callback), entry);
        g_signal_connect(entry, "changed", G_CALLBACK(args_entry_callback), entry);

        GtkWidget *args_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_box_pack_start(GTK_BOX(args_box), label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(args_box), entry, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(main_vbox), args_box, FALSE, FALSE, 0);
    }

    /* bottom OK/Cancel part */

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_set_homogeneous(GTK_BOX(button_box), TRUE);
    gtk_box_pack_start(GTK_BOX(main_vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *ok_button = gtk_button_new_with_label("OK");
    gtkwave_signal_connect_object(ok_button,
                                  "clicked",
                                  G_CALLBACK(ok_callback),
                                  GLOBALS->window_ttranslate_c_5);
    gtk_tooltips_set_tip_2(ok_button,
                           "Add selected signals to end of the display on the main window.");

    gtk_box_pack_start(GTK_BOX(button_box), ok_button, TRUE, TRUE, 0);

    GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
    gtkwave_signal_connect_object(cancel_button,
                                  "clicked",
                                  G_CALLBACK(destroy_callback),
                                  GLOBALS->window_ttranslate_c_5);
    gtk_tooltips_set_tip_2(cancel_button, "Do nothing and return to the main window.");
    gtk_box_pack_start(GTK_BOX(button_box), cancel_button, TRUE, TRUE, 0);

    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->window_ttranslate_c_5), 400, 400);
    gtk_widget_show_all(GLOBALS->window_ttranslate_c_5);
}

/*
 * currently only called by parsewavline
 */
void set_current_translate_ttrans(char *name)
{
    int i;

    for (i = 1; i < GLOBALS->num_ttrans_filters + 1; i++) {
        if (!strcmp(GLOBALS->ttranssel_filter[i], name)) {
            GLOBALS->current_translate_ttrans = i;
            return;
        }
    }

    if (GLOBALS->num_ttrans_filters < TTRANS_FILTER_MAX) {
        GLOBALS->num_ttrans_filters++;
        load_ttrans_filter(GLOBALS->num_ttrans_filters, name);
        if (!GLOBALS->ttrans_filter[GLOBALS->num_ttrans_filters]) {
            GLOBALS->num_ttrans_filters--;
            GLOBALS->current_translate_ttrans = 0;
        } else {
            if (GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters])
                free_2(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]);
            GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters] = malloc_2(strlen(name) + 1);
            strcpy(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters], name);
            GLOBALS->current_translate_ttrans = GLOBALS->num_ttrans_filters;
        }
    }
}

int traverse_vector_nodes(GwTrace *t)
{
    int i;
    int cvt_ok = 0;

    if ((t->t_filter) && (t->flags & TR_TTRANSLATED) && (t->vector) && (!t->t_filter_converted)) {
#if !defined __MINGW32__
        int rc = save_nodes_to_trans(GLOBALS->ttrans_filter[t->t_filter]->sout, t);
#else
        int rc =
            save_nodes_to_trans((FILE *)(GLOBALS->ttrans_filter[t->t_filter]->g_hChildStd_IN_Wr),
                                t);
#endif

        if (rc == VCDSAV_OK) {
            int is_finish = 0;
            GwBitVector *prev_transaction_trace = NULL;

            while (!is_finish) {
                GwVectorEnt *vt_head = NULL;
                GwVectorEnt *vt_curr = NULL;
                GwVectorEnt *vt;
                GwVectorEnt *vprev;
                GwBitVector *bv;
                int regions = 2;
                GwTime prev_tim = GW_TIME_CONSTANT(-1);
                char *trace_name = NULL;
                char *orig_name = t->n.vec->bvname;

                cvt_ok = 1;

                vt_head = vt_curr = vt = calloc_2(1, sizeof(GwVectorEnt) + 1);
                vt->time = GW_TIME_CONSTANT(-2);
                vprev = vt; /* for duplicate removal */

                vt_curr = vt_curr->next = vt = calloc_2(1, sizeof(GwVectorEnt) + 1);
                vt->time = GW_TIME_CONSTANT(-1);

                for (;;) {
                    char buf[1025];
                    char *pnt, *rtn;

#if !defined __MINGW32__
                    if (feof(GLOBALS->ttrans_filter[t->t_filter]->sin))
                        break; /* should never happen */

                    buf[0] = 0;
                    pnt = fgets(buf, 1024, GLOBALS->ttrans_filter[t->t_filter]->sin);
                    if (!pnt)
                        break;
                    rtn = pnt;
                    while (*rtn) {
                        if ((*rtn == '\n') || (*rtn == '\r')) {
                            *rtn = 0;
                            break;
                        }
                        rtn++;
                    }
#else
                    {
                        BOOL bSuccess;
                        DWORD dwRead;
                        int n;

                        for (n = 0; n < 1024; n++) {
                            do {
                                bSuccess = ReadFile(
                                    GLOBALS->ttrans_filter[t->t_filter]->g_hChildStd_OUT_Rd,
                                    buf + n,
                                    1,
                                    &dwRead,
                                    NULL);
                                if ((!bSuccess) || (buf[n] == '\n')) {
                                    goto ex;
                                }

                            } while (buf[n] == '\r');
                        }
                    ex:
                        buf[n] = 0;
                        pnt = buf;
                    }
#endif

                    while (*pnt) {
                        if (isspace((int)(unsigned char)*pnt))
                            pnt++;
                        else
                            break;
                    }

                    if (*pnt == '#') {
                        GwTime tim = atoi_64(pnt + 1) * GLOBALS->time_scale;
                        int slen;
                        char *sp;

                        while (*pnt) {
                            if (!isspace((int)(unsigned char)*pnt))
                                pnt++;
                            else
                                break;
                        }
                        while (*pnt) {
                            if (isspace((int)(unsigned char)*pnt))
                                pnt++;
                            else
                                break;
                        }

                        sp = pnt;
                        slen = strlen(sp);

                        if (slen) {
                            pnt = sp + slen - 1;
                            do {
                                if (isspace((int)(unsigned char)*pnt)) {
                                    *pnt = 0;
                                    pnt--;
                                    slen--;
                                } else {
                                    break;
                                }
                            } while (pnt != (sp - 1));
                        }

                        vt = calloc_2(1, sizeof(GwVectorEnt) + slen + 1);
                        if (sp)
                            strcpy((char *)vt->v, sp);

                        if (tim > prev_tim) {
                            prev_tim = vt->time = tim;
                            vt_curr->next = vt;
                            vt_curr = vt;
                            vprev = vprev->next; /* bump forward the -2 node pointer */
                            regions++;
                        } else if (tim == prev_tim) {
                            vt->time = prev_tim;
                            free_2(vt_curr);
                            vt_curr = vprev->next = vt; /* splice new one in -1 node place */
                        } else {
                            free_2(vt); /* throw it away */
                        }

                        continue;
                    } else if ((*pnt == 'M') || (*pnt == 'm')) {
                        int mlen;
                        pnt++;

                        mlen = bijective_marker_id_string_len(pnt);
                        if (mlen) {
                            int which_marker = bijective_marker_id_string_hash(pnt);

                            GwNamedMarkers *markers =
                                gw_project_get_named_markers(GLOBALS->project);
                            GwMarker *marker = gw_named_markers_get(markers, which_marker);

                            if (marker != NULL) {
                                GwTime tim = atoi_64(pnt + mlen) * GLOBALS->time_scale;
                                int slen;
                                char *sp;

                                if (tim < GW_TIME_CONSTANT(0))
                                    tim = GW_TIME_CONSTANT(-1);

                                gw_marker_set_position(marker, tim);
                                gw_marker_set_enabled(marker, tim >= 0);

                                while (*pnt) {
                                    if (!isspace((int)(unsigned char)*pnt))
                                        pnt++;
                                    else
                                        break;
                                }
                                while (*pnt) {
                                    if (isspace((int)(unsigned char)*pnt))
                                        pnt++;
                                    else
                                        break;
                                }

                                sp = pnt;
                                slen = strlen(sp);

                                if (slen) {
                                    pnt = sp + slen - 1;
                                    do {
                                        if (isspace((int)(unsigned char)*pnt)) {
                                            *pnt = 0;
                                            pnt--;
                                            slen--;
                                        } else {
                                            break;
                                        }
                                    } while (pnt != (sp - 1));
                                }

                                gw_marker_set_alias(marker, sp);
                            }
                        }

                        continue;
                    } else if (*pnt == '$') {
                        if (!strncmp(pnt + 1, "finish", 6)) {
                            is_finish = 1;
                            break;
                        } else if (!strncmp(pnt + 1, "next", 4)) {
                            break;
                        } else if (!strncmp(pnt + 1, "name", 4)) {
                            int slen;
                            char *sp;

                            pnt += 5;
                            while (*pnt) {
                                if (isspace((int)(unsigned char)*pnt))
                                    pnt++;
                                else
                                    break;
                            }

                            sp = pnt;
                            slen = strlen(sp);

                            if (slen) {
                                pnt = sp + slen - 1;
                                do {
                                    if (isspace((int)(unsigned char)*pnt)) {
                                        *pnt = 0;
                                        pnt--;
                                        slen--;
                                    } else {
                                        break;
                                    }
                                } while (pnt != (sp - 1));
                            }

                            if (sp && *sp) {
                                if (trace_name)
                                    free_2(trace_name);
                                trace_name = strdup_2(sp);
                            }
                        }
                    }
                }

                vt_curr = vt_curr->next = vt = calloc_2(1, sizeof(GwVectorEnt) + 1);
                vt->time = MAX_HISTENT_TIME - 1;
                regions++;

                /* vt_curr = */ vt_curr->next = vt =
                    calloc_2(1, sizeof(GwVectorEnt) + 1); /* scan-build */
                vt->time = MAX_HISTENT_TIME;
                regions++;

                bv = calloc_2(1, sizeof(GwBitVector) + (sizeof(GwVectorEnt *) * (regions)));
                bv->bvname = strdup_2(trace_name ? trace_name : orig_name);
                bv->nbits = 1;
                bv->numregions = regions;
                bv->bits = t->n.vec->bits;

                vt = vt_head;
                for (i = 0; i < regions; i++) {
                    bv->vectors[i] = vt;
                    vt = vt->next;
                }

                if (!prev_transaction_trace) {
                    prev_transaction_trace = bv;
                    bv->transaction_cache = t->n.vec; /* for possible restore later */
                    t->n.vec = bv;

                    t->t_filter_converted = 1;

                    if (trace_name) /* if NULL, no need to regen display as trace name didn't change
                                     */
                    {
                        t->name = t->n.vec->bvname;
                        if (GLOBALS->hier_max_level)
                            t->name = hier_extract(t->name, GLOBALS->hier_max_level);

                        GLOBALS->signalwindow_width_dirty = 1;
                        redraw_signals_and_waves();
                    }
                } else {
                    prev_transaction_trace->transaction_chain = bv;
                    prev_transaction_trace = bv;
                }
            }
        } else {
            /* failed */
            t->flags &= ~(TR_TTRANSLATED | TR_ANALOGMASK);
        }
    }

    return (cvt_ok);
}
