/*
 * Copyright (c) Tony Bybell 2008-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "currenttime.h"
#include "pixmaps.h"
#include "strace.h"
#include "debug.h"

static Trptr find_first_highlighted_trace(void)
{
Trptr t = NULL;

for(t=GLOBALS->traces.first;t;t=t->t_next)
	{
	if ((t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))||(!(t->flags&TR_HIGHLIGHT))||(!(t->name)))
		{
		continue;
		}
		else
		{
		break;
		}
	}

return(t);
}

static Trptr find_next_highlighted_trace(Trptr t)
{
if(t)
	{
	t = t->t_next;
	for(;t;t=t->t_next)
		{
		if ((t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))||(!(t->flags&TR_HIGHLIGHT))||(!(t->name)))
			{
			continue;
			}
			else
			{
			break;
			}
		}
	}

return(t);
}

/************************************************/

/*
 * strace backward or forward..this was cut and
 * pasted from strace.c and special cased to handle
 * just a single trace.  this might be relaxed later.
 */
static void edge_search_2(int direction, int is_last_iteration)
{
struct strace s_tmp;
struct strace *s_head, *s;
TimeType basetime, maxbase, sttim, fintim;
Trptr t = find_first_highlighted_trace();
int totaltraces;
#ifdef DEBUG_PRINTF
int passcount;
#endif
int whichpass;
TimeType middle=0, width;

if(!t) return;
memset(s_head = &s_tmp, 0, sizeof(struct strace));
s_head->trace = t;
s_head->value = ST_ANY;
s = s_head;

while(t)
	{
	t = find_next_highlighted_trace(t);
	if(t)
		{
		s->next = wave_alloca(sizeof(struct strace));
		memset(s = s->next, 0, sizeof(struct strace));
		s->trace = t;
		s->value = ST_ANY;
		}
	}

if(direction==STRACE_BACKWARD) /* backwards */
{
if(GLOBALS->tims.marker<0)
	{
	basetime=MAX_HISTENT_TIME;
	}
	else
	{
	basetime=GLOBALS->tims.marker;
	}
}
else /* go forwards */
{
if(GLOBALS->tims.marker<0)
	{
	basetime=GLOBALS->tims.first;
	}
	else
	{
	basetime=GLOBALS->tims.marker;
	}
}

sttim=GLOBALS->tims.first;
fintim=GLOBALS->tims.last;

for(whichpass=0;;whichpass++)
{

if(direction==STRACE_BACKWARD) /* backwards */
{
maxbase=-1;
s=s_head;
while(s)
	{
	t=s->trace;
	GLOBALS->shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		hptr *hp;
		UTimeType utt;
		TimeType  tt;

		/* h= */ bsearch_node(t->n.nd, basetime - t->shift); /* scan-build */
		hp=GLOBALS->max_compare_index;
		if((hp==&(t->n.nd->harray[1]))||(hp==&(t->n.nd->harray[0]))) return;
		if(basetime == ((*hp)->time+GLOBALS->shift_timebase)) hp--;
		h=*hp;
		s->his.h=h;
		utt=strace_adjust(h->time,GLOBALS->shift_timebase); tt=utt;
		if(tt > maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		vptr *vp;
		UTimeType utt;
		TimeType  tt;

		/* v= */ bsearch_vector(t->n.vec, basetime - t->shift); /* scan-build */
		vp=GLOBALS->vmax_compare_index;
		if((vp==&(t->n.vec->vectors[1]))||(vp==&(t->n.vec->vectors[0]))) return;
		if(basetime == ((*vp)->time+GLOBALS->shift_timebase)) vp--;
		v=*vp;
		s->his.v=v;
		utt=strace_adjust(v->time,GLOBALS->shift_timebase); tt=utt;
		if(tt > maxbase) maxbase=tt;
		}

	s=s->next;
	}
}
else /* go forward */
{
maxbase=MAX_HISTENT_TIME;
s=s_head;
while(s)
	{
	t=s->trace;
	GLOBALS->shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime - t->shift);
		while(h->next && h->time==h->next->time) h=h->next;
		if((whichpass)||(GLOBALS->tims.marker>=0)) h=h->next;
		if(!h) return;
		s->his.h=h;
		utt=strace_adjust(h->time,GLOBALS->shift_timebase); tt=utt;
		if(tt < maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime - t->shift);
		while(v->next && v->time==v->next->time) v=v->next;
		if((whichpass)||(GLOBALS->tims.marker>=0)) v=v->next;
		if(!v) return;
		s->his.v=v;
		utt=strace_adjust(v->time,GLOBALS->shift_timebase); tt=utt;
		if(tt < maxbase) maxbase=tt;
		}

	s=s->next;
	}
}

s=s_head;
totaltraces=0;	/* increment when not don't care */
while(s)
	{
/* commented out, maybe will have possible future expansion later,
 * this was cut and pasted from strace.c */
#if 0
	char str[2];
#endif
	t=s->trace;
	s->search_result=0;	/* explicitly must set this */
	GLOBALS->shift_timebase=t->shift;

	if((!t->vector)&&(!(t->n.nd->extvals)))
		{
		if(strace_adjust(s->his.h->time,GLOBALS->shift_timebase)!=maxbase)
			{
			s->his.h=bsearch_node(t->n.nd, maxbase - t->shift);
			while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
			}
/* commented out, maybe will have possible future expansion later,
 * this was cut and pasted from strace.c */
#if 0
		if(t->flags&TR_INVERT)
                	{
                        str[0]=AN_STR_INV[s->his.h->v.h_val];
                        }
                        else
                        {
                        str[0]=AN_STR[s->his.h->v.h_val];
                        }
		str[1]=0x00;
#endif

		switch(s->value)
			{
			case ST_ANY:
				totaltraces++;
				s->search_result=1;
				break;

/* commented out, maybe will have possible future expansion later,
 * this was cut and pasted from strace.c */
#if 0
			case ST_DC:
				break;

			case ST_HIGH:
				totaltraces++;
				if((str[0]=='1')||(str[0]=='H')) s->search_result=1;
				break;

			case ST_RISE:
				if((str[0]=='1')||(str[0]=='H')) s->search_result=1;
				totaltraces++;
				break;

			case ST_LOW:
				totaltraces++;
				if((str[0]=='0')||(str[0]=='L')) s->search_result=1;
				break;

			case ST_FALL:
				totaltraces++;
				if((str[0]=='0')||(str[0]=='L')) s->search_result=1;
				break;

			case ST_MID:
				totaltraces++;
				if(str[0]=='Z')
 					s->search_result=1;
				break;

			case ST_X:
				totaltraces++;
				if(str[0]=='X') s->search_result=1;
				break;

			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr_i(s->string,str)) s->search_result=1;
				break;
#endif

			default:
				fprintf(stderr, "Internal error: st_type of %d\n",s->value);
				exit(255);
			}


		}
		else
		{
		char *chval, *chval2;
		char ch;

		if(t->vector)
			{
			if(strace_adjust(s->his.v->time,GLOBALS->shift_timebase)!=maxbase)
				{
				s->his.v=bsearch_vector(t->n.vec, maxbase - t->shift);
				while(s->his.v->next && s->his.v->time==s->his.v->next->time) s->his.v=s->his.v->next;
				}
			chval=convert_ascii(t,s->his.v);
			}
			else
			{
			if(strace_adjust(s->his.h->time,GLOBALS->shift_timebase)!=maxbase)
				{
				s->his.h=bsearch_node(t->n.nd, maxbase - t->shift);
				while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
				}
			if(s->his.h->flags&HIST_REAL)
				{
				if(!(s->his.h->flags&HIST_STRING))
					{
#ifdef WAVE_HAS_H_DOUBLE
					chval=convert_ascii_real(t, &s->his.h->v.h_double);
#else
					chval=convert_ascii_real(t, (double *)s->his.h->v.h_vector);
#endif
					}
					else
					{
					chval=convert_ascii_string((char *)s->his.h->v.h_vector);
					chval2=chval;
					while((ch=*chval2))	/* toupper() the string */
						{
						if((ch>='a')&&(ch<='z')) { *chval2= ch-('a'-'A'); }
						chval2++;
						}
					}
				}
				else
				{
				chval=convert_ascii_vec(t,s->his.h->v.h_vector);
				}
			}

		switch(s->value)
			{
			case ST_ANY:
				totaltraces++;
				s->search_result=1;
				break;

/* commented out, maybe will have possible future expansion later,
 * this was cut and pasted from strace.c */
#if 0
			case ST_DC:
				break;

			case ST_RISE:
			case ST_FALL:
				totaltraces++;
				break;

			case ST_HIGH:
				totaltraces++;
				if((chval2=chval))
				while((ch=*(chval2++)))
					{
					if(((ch>='1')&&(ch<='9'))||(ch=='h')||(ch=='H')||((ch>='A')&&(ch<='F')))
						{
						s->search_result=1;
						break;
						}
					}
				break;

			case ST_LOW:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='0')&&(ch!='l')&&(ch!='L'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_MID:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='z')&&(ch!='Z'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_X:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if((ch!='X')&&(ch!='W')&&(ch!='x')&&(ch!='w'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr_i(chval, s->string)) s->search_result=1;
				break;
#endif

			default:
				fprintf(stderr, "Internal error: st_type of %d\n",s->value);
				exit(255);
			}

		free_2(chval);
		}
	s=s->next;
	}

if((maxbase<sttim)||(maxbase>fintim)) return;

#ifdef DEBUG_PRINTF
DEBUG(printf("Maxbase: "TTFormat", total traces: %d\n",maxbase, totaltraces));
s=s_head;
passcount=0;
while(s)
	{
	DEBUG(printf("\tPass: %d, Name: %s\n",s->search_result, s->trace->name));
	if(s->search_result) passcount++;
	s=s->next;
	}
#endif

if(totaltraces)
	{
	break;
	}

basetime=maxbase;
}


GLOBALS->tims.marker=maxbase;
if(is_last_iteration)
	{
	update_markertime(GLOBALS->tims.marker);

	width=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	if((GLOBALS->tims.marker<GLOBALS->tims.start)||(GLOBALS->tims.marker>=GLOBALS->tims.start+width))
		{
		if((GLOBALS->tims.marker<0)||(GLOBALS->tims.marker<GLOBALS->tims.first)||(GLOBALS->tims.marker>GLOBALS->tims.last))
		                {
		                if(GLOBALS->tims.end>GLOBALS->tims.last) GLOBALS->tims.end=GLOBALS->tims.last;
		                middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
		                if((GLOBALS->tims.start&1)&&(GLOBALS->tims.end&1)) middle++;
		                }
		                else
		                {
		                middle=GLOBALS->tims.marker;
		                }

		GLOBALS->tims.start=time_trunc(middle-(width/2));
		if(GLOBALS->tims.start+width>GLOBALS->tims.last) GLOBALS->tims.start=GLOBALS->tims.last-width;
		if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
		GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.timecache=GLOBALS->tims.start;
		}

	MaxSignalLength();
	signalarea_configure_event(GLOBALS->signalarea, NULL);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

void edge_search(int direction)
{
int i;
int i_high_cnt = ((GLOBALS->strace_repeat_count > 0) ? GLOBALS->strace_repeat_count : 1) - 1;

for(i=0;i<=i_high_cnt;i++)
	{
	edge_search_2(direction, (i == i_high_cnt));
	}
}

/************************************************/

void
service_left_edge(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nFind Previous Edge");
        help_text(
                " moves the marker to the closest transition to the left of the marker"
		" of the first highlighted trace.  If the marker is not nailed down, it starts from max time."
        );
        return;
        }

edge_search(STRACE_BACKWARD);

DEBUG(printf("Edge Left\n"));
}

void
service_right_edge(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nFind Next Edge");
        help_text(
                " moves the marker to the closest transition to the right of the marker"
		" of the first highlighted trace.  If the marker is not nailed down, it starts from min time."
        );
        return;
        }

edge_search(STRACE_FORWARD);

DEBUG(printf("Edge Right\n"));
}

/* Create shift buttons */
GtkWidget *
create_edge_buttons (void)
{
GtkWidget *table;
GtkWidget *table2;
GtkWidget *frame;
GtkWidget *main_vbox;
GtkWidget *b1;
GtkWidget *b2;
GtkWidget *pixmapwid1, *pixmapwid2;

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

pixmapwid1=gtk_pixmap_new(GLOBALS->larrow_pixmap, GLOBALS->larrow_mask);
gtk_widget_show(pixmapwid1);
pixmapwid2=gtk_pixmap_new(GLOBALS->rarrow_pixmap, GLOBALS->rarrow_mask);
gtk_widget_show(pixmapwid2);

/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 1, FALSE);

main_vbox = gtk_vbox_new (FALSE, 1);
gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Edge ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = gtk_table_new (2, 1, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapwid1);
gtk_table_attach (GTK_TABLE (table2), b1, 0, 1, 0, 1,
			GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b1), "clicked", GTK_SIGNAL_FUNC(service_left_edge), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b1, "Find next transition of highlighted trace scanning left", NULL);
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapwid2);
gtk_table_attach (GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b2), "clicked", GTK_SIGNAL_FUNC(service_right_edge), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b2, "Find next transition of highlighted trace scanning right", NULL);
gtk_widget_show(b2);

gtk_container_add (GTK_CONTAINER (frame), table2);
gtk_widget_show(table2);
return(table);
}

