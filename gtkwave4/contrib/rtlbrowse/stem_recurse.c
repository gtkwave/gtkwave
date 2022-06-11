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

/*
#define WAVE_CRASH_ON_GTK_WARNING
*/

GtkTreeStore *treestore_main = NULL;
GtkWidget *treeview_main = NULL;

#if defined __MINGW32__
#define shmat(a,b,c) NULL
#define shmdt(a)
#endif

void treebox(char *title, GCallback func, GtkWidget *old_window);
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

#ifdef AET2_IS_PRESENT
FILE *aetf;
AE2_HANDLE ae2 = NULL;
#endif

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



int main_2r(int argc, char **argv)
{
FILE *f;
ds_Tree *modules = NULL;
char *id;
int i;
int len;

if(argc != 2)
	{
	printf("Usage:\n------\n%s stems_filename\n\n", argv[0]);
	exit(0);
	}

id = argv[1];
len = strlen(id);

for(i=0;i<len;i++)
	{
	if(!isxdigit((int)(unsigned char)id[i])) break;
	}
if(i==len)
	{
	unsigned int shmid;

	sscanf(id, "%x", &shmid);
#ifdef __MINGW32__
                {
                HANDLE hMapFile;
                char mapName[257];

                sprintf(mapName, "rtlbrowse%d", shmid);
                hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapName);
                if(hMapFile == NULL)
                        {
                        fprintf(stderr, "Could not attach shared memory map name '%s', exiting.\n", mapName);
                        exit(255);
                        }
                anno_ctx = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct gtkwave_annotate_ipc_t));
                if(anno_ctx == NULL)
                        {
                        fprintf(stderr, "Could not map view of file '%s', exiting.\n", mapName);
                        exit(255);
                        }
                }
#else
	anno_ctx = shmat(shmid, NULL, 0);
#endif
	if(anno_ctx)
		{
		if((!memcmp(anno_ctx->matchword, WAVE_MATCHWORD, 4))&&(anno_ctx->aet_type > WAVE_ANNO_NONE)&&(anno_ctx->aet_type < WAVE_ANNO_MAX))
			{
			id = anno_ctx->stems_name;
			}
			else
			{
			shmdt((void *)anno_ctx);
			fprintf(stderr, "Not a valid shared memory ID from gtkwave, exiting.\n");
			exit(255);
			}
		}
		else
		{
		id = argv[1];
		}
	}
	else
	{
	id = argv[1];
	}

f = fopen(id, "rb");
if(!f)
	{
	fprintf(stderr, "*** Could not open '%s'\n", argv[1]);
	perror("Why");
	exit(255);
	}

/* read_filename: */
while(!feof(f))
	{
	char *ln = fgetmalloc(f);

	if(fgetmalloc_len > 4)
		{
		if((ln[0] == '+')&&(ln[1] == '+')&&(ln[2]==' '))
			{
			if(ln[3]=='c')
				{
				char cname[1024], mname[1024], pname[1024], scratch[128];
				ds_Tree *which_module;
				struct ds_component *dc;

				sscanf(ln+8, "%s %s %s %s %s", cname, scratch, mname, scratch, pname);
				/* printf("comp: %s module: %s, parent: %s\n", cname, mname, pname); */

				modules = ds_splay(mname, modules);
				if((!modules)||(strcmp(modules->item, mname)))
					{
					modules = ds_insert(strdup(mname), modules);
					}
				which_module = modules;
				which_module->refcnt++;

				modules = ds_splay(pname, modules);
				if(strcmp(modules->item, pname))
					{
					modules = ds_insert(strdup(pname), modules);
					}

				dc = calloc(1, sizeof(struct ds_component));
				dc->compname = strdup(cname);
				dc->module = which_module;
				dc->next = modules->comp;
				modules->comp = dc;
				}
			else
			if((ln[3]=='m')||(ln[3]=='u'))
				{
				char scratch[128], mname[1024], fname[1024];
				int s_line, e_line;

				sscanf(ln+3, "%s %s %s %s %s %d %s %d", scratch, mname, scratch, fname, scratch, &s_line, scratch, &e_line);
				/* printf("mod: %s from %s %d-%d\n", mname, fname, s_line, e_line); */

				modules = ds_insert(strdup(mname), modules);
				modules->filename = strdup(fname);
				modules->s_line = s_line;
				modules->e_line = e_line;
				modules->resolved = 1;
				}
			else
			if(ln[3]=='v')
				{
				}
			}
		}


	free(ln);
	}
fclose(f);

mod_cnt = 0;
rec_tree(modules, &mod_cnt);
/* printf("number of modules: %d\n", mod_cnt); */

mod_list = calloc(mod_cnt /* scan-build */ ? mod_cnt : 1, sizeof(ds_Tree *));
mod_cnt = 0;
rec_tree_populate(modules, &mod_cnt, mod_list);

return(0);
}


/**********************************************************/
/**********************************************************/
/**********************************************************/

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

/**********************************************************/
/**********************************************************/

#ifdef AET2_IS_PRESENT

static void *alloc_fn(size_t size)
{
void *pnt = calloc(1, size);
return(pnt);
}

static void free_fn(void* ptr, size_t size)
{
(void)size;

free(ptr);
}

static void error_fn(const char *format, ...)
{
va_list ap;
va_start(ap, format);
vfprintf(stderr, format, ap);
fprintf(stderr, "\n");
va_end(ap);
exit(255);
}

static void msg_fn(int sev, const char *format, ...)
{
va_list ap;
va_start(ap, format);

fprintf(stderr, "AE2 %03d | ", sev);
vfprintf(stderr, format, ap);
fprintf(stderr, "\n");
va_end(ap);
}

#endif

/**********************************************************/
/**********************************************************/

#if GTK_CHECK_VERSION(3,0,0)
static GLogWriterOutput
gtkwave_glib_log_handler (GLogLevelFlags log_level,
                   const GLogField *fields,
                   gsize n_fields,
                   gpointer user_data)
{
(void) user_data;

#ifndef WAVE_CRASH_ON_GTK_WARNING
if(log_level & (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG))
        {
	/* filter out low-level warnings as GTK3 is too chatty */
        }
        else
#endif
        {
        gsize i;
	for(i=0;i<n_fields;i++)
                {
                fprintf(stderr, "GTKWAVE | %s: %s\n", fields[i].key, (const char *)fields[i].value); /* provides exact location: much better than stock message */
                }
        }

#ifdef WAVE_CRASH_ON_GTK_WARNING
abort();
#endif

return(G_LOG_WRITER_HANDLED);
}
#endif


int main(int argc, char **argv)
{
WAVE_LOCALE_FIX

main_2r(argc, argv);

if(!gtk_init_check(&argc, &argv))
        {
        printf("Could not initialize GTK!  Is DISPLAY env var/xhost set?\n\n");
        exit(255);
        }

#if GTK_CHECK_VERSION(3,0,0)
g_log_set_writer_func (gtkwave_glib_log_handler, NULL, NULL);
#endif

#ifdef WAVE_CRASH_ON_GTK_WARNING
        g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING);
#endif

if(anno_ctx)
	{
	switch(anno_ctx->aet_type)
		{
		case WAVE_ANNO_FST:
			fst=fstReaderOpen(anno_ctx->aet_name);
			if(!fst)
			        {
			        fprintf(stderr, "Could not initialize '%s', exiting.\n", anno_ctx->aet_name);
			        exit(255);
			        }
				else
				{
				timezero = fstReaderGetTimezero(fst);
				}
			break;

		case WAVE_ANNO_VZT:
			vzt=vzt_rd_init(anno_ctx->aet_name);
			if(!vzt)
			        {
			        fprintf(stderr, "Could not initialize '%s', exiting.\n", anno_ctx->aet_name);
			        exit(255);
			        }
			break;

		case WAVE_ANNO_LXT2:
			lx2=lxt2_rd_init(anno_ctx->aet_name);
			if(!lx2)
			        {
			        fprintf(stderr, "Could not initialize '%s', exiting.\n", anno_ctx->aet_name);
			        exit(255);
			        }
			break;

		case WAVE_ANNO_AE2:
#ifdef AET2_IS_PRESENT
			ae2_initialize(error_fn, msg_fn, alloc_fn, free_fn);
			if ( (!(aetf=fopen(anno_ctx->aet_name, "rb"))) || (!(ae2 = ae2_read_initialize(aetf))) )
        			{
        			fprintf(stderr, "Could not initialize '%s', exiting.\n", anno_ctx->aet_name);
        			exit(255);
        			}

			break;

#endif
		default:
			fprintf(stderr, "Unsupported wave file type %d encountered, exiting.\n", anno_ctx->aet_type);
			exit(255);
			break;
		}
	}

treebox("RTL Design Hierarchy", NULL, NULL);

g_timeout_add(100, update_ctx_when_idle, NULL);
gtk_main();

return(0);
}

