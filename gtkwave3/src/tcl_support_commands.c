/*
 * Copyright (c) Yiftach Tzori 2009-2012.
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
#include "tcl_support_commands.h"

/* **
 * Search for a tree node that is associated with the hierarchical path
 * return pointer to this node or NULL
 */
GtkCTreeNode *SST_find_node_by_path(GtkCTreeRow *root, char *path) {
  char *s = strdup_2(path) ;
  char *p = s ;
  char *p1 ;
  GtkCTreeRow *gctr = root ;
  GtkCTreeNode *node = gctr->parent ;
  struct tree *t ;
  int i ;
  /* in the cases of path that where generated inside a `generate-for' block
   * path name will contain '[]'. This will prompt TCL to soround PATH with
   * '{' and '}'
   */
  if (*p == '{' && (p1 = strrchr(p, '}'))) {
    p++ ;
    *p1 = '\0' ;
  }
  while (gctr)  {
    if ((p1 = strchr(p, '.')))
      *p1 = '\0' ;
    t = (struct tree *)(gctr->row.data) ;
    i = 0 ;
    while (strcmp(t->name, p)) { /* name mis-match */
      if (!(node = gctr->sibling)) { /* no more siblings */
	gctr = NULL ;
	break ;
      } else {
	gctr = GTK_CTREE_ROW(node);
	t = (struct tree *)(gctr->row.data) ;
      }
      i++ ;
    }
    if (gctr) {			/* normal exit from the above */
      if(!p1) {/* first/last in chain */
	if(i == 0)		/* first */
	  if(gctr->children)
	    node = GTK_CTREE_ROW(gctr->children)->parent ;
	break ;
      } else {			/* keep going down the hierarchy */
	if (!(node = gctr->children))
	  break ;
	else {
	  gctr = GTK_CTREE_ROW(gctr->children) ;
	  p = p1 + 1 ;
	}
      }
    }
  }
  free_2(s) ;
  return node ;
}

/* **
 * Open the hierarchy tree, starting from 'node' up to the root
 */
int SST_open_path(GtkCTree *ctree, GtkCTreeNode *node) {
  GtkCTreeRow *row ;
  for(row = GTK_CTREE_ROW(node) ; row->parent; row = GTK_CTREE_ROW(row->parent)) {
    if(row->parent)
      gtk_ctree_expand(ctree, row->parent);
    else
      break ;
  }
  return 0 ;
}

/* **
 * Main function called by gtkwavetcl_forceOpenTreeNode
 * Inputs:
 *   char *name :: hierachical path to open
 * Output:
 *   One of:
 *     SST_NODE_FOUND - if path is in the dump file
 *     SST_NODE_NOT_EXIST - is path is not in the dump
 *     SST_TREE_NOT_EXIST - is Tree widget does not exist
 * Side effects:
 *    If  path is in the dump then its tree is opened and scrolled
 *    to be it to display. Node is selected and associated signals
 *    are displayed.
 *    No change in any other case
  */

int SST_open_node(char *name) {
  int rv ;
#ifdef WAVE_USE_GTK2
   GtkCTree *ctree = GLOBALS->ctree_main;
   if (ctree) {
     GtkCTreeRow *gctr;
     GtkCTreeNode *target_node ;
     for(gctr = GTK_CTREE_ROW(GLOBALS->any_tree_node); gctr->parent;
	 gctr = GTK_CTREE_ROW(gctr->parent)) ;
     if ((target_node = SST_find_node_by_path(gctr, name))) {
       struct tree *t ;
       rv = SST_NODE_FOUND ;
       gtk_ctree_collapse_recursive(ctree, gctr->parent) ;
       SST_open_path(ctree, target_node) ;
       gtk_ctree_node_moveto(ctree, target_node, 0, 0.5, 0.5);
       gtk_ctree_select(ctree, target_node);
       gctr = GTK_CTREE_ROW(target_node) ;
       t = (struct tree *)(gctr->row.data) ;
       GLOBALS->sig_root_treesearch_gtk2_c_1 = t->child;
       fill_sig_store ();
     } else {
       rv = SST_NODE_NOT_EXIST ;
     }
   } else {
     rv = SST_TREE_NOT_EXIST ;
   }
#else
(void)name;
  rv = SST_TREE_NOT_EXIST ;
#endif

   return rv ;
}
/* ===== Double link lists */
llist_p *llist_new(llist_u v, ll_elem_type type, int arg) {
  llist_p *p = (llist_p *)malloc_2(sizeof(llist_p)) ;
  p->next = p->prev = NULL ;
  switch(type) {
  case LL_INT: p->u.i = v.i ; break ;
  case LL_UINT: p->u.u = v.u ; break ;
  case LL_TIMETYPE: p->u.tt = v.tt ; break ;
  case LL_CHAR: p->u.c = v.c ; break ;
  case LL_SHORT: p->u.s = v.s ; break ;
  case LL_STR:
    if(arg == -1)
      p->u.str = strdup_2(v.str) ;
    else {
      p->u.str = (char *)malloc_2(arg) ;
      strncpy(p->u.str, v.str, arg) ;
      p->u.str[arg] = '\0' ;
    }
    break ;
  case LL_VOID_P: p->u.p = v.p ; break ;
  default:
	fprintf(stderr, "Internal error in llist_new(), type: %d\n", type);
	exit(255);
  }
  return p ;
}

/*
* append llist_p element ELEM to the of the list whose first member is HEAD amd
* last is TAIL. and return the head of the list.
* if HEAD is NULL ELEM is returned.
* if TAIL is defined then ELEM is chained to it and TAIL is set to point to
* ELEM
*/

llist_p *llist_append(llist_p *head, llist_p *elem, llist_p **tail) {
  llist_p *p ;
  if (*tail) {
    p = tail[0] ;
    p->next = elem ;
    elem->prev = p ;
    tail[0] = elem ;
  } else {
    if (head) {
      for(p = head ; p->next; p = p->next) ;
      p->next = elem ;
      elem ->prev = p ;
    } else {
      head = elem ;
    }
  }
  return head ;
}
/*
* Remove the last element from list whose first member is HEAD
* if TYPE is LL_STR the memory allocated for this string is freed.
* if the TYPE is LL_VOID_P that the caller supplied function pointer F() is
*  is executed (if not NULL)
* HEAD and TAIL are updated.
 */

llist_p *llist_remove_last(llist_p *head, llist_p **tail, ll_elem_type type, void *f(void *) ) {
  if (head) {
    llist_p *p = tail[0] ;
    switch(type) {
    case LL_STR: free_2(p->u.str) ; break ;
    case LL_VOID_P:
      if (f)
	f(p->u.p) ;
      break ;
    default:
      fprintf(stderr, "Internal error in llist_remove_last(), type: %d\n", type);
      exit(255);
    }
    if (p->prev) {
      tail[0] = p->prev ;
    } else {
      head = tail[0] = NULL ;
    }
    free_2(p) ;
  }
  return head ;
}

/* Destroy the list whose first member is HEAD
* function pointer F() is called in type is LL_VOID_P
* if TYPE is LL_STR then string is freed
*/
void llist_free(llist_p *head, ll_elem_type type, void *f(void *)) {
  llist_p *p = head, *p1 ;
  while(p) {
    p1 = p->next ;
    switch(type) {
    case LL_STR: free_2(p->u.str) ; break ;
    case LL_VOID_P:
      if (f)
	f(p->u.p) ;
      break ;
    default:
      fprintf(stderr, "Internal error in llist_free(), type: %d\n", type);
      exit(255);
    }
    free_2(p) ;
    p = p1 ;
  }
}
/* ===================================================== */
/* Create a Trptr structure that contains the bit-vector VEC
* This is based on the function AddVector()
 */
Trptr BitVector_to_Trptr(bvptr vec) {
  Trptr  t;
  int    n;

  GLOBALS->signalwindow_width_dirty=1;

  n = vec->nbits;
  t = (Trptr) calloc_2(1, sizeof( TraceEnt ) );
  if( t == NULL ) {
    fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	     vec->bvname );
    return( 0 );
  }

  t->name = vec->bvname;

  if(GLOBALS->hier_max_level)
    t->name = hier_extract(t->name, GLOBALS->hier_max_level);

  t->flags = ( n > 3 ) ? TR_HEX|TR_RJUSTIFY : TR_BIN|TR_RJUSTIFY;
  t->vector = TRUE;
  t->n.vec = vec;
  /* AddTrace( t ); */
  return( t );
}

Trptr find_first_highlighted_trace(void) {
  Trptr t=GLOBALS->traces.first;
  while(t) {
    if(t->flags&TR_HIGHLIGHT) {
      if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))) {
	break;
      }
    }
    t=t->t_next;
  }
  return(t);
}

/* Find is signal named NAME is on display and return is Trptr value
* or NULL
* NAME is a full hierarchical name, but may not in include range '[..:..]'
*  information.
 */
Trptr is_signal_displayed(char *name) {
  Trptr t=GLOBALS->traces.first ;
  char *p = strchr(name, '['), *p1 ;
  unsigned int len, len1 ;
  if(p)
    *p = '\0' ;
  len = strlen(name) ;
  while(t) {
    int was_packed = HIER_DEPACK_ALLOC;
    int cc;
    if(t->vector)
	{
	p = t->n.vec->bvname;
	}
	else
	{
	if(t->n.vec)
		{
		p = hier_decompress_flagged(t->n.nd->nname, &was_packed);
		}
		else
		{
		p = NULL;
		}
	}

    if(p) {
      p1 = strchr(p,'[') ;
      len1 = (p1) ? (unsigned int)(p1 - p) : strlen(p) ;
      cc = ((len == len1) && !strncmp(name, p, len));
      if(was_packed) free_2(p);
      if(cc)
	break ;
    }
    t = t->t_next ;
  }
  return t ;
}

/* Create a Trptr structure for ND and return its value
* This is based on the function AddNodeTraceReturn()
*/
Trptr Node_to_Trptr(nptr nd)
{
  Trptr  t = NULL;
  hptr histpnt;
  hptr *harray;
  int histcount;
  int i;

  if(nd->mv.mvlfac) import_trace(nd);

  GLOBALS->signalwindow_width_dirty=1;

  if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL ) {
    fprintf( stderr, "Out of memory, can't add to analyzer\n" );
    return( 0 );
  }

  if(!nd->harray) { /* make quick array lookup for aet display */
    histpnt=&(nd->head);
    histcount=0;

    while(histpnt) {
      histcount++;
      histpnt=histpnt->next;
    }

    nd->numhist=histcount;

    if(!(nd->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr)))) {
      fprintf( stderr, "Out of memory, can't add to analyzer\n" );
      free_2(t);
      return(0);
    }

    histpnt=&(nd->head);
    for(i=0;i<histcount;i++) {
      *harray=histpnt;
      harray++;
      histpnt=histpnt->next;
    }
  }

  if(!GLOBALS->hier_max_level) {
    int flagged = HIER_DEPACK_ALLOC;

    t->name = hier_decompress_flagged(nd->nname, &flagged);
    t->is_depacked = (flagged != 0);
  }
  else {
    int flagged = HIER_DEPACK_ALLOC;
    char *tbuff = hier_decompress_flagged(nd->nname, &flagged);
    if(!flagged) {
      t->name = hier_extract(nd->nname, GLOBALS->hier_max_level);
    }
    else {
      t->name = strdup_2(hier_extract(tbuff, GLOBALS->hier_max_level));
      free_2(tbuff);
      t->is_depacked = 1;
    }
  }

  if(nd->extvals) { /* expansion vectors */
    int n;

    n = nd->msi - nd->lsi;
    if(n<0)n=-n;
    n++;

    t->flags = (( n > 3 )||( n < -3 )) ? TR_HEX|TR_RJUSTIFY :
      TR_BIN|TR_RJUSTIFY;
  }
  else {
    t->flags |= TR_BIN;	/* binary */
  }
  t->vector = FALSE;
  t->n.nd = nd;
  /* if(tret) *tret = t;		... for expand */
  return t ;
}
/*
* Search for the signal named (full path) NAME in the signal data base and
* create a Trptr structure for it
* NAME is a full hierarchy name, but may not include range information.
* Return the structure created or NULL
*/
Trptr sig_name_to_Trptr(char *name) {
  Trptr t = NULL ;
  int was_packed = HIER_DEPACK_ALLOC;
  int i, name_len;
  char *hfacname = NULL;
  struct symbol *s = NULL, *s2 ;
  int len = 0 ;
  bvptr v = NULL;
  bptr b = NULL;
  int pre_import = 0;

  if(name)
	{
	name_len = strlen(name);
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		hfacname = hier_decompress_flagged(GLOBALS->facs[i]->name,  &was_packed);
		if(!strcmp(name, hfacname) || ((!strncmp(name, hfacname, name_len) && hfacname[name_len] == '[')))
			{
			s = GLOBALS->facs[i];
			if((s2 = s->vec_root))
				{
				s = s2;
				}
				else
				{
				s2 = s;
				}

			if(GLOBALS->is_lx2)
				{
				while(s2)
					{
	                                if(s2->n->mv.mvlfac)	/* the node doesn't exist yet! */
	                                        {
	                                        lx2_set_fac_process_mask(s2->n);
	                                        pre_import++;
	                                        }

					s2 = s2->vec_chain;
					len++;
					}
				}
				else
				{
				while(s2)
					{
					s2 = s2->vec_chain;
					len++;
					}
				}

			if(was_packed) { free_2(hfacname); }
			break;
			}
		if(was_packed) { free_2(hfacname); }
		s = NULL;
		}

	if(s)
		{
	        if(pre_import)
        	        {
        	        lx2_import_masked();		/* import any missing nodes */
        	        }

		if(len > 1)
			{
			if ((b = makevec_chain(NULL, s, len)))
				{
				if((v=bits2vector(b)))
					{
					t = BitVector_to_Trptr(v) ;
					}
			                else
			                {
			                free_2(b->name);
			                if(b->attribs) free_2(b->attribs);
			                free_2(b);
			                }
				}
			}
			else
			{
			nptr node = s->n ;
			t = Node_to_Trptr(node) ;
			}
		}

	}

  return t ;
}

/* Return the base prefix for the signal value */
char *signal_value_prefix(TraceFlagsType flags) {
  if(flags & TR_BIN) return "0b" ;
  if(flags & TR_HEX) return "0x" ;
  if(flags & TR_OCT) return "0" ;
  return "" ;
}

/* ===================================================== */

llist_p *signal_change_list(char *sig_name, int dir, TimeType start_time,
		       TimeType end_time, int max_elements) {
  llist_p *l0_head = NULL, *l0_tail = NULL, *l1_head = NULL,*l_elem, *lp ;
  llist_p *l1_tail = NULL ;
  char *s, s1[1024] ;
  hptr h_ptr ;
  Trptr t = NULL ;
  Trptr t_created = NULL;
  if(!sig_name) {
    t = (Trptr)find_first_highlighted_trace();
  } else {
    /* case of sig name, find the representing Trptr structure */
    if (!(t = is_signal_displayed(sig_name)))
      t = t_created = sig_name_to_Trptr(sig_name) ;
  }
  if (t) {			/* we have a signal */
    /* create a list of value change structs (hptrs or vptrs */
    int nelem = 0 /* , bw = -1 */ ; /* scan-build */
    TimeType tstart = (dir == STRACE_FORWARD) ? start_time : end_time ;
    TimeType tend = (dir == STRACE_FORWARD) ? end_time : start_time ;
    if ((dir == STRACE_BACKWARD) && (max_elements == 1))
	{
	max_elements++;
	}
    if (!t->vector) {
      hptr h, h1;
      int len = 0  ;
      /* scan-build :
      if(t->n.nd->extvals) {
	bw = abs(t->n.nd->msi - t->n.nd->lsi) + 1 ;
      }
      */
      h = bsearch_node(t->n.nd, tstart - t->shift) ;
      for(h1 = h; h1; h1 = h1->next) {
	if (h1->time <= tend) {
	  if (len++ < max_elements) {
	    llist_u llp; llp.p = h1;
	    l_elem = llist_new(llp, LL_VOID_P, -1) ;
	    l0_head = llist_append(l0_head, l_elem, &l0_tail) ;
	    if(!l0_tail) l0_tail = l0_head ;
	  } else {
	    if(dir == STRACE_FORWARD)
	      break ;
	    else {
	      if(!l0_head) /* null pointer deref found by scan-build */
			{
		        llist_u llp; llp.p = h1;
		        l_elem = llist_new(llp, LL_VOID_P, -1) ;
		        l0_head = llist_append(l0_head, l_elem, &l0_tail) ;
		        if(!l0_tail) l0_tail = l0_head ;
			}
	      l_elem = l0_head ;
	      l0_head = l0_head->next ; /* what scan-build flagged as null */
	      l0_head->prev = NULL ;
	      l_elem->u.p = (void *)h1 ;
	      l_elem->next = NULL ;
	      l_elem->prev = l0_tail ;
	      l0_tail->next = l_elem ;
	      l0_tail = l_elem ;
	    }
	  }
	}
      }
    } else {
      vptr v, v1;
      v = bsearch_vector(t->n.vec, tstart - t->shift) ;
      for(v1 = v; v1; v1 = v1->next) {
	if (v1->time <= tend) {
	  llist_u llp; llp.p = v1;
	  l_elem = llist_new(llp, LL_VOID_P, -1) ;
	  l0_head = llist_append(l0_head, l_elem, &l0_tail) ;
	  if(!l0_tail) l0_tail = l0_head ;
	}
      }
    }
    lp = (start_time < end_time) ? l0_head : l0_tail ;
    /* now create a linked list of time,value.. */
    while (lp && (nelem++ < max_elements)) {
      llist_u llp; llp.tt = ((t->vector) ? ((vptr)lp->u.p)->time: ((hptr)lp->u.p)->time);
      l_elem = llist_new(llp, LL_TIMETYPE, -1) ;
      l1_head = llist_append(l1_head, l_elem, &l1_tail) ;
      if(!l1_tail) l1_tail = l1_head ;
      if(t->vector == 0) {
	if(!t->n.nd->extvals) {	/* really single bit */
	  switch(((hptr)lp->u.p)->v.h_val) {
	  case '0':
	  case AN_0: llp.str = "0"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case '1':
	  case AN_1: llp.str = "1"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case 'x':
	  case 'X':
	  case AN_X: llp.str = "x"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case 'z':
	  case 'Z':
	  case AN_Z: llp.str = "z"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case 'h':
	  case 'H':
	  case AN_H: llp.str = "h"; l_elem = llist_new(llp, LL_STR, -1) ; break ; /* added for GHW... */

	  case 'u':
	  case 'U':
	  case AN_U: llp.str = "u"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case 'w':
	  case 'W':
	  case AN_W: llp.str = "w"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case 'l':
	  case 'L':
	  case AN_L: llp.str = "l"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  case '-':
	  case AN_DASH: llp.str = "-"; l_elem = llist_new(llp, LL_STR, -1) ; break ;

	  default:      llp.str = "?"; l_elem = llist_new(llp, LL_STR, -1) ; break ; /* ...added for GHW */
	  }
	} else {		/* this is still an array */
	  h_ptr = (hptr)lp->u.p ;
	  if(h_ptr->flags&HIST_REAL) {
	    if(!(h_ptr->flags&HIST_STRING)) {
#ifdef WAVE_HAS_H_DOUBLE
	      s=convert_ascii_real(t, &h_ptr->v.h_double);
#else
	      s=convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
	    } else {
	      s=convert_ascii_string((char *)h_ptr->v.h_vector);
	    }
	  } else {
	    s=convert_ascii_vec(t,h_ptr->v.h_vector);
	  }
	  if(s) {
	    sprintf(s1,"%s%s", signal_value_prefix(t->flags), s) ;
	    llp.str = s1;
	    l_elem = llist_new(llp, LL_STR, -1) ;
	  } else {
	    l1_head = llist_remove_last(l1_head, &l1_tail, LL_INT, NULL) ;
	  }
	}
      } else {
        sprintf(s1, "%s%s", signal_value_prefix(t->flags),
		convert_ascii(t, (vptr)lp->u.p)) ;
        llp.str = s1 ;
	l_elem = llist_new(llp, LL_STR, -1) ;
      }
      l1_head = llist_append(l1_head, l_elem, &l1_tail) ;
      lp = (start_time < end_time) ? lp->next : lp->prev ;
    }
    llist_free(l0_head, LL_VOID_P, NULL) ;
  }

  if(t_created)
	{
	FreeTrace(t_created);
	}

  return l1_head ;
}
