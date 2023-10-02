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

static GwTrace *find_first_highlighted_trace(void)
{
    GwTrace *t = NULL;

    for (t = GLOBALS->traces.first; t; t = t->t_next) {
        if ((t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) || (!(t->flags & TR_HIGHLIGHT)) ||
            (!(t->name))) {
            continue;
        } else {
            break;
        }
    }

    return (t);
}

static GwTrace *find_next_highlighted_trace(GwTrace *t)
{
    if (t) {
        t = t->t_next;
        for (; t; t = t->t_next) {
            if ((t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) || (!(t->flags & TR_HIGHLIGHT)) ||
                (!(t->name))) {
                continue;
            } else {
                break;
            }
        }
    }

    return (t);
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
    GwTime basetime, maxbase, sttim, fintim;
    GwTrace *t = find_first_highlighted_trace();
    int totaltraces;
#ifdef DEBUG_PRINTF
    int passcount;
#endif
    int whichpass;
    GwTime middle = 0, width;

    if (!t)
        return;
    memset(s_head = &s_tmp, 0, sizeof(struct strace));
    s_head->trace = t;
    s_head->value = ST_ANY;
    s = s_head;

    while (t) {
        t = find_next_highlighted_trace(t);
        if (t) {
            s->next = g_alloca(sizeof(struct strace));
            memset(s = s->next, 0, sizeof(struct strace));
            s->trace = t;
            s->value = ST_ANY;
        }
    }

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);

    if (gw_marker_is_enabled(primary_marker)) {
        basetime = gw_marker_get_position(primary_marker);
    } else {
        if (direction == STRACE_BACKWARD) {
            basetime = MAX_HISTENT_TIME; // backwards
        } else {
            basetime = GLOBALS->tims.first; // go forwards
        }
    }

    sttim = GLOBALS->tims.first;
    fintim = GLOBALS->tims.last;

    for (whichpass = 0;; whichpass++) {
        if (direction == STRACE_BACKWARD) /* backwards */
        {
            maxbase = -1;
            s = s_head;
            while (s) {
                t = s->trace;
                GLOBALS->shift_timebase = t->shift;
                if (!(t->vector)) {
                    GwHistEnt *h;
                    GwHistEnt **hp;
                    GwUTime utt;
                    GwTime tt;

                    /* h= */ bsearch_node(t->n.nd, basetime - t->shift); /* scan-build */
                    hp = GLOBALS->max_compare_index;
                    if ((hp == &(t->n.nd->harray[1])) || (hp == &(t->n.nd->harray[0])))
                        return;
                    if (basetime == ((*hp)->time + GLOBALS->shift_timebase))
                        hp--;
                    h = *hp;
                    s->his.h = h;
                    utt = strace_adjust(h->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt > maxbase)
                        maxbase = tt;
                } else {
                    GwVectorEnt *v;
                    GwVectorEnt **vp;
                    GwUTime utt;
                    GwTime tt;

                    /* v= */ bsearch_vector(t->n.vec, basetime - t->shift); /* scan-build */
                    vp = GLOBALS->vmax_compare_index;
                    if ((vp == &(t->n.vec->vectors[1])) || (vp == &(t->n.vec->vectors[0])))
                        return;
                    if (basetime == ((*vp)->time + GLOBALS->shift_timebase))
                        vp--;
                    v = *vp;
                    s->his.v = v;
                    utt = strace_adjust(v->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt > maxbase)
                        maxbase = tt;
                }

                s = s->next;
            }
        } else /* go forward */
        {
            maxbase = MAX_HISTENT_TIME;
            s = s_head;
            while (s) {
                t = s->trace;
                GLOBALS->shift_timebase = t->shift;
                if (!(t->vector)) {
                    GwHistEnt *h;
                    GwUTime utt;
                    GwTime tt;

                    h = bsearch_node(t->n.nd, basetime - t->shift);
                    while (h->next && h->time == h->next->time)
                        h = h->next;
                    if ((whichpass) || gw_marker_is_enabled(primary_marker))
                        h = h->next;
                    if (!h)
                        return;
                    s->his.h = h;
                    utt = strace_adjust(h->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt < maxbase)
                        maxbase = tt;
                } else {
                    GwVectorEnt *v;
                    GwUTime utt;
                    GwTime tt;

                    v = bsearch_vector(t->n.vec, basetime - t->shift);
                    while (v->next && v->time == v->next->time)
                        v = v->next;
                    if ((whichpass) || gw_marker_is_enabled(primary_marker))
                        v = v->next;
                    if (!v)
                        return;
                    s->his.v = v;
                    utt = strace_adjust(v->time, GLOBALS->shift_timebase);
                    tt = utt;
                    if (tt < maxbase)
                        maxbase = tt;
                }

                s = s->next;
            }
        }

        s = s_head;
        totaltraces = 0; /* increment when not don't care */
        while (s) {
/* commented out, maybe will have possible future expansion later,
 * this was cut and pasted from strace.c */
#if 0
	char str[2];
#endif
            t = s->trace;
            s->search_result = 0; /* explicitly must set this */
            GLOBALS->shift_timebase = t->shift;

            if ((!t->vector) && (!(t->n.nd->extvals))) {
                if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) != maxbase) {
                    s->his.h = bsearch_node(t->n.nd, maxbase - t->shift);
                    while (s->his.h->next && s->his.h->time == s->his.h->next->time)
                        s->his.h = s->his.h->next;
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

                switch (s->value) {
                    case ST_ANY:
                        totaltraces++;
                        s->search_result = 1;
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
                        fprintf(stderr, "Internal error: st_type of %d\n", s->value);
                        exit(255);
                }

            } else {
                char *chval, *chval2;
                char ch;

                if (t->vector) {
                    if (strace_adjust(s->his.v->time, GLOBALS->shift_timebase) != maxbase) {
                        s->his.v = bsearch_vector(t->n.vec, maxbase - t->shift);
                        while (s->his.v->next && s->his.v->time == s->his.v->next->time)
                            s->his.v = s->his.v->next;
                    }
                    chval = convert_ascii(t, s->his.v);
                } else {
                    if (strace_adjust(s->his.h->time, GLOBALS->shift_timebase) != maxbase) {
                        s->his.h = bsearch_node(t->n.nd, maxbase - t->shift);
                        while (s->his.h->next && s->his.h->time == s->his.h->next->time)
                            s->his.h = s->his.h->next;
                    }
                    if (s->his.h->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(s->his.h->flags & GW_HIST_ENT_FLAG_STRING)) {
                            chval = convert_ascii_real(t, &s->his.h->v.h_double);
                        } else {
                            chval = convert_ascii_string((char *)s->his.h->v.h_vector);
                            chval2 = chval;
                            while ((ch = *chval2)) /* toupper() the string */
                            {
                                if ((ch >= 'a') && (ch <= 'z')) {
                                    *chval2 = ch - ('a' - 'A');
                                }
                                chval2++;
                            }
                        }
                    } else {
                        chval = convert_ascii_vec(t, s->his.h->v.h_vector);
                    }
                }

                switch (s->value) {
                    case ST_ANY:
                        totaltraces++;
                        s->search_result = 1;
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
                        fprintf(stderr, "Internal error: st_type of %d\n", s->value);
                        exit(255);
                }

                free_2(chval);
            }
            s = s->next;
        }

        if ((maxbase < sttim) || (maxbase > fintim))
            return;

#ifdef DEBUG_PRINTF
        DEBUG(printf("Maxbase: %" GW_TIME_FORMAT ", total traces: %d\n", maxbase, totaltraces));
        s = s_head;
        passcount = 0;
        while (s) {
            DEBUG(printf("\tPass: %d, Name: %s\n", s->search_result, s->trace->name));
            if (s->search_result)
                passcount++;
            s = s->next;
        }
#endif

        if (totaltraces) {
            break;
        }

        basetime = maxbase;
    }

    gw_marker_set_position(primary_marker, maxbase);
    gw_marker_set_enabled(primary_marker, TRUE);

    if (is_last_iteration) {
        update_time_box();

        width = (GwTime)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
        if ((maxbase < GLOBALS->tims.start) || (maxbase >= GLOBALS->tims.start + width)) {
            if ((maxbase < 0) || (maxbase < GLOBALS->tims.first) ||
                (maxbase > GLOBALS->tims.last)) {
                if (GLOBALS->tims.end > GLOBALS->tims.last)
                    GLOBALS->tims.end = GLOBALS->tims.last;
                middle = (GLOBALS->tims.start / 2) + (GLOBALS->tims.end / 2);
                if ((GLOBALS->tims.start & 1) && (GLOBALS->tims.end & 1))
                    middle++;
            } else {
                middle = maxbase;
            }

            GLOBALS->tims.start = time_trunc(middle - (width / 2));
            if (GLOBALS->tims.start + width > GLOBALS->tims.last)
                GLOBALS->tims.start = GLOBALS->tims.last - width;
            if (GLOBALS->tims.start < GLOBALS->tims.first)
                GLOBALS->tims.start = GLOBALS->tims.first;
            gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                     GLOBALS->tims.timecache = GLOBALS->tims.start);
        }

        redraw_signals_and_waves();
    }
}

void edge_search(int direction)
{
    int i;
    int i_high_cnt = ((GLOBALS->strace_repeat_count > 0) ? GLOBALS->strace_repeat_count : 1) - 1;

    for (i = 0; i <= i_high_cnt; i++) {
        edge_search_2(direction, (i == i_high_cnt));
    }
}

/************************************************/

void service_left_edge(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    edge_search(STRACE_BACKWARD);

    DEBUG(printf("Edge Left\n"));
}

void service_right_edge(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    edge_search(STRACE_FORWARD);

    DEBUG(printf("Edge Right\n"));
}
