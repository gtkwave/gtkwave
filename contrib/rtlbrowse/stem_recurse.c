/*
 * Copyright (c) Tony Bybell 2006-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "splay.h"
#include "wavelink.h"

#include "wave_locale.h"

GtkTreeStore *treestore_main = NULL;
GtkWidget *treeview_main = NULL;

#if defined __MINGW32__
#define shmat(a,b,c) NULL
#define shmdt(a)
#endif

void treebox(char *title, GCallback func, GtkWidget *old_window, GtkApplication *app);
gboolean update_ctx_when_idle(gpointer dummy);

int verilog_2005 = 0; /* currently 1364-2005 keywords are disabled */

int mod_cnt;
ds_Tree **mod_list;
ds_Tree *flattened_mod_list_root = NULL;

struct gtkwave_annotate_ipc_t *anno_ctx = NULL;
struct vzt_rd_trace  *vzt=NULL;
struct lxt2_rd_trace *lx2=NULL;
void *fst=NULL;
int64_t timezero = 0; /* only currently used for FST */

static int bwsigcmp(char *s1, char *s2)
{
unsigned char c1, c2;
int u1, u2;

for(;;)
        {
        c1=(unsigned char)*(s1++);
        c2=(unsigned char)*(s2++);

        if((!c1)&&(!c2)) return(0);
        if((c1<='9')&&(c2<='9')&&(c2>='0')&&(c1>='0'))
                {
                u1=(int)(c1&15);
                u2=(int)(c2&15);

                while(((c2=(unsigned char)*s2)>='0')&&(c2<='9'))
                        {
                        u2*=10;
                        u2+=(unsigned int)(c2&15);
                        s2++;
                        }

                while(((c2=(unsigned char)*s1)>='0')&&(c2<='9'))
                        {
                        u1*=10;
                        u1+=(unsigned int)(c2&15);
                        s1++;
                        }

                if(u1==u2) continue;
                        else return((int)u1-(int)u2);
                }
                else
                {
                if(c1!=c2) return((int)c1-(int)c2);
                }
        }
}



static int compar_comp_array_bsearch(const void *s1, const void *s2)
{
char *key, *obj;

key=(*((struct ds_component **)s1))->compname;
obj=(*((struct ds_component **)s2))->compname;

return(bwsigcmp(key, obj));
}


void recurse_into_modules(char *compname_build, char *compname, ds_Tree *t, int depth, GtkTreeIter *ti_subtree)
{
struct ds_component *comp;
struct ds_component **comp_array;
int i;
char *compname_full;
char *txt, *txt2 = NULL;
ds_Tree *tdup = malloc(sizeof(ds_Tree));
int numcomps;
char *colon;
char *compname2 = compname ? strdup(compname) : NULL;
char *compname_colon = NULL;

if(compname2)
	{
	compname_colon = strchr(compname2, ':');
	if(compname_colon)
		{
		*compname_colon = 0;
		}
	}

memcpy(tdup, t, sizeof(ds_Tree));
t = tdup;
colon = strchr(t->item, ':');
if(colon)
	{
	*colon = 0; /* remove generate hack */
	}

t->next_flat = flattened_mod_list_root;
flattened_mod_list_root = t;

if(compname_build)
	{
	int cnl = strlen(compname_build);

	compname_full = malloc(cnl + 1 + strlen(compname2) + 1);
	strcpy(compname_full, compname_build);
	compname_full[cnl] = '.';
	strcpy(compname_full + cnl + 1, compname2);
	}
	else
	{
	compname_full = strdup(t->item);
	}

t->fullname = compname_full;
txt = compname_build ? compname2 : t->item;
if(!t->filename)
	{
	txt2 = malloc(strlen(txt) + strlen(" [MISSING]") + 1);
	strcpy(txt2, txt);
	strcat(txt2, " [MISSING]");
	/* txt = txt2; */ /* scan-build */
	}

gtk_tree_store_set (treestore_main, ti_subtree,
                    XXX_NAME_COLUMN, compname2 ? compname2 : t->item, /* t->comp->compname? */
                    XXX_TREE_COLUMN, t,
                    -1);

if(colon)
	{
	*colon = ':';
	}

free(compname2); compname2 = NULL;

comp = t->comp;
if(comp)
	{
	numcomps = 0;
	while(comp)
		{
		numcomps++;
		comp = comp->next;
		}

	comp_array = calloc(numcomps, sizeof(struct ds_component *));

	comp = t->comp;
	for(i=0;i<numcomps;i++)
		{
		comp_array[i] = comp;
		comp = comp->next;
		}
	qsort(comp_array, numcomps, sizeof(struct ds_component *), compar_comp_array_bsearch);
	for(i=0;i<numcomps;i++)
		{
                GtkTreeIter iter;
                gtk_tree_store_append (treestore_main, &iter, ti_subtree);

		comp = comp_array[i];

		if(0)
                gtk_tree_store_set (treestore_main, &iter,
                    XXX_NAME_COLUMN, comp->compname,
                    XXX_TREE_COLUMN, comp->module,
                    -1);

		recurse_into_modules(compname_full, comp->compname, comp->module, depth+1, &iter);
		}

	free(comp_array);
	}

if(txt2)
	{
	free(txt2);
	}
}

/* Recursively counts the total number of nodes in ds_Tree */
void rec_tree(ds_Tree *t, int *cnt)
{
if(!t) return;

if(t->left)
	{
	rec_tree(t->left, cnt);
	}

(*cnt)++;

if(t->right)
	{
	rec_tree(t->right, cnt);
	}
}

/* Recursively populates an array with ds_tree_node in inorder traversal sequence */
void rec_tree_populate(ds_Tree *t, int *cnt, ds_Tree **list_root)
{
if(!t) return;

if(t->left)
	{
	rec_tree_populate(t->left, cnt, list_root);
	}

list_root[*cnt] = t;
(*cnt)++;

if(t->right)
	{
	rec_tree_populate(t->right, cnt, list_root);
	}
}


ds_Tree *load_stems_file(FILE *f)
{
    ds_Tree *modules = NULL;
    while (!feof(f)) {
        char *ln = fgetmalloc(f);

        if (fgetmalloc_len > 4) {
            if ((ln[0] == '+') && (ln[1] == '+') && (ln[2] == ' ')) {
                if (ln[3] == 'c') {
                    char cname[1024], mname[1024], pname[1024], scratch[128];
                    ds_Tree *which_module;
                    struct ds_component *dc;

                    sscanf(ln + 8, "%s %s %s %s %s", cname, scratch, mname, scratch, pname);
                    /* printf("comp: %s module: %s, parent: %s\n", cname, mname, pname); */

                    modules = ds_splay(mname, modules);
                    if ((!modules) || (strcmp(modules->item, mname))) {
                        modules = ds_insert(strdup(mname), modules);
                    }
                    which_module = modules;
                    which_module->refcnt++;

                    modules = ds_splay(pname, modules);
                    if (strcmp(modules->item, pname)) {
                        modules = ds_insert(strdup(pname), modules);
                    }

                    dc = calloc(1, sizeof(struct ds_component));
                    dc->compname = strdup(cname);
                    dc->module = which_module;
                    dc->next = modules->comp;
                    modules->comp = dc;
                } else if ((ln[3] == 'm') || (ln[3] == 'u')) {
                    char scratch[128], mname[1024], fname[1024];
                    int s_line, e_line;

                    sscanf(ln + 3,
                           "%s %s %s %s %s %d %s %d",
                           scratch,
                           mname,
                           scratch,
                           fname,
                           scratch,
                           &s_line,
                           scratch,
                           &e_line);
                    /* printf("mod: %s from %s %d-%d\n", mname, fname, s_line, e_line); */

                    modules = ds_insert(strdup(mname), modules);
                    modules->filename = strdup(fname);
                    modules->s_line = s_line;
                    modules->e_line = e_line;
                    modules->resolved = 1;
                } else if (ln[3] == 'v') {
                }
            }
        }

        free(ln);
    }
    return modules;
}

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

void bwmaketree(void)
{
int i;

treestore_main = gtk_tree_store_new (XXX2_NUM_COLUMNS,       /* Total number of columns */
                                          G_TYPE_STRING,             /* name */
                                          G_TYPE_POINTER);           /* tree */

for(i=0;i<mod_cnt;i++)
	{
	ds_Tree *t = mod_list[i];

	if(!t->refcnt)
		{
		/* printf("TOP: %s\n", t->item); */
                GtkTreeIter iter;
                gtk_tree_store_append (treestore_main, &iter, NULL);

		recurse_into_modules(NULL, NULL, t, 0, &iter /* ti_subtree */);
		}
	else
	if(!t->resolved)
		{
		/* printf("MISSING: %s\n", t->item); */
		}
	}


GtkCellRenderer *renderer_t;
GtkTreeViewColumn *column;

treeview_main = gtk_tree_view_new_with_model (GTK_TREE_MODEL (treestore_main));
gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview_main), FALSE);
gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW(treeview_main), TRUE);

column = gtk_tree_view_column_new();
renderer_t = gtk_cell_renderer_text_new ();

gtk_tree_view_column_pack_start (column, renderer_t, FALSE);

gtk_tree_view_column_add_attribute (column,
                                    renderer_t,
                                    "text",
                                    XXX2_NAME_COLUMN);

gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_main), column);

gtk_widget_show(treeview_main);
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

    treebox("RTL Design Hierarchy", NULL, NULL, GTK_APPLICATION(app));

    g_timeout_add(100, update_ctx_when_idle, NULL);
}

static gint
command_line (GApplication            *app,
              GApplicationCommandLine *cmdline)
{
    GVariantDict *options;
    gchar** cmd;

    options = g_application_command_line_get_options_dict (cmdline);
    if (!g_variant_dict_lookup (options, G_OPTION_REMAINING, "^a&s", &cmd) ||
        g_strv_length(cmd) != 1)
    {
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
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, NULL, "", NULL },
      G_OPTION_ENTRY_NULL
    };

    if (!gtk_init_check(&argc, &argv)) {
        printf("Could not initialize GTK!  Is DISPLAY env var/xhost set?\n\n");
        exit(255);
    }

    app = gtk_application_new("io.github.gtkwave.RTLBrowse",
                                  G_APPLICATION_HANDLES_COMMAND_LINE|G_APPLICATION_NON_UNIQUE);

    g_application_add_main_option_entries(G_APPLICATION(app), entries);
    g_application_set_option_context_parameter_string(G_APPLICATION(app), "stems_filename");
    g_application_set_option_context_summary (
        G_APPLICATION(app),
        "RTLBrowse is used to view and navigate through RTL source code, often called as\n"
        "a helper application by GTKWave.\n"
        "If GTKWave is started with the --stems option, the stems file is parsed and\n"
        "RTLBrowse is launched.");

    g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);

    g_signal_connect(G_APPLICATION(app), "activate", G_CALLBACK(activate), app);
    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}