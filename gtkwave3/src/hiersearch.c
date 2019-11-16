/*
 * Copyright (c) Tony Bybell 1999-2011.
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
#include "gtk12compat.h"
#include "analyzer.h"
#include "symbol.h"
#include "lx2.h"
#include "vcd.h"
#include "busy.h"
#include "debug.h"


int hier_searchbox_is_active(void)
{
return(GLOBALS->is_active_hiersearch_c_1);
}


void refresh_hier_tree(struct tree *t)
{
struct tree *t2;
GtkCList *cl;
int len;
int row;
int pixlen=0, maxpixlen=0;
static char *dotdot="..";
struct treechain *tc;

gtk_clist_freeze(cl=GTK_CLIST(GLOBALS->clist_hiersearch_c_1));
gtk_clist_clear(cl);

GLOBALS->num_rows_hiersearch_c_1=0;

if(t!=GLOBALS->treeroot)
	{
	maxpixlen=font_engine_string_measure(GLOBALS->signalfont,(gchar *)(dotdot));
	}

if(!GLOBALS->hier_grouping)
	{
        char *tmp, *tmp2, *tmp3;
	t2=t;
	while(t2)
		{
	                {
	                if(t2->child)
	                        {
	                        tmp=wave_alloca(strlen(t2->name)+5);
	                        strcpy(tmp,   "(+) ");
	                        strcpy(tmp+4, t2->name);
	                        }
                        	else
				if(t2->t_which >= 0)
				{
				if(GLOBALS->facs[t2->t_which]->vec_root)
					{
					if(GLOBALS->autocoalesce)
						{
						if(GLOBALS->facs[t2->t_which]->vec_root!=GLOBALS->facs[t2->t_which])
							{
							t2=t2->next;
							continue;
							}

						tmp2=makename_chain(GLOBALS->facs[t2->t_which]);
						tmp3=leastsig_hiername(tmp2);
						tmp=wave_alloca(strlen(tmp3)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, tmp3);
						free_2(tmp2);
						}
						else
						{
						tmp=wave_alloca(strlen(t2->name)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, t2->name);
						}
					}
					else
					{
					tmp=t2->name;
					}
				}
				else
	                        {
	                        /* tmp=t2->name; */
				goto skip_node; /* GHW */
	                        }
			row=gtk_clist_prepend(cl,(gchar **)&tmp);
			pixlen=font_engine_string_measure(GLOBALS->signalfont,(gchar *)(tmp));
			}
		maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;
		gtk_clist_set_row_data(cl, row,t2);
		GLOBALS->num_rows_hiersearch_c_1++;
skip_node:	t2=t2->next;
		}
	}
	else
	{
        char *tmp, *tmp2, *tmp3;

	t2=t;
	while(t2)
		{
                if(!t2->child)
                        {
			if(t2->t_which >= 0)
				{
				if(GLOBALS->facs[t2->t_which]->vec_root)
					{
					if(GLOBALS->autocoalesce)
						{
						if(GLOBALS->facs[t2->t_which]->vec_root!=GLOBALS->facs[t2->t_which])
							{
							t2=t2->next;
							continue;
							}

						tmp2=makename_chain(GLOBALS->facs[t2->t_which]);
						tmp3=leastsig_hiername(tmp2);
						tmp=wave_alloca(strlen(tmp3)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, tmp3);
						free_2(tmp2);
						}
						else
						{
						tmp=wave_alloca(strlen(t2->name)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, t2->name);
						}
					}
					else
					{
					tmp=t2->name;
					}
				}
				else
	                        {
	                        /* tmp=t2->name; */
				goto skip_node_2; /* GHW */
	                        }

			row=gtk_clist_prepend(cl,(gchar **)&tmp);
			pixlen=font_engine_string_measure(GLOBALS->signalfont,(gchar *)(tmp));
			maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;
			gtk_clist_set_row_data(cl, row,t2);
			GLOBALS->num_rows_hiersearch_c_1++;
                        }
skip_node_2:	t2=t2->next;
		}

	t2=t;
	while(t2)
		{
                if(t2->child)
                        {
                        tmp=wave_alloca(strlen(t2->name)+5);
                        strcpy(tmp,   "(+) ");
                        strcpy(tmp+4, t2->name);
			row=gtk_clist_prepend(cl,(gchar **)&tmp);
			pixlen=font_engine_string_measure(GLOBALS->signalfont,(gchar *)(tmp));
			maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;
			gtk_clist_set_row_data(cl, row,t2);
			GLOBALS->num_rows_hiersearch_c_1++;
                        }
		t2=t2->next;
		}
	}


if(t!=GLOBALS->treeroot)
	{
	row=gtk_clist_prepend(cl,(gchar **)&dotdot);
	gtk_clist_set_row_data(cl, row,NULL);
	GLOBALS->num_rows_hiersearch_c_1++;
	}

if(maxpixlen)gtk_clist_set_column_width(GTK_CLIST(GLOBALS->clist_hiersearch_c_1),0,maxpixlen);
gtk_clist_thaw(cl);

if((tc=GLOBALS->treechain_hiersearch_c_1))
	{
	char *buf;
	char hier_str[2];

	len=1;
	while(tc)
		{
		len+=strlen(tc->label->name);
		if(tc->next) len++;
		tc=tc->next;
		}

	buf=calloc_2(1,len);
	hier_str[0]=GLOBALS->hier_delimeter;
	hier_str[1]=0;

	tc=GLOBALS->treechain_hiersearch_c_1;
	while(tc)
		{
		strcat(buf,tc->label->name);
		if(tc->next) strcat(buf,hier_str);
		tc=tc->next;
		}
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->entry_main_hiersearch_c_1), buf);
	free_2(buf);
	}
	else
	{
	gtk_entry_set_text(GTK_ENTRY(GLOBALS->entry_main_hiersearch_c_1),"");
	}
}


static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  G_CONST_RETURN gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(GLOBALS->entry_hiersearch_c_2));
  entry_text = entry_text ? entry_text : "";
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) GLOBALS->entrybox_text_local_hiersearch_c_1=NULL;
	else strcpy((GLOBALS->entrybox_text_local_hiersearch_c_1=(char *)malloc_2(len+1)),entry_text);

  wave_gtk_grab_remove(GLOBALS->window1_hiersearch_c_1);
  gtk_widget_destroy(GLOBALS->window1_hiersearch_c_1);
  GLOBALS->window1_hiersearch_c_1 = NULL;

  GLOBALS->cleanup_e_hiersearch_c_1();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  DEBUG(printf("Entry Cancel\n"));
  GLOBALS->entrybox_text_local_hiersearch_c_1=NULL;
  wave_gtk_grab_remove(GLOBALS->window1_hiersearch_c_1);
  gtk_widget_destroy(GLOBALS->window1_hiersearch_c_1);
  GLOBALS->window1_hiersearch_c_1 = NULL;
}

static void entrybox_local(char *title, int width, char *default_text, int maxch, GtkSignalFunc func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    GLOBALS->cleanup_e_hiersearch_c_1=func;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    /* create a new modal window */
    GLOBALS->window1_hiersearch_c_1 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window1_hiersearch_c_1, ((char *)&GLOBALS->window1_hiersearch_c_1) - ((char *)GLOBALS));

    gtk_widget_set_usize( GTK_WIDGET (GLOBALS->window1_hiersearch_c_1), width, 60);
    gtk_window_set_title(GTK_WINDOW (GLOBALS->window1_hiersearch_c_1), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window1_hiersearch_c_1), "delete_event",(GtkSignalFunc) destroy_callback_e, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window1_hiersearch_c_1), vbox);
    gtk_widget_show (vbox);

    GLOBALS->entry_hiersearch_c_2 = gtk_entry_new_with_max_length (maxch);
    gtkwave_signal_connect(GTK_OBJECT(GLOBALS->entry_hiersearch_c_2), "activate",GTK_SIGNAL_FUNC(enter_callback_e),GLOBALS->entry_hiersearch_c_2);
    gtk_entry_set_text (GTK_ENTRY (GLOBALS->entry_hiersearch_c_2), default_text);
    gtk_entry_select_region (GTK_ENTRY (GLOBALS->entry_hiersearch_c_2),0, GTK_ENTRY(GLOBALS->entry_hiersearch_c_2)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), GLOBALS->entry_hiersearch_c_2, TRUE, TRUE, 0);
    gtk_widget_show (GLOBALS->entry_hiersearch_c_2);

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

    gtk_widget_show(GLOBALS->window1_hiersearch_c_1);
    wave_gtk_grab_add(GLOBALS->window1_hiersearch_c_1);
}

/***************************************************************************/

void recurse_fetch_high_low(struct tree *t)
{
top:
if(t->t_which >= 0)
        {
        if(t->t_which > GLOBALS->fetchhigh) GLOBALS->fetchhigh = t->t_which;
        if(GLOBALS->fetchlow < 0)
                {
                GLOBALS->fetchlow = t->t_which;
                }
        else
        if(t->t_which < GLOBALS->fetchlow)
                {
                GLOBALS->fetchlow = t->t_which;
                }
        }

if(t->child)
        {
        recurse_fetch_high_low(t->child);
        }

if(t->next) { t = t->next; goto top; }
}

/* Get the highest signal from T.  */
struct tree *fetchhigh(struct tree *t)
{
while(t->child) t=t->child;
return(t);
}

/* Get the lowest signal from T.  */
struct tree *fetchlow(struct tree *t)
{
if(t->child)
	{
        t=t->child;

        for(;;)
                {
                while(t->next) t=t->next;
                if(t->child) t=t->child; else break;
                }
        }
return(t);
}

static void fetchvex2(struct tree *t, char direction, char level)
{
while(t)
        {
        if(t->child)
                {
                if(t->child->child)
                        {
                        fetchvex2(t->child, direction, 1);
                        }
                        else
                        {
                        add_vector_range(NULL, fetchlow(t)->t_which,
                                fetchhigh(t)->t_which, direction);
                        }
                }
        if(level) { t=t->next; } else { break; }
        }
}

void fetchvex(struct tree *t, char direction)
{
if(t)
     	{
        if(t->child)
                {
                fetchvex2(t, direction, 0);
                }
                else
                {
                add_vector_range(NULL, fetchlow(t)->t_which,
                        fetchhigh(t)->t_which, direction);
                }
        }
}


static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

int i;

if((!GLOBALS->h_selectedtree_hiersearch_c_1)||(!GLOBALS->h_selectedtree_hiersearch_c_1->child)) return;

set_window_busy(widget);

GLOBALS->fetchlow = GLOBALS->fetchhigh = -1;
recurse_fetch_high_low(GLOBALS->h_selectedtree_hiersearch_c_1->child);

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


static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

Traces tcache;
int i;

if((!GLOBALS->h_selectedtree_hiersearch_c_1)||(!GLOBALS->h_selectedtree_hiersearch_c_1->child)) return;

memcpy(&tcache,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

set_window_busy(widget);

GLOBALS->fetchlow = GLOBALS->fetchhigh = -1;
recurse_fetch_high_low(GLOBALS->h_selectedtree_hiersearch_c_1->child);

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


static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

Traces tcache;
int i;
Trptr tfirst=NULL, tlast=NULL;

if((!GLOBALS->h_selectedtree_hiersearch_c_1)||(!GLOBALS->h_selectedtree_hiersearch_c_1->child)) return;

memcpy(&tcache,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

set_window_busy(widget);

GLOBALS->fetchlow = GLOBALS->fetchhigh = -1;
recurse_fetch_high_low(GLOBALS->h_selectedtree_hiersearch_c_1->child);

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


static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

if(GLOBALS->entrybox_text_local_hiersearch_c_1)
        {
        char *efix;

	if(!strlen(GLOBALS->entrybox_text_local_hiersearch_c_1))
		{
        	DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
        	fetchvex(GLOBALS->h_selectedtree_hiersearch_c_1, GLOBALS->bundle_direction_hiersearch_c_1);
		}
		else
		{
	        efix=GLOBALS->entrybox_text_local_hiersearch_c_1;
	        while(*efix)
	                {
	                if(*efix==' ')
	                        {
	                        *efix='_';
	                        }
	                efix++;
	                }

	        DEBUG(printf("Bundle name is: %s\n",GLOBALS->entrybox_text_local_hiersearch_c_1));
	        add_vector_range(GLOBALS->entrybox_text_local_hiersearch_c_1,
	                        fetchlow(GLOBALS->h_selectedtree_hiersearch_c_1)->t_which,
	                        fetchhigh(GLOBALS->h_selectedtree_hiersearch_c_1)->t_which,
	                        GLOBALS->bundle_direction_hiersearch_c_1);
		}
        free_2(GLOBALS->entrybox_text_local_hiersearch_c_1);
        }
        else
        {
        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
        fetchvex(GLOBALS->h_selectedtree_hiersearch_c_1, GLOBALS->bundle_direction_hiersearch_c_1);
        }

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);
}


static void
bundle_callback_generic(void)
{
if(!GLOBALS->autoname_bundles)
	{
	entrybox_local("Enter Bundle Name",300,"",128,GTK_SIGNAL_FUNC(bundle_cleanup));
	}
	else
	{
	GLOBALS->entrybox_text_local_hiersearch_c_1=NULL;
	bundle_cleanup(NULL, NULL);
	}
}


static void
bundle_callback_up(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

GLOBALS->bundle_direction_hiersearch_c_1=0;
bundle_callback_generic();
}

static void
bundle_callback_down(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

GLOBALS->bundle_direction_hiersearch_c_1=1;
bundle_callback_generic();
}

/****************************************************************************/

static void select_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
(void)widget;
(void)column;
(void)event;
(void)data;

struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->clist_hiersearch_c_1), row);
if(t)
	{
        GLOBALS->h_selectedtree_hiersearch_c_1=t;
	DEBUG(printf("Selected: %s\n",t->name));
	if(t->child)
		{
		struct treechain *tc, *tc2;

		tc=GLOBALS->treechain_hiersearch_c_1;
		if(tc)
			{
			while(tc->next) tc=tc->next;

    			tc2=calloc_2(1,sizeof(struct treechain));
			tc2->label=t;
			tc2->tree=GLOBALS->current_tree_hiersearch_c_1;
			tc->next=tc2;
			}
			else
			{
    			GLOBALS->treechain_hiersearch_c_1=calloc_2(1,sizeof(struct treechain));
    			GLOBALS->treechain_hiersearch_c_1->tree=GLOBALS->current_tree_hiersearch_c_1;
			GLOBALS->treechain_hiersearch_c_1->label=t;
			}

		GLOBALS->current_tree_hiersearch_c_1=t->child;
		refresh_hier_tree(GLOBALS->current_tree_hiersearch_c_1);
		}
	}
	else
	{
	struct treechain *tc;

	GLOBALS->h_selectedtree_hiersearch_c_1=NULL;
	tc=GLOBALS->treechain_hiersearch_c_1;
	if(tc)
		{
		for(;;)
			{
			if(tc->next)
				{
				if(tc->next->next)
					{
					tc=tc->next;
					continue;
					}
					else
					{
					GLOBALS->current_tree_hiersearch_c_1=tc->next->tree;
					free_2(tc->next);
					tc->next=NULL;
					break;
					}
				}
				else
				{
				free_2(tc);
				GLOBALS->treechain_hiersearch_c_1=NULL;
				GLOBALS->current_tree_hiersearch_c_1=GLOBALS->treeroot;
				break;
				}

			}
		refresh_hier_tree(GLOBALS->current_tree_hiersearch_c_1);
		}
	}

}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
(void)widget;
(void)column;
(void)event;
(void)data;

struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->clist_hiersearch_c_1), row);
GLOBALS->h_selectedtree_hiersearch_c_1=NULL;

if(t)
	{
	DEBUG(printf("Unselected: %s\n",t->name));
	}
	else
	{
	/* just ignore */
	}
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  GLOBALS->is_active_hiersearch_c_1=0;
  gtk_widget_destroy(GLOBALS->window_hiersearch_c_3);
  GLOBALS->window_hiersearch_c_3 = NULL;
}


/*
 * mainline..
 */
void hier_searchbox(char *title, GtkSignalFunc func)
{
    GtkWidget *scrolled_win;
    GtkWidget *vbox1, *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5;
    GtkWidget *label;
    gchar *titles[]={"Children"};
    GtkWidget *frame1, *frame2, *frameh;
    GtkWidget *table;
    GtkTooltips *tooltips;

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
    if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

    if(GLOBALS->is_active_hiersearch_c_1)
	{
	gdk_window_raise(GLOBALS->window_hiersearch_c_3->window);
	return;
	}

    GLOBALS->is_active_hiersearch_c_1=1;
    GLOBALS->cleanup_hiersearch_c_3=func;
    GLOBALS->num_rows_hiersearch_c_1=GLOBALS->selected_rows_hiersearch_c_1=0;

    /* create a new modal window */
    GLOBALS->window_hiersearch_c_3 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_hiersearch_c_3, ((char *)&GLOBALS->window_hiersearch_c_3) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_hiersearch_c_3), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_hiersearch_c_3), "delete_event",(GtkSignalFunc) destroy_callback, NULL);

    tooltips=gtk_tooltips_new_2();

    table = gtk_table_new (256, 1, FALSE);
    gtk_widget_show (table);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (vbox1), 3);
    gtk_widget_show (vbox1);
    frame1 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame1), 3);
    gtk_widget_show(frame1);
    gtk_table_attach (GTK_TABLE (table), frame1, 0, 1, 0, 1,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    label=gtk_label_new("Signal Hierarchy");
    gtk_widget_show(label);

    gtk_box_pack_start (GTK_BOX (vbox1), label, TRUE, TRUE, 0);

    GLOBALS->entry_main_hiersearch_c_1 = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(GLOBALS->entry_main_hiersearch_c_1), FALSE);
    gtk_widget_show (GLOBALS->entry_main_hiersearch_c_1);
    gtk_tooltips_set_tip_2(tooltips, GLOBALS->entry_main_hiersearch_c_1,
		"The hierarchy is built here by clicking on the appropriate "
		"items below in the scrollable window.  Click on \"..\" to "
		"go up a level."
		,NULL);

    gtk_box_pack_start (GTK_BOX (vbox1), GLOBALS->entry_main_hiersearch_c_1, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (frame1), vbox1);

    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 1, 254,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    GLOBALS->clist_hiersearch_c_1=gtk_clist_new_with_titles(1,titles);
    gtk_clist_column_titles_passive(GTK_CLIST(GLOBALS->clist_hiersearch_c_1));

    gtk_clist_set_selection_mode(GTK_CLIST(GLOBALS->clist_hiersearch_c_1), GTK_SELECTION_SINGLE);
    gtkwave_signal_connect_object (GTK_OBJECT (GLOBALS->clist_hiersearch_c_1), "select_row",GTK_SIGNAL_FUNC(select_row_callback),NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (GLOBALS->clist_hiersearch_c_1), "unselect_row",GTK_SIGNAL_FUNC(unselect_row_callback),NULL);
    gtk_widget_show (GLOBALS->clist_hiersearch_c_1);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_widget_show(scrolled_win);

    /* gtk_scrolled_window_add_with_viewport doesn't seen to work right here.. */
    gtk_container_add (GTK_CONTAINER (scrolled_win), GLOBALS->clist_hiersearch_c_1);

    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);

    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_table_attach (GTK_TABLE (table), frameh, 0, 1, 255, 256,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked",GTK_SIGNAL_FUNC(ok_callback),GTK_OBJECT (GLOBALS->window_hiersearch_c_3));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1,
		"Add selected signals to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button2), "clicked",GTK_SIGNAL_FUNC(insert_callback),GTK_OBJECT (GLOBALS->window_hiersearch_c_3));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2,
		"Add children after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(GLOBALS->vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtkwave_signal_connect_object (GTK_OBJECT (button3), "clicked",GTK_SIGNAL_FUNC(bundle_callback_up),GTK_OBJECT (GLOBALS->window_hiersearch_c_3));
    	gtk_widget_show (button3);
    	gtk_tooltips_set_tip_2(tooltips, button3,
		"Bundle children into a single bit vector with the topmost signal as the LSB and the lowest as the MSB.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3, TRUE, FALSE, 0);

    	button3a = gtk_button_new_with_label (" Bundle Down ");
    	gtk_container_border_width (GTK_CONTAINER (button3a), 3);
    	gtkwave_signal_connect_object (GTK_OBJECT (button3a), "clicked",GTK_SIGNAL_FUNC(bundle_callback_down),GTK_OBJECT (GLOBALS->window_hiersearch_c_3));
    	gtk_widget_show (button3a);
    	gtk_tooltips_set_tip_2(tooltips, button3a,
		"Bundle children into a single bit vector with the topmost signal as the MSB and the lowest as the LSB.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3a, TRUE, FALSE, 0);
	}

    button4 = gtk_button_new_with_label (" Replace ");
    gtk_container_border_width (GTK_CONTAINER (button4), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button4), "clicked",GTK_SIGNAL_FUNC(replace_callback),GTK_OBJECT (GLOBALS->window_hiersearch_c_3));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4,
		"Replace highlighted signals on the main window with children shown above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Exit ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button5), "clicked",GTK_SIGNAL_FUNC(destroy_callback),GTK_OBJECT (GLOBALS->window_hiersearch_c_3));
    gtk_tooltips_set_tip_2(tooltips, button5,
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_hiersearch_c_3), table);

    gtk_widget_show(GLOBALS->window_hiersearch_c_3);

    if(!GLOBALS->current_tree_hiersearch_c_1)
	{
	GLOBALS->current_tree_hiersearch_c_1=GLOBALS->treeroot;
    	GLOBALS->h_selectedtree_hiersearch_c_1=NULL;
	}

    refresh_hier_tree(GLOBALS->current_tree_hiersearch_c_1);
}

