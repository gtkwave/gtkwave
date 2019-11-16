/*
 * Copyright (c) Tristan Gingold and Tony Bybell 2006-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <config.h>
#include "globals.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <ctype.h>
#include "gtk12compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "busy.h"
#include "debug.h"
#include "hierpack.h"
#include "tcl_helper.h"

WAVE_NODEVARTYPE_STR
WAVE_NODEVARDIR_STR
WAVE_NODEVARDATATYPE_STR

enum { VIEW_DRAG_INACTIVE, TREE_TO_VIEW_DRAG_ACTIVE, SEARCH_TO_VIEW_DRAG_ACTIVE };

/* Treesearch is a pop-up window used to select signals.
   It is composed of two main areas:
   * A tree area to select the hierarchy [tree area]
   * The (filtered) list of signals contained in the hierarchy [signal area].
*/

/* SIG_ROOT is the branch currently selected.
   Signals of SIG_ROOT are displayed in the signals window.  */

/* Only signals which match the filter are displayed in the signal area.  */

/* The signal area is based on a tree view which requires a store model.
   This store model contains the list of signals to be displayed.
*/
enum { NAME_COLUMN, TREE_COLUMN, TYPE_COLUMN, DIR_COLUMN, DTYPE_COLUMN, N_COLUMNS };

/* list of autocoalesced (synthesized) filter names that need to be freed at some point) */

void free_afl(void)
{
struct autocoalesce_free_list *at;

while(GLOBALS->afl_treesearch_gtk2_c_1)
	{
	if(GLOBALS->afl_treesearch_gtk2_c_1->name) free_2(GLOBALS->afl_treesearch_gtk2_c_1->name);
	at = GLOBALS->afl_treesearch_gtk2_c_1->next;
	free_2(GLOBALS->afl_treesearch_gtk2_c_1);
	GLOBALS->afl_treesearch_gtk2_c_1 = at;
	}
}


/* point to pure signame (remove hierarchy) for fill_sig_store() */
static char *prune_hierarchy(char *nam)
{
char cmpchar = GLOBALS->alt_hier_delimeter ? GLOBALS->alt_hier_delimeter : '.';
char *t = nam;
char *lastmatch = NULL;

while(t && *t)
	{
	if(*t == cmpchar) { lastmatch = t+1; }
	t++;
	}

return(lastmatch ? lastmatch : nam);
}

/* fix escaped signal names */
static void *fix_escaped_names(char *s, int do_free)
{
char *s2 = s;
int found = 0;

while(*s2)
	{
	if((*s2) == VCDNAM_ESCAPE)
		{
		found = 1;
		break;
		}
	s2++;
	}

if(found)
	{
	s2 = strdup_2(s);
	if(do_free) free_2(s);
	s = s2;

	while(*s2)
		{
		if(*s2 == VCDNAM_ESCAPE) { *s2 = GLOBALS->hier_delimeter; } /* restore back to normal */
		s2++;
		}
	}

return(s);
}


/* truncate VHDL types to string directly after final '.' */
char *varxt_fix(char *s)
{
char *pnt = strrchr(s, '.');
return(pnt ? (pnt+1) : s);
}


/* Fill the store model using current SIG_ROOT and FILTER_STR.  */
void
fill_sig_store (void)
{
  struct tree *t;
  struct tree *t_prev = NULL;
  GtkTreeIter iter;

  if(GLOBALS->selected_sig_name)
	{
	free_2(GLOBALS->selected_sig_name);
	GLOBALS->selected_sig_name = NULL;
	}

  free_afl();
  gtk_list_store_clear (GLOBALS->sig_store_treesearch_gtk2_c_1);

  for (t = GLOBALS->sig_root_treesearch_gtk2_c_1; t != NULL; t = t->next)
	{
	int i = t->t_which;
	char *s, *tmp2;
	int vartype;
	int vardir;
	int is_tname = 0;
	int wrexm;
	int vardt;
	unsigned int varxt;
	char *varxt_pnt;

	if(i < 0)
		{
		t_prev = NULL;
		continue;
		}

	if(t_prev) /* duplicates removal for faulty dumpers */
		{
		if(!strcmp(t_prev->name, t->name))
			{
			continue;
			}
		}
	t_prev = t;

	varxt = GLOBALS->facs[i]->n->varxt;
	varxt_pnt = varxt ? varxt_fix(GLOBALS->subvar_pnt[varxt]) : NULL;

	vartype = GLOBALS->facs[i]->n->vartype;
	if((vartype < 0) || (vartype > ND_VARTYPE_MAX))
		{
		vartype = 0;
		}

	vardir = GLOBALS->facs[i]->n->vardir; /* two bit already chops down to 0..3, but this doesn't hurt */
	if((vardir < 0) || (vardir > ND_DIR_MAX))
		{
		vardir = 0;
		}

	vardt = GLOBALS->facs[i]->n->vardt;
	if((vardt < 0) || (vardt > ND_VDT_MAX))
		{
		vardt = 0;
		}

        if(!GLOBALS->facs[i]->vec_root)
		{
		is_tname = 1;
		s = t->name;
		s = fix_escaped_names(s, 0);
                }
                else
                {
                if(GLOBALS->autocoalesce)
                	{
			char *p;
                        if(GLOBALS->facs[i]->vec_root!=GLOBALS->facs[i]) continue;

                        tmp2=makename_chain(GLOBALS->facs[i]);
			p = prune_hierarchy(tmp2);
                        s=(char *)malloc_2(strlen(p)+4);
                        strcpy(s,"[] ");
                        strcpy(s+3, p);
			s = fix_escaped_names(s, 1);
                        free_2(tmp2);
                        }
                        else
                        {
			char *p = prune_hierarchy(GLOBALS->facs[i]->name);
                        s=(char *)malloc_2(strlen(p)+4);
                        strcpy(s,"[] ");
                        strcpy(s+3, p);
			s = fix_escaped_names(s, 1);
                        }
                }

	wrexm = 0;
	if 	(
		(GLOBALS->filter_str_treesearch_gtk2_c_1 == NULL) ||
		((!GLOBALS->filter_noregex_treesearch_gtk2_c_1) && (wrexm = wave_regex_match(t->name, WAVE_REGEX_TREE)) && (!GLOBALS->filter_matlen_treesearch_gtk2_c_1)) ||
		(GLOBALS->filter_matlen_treesearch_gtk2_c_1 && ((GLOBALS->filter_typ_treesearch_gtk2_c_1 == vardir) ^ GLOBALS->filter_typ_polarity_treesearch_gtk2_c_1) &&
			(wrexm || (wrexm = wave_regex_match(t->name, WAVE_REGEX_TREE)))	)
		)
      		{
		gtk_list_store_prepend (GLOBALS->sig_store_treesearch_gtk2_c_1, &iter);
		if(is_tname)
			{
			gtk_list_store_set (GLOBALS->sig_store_treesearch_gtk2_c_1, &iter,
				    NAME_COLUMN, s,
				    TREE_COLUMN, t,
				    TYPE_COLUMN,
						(((GLOBALS->supplemental_datatypes_encountered) && (!GLOBALS->supplemental_vartypes_encountered)) ?
							(varxt ? varxt_pnt : vardatatype_strings[vardt]) : vartype_strings[vartype]),
				    DIR_COLUMN, vardir_strings[vardir],
				    DTYPE_COLUMN, varxt ? varxt_pnt : vardatatype_strings[vardt],
				    -1);

			if(s != t->name)
				{
				free_2(s);
				}
			}
			else
			{
			struct autocoalesce_free_list *a = calloc_2(1, sizeof(struct autocoalesce_free_list));
			a->name = s;
			a->next = GLOBALS->afl_treesearch_gtk2_c_1;
			GLOBALS->afl_treesearch_gtk2_c_1 = a;

			gtk_list_store_set (GLOBALS->sig_store_treesearch_gtk2_c_1, &iter,
				    NAME_COLUMN, s,
				    TREE_COLUMN, t,
				    TYPE_COLUMN,
						(((GLOBALS->supplemental_datatypes_encountered) && (!GLOBALS->supplemental_vartypes_encountered)) ?
							(varxt ? varxt_pnt : vardatatype_strings[vardt]) : vartype_strings[vartype]),
				    DIR_COLUMN, vardir_strings[vardir],
				    DTYPE_COLUMN, varxt ? varxt_pnt : vardatatype_strings[vardt],
				    -1);
			}
      		}
		else
		{
		if(s != t->name)
			{
			free_2(s);
			}
		}
	}
}

/*
 * tree open/close handling
 */
static void create_sst_nodes_if_necessary(GtkCTreeNode *node)
{
#ifndef WAVE_DISABLE_FAST_TREE
if(node)
	{
	GtkCTreeRow *gctr = GTK_CTREE_ROW(node);
	struct tree *t = (struct tree *)(gctr->row.data);

	if(t->child)
		{
		GtkCTree *ctree = GLOBALS->ctree_main;
		GtkCTreeNode *n = gctr->children;

		gtk_clist_freeze(GTK_CLIST(ctree));

		while(n)
			{
			GtkCTreeRow *g = GTK_CTREE_ROW(n);

			t = (struct tree *)(g->row.data);
			if(t->t_which < 0)
				{
				if(!t->children_in_gui)
					{
					t->children_in_gui = 1;
					maketree2(n, t, 0, n);
					}
				}

			n = GTK_CTREE_NODE_NEXT(n);
			}

		gtk_clist_thaw(GTK_CLIST(ctree));
		}
	}
#endif
}


int force_open_tree_node(char *name, int keep_path_nodes_open, struct tree **t_pnt) {
  GtkCTree *ctree = GLOBALS->ctree_main;
  int rv = 1 ;			/* can possible open */
  if(ctree && GLOBALS->any_tree_node) {
    int namlen = strlen(name);
    char *namecache = wave_alloca(namlen+1);
    char *name_end = name + namlen - 1;
    char *zap = name;
    GtkCTreeNode *node = GLOBALS->any_tree_node;
    GtkCTreeRow *gctr = GTK_CTREE_ROW(node);
    int depth = 1;

    strcpy(namecache, name);
    while(gctr->parent)	{
      node = gctr->parent;
      gctr = GTK_CTREE_ROW(node);
    }
    for(;;) {
      struct tree *t = gctr->row.data;
      if(t_pnt) { *t_pnt = t; }
      while(*zap) {
	if(*zap != GLOBALS->hier_delimeter) {
	  zap++;
	}
	else {
	  *zap = 0;
	  break;
	}
      }
      if(!strcmp(t->name, name)) {
	if(zap == name_end) {
	  GtkCTreeNode **nodehist = wave_alloca(depth * sizeof(GtkCTreeNode *));
	  int *exp1 = wave_alloca(depth * sizeof(int));
	  int i = depth-1;
	  nodehist[i] = node;
	  exp1[i--] = 1;
	  /* now work backwards up to parent getting the node open/close history*/
	  gctr = GTK_CTREE_ROW(node);
	  while(gctr->parent) {
	    node = gctr->parent;
	    gctr = GTK_CTREE_ROW(node);
	    nodehist[i] = node;
	    exp1[i--] = gctr->expanded || (keep_path_nodes_open != 0);
	  }
	  gtk_clist_freeze(GTK_CLIST(ctree));
	  /* fully expand down */
	  for(i=0;i<depth;i++) {
	    gtk_ctree_expand(ctree, nodehist[i]);
	  }
	  /* work backwards and close up nodes that were originally closed */
	  for(i=depth-1;i>=0;i--) {
	    if(exp1[i]) {
	      gtk_ctree_expand(ctree, nodehist[i]);
	    }
	    else {
	      gtk_ctree_collapse(ctree, nodehist[i]);
	    }
	  }
	  gtk_clist_thaw(GTK_CLIST(ctree));

	  gtk_ctree_node_moveto(ctree, nodehist[depth-1],
				depth /*column*/,
				0.5 /*row_align*/,
				0.5 /*col_align*/);
	  /* printf("[treeopennode] '%s' ok\n", name); */
	  rv = 0 ;		/* opened */
	  GLOBALS->open_tree_nodes = xl_insert(namecache, GLOBALS->open_tree_nodes, NULL);
	  return rv;
	}
	else {
	  depth++;
	  create_sst_nodes_if_necessary(node);
	  node = gctr->children;
	  if(!node) break;
	  gctr = GTK_CTREE_ROW(node);
	  if(!gctr) break;
	  name = ++zap;
	  continue;
	}
      }
      node = gctr->sibling;
      if(!node) break;
      gctr = GTK_CTREE_ROW(node);
    }
     /* printf("[treeopennode] '%s' failed\n", name); */
  } else {
    rv = -1 ;			/* tree does not exist */
  }
  return rv ;
}

void dump_open_tree_nodes(FILE *wave, xl_Tree *t)
{
if(t->left)
	{
	dump_open_tree_nodes(wave, t->left);
	}

fprintf(wave, "[treeopen] %s\n", t->item);

if(t->right)
	{
	dump_open_tree_nodes(wave, t->right);
	}
}

static void generic_tree_expand_collapse_callback(int is_expand, GtkCTreeNode *node)
{
GtkCTreeRow **gctr;
int depth, i;
int len = 1;
char *tstring;
char hier_suffix[2];
int found;

hier_suffix[0] = GLOBALS->hier_delimeter;
hier_suffix[1] = 0;

if(!node) return;

depth = GTK_CTREE_ROW(node)->level;
gctr = wave_alloca(depth * sizeof(GtkCTreeRow *));

for(i=depth-1;i>=0;i--)
	{
	struct tree *t;
	gctr[i] = GTK_CTREE_ROW(node);
	t = gctr[i]->row.data;
	len += (strlen(t->name) + 1);
	node = gctr[i]->parent;
	}

tstring = wave_alloca(len);
memset(tstring, 0, len);

for(i=0;i<depth;i++)
	{
	struct tree *t = gctr[i]->row.data;
	strcat(tstring, t->name);
	strcat(tstring, hier_suffix);
	}


if(GLOBALS->open_tree_nodes) /* cut down on chatter to Tcl clients */
	{
	GLOBALS->open_tree_nodes = xl_splay(tstring, GLOBALS->open_tree_nodes);
	if(!strcmp(GLOBALS->open_tree_nodes->item, tstring))
		{
		found = 1;
		}
		else
		{
		found = 0;
		}
	}
	else
	{
	found = 0;
	}

if(is_expand)
	{
	GLOBALS->open_tree_nodes = xl_insert(tstring, GLOBALS->open_tree_nodes, NULL);
	if(!found)
		{
		gtkwavetcl_setvar(WAVE_TCLCB_TREE_EXPAND, tstring, WAVE_TCLCB_TREE_EXPAND_FLAGS);
		}
	}
	else
	{
	GLOBALS->open_tree_nodes = xl_delete(tstring, GLOBALS->open_tree_nodes);
	if(found)
		{
		gtkwavetcl_setvar(WAVE_TCLCB_TREE_COLLAPSE, tstring, WAVE_TCLCB_TREE_COLLAPSE_FLAGS);
		}
	}
}

static void tree_expand_callback(GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data)
{
(void)ctree;
(void)user_data;

create_sst_nodes_if_necessary(node);
generic_tree_expand_collapse_callback(1, node);
#ifdef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND
#ifdef MAC_INTEGRATION
/* workaround for ctree not rendering properly in OSX */
gtk_widget_hide(GTK_WIDGET(GLOBALS->tree_treesearch_gtk2_c_1));
gtk_widget_show(GTK_WIDGET(GLOBALS->tree_treesearch_gtk2_c_1));
#endif
#endif
}

static void tree_collapse_callback(GtkCTree *ctree, GtkCTreeNode *node, gpointer user_data)
{
(void)ctree;
(void)user_data;

generic_tree_expand_collapse_callback(0, node);
#ifdef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND
#ifdef MAC_INTEGRATION
/* workaround for ctree not rendering properly in OSX */
gtk_widget_hide(GTK_WIDGET(GLOBALS->tree_treesearch_gtk2_c_1));
gtk_widget_show(GTK_WIDGET(GLOBALS->tree_treesearch_gtk2_c_1));
#endif
#endif
}


void select_tree_node(char *name)
{
GtkCTree *ctree = GLOBALS->ctree_main;

if(ctree && GLOBALS->any_tree_node)
	{
	int namlen = strlen(name);
	char *namecache = wave_alloca(namlen+1);
	char *name_end = name + namlen - 1;
	char *zap = name;
	GtkCTreeNode *node = GLOBALS->any_tree_node;
	GtkCTreeRow *gctr = GTK_CTREE_ROW(node);

	strcpy(namecache, name);

	while(gctr->parent)
		{
		node = gctr->parent;
		gctr = GTK_CTREE_ROW(node);
		}

	for(;;)
		{
		struct tree *t = gctr->row.data;

		while(*zap)
			{
			if(*zap != GLOBALS->hier_delimeter)
				{
				zap++;
				}
				else
				{
				*zap = 0;
				break;
				}
			}

		if(!strcmp(t->name, name))
			{
			if(zap == name_end)
				{
				/* printf("[treeselectnode] '%s' ok\n", name); */
				gtk_ctree_select(ctree, node);

				GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = t;
				GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
				fill_sig_store ();

				return;
				}
				else
				{
				node = gctr->children;
				gctr = GTK_CTREE_ROW(node);
				if(!gctr) break;

				name = ++zap;
				continue;
				}
			}

		node = gctr->sibling;
		if(!node) break;
		gctr = GTK_CTREE_ROW(node);
		}

	/* printf("[treeselectnode] '%s' failed\n", name); */
	}
}



/* Callbacks for tree area when a row is selected/deselected.  */
static void select_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
(void)widget;
(void)column;
(void)event;
(void)data;

struct tree *t;
GtkCTreeNode* node = gtk_ctree_node_nth(GLOBALS->ctree_main, row);

if(node)
	{
	GtkCTreeRow **gctr;
	int depth, i;
	int len = 1;
	char *tstring;
	char hier_suffix[2];

	hier_suffix[0] = GLOBALS->hier_delimeter;
	hier_suffix[1] = 0;

	depth = GTK_CTREE_ROW(node)->level;
	gctr = wave_alloca(depth * sizeof(GtkCTreeRow *));

	for(i=depth-1;i>=0;i--)
	        {
	        gctr[i] = GTK_CTREE_ROW(node);
	        t = gctr[i]->row.data;
	        len += (strlen(t->name) + 1);
	        node = gctr[i]->parent;
	        }

	tstring = wave_alloca(len);
	memset(tstring, 0, len);

	for(i=0;i<depth;i++)
	        {
	        t = gctr[i]->row.data;
	        strcat(tstring, t->name);
	        strcat(tstring, hier_suffix);
	        }

	if(GLOBALS->selected_hierarchy_name)
		{
		free_2(GLOBALS->selected_hierarchy_name);
		}
	GLOBALS->selected_hierarchy_name = strdup_2(tstring);
	}

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->ctree_main), row);
DEBUG(printf("TS: %08x %s\n",t,t->name));
 GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = t;
 GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
 fill_sig_store ();
 gtkwavetcl_setvar(WAVE_TCLCB_TREE_SELECT, GLOBALS->selected_hierarchy_name, WAVE_TCLCB_TREE_SELECT_FLAGS);
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
(void)widget;
(void)column;
(void)event;
(void)data;

struct tree *t;

if(GLOBALS->selected_hierarchy_name)
	{
	gtkwavetcl_setvar(WAVE_TCLCB_TREE_UNSELECT, GLOBALS->selected_hierarchy_name, WAVE_TCLCB_TREE_UNSELECT_FLAGS);
	free_2(GLOBALS->selected_hierarchy_name);
	GLOBALS->selected_hierarchy_name = NULL;
	}

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->ctree_main), row);
if(t)
	{
	/* unused */
	}
DEBUG(printf("TU: %08x %s\n",t,t->name));
 GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = NULL;
 GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
 fill_sig_store ();
}

/* Signal callback for the filter widget.
   This catch the return key to update the signal area.  */
static
gboolean filter_edit_cb (GtkWidget *widget, GdkEventKey *ev, gpointer *data)
{
(void)data;

  /* Maybe this test is too strong ?  */
  if (ev->keyval == GDK_Return)
    {
      const char *t;

      /* Get the filter string, save it and change the store.  */
      if(GLOBALS->filter_str_treesearch_gtk2_c_1)
	{
      	free_2((char *)GLOBALS->filter_str_treesearch_gtk2_c_1);
	GLOBALS->filter_str_treesearch_gtk2_c_1 = NULL;
	}
      t = gtk_entry_get_text (GTK_ENTRY (widget));
      if (t == NULL || *t == 0)
	GLOBALS->filter_str_treesearch_gtk2_c_1 = NULL;
      else
	{
	int i;

	GLOBALS->filter_str_treesearch_gtk2_c_1 = malloc_2(strlen(t) + 1);
	strcpy(GLOBALS->filter_str_treesearch_gtk2_c_1, t);

	GLOBALS->filter_typ_treesearch_gtk2_c_1 = ND_DIR_UNSPECIFIED;
	GLOBALS->filter_typ_polarity_treesearch_gtk2_c_1 = 0;
	GLOBALS->filter_matlen_treesearch_gtk2_c_1 = 0;
	GLOBALS->filter_noregex_treesearch_gtk2_c_1 = 0;

	if(GLOBALS->filter_str_treesearch_gtk2_c_1[0] == '+')
		{
		for(i=0;i<=ND_DIR_MAX;i++)
			{
			int tlen = strlen(vardir_strings[i]);
			if(!strncasecmp(vardir_strings[i], GLOBALS->filter_str_treesearch_gtk2_c_1 + 1, tlen))
				{
				if(GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 1] == '+')
					{
					GLOBALS->filter_matlen_treesearch_gtk2_c_1 = tlen + 2;
					GLOBALS->filter_typ_treesearch_gtk2_c_1 = i;
					if(GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 2] == 0)
						{
						GLOBALS->filter_noregex_treesearch_gtk2_c_1 = 1;
						}
					}
				}
			}
		}
	else
	if(GLOBALS->filter_str_treesearch_gtk2_c_1[0] == '-')
		{
		for(i=0;i<=ND_DIR_MAX;i++)
			{
			int tlen = strlen(vardir_strings[i]);
			if(!strncasecmp(vardir_strings[i], GLOBALS->filter_str_treesearch_gtk2_c_1 + 1, tlen))
				{
				if(GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 1] == '-')
					{
					GLOBALS->filter_matlen_treesearch_gtk2_c_1 = tlen + 2;
					GLOBALS->filter_typ_treesearch_gtk2_c_1 = i;
					GLOBALS->filter_typ_polarity_treesearch_gtk2_c_1 = 1; /* invert via XOR with 1 */
					if(GLOBALS->filter_str_treesearch_gtk2_c_1[tlen + 2] == 0)
						{
						GLOBALS->filter_noregex_treesearch_gtk2_c_1 = 1;
						}
					}
				}
			}
		}

	wave_regex_compile(GLOBALS->filter_str_treesearch_gtk2_c_1 + GLOBALS->filter_matlen_treesearch_gtk2_c_1, WAVE_REGEX_TREE);
	}
      fill_sig_store ();
    }
  return FALSE;
}


/*
 * for dynamic updates, simply fake the return key to the function above
 */
static
void press_callback (GtkWidget *widget, gpointer *data)
{
GdkEventKey ev;

ev.keyval = GDK_Return;

filter_edit_cb (widget, &ev, data);
}


/*
 * select/unselect all in treeview
 */
void treeview_select_all_callback(void)
{
GtkTreeSelection* ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
gtk_tree_selection_select_all(ts);
}

void treeview_unselect_all_callback(void)
{
GtkTreeSelection* ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
gtk_tree_selection_unselect_all(ts);
}


int treebox_is_active(void)
{
return(GLOBALS->is_active_treesearch_gtk2_c_6);
}

#if 0
static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  G_CONST_RETURN gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_a_treesearch_gtk2_c_2));
  entry_text = entry_text ? entry_text : "";
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=NULL;
	else strcpy((GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=(char *)malloc_2(len+1)),entry_text);

  wave_gtk_grab_remove(GLOBALS->window1_treesearch_gtk2_c_3);
  gtk_widget_destroy(GLOBALS->window1_treesearch_gtk2_c_3);
  GLOBALS->window1_treesearch_gtk2_c_3 = NULL;

  GLOBALS->cleanup_e_treesearch_gtk2_c_3();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  DEBUG(printf("Entry Cancel\n"));
  GLOBALS->entrybox_text_local_treesearch_gtk2_c_3=NULL;
  wave_gtk_grab_remove(GLOBALS->window1_treesearch_gtk2_c_3);
  gtk_widget_destroy(GLOBALS->window1_treesearch_gtk2_c_3);
  GLOBALS->window1_treesearch_gtk2_c_3 = NULL;
}

static void entrybox_local(char *title, int width, char *default_text, int maxch, GtkSignalFunc func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    GLOBALS->cleanup_e_treesearch_gtk2_c_3=func;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    /* create a new modal window */
    GLOBALS->window1_treesearch_gtk2_c_3 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window1_treesearch_gtk2_c_3, ((char *)&GLOBALS->window1_treesearch_gtk2_c_3) - ((char *)GLOBALS));

    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window1_treesearch_gtk2_c_3), width, 60);
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window1_treesearch_gtk2_c_3), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window1_treesearch_gtk2_c_3), "delete_event",(GtkSignalFunc) destroy_callback_e, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window1_treesearch_gtk2_c_3), vbox);
    gtk_widget_show (vbox);

    GLOBALS->entry_a_treesearch_gtk2_c_2 = gtk_entry_new_with_max_length (maxch);
    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->entry_a_treesearch_gtk2_c_2), "activate",GTK_SIGNAL_FUNC(enter_callback_e),GLOBALS->entry_a_treesearch_gtk2_c_2);
    gtk_entry_set_text (GTK_ENTRY (GLOBALS->entry_a_treesearch_gtk2_c_2), default_text);
    gtk_entry_select_region (GTK_ENTRY (GLOBALS->entry_a_treesearch_gtk2_c_2),0, GTK_ENTRY(GLOBALS->entry_a_treesearch_gtk2_c_2)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), GLOBALS->entry_a_treesearch_gtk2_c_2, TRUE, TRUE, 0);
    gtk_widget_show (GLOBALS->entry_a_treesearch_gtk2_c_2);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("OK");
    gtk_widget_set_usize(button1, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(enter_callback_e), NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "realize", (GtkSignalFunc) gtk_widget_grab_default, GTK_OBJECT (button1));

    button2 = gtk_button_new_with_label ("Cancel");
    gtk_widget_set_usize(button2, 100, -1);
    gtkwave_signal_connect(GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(destroy_callback_e), NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(GLOBALS->window1_treesearch_gtk2_c_3);
    wave_gtk_grab_add(GLOBALS->window1_treesearch_gtk2_c_3);
}
#endif

/***************************************************************************/

/* Callback for insert/replace/append buttions.
   This call-back is called for every signal selected.  */

static void
sig_selection_foreach (GtkTreeModel *model,
		       GtkTreePath *path,
		       GtkTreeIter *iter,
		       gpointer data)
{
(void)path;
(void)data;

  struct tree *sel;
  /* const enum sst_cb_action action = (enum sst_cb_action)data; */
  int i;
  int low, high;

  /* Get the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

  low = fetchlow(sel)->t_which;
  high = fetchhigh(sel)->t_which;

  /* Add signals and vectors.  */
  for(i=low;i<=high;i++)
        {
	int len;
        struct symbol *s, *t;
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t,0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);
		}
        }
}

static void
sig_selection_foreach_finalize (gpointer data)
{
 const enum sst_cb_action action = (enum sst_cb_action)data;

 if (action == SST_ACTION_REPLACE || action == SST_ACTION_INSERT || action == SST_ACTION_PREPEND)
   {
     Trptr tfirst=NULL, tlast=NULL;
     Trptr t;
     Trptr *tp = NULL;
     int numhigh = 0;
     int it;

     if (action == SST_ACTION_REPLACE)
       {
	tfirst=GLOBALS->traces.first; tlast=GLOBALS->traces.last; /* cache for highlighting */
       }

     GLOBALS->traces.buffercount=GLOBALS->traces.total;
     GLOBALS->traces.buffer=GLOBALS->traces.first;
     GLOBALS->traces.bufferlast=GLOBALS->traces.last;
     GLOBALS->traces.first=GLOBALS->tcache_treesearch_gtk2_c_2.first;
     GLOBALS->traces.last=GLOBALS->tcache_treesearch_gtk2_c_2.last;
     GLOBALS->traces.total=GLOBALS->tcache_treesearch_gtk2_c_2.total;

     if (action == SST_ACTION_REPLACE)
       {
	t = GLOBALS->traces.first;
	while(t) { if(t->flags & TR_HIGHLIGHT) { numhigh++; } t = t->t_next; }
	if(numhigh)
	        {
	        tp = calloc_2(numhigh, sizeof(Trptr));
	        t = GLOBALS->traces.first;
	        it = 0;
	        while(t) { if(t->flags & TR_HIGHLIGHT) { tp[it++] = t; } t = t->t_next; }
	        }
       }

     if(action == SST_ACTION_PREPEND)
	{
	PrependBuffer();
	}
	else
	{
	PasteBuffer();
	}

     GLOBALS->traces.buffercount=GLOBALS->tcache_treesearch_gtk2_c_2.buffercount;
     GLOBALS->traces.buffer=GLOBALS->tcache_treesearch_gtk2_c_2.buffer;
     GLOBALS->traces.bufferlast=GLOBALS->tcache_treesearch_gtk2_c_2.bufferlast;

     if (action == SST_ACTION_REPLACE)
       {
	for(it=0;it<numhigh;it++)
	        {
	        tp[it]->flags |= TR_HIGHLIGHT;
	        }

	t = tfirst;
	while(t)
	        {
	        t->flags &= ~TR_HIGHLIGHT;
	        if(t==tlast) break;
	        t=t->t_next;
	        }

	CutBuffer();

	while(tfirst)
	        {
	        tfirst->flags |= TR_HIGHLIGHT;
	        if(tfirst==tlast) break;
	        tfirst=tfirst->t_next;
	        }

	if(tp)
	        {
	        free_2(tp);
	        }
       }
   }
}

static void
sig_selection_foreach_preload_lx2
		      (GtkTreeModel *model,
		       GtkTreePath *path,
		       GtkTreeIter *iter,
		       gpointer data)
{
(void)path;
(void)data;

  struct tree *sel;
  /* const enum sst_cb_action action = (enum sst_cb_action)data; */
  int i;
  int low, high;

  /* Get the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

  low = fetchlow(sel)->t_which;
  high = fetchhigh(sel)->t_which;

  /* If signals are vectors, coalesces vectors if so.  */
  for(i=low;i<=high;i++)
        {
        struct symbol *s;
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

  /* LX2 */
  if(GLOBALS->is_lx2)
        {
        for(i=low;i<=high;i++)
                {
                struct symbol *s, *t;
                s=GLOBALS->facs[i];
                t=s->vec_root;
                if((t)&&(GLOBALS->autocoalesce))
                        {
                        if(get_s_selected(t))
                                {
                                while(t)
                                        {
                                        if(t->n->mv.mvlfac)
                                                {
                                                lx2_set_fac_process_mask(t->n);
                                                GLOBALS->pre_import_treesearch_gtk2_c_1++;
                                                }
                                        t=t->vec_chain;
                                        }
                                }
                        }
                        else
                        {
                        if(s->n->mv.mvlfac)
                                {
                                lx2_set_fac_process_mask(s->n);
                                GLOBALS->pre_import_treesearch_gtk2_c_1++;
                                }
                        }
                }
        }
  /* LX2 */
}


static void
action_callback(enum sst_cb_action action)
{
  if(action == SST_ACTION_NONE) return; /* only used for double-click in signals pane of SST */

  GLOBALS->pre_import_treesearch_gtk2_c_1 = 0;

  /* once through to mass gather lx2 traces... */
  gtk_tree_selection_selected_foreach
    (GLOBALS->sig_selection_treesearch_gtk2_c_1, &sig_selection_foreach_preload_lx2, (void *)action);
  if(GLOBALS->pre_import_treesearch_gtk2_c_1)
	{
        lx2_import_masked();
        }

  /* then do */
  if (action == SST_ACTION_INSERT || action == SST_ACTION_REPLACE || action == SST_ACTION_PREPEND)
    {
      /* Save and clear current traces.  */
      memcpy(&GLOBALS->tcache_treesearch_gtk2_c_2,&GLOBALS->traces,sizeof(Traces));
      GLOBALS->traces.total=0;
      GLOBALS->traces.first=GLOBALS->traces.last=NULL;
    }

  gtk_tree_selection_selected_foreach
    (GLOBALS->sig_selection_treesearch_gtk2_c_1, &sig_selection_foreach, (void *)action);

  sig_selection_foreach_finalize((void *)action);

  if(action == SST_ACTION_APPEND)
	{
	GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = GLOBALS->traces.last;
	}
  MaxSignalLength();
  signalarea_configure_event(GLOBALS->signalarea, NULL);
  wavearea_configure_event(GLOBALS->wavearea, NULL);
}

static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

  set_window_busy(widget);
  action_callback (SST_ACTION_INSERT);
  set_window_idle(widget);
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

  set_window_busy(widget);
  action_callback (SST_ACTION_REPLACE);
  set_window_idle(widget);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

  set_window_busy(widget);
  action_callback (SST_ACTION_APPEND);
  set_window_idle(widget);
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  GLOBALS->is_active_treesearch_gtk2_c_6=0;
  gtk_widget_destroy(GLOBALS->window_treesearch_gtk2_c_12);
  GLOBALS->window_treesearch_gtk2_c_12 = NULL;
  GLOBALS->dnd_sigview = NULL;
  GLOBALS->gtk2_tree_frame = NULL;
  GLOBALS->ctree_main = NULL;
  GLOBALS->filter_entry = NULL; /* when treebox() SST goes away after closed and rc hide_sst is true */
  free_afl();

  if(GLOBALS->selected_hierarchy_name)
	{
	free_2(GLOBALS->selected_hierarchy_name);
	GLOBALS->selected_hierarchy_name = NULL;
	}
}

/**********************************************************************/

static gboolean
  view_selection_func (GtkTreeSelection *selection,
                       GtkTreeModel     *model,
                       GtkTreePath      *path,
                       gboolean          path_currently_selected,
                       gpointer          userdata)
  {
(void)selection;
(void)userdata;

    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
      gchar *name;

      gtk_tree_model_get(model, &iter, 0, &name, -1);

      if(GLOBALS->selected_sig_name)
	{
	free_2(GLOBALS->selected_sig_name);
	GLOBALS->selected_sig_name = NULL;
	}

      if (!path_currently_selected)
      {
	GLOBALS->selected_sig_name = strdup_2(name);
	gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_SELECT, name, WAVE_TCLCB_TREE_SIG_SELECT_FLAGS);
      }
      else
      {
	gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_UNSELECT, name, WAVE_TCLCB_TREE_SIG_UNSELECT_FLAGS);
      }

      g_free(name);
    }
    return TRUE; /* allow selection state to change */
  }

/**********************************************************************/

static gint button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
(void)widget;

if(event->type == GDK_2BUTTON_PRESS)
	{
	if(GLOBALS->selected_hierarchy_name && GLOBALS->selected_sig_name)
		{
		char *sstr = wave_alloca(strlen(GLOBALS->selected_hierarchy_name) + strlen(GLOBALS->selected_sig_name) + 1);
		strcpy(sstr, GLOBALS->selected_hierarchy_name);
		strcat(sstr, GLOBALS->selected_sig_name);

		gtkwavetcl_setvar(WAVE_TCLCB_TREE_SIG_DOUBLE_CLICK, sstr, WAVE_TCLCB_TREE_SIG_DOUBLE_CLICK_FLAGS);
		action_callback(GLOBALS->sst_dbl_action_type);
		}
	}

return(FALSE);
}

static gint hier_top_button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
if((event->button == 3) && (event->type == GDK_BUTTON_PRESS))
        {
	if(GLOBALS->sst_sig_root_treesearch_gtk2_c_1)
		{
	        do_sst_popup_menu (widget, event);
		return(TRUE);
		}
        }

return(FALSE);
}

/**********************************************************************/

/*
 * mainline..
 */
void treebox(char *title, GtkSignalFunc func, GtkWidget *old_window)
{
    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button4, *button5;
    GtkWidget *frameh, *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *filter_label;
    GtkWidget *sig_view;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    if(old_window)
	{
	GLOBALS->is_active_treesearch_gtk2_c_6=1;
    	GLOBALS->cleanup_treesearch_gtk2_c_8=func;
	goto do_tooltips;
	}

    if(GLOBALS->is_active_treesearch_gtk2_c_6)
	{
	if(GLOBALS->window_treesearch_gtk2_c_12)
		{
		gdk_window_raise(GLOBALS->window_treesearch_gtk2_c_12->window);
		}
		else
		{
#if GTK_CHECK_VERSION(2,4,0)
		if(GLOBALS->expanderwindow)
			{
			gtk_expander_set_expanded(GTK_EXPANDER(GLOBALS->expanderwindow), TRUE);
			}
#endif
		}
	return;
	}

    GLOBALS->is_active_treesearch_gtk2_c_6=1;
    GLOBALS->cleanup_treesearch_gtk2_c_8=func;

    /* create a new modal window */
    GLOBALS->window_treesearch_gtk2_c_12 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_treesearch_gtk2_c_12, ((char *)&GLOBALS->window_treesearch_gtk2_c_12) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_treesearch_gtk2_c_12), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12), "delete_event",(GtkSignalFunc) destroy_callback, NULL);

do_tooltips:
    tooltips=gtk_tooltips_new_2();

    GLOBALS->treesearch_gtk2_window_vbox = vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    gtkwave_signal_connect(GTK_OBJECT(vbox), "button_press_event",GTK_SIGNAL_FUNC(hier_top_button_press_event_std), NULL);

    vpan = gtk_vpaned_new ();
    gtk_widget_show (vpan);
    gtk_box_pack_start (GTK_BOX (vbox), vpan, TRUE, TRUE, 1);

    /* Hierarchy.  */
    GLOBALS->gtk2_tree_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), 3);
    gtk_widget_show(GLOBALS->gtk2_tree_frame);

    gtk_paned_pack1 (GTK_PANED (vpan), GLOBALS->gtk2_tree_frame, TRUE, FALSE);

    GLOBALS->tree_treesearch_gtk2_c_1=gtk_ctree_new(1,0);
    GLOBALS->ctree_main=GTK_CTREE(GLOBALS->tree_treesearch_gtk2_c_1);
    gtk_clist_set_column_auto_resize (GTK_CLIST (GLOBALS->tree_treesearch_gtk2_c_1), 0, TRUE);
    gtk_widget_show(GLOBALS->tree_treesearch_gtk2_c_1);

    gtk_clist_set_use_drag_icons(GTK_CLIST (GLOBALS->tree_treesearch_gtk2_c_1), FALSE);

    clist=GTK_CLIST(GLOBALS->tree_treesearch_gtk2_c_1);

    gtkwave_signal_connect_object (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC(select_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "unselect_row", GTK_SIGNAL_FUNC(unselect_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_expand", GTK_SIGNAL_FUNC(tree_expand_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_collapse", GTK_SIGNAL_FUNC(tree_collapse_callback), NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    decorated_module_cleanup();
    maketree(NULL, GLOBALS->treeroot);
    gtk_clist_thaw(clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (GLOBALS->tree_treesearch_gtk2_c_1));
    gtk_container_add (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), scrolled_win);


    /* Signal names.  */
    GLOBALS->sig_store_treesearch_gtk2_c_1 = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = NULL;
    GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
    fill_sig_store ();

    sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (GLOBALS->sig_store_treesearch_gtk2_c_1));
    gtkwave_signal_connect(GTK_OBJECT(sig_view), "button_press_event",GTK_SIGNAL_FUNC(hier_top_button_press_event_std), NULL);

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (GLOBALS->sig_store_treesearch_gtk2_c_1));


      {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();

	switch(GLOBALS->loaded_file_type)
		{
#ifdef EXTLOAD_SUFFIX
		case EXTLOAD_FILE:
#endif
		case FST_FILE:
					/* fallthrough for Dir is deliberate for extload and FST */
					if(GLOBALS->nonimplicit_direction_encountered)
						{
						column = gtk_tree_view_column_new_with_attributes ("Dir",
								   renderer,
								   "text", DIR_COLUMN,
								   NULL);
						gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
						}
					/* fallthrough */
		case AE2_FILE:
		case VCD_FILE:
		case VCD_RECODER_FILE:
		case DUMPLESS_FILE:
					column = gtk_tree_view_column_new_with_attributes (((GLOBALS->supplemental_datatypes_encountered) && (GLOBALS->supplemental_vartypes_encountered)) ? "VType" : "Type",
							   renderer,
							   "text", TYPE_COLUMN,
							   NULL);
					gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
					if((GLOBALS->supplemental_datatypes_encountered) && (GLOBALS->supplemental_vartypes_encountered))
						{
						column = gtk_tree_view_column_new_with_attributes ("DType",
							   renderer,
							   "text", DTYPE_COLUMN,
							   NULL);
						gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
						}
					break;
		default:
			 		break;
		}

	column = gtk_tree_view_column_new_with_attributes ("Signals",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);


	/* Setup the selection handler */
	GLOBALS->sig_selection_treesearch_gtk2_c_1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
	gtk_tree_selection_set_mode (GLOBALS->sig_selection_treesearch_gtk2_c_1, GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function (GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                                view_selection_func, NULL, NULL);

	gtkwave_signal_connect(GTK_OBJECT(sig_view), "button_press_event",GTK_SIGNAL_FUNC(button_press_event_std), NULL);
      }

    GLOBALS->dnd_sigview = sig_view;
    dnd_setup(GLOBALS->dnd_sigview, GLOBALS->signalarea, 0);

    sig_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (sig_frame), 3);
    gtk_widget_show(sig_frame);

    gtk_paned_pack2 (GTK_PANED (vpan), sig_frame, TRUE, FALSE);

    sig_scroll_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize (GTK_WIDGET (sig_scroll_win), 80, 100);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sig_scroll_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_frame), sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_scroll_win), sig_view);
    gtk_widget_show (sig_view);


    /* Filter.  */
    filter_hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (filter_hbox);

    filter_label = gtk_label_new ("Filter:");
    gtk_widget_show (filter_label);
    gtk_box_pack_start (GTK_BOX (filter_hbox), filter_label, FALSE, FALSE, 1);

    GLOBALS->filter_entry = gtk_entry_new ();
    if(GLOBALS->filter_str_treesearch_gtk2_c_1)
	{
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry), GLOBALS->filter_str_treesearch_gtk2_c_1);
	}

    gtk_widget_show (GLOBALS->filter_entry);

    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "activate", GTK_SIGNAL_FUNC(press_callback), NULL);
    if(!GLOBALS->do_dynamic_treefilter)
	{
    	gtkwave_signal_connect(GTK_OBJECT (GLOBALS->filter_entry), "key_press_event", (GtkSignalFunc) filter_edit_cb, NULL);
	}
	else
	{
    	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "changed", GTK_SIGNAL_FUNC(press_callback), NULL);
	}

    gtk_tooltips_set_tip_2(tooltips, GLOBALS->filter_entry,
	   "Add a POSIX filter. "
	   "'.*' matches any number of characters,"
	   " '.' matches any character.  Hit Return to apply."
	   " The filter may be preceded with the port direction if it exists such as ++ (show only non-port), +I+, +O+, +IO+, etc."
	   " Use -- to exclude all non-ports (i.e., show only all ports), -I- to exclude all input ports, etc.",
	   NULL);

    gtk_box_pack_start (GTK_BOX (filter_hbox), GLOBALS->filter_entry, FALSE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX (vbox), filter_hbox, FALSE, FALSE, 1);

    /* Buttons.  */
    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_box_pack_start (GTK_BOX (vbox), frameh, FALSE, FALSE, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked",GTK_SIGNAL_FUNC(ok_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1,
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button2), "clicked",GTK_SIGNAL_FUNC(insert_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2,
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    button4 = gtk_button_new_with_label (" Replace ");
    gtk_container_border_width (GTK_CONTAINER (button4), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button4), "clicked",GTK_SIGNAL_FUNC(replace_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4,
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Exit ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button5), "clicked",GTK_SIGNAL_FUNC(destroy_callback),GTK_OBJECT (GLOBALS->window_treesearch_gtk2_c_12));
    gtk_tooltips_set_tip_2(tooltips, button5,
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_treesearch_gtk2_c_12), vbox);

    gtk_window_set_default_size (GTK_WINDOW (GLOBALS->window_treesearch_gtk2_c_12), 200, 400);
    gtk_widget_show(GLOBALS->window_treesearch_gtk2_c_12);
}


/*
 * for use with expander in gtk2.4 and higher...
 */
GtkWidget* treeboxframe(char *title, GtkSignalFunc func)
{
(void)title;

    GtkWidget *scrolled_win, *sig_scroll_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button4;
    GtkWidget *frameh, *sig_frame;
    GtkWidget *vbox, *vpan, *filter_hbox;
    GtkWidget *filter_label;
    GtkWidget *sig_view;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    GLOBALS->is_active_treesearch_gtk2_c_6=1;
    GLOBALS->cleanup_treesearch_gtk2_c_8=func;

    /* create a new modal window */
    tooltips=gtk_tooltips_new_2();

    vbox = gtk_vbox_new (FALSE, 1);
    gtk_widget_show (vbox);

    gtkwave_signal_connect(GTK_OBJECT(vbox), "button_press_event",GTK_SIGNAL_FUNC(hier_top_button_press_event_std), NULL);

    vpan = gtk_vpaned_new (); /* GLOBALS->sst_vpaned is to be used to clone position over during reload */
    GLOBALS->sst_vpaned = (GtkPaned *)vpan;
    if(GLOBALS->vpanedwindow_size_cache)
	{
	gtk_paned_set_position(GTK_PANED(GLOBALS->sst_vpaned), GLOBALS->vpanedwindow_size_cache);
	GLOBALS->vpanedwindow_size_cache = 0;
	}
    gtk_widget_show (vpan);
    gtk_box_pack_start (GTK_BOX (vbox), vpan, TRUE, TRUE, 1);

    /* Hierarchy.  */
    GLOBALS->gtk2_tree_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), 3);
    gtk_widget_show(GLOBALS->gtk2_tree_frame);

    gtk_paned_pack1 (GTK_PANED (vpan), GLOBALS->gtk2_tree_frame, TRUE, FALSE);

    GLOBALS->tree_treesearch_gtk2_c_1=gtk_ctree_new(1,0);
    GLOBALS->ctree_main=GTK_CTREE(GLOBALS->tree_treesearch_gtk2_c_1);
    gtk_clist_set_column_auto_resize (GTK_CLIST (GLOBALS->tree_treesearch_gtk2_c_1), 0, TRUE);
    gtk_widget_show(GLOBALS->tree_treesearch_gtk2_c_1);

    clist=GTK_CLIST(GLOBALS->tree_treesearch_gtk2_c_1);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "select_row", GTK_SIGNAL_FUNC(select_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "unselect_row", GTK_SIGNAL_FUNC(unselect_row_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_expand", GTK_SIGNAL_FUNC(tree_expand_callback), NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (clist), "tree_collapse", GTK_SIGNAL_FUNC(tree_collapse_callback), NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    decorated_module_cleanup();
    maketree(NULL, GLOBALS->treeroot);
    gtk_clist_thaw(clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 50);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (GLOBALS->tree_treesearch_gtk2_c_1));
    gtk_container_add (GTK_CONTAINER (GLOBALS->gtk2_tree_frame), scrolled_win);


    /* Signal names.  */
    GLOBALS->sig_store_treesearch_gtk2_c_1 = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GLOBALS->sst_sig_root_treesearch_gtk2_c_1 = NULL;
    GLOBALS->sig_root_treesearch_gtk2_c_1 = GLOBALS->treeroot;
    fill_sig_store ();

    sig_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (GLOBALS->sig_store_treesearch_gtk2_c_1));
    gtkwave_signal_connect(GTK_OBJECT(sig_view), "button_press_event",GTK_SIGNAL_FUNC(hier_top_button_press_event_std), NULL);

    /* The view now holds a reference.  We can get rid of our own reference */
    g_object_unref (G_OBJECT (GLOBALS->sig_store_treesearch_gtk2_c_1));


      {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();

	switch(GLOBALS->loaded_file_type)
		{
#ifdef EXTLOAD_SUFFIX
		case EXTLOAD_FILE:
#endif
		case FST_FILE:
					/* fallthrough for Dir is deliberate for extload and FST */
					if(GLOBALS->nonimplicit_direction_encountered)
						{
						column = gtk_tree_view_column_new_with_attributes ("Dir",
								   renderer,
								   "text", DIR_COLUMN,
								   NULL);
						gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
						}
					/* fallthrough */

		case AE2_FILE:
		case VCD_FILE:
		case VCD_RECODER_FILE:
		case DUMPLESS_FILE:
					column = gtk_tree_view_column_new_with_attributes (((GLOBALS->supplemental_datatypes_encountered) && (GLOBALS->supplemental_vartypes_encountered)) ? "VType" : "Type",
							   renderer,
							   "text", TYPE_COLUMN,
							   NULL);
					gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
					if((GLOBALS->supplemental_datatypes_encountered) && (GLOBALS->supplemental_vartypes_encountered))
						{
						column = gtk_tree_view_column_new_with_attributes ("DType",
							   renderer,
							   "text", DTYPE_COLUMN,
							   NULL);
						gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);
						}
					break;
		default:
					break;
		}

	column = gtk_tree_view_column_new_with_attributes ("Signals",
							   renderer,
							   "text", NAME_COLUMN,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (sig_view), column);

	/* Setup the selection handler */
	GLOBALS->sig_selection_treesearch_gtk2_c_1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (sig_view));
	gtk_tree_selection_set_mode (GLOBALS->sig_selection_treesearch_gtk2_c_1, GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function (GLOBALS->sig_selection_treesearch_gtk2_c_1,
                                                view_selection_func, NULL, NULL);

	gtkwave_signal_connect(GTK_OBJECT(sig_view), "button_press_event",GTK_SIGNAL_FUNC(button_press_event_std), NULL);
      }

    GLOBALS->dnd_sigview = sig_view;

    sig_frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (sig_frame), 3);
    gtk_widget_show(sig_frame);

    gtk_paned_pack2 (GTK_PANED (vpan), sig_frame, TRUE, FALSE);

    sig_scroll_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize (GTK_WIDGET (sig_scroll_win), 80, 100);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sig_scroll_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_frame), sig_scroll_win);
    gtk_container_add (GTK_CONTAINER (sig_scroll_win), sig_view);
    gtk_widget_show (sig_view);


    /* Filter.  */
    filter_hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (filter_hbox);

    filter_label = gtk_label_new ("Filter:");
    gtk_widget_show (filter_label);
    gtk_box_pack_start (GTK_BOX (filter_hbox), filter_label, FALSE, FALSE, 1);

    GLOBALS->filter_entry = gtk_entry_new ();
    if(GLOBALS->filter_str_treesearch_gtk2_c_1) { gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry), GLOBALS->filter_str_treesearch_gtk2_c_1); }
    gtk_widget_show (GLOBALS->filter_entry);

    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "activate", GTK_SIGNAL_FUNC(press_callback), NULL);
    if(!GLOBALS->do_dynamic_treefilter)
	{
    	gtkwave_signal_connect(GTK_OBJECT (GLOBALS->filter_entry), "key_press_event", (GtkSignalFunc) filter_edit_cb, NULL);
	}
	else
	{
	gtkwave_signal_connect(GTK_OBJECT(GLOBALS->filter_entry), "changed", GTK_SIGNAL_FUNC(press_callback), NULL);
	}

    gtk_tooltips_set_tip_2(tooltips, GLOBALS->filter_entry,
	   "Add a POSIX filter. "
	   "'.*' matches any number of characters,"
	   " '.' matches any character.  Hit Return to apply."
	   " The filter may be preceded with the port direction if it exists such as ++ (show only non-port), +I+, +O+, +IO+, etc."
	   " Use -- to exclude all non-ports (i.e., show only all ports), -I- to exclude all input ports, etc.",
	   NULL);

    gtk_box_pack_start (GTK_BOX (filter_hbox), GLOBALS->filter_entry, FALSE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX (vbox), filter_hbox, FALSE, FALSE, 1);

    /* Buttons.  */
    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_box_pack_start (GTK_BOX (vbox), frameh, FALSE, FALSE, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC(ok_callback), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1,
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC(insert_callback), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2,
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    button4 = gtk_button_new_with_label (" Replace ");
    gtk_container_border_width (GTK_CONTAINER (button4), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button4), "clicked", GTK_SIGNAL_FUNC(replace_callback), GTK_OBJECT (GLOBALS->gtk2_tree_frame));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4,
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    return vbox;
}


/****************************************************************
 **
 ** dnd
 **
 ****************************************************************/

/*
 *	DND "drag_begin" handler, this is called whenever a drag starts.
 */
static void DNDBeginCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)data;

        if((widget == NULL) || (dc == NULL))
		return;

	/* Put any needed drag begin setup code here. */
	if(!GLOBALS->dnd_state)
		{
		if(widget == GLOBALS->clist_search_c_3)
			{
			GLOBALS->tree_dnd_begin = SEARCH_TO_VIEW_DRAG_ACTIVE;
			}
			else
			{
			GLOBALS->tree_dnd_begin = TREE_TO_VIEW_DRAG_ACTIVE;
			}
		}
}

/*
 *      DND "drag_failed" handler, this is called when a drag and drop has
 *      failed (e.g., by pressing ESC).
 */
static gboolean DNDFailedCB(
        GtkWidget *widget, GdkDragContext *context, GtkDragResult result)
{
(void)widget;
(void)context;
(void)result;

GLOBALS->dnd_state = 0;
GLOBALS->tree_dnd_begin = VIEW_DRAG_INACTIVE;
GLOBALS->tree_dnd_requested = 0;

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);

return(FALSE);
}


/*
 *      DND "drag_end" handler, this is called when a drag and drop has
 *	completed. So this function is the last one to be called in
 *	any given DND operation.
 */
static void DNDEndCB_2(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

Trptr t;
int trwhich, trtarget;
GdkModifierType state;
gdouble x, y;
#ifdef WAVE_USE_GTK2
gint xi, yi;
#endif

/* Put any needed drag end cleanup code here. */

if(GLOBALS->dnd_tgt_on_signalarea_treesearch_gtk2_c_1)
	{
	WAVE_GDK_GET_POINTER(GLOBALS->signalarea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	if((x<0)||(y<0)||(x>=GLOBALS->signalarea->allocation.width)||(y>=GLOBALS->signalarea->allocation.height)) return;
	}
else
if(GLOBALS->dnd_tgt_on_wavearea_treesearch_gtk2_c_1)
	{
	WAVE_GDK_GET_POINTER(GLOBALS->wavearea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	if((x<0)||(y<0)||(x>=GLOBALS->wavearea->allocation.width)||(y>=GLOBALS->wavearea->allocation.height)) return;
	}
else
	{
	return;
	}


if((t=GLOBALS->traces.first))
        {
        while(t)
                {
                t->flags&=~TR_HIGHLIGHT;
                t=t->t_next;
                }
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

trtarget = ((int)y / (int)GLOBALS->fontheight) - 2;
if(trtarget < 0)
	{
	Trptr tp = GLOBALS->topmost_trace ? GivePrevTrace(GLOBALS->topmost_trace): NULL;
	trtarget = 0;

	if(tp)
		{
		t = tp;
		}
		else
		{
		if(GLOBALS->tree_dnd_begin == SEARCH_TO_VIEW_DRAG_ACTIVE)
			{
			if(GLOBALS->window_search_c_7)
				{
				search_insert_callback(GLOBALS->window_search_c_7, 1 /* is prepend */);
				}
			}
			else
			{
			action_callback(SST_ACTION_PREPEND);  /* prepend in this widget only ever used by this function call */
			}
		goto dnd_import_fini;
		}
	}
	else
	{
	t=GLOBALS->topmost_trace;
	}

trwhich=0;
while(t)
	{
        if((trwhich<trtarget)&&(GiveNextTrace(t)))
        	{
                trwhich++;
                t=GiveNextTrace(t);
                }
                else
                {
                break;
                }
	}

if(t)
	{
	t->flags |= TR_HIGHLIGHT;
	}

if(GLOBALS->tree_dnd_begin == SEARCH_TO_VIEW_DRAG_ACTIVE)
	{
	if(GLOBALS->window_search_c_7)
		{
		search_insert_callback(GLOBALS->window_search_c_7, 0 /* is insert */);
		}
	}
	else
	{
	action_callback (SST_ACTION_INSERT);
	}

if(t)
	{
	t->flags &= ~TR_HIGHLIGHT;
	}

dnd_import_fini:

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void DNDEndCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
if((widget == NULL) || (dc == NULL))
	{
	GLOBALS->tree_dnd_begin = VIEW_DRAG_INACTIVE;
	return;
	}

if(GLOBALS->tree_dnd_begin == VIEW_DRAG_INACTIVE)
	{
	return; /* to keep cut and paste in signalwindow from conflicting */
	}

if(GLOBALS->tree_dnd_requested)
	{
	GLOBALS->tree_dnd_requested = 0;

	if(GLOBALS->is_lx2 == LXT2_IS_VLIST)
		{
		set_window_busy(NULL);
		}
	DNDEndCB_2(widget, dc, data);
	if(GLOBALS->is_lx2 == LXT2_IS_VLIST)
		{
		set_window_idle(NULL);
		}
	}

GLOBALS->tree_dnd_begin = VIEW_DRAG_INACTIVE;
}



/*
 *	DND "drag_motion" handler, this is called whenever the
 *	pointer is dragging over the target widget.
 */
static gboolean DNDDragMotionCB(
        GtkWidget *widget, GdkDragContext *dc,
        gint x, gint y, guint t,
        gpointer data
)
{
(void)x; 
(void)y;
(void)data;

	gboolean same_widget;
	GdkDragAction suggested_action;
	GtkWidget *src_widget, *tar_widget;
        if((widget == NULL) || (dc == NULL))
		{
                gdk_drag_status(dc, 0, t);
                return(FALSE);
		}

	/* Get source widget and target widget. */
	src_widget = gtk_drag_get_source_widget(dc);
	tar_widget = widget;

	/* Note if source widget is the same as the target. */
	same_widget = (src_widget == tar_widget) ? TRUE : FALSE;

	GLOBALS->dnd_tgt_on_signalarea_treesearch_gtk2_c_1 = (tar_widget == GLOBALS->signalarea);
	GLOBALS->dnd_tgt_on_wavearea_treesearch_gtk2_c_1 = (tar_widget == GLOBALS->wavearea);

	/* If this is the same widget, our suggested action should be
	 * move.  For all other case we assume copy.
	 */
	if(same_widget)
		suggested_action = GDK_ACTION_MOVE;
	else
		suggested_action = GDK_ACTION_COPY;

	/* Respond with default drag action (status). First we check
	 * the dc's list of actions. If the list only contains
	 * move, copy, or link then we select just that, otherwise we
	 * return with our default suggested action.
	 * If no valid actions are listed then we respond with 0.
	 */

        /* Only move? */
        if(dc->actions == GDK_ACTION_MOVE)
            gdk_drag_status(dc, GDK_ACTION_MOVE, t);
        /* Only copy? */
        else if(dc->actions == GDK_ACTION_COPY)
            gdk_drag_status(dc, GDK_ACTION_COPY, t);
        /* Only link? */
        else if(dc->actions == GDK_ACTION_LINK)
            gdk_drag_status(dc, GDK_ACTION_LINK, t);
        /* Other action, check if listed in our actions list? */
        else if(dc->actions & suggested_action)
            gdk_drag_status(dc, suggested_action, t);
        /* All else respond with 0. */
        else
            gdk_drag_status(dc, 0, t);

	return(FALSE);
}

/*
 *	DND "drag_data_get" handler, for handling requests for DND
 *	data on the specified widget. This function is called when
 *	there is need for DND data on the source, so this function is
 *	responsable for setting up the dynamic data exchange buffer
 *	(DDE as sometimes it is called) and sending it out.
 */
static void DNDDataRequestCB(
	GtkWidget *widget, GdkDragContext *dc,
	GtkSelectionData *selection_data, guint info, guint t,
	gpointer data
)
{
(void)dc;
(void)info;
(void)t;
(void)data;

int upd = 0;
GLOBALS->tree_dnd_requested = 1;  /* indicate that a request for data occurred... */

if(widget == GLOBALS->clist_search_c_3)	/* from search */
	{
	char *text = add_dnd_from_searchbox();
	if(text)
		{
		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)text, strlen(text));
		free_2(text);
		}
	upd = 1;
	}
else if(widget == GLOBALS->signalarea)
	{
	char *text = add_dnd_from_signal_window();
	if(text)
		{
		char *text2 = emit_gtkwave_savefile_formatted_entries_in_tcl_list(GLOBALS->traces.first, TRUE);
		if(text2)
			{
			int textlen = strlen(text);
			int text2len = strlen(text2);
			char *pnt = calloc_2(1, textlen + text2len + 1);

			memcpy(pnt, text, textlen);
			memcpy(pnt + textlen, text2, text2len);

			free_2(text2);
			free_2(text);
			text = pnt;
			}

		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)text, strlen(text));
		free_2(text);
		}
	upd = 1;
	}
else if(widget == GLOBALS->dnd_sigview)
	{
	char *text = add_dnd_from_tree_window();
	if(text)
		{
		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)text, strlen(text));
		free_2(text);
		}
	upd = 1;
	}

if(upd)
	{
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

/*
 *      DND "drag_data_received" handler. When DNDDataRequestCB()
 *	calls gtk_selection_data_set() to send out the data, this function
 *	receives it and is responsible for handling it.
 *
 *	This is also the only DND callback function where the given
 *	inputs may reflect those of the drop target so we need to check
 *	if this is the same structure or not.
 */
static void DNDDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data) {
(void)x;
(void)y;
(void)t;

    gboolean same;
    GtkWidget *source_widget;

    if((widget == NULL) || (data == NULL) || (dc == NULL)) return;

    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if(selection_data == NULL)     return;
    if(selection_data->length < 0) return;

    /* Source and target widgets are the same? */
    source_widget = gtk_drag_get_source_widget(dc);
    same = (source_widget == widget) ? TRUE : FALSE;
    if(same)
	{
	/* unused */
	}

    if(source_widget)
    if((source_widget == GLOBALS->clist_search_c_3) || /* from search */
       (source_widget == GLOBALS->signalarea) ||
       (source_widget == GLOBALS->dnd_sigview))
		{
		/* use internal mechanism instead of passing names around... */
		return;
		}

    GLOBALS->dnd_state = 0;
    GLOBALS->tree_dnd_requested = 0;

    /* Now check if the data format type is one that we support
     * (remember, data format type, not data type).
     *
     * We check this by testing if info matches one of the info
     * values that we have defined.
     *
     * Note that we can also iterate through the atoms in:
     *	GList *glist = dc->targets;
     *
     *	while(glist != NULL)
     *	{
     *	    gchar *name = gdk_atom_name((GdkAtom)glist->data);
     *	     * strcmp the name to see if it matches
     *	     * one that we support
     *	     *
     *	    glist = glist->next;
     *	}
     */
    if((info == WAVE_DRAG_TAR_INFO_0) ||
       (info == WAVE_DRAG_TAR_INFO_1) ||
       (info == WAVE_DRAG_TAR_INFO_2))
    {
    /* printf("XXX %08x '%s'\n", selection_data->data, selection_data->data); */
#ifndef MAC_INTEGRATION
    DND_helper_quartz((char *)selection_data->data);
#else
    if(!GLOBALS->dnd_helper_quartz)
        {
        GLOBALS->dnd_helper_quartz = strdup_2((const char *)selection_data->data);
        }
#endif
    }
}


void DND_helper_quartz(char *data)
{
int num_found;

if(!GLOBALS->splash_is_loading)
	{
#if GTK_CHECK_VERSION(2,4,0)
	if(!GLOBALS->pFileChoose)
#endif
		{
		if(!(num_found = process_url_list(data)))
		        {
		        num_found = process_tcl_list(data, TRUE);
		        }

		if(num_found)
		        {
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
		        }
		}
#if GTK_CHECK_VERSION(2,4,0)
		else
		{
	        MaxSignalLength();
	        signalarea_configure_event(GLOBALS->signalarea, NULL);
	        wavearea_configure_event(GLOBALS->wavearea, NULL);
		}
#endif
	}
}


/*
 *	DND "drag_data_delete" handler, this function is called when
 *	the data on the source `should' be deleted (ie if the DND was
 *	a move).
 */
static void DNDDataDeleteCB(
	GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

/* nothing */
}


/***********************/


void dnd_setup(GtkWidget *src, GtkWidget *w, int enable_receive)
{
	GtkWidget *win = w;
	GtkTargetEntry target_entry[3];

	/* Realize the clist widget and make sure it has a window,
	 * this will be for DND setup.
	 */
        if(!GTK_WIDGET_NO_WINDOW(w))
	{
		/* DND: Set up the clist as a potential DND destination.
		 * First we set up target_entry which is a sequence of of
		 * structure which specify the kinds (which we define) of
		 * drops accepted on this widget.
		 */

		/* Set up the list of data format types that our DND
		 * callbacks will accept.
		 */
		target_entry[0].target = WAVE_DRAG_TAR_NAME_0;
		target_entry[0].flags = 0;
		target_entry[0].info = WAVE_DRAG_TAR_INFO_0;
                target_entry[1].target = WAVE_DRAG_TAR_NAME_1;
                target_entry[1].flags = 0;
                target_entry[1].info = WAVE_DRAG_TAR_INFO_1;
                target_entry[2].target = WAVE_DRAG_TAR_NAME_2;
                target_entry[2].flags = 0;
                target_entry[2].info = WAVE_DRAG_TAR_INFO_2;

		/* Set the drag destination for this widget, using the
		 * above target entry types, accept move's and coppies'.
		 */
		gtk_drag_dest_set(
			w,
			GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
			GTK_DEST_DEFAULT_DROP,
			target_entry,
			sizeof(target_entry) / sizeof(GtkTargetEntry),
			GDK_ACTION_MOVE | GDK_ACTION_COPY
		);
		gtkwave_signal_connect(GTK_OBJECT(w), "drag_motion", GTK_SIGNAL_FUNC(DNDDragMotionCB), win);

		/* Set the drag source for this widget, allowing the user
		 * to drag items off of this clist.
		 */
		if(src)
			{
			gtk_drag_source_set(
				src,
				GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
	                        target_entry,
	                        sizeof(target_entry) / sizeof(GtkTargetEntry),
				GDK_ACTION_MOVE | GDK_ACTION_COPY
			);
			/* Set DND signals on clist. */
			gtkwave_signal_connect(GTK_OBJECT(src), "drag_begin", GTK_SIGNAL_FUNC(DNDBeginCB), win);
	                gtkwave_signal_connect(GTK_OBJECT(src), "drag_end", GTK_SIGNAL_FUNC(DNDEndCB), win);
	                gtkwave_signal_connect(GTK_OBJECT(src), "drag_data_get", GTK_SIGNAL_FUNC(DNDDataRequestCB), win);
	                gtkwave_signal_connect(GTK_OBJECT(src), "drag_data_delete", GTK_SIGNAL_FUNC(DNDDataDeleteCB), win);
			gtkwave_signal_connect(GTK_OBJECT(src), "drag_failed", GTK_SIGNAL_FUNC(DNDFailedCB), win);
			}

                if(enable_receive) gtkwave_signal_connect(GTK_OBJECT(w),   "drag_data_received", GTK_SIGNAL_FUNC(DNDDataReceivedCB), win);
	}
}

/***************************************************************************/

static void recurse_append_callback(GtkWidget *widget, gpointer data)
{
int i;

if(!GLOBALS->sst_sig_root_treesearch_gtk2_c_1 || !data) return;

set_window_busy(widget);

for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
        {
        struct symbol *s;
	if(i<0) break; /* GHW */
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

/* LX2 */
if(GLOBALS->is_lx2)
	{
	int pre_import = 0;

	for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
	        {
	        struct symbol *s, *t;
		if(i<0) break; /* GHW */
	        s=GLOBALS->facs[i];
		t=s->vec_root;
		if((t)&&(GLOBALS->autocoalesce))
			{
			if(get_s_selected(t))
				{
				while(t)
					{
					if(t->n->mv.mvlfac)
						{
						lx2_set_fac_process_mask(t->n);
						pre_import++;
						}
					t=t->vec_chain;
					}
				}
			}
			else
			{
			if(s->n->mv.mvlfac)
				{
				lx2_set_fac_process_mask(s->n);
				pre_import++;
				}
			}
	        }

	if(pre_import)
		{
		lx2_import_masked();
		}
	}
/* LX2 */

for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
        {
	int len;
        struct symbol *s, *t;
	if(i<0) break; /* GHW */
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t, 0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);
		}
        }

set_window_idle(widget);

GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = GLOBALS->traces.last;
MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void recurse_insert_callback(GtkWidget *widget, gpointer data)
{
Traces tcache;
int i;

if(!GLOBALS->sst_sig_root_treesearch_gtk2_c_1 || !data) return;

memcpy(&tcache,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

set_window_busy(widget);

for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
        {
        struct symbol *s;
	if(i<0) break; /* GHW */
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

/* LX2 */
if(GLOBALS->is_lx2)
	{
	int pre_import = 0;

	for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
	        {
	        struct symbol *s, *t;
		if(i<0) break; /* GHW */
	        s=GLOBALS->facs[i];
		t=s->vec_root;
		if((t)&&(GLOBALS->autocoalesce))
			{
			if(get_s_selected(t))
				{
				while(t)
					{
					if(t->n->mv.mvlfac)
						{
						lx2_set_fac_process_mask(t->n);
						pre_import++;
						}
					t=t->vec_chain;
					}
				}
			}
			else
			{
			if(s->n->mv.mvlfac)
				{
				lx2_set_fac_process_mask(s->n);
				pre_import++;
				}
			}
	        }

	if(pre_import)
		{
		lx2_import_masked();
		}
	}
/* LX2 */

for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
        {
	int len;
        struct symbol *s, *t;
	if(i<0) break; /* GHW */
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t, 0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);
		}
        }

set_window_idle(widget);

GLOBALS->traces.buffercount=GLOBALS->traces.total;
GLOBALS->traces.buffer=GLOBALS->traces.first;
GLOBALS->traces.bufferlast=GLOBALS->traces.last;
GLOBALS->traces.first=tcache.first;
GLOBALS->traces.last=tcache.last;
GLOBALS->traces.total=tcache.total;

PasteBuffer();

GLOBALS->traces.buffercount=tcache.buffercount;
GLOBALS->traces.buffer=tcache.buffer;
GLOBALS->traces.bufferlast=tcache.bufferlast;

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void recurse_replace_callback(GtkWidget *widget, gpointer data)
{
Traces tcache;
int i;
Trptr tfirst=NULL, tlast=NULL;

if(!GLOBALS->sst_sig_root_treesearch_gtk2_c_1 || !data) return;

memcpy(&tcache,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

set_window_busy(widget);

for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
        {
        struct symbol *s;
	if(i<0) break; /* GHW */
        s=GLOBALS->facs[i];
	if(s->vec_root)
		{
		set_s_selected(s->vec_root, GLOBALS->autocoalesce);
		}
        }

/* LX2 */
if(GLOBALS->is_lx2)
	{
	int pre_import = 0;

	for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
	        {
	        struct symbol *s, *t;
		if(i<0) break; /* GHW */
	        s=GLOBALS->facs[i];
		t=s->vec_root;
		if((t)&&(GLOBALS->autocoalesce))
			{
			if(get_s_selected(t))
				{
				while(t)
					{
					if(t->n->mv.mvlfac)
						{
						lx2_set_fac_process_mask(t->n);
						pre_import++;
						}
					t=t->vec_chain;
					}
				}
			}
			else
			{
			if(s->n->mv.mvlfac)
				{
				lx2_set_fac_process_mask(s->n);
				pre_import++;
				}
			}
	        }

	if(pre_import)
		{
		lx2_import_masked();
		}
	}
/* LX2 */

for(i=GLOBALS->fetchlow;i<=GLOBALS->fetchhigh;i++)
        {
	int len;
        struct symbol *s, *t;
	if(i<0) break; /* GHW */
        s=GLOBALS->facs[i];
	t=s->vec_root;
	if((t)&&(GLOBALS->autocoalesce))
		{
		if(get_s_selected(t))
			{
			set_s_selected(t, 0);
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNodeUnroll(s->n, NULL);
		}
        }

set_window_idle(widget);

tfirst=GLOBALS->traces.first; tlast=GLOBALS->traces.last;       /* cache for highlighting */

GLOBALS->traces.buffercount=GLOBALS->traces.total;
GLOBALS->traces.buffer=GLOBALS->traces.first;
GLOBALS->traces.bufferlast=GLOBALS->traces.last;
GLOBALS->traces.first=tcache.first;
GLOBALS->traces.last=tcache.last;
GLOBALS->traces.total=tcache.total;

{
Trptr t = GLOBALS->traces.first;
Trptr *tp = NULL;
int numhigh = 0;
int it;

while(t) { if(t->flags & TR_HIGHLIGHT) { numhigh++; } t = t->t_next; }
if(numhigh)
        {
        tp = calloc_2(numhigh, sizeof(Trptr));
        t = GLOBALS->traces.first;
        it = 0;
        while(t) { if(t->flags & TR_HIGHLIGHT) { tp[it++] = t; } t = t->t_next; }
        }

PasteBuffer();

GLOBALS->traces.buffercount=tcache.buffercount;
GLOBALS->traces.buffer=tcache.buffer;
GLOBALS->traces.bufferlast=tcache.bufferlast;

for(i=0;i<numhigh;i++)
        {
        tp[i]->flags |= TR_HIGHLIGHT;
        }

t = tfirst;
while(t)
        {
        t->flags &= ~TR_HIGHLIGHT;
        if(t==tlast) break;
        t=t->t_next;
        }

CutBuffer();

while(tfirst)
        {
        tfirst->flags |= TR_HIGHLIGHT;
        if(tfirst==tlast) break;
        tfirst=tfirst->t_next;
        }

if(tp)
        {
        free_2(tp);
        }
}

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


void recurse_import(GtkWidget *widget, guint callback_action)
{
if(GLOBALS->sst_sig_root_treesearch_gtk2_c_1)
	{
	int fz;

	GLOBALS->fetchlow = GLOBALS->fetchhigh = -1;
	if(GLOBALS->sst_sig_root_treesearch_gtk2_c_1->child) recurse_fetch_high_low(GLOBALS->sst_sig_root_treesearch_gtk2_c_1->child);
	fz = GLOBALS->fetchhigh - GLOBALS->fetchlow + 1;
	void (*func)(GtkWidget *, gpointer);

	switch(callback_action)
		{
		case WV_RECURSE_INSERT:		func = recurse_insert_callback; break;
		case WV_RECURSE_REPLACE:	func = recurse_replace_callback; break;
	
		case WV_RECURSE_APPEND:
		default:			func = recurse_append_callback; break;
		}

	if((GLOBALS->fetchlow >= 0) && (GLOBALS->fetchhigh >= 0))
		{
		widget = GLOBALS->mainwindow; /* otherwise using widget passed from the menu item crashes on OSX */

		if(fz > WV_RECURSE_IMPORT_WARN)
			{
			char recwarn[128];
			sprintf(recwarn, "Really import %d facilit%s?", fz, (fz==1)?"y":"ies");
		
			simplereqbox("Recurse Warning",300,recwarn,"Yes", "No", GTK_SIGNAL_FUNC(func), 0);
			}
			else
			{
			func(widget, (gpointer)1);
			}
		}
	}
}

/***************************************************************************/
