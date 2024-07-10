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
static void parse_args(const gchar *arg)
{
    FILE *f;
    const char *file_name;
    ds_Tree *modules = NULL;
    int i;
    int len;

    len = strlen(arg);

    /* Determine whether arg is file name or shmid */
    for (i = 0; i < len; i++) {
        if (!g_ascii_isxdigit(arg[i]))
            break;
    }
    if (i == len) {
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
        if (anno_ctx != (void *)-1) {
            if ((!memcmp(anno_ctx->matchword, WAVE_MATCHWORD, 4)) &&
                (anno_ctx->aet_type > WAVE_ANNO_NONE) && (anno_ctx->aet_type < WAVE_ANNO_MAX)) {
                file_name = anno_ctx->stems_name;
            } else {
                shmdt((void *)anno_ctx);
                fprintf(stderr, "Not a valid shared memory ID from gtkwave, exiting.\n");
                exit(255);
            }
        } else {
            file_name = arg;
        }
    } else {
        file_name = arg;
    }

    f = fopen(file_name, "rb");
    if (!f) {
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
    GVariantDict *options;
    gchar **cmd;

    options = g_application_command_line_get_options_dict(cmdline);
    if (!g_variant_dict_lookup(options, G_OPTION_REMAINING, "^a&s", &cmd) ||
        g_strv_length(cmd) != 1) {
        // g_option_context_get_help can not be used to render help text here.
        // Since the `context` it need is constructed inside g_application_parse_command_line,
        // using private members of GApplication. Is there another way to print the help?
        g_print("try '%s --help' for help.\n", g_get_prgname());
        g_application_quit(app);
        return -1;
    }
    parse_args(cmd[0]);
    g_free(cmd);

    activate(app);
    return 0;
}

int main(int argc, char **argv)
{
    WAVE_LOCALE_FIX

    GtkApplication *app;
    int status;
    const GOptionEntry entries[] = {
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, "", NULL},
        G_OPTION_ENTRY_NULL};

    if (!gtk_init_check()) {
        printf("Could not initialize GTK!  Is DISPLAY env var/xhost set?\n\n");
        exit(255);
    }

    app = gtk_application_new("io.github.gtkwave.RTLBrowse",
                              G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_NON_UNIQUE);

    g_application_add_main_option_entries(G_APPLICATION(app), entries);
    g_application_set_option_context_parameter_string(G_APPLICATION(app), "stems_filename");
    g_application_set_option_context_summary(
        G_APPLICATION(app),
        "RTLBrowse is used to view and navigate through RTL source code, often called as\n"
        "a helper application by GTKWave.\n"
        "If GTKWave is started with the --stems option, the stems file is parsed and\n"
        "RTLBrowse is launched.");

    g_signal_connect(app, "command-line", G_CALLBACK(command_line), NULL);
    g_signal_connect(G_APPLICATION(app), "activate", G_CALLBACK(activate), app);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}