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
#include "gtk12compat.h"
#include "symbol.h"
#include "ttranslate.h"
#include "pipeio.h"
#include "debug.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static void args_entry_callback(GtkWidget *widget, GtkWidget *entry)
{
(void)widget;

G_CONST_RETURN gchar *entry_text;

entry_text=gtk_entry_get_text(GTK_ENTRY(entry));
entry_text = entry_text ? entry_text : "";

if(GLOBALS->ttranslate_args)
	{
	free_2(GLOBALS->ttranslate_args);
	}
GLOBALS->ttranslate_args = strdup_2(entry_text);

DEBUG(printf("Args Entry contents: %s\n",entry_text));
}


void init_ttrans_data(void)
{
int i;

if(!GLOBALS->ttranssel_filter) { GLOBALS->ttranssel_filter = calloc_2(TTRANS_FILTER_MAX+1, sizeof(char *)); }
if(!GLOBALS->ttrans_filter) { GLOBALS->ttrans_filter = calloc_2(TTRANS_FILTER_MAX+1, sizeof(struct pipe_ctx *)); }

for(i=0;i<TTRANS_FILTER_MAX+1;i++)
	{
	GLOBALS->ttranssel_filter[i] = NULL;
	GLOBALS->ttrans_filter[i] = NULL;
	}
}

void remove_all_ttrans_filters(void)
{
struct Global *GLOBALS_cache = GLOBALS;
unsigned int i, j;

for(j=0;j<GLOBALS->num_notebook_pages;j++)
        {
        GLOBALS = (*GLOBALS->contexts)[j];

	for(i=1;i<TTRANS_FILTER_MAX+1;i++)
		{
		if(GLOBALS->ttrans_filter[i])
			{
			pipeio_destroy(GLOBALS->ttrans_filter[i]);
			GLOBALS->ttrans_filter[i] = NULL;
			}

		if(GLOBALS->ttranssel_filter[i])
			{
			free_2(GLOBALS->ttranssel_filter[i]);
			GLOBALS->ttranssel_filter[i] = NULL;
			}
		}

	GLOBALS = GLOBALS_cache;
	}
}


int traverse_vector_nodes(Trptr t);

static void regen_display(void)
{
if(GLOBALS->signalarea && GLOBALS->wavearea)
	{
	GLOBALS->signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}


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
		regen_display();
	        }
	}
}
#endif

static void load_ttrans_filter(int which, char *name)
{

  FILE *stream;

  char *cmd;
  char exec_name[1025];
  char abs_path [1025];
  char* arg, end;
  int result;

  exec_name[0] = 0;
  abs_path[0]  = 0;

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
#if !defined __MINGW32__ && !defined _MSC_VER
  cmd = (char *)malloc_2(strlen(exec_name)+6+1);
  sprintf(cmd, "which %s", exec_name);
  stream = popen(cmd, "r");

  result = fscanf(stream, "%s", abs_path);

  if((strlen(abs_path) == 0)||(!result))
    {
      status_text("Could not find transaction filter process!\n");
      pclose(stream); /* cppcheck */
      return;

    }

  pclose(stream);
  free_2(cmd);
#else
  strcpy(abs_path, exec_name);
#endif

  /* remove_ttrans_filter(which, 0); ... should never happen from GUI, but perhaps possible from save files or other weirdness */
  if(!GLOBALS->ttrans_filter[which])
	{
  	GLOBALS->ttrans_filter[which] = pipeio_create(abs_path, arg);
	}
}

int install_ttrans_filter(int which)
{
int found = 0;

if((which<0)||(which>=(PROC_FILTER_MAX+1)))
        {
        which = 0;
        }

if(GLOBALS->traces.first)
        {
        Trptr t = GLOBALS->traces.first;
        while(t)
                {
                if(t->flags&TR_HIGHLIGHT)
                        {
			if((!t->vector) && (which))
				{
				bvptr v = combine_traces(1, t); /* down: make single signal a vector */
				if(v)
					{
					v->transaction_nd = t->n.nd; /* cache for savefile, disable */
					t->vector = 1;
					t->n.vec = v;		/* splice in */
					}
				}

                        if((t->vector) && (!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))))
                                {
                                t->t_filter = which;
				t->t_filter_converted = 0;

				/* back out allocation to revert (if any) */
				if(t->n.vec->transaction_cache)
					{
					int i;
					bvptr bv = t->n.vec;
					bvptr bv2;
					nptr ndcache = NULL;

					t->n.vec = bv->transaction_cache;
					if((t->n.vec->transaction_nd) && (!which))
						{
						ndcache = t->n.vec->transaction_nd;
						}

					while(bv)
						{
						bv2 = bv->transaction_chain;

						if(bv->bvname)
							{
							free_2(bv->bvname);
							}

						for(i=0;i<bv->numregions;i++)
							{
							free_2(bv->vectors[i]);
							}

						free_2(bv);
						bv = bv2;
						}

					t->name = t->n.vec->bvname;
	                  		if(GLOBALS->hier_max_level)
	                    			t->name = hier_extract(t->name, GLOBALS->hier_max_level);

					if(ndcache)
						{
						t->n.nd = ndcache;
						t->vector = 0;
						/* still need to deallocate old t->n.vec! */
						}
					}

				if(!which)
					{
					t->flags &= (~(TR_TTRANSLATED|TR_ANALOGMASK));
					}
					else
					{
					t->flags &= (~(TR_ANALOGMASK));
					t->flags |= TR_TTRANSLATED;
					if(t->transaction_args) free_2(t->transaction_args);
					if(GLOBALS->ttranslate_args)
						{
						t->transaction_args = strdup_2(GLOBALS->ttranslate_args);
						}
						else
						{
						t->transaction_args = NULL;
						}
					traverse_vector_nodes(t);
					}
                                found++;

				if(t->t_match)
					{
					Trptr curr_trace = t;
					t = t->t_next;
					while(t && (t->t_match != curr_trace))
						{
						t = t->t_next;
						}
					}
                                }
                        }
                t=GiveNextTrace(t);
                }
        }

if(found)
	{
	regen_display();
	}

return(found);
}

/************************************************************************/



static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

GLOBALS->is_active_ttranslate_c_2=0;
gtk_widget_destroy(GLOBALS->window_ttranslate_c_5);
GLOBALS->window_ttranslate_c_5 = NULL;
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
install_ttrans_filter(GLOBALS->current_filter_ttranslate_c_1);
destroy_callback(widget, nothing);
}

static void select_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
(void)widget;
(void)column;
(void)event;
(void)data;

GLOBALS->current_filter_ttranslate_c_1 = row + 1;
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
(void)widget;
(void)row;
(void)column;
(void)event;
(void)data;

GLOBALS->current_filter_ttranslate_c_1 = 0; /* none */
}


static void add_filter_callback_2(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

int i;
GtkCList *cl;

if(!GLOBALS->filesel_ok) { return; }

if(*GLOBALS->fileselbox_text)
	{
	for(i=0;i<GLOBALS->num_ttrans_filters;i++)
		{
		if(GLOBALS->ttranssel_filter[i])
			{
			if(!strcmp(GLOBALS->ttranssel_filter[i], *GLOBALS->fileselbox_text))
				{
				status_text("Filter already imported.\n");
				if(GLOBALS->is_active_ttranslate_c_2) gdk_window_raise(GLOBALS->window_ttranslate_c_5->window);
				return;
				}
			}
		}
	}

GLOBALS->num_ttrans_filters++;
load_ttrans_filter(GLOBALS->num_ttrans_filters, *GLOBALS->fileselbox_text);
if(GLOBALS->ttrans_filter[GLOBALS->num_ttrans_filters])
	{
	if(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]) free_2(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]);
	GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters] = malloc_2(strlen(*GLOBALS->fileselbox_text) + 1);
	strcpy(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters], *GLOBALS->fileselbox_text);

	cl=GTK_CLIST(GLOBALS->clist_ttranslate_c_2);
	gtk_clist_freeze(cl);
	gtk_clist_append(cl,(gchar **)&(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]));

	gtk_clist_set_column_width(cl,0,gtk_clist_optimal_column_width(cl,0));
	gtk_clist_thaw(cl);
	}
	else
	{
	GLOBALS->num_ttrans_filters--;
	}

if(GLOBALS->is_active_ttranslate_c_2) gdk_window_raise(GLOBALS->window_ttranslate_c_5->window);
}

static void add_filter_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

if(GLOBALS->num_ttrans_filters == TTRANS_FILTER_MAX)
	{
	status_text("Max number of transaction filters processes installed already.\n");
	return;
	}

fileselbox("Select Transaction Filter Process",&GLOBALS->fcurr_ttranslate_c_1,GTK_SIGNAL_FUNC(add_filter_callback_2), GTK_SIGNAL_FUNC(NULL),"*", 0);
}

/*
 * mainline..
 */
void ttrans_searchbox(char *title)
{
    int i;

    GtkWidget *scrolled_win;
    GtkWidget *vbox1, *hbox, *hbox0;
    GtkWidget *button1, *button5, *button6;
    gchar *titles[]={"Transaction Process Filter Select"};
    GtkWidget *frame2, *frameh, *frameh0;
    GtkWidget *table;
    GtkTooltips *tooltips;
    GtkWidget *label;
    GtkWidget *entry;

    if(GLOBALS->is_active_ttranslate_c_2)
	{
	gdk_window_raise(GLOBALS->window_ttranslate_c_5->window);
	return;
	}

    GLOBALS->is_active_ttranslate_c_2=1;
    GLOBALS->current_filter_ttranslate_c_1 = 0;

    /* create a new modal window */
    GLOBALS->window_ttranslate_c_5 = gtk_window_new(GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
    install_focus_cb(GLOBALS->window_ttranslate_c_5, ((char *)&GLOBALS->window_ttranslate_c_5) - ((char *)GLOBALS));

    gtk_window_set_title(GTK_WINDOW (GLOBALS->window_ttranslate_c_5), title);
    gtkwave_signal_connect(GTK_OBJECT (GLOBALS->window_ttranslate_c_5), "delete_event",(GtkSignalFunc) destroy_callback, NULL);

    tooltips=gtk_tooltips_new_2();

    table = gtk_table_new (256, 1, FALSE);
    gtk_widget_show (table);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (vbox1), 3);
    gtk_widget_show (vbox1);


    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 0, 253,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    GLOBALS->clist_ttranslate_c_2=gtk_clist_new_with_titles(1,titles);
    gtk_clist_column_titles_passive(GTK_CLIST(GLOBALS->clist_ttranslate_c_2));

    gtk_clist_set_selection_mode(GTK_CLIST(GLOBALS->clist_ttranslate_c_2), GTK_SELECTION_EXTENDED);
    gtkwave_signal_connect_object (GTK_OBJECT (GLOBALS->clist_ttranslate_c_2), "select_row",GTK_SIGNAL_FUNC(select_row_callback),NULL);
    gtkwave_signal_connect_object (GTK_OBJECT (GLOBALS->clist_ttranslate_c_2), "unselect_row",GTK_SIGNAL_FUNC(unselect_row_callback),NULL);

    for(i=0;i<GLOBALS->num_ttrans_filters;i++)
	{
	gtk_clist_append(GTK_CLIST(GLOBALS->clist_ttranslate_c_2),(gchar **)&(GLOBALS->ttranssel_filter[i+1]));
	}
    gtk_clist_set_column_width(GTK_CLIST(GLOBALS->clist_ttranslate_c_2),0,gtk_clist_optimal_column_width(GTK_CLIST(GLOBALS->clist_ttranslate_c_2),0));

    gtk_widget_show (GLOBALS->clist_ttranslate_c_2);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_widget_show(scrolled_win);

    /* gtk_scrolled_window_add_with_viewport doesn't seen to work right here.. */
    gtk_container_add (GTK_CONTAINER (scrolled_win), GLOBALS->clist_ttranslate_c_2);

    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);


    frameh0 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh0), 3);
    gtk_widget_show(frameh0);
    gtk_table_attach (GTK_TABLE (table), frameh0, 0, 1, 253, 254,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox0 = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox0);

    button6 = gtk_button_new_with_label (" Add Trans Filter to List ");
    gtk_container_border_width (GTK_CONTAINER (button6), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button6), "clicked",GTK_SIGNAL_FUNC(add_filter_callback),GTK_OBJECT (GLOBALS->window_ttranslate_c_5));
    gtk_widget_show (button6);
    gtk_tooltips_set_tip_2(tooltips, button6,
		"Bring up a file requester to add a transaction process filter to the filter select window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox0), button6, TRUE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frameh0), hbox0);

    	/* args entry box */
	{
	Trptr t=GLOBALS->traces.first;
	while(t)
	        {
		if(t->flags&TR_HIGHLIGHT)
			{
			if(t->transaction_args)
				{
				if(GLOBALS->ttranslate_args) free_2(GLOBALS->ttranslate_args);
				GLOBALS->ttranslate_args = strdup_2(t->transaction_args);
				break;
				}
	                }

	        t=t->t_next;
	        }

    	frameh0 = gtk_frame_new (NULL);
    	gtk_container_border_width (GTK_CONTAINER (frameh0), 3);
    	gtk_widget_show(frameh0);
    	gtk_table_attach (GTK_TABLE (table), frameh0, 0, 1, 254, 255,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

	label=gtk_label_new("Args:");
	entry=gtk_entry_new_with_max_length(1025);

	gtk_entry_set_text(GTK_ENTRY(entry), GLOBALS->ttranslate_args ? GLOBALS->ttranslate_args : "");
	gtk_signal_connect (GTK_OBJECT (entry), "activate",GTK_SIGNAL_FUNC (args_entry_callback), entry);
	gtk_signal_connect (GTK_OBJECT (entry), "changed",GTK_SIGNAL_FUNC (args_entry_callback), entry);
	hbox0=gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox0), label, FALSE, FALSE, 0);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox0), entry, TRUE, TRUE, 0);
	gtk_widget_show(entry);
	gtk_widget_show(hbox0);

	gtk_container_add (GTK_CONTAINER (frameh0), hbox0);
	}

    /* bottom OK/Cancel part */
    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_table_attach (GTK_TABLE (table), frameh, 0, 1, 255, 256,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label (" OK ");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button1), "clicked",GTK_SIGNAL_FUNC(ok_callback),GTK_OBJECT (GLOBALS->window_ttranslate_c_5));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1,
		"Add selected signals to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Cancel ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtkwave_signal_connect_object (GTK_OBJECT (button5), "clicked",GTK_SIGNAL_FUNC(destroy_callback),GTK_OBJECT (GLOBALS->window_ttranslate_c_5));
    gtk_tooltips_set_tip_2(tooltips, button5,
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (GLOBALS->window_ttranslate_c_5), table);

    gtk_widget_set_usize(GTK_WIDGET(GLOBALS->window_ttranslate_c_5), 400, 400);
    gtk_widget_show(GLOBALS->window_ttranslate_c_5);
}


/*
 * currently only called by parsewavline
 */
void set_current_translate_ttrans(char *name)
{
int i;

for(i=1;i<GLOBALS->num_ttrans_filters+1;i++)
	{
	if(!strcmp(GLOBALS->ttranssel_filter[i], name)) { GLOBALS->current_translate_ttrans = i; return; }
	}

if(GLOBALS->num_ttrans_filters < TTRANS_FILTER_MAX)
	{
	GLOBALS->num_ttrans_filters++;
	load_ttrans_filter(GLOBALS->num_ttrans_filters, name);
	if(!GLOBALS->ttrans_filter[GLOBALS->num_ttrans_filters])
		{
		GLOBALS->num_ttrans_filters--;
		GLOBALS->current_translate_ttrans = 0;
		}
		else
		{
		if(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]) free_2(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters]);
		GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters] = malloc_2(strlen(name) + 1);
		strcpy(GLOBALS->ttranssel_filter[GLOBALS->num_ttrans_filters], name);
		GLOBALS->current_translate_ttrans = GLOBALS->num_ttrans_filters;
		}
	}
}


int traverse_vector_nodes(Trptr t)
{
int i;
int cvt_ok = 0;

if((t->t_filter) && (t->flags & TR_TTRANSLATED) && (t->vector) && (!t->t_filter_converted))
	{
#if !defined _MSC_VER && !defined __MINGW32__
	int rc = save_nodes_to_trans(GLOBALS->ttrans_filter[t->t_filter]->sout, t);
#else
	int rc = save_nodes_to_trans((FILE *)(GLOBALS->ttrans_filter[t->t_filter]->g_hChildStd_IN_Wr), t);
#endif

	if(rc == VCDSAV_OK)
		{
		int is_finish = 0;
		bvptr prev_transaction_trace = NULL;

		while(!is_finish)
			{
			struct VectorEnt *vt_head = NULL, *vt_curr = NULL;
			struct VectorEnt *vt;
			struct VectorEnt *vprev;
			bvptr bv;
			int regions = 2;
			TimeType prev_tim = LLDescriptor(-1);
			char *trace_name = NULL;
			char *orig_name = t->n.vec->bvname;

			cvt_ok = 1;

			vt_head = vt_curr = vt = calloc_2(1, sizeof(struct VectorEnt) + 1);
			vt->time = LLDescriptor(-2);
			vprev = vt; /* for duplicate removal */

			vt_curr = vt_curr->next = vt = calloc_2(1, sizeof(struct VectorEnt) + 1);
			vt->time = LLDescriptor(-1);

			for(;;)
				{
				char buf[1025];
				char *pnt, *rtn;

#if !defined _MSC_VER && !defined __MINGW32__
				if(feof(GLOBALS->ttrans_filter[t->t_filter]->sin)) break; /* should never happen */

				buf[0] = 0;
				pnt = fgets(buf, 1024, GLOBALS->ttrans_filter[t->t_filter]->sin);
				if(!pnt) break;
				rtn = pnt;
				while(*rtn)
					{
					if((*rtn == '\n') || (*rtn == '\r')) { *rtn = 0; break; }
					rtn++;
					}
#else
			        {
			        BOOL bSuccess;
			        DWORD dwRead;
				int n;

			        for(n=0;n<1024;n++)
			                {
			                do      {
			                        bSuccess = ReadFile(GLOBALS->ttrans_filter[t->t_filter]->g_hChildStd_OUT_Rd, buf+n, 1, &dwRead, NULL);
			                        if((!bSuccess)||(buf[n]=='\n'))
			                                {
			                                goto ex;
			                                }

			                        } while(buf[n]=='\r');
			                }
ex:     			buf[n] = 0;
				pnt = buf;
			        }
#endif


				while(*pnt) { if(isspace((int)(unsigned char)*pnt)) pnt++; else break;}

				if(*pnt=='#')
					{
					TimeType tim = atoi_64(pnt+1) * GLOBALS->time_scale;
					int slen;
					char *sp;

					while(*pnt) { if(!isspace((int)(unsigned char)*pnt)) pnt++; else break; }
					while(*pnt) { if(isspace((int)(unsigned char)*pnt)) pnt++; else break; }

					sp = pnt;
					slen = strlen(sp);

					if(slen)
						{
						pnt = sp + slen - 1;
						do
							{
							if(isspace((int)(unsigned char)*pnt)) { *pnt = 0; pnt--; slen--; } else { break; }
							} while(pnt != (sp-1));
						}

					vt = calloc_2(1, sizeof(struct VectorEnt) + slen + 1);
					if(sp) strcpy((char *)vt->v, sp);

					if(tim > prev_tim)
						{
						prev_tim = vt->time = tim;
						vt_curr->next = vt;
						vt_curr = vt;
						vprev = vprev->next; /* bump forward the -2 node pointer */
						regions++;
						}
					else if(tim == prev_tim)
						{
						vt->time = prev_tim;
						free_2(vt_curr);
						vt_curr = vprev->next = vt; /* splice new one in -1 node place */
						}
					else
						{
						free_2(vt); /* throw it away */
						}

					continue;
					}
				else
				if((*pnt=='M')||(*pnt=='m'))
					{
					int mlen;
					pnt++;

					mlen = bijective_marker_id_string_len(pnt);
					if(mlen)
						{
						int which_marker = bijective_marker_id_string_hash(pnt);
						if((which_marker >= 0) && (which_marker < WAVE_NUM_NAMED_MARKERS))
							{
							TimeType tim = atoi_64(pnt+mlen) * GLOBALS->time_scale;
							int slen;
							char *sp;

							if(tim < LLDescriptor(0)) tim = LLDescriptor(-1);
							GLOBALS->named_markers[which_marker] = tim;

							while(*pnt) { if(!isspace((int)(unsigned char)*pnt)) pnt++; else break; }
							while(*pnt) { if(isspace((int)(unsigned char)*pnt)) pnt++; else break; }

							sp = pnt;
							slen = strlen(sp);

							if(slen)
								{
								pnt = sp + slen - 1;
								do
									{
									if(isspace((int)(unsigned char)*pnt)) { *pnt = 0; pnt--; slen--; } else { break; }
									} while(pnt != (sp-1));
								}

		                                	if(GLOBALS->marker_names[which_marker]) free_2(GLOBALS->marker_names[which_marker]);
		                                	GLOBALS->marker_names[which_marker] = (sp && (*sp) && (tim >= LLDescriptor(0))) ? strdup_2(sp) : NULL;
							}
						}

					continue;
					}
				else if(*pnt == '$')
					{
					if(!strncmp(pnt+1, "finish", 6))
						{
						is_finish = 1;
						break;
						}
					else
					if(!strncmp(pnt+1, "next", 4))
						{
						break;
						}
					else
					if(!strncmp(pnt+1, "name", 4))
						{
						int slen;
						char *sp;

						pnt+=5;
						while(*pnt) { if(isspace((int)(unsigned char)*pnt)) pnt++; else break; }

						sp = pnt;
						slen = strlen(sp);

						if(slen)
							{
							pnt = sp + slen - 1;
							do
								{
								if(isspace((int)(unsigned char)*pnt)) { *pnt = 0; pnt--; slen--; } else { break; }
								} while(pnt != (sp-1));
							}

						if(sp && *sp)
							{
							if(trace_name) free_2(trace_name);
							trace_name = strdup_2(sp);
							}
						}
					}
				}

			vt_curr = vt_curr->next = vt = calloc_2(1, sizeof(struct VectorEnt) + 1);
			vt->time = MAX_HISTENT_TIME - 1;
			regions++;

			/* vt_curr = */ vt_curr->next = vt = calloc_2(1, sizeof(struct VectorEnt) + 1); /* scan-build */
			vt->time = MAX_HISTENT_TIME;
			regions++;

			bv = calloc_2(1, sizeof(struct BitVector) + (sizeof(vptr) * (regions)));
			bv->bvname = strdup_2(trace_name ? trace_name : orig_name);
			bv->nbits = 1;
			bv->numregions = regions;
			bv->bits = t->n.vec->bits;

			vt = vt_head;
			for(i=0;i<regions;i++)
				{
				bv->vectors[i] = vt;
				vt = vt->next;
				}

			if(!prev_transaction_trace)
				{
				prev_transaction_trace = bv;
				bv->transaction_cache = t->n.vec; /* for possible restore later */
				t->n.vec = bv;

				t->t_filter_converted = 1;

				if(trace_name)	/* if NULL, no need to regen display as trace name didn't change */
					{
					t->name = t->n.vec->bvname;
			               		if(GLOBALS->hier_max_level)
			               			t->name = hier_extract(t->name, GLOBALS->hier_max_level);

					regen_display();
					}
				}
				else
				{
				prev_transaction_trace->transaction_chain = bv;
				prev_transaction_trace = bv;
				}
			}
		}
		else
		{
		/* failed */
		t->flags &= ~(TR_TTRANSLATED|TR_ANALOGMASK);
		}
	}

return(cvt_ok);
}
