/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */
#include "gtk/gtkcssprovider.h"
#include "gtk23compat.h"
#if defined _AIX
#pragma alloca
#endif

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __MINGW32__
#include <windows.h>
#else
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gtk/gtkx.h>
#endif
#endif

#include "wave_locale.h"

#if !defined __MINGW32__
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#if !defined __MINGW32__
#define WAVE_USE_XID
#else
#undef WAVE_USE_XID
#endif

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include <getopt.h>
#include <unistd.h>
#endif

#include "symbol.h"
#include "main.h"
#include "menu.h"
#include "vcd.h"
#include "lx2.h"
#include "pixmaps.h"
#include "currenttime.h"
#include "fgetdynamic.h"
#include "rc.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "signal_list.h"
#include "dump_file_main.h"
#include "gw-time-display.h"
#include "gw-vcd-file.h"
#include "gw-fst-file.h"

#include "tcl_helper.h"
#if defined(HAVE_LIBTCL)
#include <tcl.h>
#include <tk.h>
#endif

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

char *gtkwave_argv0_cached = NULL;

static void switch_page(GtkNotebook *notebook, gpointer *page, guint page_num, gpointer user_data)
{
    (void)notebook;
    (void)page;
    (void)user_data;

    char timestr[32];
    struct Global *g_old = GLOBALS;

    set_GLOBALS((*GLOBALS->contexts)[page_num]);

    GLOBALS->autoname_bundles = g_old->autoname_bundles;
    GLOBALS->autocoalesce_reversal = g_old->autocoalesce_reversal;
    GLOBALS->autocoalesce = g_old->autocoalesce;
    GLOBALS->wave_scrolling = g_old->wave_scrolling;
    GLOBALS->constant_marker_update = g_old->constant_marker_update;
    GLOBALS->do_zoom_center = g_old->do_zoom_center;
    GLOBALS->use_roundcaps = g_old->use_roundcaps;
    GLOBALS->do_resize_signals = g_old->do_resize_signals;
    GLOBALS->initial_signal_window_width = g_old->initial_signal_window_width;
    GLOBALS->scale_to_time_dimension = g_old->scale_to_time_dimension;
    GLOBALS->use_full_precision = g_old->use_full_precision;
    GLOBALS->show_base = g_old->show_base;
    GLOBALS->display_grid = g_old->display_grid;
    GLOBALS->fullscreen = g_old->fullscreen;
    GLOBALS->show_toolbar = g_old->show_toolbar;
    GLOBALS->time_box = g_old->time_box;
    GLOBALS->highlight_wavewindow = g_old->highlight_wavewindow;
    GLOBALS->fill_waveform = g_old->fill_waveform;
    GLOBALS->lz_removal = g_old->lz_removal;
    GLOBALS->disable_mouseover = g_old->disable_mouseover;
    GLOBALS->clipboard_mouseover = g_old->clipboard_mouseover;
    GLOBALS->keep_xz_colors = g_old->keep_xz_colors;
    GLOBALS->zoom_pow10_snap = g_old->zoom_pow10_snap;
    GLOBALS->hier_ignore_escapes = g_old->hier_ignore_escapes;
    GLOBALS->sst_dbl_action_type = g_old->sst_dbl_action_type;
    GLOBALS->use_gestures = g_old->use_gestures;
    GLOBALS->use_dark = g_old->use_dark;
    GLOBALS->save_on_exit = g_old->save_on_exit;
    GLOBALS->dbl_mant_dig_override = g_old->dbl_mant_dig_override;

    GwTime global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);
    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);

    reformat_time(timestr, GLOBALS->tims.first + global_time_offset, time_dimension);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry), timestr);
    reformat_time(timestr, GLOBALS->tims.last + global_time_offset, time_dimension);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry), timestr);

    update_time_box();

    if (GLOBALS->second_page_created) {
        wave_gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow),
                                  GLOBALS->winname,
                                  GLOBALS->dumpfile_is_modified ? WAVE_SET_TITLE_MODIFIED
                                                                : WAVE_SET_TITLE_NONE,
                                  0);

        redraw_signals_and_waves();
    }
}

void kill_stems_browser_single(void *V)
{
    struct Global *G = (struct Global *)V;
    if (G && G->anno_ctx) {
#ifdef __MINGW32__
        if (G->anno_ctx->browser_process) {
            TerminateProcess(G->anno_ctx->browser_process, 0);
            CloseHandle(G->anno_ctx->browser_process);
            G->anno_ctx->browser_process = 0;
        }
#else
        if (G->anno_ctx->browser_process) {
#ifdef __CYGWIN__
            G->anno_ctx->cygwin_remote_kill = 1; /* let cygwin child exit() on its own */
#else
            kill(G->anno_ctx->browser_process, SIGKILL);
#endif
            G->anno_ctx->browser_process = (pid_t)0;
        }
#endif
        G->anno_ctx = NULL;
    }
}

void kill_stems_browser(void)
{
    unsigned int ix;

    for (ix = 0; ix < GLOBALS->num_notebook_pages; ix++) {
        struct Global *G = (*GLOBALS->contexts)[ix];
        kill_stems_browser_single(G);
    }
}

#ifdef WAVE_USE_XID
static int plug_destroy(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    exit(0);

    return (FALSE);
}
#endif

#if defined __MINGW32__
static void close_all_fst_files(void) /* so mingw does delete of reader tempfiles */
{
    unsigned int i;
    for (i = 0; i < GLOBALS->num_notebook_pages; i++) {
        if ((*GLOBALS->contexts)[i]->fst_fst_c_1) {
            fstReaderClose((*GLOBALS->contexts)[i]->fst_fst_c_1);
            (*GLOBALS->contexts)[i]->fst_fst_c_1 = NULL;
        }
    }
}
#endif

void wave_gtk_window_set_title(GtkWindow *window, const gchar *title, int typ, int pct)
{
    if (window && title) {
        switch (typ) {
            case WAVE_SET_TITLE_MODIFIED: {
                const char *pfx = "[Modified] ";
                char *t = g_alloca(strlen(pfx) + strlen(title) + 1);

                strcpy(t, pfx);
                strcat(t, title);
                gtk_window_set_title(window, t);
            } break;

            case WAVE_SET_TITLE_LOADING: {
                char *t = g_alloca(64 + strlen(title) + 1); /* make extra long */

                sprintf(t, "[Loading %d%%] %s", pct, title);
                gtk_window_set_title(window, t);
            } break;

            case WAVE_SET_TITLE_NONE:
            default:
                gtk_window_set_title(window, title);
                break;
        }
    }
}

static void print_help(char *nam)
{
#if !defined __MINGW32__ && !defined __FreeBSD__ && !defined __CYGWIN__
#define WAVE_GETOPT_CPUS \
    "  -c, --cpu=NUMCPUS          specify number of CPUs for parallelizable ops\n"
#else
#define WAVE_GETOPT_CPUS
#endif

#if !defined __MINGW32__
#define VCD_GETOPT "  -o, --optimize             optimize VCD to FST\n"
#else
#define VCD_GETOPT
#endif

#define STEMS_GETOPT "  -t, --stems=FILE           specify stems file for source code annotation\n"
#define DUAL_GETOPT "  -D, --dualid=WHICH         specify multisession identifier\n"

#ifdef WAVE_USE_XID
#define XID_GETOPT "  -X, --xid=XID              specify XID of window for GtkPlug to connect to\n"
#else
#define XID_GETOPT
#endif

#if defined(WIN32) && defined(USE_TCL_STUBS)
#define WISH_GETOPT
#else
#define WISH_GETOPT \
    "  -T, --tcl_init=FILE        specify Tcl command script file to be loaded on startup\n" \
    "  -W, --wish                 enable Tcl command line on stdio\n"
#endif

#if defined(HAVE_LIBTCL)
#define REPSCRIPT_GETOPT \
    WISH_GETOPT \
    "  -R, --repscript=FILE       specify timer-driven Tcl command script file\n" \
    "  -P, --repperiod=VALUE      specify repscript period in msec (default: 500)\n"
#else
#define REPSCRIPT_GETOPT
#endif

#if !defined __MINGW32__
#define OUTPUT_GETOPT "  -O, --output=FILE          specify filename for stdout/stderr redirect\n"
#define CHDIR_GETOPT "  -2, --chdir=DIR            specify new current working directory\n"
#else
#define OUTPUT_GETOPT
#define CHDIR_GETOPT
#endif

#if defined(WAVE_HAVE_GCONF) || defined(WAVE_HAVE_GSETTINGS)
#define RPC_GETOPT "  -1, --rpcid=RPCID          specify RPCID of GConf session\n"
#if defined(WAVE_HAVE_GCONF)
#define RPC_GETOPT3 "  -3, --restore              restore previous RPCID numbered session\n"
#else
#define RPC_GETOPT3 "  -3, --restore              restore previous session\n"
#endif
#else
#define RPC_GETOPT
#define RPC_GETOPT3
#endif

    printf(
        "Usage: %s [OPTION]... [DUMPFILE] [SAVEFILE] [RCFILE]\n\n"
        "  -n, --nocli=DIRPATH        use file requester for dumpfile name\n"
        "  -f, --dump=FILE            specify dumpfile name\n" VCD_GETOPT
        "  -a, --save=FILE            specify savefile name\n"
        "  -r, --rcfile=FILE          specify override .rcfile name\n"
        "  -d, --defaultskip          if missing .rcfile, do not use useful defaults\n" DUAL_GETOPT
        "  -l, --logfile=FILE         specify simulation logfile name for time values\n"
        "  -s, --start=TIME           specify start time for FST skip\n"
        "  -e, --end=TIME             specify end time for for FST skip\n" STEMS_GETOPT
            WAVE_GETOPT_CPUS
        "  -N, --nowm                 disable window manager for most windows\n"
        "  -M, --nomenus              do not render menubar (for making applets)\n"
        "  -S, --script=FILE          specify Tcl command script file for "
        "execution\n" REPSCRIPT_GETOPT XID_GETOPT RPC_GETOPT CHDIR_GETOPT RPC_GETOPT3
        "  -4, --rcvar                specify single rc variable values individually\n"
        "  -5, --sstexclude           specify sst exclusion filter filename\n"
        "  -6, --dark                 set gtk-application-prefer-dark-theme = TRUE\n"
        "  -7, --saveonexit           prompt user to write save file at exit\n"
        "  -g, --giga                 use gigabyte mempacking when recoding (slower)\n"
        "  -v, --vcd                  use stdin as a VCD dumpfile\n" OUTPUT_GETOPT
        "  -V, --version              display version banner then exit\n"
        "  -h, --help                 display this help then exit\n"
        "  -x, --exit                 exit after loading trace (for loader benchmarks)\n\n"

        "VCD files and save files may be compressed with zip or gzip.\n"
        "GHW files may be compressed with gzip or bzip2.\n"
        "Other formats must remain uncompressed due to their non-linear access.\n"
        "Note that DUMPFILE is optional if the --dump or --nocli options are specified.\n"
        "SAVEFILE and RCFILE are always optional.\n\n"

        "Report bugs to <" PACKAGE_BUGREPORT ">.\n",
        nam);

#ifdef __MINGW32__
    fflush(stdout); /* fix for possible problem with mingw/msys shells */
#endif

    exit(0);
}

/*
 * file selection for -n/--nocli flag
 */

static void wave_get_filename_cleanup(GtkWidget *widget, gpointer data)
{
    (void)widget;
    (void)data;

    gtk_main_quit(); /* do nothing but exit gtk loop */
}

static char *wave_get_filename(char *dfile)
{
    if (dfile) {
        int len = strlen(dfile);
        GLOBALS->ftext_main_main_c_1 = malloc_2(strlen(dfile) + 2);
        strcpy(GLOBALS->ftext_main_main_c_1, dfile);
#if !defined __MINGW32__
        if ((len) && (dfile[len - 1] != '/')) {
            strcpy(GLOBALS->ftext_main_main_c_1 + len, "/");
        }
#else
        if ((len) && (dfile[len - 1] != '\\')) {
            strcpy(GLOBALS->ftext_main_main_c_1 + len, "\\");
        }
#endif
    }
    fileselbox_old("GTKWave: Select a dumpfile...",
                   &GLOBALS->ftext_main_main_c_1,
                   G_CALLBACK(wave_get_filename_cleanup),
                   G_CALLBACK(wave_get_filename_cleanup),
                   NULL,
                   0);
    gtk_main();

    return (GLOBALS->ftext_main_main_c_1);
}

/*
 * Modify the name of the executable (argv[0]) handed to Tk_MainEx;
 * The new executable name has _[pid] appended. This gives a unique
 * (and known) name to the interpreter (for use with send).
 */
void addPidToExecutableName(int argc, char *argv[], char *argv_mod[])
{
    char *pos;
    char *buffer;

    int i;
    for (i = 0; i < argc; i++) {
        argv_mod[i] = argv[i];
    }

    buffer = malloc_2(strlen(argv[0]) + 1 + 10);
    pos = buffer;
    strcpy(pos, argv[0]);
    pos = buffer + strlen(buffer);
    strcpy(pos, "_");
    pos = buffer + strlen(buffer);
    sprintf(pos, "%d", getpid());

    argv_mod[0] = buffer;
}

static void window_setup_dnd(GtkWidget *window)
{
    GtkTargetEntry targets[1];

    gchar *uri_list = g_strdup("text/uri-list");

    targets[0].target = uri_list;
    targets[0].flags = 0;
    targets[0].info = 0;

    gtk_drag_dest_set(window,
                      GTK_DEST_DEFAULT_ALL,
                      targets,
                      G_N_ELEMENTS(targets),
                      GDK_ACTION_COPY);

    g_free(uri_list);
}

static void window_drag_data_received(GtkWidget *widget,
                                      GdkDragContext *context,
                                      gint x,
                                      gint y,
                                      GtkSelectionData *selection_data,
                                      guint info,
                                      guint time_)
{
    (void)widget;
    (void)context;
    (void)x;
    (void)y;
    (void)info;
    (void)time_;

    if (gtk_selection_data_get_length(selection_data) > 0) {
        const gchar *uris = (const gchar *)gtk_selection_data_get_data(selection_data);

        gchar *uris_copy = g_strndup(uris, gtk_selection_data_get_length(selection_data));

        if (process_url_list(uris_copy) != 0) {
            redraw_signals_and_waves();
        }

        g_free(uris_copy);
    }
}

static gboolean window_key_press_event(GtkWidget *widget, GdkEventKey *event)
{
    GtkWindow *window = GTK_WINDOW(widget);
    gboolean handled = FALSE;

    if (!handled)
        handled = gtk_window_propagate_key_event(window, event);

    if (!handled)
        handled = gtk_window_activate_key(window, event);

    if (!handled) {
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(g_type_class_peek(GTK_TYPE_WIDGET));
        handled = widget_class->key_press_event(widget, event);
    }

    return TRUE;
}

static GtkWidget *toolbar_append_button(GtkWidget *toolbar,
                                        const gchar *stock_id,
                                        const char *tooltip_text,
                                        GCallback callback,
                                        gpointer user_data)
{
    GtkWidget *icon_widget = gtk_image_new_from_icon_name(stock_id, GTK_ICON_SIZE_BUTTON);
    GtkToolItem *button = gtk_tool_button_new(icon_widget, NULL);
    gtk_tool_item_set_tooltip_text(button, tooltip_text);

    g_signal_connect(button, "clicked", G_CALLBACK(callback), user_data);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);

    return GTK_WIDGET(button);
}

static GtkToolItem *toolbar_append_separator(GtkWidget *toolbar)
{
    GtkToolItem *separator = gtk_separator_tool_item_new();
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(separator), TRUE);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), separator, -1);

    return separator;
}

static void toolbar_append_widget(GtkWidget *toolbar, GtkWidget *widget)
{
    GtkToolItem *item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(item), widget);

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
}

static GtkWidget *build_toolbar(void)
{
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);

    toolbar_append_button(toolbar, "edit-cut", "Cut Traces", G_CALLBACK(menu_cut_traces), NULL);
    toolbar_append_button(toolbar, "edit-copy", "Copy Traces", G_CALLBACK(menu_copy_traces), NULL);
    toolbar_append_button(toolbar,
                          "edit-paste",
                          "Paste Traces",
                          G_CALLBACK(menu_paste_traces),
                          NULL);

    toolbar_append_separator(toolbar);

    toolbar_append_button(toolbar, "zoom-fit-best", "Zoom Fit", G_CALLBACK(service_zoom_fit), NULL);
    toolbar_append_button(toolbar, "zoom-in", "Zoom In", G_CALLBACK(service_zoom_in), NULL);
    toolbar_append_button(toolbar, "zoom-out", "Zoom Out", G_CALLBACK(service_zoom_out), NULL);
    toolbar_append_button(toolbar, "edit-undo", "Zoom Undo", G_CALLBACK(service_zoom_undo), NULL);
    toolbar_append_button(toolbar,
                          "go-first",
                          "Zoom to Start",
                          G_CALLBACK(service_zoom_left),
                          NULL);
    toolbar_append_button(toolbar, "go-last", "Zoom to End", G_CALLBACK(service_zoom_right), NULL);

    toolbar_append_separator(toolbar);

    toolbar_append_button(toolbar,
                          "go-previous",
                          "Find Previous Edge",
                          G_CALLBACK(service_left_edge),
                          NULL);
    toolbar_append_button(toolbar,
                          "go-next",
                          "Find Next Edge",
                          G_CALLBACK(service_right_edge),
                          NULL);

    toolbar_append_separator(toolbar);

    toolbar_append_widget(toolbar, create_entry_box());

    GtkToolItem *last_separator = toolbar_append_separator(toolbar);

    if ((GLOBALS->loaded_file_type != DUMPLESS_FILE) && (!GLOBALS->disable_menus)) {
        toolbar_append_button(toolbar,
                              "view-refresh",
                              "Reload",
                              G_CALLBACK(menu_reload_waveform_marshal),
                              NULL);

        last_separator = toolbar_append_separator(toolbar);
    }

    gtk_tool_item_set_expand(last_separator, TRUE);
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(last_separator), FALSE);

    GLOBALS->time_box = gw_time_display_new();
    gw_time_display_set_project(GW_TIME_DISPLAY(GLOBALS->time_box), GLOBALS->project);
    toolbar_append_widget(toolbar, GLOBALS->time_box);

    gtk_widget_show_all(toolbar);

    return toolbar;
}

static void add_custom_css(void)
{
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(css_provider, "/io/github/gtkwave/GTKWave/gtkwave.css");

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
#ifdef EXPERIMENTAL_PLUGIN_SUPPORT
#include <libpeas.h>

static void on_activate(GtkMenuItem *menu_item, gpointer user_data)
{
    GwPlugin *plugin = user_data;
    gw_plugin_activate(plugin, GLOBALS->project);
    g_printerr("activate\n");
}

static void add_plugins_to_menu(GtkWidget *widget)
{
    GwPluginManager *manager = gw_plugin_manager_new();

    GtkWidget *plugins = gtk_menu_item_new_with_label("Plugins");
    gtk_menu_shell_append(GTK_MENU_SHELL(widget), plugins);

    GtkWidget *plugins_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(plugins), plugins_menu);

    PeasEngine *engine = peas_engine_get_default();

    g_printerr("Found %d plugins\n", g_list_model_get_n_items(G_LIST_MODEL(engine)));

    for (guint i = 0; i < g_list_model_get_n_items(G_LIST_MODEL(engine)); i++) {
        PeasPluginInfo *info = g_list_model_get_item(G_LIST_MODEL(engine), i);

        gchar *name = g_strdup_printf("%d - %s", i, peas_plugin_info_get_name(info));

        GwPlugin *plugin = gw_plugin_manager_build(manager, i);

        GtkWidget *menu_item = gtk_menu_item_new_with_label(name);
        gtk_menu_shell_append(GTK_MENU_SHELL(plugins_menu), menu_item);
        g_signal_connect(menu_item, "activate", G_CALLBACK(on_activate), plugin);

        g_free(name);

        g_object_unref(info);
    }

    gtk_widget_show_all(plugins);
}
#endif

static void on_dbus_method_call(GDBusConnection *connection,
                                const gchar *sender,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *method_name,
                                GVariant *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer user_data

)
{
    if (g_strcmp0(method_name, "Reload") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);

        if (in_main_iteration()) {
            return;
        }
        reload_into_new_context();
    }
}

static const GDBusInterfaceVTable DBUS_VTABLE = {
    .method_call = on_dbus_method_call,
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    GDBusNodeInfo *node_info =
        g_dbus_node_info_new_for_xml("<node><interface name='io.github.gtkwave.GTKWave'><method "
                                     "name='Reload' /></interface></node>",
                                     NULL);

    g_dbus_connection_register_object(connection,
                                      "/io/github/gtkwave/GTKWave",
                                      node_info->interfaces[0],
                                      &DBUS_VTABLE,
                                      NULL,
                                      NULL,
                                      NULL);

    g_dbus_node_info_unref(node_info);
}

static void dbus_init(void)
{
    g_bus_own_name(G_BUS_TYPE_SESSION,
                   "io.github.gtkwave.GTKWave",
                   G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE,
                   on_bus_acquired,
                   NULL,
                   NULL,
                   NULL,
                   NULL);
}

int main(int argc, char *argv[])
{
    return (main_2(0, argc, argv));
}

int main_2(int opt_vcd, int argc, char *argv[])
{
    static const char *winprefix = "GTKWave - ";
    static const char *winstd = "GTKWave (stdio) ";
    char *output_name = NULL;
    char *chdir_cache = NULL;

    int magic_word_filetype = G_FT_UNKNOWN;

    int c;
    char is_vcd = 0;
    char is_wish = 0;
    char is_smartsave = 0;
    char is_giga = 0;
    char fast_exit = 0;
    char opt_errors_encountered = 0;
    char is_missing_file = 0;

    char *wname = NULL;
    char *override_rc = NULL;
    char *scriptfile = NULL;
    FILE *wave = NULL;

    GtkWidget *main_vbox = NULL, *top_table = NULL, *whole_table = NULL;
    GtkWidget *menubar;
    GtkWidget *panedwindow;
    int tcl_interpreter_needs_making = 0;
    struct Global *old_g = NULL;

    int splash_disable_rc_override = 0;
    int mainwindow_already_built = 0;

    struct rc_override *rc_override_head = NULL, *rc_override_curr = NULL;

    WAVE_LOCALE_FIX

    /* Initialize the GLOBALS structure for the first time... */

    if (!GLOBALS) {
        set_GLOBALS(initialize_globals());
        mainwindow_already_built = 0;
        tcl_interpreter_needs_making = 1;

        GLOBALS->logfiles = calloc(1, sizeof(void *)); /* calloc is deliberate! */
    } else {
        old_g = GLOBALS;

        set_GLOBALS(initialize_globals());

        GLOBALS->second_page_created = old_g->second_page_created = 1;

        GLOBALS->notebook = old_g->notebook;
        GLOBALS->num_notebook_pages = old_g->num_notebook_pages;
        GLOBALS->num_notebook_pages_cumulative = old_g->num_notebook_pages_cumulative;
        GLOBALS->contexts = old_g->contexts;

        GLOBALS->mainwindow = old_g->mainwindow;
        splash_disable_rc_override = 1;

        /* busy.c */
        GLOBALS->busycursor_busy_c_1 = old_g->busycursor_busy_c_1;

        /* logfiles.c */
        GLOBALS->logfiles = old_g->logfiles;

        /* menu.c */
#if defined(HAVE_LIBTCL)
        GLOBALS->interp = old_g->interp;
#endif
        GLOBALS->vcd_jmp_buf = old_g->vcd_jmp_buf;

        /* timeentry.c */
        GLOBALS->from_entry = old_g->from_entry;
        GLOBALS->to_entry = old_g->to_entry;

        /* rc.c */
        GLOBALS->possibly_use_rc_defaults = old_g->possibly_use_rc_defaults;
        GLOBALS->ignore_savefile_pane_pos = old_g->ignore_savefile_pane_pos;
        GLOBALS->ignore_savefile_pos = old_g->ignore_savefile_pos;
        GLOBALS->ignore_savefile_size = old_g->ignore_savefile_size;

        GLOBALS->color_theme = g_object_ref(old_g->color_theme);

        GLOBALS->autoname_bundles = old_g->autoname_bundles;
        GLOBALS->autocoalesce = old_g->autocoalesce;
        GLOBALS->autocoalesce_reversal = old_g->autocoalesce_reversal;
        GLOBALS->constant_marker_update = old_g->constant_marker_update;
        GLOBALS->convert_to_reals = old_g->convert_to_reals;
        GLOBALS->disable_mouseover = old_g->disable_mouseover;
        GLOBALS->clipboard_mouseover = old_g->clipboard_mouseover;
        GLOBALS->keep_xz_colors = old_g->keep_xz_colors;
        GLOBALS->disable_tooltips = old_g->disable_tooltips;
        GLOBALS->do_initial_zoom_fit = old_g->do_initial_zoom_fit;
        GLOBALS->do_resize_signals = old_g->do_resize_signals;
        GLOBALS->initial_signal_window_width = old_g->initial_signal_window_width;
        GLOBALS->scale_to_time_dimension = old_g->scale_to_time_dimension;
        GLOBALS->enable_fast_exit = old_g->enable_fast_exit;
        GLOBALS->enable_ghost_marker = old_g->enable_ghost_marker;
        GLOBALS->enable_horiz_grid = old_g->enable_horiz_grid;
        GLOBALS->fill_waveform = old_g->fill_waveform;
        GLOBALS->lz_removal = old_g->lz_removal;
        GLOBALS->enable_vert_grid = old_g->enable_vert_grid;
        GLOBALS->sst_expanded = old_g->sst_expanded;
        GLOBALS->hier_max_level = old_g->hier_max_level;
        GLOBALS->hier_max_level_shadow = old_g->hier_max_level_shadow;
        GLOBALS->paned_pack_semantics = old_g->paned_pack_semantics;
        GLOBALS->left_justify_sigs = old_g->left_justify_sigs;
        GLOBALS->ps_maxveclen = old_g->ps_maxveclen;
        GLOBALS->show_base = old_g->show_base;
        GLOBALS->display_grid = old_g->display_grid;
        GLOBALS->fullscreen = old_g->fullscreen;
        GLOBALS->show_toolbar = old_g->show_toolbar;
        GLOBALS->time_box = old_g->time_box;
        GLOBALS->highlight_wavewindow = old_g->highlight_wavewindow;
        GLOBALS->use_big_fonts = old_g->use_big_fonts;
        GLOBALS->use_full_precision = old_g->use_full_precision;
        GLOBALS->use_frequency_delta = old_g->use_frequency_delta;
        GLOBALS->use_maxtime_display = old_g->use_maxtime_display;
        GLOBALS->use_nonprop_fonts = old_g->use_nonprop_fonts;
        GLOBALS->use_roundcaps = old_g->use_roundcaps;
        GLOBALS->vector_padding = old_g->vector_padding;
        GLOBALS->wave_scrolling = old_g->wave_scrolling;
        GLOBALS->do_zoom_center = old_g->do_zoom_center;
        GLOBALS->zoom_pow10_snap = old_g->zoom_pow10_snap;
        GLOBALS->alt_hier_delimeter = old_g->alt_hier_delimeter;
        GLOBALS->cursor_snap = old_g->cursor_snap;
        GLOBALS->hier_delimeter = old_g->hier_delimeter;
        GLOBALS->hier_was_explicitly_set = old_g->hier_was_explicitly_set;
        GLOBALS->page_divisor = old_g->page_divisor;
        GLOBALS->ps_maxveclen = old_g->ps_maxveclen;
        GLOBALS->vector_padding = old_g->vector_padding;
        GLOBALS->zoombase = old_g->zoombase;
        GLOBALS->splash_disable = old_g->splash_disable;
        GLOBALS->use_pango_fonts = old_g->use_pango_fonts;
        GLOBALS->hier_ignore_escapes = old_g->hier_ignore_escapes;

        GLOBALS->ruler_origin = old_g->ruler_origin;
        GLOBALS->ruler_step = old_g->ruler_step;

        GLOBALS->settings = old_g->settings;
        GLOBALS->do_dynamic_treefilter = old_g->do_dynamic_treefilter;
        GLOBALS->dragzoom_threshold = old_g->dragzoom_threshold;

        GLOBALS->missing_file_toolbar = old_g->missing_file_toolbar;

        GLOBALS->analog_redraw_skip_count = old_g->analog_redraw_skip_count;
        GLOBALS->context_tabposition = old_g->context_tabposition;
        GLOBALS->disable_empty_gui = old_g->disable_empty_gui;
        GLOBALS->strace_repeat_count = old_g->strace_repeat_count;

        GLOBALS->sst_dbl_action_type = old_g->sst_dbl_action_type;
        GLOBALS->use_gestures = old_g->use_gestures;
        GLOBALS->use_dark = old_g->use_dark;
        GLOBALS->save_on_exit = old_g->save_on_exit;
        GLOBALS->dbl_mant_dig_override = old_g->dbl_mant_dig_override;

        GLOBALS->cr_line_width = old_g->cr_line_width;
        GLOBALS->cairo_050_offset = old_g->cairo_050_offset;

        strcpy2_into_new_context(GLOBALS,
                                 &GLOBALS->sst_exclude_filename,
                                 &old_g->sst_exclude_filename);

        strcpy2_into_new_context(GLOBALS, &GLOBALS->editor_name, &old_g->editor_name);
        strcpy2_into_new_context(GLOBALS, &GLOBALS->fontname_logfile, &old_g->fontname_logfile);
        strcpy2_into_new_context(GLOBALS, &GLOBALS->fontname_signals, &old_g->fontname_signals);
        strcpy2_into_new_context(GLOBALS, &GLOBALS->fontname_waves, &old_g->fontname_waves);
        strcpy2_into_new_context(GLOBALS, &GLOBALS->argvlist, &old_g->argvlist);

        mainwindow_already_built = 1;
    }

    GLOBALS->whoami = malloc_2(strlen(argv[0]) + 1); /* cache name in case we fork later */
    strcpy(GLOBALS->whoami, argv[0]);

    if (!mainwindow_already_built) {
#ifdef __MINGW32__
        gtk_disable_setlocale();
#endif
        if (!gtk_init_check(&argc, &argv)) {
#if defined(__APPLE__)
#ifndef MAC_INTEGRATION
            if (!getenv("DISPLAY")) {
                fprintf(stderr, "DISPLAY environment variable is not set.  Have you ensured\n");
                fprintf(stderr, "that x11 has been initialized through open-x11, launching\n");
                fprintf(stderr, "gtkwave in an xterm or x11 window, etc?\n\n");
                fprintf(stderr, "Attempting to initialize using DISPLAY=:0.0 value...\n\n");
                setenv("DISPLAY", ":0.0", 0);
                if (gtk_init_check(&argc, &argv)) {
                    goto do_primary_inits;
                }
            }
#endif
#endif
            fprintf(stderr, "Could not initialize GTK!  Is DISPLAY env var/xhost set?\n\n");
            print_help(argv[0]);
        }
    }

#if defined(__APPLE__)
#ifndef MAC_INTEGRATION
do_primary_inits:
#endif
#endif

    if (!mainwindow_already_built) {
        dbus_init();
    }

    if (!gtkwave_argv0_cached)
        gtkwave_argv0_cached = argv[0]; /* for new window option */

    init_filetrans_data(); /* for file translation splay trees */
    init_proctrans_data(); /* for proc translation structs */
    init_ttrans_data(); /* for transaction proc translation structs */

    if (!mainwindow_already_built) {
        atexit(remove_all_proc_filters);
        atexit(remove_all_ttrans_filters);
#if defined __MINGW32__
        atexit(close_all_fst_files);
#endif
    }

    if (mainwindow_already_built) {
        optind = 1;
    } else
        while (1) {
            int option_index = 0;

            static struct option long_options[] = {{"dump", 1, 0, 'f'},
                                                   {"optimize", 0, 0, 'o'},
                                                   {"nocli", 1, 0, 'n'},
                                                   {"save", 1, 0, 'a'},
                                                   {"rcfile", 1, 0, 'r'},
                                                   {"defaultskip", 0, 0, 'd'},
                                                   {"logfile", 1, 0, 'l'},
                                                   {"start", 1, 0, 's'},
                                                   {"end", 1, 0, 'e'},
                                                   {"cpus", 1, 0, 'c'},
                                                   {"stems", 1, 0, 't'},
                                                   {"nowm", 0, 0, 'N'},
                                                   {"script", 1, 0, 'S'},
                                                   {"vcd", 0, 0, 'v'},
                                                   {"version", 0, 0, 'V'},
                                                   {"help", 0, 0, 'h'},
                                                   {"exit", 0, 0, 'x'},
                                                   {"xid", 1, 0, 'X'},
                                                   {"nomenus", 0, 0, 'M'},
                                                   {"dualid", 1, 0, 'D'},
                                                   {"giga", 0, 0, 'g'},
                                                   {"tcl_init", 1, 0, 'T'},
                                                   {"wish", 0, 0, 'W'},
                                                   {"repscript", 1, 0, 'R'},
                                                   {"repperiod", 1, 0, 'P'},
                                                   {"output", 1, 0, 'O'},
                                                   {"slider-zoom", 0, 0, 'z'},
                                                   {"rpcid", 1, 0, '1'},
                                                   {"chdir", 1, 0, '2'},
                                                   {"restore", 0, 0, '3'},
                                                   {"rcvar", 1, 0, '4'},
                                                   {"sstexclude", 1, 0, '5'},
                                                   {"dark", 0, 0, '6'},
                                                   {"saveonexit", 0, 0, '7'},
                                                   {0, 0, 0, 0}};

            c = getopt_long(argc,
                            argv,
                            "zf:Fon:a:r:dl:s:e:c:t:NS:vVhxX:MD:IgCR:P:O:WT:1:2:34:5:67",
                            long_options,
                            &option_index);

            if (c == -1)
                break; /* no more args */

            switch (c) {
                case 'V':
                    printf(WAVE_VERSION_INFO "\n\n"
                                             "This is free software; see the source for copying "
                                             "conditions.  There is NO\n"
                                             "warranty; not even for MERCHANTABILITY or FITNESS "
                                             "FOR A PARTICULAR PURPOSE.\n");
                    exit(0);

                case 'W':
#if defined(HAVE_LIBTCL)
#if defined(WIN32) && defined(USE_TCL_STUBS)
#else
                    is_wish = 1;
#endif
#else
                    fprintf(stderr,
                            "GTKWAVE | Tcl support not compiled into this executable, exiting.\n");
                    exit(255);
#endif
                    break;

                case 'D': {
                    char *s = optarg;
                    char *plus = strchr(s, '+');
                    if ((plus) && (*(plus + 1))) {
                        sscanf(plus + 1, "%x", &GLOBALS->dual_attach_id_main_c_1);
                        if (plus != s) {
                            char p = *(plus - 1);

                            if (p == '0') {
                                GLOBALS->dual_id = 0;
                                break;
                            } else if (p == '1') {
                                GLOBALS->dual_id = 1;
                                break;
                            }
                        }
                    }

                    fprintf(stderr,
                            "Malformed dual session ID.  Must be of form m+nnnnnnnn where m is 0 "
                            "or 1,\n"
                            "and n is a hexadecimal shared memory ID for use with shmat()\n");
                    exit(255);
                } break;

                case 'A':
                    is_smartsave = 1;
                    break;

                case 'v':
                    is_vcd = 1;
                    if (GLOBALS->loaded_file_name)
                        free_2(GLOBALS->loaded_file_name);
                    GLOBALS->loaded_file_name = malloc_2(4 + 1);
                    strcpy(GLOBALS->loaded_file_name, "-vcd");
                    break;

                case 'o':
                    opt_vcd = 1;
                    break;

                case 'n':
                    wave_get_filename(optarg);
                    if (GLOBALS->filesel_ok) {
                        if (GLOBALS->loaded_file_name)
                            free_2(GLOBALS->loaded_file_name);
                        GLOBALS->loaded_file_name = GLOBALS->ftext_main_main_c_1;
                        GLOBALS->ftext_main_main_c_1 = NULL;
                    }
                    break;

                case 'h':
                    print_help(argv[0]);
                    break;

#ifdef WAVE_USE_XID
                case 'X':
                    sscanf(optarg, "%lx", &GLOBALS->socket_xid);
                    splash_disable_rc_override = 1;
                    break;
#endif

                case '1':
                    sscanf(optarg, "%d", &wave_rpc_id);
                    if (wave_rpc_id < 0)
                        wave_rpc_id = 0;
                    break;

                case '2': {
                    char *chdir_env = getenv("GTKWAVE_CHDIR");

                    if (chdir_cache) {
                        free_2(chdir_cache);
                    }

                    chdir_cache = strdup_2(chdir_env ? chdir_env : optarg);
                    if (chdir(chdir_cache) < 0) {
                        fprintf(stderr, "GTKWAVE | Could not chdir '%s', exiting.\n", chdir_cache);
                        perror("Why");
                        exit(255);
                    }
                } break;

                case '3':
#if defined(WAVE_HAVE_GCONF) || defined(WAVE_HAVE_GSETTINGS)
                {
                    is_vcd = 0;
                    wave_gconf_restore(&GLOBALS->loaded_file_name,
                                       &wname,
                                       &override_rc,
                                       &chdir_cache,
                                       &opt_vcd);
                    if (chdir_cache) {
                        if (chdir(chdir_cache) < 0) {
                            fprintf(stderr,
                                    "GTKWAVE | Could not chdir '%s', exiting.\n",
                                    chdir_cache);
                            perror("Why");
                            exit(255);
                        }
                    }
                    fprintf(stderr,
                            "GTKWAVE | restore cwd      '%s'\n",
                            chdir_cache ? chdir_cache : "(none)");
                    fprintf(stderr,
                            "GTKWAVE | restore dumpfile '%s'\n",
                            GLOBALS->loaded_file_name ? GLOBALS->loaded_file_name : "(none)");
                    fprintf(stderr, "GTKWAVE | restore savefile '%s'\n", wname ? wname : "(none)");
                    fprintf(stderr,
                            "GTKWAVE | restore rcfile   '%s'\n",
                            override_rc ? override_rc : "(none)");
                    fprintf(stderr, "GTKWAVE | restore optimize '%s'\n", opt_vcd ? "yes" : "no");
                }
#endif
                break;

                case 'M':
                    GLOBALS->disable_menus = 1;
                    break;

                case 'x':
                    fast_exit = 1;
                    splash_disable_rc_override = 1;
                    break;

                case 'd':
                    GLOBALS->possibly_use_rc_defaults = 0;
                    break;

                case 'f':
                    is_vcd = 0;
                    if (GLOBALS->loaded_file_name)
                        free_2(GLOBALS->loaded_file_name);
                    GLOBALS->loaded_file_name = malloc_2(strlen(optarg) + 1);
                    strcpy(GLOBALS->loaded_file_name, optarg);
                    break;

                case 'a':
                    if (wname)
                        free_2(wname);
                    wname = malloc_2(strlen(optarg) + 1);
                    strcpy(wname, optarg);
                    break;

                case 'r':
                    if (override_rc)
                        free_2(override_rc);
                    override_rc = malloc_2(strlen(optarg) + 1);
                    strcpy(override_rc, optarg);
                    break;

                case '4': {
                    struct rc_override *rco = calloc_2(1, sizeof(struct rc_override));
                    rco->str = strdup_2(optarg);

                    if (rc_override_curr) {
                        rc_override_curr->next = rco;
                        rc_override_curr = rco;
                    } else {
                        rc_override_head = rc_override_curr = rco;
                    }
                } break;

                case '5': {
                    if (GLOBALS->sst_exclude_filename) {
                        free_2(GLOBALS->sst_exclude_filename);
                    }
                    GLOBALS->sst_exclude_filename = strdup_2(optarg);
                } break;

                case '6':
                    GLOBALS->use_dark = TRUE;
                    break;

                case '7':
                    GLOBALS->save_on_exit = TRUE;
                    break;

                case 's':
                    if (GLOBALS->skip_start)
                        free_2(GLOBALS->skip_start);
                    GLOBALS->skip_start = malloc_2(strlen(optarg) + 1);
                    strcpy(GLOBALS->skip_start, optarg);
                    break;

                case 'e':
                    if (GLOBALS->skip_end)
                        free_2(GLOBALS->skip_end);
                    GLOBALS->skip_end = malloc_2(strlen(optarg) + 1);
                    strcpy(GLOBALS->skip_end, optarg);
                    break;

                case 't':
                    if (GLOBALS->stems_name)
                        free_2(GLOBALS->stems_name);
                    GLOBALS->stems_name = malloc_2(strlen(optarg) + 1);
                    strcpy(GLOBALS->stems_name, optarg);
                    break;

                case 'c':
#if !defined __MINGW32__ && !defined __FreeBSD__ && !defined __CYGWIN__
                    GLOBALS->num_cpus = atoi(optarg);
                    if (GLOBALS->num_cpus < 1)
                        GLOBALS->num_cpus = 1;
                    if (GLOBALS->num_cpus > 8)
                        GLOBALS->num_cpus = 8;
#else
                    fprintf(stderr,
                            "GTKWAVE | Warning: '%c' option does not exist in this executable\n",
                            c);
#endif
                    break;

                case 'N':
                    GLOBALS->disable_window_manager = 1;
                    break;

                case 'S':
                    if (scriptfile)
                        free_2(scriptfile);
                    scriptfile = malloc_2(strlen(optarg) + 1);
                    strcpy(scriptfile, optarg);
                    splash_disable_rc_override = 1;
                    break;

                case 'l': {
                    struct logfile_chain *l = calloc_2(1, sizeof(struct logfile_chain));
                    struct logfile_chain *ltraverse;
                    l->name = malloc_2(strlen(optarg) + 1);
                    strcpy(l->name, optarg);

                    if (GLOBALS->logfile) {
                        ltraverse = GLOBALS->logfile;
                        while (ltraverse->next)
                            ltraverse = ltraverse->next;
                        ltraverse->next = l;
                    } else {
                        GLOBALS->logfile = l;
                    }
                } break;

                case 'g':
                    is_giga = 1;
                    break;

                case 'R':
                    if (GLOBALS->repscript_name)
                        free_2(GLOBALS->repscript_name);
                    GLOBALS->repscript_name = malloc_2(strlen(optarg) + 1);
                    strcpy(GLOBALS->repscript_name, optarg);
                    break;

                case 'P': {
                    int pd = atoi(optarg);
                    if (pd > 0) {
                        GLOBALS->repscript_period = pd;
                    }
                } break;

                case 'T':
#if defined(WIN32) && defined(USE_TCL_STUBS)
                    fprintf(stderr,
                            "GTKWAVE | Warning: '%c' option does not exist in this executable\n",
                            c);
#else
                {
                    char *pos;
                    is_wish = 1;
                    if (GLOBALS->tcl_init_cmd) {
                        int length = strlen(GLOBALS->tcl_init_cmd) + 9 + strlen(optarg);
                        char *buffer = malloc_2(strlen(GLOBALS->tcl_init_cmd) + 1);
                        strcpy(buffer, GLOBALS->tcl_init_cmd);
                        free_2(GLOBALS->tcl_init_cmd);
                        GLOBALS->tcl_init_cmd = malloc_2(length + 1);
                        strcpy(GLOBALS->tcl_init_cmd, buffer);
                        pos = GLOBALS->tcl_init_cmd + strlen(GLOBALS->tcl_init_cmd);
                        free_2(buffer);
                    } else {
                        int length = 9 + strlen(optarg);
                        GLOBALS->tcl_init_cmd = malloc_2(length + 1);
                        pos = GLOBALS->tcl_init_cmd;
                    }
                    strcpy(pos, "; source ");
                    pos = GLOBALS->tcl_init_cmd + strlen(GLOBALS->tcl_init_cmd);
                    strcpy(pos, optarg);
                }
#endif
                    break;

                case 'O':
                    if (output_name)
                        free_2(output_name);
                    output_name = malloc_2(strlen(optarg) + 1);
                    strcpy(output_name, optarg);
                    break;

                case '?':
                    opt_errors_encountered = 1;
                    break;

                default:
                    /* unreachable */
                    break;
            }
        } /* ...while(1) */

    if (opt_errors_encountered) {
        print_help(argv[0]);
    }

    if (optind < argc) {
        while (optind < argc) {
            if (argv[optind][0] == '-') {
                if (!strcmp(argv[optind], "--")) {
                    break;
                }
            }

            if (!GLOBALS->loaded_file_name) {
                is_vcd = 0;
                GLOBALS->loaded_file_name = malloc_2(strlen(argv[optind]) + 1);
                strcpy(GLOBALS->loaded_file_name, argv[optind++]);
            } else if (!wname) {
                wname = malloc_2(strlen(argv[optind]) + 1);
                strcpy(wname, argv[optind++]);
            } else if (!override_rc) {
                override_rc = malloc_2(strlen(argv[optind]) + 1);
                strcpy(override_rc, argv[optind++]);
                break; /* skip any extra args */
            }
        }
    }

    if (is_wish && is_vcd) {
        fprintf(stderr,
                "GTKWAVE | Cannot use --vcd and --wish options together as both use stdin,\n"
                "GTKWAVE | exiting!\n");
        exit(255);
    }

    /* attempt to load a dump+save file if only a savefile is specified at the command line */
    if ((GLOBALS->loaded_file_name) && (!wname) &&
        (suffix_check(GLOBALS->loaded_file_name, ".gtkw") ||
         suffix_check(GLOBALS->loaded_file_name, ".sav"))) {
        char *extracted_name = extract_dumpname_from_save_file(GLOBALS->loaded_file_name,
                                                               &GLOBALS->dumpfile_is_modified,
                                                               &opt_vcd);
        if (extracted_name) {
            if (mainwindow_already_built) {
                deal_with_rpc_open_2(GLOBALS->loaded_file_name, NULL, TRUE);
                GLOBALS->loaded_file_name = extracted_name;
                /* wname is still NULL */
            } else {
                wname = GLOBALS->loaded_file_name;
                GLOBALS->loaded_file_name = extracted_name;
            }
        } else {
            char *dfn = NULL;
            char *sfn = NULL;
            off_t dumpsiz = -1;
            time_t dumptim = -1;

            read_save_helper(GLOBALS->loaded_file_name, &dfn, &sfn, &dumpsiz, &dumptim, &opt_vcd);

            fprintf(stderr,
                    "GTKWAVE | Could not initialize '%s' found in '%s', exiting.\n",
                    dfn ? dfn : "(null)",
                    GLOBALS->loaded_file_name);
            if (dfn)
                free_2(dfn);
            if (sfn)
                free_2(sfn);
            exit(255);
        }
    } else /* same as above but with --save specified */
        if ((!GLOBALS->loaded_file_name) && wname) {
            GLOBALS->loaded_file_name =
                extract_dumpname_from_save_file(wname, &GLOBALS->dumpfile_is_modified, &opt_vcd);
            /* still can be NULL if file not found... */
            if (!GLOBALS->loaded_file_name) {
                char *dfn = NULL;
                char *sfn = NULL;
                off_t dumpsiz = -1;
                time_t dumptim = -1;

                read_save_helper(wname, &dfn, &sfn, &dumpsiz, &dumptim, &opt_vcd);

                fprintf(stderr,
                        "GTKWAVE | Could not initialize '%s' found in '%s', exiting.\n",
                        dfn ? dfn : "(null)",
                        wname);
                if (dfn)
                    free_2(dfn);
                if (sfn)
                    free_2(sfn);
                exit(255);
            }
        }

    if (!old_g) /* copy all variables earlier when old_g is set */
    {
        read_rc_file(override_rc);
    }

    GLOBALS->splash_disable |= splash_disable_rc_override;

    if (!GLOBALS->loaded_file_name) {
        /* if rc can gates off gui, default is not to disable */
        if (GLOBALS->disable_empty_gui) {
            print_help(argv[0]);
        }
    }

    if (is_giga) {
        GLOBALS->settings.vlist_prepack = 1;
    }

    if (output_name) {
#if !defined __MINGW32__
        int iarg;
        time_t walltime;
        int fd_replace = open(output_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd_replace < 0) {
            fprintf(stderr, "Could not open redirect file, exiting.\n");
            perror("Why");
            exit(255);
        }

        dup2(fd_replace, 1);
        dup2(fd_replace, 2);

        time(&walltime);
        printf(WAVE_VERSION_INFO "\nDate: %s\n\n", asctime(localtime(&walltime)));

        for (iarg = 0; iarg < argc; iarg++) {
            if (iarg)
                printf("\t");
            printf("%s\n", argv[iarg]);
        }

        printf("\n\n");
        fflush(stdout);

#endif
        free_2(output_name);
        output_name = NULL;
    }

    fprintf(stderr, "\n%s\n\n", WAVE_VERSION_INFO);

    if (!old_g) /* copy all variables earlier when old_g is set */
    {
        while (rc_override_head) {
            int rco_succ;
            char *rco_copy_str = strdup_2(rc_override_head->str);
            rco_succ = insert_rc_variable(rc_override_head->str);
            fprintf(stderr, "RCVAR   | '%s' %s\n", rco_copy_str, rco_succ ? "FOUND" : "NOT FOUND");
            free_2(rco_copy_str);
            rc_override_curr = rc_override_head->next;
            free_2(rc_override_head->str);
            free_2(rc_override_head);
            rc_override_head = rc_override_curr;
        }
    }

    if (!is_wish) {
        if (tcl_interpreter_needs_making) {
            GLOBALS->argvlist = zMergeTclList(argc, (const char **)argv);
            make_tcl_interpreter(argv);
        }
    }

    if (!GLOBALS->loaded_file_name) {
        GLOBALS->loaded_file_name = strdup_2("[no file loaded]");
        is_missing_file = 1;
        if (!is_wish) {
            fprintf(stderr, "GTKWAVE | Use the -h, --help command line flags to display help.\n");
        }
    }

    /* load either the vcd or aet file depending on suffix then mode setting */
    if (is_vcd) {
        GLOBALS->winname = malloc_2(strlen(winstd) + 4 + 1);
        strcpy(GLOBALS->winname, winstd);
    } else {
        GLOBALS->winname = malloc_2(strlen(GLOBALS->loaded_file_name) + strlen(winprefix) + 1);
        strcpy(GLOBALS->winname, winprefix);
    }

    strcat(GLOBALS->winname, GLOBALS->loaded_file_name);
    sst_exclusion_loader();

loader_check_head:

    if (!is_missing_file) {
        magic_word_filetype = determine_gtkwave_filetype(GLOBALS->loaded_file_name);
    }

    if (is_missing_file) {
        GLOBALS->loaded_file_type = MISSING_FILE;
    } else if (suffix_check(GLOBALS->loaded_file_name, ".lxt") ||
               suffix_check(GLOBALS->loaded_file_name, ".lx2") ||
               suffix_check(GLOBALS->loaded_file_name, ".lxt2")) {
        fprintf(
            stderr,
            "GTKWAVE | LXT and LXT2 files are no longer supported by this version of GTKWave.\n");
        vcd_exit(255);
    } else if ((magic_word_filetype == G_FT_FST) ||
               suffix_check(GLOBALS->loaded_file_name, ".fst")) {
        GLOBALS->stems_type = WAVE_ANNO_FST;
        GLOBALS->aet_name = malloc_2(strlen(GLOBALS->loaded_file_name) + 1);
        strcpy(GLOBALS->aet_name, GLOBALS->loaded_file_name);
        GLOBALS->loaded_file_type = FST_FILE;
        GLOBALS->dump_file =
            fst_main(GLOBALS->loaded_file_name, GLOBALS->skip_start, GLOBALS->skip_end);
        if (GLOBALS->dump_file == NULL) {
            fprintf(stderr,
                    "GTKWAVE | Could not initialize '%s'%s.\n",
                    GLOBALS->loaded_file_name,
                    GLOBALS->vcd_jmp_buf ? "" : ", exiting");
            vcd_exit(255);
        }
    } else if (suffix_check(GLOBALS->loaded_file_name, ".vzt")) {
        fprintf(stderr,
                "GTKWAVE | VZT files are no longer supported by this version of GTKWave.\n");
        vcd_exit(255);
    } else if (suffix_check(GLOBALS->loaded_file_name, ".aet") ||
               suffix_check(GLOBALS->loaded_file_name, ".ae2")) {
        fprintf(stderr,
                "GTKWAVE | AET2 files are no longer supported by this version of GTKWave.\n");
        vcd_exit(255);
    } else if (suffix_check(GLOBALS->loaded_file_name, ".ghw") ||
               suffix_check(GLOBALS->loaded_file_name, ".ghw.gz") ||
               suffix_check(GLOBALS->loaded_file_name, ".ghw.bz2")) {
        GLOBALS->loaded_file_type = GHW_FILE;
        GLOBALS->dump_file = ghw_main(GLOBALS->loaded_file_name);
        if (GLOBALS->dump_file == NULL) {
            /* error message printed in ghw_main() */
            vcd_exit(255);
        }
    } else if (strlen(GLOBALS->loaded_file_name) > 4) /* case for .aet? type filenames */
    {
        char sufbuf[5];
        memcpy(sufbuf, GLOBALS->loaded_file_name + strlen(GLOBALS->loaded_file_name) - 5, 4);
        sufbuf[4] = 0;
        if (!strcasecmp(sufbuf, ".aet")) /* strncasecmp() in windows? */
        {
            fprintf(stderr,
                    "GTKWAVE | AET2 files are no longer supported by this version of GTKWave.\n");
            vcd_exit(255);
        } else {
            goto load_vcd;
        }
    } else /* nothing else left so default to "something" */
    {
    load_vcd:
#if !defined __MINGW32__
        if (opt_vcd) {
            GLOBALS->unoptimized_vcd_file_name = calloc_2(1, strlen(GLOBALS->loaded_file_name) + 1);
            strcpy(GLOBALS->unoptimized_vcd_file_name, GLOBALS->loaded_file_name);
            optimize_vcd_file();
            /* is_vcd = 0; */ /* scan-build */
            GLOBALS->optimize_vcd = 1;
            goto loader_check_head;
        }

#endif

        if (strcmp(GLOBALS->loaded_file_name, "-vcd")) {
            GLOBALS->loaded_file_type = VCD_RECODER_FILE;
        } else {
            GLOBALS->loaded_file_type = DUMPLESS_FILE;
        }
        GLOBALS->dump_file = vcd_recoder_main(GLOBALS->loaded_file_name);
    }

    // /* reset/initialize various markers and time values */
    // for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++)
    //     GLOBALS->named_markers[i] = -1; /* reset all named markers */

    GwTimeRange *time_range = gw_dump_file_get_time_range(GLOBALS->dump_file);

    GLOBALS->tims.first = gw_time_range_get_start(time_range);
    GLOBALS->tims.last = gw_time_range_get_end(time_range);
    GLOBALS->tims.start = GLOBALS->tims.first;
    GLOBALS->tims.laststart = GLOBALS->tims.first;
    GLOBALS->tims.end = GLOBALS->tims.last; /* until the configure_event of wavearea */
    GLOBALS->tims.zoom = GLOBALS->tims.prevzoom = 0; /* 1 pixel/ns default */
    gw_marker_set_enabled(gw_project_get_primary_marker(GLOBALS->project), FALSE);
    gw_marker_set_enabled(gw_project_get_baseline_marker(GLOBALS->project), FALSE);
    gw_marker_set_enabled(gw_project_get_ghost_marker(GLOBALS->project), FALSE);

    if (gw_time_range_get_end(time_range) >> DBL_MANT_DIG) {
        fprintf(stderr,
                "GTKWAVE | Warning: max_time bits > DBL_MANT_DIG (%d), GUI may malfunction!\n",
                DBL_MANT_DIG);
        if (!GLOBALS->dbl_mant_dig_override) {
            fprintf(stderr,
                    "GTKWAVE | Exiting, use dbl_mant_dig_override rc var set to 1 to disable.\n");
            exit(255);
        }
    }

    if ((wname) || (is_smartsave)) {
        int wave_is_compressed;
        char *str = NULL;

        GLOBALS->is_gtkw_save_file = (!wname) || suffix_check(wname, ".gtkw");

        if ((!wname) /* && (is_smartsave) */) {
            char *pnt = g_alloca(strlen(GLOBALS->loaded_file_name) + 1);
            char *pnt2;
            strcpy(pnt, GLOBALS->loaded_file_name);

            if ((strlen(pnt) > 2) && (!strcasecmp(pnt + strlen(pnt) - 3, ".gz"))) {
                pnt[strlen(pnt) - 3] = 0x00;
            } else if ((strlen(pnt) > 3) && (!strcasecmp(pnt + strlen(pnt) - 4, ".zip"))) {
                pnt[strlen(pnt) - 4] = 0x00;
            }

            pnt2 = pnt + strlen(pnt);
            if (pnt != pnt2) {
                do {
                    if (*pnt2 == '.') {
                        *pnt2 = 0x00;
                        break;
                    }
                } while (pnt2-- != pnt);
            }

            wname = malloc_2(strlen(pnt) + 6);
            strcpy(wname, pnt);
            strcat(wname, ".gtkw");
        }

        if (((strlen(wname) > 2) && (!strcasecmp(wname + strlen(wname) - 3, ".gz"))) ||
            ((strlen(wname) > 3) && (!strcasecmp(wname + strlen(wname) - 4, ".zip")))) {
            int dlen;
            dlen = strlen(WAVE_DECOMPRESSOR);
            str = g_alloca(strlen(wname) + dlen + 1);
            strcpy(str, WAVE_DECOMPRESSOR);
            strcpy(str + dlen, wname);
            wave = popen(str, "r");
            wave_is_compressed = ~0;
        } else {
            wave = fopen(wname, "rb");
            wave_is_compressed = 0;

            GLOBALS->filesel_writesave =
                malloc_2(strlen(wname) + 1); /* don't handle compressed files */
            strcpy(GLOBALS->filesel_writesave, wname);
        }

        if (!wave) {
            fprintf(stderr, "** WARNING: Error opening save file '%s', skipping.\n", wname);
        } else {
            char *iline;
            int s_ctx_iter;

            WAVE_STRACE_ITERATOR(s_ctx_iter)
            {
                GLOBALS->strace_ctx =
                    &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
                GLOBALS->strace_ctx->shadow_encountered_parsewavline = 0;
            }

            if (GLOBALS->is_lx2) {
                while ((iline = fgetmalloc(wave))) {
                    parsewavline_lx2(iline, NULL, 0);
                    free_2(iline);
                }

                lx2_import_masked();

                if (wave_is_compressed) {
                    pclose(wave);
                    wave = popen(str, "r");
                } else {
                    fclose(wave);
                    wave = fopen(wname, "rb");
                }

                if (!wave) {
                    fprintf(stderr, "** WARNING: Error opening save file '%s', skipping.\n", wname);
                    EnsureGroupsMatch();
                    goto savefile_bail;
                }
            }

            read_save_helper_relative_init(wname);
            GLOBALS->default_flags = TR_RJUSTIFY;
            GLOBALS->default_fpshift = 0;
            GLOBALS->shift_timebase_default_for_add = GW_TIME_CONSTANT(0);
            GLOBALS->strace_current_window = 0; /* in case there are shadow traces */
            GLOBALS->which_t_color = 0;
            while ((iline = fgetmalloc(wave))) {
                parsewavline(iline, NULL, 0);
                GLOBALS->strace_ctx->shadow_encountered_parsewavline |=
                    GLOBALS->strace_ctx->shadow_active;
                free_2(iline);
            }
            GLOBALS->which_t_color = 0;
            GLOBALS->default_flags = TR_RJUSTIFY;
            GLOBALS->default_fpshift = 0;
            GLOBALS->shift_timebase_default_for_add = GW_TIME_CONSTANT(0);

            if (wave_is_compressed)
                pclose(wave);
            else
                fclose(wave);

            EnsureGroupsMatch();

            WAVE_STRACE_ITERATOR(s_ctx_iter)
            {
                GLOBALS->strace_ctx =
                    &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];

                if (GLOBALS->strace_ctx->shadow_encountered_parsewavline) {
                    GLOBALS->strace_ctx->shadow_encountered_parsewavline = 0;

                    if (GLOBALS->strace_ctx->shadow_straces) {
                        GLOBALS->strace_ctx->shadow_active = 1;

                        swap_strace_contexts();
                        strace_maketimetrace(1);
                        swap_strace_contexts();

                        GLOBALS->strace_ctx->shadow_active = 0;
                    }
                }
            }
        }
    }

savefile_bail:
    GLOBALS->current_translate_file = 0;

    if (fast_exit) {
        printf("Exiting early because of --exit request.\n");
        exit(0);
    }

    if ((GLOBALS->loaded_file_type != MISSING_FILE) && (!GLOBALS->zoom_was_explicitly_set) &&
        ((GLOBALS->tims.last - GLOBALS->tims.first) <= 400))
        GLOBALS->do_initial_zoom_fit = 1; /* force zoom on small traces */

    calczoom(GLOBALS->tims.zoom);

    add_custom_css();

    if (!mainwindow_already_built) {
#ifdef WAVE_USE_XID
        if (!GLOBALS->socket_xid)
#endif
        {
            if (GLOBALS->use_dark) {
                g_object_set(gtk_settings_get_default(),
                             "gtk-application-prefer-dark-theme",
                             TRUE,
                             NULL);
            }

            GLOBALS->mainwindow = gtk_window_new(
                GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
            wave_gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow),
                                      GLOBALS->winname,
                                      GLOBALS->dumpfile_is_modified ? WAVE_SET_TITLE_MODIFIED
                                                                    : WAVE_SET_TITLE_NONE,
                                      0);

            if ((GLOBALS->initial_window_width > 0) && (GLOBALS->initial_window_height > 0)) {
                gtk_window_set_default_size(GTK_WINDOW(GLOBALS->mainwindow),
                                            GLOBALS->initial_window_width,
                                            GLOBALS->initial_window_height);
            } else {
                gtk_window_set_default_size(GTK_WINDOW(GLOBALS->mainwindow),
                                            GLOBALS->initial_window_x,
                                            GLOBALS->initial_window_y);
            }

            g_signal_connect(GLOBALS->mainwindow,
                             "delete_event",
                             /* formerly was "destroy" */ G_CALLBACK(file_quit_cmd_callback),
                             NULL);

            window_setup_dnd(GLOBALS->mainwindow);
            g_signal_connect(GLOBALS->mainwindow,
                             "drag-data-received",
                             G_CALLBACK(window_drag_data_received),
                             NULL);

            g_signal_connect(GLOBALS->mainwindow,
                             "key-press-event",
                             G_CALLBACK(window_key_press_event),
                             NULL);

            gtk_widget_show(GLOBALS->mainwindow);
        }
#ifdef WAVE_USE_XID
        else {
#ifdef GDK_WINDOWING_X11
            GLOBALS->mainwindow = gtk_plug_new(GLOBALS->socket_xid);
            gtk_widget_show(GLOBALS->mainwindow);

            g_signal_connect(GLOBALS->mainwindow,
                             "destroy",
                             /* formerly was "destroy" */ G_CALLBACK(plug_destroy),
                             NULL);
#else
            fprintf(stderr, "GTKWAVE | GtkPlug widget is unavailable\n");
            exit(1);
#endif
        }
#endif
    }

    if (!mainwindow_already_built) {
        main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(GLOBALS->mainwindow), main_vbox);
        gtk_widget_show(main_vbox);

        if (!GLOBALS->disable_menus) {
#ifdef WAVE_USE_XID
            if (GLOBALS->socket_xid)
                kill_main_menu_accelerators();
#endif

            menubar = alt_menu_top(GLOBALS->mainwindow);
            gtk_widget_show(menubar);

#ifdef EXPERIMENTAL_PLUGIN_SUPPORT
            add_plugins_to_menu(menubar);
#endif

            {
                gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, TRUE, 0);
            }

#ifdef MAC_INTEGRATION
            {
                GtkosxApplication *theApp = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
                gtk_widget_hide(menubar);
                gtkosx_application_set_menu_bar(theApp, GTK_MENU_SHELL(menubar));
                gtkosx_application_set_use_quartz_accelerators(theApp, TRUE);
                gtkosx_application_ready(theApp);
                if (GLOBALS->loaded_file_type == MISSING_FILE) {
                    gtkosx_application_attention_request(theApp, INFO_REQUEST);
                }

                g_signal_connect(theApp,
                                 "NSApplicationOpenFile",
                                 G_CALLBACK(deal_with_finder_open),
                                 NULL);
                g_signal_connect(theApp,
                                 "NSApplicationBlockTermination",
                                 G_CALLBACK(deal_with_termination),
                                 NULL);
            }
#endif
        }

        whole_table = gtk_grid_new();

        GtkWidget *tb = build_toolbar();
        top_table = tb;

        GLOBALS->missing_file_toolbar = tb;
        if (GLOBALS->loaded_file_type == MISSING_FILE) {
            gtk_widget_set_sensitive(GLOBALS->missing_file_toolbar, FALSE);
        }

    } /* of ...if(mainwindow_already_built) */

    load_all_fonts(); /* must be done before create_signalwindow() */
    GLOBALS->signalwindow = create_signalwindow();

    /* must be created after the signalwindow because it uses the signal_list vadjustment */
    GLOBALS->wavewindow = create_wavewindow();
    gtk_widget_show(GLOBALS->wavewindow);

    redraw_signals_and_waves();

    if (GLOBALS->do_resize_signals) {
        int os;

        if (GLOBALS->initial_signal_window_width > GLOBALS->max_signal_name_pixel_width) {
            os = GLOBALS->initial_signal_window_width;
        } else {
            os = GLOBALS->max_signal_name_pixel_width;
        }

        os = (os < 48) ? 48 : os;
        gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow), os + 30, -1);
    } else {
        if (GLOBALS->initial_signal_window_width) {
            int os;

            os = GLOBALS->initial_signal_window_width;
            os = (os < 48) ? 48 : os;
            gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow), os + 30, -1);
        }
    }

    gtk_widget_show(GLOBALS->signalwindow);

    if (GLOBALS->loaded_file_type != MISSING_FILE) {
        GLOBALS->toppanedwindow = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
        GLOBALS->sstpane = treeboxframe("SST");

        GLOBALS->expanderwindow = gtk_expander_new_with_mnemonic("_SST");
        gtk_expander_set_expanded(GTK_EXPANDER(GLOBALS->expanderwindow),
                                  (GLOBALS->sst_expanded == TRUE));
        if (GLOBALS->toppanedwindow_size_cache) {
            gtk_paned_set_position(GTK_PANED(GLOBALS->toppanedwindow),
                                   GLOBALS->toppanedwindow_size_cache);
            GLOBALS->toppanedwindow_size_cache = 0;
        }
        gtk_container_add(GTK_CONTAINER(GLOBALS->expanderwindow), GLOBALS->sstpane);
        gtk_widget_show(GLOBALS->expanderwindow);
    }

    GLOBALS->panedwindow = panedwindow = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    if (GLOBALS->panedwindow_size_cache) {
        gtk_paned_set_position(GTK_PANED(GLOBALS->panedwindow), GLOBALS->panedwindow_size_cache);
        GLOBALS->panedwindow_size_cache = 0;
    }

#ifdef HAVE_PANED_PACK
    if (GLOBALS->paned_pack_semantics) {
        gtk_paned_pack1(GTK_PANED(panedwindow), GLOBALS->signalwindow, 0, 0);
        gtk_paned_pack2(GTK_PANED(panedwindow), GLOBALS->wavewindow, ~0, 0);
    } else
#endif
    {
        gtk_paned_add1(GTK_PANED(panedwindow), GLOBALS->signalwindow);
        gtk_paned_add2(GTK_PANED(panedwindow), GLOBALS->wavewindow);
    }

    gtk_widget_show(panedwindow);

    if (GLOBALS->dnd_sigview) {
        dnd_setup(GLOBALS->dnd_sigview, FALSE);
    }

    if (GLOBALS->loaded_file_type != MISSING_FILE) {
        gtk_paned_pack1(GTK_PANED(GLOBALS->toppanedwindow), GLOBALS->expanderwindow, 0, 0);
        gtk_paned_pack2(GTK_PANED(GLOBALS->toppanedwindow), panedwindow, ~0, 0);
        gtk_widget_show(GLOBALS->toppanedwindow);
    }

    if (GLOBALS->treeopen_chain_head) {
        struct string_chain_t *t = GLOBALS->treeopen_chain_head;
        struct string_chain_t *t2;
        while (t) {
            if (GLOBALS->treestore_main) {
                force_open_tree_node(t->str, 0, NULL);
            }

            t2 = t->next;
            if (t->str)
                free_2(t->str);
            free_2(t);
            t = t2;
        }

        GLOBALS->treeopen_chain_head = GLOBALS->treeopen_chain_curr = NULL;
    }

    if (!mainwindow_already_built) {
        gtk_widget_show(top_table);
        gtk_grid_attach(GTK_GRID(whole_table), top_table, 0, 0, 16, 1);

        if (!GLOBALS->do_resize_signals) {
            int dri;
            for (dri = 0; dri < 2; dri++) {
                GLOBALS->signalwindow_width_dirty = 1;
                redraw_signals_and_waves();
            }
        }
    }

    if (!GLOBALS->notebook) {
        GLOBALS->num_notebook_pages = 1;
        GLOBALS->this_context_page = 0;
        GLOBALS->contexts = calloc(1, sizeof(struct Global **));
        /* calloc is deliberate! */ /* scan-build */
        *GLOBALS->contexts = calloc(1, sizeof(struct Global *));
        /* calloc is deliberate! */ /* scan-build */
        (*GLOBALS->contexts)[0] = GLOBALS;

        GLOBALS->dead_context = calloc(1, sizeof(struct Global **));
        /* calloc is deliberate! */ /* scan-build */
        *GLOBALS->dead_context = calloc(1, sizeof(struct Global *));
        /* calloc is deliberate! */ /* scan-build */
        *(GLOBALS->dead_context)[0] = NULL;

        GLOBALS->notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(GLOBALS->notebook),
                                 GLOBALS->context_tabposition ? GTK_POS_LEFT : GTK_POS_TOP);

        gtk_widget_show(GLOBALS->notebook);
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(GLOBALS->notebook),
                                   0); /* hide for first time until next tabs */
        gtk_notebook_set_show_border(GTK_NOTEBOOK(GLOBALS->notebook),
                                     0); /* hide for first time until next tabs */
        g_signal_connect(GLOBALS->notebook, "switch-page", G_CALLBACK(switch_page), NULL);
    } else {
        unsigned int ix;

        GLOBALS->this_context_page = GLOBALS->num_notebook_pages;
        GLOBALS->num_notebook_pages++;
        GLOBALS->num_notebook_pages_cumulative++; /* this never decreases, acts as an incrementing
                                                     flipper id for side tabs */
        *GLOBALS->contexts =
            realloc(*GLOBALS->contexts, GLOBALS->num_notebook_pages * sizeof(struct Global *));
        /* realloc is deliberate! */ /* scan-build */
        (*GLOBALS->contexts)[GLOBALS->this_context_page] = GLOBALS;

        for (ix = 0; ix < GLOBALS->num_notebook_pages; ix++) {
            (*GLOBALS->contexts)[ix]->num_notebook_pages = GLOBALS->num_notebook_pages;
            (*GLOBALS->contexts)[ix]->num_notebook_pages_cumulative =
                GLOBALS->num_notebook_pages_cumulative;
            (*GLOBALS->contexts)[ix]->dead_context =
                (*GLOBALS->contexts)[0]
                    ->dead_context; /* mirroring this is OK as page 0 always has value! */
        }

        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(GLOBALS->notebook), ~0); /* then appear */
        gtk_notebook_set_show_border(GTK_NOTEBOOK(GLOBALS->notebook), ~0); /* then appear */
        gtk_notebook_set_scrollable(GTK_NOTEBOOK(GLOBALS->notebook), ~0);
    }

    if (!GLOBALS->context_tabposition) {
        gtk_notebook_append_page(GTK_NOTEBOOK(GLOBALS->notebook),
                                 GLOBALS->toppanedwindow ? GLOBALS->toppanedwindow : panedwindow,
                                 gtk_label_new(GLOBALS->loaded_file_name));
    } else {
        char buf[40];

        sprintf(buf, "%d", GLOBALS->num_notebook_pages_cumulative);

        gtk_notebook_append_page(GTK_NOTEBOOK(GLOBALS->notebook),
                                 GLOBALS->toppanedwindow ? GLOBALS->toppanedwindow : panedwindow,
                                 gtk_label_new(buf));
    }

    if (mainwindow_already_built) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(GLOBALS->notebook), GLOBALS->this_context_page);
        return (0);
    }

    gtk_grid_attach(GTK_GRID(whole_table), GLOBALS->notebook, 0, 1, 16, 255);
    gtk_widget_set_hexpand(GLOBALS->notebook, TRUE);
    gtk_widget_set_vexpand(GLOBALS->notebook, TRUE);

    gtk_widget_show(whole_table);

    gtk_box_pack_end(GTK_BOX(main_vbox),
                     whole_table,
                     TRUE,
                     TRUE,
                     0); /* prevents shrinkage of signal/waves windows if no waves loaded */

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (gw_marker_is_enabled(primary_marker)) {
        if (gw_marker_get_position(primary_marker) < GLOBALS->tims.first) {
            gw_marker_set_position(primary_marker, GLOBALS->tims.first);
        }
    }
    update_time_box();

    set_window_xypos(GLOBALS->initial_window_xpos, GLOBALS->initial_window_ypos);
    GLOBALS->xy_ignore_main_c_1 = 1;

    if (GLOBALS->logfile) {
        struct logfile_chain *lprev;
        char buf[50];
        int which = 1;
        while (GLOBALS->logfile) {
            sprintf(buf, "Logfile viewer [%d]", which++);
            logbox(buf, 480, GLOBALS->logfile->name);
            lprev = GLOBALS->logfile;
            GLOBALS->logfile = GLOBALS->logfile->next;
            free_2(lprev->name);
            free_2(lprev);
        }
    }

    activate_stems_reader(GLOBALS->stems_name);

    gtk_events_pending_gtk_main_iteration();

    if (1) /* here in order to calculate window manager delta if present... window is completely
              rendered by here */
    {
        int dummy_x, dummy_y;
        get_window_xypos(&dummy_x, &dummy_y);
    }

    init_busy();

    if (scriptfile
#if defined(HAVE_LIBTCL)
        && GLOBALS->interp
#endif
    ) {
        execute_script(scriptfile, 1); /* deallocate the name in the script because context might
                                          swap out from under us! */
        scriptfile = NULL;
    }

#if defined(WAVE_HAVE_GCONF) || defined(WAVE_HAVE_GSETTINGS)
    if (GLOBALS->loaded_file_type != MISSING_FILE) {
        if (!chdir_cache) {
            wave_gconf_client_set_string("/current/pwd", getenv("PWD"));
        }
        wave_gconf_client_set_string("/current/dumpfile",
                                     GLOBALS->optimize_vcd ? GLOBALS->unoptimized_vcd_file_name
                                                           : GLOBALS->loaded_file_name);
        wave_gconf_client_set_string("/current/optimized_vcd", GLOBALS->optimize_vcd ? "1" : "0");
        wave_gconf_client_set_string("/current/savefile", GLOBALS->filesel_writesave);
    }
#endif

    if (GLOBALS->dual_attach_id_main_c_1) {
        fprintf(stderr,
                "GTKWAVE | Attaching %08X as dual head session %d\n",
                GLOBALS->dual_attach_id_main_c_1,
                GLOBALS->dual_id);

#ifdef __MINGW32__
        {
            HANDLE hMapFile;
            char mapName[257];

            sprintf(mapName, "twinwave%d", GLOBALS->dual_attach_id_main_c_1);
            hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapName);
            if (hMapFile == NULL) {
                fprintf(stderr,
                        "Could not attach shared memory map name '%s', exiting.\n",
                        mapName);
                exit(255);
            }
            GLOBALS->dual_ctx = MapViewOfFile(hMapFile,
                                              FILE_MAP_ALL_ACCESS,
                                              0,
                                              0,
                                              2 * sizeof(struct gtkwave_dual_ipc_t));
            if (GLOBALS->dual_ctx == NULL) {
                fprintf(stderr, "Could not map view of file '%s', exiting.\n", mapName);
                exit(255);
            }
        }
#else
        GLOBALS->dual_ctx = shmat(GLOBALS->dual_attach_id_main_c_1, NULL, 0);
#endif
        if (GLOBALS->dual_ctx != (void *)-1) {
            if (memcmp(GLOBALS->dual_ctx[GLOBALS->dual_id].matchword, DUAL_MATCHWORD, 4)) {
                fprintf(stderr, "Not a valid shared memory ID for dual head operation, exiting.\n");
                exit(255);
            }

            GLOBALS->dual_ctx[GLOBALS->dual_id].viewer_is_initialized = 1;
            for (;;) {
                GtkAdjustment *hadj;
                GwTime pageinc, gt;
#ifndef __MINGW32__
                struct timeval tv;
#endif
                if (GLOBALS->dual_ctx[1 - GLOBALS->dual_id].use_new_times) {
                    GLOBALS->dual_race_lock = 1;

                    gt = GLOBALS->dual_ctx[GLOBALS->dual_id].left_margin_time =
                        GLOBALS->dual_ctx[1 - GLOBALS->dual_id].left_margin_time;

                    GLOBALS->dual_ctx[GLOBALS->dual_id].marker =
                        GLOBALS->dual_ctx[1 - GLOBALS->dual_id].marker;
                    GLOBALS->dual_ctx[GLOBALS->dual_id].baseline =
                        GLOBALS->dual_ctx[1 - GLOBALS->dual_id].baseline;
                    GLOBALS->dual_ctx[GLOBALS->dual_id].zoom =
                        GLOBALS->dual_ctx[1 - GLOBALS->dual_id].zoom;
                    GLOBALS->dual_ctx[1 - GLOBALS->dual_id].use_new_times = 0;
                    GLOBALS->dual_ctx[GLOBALS->dual_id].use_new_times = 0;

                    GwMarker *baseline_marker = gw_project_get_baseline_marker(GLOBALS->project);

                    // TODO: don't use sentinel value
                    if (GLOBALS->dual_ctx[GLOBALS->dual_id].baseline !=
                        (gw_marker_is_enabled(baseline_marker)
                             ? gw_marker_get_position(baseline_marker)
                             : -1)) {
                        if (gw_marker_is_enabled(primary_marker) &&
                            (GLOBALS->dual_ctx[GLOBALS->dual_id].marker == -1)) {
                            GwTrace *t;

                            for (t = GLOBALS->traces.first; t; t = t->t_next) {
                                if (t->asciivalue) {
                                    free_2(t->asciivalue);
                                    t->asciivalue = NULL;
                                }
                            }

                            for (t = GLOBALS->traces.buffer; t; t = t->t_next) {
                                if (t->asciivalue) {
                                    free_2(t->asciivalue);
                                    t->asciivalue = NULL;
                                }
                            }
                        }

                        // TODO: don't use sentinel values for disabled markers
                        gw_marker_set_position(primary_marker,
                                               GLOBALS->dual_ctx[GLOBALS->dual_id].marker);
                        gw_marker_set_enabled(primary_marker,
                                              GLOBALS->dual_ctx[GLOBALS->dual_id].marker >= 0);
                        gw_marker_set_position(baseline_marker,
                                               GLOBALS->dual_ctx[GLOBALS->dual_id].baseline);
                        gw_marker_set_enabled(baseline_marker,
                                              GLOBALS->dual_ctx[GLOBALS->dual_id].baseline >= 0);

                        update_time_box();
                        GLOBALS->signalwindow_width_dirty = 1;
                        button_press_release_common();
                    } else if (GLOBALS->dual_ctx[GLOBALS->dual_id].marker !=
                               (gw_marker_is_enabled(primary_marker)
                                    ? gw_marker_get_position(primary_marker)
                                    : -1)) {
                        if (gw_marker_is_enabled(primary_marker) &&
                            (GLOBALS->dual_ctx[GLOBALS->dual_id].marker == -1)) {
                            GwTrace *t;

                            for (t = GLOBALS->traces.first; t; t = t->t_next) {
                                if (t->asciivalue) {
                                    free_2(t->asciivalue);
                                    t->asciivalue = NULL;
                                }
                            }

                            for (t = GLOBALS->traces.buffer; t; t = t->t_next) {
                                if (t->asciivalue) {
                                    free_2(t->asciivalue);
                                    t->asciivalue = NULL;
                                }
                            }
                        }

                        // TODO: don't use sentinel values for disabled markers
                        gw_marker_set_position(primary_marker,
                                               GLOBALS->dual_ctx[GLOBALS->dual_id].marker);
                        gw_marker_set_enabled(primary_marker,
                                              GLOBALS->dual_ctx[GLOBALS->dual_id].marker >= 0);

                        update_time_box();
                        GLOBALS->signalwindow_width_dirty = 1;
                        button_press_release_common();
                    }

                    GLOBALS->tims.prevzoom = GLOBALS->tims.zoom;
                    GLOBALS->tims.zoom = GLOBALS->dual_ctx[GLOBALS->dual_id].zoom;

                    if (gt < GLOBALS->tims.first)
                        gt = GLOBALS->tims.first;
                    else if (gt > GLOBALS->tims.last)
                        gt = GLOBALS->tims.last;

                    hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
                    gtk_adjustment_set_value(hadj, gt);

                    pageinc = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
                    if (gt < (GLOBALS->tims.last - pageinc + 1))
                        GLOBALS->tims.timecache = gt;
                    else {
                        GLOBALS->tims.timecache = GLOBALS->tims.last - pageinc + 1;
                        if (GLOBALS->tims.timecache < GLOBALS->tims.first)
                            GLOBALS->tims.timecache = GLOBALS->tims.first;
                    }

                    time_update();
                }

                while (gtk_events_pending())
                    gtk_main_iteration();

                GLOBALS->dual_race_lock = 0;

#ifdef __MINGW32__
                Sleep(1000 / 25);
#else
                tv.tv_sec = 0;
                tv.tv_usec = 1000000 / 25;
                select(0, NULL, NULL, NULL, &tv);
#endif
            }
        } else {
            fprintf(stderr,
                    "Could not attach to %08X, exiting.\n",
                    GLOBALS->dual_attach_id_main_c_1);
            exit(255);
        }
    } else {
#if defined(HAVE_LIBTCL)
        if (is_wish) {
            char *argv_mod[1];

            set_globals_interp(argv[0], 1);
            addPidToExecutableName(1, argv, argv_mod);
            Tk_MainEx(1, argv_mod, gtkwaveInterpreterInit, GLOBALS->interp);

            /* note: for(kk=0;kk<argc;kk++) { free_2(argv_mod[kk]); } can't really be done here,
             * doesn't matter anyway as context free will get it */
        } else {
            gtk_main();
        }
#else
        gtk_main();
#endif
    }

#ifdef MAC_INTEGRATION
    exit(0); /* gtk_target_list_find crashes in OSX/Quartz is return instead of exit */
#else
    return (0);
#endif
}

void get_window_size(int *x, int *y)
{
    gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), x, y);
}

void set_window_size(int x, int y)
{
    if (GLOBALS->block_xy_update) {
        return;
    }

    if (GLOBALS->mainwindow == NULL) {
        GLOBALS->initial_window_width = x;
        GLOBALS->initial_window_height = y;
    } else {
#ifdef WAVE_USE_XID
        if (!GLOBALS->socket_xid)
#endif
        {
#ifdef MAC_INTEGRATION
            gtk_window_resize(GTK_WINDOW(GLOBALS->mainwindow), x, y);
#else
            gtk_window_set_default_size(GTK_WINDOW(GLOBALS->mainwindow), x, y);
#endif
        }
    }
}

void get_window_xypos(int *root_x, int *root_y)
{
    if (!GLOBALS->mainwindow)
        return;

    gtk_window_get_position(GTK_WINDOW(GLOBALS->mainwindow), root_x, root_y);

    if (!GLOBALS->initial_window_get_valid) {
        if ((gtk_widget_get_window(GLOBALS->mainwindow))) {
            GLOBALS->initial_window_get_valid = 1;
            GLOBALS->initial_window_xpos_get = *root_x;
            GLOBALS->initial_window_ypos_get = *root_y;

            GLOBALS->xpos_delta =
                GLOBALS->initial_window_xpos_set - GLOBALS->initial_window_xpos_get;
            GLOBALS->ypos_delta =
                GLOBALS->initial_window_ypos_set - GLOBALS->initial_window_ypos_get;
        }
    }
}

void set_window_xypos(int root_x, int root_y)
{
#ifdef MAC_INTEGRATION
    if (GLOBALS->num_notebook_pages > 1)
        return;
#else
    if (GLOBALS->xy_ignore_main_c_1)
        return;
#endif

#if !defined __MINGW32__
    GLOBALS->initial_window_xpos = root_x;
    GLOBALS->initial_window_ypos = root_y;

    if (!GLOBALS->mainwindow)
        return;
    if ((GLOBALS->initial_window_xpos >= 0) || (GLOBALS->initial_window_ypos >= 0)) {
        if (GLOBALS->initial_window_xpos < 0) {
            GLOBALS->initial_window_xpos = 0;
        }
        if (GLOBALS->initial_window_ypos < 0) {
            GLOBALS->initial_window_ypos = 0;
        }
        gtk_window_move(GTK_WINDOW(GLOBALS->mainwindow),
                        GLOBALS->initial_window_xpos,
                        GLOBALS->initial_window_ypos);

        if (!GLOBALS->initial_window_set_valid) {
            GLOBALS->initial_window_set_valid = 1;
            GLOBALS->initial_window_xpos_set = GLOBALS->initial_window_xpos;
            GLOBALS->initial_window_ypos_set = GLOBALS->initial_window_ypos;
        }
    }
#endif
}

/*
 * bring up stems browser
 */
int stems_are_active(void)
{
#ifdef __MINGW32__
    if (GLOBALS->anno_ctx && GLOBALS->anno_ctx->browser_process) {
        /* nothing */
        return (1);
    }
#else
    if (GLOBALS->anno_ctx && GLOBALS->anno_ctx->browser_process) {
        int mystat = 0;
        pid_t pid = waitpid(GLOBALS->anno_ctx->browser_process, &mystat, WNOHANG);
        if (!pid) {
            status_text("Stems reader already active.\n");
            return (1);
        } else {
            shmdt((void *)GLOBALS->anno_ctx);
            GLOBALS->anno_ctx = NULL;
        }
    }
#endif
    return (0);
}

void activate_stems_reader(char *stems_name)
{
#ifdef __CYGWIN__
    /* ajb : ok static as this is a one-time warning message... */
    static int cyg_called = 0;
#endif

    if (!stems_name)
        return;

#ifdef __CYGWIN__
    if (GLOBALS->stems_type != WAVE_ANNO_NONE) {
        if (!cyg_called) {
            char *cygserver_env = getenv("CYGWIN");
            gboolean found = cygserver_env && (strstr(cygserver_env, "server") != NULL);

            if (!found) {
                fprintf(stderr,
                        "GTKWAVE | "
                        "=================================================================\n");
                fprintf(stderr, "GTKWAVE | If the viewer crashes with a Bad system call error,\n");
                fprintf(stderr, "GTKWAVE | make sure that Cygserver is enabled.\n");
                fprintf(stderr,
                        "GTKWAVE | The Cygserver services are used by Cygwin applications only\n");
                fprintf(stderr,
                        "GTKWAVE | if you set the environment variable CYGWIN to contain the\n");
                fprintf(stderr,
                        "GTKWAVE | string \"server\". You must do this before starting this "
                        "program.\n");
                fprintf(stderr, "GTKWAVE |\n");
                fprintf(stderr,
                        "GTKWAVE | If this still does not work, you may have to enable the "
                        "cygserver\n");
                fprintf(stderr,
                        "GTKWAVE | by entering \"cygserver-config\" and answering \"yes\" followed "
                        "by\n");
                fprintf(stderr, "GTKWAVE | \"net start cygserver\".\n");
                fprintf(stderr,
                        "GTKWAVE | "
                        "=================================================================\n");
            }
            cyg_called = 1;
        }
    }
#endif

    if (GLOBALS->stems_type != WAVE_ANNO_NONE) {
#ifdef __MINGW32__
        int shmid = getpid();
        char mapName[257];
        HANDLE hMapFile;
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        BOOL rc;

        memset(&si, 0, sizeof(STARTUPINFO));
        memset(&pi, 0, sizeof(PROCESS_INFORMATION));
        si.cb = sizeof(si);

        sprintf(mapName, "rtlbrowse%d", shmid);
        hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     sizeof(struct gtkwave_annotate_ipc_t),
                                     mapName);
        if (hMapFile != NULL) {
            GLOBALS->anno_ctx = MapViewOfFile(hMapFile,
                                              FILE_MAP_ALL_ACCESS,
                                              0,
                                              0,
                                              sizeof(struct gtkwave_annotate_ipc_t));
            if (GLOBALS->anno_ctx) {
                char mylist[257];

                sprintf(mylist, "rtlbrowse.exe %08x", shmid);

                memset(GLOBALS->anno_ctx, 0, sizeof(struct gtkwave_annotate_ipc_t));

                memcpy(GLOBALS->anno_ctx->matchword, WAVE_MATCHWORD, 4);
                GLOBALS->anno_ctx->aet_type = GLOBALS->stems_type;
                strcpy(GLOBALS->anno_ctx->aet_name, GLOBALS->aet_name);
                strcpy(GLOBALS->anno_ctx->stems_name, stems_name);

                update_time_box();

                rc = CreateProcess("rtlbrowse.exe",
                                   mylist,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   0,
                                   NULL,
                                   NULL,
                                   &si,
                                   &pi);

                if (!rc) {
                    UnmapViewOfFile(GLOBALS->anno_ctx);
                    CloseHandle(hMapFile);
                    GLOBALS->anno_ctx = NULL;
                    GLOBALS->stems_type = WAVE_ANNO_NONE;
                } else {
                    GLOBALS->anno_ctx->browser_process = pi.hProcess;
                }
            } else {
                CloseHandle(hMapFile);
                GLOBALS->stems_type = WAVE_ANNO_NONE;
            }
        }
#else
        int shmid = shmget(0, sizeof(struct gtkwave_annotate_ipc_t), IPC_CREAT | 0600);
        if (shmid >= 0) {
            struct shmid_ds ds;

            struct gtkwave_annotate_ipc_t *anno_ctx = shmat(shmid, NULL, 0);
            if (anno_ctx != (void *)-1) {
                pid_t pid;

                GLOBALS->anno_ctx = anno_ctx;

                memset(GLOBALS->anno_ctx, 0, sizeof(struct gtkwave_annotate_ipc_t));

                memcpy(GLOBALS->anno_ctx->matchword, WAVE_MATCHWORD, 4);
                GLOBALS->anno_ctx->aet_type = GLOBALS->stems_type;
                strcpy(GLOBALS->anno_ctx->aet_name, GLOBALS->aet_name);
                strcpy(GLOBALS->anno_ctx->stems_name, stems_name);

                GLOBALS->anno_ctx->gtkwave_process = getpid();
                update_time_box();

#ifdef __linux__
                shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
#endif

                pid = fork();

                if (((int)pid) < 0) {
                    /* can't do anything about this */
                } else {
                    if (pid) /* parent==original server_pid */
                    {
#ifndef __CYGWIN__
                        static int kill_installed = 0;

                        if (!kill_installed) {
                            kill_installed = 1;
                            atexit(kill_stems_browser);
                        }
#endif
                        GLOBALS->anno_ctx->browser_process = pid;
#ifndef __linux__
                        sleep(2);
                        shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
#endif
                    } else {
                        char buf[64];
#ifdef MAC_INTEGRATION
                        const gchar *p = gtkosx_application_get_executable_path();
#endif
                        sprintf(buf, "%08x", shmid);

#ifdef MAC_INTEGRATION
                        if (p && strstr(p, "Contents/")) {
                            const char *xec = "../Resources/bin/rtlbrowse";
                            char *res = strdup_2(p);
                            char *slsh = strrchr(res, '/');
                            if (slsh) {
                                *(slsh + 1) = 0;
                                res = realloc_2(res, strlen(res) + strlen(xec) + 1);
                                strcat(res, xec);
                                execlp(res, "rtlbrowse", buf, NULL);
                                fprintf(stderr, "GTKWAVE | Could not find '%s' in .app!\n", res);
                                free_2(res);
                            }
                        }
#endif
                        execlp("rtlbrowse", "rtlbrowse", buf, NULL);
                        fprintf(stderr,
                                "GTKWAVE | Could not find rtlbrowse executable, exiting!\n");
                        exit(255); /* control never gets here if successful */
                    }
                }
            } else {
                shmctl(shmid, IPC_RMID, &ds); /* actually destroy */
                GLOBALS->stems_type = WAVE_ANNO_NONE;
            }
        }
#endif
    } else {
        fprintf(stderr, "GTKWAVE | Unsupported dumpfile type for rtlbrowse.\n");
    }
}

#if !defined __MINGW32__
void optimize_vcd_file(void)
{
    if (!strcmp("-vcd", GLOBALS->unoptimized_vcd_file_name)) {
#ifdef __CYGWIN__
        char *buf = strdup_2("vcd2fst -- - vcd.fst");
        system(buf);
        free_2(buf);
        GLOBALS->loaded_file_name = strdup_2("vcd.fst");
        GLOBALS->is_optimized_stdin_vcd = 1;
#else
        pid_t pid;
        char *buf = malloc_2(strlen("vcd") + 4 + 1);
        sprintf(buf, "%s.fst", "vcd");
        pid = fork();
        if (((int)pid) < 0) {
            /* can't do anything about this */
        } else {
            if (pid) {
                int mystat;
                int rc = waitpid(pid, &mystat, 0);
                if (rc > 0) {
                    free_2(GLOBALS->loaded_file_name);
                    GLOBALS->loaded_file_name = buf;
                    GLOBALS->is_optimized_stdin_vcd = 1;
                }
            } else {
                execlp("vcd2fst", "vcd2fst", "--", "-", buf, NULL);
                exit(255);
            }
        }
#endif
    } else {
#ifdef __CYGWIN__
        char *buf = malloc_2(9 + (strlen(GLOBALS->unoptimized_vcd_file_name) + 1) +
                             (strlen(GLOBALS->unoptimized_vcd_file_name) + 4 + 1));
        sprintf(buf,
                "vcd2fst %s %s.fst",
                GLOBALS->unoptimized_vcd_file_name,
                GLOBALS->unoptimized_vcd_file_name);
        system(buf);
        free_2(buf);
        buf = malloc_2(strlen(GLOBALS->unoptimized_vcd_file_name) + 4 + 1);
        sprintf(buf, "%s.fst", GLOBALS->unoptimized_vcd_file_name);
        GLOBALS->loaded_file_name = buf;
#else
        pid_t pid;
        char *buf = malloc_2(strlen(GLOBALS->unoptimized_vcd_file_name) + 4 + 1);
        sprintf(buf, "%s.fst", GLOBALS->unoptimized_vcd_file_name);
        pid = fork();
        if (((int)pid) < 0) {
            /* can't do anything about this */
        } else {
            if (pid) {
                int mystat;
                int rc = waitpid(pid, &mystat, 0);
                if (rc > 0) {
                    free_2(GLOBALS->loaded_file_name);
                    GLOBALS->loaded_file_name = buf;
                }
            } else {
#ifdef MAC_INTEGRATION
                const gchar *p = gtkosx_application_get_executable_path();
                if (p && strstr(p, "Contents/")) {
                    const char *xec = "../Resources/bin/vcd2fst";
                    char *res = strdup_2(p);
                    char *slsh = strrchr(res, '/');
                    if (slsh) {
                        *(slsh + 1) = 0;
                        res = realloc_2(res, strlen(res) + strlen(xec) + 1);
                        strcat(res, xec);
                        execlp(res, "vcd2fst", GLOBALS->unoptimized_vcd_file_name, buf, NULL);
                        fprintf(stderr, "GTKWAVE | Could not find '%s' in .app!\n", res);
                        free_2(res);
                    }
                }
#endif
                execlp("vcd2fst", "vcd2fst", GLOBALS->unoptimized_vcd_file_name, buf, NULL);
                fprintf(stderr, "GTKWAVE | Could not find vcd2fst executable, exiting!\n");
                exit(255);
            }
        }
#endif
    }
}
#endif
