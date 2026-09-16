#include <config.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "splay.h"
#include "wavelink.h"
#include "stem_recurse.h"

#include "wave_locale.h"

#if defined __MINGW32__
#define shmat(a, b, c) NULL
#define shmdt(a)
#endif

ds_Tree *load_stems_file(FILE *f);
void rec_tree(ds_Tree *t, int *cnt);
void rec_tree_populate(ds_Tree *t, int *cnt, ds_Tree **list_root);
void treebox(const char *title, GtkApplication *app);
gboolean update_ctx_when_idle(gpointer dummy);

/* Side-effect: mod_cnt, mod_list */
static void parse_args(const gchar *arg, const gboolean is_ipc_mode)
{
    FILE *f;
    const char *file_name;
    ds_Tree *modules = NULL;

    if (is_ipc_mode) {
        unsigned int shmid;

        sscanf(arg, "%x", &shmid);
#ifdef __MINGW32__
        {
            HANDLE hMapFile;
            char mapName[257];

            sprintf(mapName, "rtlbrowse%d", shmid);
            hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapName);
            if (hMapFile == NULL) {
                fprintf(stderr,
                        "Could not attach shared memory map name '%s', exiting.\n",
                        mapName);
                exit(255);
            }
            anno_ctx = MapViewOfFile(hMapFile,
                                     FILE_MAP_ALL_ACCESS,
                                     0,
                                     0,
                                     sizeof(struct gtkwave_annotate_ipc_t));
            if (anno_ctx == NULL) {
                fprintf(stderr, "Could not map view of file '%s', exiting.\n", mapName);
                exit(255);
            }
        }
#else
        anno_ctx = shmat(shmid, NULL, 0);
#endif
        if (anno_ctx != (void *)-1 && !memcmp(anno_ctx->matchword, WAVE_MATCHWORD, 4) &&
            anno_ctx->aet_type > WAVE_ANNO_NONE && anno_ctx->aet_type < WAVE_ANNO_MAX) {
            file_name = anno_ctx->stems_name;
        } else {
            shmdt(anno_ctx);
            fprintf(stderr, "Not a valid shared memory ID from gtkwave, exiting.\n");
            exit(255);
        }
    } else {
        file_name = arg;
    }

    f = fopen(file_name, "rb");
    if (f == NULL) {
        fprintf(stderr, "*** Could not open '%s'\n", file_name);
        perror("Why");
        exit(255);
    }
    modules = load_stems_file(f);
    fclose(f);

    mod_cnt = 0;
    rec_tree(modules, &mod_cnt);
    /* printf("number of modules: %d\n", mod_cnt); */
    mod_list = calloc(mod_cnt ? mod_cnt : 1, sizeof(ds_Tree *));
    mod_cnt = 0;
    rec_tree_populate(modules, &mod_cnt, mod_list);
}

static void activate(GApplication *app)
{
    if (anno_ctx) {
        switch (anno_ctx->aet_type) {
            case WAVE_ANNO_FST:
                fst = fstReaderOpen(anno_ctx->aet_name);
                if (!fst) {
                    fprintf(stderr, "Could not initialize '%s', exiting.\n", anno_ctx->aet_name);
                    exit(255);
                }
                timezero = fstReaderGetTimezero(fst);
                break;

            default:
                fprintf(stderr,
                        "Unsupported wave file type %d encountered, exiting.\n",
                        anno_ctx->aet_type);
                exit(255);
                break;
        }
    }

    treebox("RTL Design Hierarchy", GTK_APPLICATION(app));

    g_timeout_add(100, update_ctx_when_idle, NULL);
}

static gint command_line(GApplication *app, GApplicationCommandLine *cmdline)
{
    gint argc;
    gchar **args, **argv;
    GOptionContext *context;
    GError *error = NULL;
    gchar **cmd = NULL;
    gboolean help = FALSE, is_ipc_mode = FALSE;

    GOptionEntry entries[] = {
        {"help", '?', 0, G_OPTION_ARG_NONE, &help, NULL, NULL},
        {"ipc", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &is_ipc_mode, NULL, NULL},
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &cmd, NULL, NULL},
        G_OPTION_ENTRY_NULL};

    args = g_application_command_line_get_arguments(cmdline, &argc);
    /* Keep a reference of the array, since g_option_context_parse()
     * remove string from the array without freeing them. While
     * g_application_command_line_get_arguments() requires those to be freed.
     */
    argv = g_new(gchar *, argc + 1);
    for (int i = 0; i <= argc; i++) {
        argv[i] = args[i];
    }

    context = g_option_context_new("<stems file name>");
    g_option_context_set_help_enabled(context, FALSE);
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, &args, &error)) {
        g_application_command_line_printerr(cmdline, "%s\n", error->message);
        g_error_free(error);
        return -1;
    }

    if (cmd == NULL || g_strv_length(cmd) > 1) {
        help = TRUE;
    }

    if (help) {
        gchar *text;
        text = g_option_context_get_help(context, FALSE, NULL);
        g_application_command_line_print(cmdline, "%s", text);
        g_free(text);
        g_application_quit(app);
        return 0;
    }
    parse_args(cmd[0], is_ipc_mode);

    g_strfreev(cmd);
    g_strfreev(argv);
    g_option_context_free(context);

    activate(app);
    return 0;
}

int main(int argc, char **argv)
{
    WAVE_LOCALE_FIX

    GtkApplication *app;
    int status;

    if (!gtk_init_check()) {
        printf("Could not initialize GTK!  Is DISPLAY env var/xhost set?\n\n");
        exit(255);
    }

    app = gtk_application_new("io.github.gtkwave.RTLBrowse",
                              G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_NON_UNIQUE);

    g_signal_connect(app, "command-line", G_CALLBACK(command_line), NULL);
    g_signal_connect(G_APPLICATION(app), "activate", G_CALLBACK(activate), app);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}