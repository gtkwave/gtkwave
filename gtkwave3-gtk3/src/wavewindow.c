/*
 * Copyright (c) Tony Bybell 1999-2019.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "gtk23compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "bsearch.h"
#include "color.h"
#include "rc.h"
#include "strace.h"
#include "debug.h"
#include "main.h"

#if !defined _ISOC99_SOURCE
#define _ISOC99_SOURCE 1
#endif
#include <math.h>

#define WAVE_CAIRO_050_OFFSET (GLOBALS->cairo_050_offset)

static void rendertimebar(void);
static void draw_hptr_trace(Trptr t, hptr h, int which, int dodraw, int kill_grid);
static void draw_hptr_trace_vector(Trptr t, hptr h, int which);
static void draw_vptr_trace(Trptr t, vptr v, int which);
static void rendertraces(void);
static void rendertimes(void);

static const GdkModifierType   bmask[4]= {0, GDK_BUTTON1_MASK, 0, GDK_BUTTON3_MASK };                   /* button 1, 3 press/rel encodings */
#ifndef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
static const GdkEventMask    m_bmask[4]= {0, GDK_BUTTON1_MOTION_MASK, 0, GDK_BUTTON3_MOTION_MASK };     /* button 1, 3 motion encodings */
#endif

void wavewindow_paint(cairo_t *cr) {
    int scale_factor = XXX_gtk_widget_get_scale_factor(GLOBALS->signalarea);
	cairo_matrix_t prev_matrix;
	cairo_get_matrix(cr, &prev_matrix);
	cairo_scale(cr, 1.0/scale_factor, 1.0/scale_factor);
    cairo_set_source_surface(cr, GLOBALS->surface_wavepixmap_wavewindow_c_1, 0, 0);
    cairo_paint (cr);
    cairo_set_matrix(cr, &prev_matrix);
}

/******************************************************************/

static void update_dual(void)
{
if(GLOBALS->dual_ctx && !GLOBALS->dual_race_lock)
        {
        GLOBALS->dual_ctx[GLOBALS->dual_id].zoom = GLOBALS->tims.zoom;
        GLOBALS->dual_ctx[GLOBALS->dual_id].marker = GLOBALS->tims.marker;
        GLOBALS->dual_ctx[GLOBALS->dual_id].baseline = GLOBALS->tims.baseline;
        GLOBALS->dual_ctx[GLOBALS->dual_id].left_margin_time = GLOBALS->tims.start;
        GLOBALS->dual_ctx[GLOBALS->dual_id].use_new_times = 1;
        }
}

/******************************************************************/
#if GTK_CHECK_VERSION(3,0,0)
static int gesture_filter_set = 0; /* to prevent floods of gesture events before next draw */
static int gesture_filter_cnt = 0; /* to prevent floods of gesture events before next draw */
static int gesture_in_zoom = 0; /* for suppression of filtering of redraws: indicates zoom state change, service_hslider() decrements this */
#endif

#ifdef WAVE_ALLOW_SLIDER_ZOOM
#if GTK_CHECK_VERSION(3,0,0)

static void (*draw_slider_p)    (GtkStyle               *style,
                                 cairo_t                *cr,
                                 GtkStateType            state_type,
                                 GtkShadowType           shadow_type,
                                 GtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 GtkOrientation          orientation) = NULL; /* This is intended to be global...only needed once per toolkit */

static void draw_slider         (GtkStyle               *style,
                                 cairo_t                *cr,
                                 GtkStateType            state_type,
                                 GtkShadowType           shadow_type,
                                 GtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 GtkOrientation          orientation)
{
if((GLOBALS)&&(widget == GLOBALS->hscroll_wavewindow_c_2))
	{
	GtkAllocation allocation;
      	gtk_widget_get_allocation(widget, &allocation);
	GLOBALS->str_wid_x = x - allocation.x;
	GLOBALS->str_wid_width = width;
	GLOBALS->str_wid_bigw = allocation.width;
	GLOBALS->str_wid_height = height;
	}

draw_slider_p(style, cr, state_type, shadow_type, widget, detail, x, y, width, height, orientation);
}
#else
static void (*draw_slider_p)    (GtkStyle               *style,
                                 GdkWindow              *window,
                                 GtkStateType            state_type,
                                 GtkShadowType           shadow_type,
                                 GdkRectangle           *area,
                                 GtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 GtkOrientation          orientation) = NULL; /* This is intended to be global...only needed once per toolkit */


static void draw_slider         (GtkStyle               *style,
                                 GdkWindow              *window,
                                 GtkStateType            state_type,
                                 GtkShadowType           shadow_type,
                                 GdkRectangle           *area,
                                 GtkWidget              *widget,
                                 const gchar            *detail,
                                 gint                    x,
                                 gint                    y,
                                 gint                    width,
                                 gint                    height,
                                 GtkOrientation          orientation)
{
if((GLOBALS)&&(widget == GLOBALS->hscroll_wavewindow_c_2))
        {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        GLOBALS->str_wid_x = x - allocation.x;
        GLOBALS->str_wid_width = width;
        GLOBALS->str_wid_bigw = allocation.width;
        GLOBALS->str_wid_height = height;
        }

draw_slider_p(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
}
#endif

static gint slider_bpr(GtkWidget *widget, GdkEventButton *event)
{
(void)widget;

int xi = event->x;
int xl = GLOBALS->str_wid_x;
int xr = GLOBALS->str_wid_x + GLOBALS->str_wid_width;

if((xi > (xr-8)) && (xi < (xr+8)))
	{
	GLOBALS->str_wid_state = 1;
	return(TRUE);
	}
else if((xi < (xl+8)) && (xi > (xl-8)))
	{
	GLOBALS->str_wid_state = -1;
	return(TRUE);
	}

return(FALSE);
}


static gint slider_brr(GtkWidget *widget, GdkEventButton *event)
{
(void)widget;
(void)event;

GLOBALS->str_wid_state = 0;
return(FALSE);
}


static gint slider_mnr(GtkWidget *widget, GdkEventMotion *event)
{
(void)widget;

GdkModifierType state;
gdouble my_x, xmax, ratio;
TimeType l_margin, r_margin;

gint xi, yi;

int dummy_x, dummy_y;
get_window_xypos(&dummy_x, &dummy_y);

if(event->is_hint)
        {
        WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
        }
        else
        {
        x = event->x; /* scan-build */
        y = event->y; /* scan-build */
        state = event->state;
        }

if((GLOBALS->str_wid_state)&&(!(state & (GDK_BUTTON1_MASK|GDK_BUTTON3_MASK))))
	{
	GLOBALS->str_wid_state = 0;
	}

if(GLOBALS->str_wid_state == 1)
	{
	my_x = event->x - GLOBALS->str_wid_height;
	xmax = GLOBALS->str_wid_bigw - (2*GLOBALS->str_wid_height) - GLOBALS->str_wid_slider;
	if(xmax > 1.0)
		{
		ratio = my_x/xmax;
		r_margin = (gdouble)(GLOBALS->tims.last - GLOBALS->tims.first) * ratio + GLOBALS->tims.first;
		if((r_margin > GLOBALS->tims.start) && (r_margin <= GLOBALS->tims.last))
			{
			service_dragzoom(GLOBALS->tims.start, r_margin);
			}
		return(TRUE);
		}
	}
else
if(GLOBALS->str_wid_state == -1)
	{
	my_x = event->x - GLOBALS->str_wid_height;
	xmax = GLOBALS->str_wid_bigw - (2*GLOBALS->str_wid_height) - GLOBALS->str_wid_slider;
	if(xmax > 1.0)
		{
		ratio = my_x/xmax;
		l_margin = (gdouble)(GLOBALS->tims.last - GLOBALS->tims.first) * ratio + GLOBALS->tims.first;
		r_margin = GLOBALS->tims.end;
		if((l_margin >= GLOBALS->tims.first) && (l_margin < GLOBALS->tims.end))
			{
			if(r_margin > GLOBALS->tims.last) r_margin = GLOBALS->tims.last;
			service_dragzoom(l_margin, r_margin);
			}
		return(TRUE);
		}
	}

return(FALSE);
}
#endif

/******************************************************************/

void XXX_gdk_draw_line(cairo_t *cr, wave_rgb_t gc, gint _x1, gint _y1, gint _x2, gint _y2)
{
cairo_set_source_rgba (cr, gc.r, gc.g, gc.b, gc.a);
cairo_move_to (cr, _x1+WAVE_CAIRO_050_OFFSET, _y1+WAVE_CAIRO_050_OFFSET);
cairo_line_to (cr, _x2+WAVE_CAIRO_050_OFFSET, _y2+WAVE_CAIRO_050_OFFSET);
cairo_stroke (cr);
}

void XXX_gdk_draw_rectangle(cairo_t *cr, wave_rgb_t gc, gboolean filled, gint _x1, gint _y1, gint _w, gint _h)
{
cairo_set_source_rgba (cr, gc.r, gc.g, gc.b, gc.a);
if(filled)
	{
	cairo_rectangle (cr, _x1, _y1, _w, _h);
        cairo_fill(cr);
	}
	else
	{
	cairo_rectangle (cr, _x1+WAVE_CAIRO_050_OFFSET, _y1+WAVE_CAIRO_050_OFFSET, _w, _h);
        cairo_stroke(cr);
	}
}


/*
 * aldec-like "snap" feature...
 */
TimeType cook_markertime(TimeType marker, gint x, gint y)
{
int i, num_traces_displayable;
Trptr t = NULL;
TimeType lft, rgh;
char lftinv, rghinv;
gdouble xlft, xrgh;
gdouble xlftd, xrghd;
TimeType closest_named = MAX_HISTENT_TIME;
int closest_which = -1;
gint xold = x, yold = y;
TraceEnt t_trans;

if(!GLOBALS->cursor_snap) return(marker);

/* potential snapping to a named marker time */
for(i=0;i<WAVE_NUM_NAMED_MARKERS;i++)
	{
	if(GLOBALS->named_markers[i] != -1)
		{
		TimeType dlt;

		if((GLOBALS->named_markers[i]>=GLOBALS->tims.start)&&(GLOBALS->named_markers[i]<=GLOBALS->tims.end)&&(GLOBALS->named_markers[i]<=GLOBALS->tims.last))
			{
			if(marker < GLOBALS->named_markers[i])
				{
				dlt = GLOBALS->named_markers[i] - marker;
				}
				else
				{
				dlt = marker - GLOBALS->named_markers[i];
				}

			if(dlt < closest_named)
				{
				closest_named = dlt;
				closest_which = i;
				}
			}
		}
	}

GtkAllocation allocation;
gtk_widget_get_allocation(GLOBALS->wavearea, &allocation);

num_traces_displayable=allocation.height/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

y-=GLOBALS->fontheight;
if(y<0) y=0;
y/=GLOBALS->fontheight;		    /* y now indicates the trace in question */
if(y>num_traces_displayable) y=num_traces_displayable;

t=GLOBALS->topmost_trace;
for(i=0;i<y;i++)
	{
	if(!t) goto bot;
	t=GiveNextTrace(t);
	}

if(!t) goto bot;
if((t->flags&(/*TR_BLANK|*/TR_EXCLUDE))) /* TR_BLANK removed because of transaction handling below... */
	{
	t = NULL;
	goto bot;
	}

if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
	Trptr tscan = t;
	int bcnt = 0;
        while((tscan) && (tscan = GivePrevTrace(tscan)))
                {
                if(!(tscan->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                        {
                        if(tscan->flags & TR_TTRANSLATED)
                                {
                                break; /* found it */
                                }
                                else
                                {
                                tscan = NULL;
                                }
                        }
			else
			{
			bcnt++; /* bcnt is number of blank traces */
			}
                }

	if((tscan)&&(tscan->vector))
		{
		bvptr bv = tscan->n.vec;
		do
			{
			bv = bv->transaction_chain; /* correlate to blank trace */
			} while(bv && (bcnt--));
		if(bv)
			{
			memcpy(&t_trans, tscan, sizeof(TraceEnt)); /* substitute into a synthetic trace */
			t_trans.n.vec = bv;
			t_trans.vector = 1;

                        t_trans.name = bv->bvname;
                        if(GLOBALS->hier_max_level)
                        	t_trans.name = hier_extract(t_trans.name, GLOBALS->hier_max_level);

			t = &t_trans;
			goto process_trace;
			}
		}
        }

if((t->flags&TR_BLANK))
        {
        t = NULL;
        goto bot;
        }

if(t->flags & TR_ANALOG_BLANK_STRETCH)	/* seek to real analog trace if present... */
	{
	while((t) && (t = GivePrevTrace(t)))
		{
		if(!(t->flags & TR_ANALOG_BLANK_STRETCH))
			{
			if(t->flags & TR_ANALOGMASK)
				{
				break; /* found it */
				}
				else
				{
				t = NULL;
				}
			}
		}
	}
if(!t) goto bot;

process_trace:
if(t->vector)
	{
	vptr v = bsearch_vector(t->n.vec, marker - t->shift);
	vptr v2 = v ? v->next : NULL;

	if((!v)||(!v2)) goto bot;	/* should never happen */

	lft = v->time;
	rgh = v2->time;
	}
	else
	{
	hptr h = bsearch_node(t->n.nd, marker - t->shift);
	hptr h2 = h ? h->next : NULL;

	if((!h)||(!h2)) goto bot;	/* should never happen */

	lft = h->time;
	rgh = h2->time;
	}


lftinv = (lft < (GLOBALS->tims.start - t->shift))||(lft >= (GLOBALS->tims.end - t->shift))||(lft >= (GLOBALS->tims.last - t->shift));
rghinv = (rgh < (GLOBALS->tims.start - t->shift))||(rgh >= (GLOBALS->tims.end - t->shift))||(rgh >= (GLOBALS->tims.last - t->shift));

xlft = (lft + t->shift - GLOBALS->tims.start) * GLOBALS->pxns;
xrgh = (rgh + t->shift - GLOBALS->tims.start) * GLOBALS->pxns;

xlftd = xlft - x;
if(xlftd<(gdouble)0.0) xlftd = ((gdouble)0.0) - xlftd;

xrghd = xrgh - x;
if(xrghd<(gdouble)0.0) xrghd = ((gdouble)0.0) - xrghd;

if(xlftd<=xrghd)
	{
	if((!lftinv)&&(xlftd<=GLOBALS->cursor_snap))
		{
		if(closest_which >= 0)
		        {
        		if((closest_named * GLOBALS->pxns) < xlftd)
                		{
                		marker = GLOBALS->named_markers[closest_which];
				goto xit;
                		}
			}

		marker = lft + t->shift;
		goto xit;
		}
	}
	else
	{
	if((!rghinv)&&(xrghd<=GLOBALS->cursor_snap))
		{
		if(closest_which >= 0)
		        {
        		if((closest_named * GLOBALS->pxns) < xrghd)
                		{
                		marker = GLOBALS->named_markers[closest_which];
				goto xit;
                		}
			}

		marker = rgh + t->shift;
		goto xit;
		}
	}

bot:
if(closest_which >= 0)
	{
	if((closest_named * GLOBALS->pxns) <= GLOBALS->cursor_snap)
		{
		marker = GLOBALS->named_markers[closest_which];
		}
	}

xit:
GLOBALS->mouseover_counter = -1;
move_mouseover(t, xold, yold, marker);
return(marker);
}

static void render_individual_named_marker(int i, wave_rgb_t gc, int blackout)
{
gdouble pixstep;
gint xl;
TimeType t;

if((t=GLOBALS->named_markers[i])!=-1)
	{
	if((t>=GLOBALS->tims.start)&&(t<=GLOBALS->tims.last)
		&&(t<=GLOBALS->tims.end))
		{
		/* this needs to be here rather than outside the loop as gcc does some
		   optimizations that cause it to calculate slightly different from the marker if it's not here */
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);

		xl=((gdouble)(t-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			static const double dashed1[] = { 5.0, 3.0 };
			char nbuff[16];
			make_bijective_marker_id_string(nbuff, i);

			cairo_set_dash(GLOBALS->cr_wavepixmap_wavewindow_c_1, dashed1, sizeof(dashed1) / sizeof(dashed1[0]), 0);
                        XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1,
                                        gc,
                                        xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			cairo_set_dash(GLOBALS->cr_wavepixmap_wavewindow_c_1, dashed1, 0, 0);

			if((!GLOBALS->marker_names[i])||(!GLOBALS->marker_names[i][0]))
				{
				XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->wavefont_smaller,
					&gc,
					xl-(font_engine_string_measure(GLOBALS->wavefont_smaller, nbuff)>>1)+WAVE_CAIRO_050_OFFSET,
					GLOBALS->fontheight-2+WAVE_CAIRO_050_OFFSET, nbuff);
				}
				else
				{
				int width = font_engine_string_measure(GLOBALS->wavefont_smaller, GLOBALS->marker_names[i]);
				if(blackout) /* blackout background so text is legible if overlaid with other marker labels */
					{
					XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1, TRUE,
						xl-(width>>1), GLOBALS->fontheight-2-GLOBALS->wavefont_smaller->ascent,
						width, GLOBALS->wavefont_smaller->ascent + GLOBALS->wavefont_smaller->descent);
					}

				XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->wavefont_smaller,
					&gc,
					xl-(width>>1)+WAVE_CAIRO_050_OFFSET,
					GLOBALS->fontheight-2+WAVE_CAIRO_050_OFFSET, GLOBALS->marker_names[i]);
				}
			}
		}
	}
}

static void draw_named_markers(void)
{
int i;

if(!GLOBALS->cr_wavepixmap_wavewindow_c_1) return;

for(i=0;i<WAVE_NUM_NAMED_MARKERS;i++)
	{
	if(i != GLOBALS->named_marker_lock_idx)
		{
		render_individual_named_marker(i, GLOBALS->rgb_gc.gc_mark_wavewindow_c_1, 0);
		}
	}

if(GLOBALS->named_marker_lock_idx >= 0)
	{
	render_individual_named_marker(GLOBALS->named_marker_lock_idx, GLOBALS->rgb_gc.gc_umark_wavewindow_c_1, 1);
	}
}


static void sync_marker(void)
{
if((GLOBALS->tims.prevmarker==-1)&&(GLOBALS->tims.marker!=-1))
	{
	GLOBALS->signalwindow_width_dirty=1;
	}
	else
if((GLOBALS->tims.marker==-1)&&(GLOBALS->tims.prevmarker!=-1))
	{
	GLOBALS->signalwindow_width_dirty=1;
	}
GLOBALS->tims.prevmarker=GLOBALS->tims.marker;

/* additional case for race conditions with MaxSignalLength */
if(((GLOBALS->tims.resizemarker==-1)||(GLOBALS->tims.resizemarker2==-1)) && (GLOBALS->tims.resizemarker!=GLOBALS->tims.resizemarker2))
	{
	GLOBALS->signalwindow_width_dirty=1;
	}
}


static void draw_marker(void)
{
gdouble pixstep;
gint xl;

if(!gtk_widget_get_window(GLOBALS->wavearea)) return;

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
GdkDrawingContext *gdc;
#endif
cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->wavearea)), &gdc);
cairo_set_line_width(cr, GLOBALS->cr_line_width);
cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

GLOBALS->m1x_wavewindow_c_1=GLOBALS->m2x_wavewindow_c_1=-1;

if(GLOBALS->tims.baseline>=0)
	{
	if((GLOBALS->tims.baseline>=GLOBALS->tims.start)&&(GLOBALS->tims.baseline<=GLOBALS->tims.last)
		&&(GLOBALS->tims.baseline<=GLOBALS->tims.end))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		xl=((gdouble)(GLOBALS->tims.baseline-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			XXX_gdk_draw_line(cr,GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1,xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			}
		}
	}

if(GLOBALS->tims.marker>=0)
	{
	if((GLOBALS->tims.marker>=GLOBALS->tims.start)&&(GLOBALS->tims.marker<=GLOBALS->tims.last)
		&&(GLOBALS->tims.marker<=GLOBALS->tims.end))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		xl=((gdouble)(GLOBALS->tims.marker-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			XXX_gdk_draw_line(cr,GLOBALS->rgb_gc.gc_umark_wavewindow_c_1,xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			GLOBALS->m1x_wavewindow_c_1=xl;
			}
		}
	}


if((GLOBALS->enable_ghost_marker)&&(GLOBALS->in_button_press_wavewindow_c_1)&&(GLOBALS->tims.lmbcache>=0))
	{
	if((GLOBALS->tims.lmbcache>=GLOBALS->tims.start)&&(GLOBALS->tims.lmbcache<=GLOBALS->tims.last)
		&&(GLOBALS->tims.lmbcache<=GLOBALS->tims.end))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		xl=((gdouble)(GLOBALS->tims.lmbcache-GLOBALS->tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<GLOBALS->wavewidth))
			{
			XXX_gdk_draw_line(cr,GLOBALS->rgb_gc.gc_umark_wavewindow_c_1,xl, GLOBALS->fontheight-1, xl, GLOBALS->waveheight-1);
			GLOBALS->m2x_wavewindow_c_1=xl;
			}
		}
	}

if(GLOBALS->m1x_wavewindow_c_1>GLOBALS->m2x_wavewindow_c_1)		/* ensure m1x <= m2x for partitioned refresh */
	{
	gint t;

	t=GLOBALS->m1x_wavewindow_c_1;
	GLOBALS->m1x_wavewindow_c_1=GLOBALS->m2x_wavewindow_c_1;
	GLOBALS->m2x_wavewindow_c_1=t;
	}

if(GLOBALS->m1x_wavewindow_c_1==-1) GLOBALS->m1x_wavewindow_c_1=GLOBALS->m2x_wavewindow_c_1;	/* make both markers same if no ghost marker or v.v. */

update_dual();

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->wavearea), gdc);
#else
cairo_destroy (cr);
#endif

// #ifdef GDK_WINDOWING_WAYLAND
// if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) gtk_widget_queue_draw(GLOBALS->wavearea);
// #endif
}


static void draw_marker_partitions(void)
{
draw_marker();

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
GdkDrawingContext *gdc;
#endif
cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->wavearea)), &gdc);
wavewindow_paint(cr);

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->wavearea), gdc);
#else
cairo_destroy (cr);
#endif

draw_marker();
}

static void renderblackout(void)
{
gfloat pageinc;
TimeType lhs, rhs, lclip, rclip;
struct blackout_region_t *bt = GLOBALS->blackout_regions;

if(bt)
	{
	pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	lhs = GLOBALS->tims.start;
	rhs = pageinc + lhs;

	while(bt)
		{
		if((bt->bend < lhs) || (bt->bstart > rhs))
			{
			/* nothing, out of bounds */
			}
			else
			{
			lclip = bt->bstart; rclip = bt->bend;

			if(lclip < lhs) lclip = lhs;
				else if (lclip > rhs) lclip = rhs;

			if(rclip < lhs) rclip = lhs;

			lclip -= lhs;
			rclip -= lhs;
			if(rclip>((GLOBALS->wavewidth+1)*GLOBALS->nspx)) rclip = (GLOBALS->wavewidth+1)*(GLOBALS->nspx);

			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1, TRUE, (((gdouble)lclip)*GLOBALS->pxns), GLOBALS->fontheight,(((gdouble)(rclip-lclip))*GLOBALS->pxns), GLOBALS->waveheight-GLOBALS->fontheight);
			}

		bt=bt->next;
		}
	}
}

static void
service_hslider(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

DEBUG(printf("Wave HSlider Moved\n"));

if((GLOBALS->cr_wavepixmap_wavewindow_c_1)&&(GLOBALS->wavewidth>1))
	{
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
	TimeType old_start = GLOBALS->tims.start;
#endif
	GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);

	if(!GLOBALS->tims.timecache)
		{
		GLOBALS->tims.start=time_trunc(gtk_adjustment_get_value(hadj));
		}
		else
		{
		GLOBALS->tims.start=time_trunc(GLOBALS->tims.timecache);
		GLOBALS->tims.timecache=0;	/* reset */
		}

	if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
		else if(GLOBALS->tims.start>GLOBALS->tims.last) GLOBALS->tims.start=GLOBALS->tims.last;

	GLOBALS->tims.laststart=GLOBALS->tims.start;

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
        if((gesture_in_zoom)||(!GLOBALS->swipe_init_time)||(GLOBALS->wavearea_gesture_swipe_velocity_x == 0.0)||(old_start != GLOBALS->tims.start)) /* cut down on redundant draws when swiping */
#endif
                {
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_back_wavewindow_c_1, TRUE, 0, 0,GLOBALS->wavewidth, GLOBALS->waveheight);
                rendertimebar();
                }
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
	if(gesture_in_zoom) gesture_in_zoom--;
#endif
	}
}

#ifndef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
static
#endif
void
service_vslider(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

GtkAdjustment *sadj, *hadj;
int trtarget;
int xsrc;

if(GLOBALS->cr_signalpixmap)
	{
	hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
	sadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	xsrc=(gint)gtk_adjustment_get_value(hadj);

	trtarget=(int)(gtk_adjustment_get_value(sadj));
	DEBUG(printf("Wave VSlider Moved to %d\n",trtarget));

	GtkAllocation allocation;
	gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

	cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc.gc_ltgray.r, GLOBALS->rgb_gc.gc_ltgray.g, GLOBALS->rgb_gc.gc_ltgray.b, GLOBALS->rgb_gc.gc_ltgray.a);
	cairo_rectangle (GLOBALS->cr_signalpixmap, 0, 0,
                        GLOBALS->signal_fill_width, allocation.height);
	cairo_fill (GLOBALS->cr_signalpixmap); 

		sync_marker();
		RenderSigs(trtarget,(GLOBALS->old_wvalue==gtk_adjustment_get_value(sadj))?0:1);

	GLOBALS->old_wvalue=gtk_adjustment_get_value(sadj);

		draw_named_markers();

		if(GLOBALS->signalarea_has_focus)
			{
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
			GdkDrawingContext *gdc;
#endif
			cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
			cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
			cairo_clip (cr);

            signalwindow_paint(cr);
			draw_signalarea_focus(cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
			gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
			cairo_destroy (cr);
#endif
			}
			else
			{
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
			GdkDrawingContext *gdc;
#endif
			cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
			cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
			cairo_clip (cr);

            signalwindow_paint(cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
                        gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
                        cairo_destroy (cr);
#endif
			}

		draw_marker();
	}
}

void button_press_release_common(void)
{
MaxSignalLength();

GtkAllocation allocation;
gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc.gc_ltgray.r, GLOBALS->rgb_gc.gc_ltgray.g, GLOBALS->rgb_gc.gc_ltgray.b, GLOBALS->rgb_gc.gc_ltgray.a);
cairo_rectangle (GLOBALS->cr_signalpixmap, 0, 0, GLOBALS->signal_fill_width, allocation.height);
cairo_fill (GLOBALS->cr_signalpixmap); 

{
char signalwindow_width_dirty = GLOBALS->signalwindow_width_dirty;
sync_marker();
if(!signalwindow_width_dirty && GLOBALS->signalwindow_width_dirty)
	{
	MaxSignalLength_2(1);
	}
}

RenderSigs((int)(gtk_adjustment_get_value(GTK_ADJUSTMENT(GLOBALS->wave_vslider))),0);

if(GLOBALS->signalarea_has_focus)
	{
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
	GdkDrawingContext *gdc;
#endif
	cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_clip (cr);

    signalwindow_paint(cr);

	draw_signalarea_focus(cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
	gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
	cairo_destroy (cr);
#endif
	}
	else
	{
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
        GdkDrawingContext *gdc;
#endif
	cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_clip (cr);

    signalwindow_paint(cr);

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
        gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
        cairo_destroy (cr);
#endif
	}
}

static void button_motion_common(gint xin, gint yin, int pressrel, int is_button_2)
{
gdouble x,offset,pixstep;
TimeType newcurr;

if(xin<0) xin=0;
if(xin>(GLOBALS->wavewidth-1)) xin=(GLOBALS->wavewidth-1);

x=xin;	/* for pix time calc */

pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
newcurr=(TimeType)(offset=((gdouble)GLOBALS->tims.start)+(x*pixstep));

if(offset-newcurr>0.5)	/* round to nearest integer ns */
	{
	newcurr++;
	}

if(newcurr>GLOBALS->tims.last) 		/* sanity checking */
	{
	newcurr=GLOBALS->tims.last;
	}
if(newcurr>GLOBALS->tims.end)
	{
	newcurr=GLOBALS->tims.end;
	}
if(newcurr<GLOBALS->tims.start)
	{
	newcurr=GLOBALS->tims.start;
	}

newcurr = time_trunc(newcurr);
if(newcurr < 0) newcurr = GLOBALS->min_time; /* prevents marker from disappearing? */

if(!is_button_2)
	{
	update_markertime(GLOBALS->tims.marker=cook_markertime(newcurr, xin, yin));
	if(GLOBALS->tims.lmbcache<0) GLOBALS->tims.lmbcache=time_trunc(newcurr);

	draw_marker_partitions();

	if((pressrel)||(GLOBALS->constant_marker_update))
		{
		button_press_release_common();
		}
	}
	else
	{
	GLOBALS->tims.baseline = ((GLOBALS->tims.baseline<0)||(is_button_2<0)) ? cook_markertime(newcurr, xin, yin) : -1;
	update_basetime(GLOBALS->tims.baseline);
	update_markertime(GLOBALS->tims.marker);
	wavearea_configure_event(GLOBALS->wavearea, NULL);
	}
}

static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
(void)widget;

gdouble x, y, pixstep, offset;
GdkModifierType state;
TimeType newcurr;
int scrolled;

gint xi, yi;

int dummy_x, dummy_y;
get_window_xypos(&dummy_x, &dummy_y);

if(event->is_hint)
        {
	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;
        }
        else
        {
        x = event->x;
        y = event->y;
        state = event->state;
        }

do
	{
	scrolled=0;
	if(state&bmask[GLOBALS->in_button_press_wavewindow_c_1]) /* needed for retargeting in AIX/X11 */
		{
		if(x<0)
			{
			if(GLOBALS->wave_scrolling)
			if(GLOBALS->tims.start>GLOBALS->tims.first)
				{
				if(GLOBALS->nsperframe<10)
					{
					GLOBALS->tims.start-=GLOBALS->nsperframe;
					}
					else
					{
					GLOBALS->tims.start-=(GLOBALS->nsperframe/10);
					}
				if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
				gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),GLOBALS->tims.marker=time_trunc(GLOBALS->tims.timecache=GLOBALS->tims.start));

				g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
				g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");
				scrolled=1;
				}
			x=0;
			}
		else
		if(x>GLOBALS->wavewidth)
			{
			if(GLOBALS->wave_scrolling)
			if(GLOBALS->tims.start!=GLOBALS->tims.last)
				{
				gfloat pageinc;

				pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

				if(GLOBALS->nsperframe<10)
					{
					GLOBALS->tims.start+=GLOBALS->nsperframe;
					}
					else
					{
					GLOBALS->tims.start+=(GLOBALS->nsperframe/10);
					}

				if(GLOBALS->tims.start>GLOBALS->tims.last-pageinc+1) GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-pageinc+1);
				if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;

				GLOBALS->tims.marker=time_trunc(GLOBALS->tims.start+pageinc);
				if(GLOBALS->tims.marker>GLOBALS->tims.last) GLOBALS->tims.marker=GLOBALS->tims.last;

				gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),GLOBALS->tims.timecache=GLOBALS->tims.start);

				g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
				g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");
				scrolled=1;
				}
			x=GLOBALS->wavewidth-1;
			}
		}
	else if((state&GDK_BUTTON2_MASK)&&(GLOBALS->tims.baseline>=0))
		{
		button_motion_common(x,y,0,-1); /* neg one says don't clear tims.baseline */
		}

	pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
	newcurr=GLOBALS->tims.start+(offset=x*pixstep);
	if((offset-((int)offset))>0.5)  /* round to nearest integer ns */
	        {
	        newcurr++;
	        }

	if(newcurr>GLOBALS->tims.last) newcurr=GLOBALS->tims.last;

	if(newcurr!=GLOBALS->prevtim_wavewindow_c_1)
		{
		update_currenttime(time_trunc(newcurr));
		GLOBALS->prevtim_wavewindow_c_1=newcurr;
		}

	if(state&bmask[GLOBALS->in_button_press_wavewindow_c_1])
		{
		button_motion_common(x,y,0,0);
		}

	/* warp selected signals if CTRL is pressed */
#ifdef MAC_INTEGRATION
        if((event->state & GDK_MOD2_MASK)&&(state&GDK_BUTTON1_MASK))
#else
        if((event->state & GDK_CONTROL_MASK)&&(state&GDK_BUTTON1_MASK))
#endif
		{
	  	int warp = 0;
          	Trptr t = (GLOBALS->use_gestures > 0) ? NULL : GLOBALS->traces.first;
		TimeType gt, delta;

          	while ( t )
          		{
            		if ( t->flags & TR_HIGHLIGHT )
            			{
	      			warp++;

				if(!t->shift_drag_valid)
					{
					t->shift_drag = t->shift;
					t->shift_drag_valid = 1;
					}

              			gt = t->shift_drag + (GLOBALS->tims.marker - GLOBALS->tims.lmbcache);

		        	if(gt<0)
        		        	{
		                	delta=GLOBALS->tims.first-GLOBALS->tims.last;
		                	if(gt<delta) gt=delta;
		                	}
		       		else
				if(gt>0)
		                	{
		                	delta=GLOBALS->tims.last-GLOBALS->tims.first;
		                	if(gt>delta) gt=delta;
		                	}
				t->shift = gt;
            			}

            		t = t->t_next;
          		}

	  	if( warp )
	  		{
/* commented out to reduce on visual noise...

            		GLOBALS->signalwindow_width_dirty = 1;
            		MaxSignalLength(  );

...commented out to reduce on visual noise */

            		signalarea_configure_event( GLOBALS->signalarea, NULL );
            		wavearea_configure_event( GLOBALS->wavearea, NULL );
          		}
		}

	if(scrolled)	/* make sure counters up top update.. */
		{
		gtk_events_pending_gtk_main_iteration();
		}

	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	} while((scrolled)&&(state&bmask[GLOBALS->in_button_press_wavewindow_c_1]));

return(TRUE);
}

static void alternate_y_scroll(int delta)
{
GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
int value = (int)gtk_adjustment_get_value(wadj);
int target = value + delta;

GtkAllocation allocation;
gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

int num_traces_displayable=(allocation.height)/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

if(target > GLOBALS->traces.visible - num_traces_displayable) target = GLOBALS->traces.visible - num_traces_displayable;

if(target < 0) target = 0;

gtk_adjustment_set_value(wadj, target);

g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed"); /* force bar update */
g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
}


/*
 * Sane code starts here... :)
 * TomB 05Feb2012
 */

#define SANE_INCREMENT 0.25
/* don't want to increment a whole page thereby completely losing where I am... */

void
alt_move_left(gboolean fine_scroll)
{
  TimeType ntinc, ntfrac;

  ntinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);	/* really don't need this var but the speed of ui code is human dependent.. */
  ntfrac=ntinc*GLOBALS->page_divisor*(SANE_INCREMENT / (fine_scroll ? 8.0 : 1.0));
  if(!ntfrac) ntfrac = 1;

  if((GLOBALS->tims.start-ntfrac)>GLOBALS->tims.first)
    GLOBALS->tims.timecache=GLOBALS->tims.start-ntfrac;
  else
    GLOBALS->tims.timecache=GLOBALS->tims.first;

  gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),GLOBALS->tims.timecache);
  time_update();

  DEBUG(printf("Alternate move left\n"));
}

void
alt_move_right(gboolean fine_scroll)
{
  TimeType ntinc, ntfrac;

  ntinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
  ntfrac=ntinc*GLOBALS->page_divisor*(SANE_INCREMENT / (fine_scroll ? 8.0 : 1.0));
  if(!ntfrac) ntfrac = 1;

  if((GLOBALS->tims.start+ntfrac)<(GLOBALS->tims.last-ntinc+1))
  {
    GLOBALS->tims.timecache=GLOBALS->tims.start+ntfrac;
  }
  else
  {
    GLOBALS->tims.timecache=GLOBALS->tims.last-ntinc+1;
    if(GLOBALS->tims.timecache<GLOBALS->tims.first)
      GLOBALS->tims.timecache=GLOBALS->tims.first;
  }

  gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),GLOBALS->tims.timecache);
  time_update();

  DEBUG(printf("Alternate move right\n"));
}


void alt_zoom_out(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

  TimeType middle=0, width;
  TimeType marker = GLOBALS->cached_currenttimeval_currenttime_c_1;
  /* Zoom on mouse cursor, not marker */

  if(GLOBALS->do_zoom_center)
  {
    if((marker<0)||(marker<GLOBALS->tims.first)||(marker>GLOBALS->tims.last))
    {
      if(GLOBALS->tims.end>GLOBALS->tims.last)
        GLOBALS->tims.end=GLOBALS->tims.last;

      middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
      if((GLOBALS->tims.start&1)&&(GLOBALS->tims.end&1))
        middle++;
    }
    else
    {
      middle=marker;
    }
  }

  GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;

#ifdef WAVE_CTRL_SCROLL_ZOOM_FACTOR
  GLOBALS->tims.zoom-=WAVE_CTRL_SCROLL_ZOOM_FACTOR;
#else
  GLOBALS->tims.zoom--;
#endif
  calczoom(GLOBALS->tims.zoom);

  if(GLOBALS->do_zoom_center)
  {
    width=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
    GLOBALS->tims.start=time_trunc(middle-(width/2));
    if(GLOBALS->tims.start+width>GLOBALS->tims.last)
      GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-width);

    if(GLOBALS->tims.start<GLOBALS->tims.first)
      GLOBALS->tims.start=GLOBALS->tims.first;

    gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),GLOBALS->tims.timecache=GLOBALS->tims.start);
  }
  else
  {
    GLOBALS->tims.timecache=0;
  }

  fix_wavehadj();

  g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
  g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

  DEBUG(printf("Alternate Zoom out\n"));
}

void alt_zoom_in(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

  if(GLOBALS->tims.zoom<0)		/* otherwise it's ridiculous and can cause */
  {		/* overflow problems in the scope          */
    TimeType middle=0, width;
    TimeType marker = GLOBALS->cached_currenttimeval_currenttime_c_1;
    /* Zoom on mouse cursor, not marker */

    if(GLOBALS->do_zoom_center)
    {
      if((marker<0)||(marker<GLOBALS->tims.first)||(marker>GLOBALS->tims.last))
      {
        if(GLOBALS->tims.end>GLOBALS->tims.last)
          GLOBALS->tims.end=GLOBALS->tims.last;

        middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
        if((GLOBALS->tims.start&1)&&(GLOBALS->tims.end&1))
          middle++;
      }
      else
      {
        middle=marker;
      }
    }

    GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;

#ifdef WAVE_CTRL_SCROLL_ZOOM_FACTOR
  GLOBALS->tims.zoom+=WAVE_CTRL_SCROLL_ZOOM_FACTOR;
#else
    GLOBALS->tims.zoom++;
#endif
    calczoom(GLOBALS->tims.zoom);

    if(GLOBALS->do_zoom_center)
    {
      width=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
      GLOBALS->tims.start=time_trunc(middle-(width/2));
      if(GLOBALS->tims.start+width>GLOBALS->tims.last)
        GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-width);

      if(GLOBALS->tims.start<GLOBALS->tims.first)
        GLOBALS->tims.start=GLOBALS->tims.first;

      gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),GLOBALS->tims.timecache=GLOBALS->tims.start);
    }
    else
    {
    GLOBALS->tims.timecache=0;
    }

    fix_wavehadj();

    g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
    g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

    DEBUG(printf("Alternate zoom in\n"));
  }
}




static        gint
scroll_event( GtkWidget * widget, GdkEventScroll * event )
{
(void)widget;

GtkAllocation allocation;
gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

int num_traces_displayable=(allocation.height)/(GLOBALS->fontheight);
num_traces_displayable--;

  DEBUG(printf("Mouse Scroll Event\n"));

  if(GLOBALS->alt_wheel_mode)
  {
    /* TomB mouse wheel handling */
#ifdef MAC_INTEGRATION
      if ( event->state & GDK_MOD2_MASK )
#else
      if ( event->state & GDK_CONTROL_MASK )
#endif
      {
        /* CTRL+wheel - zoom in/out around current mouse cursor position */
        if(event->direction == GDK_SCROLL_UP)
          alt_zoom_in(NULL, 0);
        else if(event->direction == GDK_SCROLL_DOWN)
          alt_zoom_out(NULL, 0);
      }
      else if( event->state & GDK_MOD1_MASK )
      {
        /* ALT+wheel - edge left/right mode */
        if(event->direction == GDK_SCROLL_UP)
          service_left_edge(NULL, 0);
        else if(event->direction == GDK_SCROLL_DOWN)
          service_right_edge(NULL, 0);
      }
      else
      {
        /* wheel alone - scroll part of a page along */
        if(event->direction == GDK_SCROLL_UP)
          alt_move_left((event->state & GDK_SHIFT_MASK) != 0);  /* finer scroll if shift */
        else if(event->direction == GDK_SCROLL_DOWN)
          alt_move_right((event->state & GDK_SHIFT_MASK) != 0); /* finer scroll if shift */
      }
  }
  else
  {
    /* Original 3.3.31 mouse wheel handling */
  switch ( event->direction )
  {
    case GDK_SCROLL_UP:
      if (GLOBALS->use_scrollwheel_as_y)
	{
	if(event->state & GDK_SHIFT_MASK)
		{
		alternate_y_scroll(-num_traces_displayable);
		}
		else
		{
		alternate_y_scroll(-1);
		}
	}
	else
	{
#ifdef MAC_INTEGRATION
      	if ( event->state & GDK_MOD2_MASK )
#else
      	if ( event->state & GDK_CONTROL_MASK )
#endif
        	service_left_shift(NULL, 0);
      	else if ( event->state & GDK_MOD1_MASK )
		service_zoom_out(NULL, 0);
      	else
        	service_left_page(NULL, 0);
	}
      break;
    case GDK_SCROLL_DOWN:
      if (GLOBALS->use_scrollwheel_as_y)
	{
	if(event->state & GDK_SHIFT_MASK)
		{
		alternate_y_scroll(num_traces_displayable);
		}
		else
		{
		alternate_y_scroll(1);
		}
	}

	{
#ifdef MAC_INTEGRATION
      	if ( event->state & GDK_MOD2_MASK )
#else
      	if ( event->state & GDK_CONTROL_MASK )
#endif
        	service_right_shift(NULL, 0);
      	else if ( event->state & GDK_MOD1_MASK )
		service_zoom_in(NULL, 0);
      	else
        	service_right_page(NULL, 0);
	}
      break;

    default:
      break;
  }
  }
  return(TRUE);
}


static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
if((event->button==1)||((event->button==3)&&(!GLOBALS->in_button_press_wavewindow_c_1)))
	{
	GLOBALS->in_button_press_wavewindow_c_1=event->button;

	DEBUG(printf("Button Press Event\n"));
	GLOBALS->prev_markertime = GLOBALS->tims.marker;
	button_motion_common(event->x,event->y,1,0);
	GLOBALS->tims.timecache=GLOBALS->tims.start;

#ifdef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
	GdkDisplay *display = gdk_display_get_default();
	GdkSeat *seat = gdk_display_get_default_seat (display);
	gdk_seat_grab (seat,
               gtk_widget_get_window(widget),
               GDK_SEAT_CAPABILITY_ALL_POINTING,
               FALSE,
               NULL,
               (GLOBALS->use_gestures > 0) ? NULL : (GdkEvent *)event, /* GdkEvent is a union with type as 1st element */
               NULL,
               NULL);
#else
	gdk_pointer_grab(gtk_widget_get_window(widget), FALSE,
		m_bmask[GLOBALS->in_button_press_wavewindow_c_1] | /* key up on motion for button pressed ONLY */
		GDK_POINTER_MOTION_HINT_MASK |
	      	GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);
#endif

#ifdef MAC_INTEGRATION
	if ((event->state & GDK_MOD2_MASK) && (event->button==1))
#else
	if ((event->state & GDK_CONTROL_MASK) && (event->button==1))
#endif
		{
		Trptr t = (GLOBALS->use_gestures > 0) ? NULL : GLOBALS->traces.first;

		while(t)
			{
			if((t->flags & TR_HIGHLIGHT)&&(!t->shift_drag_valid))
				{
				t->shift_drag = t->shift; /* cache old value */
				t->shift_drag_valid = 1;
				}
			t=t->t_next;
			}
		}
	}
else
if(event->button==2)
	{
	if(!GLOBALS->button2_debounce_flag)
		{
		GLOBALS->button2_debounce_flag = 1; /* cleared by mouseover_timer() interrupt */
		button_motion_common(event->x,event->y,1,1);
		}
	}

return(TRUE);
}

static gint button_release_event(GtkWidget *widget, GdkEventButton *event)
{
(void)widget;

if((event->button)&&(event->button==GLOBALS->in_button_press_wavewindow_c_1))
	{
	GLOBALS->in_button_press_wavewindow_c_1=0;

	DEBUG(printf("Button Release Event\n"));
	button_motion_common(event->x,event->y,1,0);

	/* warp selected signals if CTRL is pressed */
        if(event->button==1)
		{
	  	int warp = 0;
          	Trptr t = (GLOBALS->use_gestures > 0) ? NULL : GLOBALS->traces.first;
#ifdef MAC_INTEGRATION
		if(event->state & GDK_MOD2_MASK)
#else
		if(event->state & GDK_CONTROL_MASK)
#endif
			{
			TimeType gt, delta;

	          	while ( t )
	          		{
	            		if ( t->flags & TR_HIGHLIGHT )
	            			{
		      			warp++;
	              			gt = (t->shift_drag_valid ? t-> shift_drag : t->shift) + (GLOBALS->tims.marker - GLOBALS->tims.lmbcache);

			        	if(gt<0)
	        		        	{
			                	delta=GLOBALS->tims.first-GLOBALS->tims.last;
			                	if(gt<delta) gt=delta;
			                	}
			       		else
					if(gt>0)
			                	{
			                	delta=GLOBALS->tims.last-GLOBALS->tims.first;
			                	if(gt>delta) gt=delta;
			                	}
					t->shift = gt;

	              			t->flags &= ( ~TR_HIGHLIGHT );
	            			}

				t->shift_drag_valid = 0;
	            		t = t->t_next;
	          		}
			}
			else	/* back out warp and keep highlighting */
			{
			while(t)
				{
				if(t->shift_drag_valid)
					{
					t->shift = t->shift_drag;
					t->shift_drag_valid = 0;
					warp++;
					}
				t=t->t_next;
				}
			}

	  	if( warp )
	  		{
            		GLOBALS->signalwindow_width_dirty = 1;
            		MaxSignalLength(  );
            		signalarea_configure_event( GLOBALS->signalarea, NULL );
            		wavearea_configure_event( GLOBALS->wavearea, NULL );
          		}
		}

	GLOBALS->tims.timecache=GLOBALS->tims.start;

	XXX_gdk_pointer_ungrab(event->time);

	if(event->button==3)	/* oh yeah, dragzoooooooom! */
		{
		service_dragzoom(GLOBALS->tims.lmbcache, GLOBALS->tims.marker);
		}

	GLOBALS->tims.lmbcache=-1;
	update_markertime(GLOBALS->tims.marker);
	}

GLOBALS->mouseover_counter = 11;
move_mouseover(NULL, 0, 0, LLDescriptor(0));
GLOBALS->tims.timecache=0;

if(GLOBALS->prev_markertime == LLDescriptor(-1))
	{
	signalarea_configure_event( GLOBALS->signalarea, NULL );
	}

return(TRUE);
}


#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT

static void wavearea_zero_out_swipe_velocity(void)
{
GLOBALS->wavearea_gesture_swipe_velocity_x = 0.0;
if(GLOBALS->swipe_init_time)
	{
	g_date_time_unref(GLOBALS->swipe_init_time);
	GLOBALS->swipe_init_time = NULL;
	}
}


void wavearea_pressed_event(GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
(void) gesture;
(void) n_press;
(void) user_data;
GdkEventButton ev;

if(gesture_filter_cnt)
	{
	return;
	}

wavearea_zero_out_swipe_velocity(); 

memset(&ev, 0, sizeof(GdkEventButton));
ev.button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
ev.x = x;
ev.y = y;

if((n_press != 2) || (ev.button != 1))
	{
	button_press_event(GLOBALS->wavearea, &ev);
	}
	else
	{
	delete_unnamed_marker(NULL, 0, NULL); /* double click gesture deletes primary marker */
	}
}


void
wavearea_released_event(GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
(void) gesture;
(void) n_press;
(void) user_data;
GdkEventButton ev;

memset(&ev, 0, sizeof(GdkEventButton));
ev.button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
ev.x = x;
ev.y = y;

button_release_event(GLOBALS->wavearea, &ev);
}


void wavearea_long_pressed_event(GtkGestureMultiPress *gesture,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
(void) gesture;
(void) user_data;
GdkEventButton ev;

wavearea_zero_out_swipe_velocity();

memset(&ev, 0, sizeof(GdkEventButton));
ev.button = 2;
ev.x = x;
ev.y = y;

button_press_event(GLOBALS->wavearea, &ev);
}


void
wavearea_drag_begin_event (GtkGestureDrag *gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
(void) gesture;
(void) user_data;

GLOBALS->wavearea_drag_start_x = start_x;
GLOBALS->wavearea_drag_start_y = start_y;

if(gesture_filter_cnt)
	{
	GLOBALS->wavearea_drag_start_x = GLOBALS->wavearea_drag_start_y = -1.0;
	return;
	}
}


void
wavearea_drag_update_event (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
(void) gesture;
(void) user_data;
GdkEventMotion ev;

if(GLOBALS->wavearea_drag_start_x < 0.0) return;

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
	{
	if(gesture_filter_set) return; /* to prevent floods of drag update events in wayland */
	gesture_filter_set = 1;
	}
#endif

memset(&ev, 0, sizeof(GdkEventMotion));
ev.is_hint = 0;
#ifdef WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON
ev.state = (gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) == 3) ? GDK_BUTTON3_MASK : GDK_BUTTON1_MASK;
#else
ev.state = GDK_BUTTON1_MASK;
#endif
ev.x = GLOBALS->wavearea_drag_start_x + offset_x;
ev.y = GLOBALS->wavearea_drag_start_y + offset_y;
ev.window = gtk_widget_get_window(GLOBALS->wavearea);

GLOBALS->wavearea_drag_active = 1;

motion_notify_event(GLOBALS->wavearea, &ev);
}


void
wavearea_drag_end_event (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
wavearea_drag_update_event(gesture, offset_x, offset_y, user_data);
GLOBALS->wavearea_drag_active = 0;
}
#endif


void make_sigarea_gcs(GtkWidget *signalarea)
{
(void) signalarea;

if(!GLOBALS->made_sgc_contexts_wavewindow_c_1)
	{
	gboolean dark = GLOBALS->use_dark;

#if GTK_CHECK_VERSION(3,0,0)
	if(!dark)
		{
		g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &dark, NULL);
		GLOBALS->use_dark = dark;
		}
#endif

	GLOBALS->rgb_gc_white = dark ? XXX_alloc_color(GLOBALS->color_black) : XXX_alloc_color(GLOBALS->color_white);
	GLOBALS->rgb_gc_black = dark ? XXX_alloc_color(GLOBALS->color_white) : XXX_alloc_color(GLOBALS->color_black);
	GLOBALS->rgb_gc.gc_ltgray= dark ? XXX_alloc_color(GLOBALS->color_black) : XXX_alloc_color(GLOBALS->color_ltgray);
	GLOBALS->rgb_gc.gc_normal= XXX_alloc_color(GLOBALS->color_normal);
	GLOBALS->rgb_gc.gc_mdgray= dark ? XXX_alloc_color(GLOBALS->color_dkgray) : XXX_alloc_color(GLOBALS->color_mdgray);
	GLOBALS->rgb_gc.gc_dkgray= dark ? XXX_alloc_color(GLOBALS->color_white) : XXX_alloc_color(GLOBALS->color_dkgray);
	GLOBALS->rgb_gc.gc_dkblue= XXX_alloc_color(GLOBALS->color_dkblue);
	GLOBALS->rgb_gc.gc_brkred= XXX_alloc_color(GLOBALS->color_brkred);
	GLOBALS->rgb_gc.gc_ltblue= XXX_alloc_color(GLOBALS->color_ltblue);
	GLOBALS->rgb_gc.gc_gmstrd= XXX_alloc_color(GLOBALS->color_gmstrd);

	GLOBALS->made_sgc_contexts_wavewindow_c_1=~0;
	}
}


static const int wave_rgb_rainbow[WAVE_NUM_RAINBOW] = WAVE_RAINBOW_RGB;

gint wavearea_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
(void)event;

if((!widget)||(!gtk_widget_get_window(widget))) return(TRUE);

GtkAllocation allocation;
gtk_widget_get_allocation(widget, &allocation);

int scale_factor = XXX_gtk_widget_get_scale_factor(widget);

DEBUG(printf("WaveWin Configure Event h: %d, w: %d\n",allocation.height,
		allocation.width));

if(GLOBALS->cr_wavepixmap_wavewindow_c_1)
	{
	if((GLOBALS->wavewidth!=allocation.width)||(GLOBALS->waveheight!=allocation.height))
		{
		GLOBALS->wavewidth=allocation.width;
		GLOBALS->waveheight=allocation.height;

                cairo_destroy (GLOBALS->cr_wavepixmap_wavewindow_c_1);
                cairo_surface_destroy (GLOBALS->surface_wavepixmap_wavewindow_c_1);

                GLOBALS->surface_wavepixmap_wavewindow_c_1 = cairo_image_surface_create (CAIRO_FORMAT_RGB24, GLOBALS->wavewidth*scale_factor, allocation.height*scale_factor);
                GLOBALS->cr_wavepixmap_wavewindow_c_1 = cairo_create (GLOBALS->surface_wavepixmap_wavewindow_c_1);
                cairo_scale(GLOBALS->cr_wavepixmap_wavewindow_c_1, scale_factor, scale_factor);
                cairo_set_line_width(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->cr_line_width);
		cairo_set_line_cap(GLOBALS->cr_wavepixmap_wavewindow_c_1, CAIRO_LINE_CAP_SQUARE);
		}
	GLOBALS->old_wvalue=-1.0;
	}
	else
	{
	GLOBALS->wavewidth=allocation.width;
	GLOBALS->waveheight=allocation.height;

        GLOBALS->surface_wavepixmap_wavewindow_c_1 = cairo_image_surface_create (CAIRO_FORMAT_RGB24, GLOBALS->wavewidth*scale_factor, allocation.height*scale_factor);
        GLOBALS->cr_wavepixmap_wavewindow_c_1 = cairo_create (GLOBALS->surface_wavepixmap_wavewindow_c_1);
        cairo_scale(GLOBALS->cr_wavepixmap_wavewindow_c_1, scale_factor, scale_factor);
        cairo_set_line_width(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->cr_line_width);
	cairo_set_line_cap(GLOBALS->cr_wavepixmap_wavewindow_c_1, CAIRO_LINE_CAP_SQUARE);
	}

if(!GLOBALS->made_gc_contexts_wavewindow_c_1)
	{
	int i;

	GLOBALS->rgb_gc.gc_back_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_back);
	GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_baseline);
	GLOBALS->rgb_gc.gc_grid_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_grid);
	GLOBALS->rgb_gc.gc_grid2_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_grid2);
	GLOBALS->rgb_gc.gc_time_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_time);
	GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_timeb);
	GLOBALS->rgb_gc.gc_value_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_value);
	GLOBALS->rgb_gc.gc_low_wavewindow_c_1    = XXX_alloc_color(GLOBALS->color_low);
	GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1=XXX_alloc_color(GLOBALS->color_highfill);
	GLOBALS->rgb_gc.gc_high_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_high);
	GLOBALS->rgb_gc.gc_trans_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_trans);
	GLOBALS->rgb_gc.gc_mid_wavewindow_c_1    = XXX_alloc_color(GLOBALS->color_mid);
	GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_xfill);
	GLOBALS->rgb_gc.gc_x_wavewindow_c_1      = XXX_alloc_color(GLOBALS->color_x);
	GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_vbox);
	GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_vtrans);
	GLOBALS->rgb_gc.gc_mark_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_mark);
	GLOBALS->rgb_gc.gc_umark_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_umark);

        GLOBALS->rgb_gc.gc_0_wavewindow_c_1      = XXX_alloc_color(GLOBALS->color_0);
        GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_1fill);
        GLOBALS->rgb_gc.gc_1_wavewindow_c_1      = XXX_alloc_color(GLOBALS->color_1);
        GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_ufill);
        GLOBALS->rgb_gc.gc_u_wavewindow_c_1      = XXX_alloc_color(GLOBALS->color_u);
        GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1  = XXX_alloc_color(GLOBALS->color_wfill);
        GLOBALS->rgb_gc.gc_w_wavewindow_c_1      = XXX_alloc_color(GLOBALS->color_w);
        GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1= XXX_alloc_color(GLOBALS->color_dashfill);
        GLOBALS->rgb_gc.gc_dash_wavewindow_c_1   = XXX_alloc_color(GLOBALS->color_dash);

	GLOBALS->made_gc_contexts_wavewindow_c_1=~0;

	memcpy(&GLOBALS->rgb_gccache, &GLOBALS->rgb_gc, sizeof(struct wave_rgbmaster_t));

	/* add rainbow colors */
	for(i=0;i<WAVE_NUM_RAINBOW;i++)
		{
		int col = wave_rgb_rainbow[i];

		GLOBALS->rgb_gc_rainbow[i*2] = XXX_alloc_color(col);
		col >>= 1;
		col &= 0x007F7F7F;
		GLOBALS->rgb_gc_rainbow[i*2+1] = XXX_alloc_color(col);
		}
	}

if(GLOBALS->timestart_from_savefile_valid)
	{
	gfloat pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

	if((GLOBALS->timestart_from_savefile >= GLOBALS->tims.first) && (GLOBALS->timestart_from_savefile <= (GLOBALS->tims.last-pageinc)))
		{
		GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
		gtk_adjustment_set_value(hadj, (gdouble)(GLOBALS->timestart_from_savefile));
		fix_wavehadj();
		g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
		g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */

		}
	GLOBALS->timestart_from_savefile_valid--; /* for previous gtk toolkit versions could set this directly to zero */
	}

if(GLOBALS->wavewidth>1)
	{
	if((!GLOBALS->do_initial_zoom_fit)||(GLOBALS->do_initial_zoom_fit_used))
		{
		calczoom(GLOBALS->tims.zoom);
		fix_wavehadj();
		g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
		g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
		}
		else
		{
		GLOBALS->do_initial_zoom_fit_used=1;
		service_zoom_fit(NULL,NULL);
		}
	}

/* tims.timecache=tims.laststart; */
return(TRUE);
}

/*
 * screengrab vs normal rendering gcs...
 */
void force_screengrab_gcs(void)
{
GLOBALS->black_and_white = 1;

GLOBALS->rgb_gc.gc_ltgray= GLOBALS->rgb_gc_white ;
GLOBALS->rgb_gc.gc_normal= GLOBALS->rgb_gc_white ;
GLOBALS->rgb_gc.gc_mdgray= GLOBALS->rgb_gc_white ;
GLOBALS->rgb_gc.gc_dkgray= GLOBALS->rgb_gc_white ;
GLOBALS->rgb_gc.gc_dkblue= GLOBALS->rgb_gc_black ;
GLOBALS->rgb_gc.gc_brkred= GLOBALS->rgb_gc_black ;
GLOBALS->rgb_gc.gc_ltblue= GLOBALS->rgb_gc_black ;
GLOBALS->rgb_gc.gc_gmstrd= GLOBALS->rgb_gc_black ;
GLOBALS->rgb_gc.gc_back_wavewindow_c_1   = GLOBALS->rgb_gc_white ;
GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1 = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_grid_wavewindow_c_1   = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_grid2_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_time_wavewindow_c_1   = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1  = GLOBALS->rgb_gc_white;
GLOBALS->rgb_gc.gc_value_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_low_wavewindow_c_1    = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1= GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_high_wavewindow_c_1   = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_trans_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_mid_wavewindow_c_1    = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_x_wavewindow_c_1      = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1   = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1 = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_mark_wavewindow_c_1   = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_umark_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_0_wavewindow_c_1      = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_1_wavewindow_c_1      = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_u_wavewindow_c_1      = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1  = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_w_wavewindow_c_1      = GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1= GLOBALS->rgb_gc_black;
GLOBALS->rgb_gc.gc_dash_wavewindow_c_1   = GLOBALS->rgb_gc_black;
}

void force_normal_gcs(void)
{
GLOBALS->black_and_white = 0;

memcpy(&GLOBALS->rgb_gc, &GLOBALS->rgb_gccache, sizeof(struct wave_rgbmaster_t));
}

static gint wavearea_configure_event_local(GtkWidget *widget, GdkEventConfigure *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
struct Global *g_old = GLOBALS;

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = wavearea_configure_event(widget, event);

set_GLOBALS(g_old);

return(rc);
}


#if GTK_CHECK_VERSION(3,0,0)
static gint draw_event(GtkWidget *widget, cairo_t *cr, gpointer      user_data)
{
(void) widget;
(void) user_data;

gint rc = FALSE;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
/* struct Global *g_old = GLOBALS; */

set_GLOBALS((*GLOBALS->contexts)[page_num]);

wavewindow_paint(cr);

draw_marker();

/* seems to cause a conflict flipping back so don't! */
/* set_GLOBALS(g_old); */

if(gesture_filter_set) gesture_filter_set = 0;

return(rc);
}
#else
static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
GdkDrawingContext *gdc;
#endif
cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(widget)), &gdc);
gdk_cairo_region (cr, event->region);
cairo_clip (cr);

wavewindow_paint(cr);

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
gdk_window_end_draw_frame(gtk_widget_get_window(widget), gdc);
#else
cairo_destroy (cr);
#endif

draw_marker();

return(FALSE);
}

static gint expose_event_local(GtkWidget *widget, GdkEventExpose *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
/* struct Global *g_old = GLOBALS; */

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = expose_event(widget, event);

/* seems to cause a conflict flipping back so don't! */
/* set_GLOBALS(g_old); */

return(rc);
}
#endif


#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
static gboolean wave_vslider_gtc(GtkWidget *widget,
                    GdkFrameClock *frame_clock,
                    gpointer user_data)
{
if(GLOBALS && GLOBALS->wave_vslider_valid && GLOBALS->wave_vslider && GLOBALS->signal_hslider)
	{
	GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	GtkAdjustment *hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);

	gtk_adjustment_set_page_size(wadj, GLOBALS->wave_vslider_page_size);
	gtk_adjustment_set_page_increment(wadj, GLOBALS->wave_vslider_page_increment);
	gtk_adjustment_set_step_increment(wadj, GLOBALS->wave_vslider_step_increment);
	gtk_adjustment_set_lower(wadj, GLOBALS->wave_vslider_lower);
	gtk_adjustment_set_upper(wadj, GLOBALS->wave_vslider_upper);
	gtk_adjustment_set_value(wadj, GLOBALS->wave_vslider_value);

	GLOBALS->wave_vslider_valid = 0;

	g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed");	/* force bar update */
	g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
	g_signal_emit_by_name (XXX_GTK_OBJECT (hadj), "changed");	/* force bar update */
	}

return(G_SOURCE_CONTINUE);
}
#endif


#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
static int
wavearea_zoom_get_gesture_xy_points(GtkGesture *gesture, gdouble *x1p, gdouble *y1p, gdouble *x2p, gdouble *y2p)
{
GList *sequences;
int rc = 0;

if(gesture)
	{
	sequences = gtk_gesture_get_sequences (gesture);
	if(sequences)
		{
		if(sequences->next)
			{
			rc = 1;
			gdouble x1, y1, x2, y2;

		      	gtk_gesture_get_point (gesture, sequences->data, &x1, &y1);
		      	gtk_gesture_get_point (gesture, sequences->next->data, &x2, &y2);

			if(x1 < x2) /* order coordinates so x1 is leftmost point */
				{
				*x1p = x1; *y1p = y1; *x2p = x2; *y2p = y2;
				}
				else
				{
				*x1p = x2; *y1p = y2; *x2p = x1; *y2p = y1;
				}
			}
		
		g_list_free (sequences);
		}
	}

return(rc);
}
#endif

static void
wavearea_zoom_begin_event (GtkGesture *gesture,
               GdkEventSequence *sequence,
               gpointer          user_data)
{
(void) sequence;
(void) user_data;
gdouble x1, y1, x2, y2;

gesture_in_zoom = 1;
GLOBALS->wavearea_gesture_initial_zoom = GLOBALS->tims.zoom;

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
if(wavearea_zoom_get_gesture_xy_points(gesture, &x1, &y1, &x2, &y2))
	{
	GLOBALS->wavearea_gesture_initial_x1tim = GLOBALS->tims.start + (x1 * GLOBALS->nspx);

	GLOBALS->wavearea_gesture_initial_zoom_x_distance = x2 - x1;
	if(GLOBALS->wavearea_gesture_initial_zoom_x_distance < 1.0) GLOBALS->wavearea_gesture_initial_zoom_x_distance = 1.0; /* min resolution is one pixel */
	}
	else
	{
        TimeType middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
	GLOBALS->wavearea_gesture_initial_x1tim = middle;

	GLOBALS->wavearea_gesture_initial_zoom_x_distance = 1.0; /* min resolution is one pixel */
	}

if(GLOBALS->wavearea_gesture_initial_x1tim < GLOBALS->tims.first) GLOBALS->wavearea_gesture_initial_x1tim = GLOBALS->tims.first;
if(GLOBALS->wavearea_gesture_initial_x1tim > GLOBALS->tims.last) GLOBALS->wavearea_gesture_initial_x1tim = GLOBALS->tims.last;

#endif

#ifdef WAVE_GTK3_GESTURE_ZOOM_USES_GTK_PHASE_CAPTURE
gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
#endif
}


static void
wavearea_zoom_scale_changed_event (GtkGestureZoom *controller,
               gdouble         scale,
               gpointer        user_data)
{
(void) controller;
gdouble zb, ls, lzb, r, z0;
#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
gdouble x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

if(gesture_filter_set) return; /* to prevent floods of zoom events */
gesture_filter_set = 1;
#endif

gesture_in_zoom = 1;

zb = GLOBALS->zoombase;
lzb = log(zb);

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
{
GtkGesture *gesture = (GtkGesture *)user_data;
gdouble dist;
if(wavearea_zoom_get_gesture_xy_points(gesture, &x1, &y1, &x2, &y2))
        {
	dist = x2 - x1;
        if(dist < 1.0) dist = 1.0; /* min resolution is one pixel */
        }
	else
	{
	dist = 1.0; /* min resolution is one pixel */
        }

if(GLOBALS->wavearea_gesture_initial_zoom_x_distance < 1.0) GLOBALS->wavearea_gesture_initial_zoom_x_distance = 1.0; /* min resolution is one pixel */
scale = GLOBALS->wavearea_gesture_initial_zoom_x_distance / dist;
}
#else
{
if(scale < 0.00001) scale = 0.00001;
scale = (1.0 / scale); /* matches version for WAVE_GTK3_GESTURE_ZOOM_IS_1D which is initial_distance/distance instead of distance/priv->initial_distance from gtk */
}
#endif

ls = log(scale);

if((lzb != 0.0) && (scale > 0.0))
	{
	r = ls / lzb;
	z0 = GLOBALS->wavearea_gesture_initial_zoom - r;
	if((z0 <= 0.0) && (isnormal(z0)))
		{
		/* printf("XXX %e %e\n", scale, z0); */

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
			{
			TimeType width, new_x1tim;

                        calczoom(GLOBALS->tims.zoom = z0);
                        width = (TimeType)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
                        GLOBALS->tims.start = GLOBALS->wavearea_gesture_initial_x1tim - (x1 * GLOBALS->nspx);

                        new_x1tim  = GLOBALS->tims.start + (x1 * GLOBALS->nspx);
                        GLOBALS->tims.start += (GLOBALS->wavearea_gesture_initial_x1tim - new_x1tim);

			GLOBALS->tims.marker = new_x1tim;
			GLOBALS->tims.baseline = GLOBALS->tims.start + (x2 * GLOBALS->nspx);
			GLOBALS->tims.lmbcache = -1;
			GLOBALS->in_button_press_wavewindow_c_1 = 0;

			if(GLOBALS->tims.start + width > GLOBALS->tims.last) GLOBALS->tims.start = time_trunc(GLOBALS->tims.last - width);
			if(GLOBALS->tims.start < GLOBALS->tims.first) GLOBALS->tims.start = GLOBALS->tims.first;
			gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache = GLOBALS->tims.start);
	
			fix_wavehadj();
	
			g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
			g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
			}
#else
			{
			GLOBALS->tims.zoom = z0 + 1.0; /* 1.0 compensates for what service_zoom_out does */
			service_zoom_out(NULL, NULL);
			}
#endif

		GLOBALS->tims.prevzoom = GLOBALS->wavearea_gesture_initial_zoom;
		}
	}
}


#ifdef WAVE_GTK3_GESTURE_ZOOM_USES_GTK_PHASE_CAPTURE
static void
wavearea_zoom_update_event (GtkGestureZoom   *gesture,
                        GdkEventSequence *sequence,
                        gpointer user_data)
{
(void) sequence;

wavearea_zoom_scale_changed_event(gesture, gtk_gesture_zoom_get_scale_delta (gesture), user_data);
}
#endif

static void
wavearea_zoom_end_event (GtkGestureZoom   *gesture,
                     	GdkEventSequence *sequence,
                     	gpointer user_data)
{
(void) gesture;
(void) sequence;
(void) user_data;

GLOBALS->tims.marker = -1;
GLOBALS->tims.baseline = -1;
GLOBALS->tims.lmbcache = -1;
GLOBALS->in_button_press_wavewindow_c_1 = 0;

g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

gesture_in_zoom = 1;

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
	{
	/* nothing for wayland */
	}
	else
#endif
	{
	gesture_filter_cnt = 10; /* to prevent mouse pointer from flying all over at zoom release */
	}
}


void
wavearea_swipe_event (GtkGestureSwipe *gesture,
               gdouble          velocity_x,
               gdouble          velocity_y,
               gpointer         user_data)
{
(void) gesture;
(void) user_data;
(void) velocity_y;

if(gesture_filter_cnt)
	{
	wavearea_zero_out_swipe_velocity();
	return;
	}

GLOBALS->wavearea_gesture_swipe_velocity_x = velocity_x;

if(GLOBALS->swipe_init_time) { g_date_time_unref(GLOBALS->swipe_init_time); }
GLOBALS->swipe_init_time = g_date_time_new_now_utc();

GLOBALS->swipe_init_start = GLOBALS->tims.start;
}


void
wavearea_swipe_update_event (GtkGesture *gesture,
               GdkEventSequence *sequence,
               gpointer          user_data)
{
(void) gesture;
(void) sequence;
(void) user_data;
gdouble velocity_x, velocity_y;

gboolean rc = gtk_gesture_swipe_get_velocity (GTK_GESTURE_SWIPE(GLOBALS->wavearea_gesture_swipe),
                                &velocity_x,
                                &velocity_y);
if(rc && !gesture_filter_cnt)
	{
	GLOBALS->wavearea_gesture_swipe_velocity_x = velocity_x;
	}
}

static gboolean wavearea_swipe_tick(GtkWidget *widget,
                    GdkFrameClock *frame_clock,
                    gpointer user_data)
{
(void) widget;
(void) frame_clock;
(void) user_data;

if(gesture_filter_cnt > 0) /* for X11 */
	{
	wavearea_zero_out_swipe_velocity();
	gesture_filter_cnt--;
	return(G_SOURCE_CONTINUE);
	}

if(GLOBALS->swipe_init_time)
	{
	GDateTime *gd2 = g_date_time_new_now_utc();
	GTimeSpan elapsed = g_date_time_difference(gd2, GLOBALS->swipe_init_time);
	gdouble decaying = 0.0;
	g_date_time_unref(gd2);
	if(elapsed < WAVE_GTK3_SWIPE_MICROSECONDS)
		{
		decaying = exp(-elapsed / ((gdouble)WAVE_GTK3_SWIPE_TIME_CONSTANT));
		}

	if(GLOBALS->wavearea_gesture_swipe_velocity_x < -1.0)
		{
		GtkAdjustment *hadj;
		gfloat inc;
		TimeType ntinc, pageinc;

		if(!GLOBALS->wavearea_drag_active)
			{
			TimeType dest = GLOBALS->swipe_init_start - (GLOBALS->wavearea_gesture_swipe_velocity_x * GLOBALS->nspx * WAVE_GTK3_SWIPE_VEL_VS_DIST_FACTOR) * (1.0 - decaying);
			/* gdouble vp = -GLOBALS->wavearea_gesture_swipe_velocity_x; */
			hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
			ntinc = inc = dest - gtk_adjustment_get_value(hadj);
			if(ntinc)
				{
				pageinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

				if(dest <= (GLOBALS->tims.last-pageinc)) gtk_adjustment_set_value(hadj, GLOBALS->tims.timecache = dest);
					else { gtk_adjustment_set_value(hadj, GLOBALS->tims.timecache = GLOBALS->tims.last-pageinc); elapsed = WAVE_GTK3_SWIPE_MICROSECONDS; }

			        if(GLOBALS->tims.timecache<GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.first;

				time_update();
				}
			}

		if(elapsed >= WAVE_GTK3_SWIPE_MICROSECONDS)
			{
			wavearea_zero_out_swipe_velocity();
			GLOBALS->tims.baseline = -1;
			GLOBALS->tims.lmbcache = -1;
			GLOBALS->in_button_press_wavewindow_c_1 = 0;
			}
		}

	if(GLOBALS->wavearea_gesture_swipe_velocity_x > 1.0)
		{
		GtkAdjustment *hadj;

		if(!GLOBALS->wavearea_drag_active)
			{
			TimeType dest = GLOBALS->swipe_init_start - (GLOBALS->wavearea_gesture_swipe_velocity_x * GLOBALS->nspx * WAVE_GTK3_SWIPE_VEL_VS_DIST_FACTOR) * (1.0 - decaying);
			/* gdouble vp = GLOBALS->wavearea_gesture_swipe_velocity_x; */
			hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);

				if(dest>=GLOBALS->tims.first) gtk_adjustment_set_value(hadj, dest);
				        else { gtk_adjustment_set_value(hadj, GLOBALS->tims.first); ; elapsed = WAVE_GTK3_SWIPE_MICROSECONDS; }

				if(dest>=GLOBALS->tims.first) GLOBALS->tims.timecache=dest;
				        else { GLOBALS->tims.timecache=GLOBALS->tims.first; elapsed = WAVE_GTK3_SWIPE_MICROSECONDS; }

				time_update();
			}

		if(elapsed >= WAVE_GTK3_SWIPE_MICROSECONDS)
			{
			wavearea_zero_out_swipe_velocity();
			GLOBALS->tims.baseline = -1;
			GLOBALS->tims.lmbcache = -1;
			GLOBALS->in_button_press_wavewindow_c_1 = 0;
			}
		}
	}

GdkWindow *gw = gtk_widget_get_window(GLOBALS->wavearea);

if(gw)
	{
	gdouble x = 0, pixstep, offset;
	TimeType newcurr = 0;
	GdkModifierType state = 0;
	gint xi = 0, yi = 0;

	int dummy_x, dummy_y;
	get_window_xypos(&dummy_x, &dummy_y);

	WAVE_GDK_GET_POINTER(gtk_widget_get_window(GLOBALS->wavearea), &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY_XONLY;

	if((x >= 0) && (x < GLOBALS->wavewidth))
		{
		pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
		newcurr=GLOBALS->tims.start+(offset=x*pixstep);
		if((offset-((int)offset))>0.5)  /* round to nearest integer ns */
			{
		        newcurr++;
		        }
		
		if(newcurr>GLOBALS->tims.last) newcurr=GLOBALS->tims.last;
		
		if(newcurr!=GLOBALS->prevtim_wavewindow_c_1)
			{
		        update_currenttime(time_trunc(newcurr));
		        GLOBALS->prevtim_wavewindow_c_1=newcurr;
		        }
		}
	}

return(G_SOURCE_CONTINUE);
}

#endif


GtkWidget *
create_wavewindow(void)
{
GtkWidget *table;
GtkWidget *frame;
GtkAdjustment *hadj, *vadj;

table = XXX_gtk_table_new(10, 10, FALSE);

#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_DEPRECATED_API
/* this removes the warning generated from XXX_gtk_table_attach() on GLOBALS->vscroll_wavewindow_c_1 below */
gtk_container_set_resize_mode(GTK_CONTAINER(table), GTK_RESIZE_IMMEDIATE);
#endif

GLOBALS->wavearea=gtk_drawing_area_new();
gtk_widget_show(GLOBALS->wavearea);

gtk_widget_set_events(GLOBALS->wavearea,
		GDK_SCROLL_MASK
                | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
                | GDK_BUTTON_RELEASE_MASK
                | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
		| GDK_TOUCH_MASK
		| GDK_TABLET_PAD_MASK
		| GDK_TOUCHPAD_GESTURE_MASK
#endif
                );

g_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "configure_event",G_CALLBACK(wavearea_configure_event_local), NULL);
#if GTK_CHECK_VERSION(3,0,0)
g_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "draw",G_CALLBACK(draw_event), NULL);
#else
g_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "expose_event",G_CALLBACK(expose_event_local), NULL);
#endif

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
if(GLOBALS->use_gestures < 0) /* <0 means "maybe (if available, enable)" */
	{
	GdkDisplay *display = gdk_display_get_default();
	GdkSeat *seat = gdk_display_get_default_seat (display);
	GdkSeatCapabilities gsc = gdk_seat_get_capabilities(seat);
	if(gsc & GDK_SEAT_CAPABILITY_TOUCH)
		{
		printf("GTKWAVE | Touch screen detected, enabling gestures.\n");
		GLOBALS->use_gestures = 1;
		}
		else
		{
		GLOBALS->use_gestures = 0;
		}
	}

if(GLOBALS->use_gestures)
{
/* so far is mutually exclusive with existing motion/button action below */
GtkGesture *gs;

GLOBALS->wavearea_gesture_initial_zoom = GLOBALS->tims.zoom;
gs =  gtk_gesture_zoom_new(GLOBALS->wavearea);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "begin", G_CALLBACK(wavearea_zoom_begin_event), gs);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "end", G_CALLBACK(wavearea_zoom_end_event), gs);
#ifdef WAVE_GTK3_GESTURE_ZOOM_USES_GTK_PHASE_CAPTURE
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "update", G_CALLBACK(wavearea_zoom_update_event), gs);
gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER(gs), GTK_PHASE_CAPTURE);
#else
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "scale-changed", G_CALLBACK(wavearea_zoom_scale_changed_event), gs);
#endif

gs = gtk_gesture_multi_press_new (GLOBALS->wavearea);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "pressed", G_CALLBACK(wavearea_pressed_event), NULL);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "released", G_CALLBACK(wavearea_released_event), NULL);
#ifdef WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON
gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gs), 0);
#endif

gs = gtk_gesture_long_press_new (GLOBALS->wavearea);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "pressed", G_CALLBACK(wavearea_long_pressed_event), NULL);

gs = gtk_gesture_drag_new (GLOBALS->wavearea);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "drag_begin", G_CALLBACK(wavearea_drag_begin_event), NULL);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "drag_update", G_CALLBACK(wavearea_drag_update_event), NULL);
gtkwave_signal_connect(XXX_GTK_OBJECT(gs), "drag_end", G_CALLBACK(wavearea_drag_end_event), NULL);
#ifdef WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON
gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gs), 0);
#endif

GLOBALS->wavearea_gesture_swipe = gtk_gesture_swipe_new (GLOBALS->wavearea);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea_gesture_swipe), "swipe", G_CALLBACK(wavearea_swipe_event), GLOBALS);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea_gesture_swipe), "update", G_CALLBACK(wavearea_swipe_update_event), GLOBALS);
gtk_widget_add_tick_callback (GTK_WIDGET(GLOBALS->wavearea), wavearea_swipe_tick, NULL, NULL);
}
else
#endif
{
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "motion_notify_event",G_CALLBACK(motion_notify_event), NULL);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "button_press_event",G_CALLBACK(button_press_event), NULL);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "button_release_event",G_CALLBACK(button_release_event), NULL);
}

gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "scroll_event",G_CALLBACK(scroll_event), NULL);
gtk_widget_set_can_focus(GTK_WIDGET(GLOBALS->wavearea), TRUE);

XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->wavearea, 0, 9, 0, 9,GTK_FILL | GTK_EXPAND,GTK_FILL | GTK_EXPAND, 3, 2);

GLOBALS->wave_vslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
vadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wave_vslider), "value_changed",G_CALLBACK(service_vslider), NULL);
GLOBALS->vscroll_wavewindow_c_1=XXX_gtk_vscrollbar_new(vadj);
/* GTK_WIDGET_SET_FLAGS(GLOBALS->vscroll_wavewindow_c_1, GTK_CAN_FOCUS); */
gtk_widget_show(GLOBALS->vscroll_wavewindow_c_1);

/* this moves GLOBALS->wave_vslider updates out from signalarea_configure_event where it was causing size allocation warnings */
#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
gtk_widget_add_tick_callback (GTK_WIDGET(GLOBALS->vscroll_wavewindow_c_1),
                              wave_vslider_gtc,
                              NULL,
                              NULL);
#endif

#ifdef WAVE_ALLOW_GTK3_GRID
XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->vscroll_wavewindow_c_1, 9, 10, 0, 9,
                        0,
                        GTK_SHRINK, 3, 3);
#else
XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->vscroll_wavewindow_c_1, 9, 10, 0, 9,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 3);
#endif
GLOBALS->wave_hslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wave_hslider), "value_changed",G_CALLBACK(service_hslider), NULL);
GLOBALS->hscroll_wavewindow_c_2=XXX_gtk_hscrollbar_new(hadj);
/* GTK_WIDGET_SET_FLAGS(GLOBALS->hscroll_wavewindow_c_2, GTK_CAN_FOCUS); */
gtk_widget_show(GLOBALS->hscroll_wavewindow_c_2);

#ifdef WAVE_ALLOW_SLIDER_ZOOM
if(GLOBALS->enable_slider_zoom)
	{
	GValue gvalue;

	if(!draw_slider_p)
		{
		GtkStyle *gs = gtk_widget_get_style(GLOBALS->hscroll_wavewindow_c_2);
		draw_slider_p = GTK_STYLE_GET_CLASS(gs)->draw_slider;
		GTK_STYLE_GET_CLASS(gs)->draw_slider = draw_slider;
		}

	memset(&gvalue, 0, sizeof(GValue));
	g_value_init(&gvalue, G_TYPE_INT);
	gtk_widget_style_get_property(GLOBALS->hscroll_wavewindow_c_2, "min-slider-length", &gvalue);

	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->hscroll_wavewindow_c_2), "button_press_event",G_CALLBACK(slider_bpr), NULL);
	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->hscroll_wavewindow_c_2), "button_release_event",G_CALLBACK(slider_brr), NULL);
	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->hscroll_wavewindow_c_2), "motion_notify_event",G_CALLBACK(slider_mnr), NULL);
	}
#endif

#ifdef WAVE_ALLOW_GTK3_GRID
XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->hscroll_wavewindow_c_2, 0, 9, 9, 10,
                        0,
                        GTK_SHRINK, 3, 4);
#else
XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->hscroll_wavewindow_c_2, 0, 9, 9, 10,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 4);
#endif
gtk_widget_show(table);

frame=gtk_frame_new("Waves");
gtk_container_set_border_width(GTK_CONTAINER(frame),2);

gtk_container_add(GTK_CONTAINER(frame),table);
return(frame);
}


/**********************************************/

void RenderSigs(int trtarget, int update_waves)
{
Trptr t;
int i, trwhich;
int num_traces_displayable;
GtkAdjustment *hadj;
int xsrc;

hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
xsrc=(gint)gtk_adjustment_get_value(hadj);

GtkAllocation allocation;
gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

num_traces_displayable=allocation.height/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

if(!GLOBALS->use_dark)
	{
	cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc.gc_mdgray.r, GLOBALS->rgb_gc.gc_mdgray.g, GLOBALS->rgb_gc.gc_mdgray.b, GLOBALS->rgb_gc.gc_mdgray.a);
	cairo_rectangle (GLOBALS->cr_signalpixmap, 0, -1, GLOBALS->signal_fill_width, GLOBALS->fontheight);
	cairo_fill (GLOBALS->cr_signalpixmap);
	}

cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_white.r, GLOBALS->rgb_gc_white.g, GLOBALS->rgb_gc_white.b, GLOBALS->rgb_gc_white.a);
cairo_move_to (GLOBALS->cr_signalpixmap, WAVE_CAIRO_050_OFFSET, GLOBALS->fontheight-1+WAVE_CAIRO_050_OFFSET);
cairo_line_to (GLOBALS->cr_signalpixmap, GLOBALS->signal_fill_width-1+WAVE_CAIRO_050_OFFSET, GLOBALS->fontheight-1+WAVE_CAIRO_050_OFFSET);
cairo_stroke (GLOBALS->cr_signalpixmap);

if(!GLOBALS->use_dark)
	{
	XXX_font_engine_draw_string(GLOBALS->cr_signalpixmap, GLOBALS->signalfont, &(GLOBALS->rgb_gc_black),
		3+xsrc+WAVE_CAIRO_050_OFFSET, GLOBALS->fontheight-4+WAVE_CAIRO_050_OFFSET, "Time");
	}

t=GLOBALS->traces.first;
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

GLOBALS->topmost_trace=t;
if(t)
        {
        for(i=0;(i<num_traces_displayable)&&(t);i++)
                {
		RenderSig(t, i, 0);
                t=GiveNextTrace(t);
                }
        }

if(GLOBALS->signalarea_has_focus)
	{
	cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_black.r, GLOBALS->rgb_gc_black.g, GLOBALS->rgb_gc_black.b, GLOBALS->rgb_gc_black.a);
	cairo_rectangle (GLOBALS->cr_signalpixmap, 0, 0, GLOBALS->signal_fill_width-1, allocation.height-1);
	cairo_stroke (GLOBALS->cr_signalpixmap);
	}

if((GLOBALS->cr_wavepixmap_wavewindow_c_1)&&(update_waves))
	{
        XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_back_wavewindow_c_1, TRUE, 0, 0,GLOBALS->wavewidth, GLOBALS->waveheight);

	/* if(GLOBALS->display_grid) */ rendertimes();

	rendertraces();
	}
}


void populateBuffer (Trptr t, char *altname, char* buf)
{
  char* ptr = buf;
  char *tname = altname ? altname : t->name;

  if (HasWave(t))
    {
      if (tname)
	{
	  strcpy(ptr, tname);
	  ptr = ptr + strlen(ptr);

	  if((tname)&&(t->shift))
	    {
	      ptr[0]='`';
	      reformat_time(ptr+1, t->shift, GLOBALS->time_dimension);
	      ptr = ptr + strlen(ptr+1) + 1;
	      strcpy(ptr,"\'");
#ifdef WAVE_ARRAY_SUPPORT
	      ptr = ptr + strlen(ptr); /* really needed for aet2 only */
#endif
	    }

#ifdef WAVE_ARRAY_SUPPORT
	  if((!t->vector)&&(t->n.nd)&&(t->n.nd->array_height))
	    {
	      sprintf(ptr, "{%d}", t->n.nd->this_row);
	      /* ptr = ptr + strlen(ptr); */ /* scan-build */
	    }
#endif
	}

      if (IsGroupBegin(t))
	{
	  char * pch;
	  ptr = buf;
	  if (IsClosed(t)) {
	    pch = strstr (ptr,"[-]");
	    if(pch) {memcpy (pch,"[+]", 3); }
	  } else {
	    pch = strstr (ptr,"[+]");
	    if(pch) {memcpy (pch,"[-]", 3); }
	  }
	}
    }
  else
    {
      if (tname)
	{

	  if (IsGroupEnd(t))
	    {
	      strcpy(ptr, "} ");
	      ptr = ptr + strlen(ptr);
	    }

	  strcpy(ptr, tname);
	  ptr = ptr + strlen(ptr);

	  if (IsGroupBegin(t))
	    {
	      if (IsClosed(t) && IsCollapsed(t->t_match))
		{
		  strcpy(ptr, " {}");
		}
	      else
		{
		  strcpy(ptr, " {");
		}
	      /* ptr = ptr + strlen(ptr); */ /* scan-build */
	    }
	}
    }

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) gtk_widget_queue_draw(GLOBALS->signalarea);
#endif
}

/***************************************************************************/

int RenderSig(Trptr t, int i, int dobackground)
{
  int texty, liney;
  int retval;
  char buf[2048];
  wave_rgb_t clr_comment;
  wave_rgb_t clr_group;
  wave_rgb_t clr_shadowed;
  wave_rgb_t clr_signal;
  wave_rgb_t bg_color;
  wave_rgb_t text_color;
  unsigned left_justify;
  char *subname = NULL;
  bvptr bv = NULL;

  buf[0] = 0;

  if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        Trptr tscan = t;
        int bcnt = 0;
        while((tscan) && (tscan = GivePrevTrace(tscan)))
                {
                if(!(tscan->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                        {
                        if(tscan->flags & TR_TTRANSLATED)
                                {
                                break; /* found it */
                                }
                                else
                                {
                                tscan = NULL;
                                }
                        }
                        else
                        {
                        bcnt++; /* bcnt is number of blank traces */
                        }
                }

        if((tscan)&&(tscan->vector))
                {
                bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
                        subname = bv->bvname;
                        if(GLOBALS->hier_max_level)
                                subname = hier_extract(subname, GLOBALS->hier_max_level);
                        }
                }
        }

  populateBuffer(t, subname, buf);

  clr_comment  = GLOBALS->rgb_gc.gc_brkred;
  clr_group    = GLOBALS->rgb_gc.gc_gmstrd;
  clr_shadowed = GLOBALS->rgb_gc.gc_ltblue;
  clr_signal   = GLOBALS->rgb_gc.gc_dkblue;

  UpdateSigValue(t); /* in case it's stale on nonprop */

  liney=((i+2)*GLOBALS->fontheight)-2;
  texty=liney-(GLOBALS->signalfont->descent);

  retval=liney-GLOBALS->fontheight+1;

  left_justify = ((IsGroupBegin(t) || IsGroupEnd(t)) && !HasWave(t))|| GLOBALS->left_justify_sigs;

  if (IsSelected(t))
    {
      bg_color = (!HasWave(t))
	? ((IsGroupBegin(t) || IsGroupEnd(t)) ? clr_group : clr_comment)
	: ((IsShadowed(t)) ? clr_shadowed : clr_signal);
      text_color = GLOBALS->rgb_gc_white;
    }
  else
    {
      bg_color = (dobackground==2) ?  GLOBALS->rgb_gc.gc_normal : GLOBALS->rgb_gc.gc_ltgray;
      if(HasWave(t))
	{ text_color = GLOBALS->rgb_gc_black; }
      else
	{ text_color = (IsGroupBegin(t) || IsGroupEnd(t)) ? clr_group : clr_comment; }
    }

  if (dobackground || IsSelected(t))
    {
	cairo_set_source_rgba (GLOBALS->cr_signalpixmap, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
	cairo_rectangle (GLOBALS->cr_signalpixmap, 0, retval,
                         GLOBALS->signal_fill_width, GLOBALS->fontheight-1);
	cairo_fill (GLOBALS->cr_signalpixmap);
    }


  cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_white.r, GLOBALS->rgb_gc_white.g, GLOBALS->rgb_gc_white.b, GLOBALS->rgb_gc_white.a);
  cairo_move_to (GLOBALS->cr_signalpixmap, WAVE_CAIRO_050_OFFSET, liney+WAVE_CAIRO_050_OFFSET);
  cairo_line_to (GLOBALS->cr_signalpixmap, GLOBALS->signal_fill_width-1+WAVE_CAIRO_050_OFFSET, liney+WAVE_CAIRO_050_OFFSET);
  cairo_stroke (GLOBALS->cr_signalpixmap);

  if((t->name)||(subname))
    {
      XXX_font_engine_draw_string(GLOBALS->cr_signalpixmap,
			      GLOBALS->signalfont,
			      &text_color,
			      (left_justify?3:3+GLOBALS->max_signal_name_pixel_width-
			      font_engine_string_measure(GLOBALS->signalfont, buf))+WAVE_CAIRO_050_OFFSET,
			      texty+WAVE_CAIRO_050_OFFSET,
			      buf);
    }

  if (HasWave(t) || bv)
    {
      if((t->asciivalue)&&(!(t->flags&TR_EXCLUDE)))
	XXX_font_engine_draw_string(GLOBALS->cr_signalpixmap,
				GLOBALS->signalfont,
				&text_color,
				GLOBALS->max_signal_name_pixel_width+6+WAVE_CAIRO_050_OFFSET,
				texty+WAVE_CAIRO_050_OFFSET,
				t->asciivalue);
    }

  return(retval);
}


/***************************************************************************/

void MaxSignalLength(void)
{
Trptr t;
int len=0,maxlen=0;
int vlen=0, vmaxlen=0;
char buf[2048];
char dirty_kick;
bvptr bv;
Trptr tscan;

DEBUG(printf("signalwindow_width_dirty: %d\n",GLOBALS->signalwindow_width_dirty));

if((!GLOBALS->signalwindow_width_dirty)&&(GLOBALS->use_nonprop_fonts)) return;

dirty_kick = GLOBALS->signalwindow_width_dirty;
GLOBALS->signalwindow_width_dirty=0;

t=GLOBALS->traces.first;


while(t)
  {
  char *subname = NULL;
  bv = NULL;
  tscan = NULL;

  if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        int bcnt = 0;
        tscan = t;
        while((tscan) && (tscan = GivePrevTrace(tscan)))
                {
                if(!(tscan->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                        {
                        if(tscan->flags & TR_TTRANSLATED)
                                {
                                break; /* found it */
                                }
                                else
                                {
                                tscan = NULL;
                                }
                        }
                        else
                        {
                        bcnt++; /* bcnt is number of blank traces */
                        }
                }

        if((tscan)&&(tscan->vector))
                {
                bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
                        subname = bv->bvname;
                        if(GLOBALS->hier_max_level)
                                subname = hier_extract(subname, GLOBALS->hier_max_level);
                        }
                }
        }

    populateBuffer(t, subname, buf);

    if(!bv && (t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))	/* for "comment" style blank traces */
      {
	if(t->name || subname)
	  {
	    len=font_engine_string_measure(GLOBALS->signalfont, buf);
	    if(len>maxlen) maxlen=len;
	  }

	if(t->asciivalue)
		{
		free_2(t->asciivalue); t->asciivalue = NULL;
		}
	t=GiveNextTrace(t);
      }
    else
      if(t->name || subname)
	{
	  len=font_engine_string_measure(GLOBALS->signalfont, buf);
	  if(len>maxlen) maxlen=len;

	  if((GLOBALS->tims.marker!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
		t->asciitime=GLOBALS->tims.marker;
		if(t->asciivalue) { free_2(t->asciivalue); } t->asciivalue = NULL;

		if(bv || t->vector)
			{
			char *str, *str2;
			vptr v;
			Trptr ts;
			TraceEnt t_temp;

			if(bv)
				{
				ts = &t_temp;
				memcpy(ts, tscan, sizeof(TraceEnt));
				ts->vector = 1;
				ts->n.vec = bv;
				}
				else
				{
				ts = t;
				bv = t->n.vec;
				}


                        v=bsearch_vector(bv, GLOBALS->tims.marker - ts->shift);
                        str=convert_ascii(ts,v);
			if(str)
				{
				str2=(char *)malloc_2(strlen(str)+2);
				*str2='=';
				strcpy(str2+1,str);
				free_2(str);

				vlen=font_engine_string_measure(GLOBALS->signalfont,str2);
				t->asciivalue=str2;
				}
				else
				{
				vlen=0;
				t->asciivalue=NULL;
				}
			}
			else
			{
			char *str;
			hptr h_ptr;
			if((h_ptr=bsearch_node(t->n.nd,GLOBALS->tims.marker - t->shift)))
				{
				if(!t->n.nd->extvals)
					{
					unsigned char h_val = h_ptr->v.h_val;

					str=(char *)calloc_2(1,3*sizeof(char));
					str[0]='=';
					if(t->n.nd->vartype == ND_VCD_EVENT)
						{
						h_val = (h_ptr->time >= GLOBALS->tims.first) && ((GLOBALS->tims.marker-GLOBALS->shift_timebase) == h_ptr->time) ? AN_1 : AN_0; /* generate impulse */
						}

					if(t->flags&TR_INVERT)
						{
						str[1]=AN_STR_INV[h_val];
						}
						else
						{
						str[1]=AN_STR[h_val];
						}
					t->asciivalue=str;
					vlen=font_engine_string_measure(GLOBALS->signalfont,str);
					}
					else
					{
					char *str2;

					if(h_ptr->flags&HIST_REAL)
						{
						if(!(h_ptr->flags&HIST_STRING))
							{
#ifdef WAVE_HAS_H_DOUBLE
							str=convert_ascii_real(t, &h_ptr->v.h_double);
#else
							str=convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
							}
							else
							{
							str=convert_ascii_string((char *)h_ptr->v.h_vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,h_ptr->v.h_vector);
						}

					if(str)
						{
						str2=(char *)malloc_2(strlen(str)+2);
						*str2='=';
						strcpy(str2+1,str);

						free_2(str);

						vlen=font_engine_string_measure(GLOBALS->signalfont,str2);
						t->asciivalue=str2;
						}
						else
						{
						vlen=0;
						t->asciivalue=NULL;
						}
					}
				}
				else
				{
				vlen=0;
				t->asciivalue=NULL;
				}
			}

		if(vlen>vmaxlen)
			{
			vmaxlen=vlen;
			}
		}

	t=GiveNextTrace(t);
	}
	else
	{
	t=GiveNextTrace(t);
	}
}

GLOBALS->max_signal_name_pixel_width = maxlen;
GLOBALS->signal_pixmap_width=maxlen+6; 		/* 2 * 3 pixel pad */
if(GLOBALS->tims.marker!=-1)
	{
	GLOBALS->signal_pixmap_width+=(vmaxlen+6);
	if(GLOBALS->signal_pixmap_width > 32767) GLOBALS->signal_pixmap_width = 32767; /* fixes X11 protocol limitation crash */
	}

GLOBALS->tims.resizemarker2 = GLOBALS->tims.resizemarker;
GLOBALS->tims.resizemarker = GLOBALS->tims.marker;

if(GLOBALS->signal_pixmap_width<60) GLOBALS->signal_pixmap_width=60;

MaxSignalLength_2(dirty_kick);
}


void MaxSignalLength_2(char dirty_kick)
{
if(!GLOBALS->in_button_press_wavewindow_c_1)
	{
	if(!GLOBALS->do_resize_signals)
		{
		int os;
		os=48;
		if(GLOBALS->initial_signal_window_width > os)
			{
			os = GLOBALS->initial_signal_window_width;
			}

		if(GLOBALS->signalwindow)
			{
			GtkAllocation allocation;
			gtk_widget_get_allocation(GLOBALS->signalwindow, &allocation);

			  /* printf("VALUES: %d %d %d\n", GLOBALS->initial_signal_window_width, allocation.width, GLOBALS->max_signal_name_pixel_width); */
			  if (GLOBALS->first_unsized_signals && GLOBALS->max_signal_name_pixel_width !=0)
			    {
			      GLOBALS->first_unsized_signals = 0;
			      gtk_paned_set_position(GTK_PANED(GLOBALS->panedwindow), GLOBALS->max_signal_name_pixel_width+30);
			    } else {
			      gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow), os+30, -1);
		            }
			}
		}
	else
	if((GLOBALS->do_resize_signals)&&(GLOBALS->signalwindow))
		{
		int oldusize;
		int rs;

                if(GLOBALS->initial_signal_window_width > GLOBALS->max_signal_name_pixel_width)
                        {
                        rs=GLOBALS->initial_signal_window_width;
                        }
                        else
                        {
                        rs=GLOBALS->max_signal_name_pixel_width;
                        }

		GtkAllocation allocation;
		gtk_widget_get_allocation(GLOBALS->signalwindow, &allocation);

		oldusize=allocation.width;
		if((oldusize!=rs)||(dirty_kick))
			{ /* keep signalwindow from expanding arbitrarily large */
			int wx, wy;
			get_window_size(&wx, &wy);

			if((3*rs) < (2*wx))	/* 2/3 width max */
				{
				int os;
				os=rs;
				os=(os<48)?48:os;
				gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow),
						os+30, -1);
				}
				else
				{
				int os;
				os=48;
		                if(GLOBALS->initial_signal_window_width > os)
                		        {
		                        os = GLOBALS->initial_signal_window_width;
		                        }

				gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow),
						os+30, -1);
				}
			}
		}
	}
}

/***************************************************************************/

void UpdateSigValue(Trptr t)
{
bvptr bv = NULL;
Trptr tscan = NULL;

if(!t) return;
if((t->asciivalue)&&(t->asciitime==GLOBALS->tims.marker))return;

if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        int bcnt = 0;
        tscan = t;
        while((tscan) && (tscan = GivePrevTrace(tscan)))
                {
                if(!(tscan->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                        {
                        if(tscan->flags & TR_TTRANSLATED)
                                {
                                break; /* found it */
                                }
                                else
                                {
                                tscan = NULL;
                                }
                        }
                        else
                        {
                        bcnt++; /* bcnt is number of blank traces */
                        }
                }

        if((tscan)&&(tscan->vector))
                {
                bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
			/* nothing, we just want to set bv */
                        }
                }
        }


if((t->name || bv)&&(bv || !(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))))
	{
	GLOBALS->shift_timebase=t->shift;
	DEBUG(printf("UpdateSigValue: %s\n",t->name));

	if((GLOBALS->tims.marker!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
		t->asciitime=GLOBALS->tims.marker;
		if(t->asciivalue) free_2(t->asciivalue);

		if(bv || t->vector)
			{
			char *str, *str2;
			vptr v;
                        Trptr ts;
                        TraceEnt t_temp;

                        if(bv)
                                {
                                ts = &t_temp;
                                memcpy(ts, tscan, sizeof(TraceEnt));
                                ts->vector = 1;
                                ts->n.vec = bv;
                                }
                                else
                                {
                                ts = t;
                                bv = t->n.vec;
				}

                        v=bsearch_vector(bv,GLOBALS->tims.marker - ts->shift);
                        str=convert_ascii(ts,v);
			if(str)
				{
				str2=(char *)malloc_2(strlen(str)+2);
				*str2='=';
				strcpy(str2+1,str);
				free_2(str);

				t->asciivalue=str2;
				}
				else
				{
				t->asciivalue=NULL;
				}
			}
			else
			{
			char *str;
			hptr h_ptr;
			if((h_ptr=bsearch_node(t->n.nd,GLOBALS->tims.marker - t->shift)))
				{
				if(!t->n.nd->extvals)
					{
					unsigned char h_val = h_ptr->v.h_val;
					if(t->n.nd->vartype == ND_VCD_EVENT)
						{
						h_val = (h_ptr->time >= GLOBALS->tims.first) && ((GLOBALS->tims.marker-GLOBALS->shift_timebase) == h_ptr->time) ? AN_1 : AN_0; /* generate impulse */
						}

					str=(char *)calloc_2(1,3*sizeof(char));
					str[0]='=';
					if(t->flags&TR_INVERT)
						{
						str[1]=AN_STR_INV[h_val];
						}
						else
						{
						str[1]=AN_STR[h_val];
						}
					t->asciivalue=str;
					}
					else
					{
					char *str2;

					if(h_ptr->flags&HIST_REAL)
						{
						if(!(h_ptr->flags&HIST_STRING))
							{
#ifdef WAVE_HAS_H_DOUBLE
							str=convert_ascii_real(t, &h_ptr->v.h_double);
#else
							str=convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
							}
							else
							{
							str=convert_ascii_string((char *)h_ptr->v.h_vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,h_ptr->v.h_vector);
						}

					if(str)
						{
						str2=(char *)malloc_2(strlen(str)+2);
						*str2='=';
						strcpy(str2+1,str);
						free_2(str);

						t->asciivalue=str2;
						}
						else
						{
						t->asciivalue=NULL;
						}
					}
				}
				else
				{
				t->asciivalue=NULL;
				}
			}
		}
	}
}

/***************************************************************************/

void calczoom(gdouble z0)
{
gdouble ppf, frame;
ppf=((gdouble)(GLOBALS->pixelsperframe=200));
frame=pow(GLOBALS->zoombase,-z0);

if(frame>((gdouble)MAX_HISTENT_TIME/(gdouble)4.0))
	{
	GLOBALS->nsperframe=((gdouble)MAX_HISTENT_TIME/(gdouble)4.0);
	}
	else
	if(frame<(gdouble)1.0)
	{
	GLOBALS->nsperframe=1.0;
	}
	else
	{
	GLOBALS->nsperframe=frame;
	}

GLOBALS->hashstep=10.0;

if(GLOBALS->zoom_pow10_snap)
if(GLOBALS->nsperframe>10.0)
	{
	TimeType nsperframe2;
	gdouble p=10.0;
	gdouble scale;
	int l;
	l=(int)((log(GLOBALS->nsperframe)/log(p))+0.5);	/* nearest power of 10 */
	nsperframe2=pow(p, (gdouble)l);

	scale = (gdouble)nsperframe2 / (gdouble)GLOBALS->nsperframe;
	ppf *= scale;
	GLOBALS->pixelsperframe = ppf;

	GLOBALS->nsperframe = nsperframe2;
	GLOBALS->hashstep = ppf / 10.0;
	}

GLOBALS->nspx=GLOBALS->nsperframe/ppf;
GLOBALS->pxns=ppf/GLOBALS->nsperframe;

time_trunc_set();	/* map nspx to rounding value */

DEBUG(printf("Zoom: %e Pixelsperframe: %d, nsperframe: %e\n",z0, (int)GLOBALS->pixelsperframe,(float)GLOBALS->nsperframe));
}

static void renderhash(int x, TimeType tim)
{
TimeType rborder;
int fhminus2;
int rhs;
gdouble dx;
gdouble hashoffset;
int iter = 0;
int s_ctx_iter;
int timearray_encountered = (GLOBALS->ruler_step != 0);

fhminus2=GLOBALS->fontheight-2;

WAVE_STRACE_ITERATOR(s_ctx_iter)
	{
	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
	if(GLOBALS->strace_ctx->timearray)
		{
		timearray_encountered = 1;
		break;
		}
	}

cairo_set_source_rgba (GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.r, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.g, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.b, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.a);
cairo_move_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, WAVE_CAIRO_050_OFFSET);
cairo_line_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, (((!timearray_encountered)&&(GLOBALS->display_grid)&&(GLOBALS->enable_vert_grid))?GLOBALS->waveheight:fhminus2)+WAVE_CAIRO_050_OFFSET);
cairo_stroke (GLOBALS->cr_wavepixmap_wavewindow_c_1);

if(tim==GLOBALS->tims.last) return;

rborder=(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns;
DEBUG(printf("Rborder: %lld, Wavewidth: %d\n", rborder, GLOBALS->wavewidth));
if(rborder>GLOBALS->wavewidth) rborder=GLOBALS->wavewidth;
if((rhs=x+GLOBALS->pixelsperframe)>rborder) rhs=rborder;

cairo_move_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, GLOBALS->wavecrosspiece+WAVE_CAIRO_050_OFFSET);
cairo_line_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, rhs+WAVE_CAIRO_050_OFFSET, GLOBALS->wavecrosspiece+WAVE_CAIRO_050_OFFSET);
cairo_stroke (GLOBALS->cr_wavepixmap_wavewindow_c_1);

dx = x + (hashoffset=GLOBALS->hashstep);
x  = dx;

while((hashoffset<GLOBALS->pixelsperframe)&&(x<=rhs)&&(iter<9))
	{
	cairo_move_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, GLOBALS->wavecrosspiece+WAVE_CAIRO_050_OFFSET);
	cairo_line_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, fhminus2+WAVE_CAIRO_050_OFFSET);
	cairo_stroke (GLOBALS->cr_wavepixmap_wavewindow_c_1);

	hashoffset+=GLOBALS->hashstep;
	dx=dx+GLOBALS->hashstep;
	if((GLOBALS->pixelsperframe!=200)||(GLOBALS->hashstep!=10.0)) iter++; /* fix any roundoff errors */
	x = dx;
	}

}

static void rendertimes(void)
{
int lastx = -1000; /* arbitrary */
int x, lenhalf;
TimeType tim, rem;
char timebuff[32];
char prevover=0;
gdouble realx;
int s_ctx_iter;
int timearray_encountered = 0;
static const double dashed1[] = { 8.0, 8.0 };

renderblackout();

tim=GLOBALS->tims.start;

GLOBALS->tims.end=GLOBALS->tims.start+(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

/***********/
WAVE_STRACE_ITERATOR_FWD(s_ctx_iter)
	{
	wave_rgb_t gc;

	if(!s_ctx_iter)
		{
		gc = GLOBALS->rgb_gc.gc_grid_wavewindow_c_1;
		cairo_set_dash(GLOBALS->cr_wavepixmap_wavewindow_c_1, dashed1, 0, 0);
		}
		else
		{
		gc = GLOBALS->rgb_gc.gc_grid2_wavewindow_c_1;
		/* gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_BEVEL); */
		cairo_set_dash(GLOBALS->cr_wavepixmap_wavewindow_c_1, dashed1, sizeof(dashed1) / sizeof(dashed1[0]), 0);
		}

	cairo_set_source_rgba (GLOBALS->cr_wavepixmap_wavewindow_c_1, gc.r, gc.g, gc.b, gc.a);

	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];

	if(GLOBALS->strace_ctx->timearray)
		{
		int pos, pos2;
		TimeType *t, tm;
		int y=GLOBALS->fontheight+2;
		int oldx=-1;

		timearray_encountered = 1;

		pos=bsearch_timechain(GLOBALS->tims.start);
		top:
		if((pos>=0)&&(pos<GLOBALS->strace_ctx->timearray_size))
			{
			t=GLOBALS->strace_ctx->timearray+pos;
			for(;pos<GLOBALS->strace_ctx->timearray_size;t++, pos++)
				{
				tm=*t;
				if(tm>=GLOBALS->tims.start)
					{
					if(tm<=GLOBALS->tims.end)
						{
						x=(tm-GLOBALS->tims.start)*GLOBALS->pxns;
						if(oldx==x)
							{
							pos2=bsearch_timechain(GLOBALS->tims.start+(((gdouble)(x+1))*GLOBALS->nspx));
							if(pos2>pos) { pos=pos2; goto top; } else continue;
							}
						oldx=x;

						cairo_move_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, y+WAVE_CAIRO_050_OFFSET);
					        cairo_line_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, GLOBALS->waveheight+WAVE_CAIRO_050_OFFSET);
						cairo_stroke (GLOBALS->cr_wavepixmap_wavewindow_c_1);
						}
						else
						{
						break;
						}
					}
				}
			}
		}


	if(s_ctx_iter)
		{
		/* gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL); */
		}
	}

GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = 0];
/***********/

cairo_set_dash(GLOBALS->cr_wavepixmap_wavewindow_c_1, dashed1, 0, 0);

if(GLOBALS->ruler_step && !timearray_encountered)
	{
	TimeType rhs = (GLOBALS->tims.end > GLOBALS->tims.last) ? GLOBALS->tims.last : GLOBALS->tims.end;
	TimeType low_x = (GLOBALS->tims.start - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
	TimeType high_x = (rhs - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
	TimeType iter_x, tm;
        int y=GLOBALS->fontheight+2;
        int oldx=-1;

	cairo_set_source_rgba (GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.r, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.g, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.b, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.a);

	for(iter_x = low_x; iter_x <= high_x; iter_x++)
		{
		tm = GLOBALS->ruler_step * iter_x +  GLOBALS->ruler_origin;
		x=(tm-GLOBALS->tims.start)*GLOBALS->pxns;
		if(oldx==x)
			{
			gdouble xd,offset,pixstep;
			TimeType newcurr;

			xd=x+1;  /* for pix time calc */

			pixstep=((gdouble)GLOBALS->nsperframe)/((gdouble)GLOBALS->pixelsperframe);
			newcurr=(TimeType)(offset=((gdouble)GLOBALS->tims.start)+(xd*pixstep));

			if(offset-newcurr>0.5)  /* round to nearest integer ns */
			        {
			        newcurr++;
			        }

			low_x = (newcurr - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
			if(low_x <= iter_x) low_x = (iter_x+1);
			iter_x = low_x;
			tm = GLOBALS->ruler_step * iter_x +  GLOBALS->ruler_origin;
			x=(tm-GLOBALS->tims.start)*GLOBALS->pxns;
			}

		if(x>=GLOBALS->wavewidth) break;
		oldx=x;

		cairo_move_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, y+WAVE_CAIRO_050_OFFSET);
	        cairo_line_to (GLOBALS->cr_wavepixmap_wavewindow_c_1, x+WAVE_CAIRO_050_OFFSET, GLOBALS->waveheight+WAVE_CAIRO_050_OFFSET);
		cairo_stroke (GLOBALS->cr_wavepixmap_wavewindow_c_1);
		}
	}

/***********/

DEBUG(printf("Ruler Start time: "TTFormat", Finish time: "TTFormat"\n",GLOBALS->tims.start, GLOBALS->tims.end));

x=0;
realx=0;
if(tim)
	{
	rem=tim%((TimeType)GLOBALS->nsperframe);
	if(rem)
		{
		tim=tim-GLOBALS->nsperframe-rem;
		x=-GLOBALS->pixelsperframe-((rem*GLOBALS->pixelsperframe)/GLOBALS->nsperframe);
		realx=-GLOBALS->pixelsperframe-((rem*GLOBALS->pixelsperframe)/GLOBALS->nsperframe);
		}
	}

for(;;)
	{
	renderhash(realx, tim);

	if(tim + GLOBALS->global_time_offset)
		{
		if(tim != GLOBALS->min_time)
			{
			reformat_time(timebuff, time_trunc(tim) + GLOBALS->global_time_offset, GLOBALS->time_dimension);
			}
			else
			{
			timebuff[0] = 0;
			}
		}
		else
		{
		strcpy(timebuff, "0");
		}

	lenhalf=font_engine_string_measure(GLOBALS->wavefont, timebuff) >> 1;

	if((x-lenhalf >= lastx) || (GLOBALS->pixelsperframe >= 200))
		{
		XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1,GLOBALS->wavefont,&GLOBALS->rgb_gc.gc_time_wavewindow_c_1,x-lenhalf+WAVE_CAIRO_050_OFFSET, GLOBALS->wavefont->ascent-1+WAVE_CAIRO_050_OFFSET,timebuff);
		lastx = x+lenhalf;
		}

	tim+=GLOBALS->nsperframe;
	x+=GLOBALS->pixelsperframe;
	realx+=GLOBALS->pixelsperframe;
	if((prevover)||(tim>GLOBALS->tims.last)) break;
	if(x>=GLOBALS->wavewidth) prevover=1;
	}
}

/***************************************************************************/

static void rendertimebar(void)
{
XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1, TRUE,0, -1, GLOBALS->wavewidth, GLOBALS->fontheight);
rendertimes();
rendertraces();

update_dual();
}


static void gc_save(Trptr t, struct wave_rgbmaster_t *gc_sav)
{
if((!GLOBALS->black_and_white) && (t->t_color))
	{
	int color = t->t_color;

	color--;

	memcpy(gc_sav, &GLOBALS->rgb_gc, sizeof(struct wave_rgbmaster_t));

	if(color < WAVE_NUM_RAINBOW)
		{
		XXX_set_alternate_gcs(GLOBALS->rgb_gc_rainbow[2*color], GLOBALS->rgb_gc_rainbow[2*color+1]);
		}
	}
}

static void gc_restore(Trptr t, struct wave_rgbmaster_t *gc_sav)
{
if((!GLOBALS->black_and_white) && (t->t_color))
	{
	memcpy(&GLOBALS->rgb_gc, gc_sav, sizeof(struct wave_rgbmaster_t));
	}
}


static void rendertraces(void)
{
struct wave_rgbmaster_t gc_sav;

if(!GLOBALS->topmost_trace)
	{
	GLOBALS->topmost_trace=GLOBALS->traces.first;
	}

if(GLOBALS->topmost_trace)
	{
	Trptr t = GLOBALS->topmost_trace;
	Trptr tback = t;
	hptr h;
	vptr v;
	int i = 0, num_traces_displayable;
	int iback = 0;

	GtkAllocation allocation;
	gtk_widget_get_allocation(GLOBALS->wavearea, &allocation);

	num_traces_displayable=allocation.height/(GLOBALS->fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	/* ensure that transaction traces are visible even if the topmost traces are blanks */
	while(tback)
		{
		if(tback->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))
			{
			tback = GivePrevTrace(tback);
			iback--;
			}
		else if(tback->flags & TR_TTRANSLATED)
			{
			if(tback != t)
				{
				t = tback;
				i = iback;
				}
			break;
			}
		else
			{
			break;
			}
		}

	for(;((i<num_traces_displayable)&&(t));i++)
		{
		if(!(t->flags&(TR_EXCLUDE|TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			GLOBALS->shift_timebase=t->shift;
			if(!t->vector)
				{
				h=bsearch_node(t->n.nd, GLOBALS->tims.start - t->shift);
				DEBUG(printf("Start time: "TTFormat", Histent time: "TTFormat"\n", GLOBALS->tims.start,(h->time+GLOBALS->shift_timebase)));

				if(!t->n.nd->extvals)
					{
					if(i>=0)
						{
						gc_save(t, &gc_sav);
						draw_hptr_trace(t,h,i,1,0);
						gc_restore(t, &gc_sav);
						}
					}
					else
					{
					if(i>=0)
						{
						gc_save(t, &gc_sav);
						draw_hptr_trace_vector(t,h,i);
						gc_restore(t, &gc_sav);
						}
					}
				}
				else
				{
				Trptr t_orig, tn;
				bvptr bv = t->n.vec;

				v=bsearch_vector(bv, GLOBALS->tims.start - t->shift);
				DEBUG(printf("Vector Trace: %s, %s\n", t->name, bv->bvname));
				DEBUG(printf("Start time: "TTFormat", Vectorent time: "TTFormat"\n", GLOBALS->tims.start,(v->time+GLOBALS->shift_timebase)));
				if(i>=0)
					{
					gc_save(t, &gc_sav);
					draw_vptr_trace(t,v,i);
					gc_restore(t, &gc_sav);
					}

				if((bv->transaction_chain) && (t->flags & TR_TTRANSLATED))
					{
					t_orig = t;
					for(;;)
						{
						tn = GiveNextTrace(t);
						bv = bv->transaction_chain;

						if(bv && tn && (tn->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
							{
							i++;
							if(i<num_traces_displayable)
								{
								v=bsearch_vector(bv, GLOBALS->tims.start - t->shift);
								if(i>=0)
									{
									gc_save(t, &gc_sav);
									draw_vptr_trace(t_orig,v,i);
									gc_restore(t, &gc_sav);
									}
								t = tn;
								continue;
								}
							}
						break;
						}
					}
				}
			}
			else
			{
			int kill_dodraw_grid = t->flags & TR_ANALOG_BLANK_STRETCH;

			if(kill_dodraw_grid)
				{
				Trptr tn = GiveNextTrace(t);
				if(!tn)
					{
					kill_dodraw_grid = 0;
					}
				else
				if(!(tn->flags & TR_ANALOG_BLANK_STRETCH))
					{
					kill_dodraw_grid = 0;
					}
				}

			if(i>=0)
				{
				gc_save(t, &gc_sav);
				draw_hptr_trace(NULL,NULL,i,0,kill_dodraw_grid);
				gc_restore(t, &gc_sav);
				}
			}
		t=GiveNextTrace(t);
/* bot:		1; */
		}
	}


draw_named_markers();
draw_marker_partitions();

if(GLOBALS->traces.dirty)
	{
	char dbuf[32];
	sprintf(dbuf, "%d", GLOBALS->traces.total);
	GLOBALS->traces.dirty = 0;
	gtkwavetcl_setvar(WAVE_TCLCB_TRACES_UPDATED, dbuf, WAVE_TCLCB_TRACES_UPDATED_FLAGS);
	}

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) gtk_widget_queue_draw(GLOBALS->wavearea);
#endif
}


/*
 * draw single traces and use this for rendering the grid lines
 * for "excluded" traces
 */
static void draw_hptr_trace(Trptr t, hptr h, int which, int dodraw, int kill_grid)
{
TimeType _x0, _x1, newtime;
int _y0, _y1, yu, liney, ytext;
TimeType tim, h2tim;
hptr h2, h3;
char hval, h2val, invert;
wave_rgb_t c;
wave_rgb_t gcx, gcxf;
char identifier_str[2];
int is_event = t && t->n.nd && (t->n.nd->vartype == ND_VCD_EVENT);

GLOBALS->tims.start-=GLOBALS->shift_timebase;
GLOBALS->tims.end-=GLOBALS->shift_timebase;

liney=((which+2)*GLOBALS->fontheight)-2;
if(((t)&&(t->flags&TR_INVERT))&&(!is_event))
	{
	_y0=((which+1)*GLOBALS->fontheight)+2;
	_y1=liney-2;
	invert=1;
	}
	else
	{
	_y1=((which+1)*GLOBALS->fontheight)+2;
	_y0=liney-2;
	invert=0;
	}
yu=(_y0+_y1)/2;
ytext=yu-(GLOBALS->wavefont->ascent/2)+GLOBALS->wavefont->ascent;

if((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) && (!GLOBALS->black_and_white) && (!kill_grid))
	{
	XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
		TRUE,0, liney - GLOBALS->fontheight,
		GLOBALS->wavewidth, GLOBALS->fontheight);
	}
	else
if((GLOBALS->display_grid)&&(GLOBALS->enable_horiz_grid)&&(!kill_grid))
	{
	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
		(GLOBALS->tims.start<GLOBALS->tims.first)?(GLOBALS->tims.first-GLOBALS->tims.start)*GLOBALS->pxns:0, liney,(GLOBALS->tims.last<=GLOBALS->tims.end)?(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns:GLOBALS->wavewidth-1, liney);
	}

if((h)&&(GLOBALS->tims.start==h->time))
	if (h->v.h_val != AN_Z) {

	switch(h->v.h_val)
		{
		case AN_X:	c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; break;
		case AN_U:	c = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; break;
		case AN_W:	c = GLOBALS->rgb_gc.gc_w_wavewindow_c_1; break;
		case AN_DASH:	c = GLOBALS->rgb_gc.gc_dash_wavewindow_c_1; break;
		default:	c = (h->v.h_val == AN_X) ? GLOBALS->rgb_gc.gc_x_wavewindow_c_1: GLOBALS->rgb_gc.gc_trans_wavewindow_c_1;
		}
	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, 0, _y0, 0, _y1);
	}

if(dodraw && t)
for(;;)
{
if(!h) break;
tim=(h->time);

if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x0<-1)
	{
	_x0=-1;
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1;
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x1<-1)
	{
	_x1=-1;
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}

if(_x0!=_x1)
	{
	if(is_event)
		{
		if(h->time >= GLOBALS->tims.first)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x0, _y0, _x0, _y1);
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x0, _y1, _x0+2, _y1+2);
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x0, _y1, _x0-2, _y1+2);
			}
		h=h->next;
		continue;
		}

	hval=h->v.h_val;
	h2val=h2->v.h_val;

	switch(h2val)
		{
		case AN_X:	c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; break;
		case AN_U:	c = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; break;
		case AN_W:	c = GLOBALS->rgb_gc.gc_w_wavewindow_c_1; break;
		case AN_DASH:	c = GLOBALS->rgb_gc.gc_dash_wavewindow_c_1; break;
		default:	c = (hval == AN_X) ? GLOBALS->rgb_gc.gc_x_wavewindow_c_1: GLOBALS->rgb_gc.gc_trans_wavewindow_c_1;
		}

	switch(hval)
		{
		case AN_0:	/* 0 */
		case AN_L:	/* L */
		if(GLOBALS->fill_waveform && invert)
		{
			switch(hval)
				{
				case AN_0: gcxf = GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1; break;
				case AN_L: gcxf = GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1; break;
				}
			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, gcxf, TRUE,_x0+1, _y0, _x1-_x0, _y1-_y0+1);
		}
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, (hval==AN_0) ? GLOBALS->rgb_gc.gc_0_wavewindow_c_1 : GLOBALS->rgb_gc.gc_low_wavewindow_c_1,_x0, _y0,_x1, _y0);

		if(h2tim<=GLOBALS->tims.end)
		switch(h2val)
			{
			case AN_0:
			case AN_L:	break;

			case AN_Z:	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, yu); break;
			default:	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1); break;
			}
		break;

		case AN_X: /* X */
		case AN_W: /* W */
		case AN_U: /* U */
		case AN_DASH: /* - */

		identifier_str[1] = 0;
		switch(hval)
			{
			case AN_X:	c = gcx = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; gcxf = GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1; identifier_str[0] = 0; break;
			case AN_W:	c = gcx = GLOBALS->rgb_gc.gc_w_wavewindow_c_1; gcxf = GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1; identifier_str[0] = 'W'; break;
			case AN_U:	c = gcx = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; gcxf = GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1; identifier_str[0] = 'U'; break;
			default:	c = gcx = GLOBALS->rgb_gc.gc_dash_wavewindow_c_1; gcxf = GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1; identifier_str[0] = '-'; break;
			}

		if(invert)
			{
			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, gcx, TRUE,_x0+1, _y0, _x1-_x0, _y1-_y0+1);
			}
			else
			{
			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, gcxf, TRUE,_x0+1, _y1, _x1-_x0, _y0-_y1+1);
			}

		if(identifier_str[0])
			{
			int _x0_new = (_x0>=0) ? _x0 : 0;
			int width;

			if((width=_x1-_x0_new)>GLOBALS->vector_padding)
				{
				if((_x1>=GLOBALS->wavewidth)||(font_engine_string_measure(GLOBALS->wavefont, identifier_str)+GLOBALS->vector_padding<=width))
					{
		                        XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1,GLOBALS->wavefont,  &GLOBALS->rgb_gc.gc_value_wavewindow_c_1,  _x0+2+WAVE_CAIRO_050_OFFSET,ytext+WAVE_CAIRO_050_OFFSET,identifier_str);
					}
				}
			}

		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gcx,_x0, _y0,_x1, _y0);
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gcx,_x0, _y1,_x1, _y1);
		if(h2tim<=GLOBALS->tims.end) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1);
		break;

		case AN_Z: /* Z */
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,_x0, yu,_x1, yu);
		if(h2tim<=GLOBALS->tims.end)
		switch(h2val)
			{
			case AN_0:
			case AN_L:
					XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, yu, _x1, _y0); break;
			case AN_1:
			case AN_H:
					XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, yu, _x1, _y1); break;
			default:	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1); break;
			}
		break;

		case AN_1: /* 1 */
		case AN_H: /* 1 */
		if(GLOBALS->fill_waveform && !invert)
		{
			switch(hval)
				{
				case AN_1: gcxf = GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1; break;
				case AN_H: gcxf = GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1; break;
				}
			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, gcxf, TRUE,_x0+1, _y1, _x1-_x0, _y0-_y1+1);
		}
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, (hval==AN_1) ? GLOBALS->rgb_gc.gc_1_wavewindow_c_1 : GLOBALS->rgb_gc.gc_high_wavewindow_c_1,_x0, _y1,_x1, _y1);
		if(h2tim<=GLOBALS->tims.end)
		switch(h2val)
			{
			case AN_1:
			case AN_H:	break;

			case AN_0:
			case AN_L:
					XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y1, _x1, _y0); break;
			case AN_Z:	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y1, _x1, yu); break;
			default:	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c, _x1, _y0, _x1, _y1); break;
			}
		break;

		default:
		break;
		}
	}
	else
	{
	if(!is_event)
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_trans_wavewindow_c_1, _x1, _y0, _x1, _y1);
		}
		else
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x1, _y0, _x1, _y1);
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x0, _y1, _x0+2, _y1+2);
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x0, _y1, _x0-2, _y1+2);
		}
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start/*+GLOBALS->shift_timebase*/;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		continue;
		}
	}

if((h->flags & HIST_GLITCH) && (GLOBALS->vcd_preserve_glitches))
        {
        XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,
                        TRUE,_x1-1, yu-1,
                        3, 3);
        }

h=h->next;
}

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_hptr_trace_vector_analog(Trptr t, hptr h, int which, int num_extension)
{
TimeType _x0, _x1, newtime;
int _y0, _y1, yu, liney, yt0, yt1;
TimeType tim, h2tim;
hptr h2, h3;
int endcnt = 0;
int type;
/* int lasttype=-1; */ /* scan-build */
wave_rgb_t c, ci;
wave_rgb_t cnan = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
wave_rgb_t cinf = GLOBALS->rgb_gc.gc_w_wavewindow_c_1;
wave_rgb_t cfixed;
double mynan = strtod("NaN", NULL);
double tmin = mynan, tmax = mynan, tv, tv2;
gint rmargin;
int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
int any_infs = 0, any_infp = 0, any_infm = 0;
int skipcnt = 0;

ci = GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1;

liney=((which+2+num_extension)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;
_y0=liney-2;
yu=(_y0+_y1)/2;

if(t->flags & TR_ANALOG_FULLSCALE) /* otherwise use dynamic */
	{
	if((!t->minmax_valid)||(t->d_num_ext != num_extension))
		{
		h3 = &t->n.nd->head;
		for(;;)
			{
			if(!h3) break;

			if((h3->time >= GLOBALS->tims.first) && (h3->time <= GLOBALS->tims.last))
				{
				tv = mynan;
				if(h3->flags&HIST_REAL)
					{
#ifdef WAVE_HAS_H_DOUBLE
					if(!(h3->flags&HIST_STRING)) tv = h3->v.h_double;
#else
					if(!(h3->flags&HIST_STRING) && h3->v.h_vector)
						tv = *(double *)h3->v.h_vector;
#endif
					}
					else
					{
					if(h3->time <= GLOBALS->tims.last) tv=convert_real_vec(t,h3->v.h_vector);
					}


				if (!isnan(tv) && !isinf(tv))
					{
					if (isnan(tmin) || tv < tmin)
						tmin = tv;
					if (isnan(tmax) || tv > tmax)
						tmax = tv;
					}
				else
				if(isinf(tv))
					{
					any_infs = 1;

					if(tv > 0)
						{
						any_infp = 1;
						}
						else
						{
						any_infm = 1;
						}
					}
				}
			h3 = h3->next;
			}

		if (isnan(tmin) || isnan(tmax))
			{
			tmin = tmax = 0;
			}

		if(any_infs)
			{
			double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

			if(any_infp) tmax = tmax + tdelta;
			if(any_infm) tmin = tmin - tdelta;
			}

		if ((tmax - tmin) < 1e-20)
			{
			tmax = 1;
			tmin -= 0.5 * (_y1 - _y0);
			}
			else
			{
			tmax = (_y1 - _y0) / (tmax - tmin);
			}

		t->minmax_valid = 1;
		t->d_minval = tmin;
		t->d_maxval = tmax;
		t->d_num_ext = num_extension;
		}
		else
		{
		tmin = t->d_minval;
		tmax = t->d_maxval;
		}
	}
	else
	{
	h3 = h;
	for(;;)
	{
	if(!h3) break;
	tim=(h3->time);
	if(tim>GLOBALS->tims.end) { endcnt++; if(endcnt==2) break; }
	if(tim>GLOBALS->tims.last) break;

	_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
	if((_x0>GLOBALS->wavewidth)&&(endcnt==2))
		{
		break;
		}

	tv = mynan;
	if(h3->flags&HIST_REAL)
		{
#ifdef WAVE_HAS_H_DOUBLE
		if(!(h3->flags&HIST_STRING)) tv = h3->v.h_double;
#else
		if(!(h3->flags&HIST_STRING) && h3->v.h_vector)
			tv = *(double *)h3->v.h_vector;
#endif
		}
		else
		{
		if(h3->time <= GLOBALS->tims.last) tv=convert_real_vec(t,h3->v.h_vector);
		}

	if (!isnan(tv) && !isinf(tv))
		{
		if (isnan(tmin) || tv < tmin)
			tmin = tv;
		if (isnan(tmax) || tv > tmax)
			tmax = tv;
		}
	else
	if(isinf(tv))
		{
		any_infs = 1;
		if(tv > 0)
			{
			any_infp = 1;
			}
			else
			{
			any_infm = 1;
			}
		}

	h3 = h3->next;
	}

	if (isnan(tmin) || isnan(tmax))
		tmin = tmax = 0;

	if(any_infs)
		{
		double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

		if(any_infp) tmax = tmax + tdelta;
		if(any_infm) tmin = tmin - tdelta;
		}

	if ((tmax - tmin) < 1e-20)
		{
		tmax = 1;
		tmin -= 0.5 * (_y1 - _y0);
		}
		else
		{
		tmax = (_y1 - _y0) / (tmax - tmin);
		}
	}

if(GLOBALS->tims.last - GLOBALS->tims.start < GLOBALS->wavewidth)
	{
	rmargin=(GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
	}
	else
	{
	rmargin = GLOBALS->wavewidth;
	}

/* now do the actual drawing */
h3 = NULL;
for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x0<-1)
	{
	_x0=-1;
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}
*/

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
/*	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1; */
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x1<-1)
	{
	_x1=-1;
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}
*/

/* draw trans */
type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.h_vector) : AN_COUNT;
tv = tv2 = mynan;

if(h->flags&HIST_REAL)
	{
#ifdef WAVE_HAS_H_DOUBLE
	if(!(h->flags&HIST_STRING)) tv = h->v.h_double;
#else
	if(!(h->flags&HIST_STRING) && h->v.h_vector)
		tv = *(double *)h->v.h_vector;
#endif
	}
	else
	{
	if(h->time <= GLOBALS->tims.last) tv=convert_real_vec(t,h->v.h_vector);
	}

if(h2->flags&HIST_REAL)
	{
#ifdef WAVE_HAS_H_DOUBLE
	if(!(h2->flags&HIST_STRING)) tv2 = h2->v.h_double;
#else
	if(!(h2->flags&HIST_STRING) && h2->v.h_vector)
		tv2 = *(double *)h2->v.h_vector;
#endif
	}
	else
	{
	if(h2->time <= GLOBALS->tims.last) tv2=convert_real_vec(t,h2->v.h_vector);
	}

if((is_inf = isinf(tv)))
	{
	if(tv < 0)
		{
		yt0 = _y0;
		}
		else
		{
		yt0 = _y1;
		}
	}
else
if((is_nan = isnan(tv)))
	{
	yt0 = yu;
	}
	else
	{
	yt0 = _y0 + (tv - tmin) * tmax;
	}

if((is_inf2 = isinf(tv2)))
	{
	if(tv2 < 0)
		{
		yt1 = _y0;
		}
		else
		{
		yt1 = _y1;
		}
	}
else
if((is_nan2 = isnan(tv2)))
	{
	yt1 = yu;
	}
	else
	{
	yt1 = _y0 + (tv2 - tmin) * tmax;
	}

if((_x0!=_x1)||(skipcnt < GLOBALS->analog_redraw_skip_count)) /* lower number = better performance */
	{
	if(_x0==_x1)
		{
		skipcnt++;
		}
		else
		{
		skipcnt = 0;
		}

	if(type != AN_X)
		{
		c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
		}
		else
		{
		c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
		}

	if(h->next)
		{
		if(h->next->time > GLOBALS->max_time)
			{
			yt1 = yt0;
			}
		}

	cfixed = is_inf ? cinf : c;
	if((is_nan2) && (h2tim > GLOBALS->max_time)) is_nan2 = 0;

/* clamp to top/bottom because of integer rounding errors */

if(yt0 < _y1) yt0 = _y1;
else if(yt0 > _y0) yt0 = _y0;

if(yt1 < _y1) yt1 = _y1;
else if(yt1 > _y0) yt1 = _y0;

/* clipping... */
{
int coords[4];
int rect[4];

if(_x0 < INT_MIN) { coords[0] = INT_MIN; }
else if(_x0 > INT_MAX) { coords[0] = INT_MAX; }
else { coords[0] = _x0; }

if(_x1 < INT_MIN) { coords[2] = INT_MIN; }
else if(_x1 > INT_MAX) { coords[2] = INT_MAX; }
else { coords[2] = _x1; }

coords[1] = yt0;
coords[3] = yt1;


rect[0] = -10;
rect[1] = _y1;
rect[2] = GLOBALS->wavewidth + 10;
rect[3] = _y0;

if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) != TR_ANALOG_STEP)
	{
	wave_lineclip(coords, rect);
	}
	else
	{
	if(coords[0] < rect[0]) coords[0] = rect[0];
	if(coords[2] < rect[0]) coords[2] = rect[0];

	if(coords[0] > rect[2]) coords[0] = rect[2];
	if(coords[2] > rect[2]) coords[2] = rect[2];

	if(coords[1] < rect[1]) coords[1] = rect[1];
	if(coords[3] < rect[1]) coords[3] = rect[1];

	if(coords[1] > rect[3]) coords[1] = rect[3];
	if(coords[3] > rect[3]) coords[3] = rect[3];
	}

_x0 = coords[0];
yt0 = coords[1];
_x1 = coords[2];
yt1 = coords[3];
}
/* ...clipping */

	if(is_nan || is_nan2)
		{
		if(is_nan)
			{
			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, cnan, TRUE, _x0, _y1, _x1-_x0, _y0-_y1);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1-1, yt1,_x1+1, yt1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1, yt1-1,_x1, yt1+1);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, _y0,_x0+1, _y0);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, _y0-1,_x0, _y0+1);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, _y1,_x0+1, _y1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, _y1-1,_x0, _y1+1);
				}
			}
		if(is_nan2)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0, _x1, yt0);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cnan,_x1, yt1,_x1, yt0);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1-1, _y0,_x1+1, _y0);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1, _y0-1,_x1, _y0+1);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1-1, _y1,_x1+1, _y1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1, _y1-1,_x1, _y1+1);
				}
			}
		}
	else
	if((t->flags & TR_ANALOG_INTERPOLATED) && !is_inf && !is_inf2)
		{
		if(t->flags & TR_ANALOG_STEP)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}

		if(rmargin != GLOBALS->wavewidth)	/* the window is clipped in postscript */
			{
			if((yt0==yt1)&&((_x0 > _x1)||(_x0 < 0)))
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,0, yt0,_x1,yt1);
				}
				else
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
				}
			}
			else
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
			}
		}
	else
	/* if(t->flags & TR_ANALOG_STEP) */
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1, yt0);

		if(is_inf2) cfixed = cinf;
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x1, yt0,_x1, yt1);

		if ((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}
		}
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start/*+GLOBALS->shift_timebase*/;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		/* lasttype=type; */ /* scan-build */
		continue;
		}
	}

h=h->next;
/* lasttype=type; */ /* scan-build */
}

}

/*
 * draw hptr vectors (integer+real)
 */
static void draw_hptr_trace_vector(Trptr t, hptr h, int which)
{
TimeType _x0, _x1, newtime, width;
int _y0, _y1, yu, liney, ytext;
TimeType tim /* , h2tim */; /* scan-build */
hptr h2, h3;
char *ascii=NULL;
int type;
int lasttype=-1;
wave_rgb_t c;

GLOBALS->tims.start-=GLOBALS->shift_timebase;
GLOBALS->tims.end-=GLOBALS->shift_timebase;

liney=((which+2)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;
_y0=liney-2;
yu=(_y0+_y1)/2;
ytext=yu-(GLOBALS->wavefont->ascent/2)+GLOBALS->wavefont->ascent;

if((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) && (!GLOBALS->black_and_white))
        {
	Trptr tn = GiveNextTrace(t);
	if((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH))
		{
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                        TRUE,0, liney - GLOBALS->fontheight,
                        GLOBALS->wavewidth, GLOBALS->fontheight);
		}
		else
                {
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                        TRUE,0, liney - GLOBALS->fontheight,
                        GLOBALS->wavewidth, GLOBALS->fontheight);
                }
	}
else
if((GLOBALS->display_grid)&&(GLOBALS->enable_horiz_grid))
	{
	Trptr tn = GiveNextTrace(t);
	if((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH))
		{
		}
		else
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
			(GLOBALS->tims.start<GLOBALS->tims.first)?(GLOBALS->tims.first-GLOBALS->tims.start)*GLOBALS->pxns:0, liney,(GLOBALS->tims.last<=GLOBALS->tims.end)?(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns:GLOBALS->wavewidth-1, liney);
		}
	}

if((t->flags & TR_ANALOGMASK) && (!(h->flags&HIST_STRING) || !(h->flags&HIST_REAL)))
	{
	Trptr te = GiveNextTrace(t);
	int ext = 0;

	while(te)
		{
		if(te->flags & TR_ANALOG_BLANK_STRETCH)
			{
			ext++;
			te = GiveNextTrace(te);
			}
			else
			{
			break;
			}
		}

 	if((ext) && (GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) && (!GLOBALS->black_and_white))
                {
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                        TRUE,0, liney,
                        GLOBALS->wavewidth, GLOBALS->fontheight * ext);
                }

	draw_hptr_trace_vector_analog(t, h, which, ext);
	GLOBALS->tims.start+=GLOBALS->shift_timebase;
	GLOBALS->tims.end+=GLOBALS->shift_timebase;
	return;
	}

GLOBALS->color_active_in_filter = 1;

for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x0<-1)
	{
	_x0=-1;
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
/* h2tim= */ tim=(h2->time); /* scan-build */
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1;
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x1<-1)
	{
	_x1=-1;
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}

/* draw trans */
if(!(h->flags&(HIST_REAL|HIST_STRING)))
        {
        type = vtype(t,h->v.h_vector);
        }
        else
        {
	/* s\000 ID is special "z" case */
	type = AN_COUNT;

        if(h->flags&HIST_STRING)
                {
		if(h->v.h_vector)
			{
			if(!h->v.h_vector[0])
				{
				type = AN_Z;
				}
			else
				{
				if(!strcmp(h->v.h_vector, "UNDEF"))
					{
					type = AN_X;
					}
				}
			}
			else
			{
			type = AN_X;
			}
                }
        }
/* type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.h_vector) : AN_COUNT; */
if(_x0>-1) {
wave_rgb_t gltype, gtype;

switch(lasttype)
	{
	case AN_X:	gltype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; break;
	case AN_U:	gltype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; break;
	default:	gltype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1; break;
	}
switch(type)
	{
	case AN_X:	gtype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; break;
	case AN_U:	gtype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; break;
	default:	gtype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1; break;
	}

if(GLOBALS->use_roundcaps)
	{

	if (type == AN_Z) {
		if (lasttype != -1) {
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0-1, _y0,_x0,   yu);
		if(lasttype != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0, yu,_x0-1, _y1);
		}
	} else
	if (lasttype==AN_Z) {
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0+1, _y0,_x0,   yu);
		if(    type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0, yu,_x0+1, _y1);
	} else {
		if (lasttype != type) {
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0-1, _y0,_x0,   yu);
		if(lasttype != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0, yu,_x0-1, _y1);
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0+1, _y0,_x0,   yu);
		if(    type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0, yu,_x0+1, _y1);
		} else {
	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0-2, _y0,_x0+2, _y1);
	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0+2, _y0,_x0-2, _y1);
		}
	}
	}
	else
	{
	XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0, _y0,_x0, _y1);
	}
}

if(_x0!=_x1)
	{
	if (type == AN_Z)
		{
		if(GLOBALS->use_roundcaps)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,_x0+1, yu,_x1-1, yu);
			}
			else
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,_x0, yu,_x1, yu);
			}
		}
		else
		{
		if((type != AN_X) && (type != AN_U))
			{
			c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
			}
			else
			{
			c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
			}

	if(GLOBALS->use_roundcaps)
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0+2, _y0,_x1-2, _y0);
		if(type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0+2, _y1,_x1-2, _y1);
		if(type == AN_1) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0+2, _y1+1,_x1-2, _y1+1);
		}
		else
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0, _y0,_x1, _y0);
		if(type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0, _y1,_x1, _y1);
		if(type == AN_1) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0, _y1+1,_x1, _y1+1);
		}

if(_x0<0) _x0=0;	/* fixup left margin */

	if((width=_x1-_x0)>GLOBALS->vector_padding)
		{
		char *ascii2;

		if(h->flags&HIST_REAL)
			{
			if(!(h->flags&HIST_STRING))
				{
#ifdef WAVE_HAS_H_DOUBLE
				ascii=convert_ascii_real(t, &h->v.h_double);
#else
				ascii=convert_ascii_real(t, (double *)h->v.h_vector);
#endif
				}
				else
				{
				ascii=convert_ascii_string((char *)h->v.h_vector);
				}
			}
			else
			{
			ascii=convert_ascii_vec(t,h->v.h_vector);
			}

		ascii2 = ascii;
		if(*ascii == '?')
			{
			wave_rgb_t cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = XXX_get_gc_from_name(ascii+1);
				if(cb.a != 0.0)
					{
					ascii2 =  srch_for_color + 1;
					if(memcmp(&GLOBALS->rgb_gc.gc_back_wavewindow_c_1, &GLOBALS->rgb_gc_white, sizeof(wave_rgb_t)))
						{
						if(!GLOBALS->black_and_white) XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, cb, TRUE, _x0+1, _y1+1, width-1, (_y0-1) - (_y1+1) + 1);
						}
					GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1 = 1;
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}

		if((_x1>=GLOBALS->wavewidth)||(font_engine_string_measure(GLOBALS->wavefont, ascii2)+GLOBALS->vector_padding<=width))
			{
			XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1,GLOBALS->wavefont,&GLOBALS->rgb_gc.gc_value_wavewindow_c_1,_x0+2+WAVE_CAIRO_050_OFFSET,ytext+WAVE_CAIRO_050_OFFSET,ascii2);
			}
		else
			{
			char *mod;

			mod=bsearch_trunc(ascii2,width-GLOBALS->vector_padding);
			if(mod)
				{
				*mod='+';
				*(mod+1)=0;

				XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1,GLOBALS->wavefont,&GLOBALS->rgb_gc.gc_value_wavewindow_c_1,_x0+2+WAVE_CAIRO_050_OFFSET,ytext+WAVE_CAIRO_050_OFFSET,ascii2);
				}
			}
		}
		else if(GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1)
		{
		/* char *ascii2; */ /* scan-build */

		if(h->flags&HIST_REAL)
			{
			if(!(h->flags&HIST_STRING))
				{
#ifdef WAVE_HAS_H_DOUBLE
				ascii=convert_ascii_real(t, &h->v.h_double);
#else
				ascii=convert_ascii_real(t, (double *)h->v.h_vector);
#endif
				}
				else
				{
				ascii=convert_ascii_string((char *)h->v.h_vector);
				}
			}
			else
			{
			ascii=convert_ascii_vec(t,h->v.h_vector);
			}

		/* ascii2 = ascii; */ /* scan-build */
		if(*ascii == '?')
			{
			wave_rgb_t cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = XXX_get_gc_from_name(ascii+1);
				if(cb.a != 0.0)
					{
					/* ascii2 =  srch_for_color + 1; */ /* scan-build */
					if(memcmp(&GLOBALS->rgb_gc.gc_back_wavewindow_c_1, &GLOBALS->rgb_gc_white, sizeof(wave_rgb_t)))
						{
						if(!GLOBALS->black_and_white) XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, cb, TRUE, _x0, _y1+1, width, (_y0-1) - (_y1+1) + 1);
						}
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}

		}
	    }
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start/*+GLOBALS->shift_timebase*/;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

if(ascii) { free_2(ascii); ascii=NULL; }
h=h->next;
lasttype=type;
}

GLOBALS->color_active_in_filter = 0;

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_vptr_trace_analog(Trptr t, vptr v, int which, int num_extension)
{
TimeType _x0, _x1, newtime;
int _y0, _y1, yu, liney, yt0, yt1;
TimeType tim, h2tim;
vptr h, h2, h3;
int endcnt = 0;
int type;
/* int lasttype=-1; */ /* scan-build */
wave_rgb_t c, ci;
wave_rgb_t cnan = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
wave_rgb_t cinf = GLOBALS->rgb_gc.gc_w_wavewindow_c_1;
wave_rgb_t cfixed;
double mynan = strtod("NaN", NULL);
double tmin = mynan, tmax = mynan, tv=0.0, tv2;
gint rmargin;
int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
int any_infs = 0, any_infp = 0, any_infm = 0;
int skipcnt = 0;

ci = GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1;

h=v;
liney=((which+2+num_extension)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;
_y0=liney-2;
yu=(_y0+_y1)/2;

if(t->flags & TR_ANALOG_FULLSCALE) /* otherwise use dynamic */
        {
	if((!t->minmax_valid)||(t->d_num_ext != num_extension))
                {
                h3 = t->n.vec->vectors[0];
                for(;;)
                        {
                        if(!h3) break;

                        if((h3->time >= GLOBALS->tims.first) && (h3->time <= GLOBALS->tims.last))
                                {
                                /* tv = mynan; */ /* scan-build */

				tv=convert_real(t,h3);
				if (!isnan(tv) && !isinf(tv))
					{
					if (isnan(tmin) || tv < tmin)
						tmin = tv;
					if (isnan(tmax) || tv > tmax)
						tmax = tv;
					}
				}
                                else
                                if(isinf(tv))
                                        {
                                        any_infs = 1;

                                        if(tv > 0)
                                                {
                                                any_infp = 1;
                                                }
                                                else
                                                {
                                                any_infm = 1;
                                                }
                                        }

			h3 = h3->next;
			}

		if (isnan(tmin) || isnan(tmax))
		tmin = tmax = 0;

                if(any_infs)
                        {
			double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

                        if(any_infp) tmax = tmax + tdelta;
                        if(any_infm) tmin = tmin - tdelta;
                        }

		if ((tmax - tmin) < 1e-20)
			{
			tmax = 1;
			tmin -= 0.5 * (_y1 - _y0);
			}
			else
			{
			tmax = (_y1 - _y0) / (tmax - tmin);
			}

                t->minmax_valid = 1;
                t->d_minval = tmin;
                t->d_maxval = tmax;
		t->d_num_ext = num_extension;
                }
                else
                {
                tmin = t->d_minval;
                tmax = t->d_maxval;
                }
	}
	else
	{
	h3 = h;
	for(;;)
	{
	if(!h3) break;
	tim=(h3->time);

	if(tim>GLOBALS->tims.end) { endcnt++; if(endcnt==2) break; }
	if(tim>GLOBALS->tims.last) break;

	_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
	if((_x0>GLOBALS->wavewidth)&&(endcnt==2))
	        {
	        break;
	        }

	tv=convert_real(t,h3);
	if (!isnan(tv) && !isinf(tv))
		{
		if (isnan(tmin) || tv < tmin)
			tmin = tv;
		if (isnan(tmax) || tv > tmax)
			tmax = tv;
		}
        else
        if(isinf(tv))
                {
                any_infs = 1;
                if(tv > 0)
                        {
                        any_infp = 1;
                        }
                        else
                        {
                        any_infm = 1;
                        }
                }

	h3 = h3->next;
	}
	if (isnan(tmin) || isnan(tmax))
		tmin = tmax = 0;

        if(any_infs)
                {
		double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

                if(any_infp) tmax = tmax + tdelta;
                if(any_infm) tmin = tmin - tdelta;
                }

	if ((tmax - tmin) < 1e-20)
		{
		tmax = 1;
		tmin -= 0.5 * (_y1 - _y0);
		}
		else
		{
		tmax = (_y1 - _y0) / (tmax - tmin);
		}
	}

if(GLOBALS->tims.last - GLOBALS->tims.start < GLOBALS->wavewidth)
	{
	rmargin=(GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
	}
	else
	{
	rmargin = GLOBALS->wavewidth;
	}

h3 = NULL;
for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x0<-1)
	{
	_x0=-1;
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}
*/

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
/*	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1; */
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;

/*
if(_x1<-1)
	{
	_x1=-1;
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}
*/
/* draw trans */
type = vtype2(t,h);
tv=convert_real(t,h);
tv2=convert_real(t,h2);

if((is_inf = isinf(tv)))
	{
	if(tv < 0)
		{
		yt0 = _y0;
		}
		else
		{
		yt0 = _y1;
		}
	}
else
if((is_nan = isnan(tv)))
	{
	yt0 = yu;
	}
	else
	{
	yt0 = _y0 + (tv - tmin) * tmax;
	}

if((is_inf2 = isinf(tv2)))
	{
	if(tv2 < 0)
		{
		yt1 = _y0;
		}
		else
		{
		yt1 = _y1;
		}
	}
else
if((is_nan2 = isnan(tv2)))
	{
	yt1 = yu;
	}
	else
	{
	yt1 = _y0 + (tv2 - tmin) * tmax;
	}

if((_x0!=_x1)||(skipcnt < GLOBALS->analog_redraw_skip_count)) /* lower number = better performance */
	{
        if(_x0==_x1)
                {
                skipcnt++;
                }
                else
                {
                skipcnt = 0;
                }

	if(type != AN_X)
		{
		c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
		}
		else
		{
		c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
		}

	if(h->next)
		{
		if(h->next->time > GLOBALS->max_time)
			{
			yt1 = yt0;
			}
		}

	cfixed = is_inf ? cinf : c;
	if((is_nan2) && (h2tim > GLOBALS->max_time)) is_nan2 = 0;

/* clamp to top/bottom because of integer rounding errors */

if(yt0 < _y1) yt0 = _y1;
else if(yt0 > _y0) yt0 = _y0;

if(yt1 < _y1) yt1 = _y1;
else if(yt1 > _y0) yt1 = _y0;

/* clipping... */
{
int coords[4];
int rect[4];

if(_x0 < INT_MIN) { coords[0] = INT_MIN; }
else if(_x0 > INT_MAX) { coords[0] = INT_MAX; }
else { coords[0] = _x0; }

if(_x1 < INT_MIN) { coords[2] = INT_MIN; }
else if(_x1 > INT_MAX) { coords[2] = INT_MAX; }
else { coords[2] = _x1; }

coords[1] = yt0;
coords[3] = yt1;


rect[0] = -10;
rect[1] = _y1;
rect[2] = GLOBALS->wavewidth + 10;
rect[3] = _y0;

if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) != TR_ANALOG_STEP)
	{
	wave_lineclip(coords, rect);
	}
	else
	{
	if(coords[0] < rect[0]) coords[0] = rect[0];
	if(coords[2] < rect[0]) coords[2] = rect[0];

	if(coords[0] > rect[2]) coords[0] = rect[2];
	if(coords[2] > rect[2]) coords[2] = rect[2];

	if(coords[1] < rect[1]) coords[1] = rect[1];
	if(coords[3] < rect[1]) coords[3] = rect[1];

	if(coords[1] > rect[3]) coords[1] = rect[3];
	if(coords[3] > rect[3]) coords[3] = rect[3];
	}

_x0 = coords[0];
yt0 = coords[1];
_x1 = coords[2];
yt1 = coords[3];
}
/* ...clipping */

        if(is_nan || is_nan2)
                {
		if(is_nan)
			{
			XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, cnan, TRUE, _x0, _y1, _x1-_x0, _y0-_y1);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1-1, yt1,_x1+1, yt1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1, yt1-1,_x1, yt1+1);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, _y0,_x0+1, _y0);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, _y0-1,_x0, _y0+1);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, _y1,_x0+1, _y1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, _y1-1,_x0, _y1+1);
				}
			}
		if(is_nan2)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0, _x1, yt0);

			if((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cnan,_x1, yt1,_x1, yt0);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1-1, _y0,_x1+1, _y0);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1, _y0-1,_x1, _y0+1);

				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1-1, _y1,_x1+1, _y1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x1, _y1-1,_x1, _y1+1);
				}
			}
                }
        else
	if((t->flags & TR_ANALOG_INTERPOLATED) && !is_inf && !is_inf2)
		{
		if(t->flags & TR_ANALOG_STEP)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}

		if(rmargin != GLOBALS->wavewidth)	/* the window is clipped in postscript */
			{
			if((yt0==yt1)&&((_x0 > _x1)||(_x0 < 0)))
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,0, yt0,_x1,yt1);
				}
				else
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
				}
			}
			else
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1,yt1);
			}
		}
	else
	/* if(t->flags & TR_ANALOG_STEP) */
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x0, yt0,_x1, yt0);

		if(is_inf2) cfixed = cinf;
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, cfixed,_x1, yt0,_x1, yt1);

		if ((t->flags & (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP)) == (TR_ANALOG_INTERPOLATED|TR_ANALOG_STEP))
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0-1, yt0,_x0+1, yt0);
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, ci,_x0, yt0-1,_x0, yt0+1);
			}
		}
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start/*+GLOBALS->shift_timebase*/;	/* skip to next pixel */
	h3=bsearch_vector(t->n.vec,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		/* lasttype=type; */
		continue;
		}
	}

h=h->next;
/* lasttype=type; */
}

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}

/*
 * draw vector traces
 */
static void draw_vptr_trace(Trptr t, vptr v, int which)
{
TimeType _x0, _x1, newtime, width;
int _y0, _y1, yu, liney, ytext;
TimeType tim /* , h2tim */; /* scan-build */
vptr h, h2, h3;
char *ascii=NULL;
int type;
int lasttype=-1;
wave_rgb_t c;

GLOBALS->tims.start-=GLOBALS->shift_timebase;
GLOBALS->tims.end-=GLOBALS->shift_timebase;

liney=((which+2)*GLOBALS->fontheight)-2;
_y1=((which+1)*GLOBALS->fontheight)+2;
_y0=liney-2;
yu=(_y0+_y1)/2;
ytext=yu-(GLOBALS->wavefont->ascent/2)+GLOBALS->wavefont->ascent;

if((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) && (!GLOBALS->black_and_white))
	{
	Trptr tn = GiveNextTrace(t);
	if((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH))
		{
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                        TRUE,0, liney - GLOBALS->fontheight,
                        GLOBALS->wavewidth, GLOBALS->fontheight);
                }
		else
                {
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                        TRUE,0, liney - GLOBALS->fontheight,
                        GLOBALS->wavewidth, GLOBALS->fontheight);
                }
	}
else
if((GLOBALS->display_grid)&&(GLOBALS->enable_horiz_grid))
	{
	Trptr tn = GiveNextTrace(t);
	if((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH))
		{
		}
		else
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
			(GLOBALS->tims.start<GLOBALS->tims.first)?(GLOBALS->tims.first-GLOBALS->tims.start)*GLOBALS->pxns:0, liney,(GLOBALS->tims.last<=GLOBALS->tims.end)?(GLOBALS->tims.last-GLOBALS->tims.start)*GLOBALS->pxns:GLOBALS->wavewidth-1, liney);
		}
	}

h = v;
/* obsolete:
if(t->flags & TR_TTRANSLATED)
	{
	traverse_vector_nodes(t);
	h=v=bsearch_vector(t->n.vec, GLOBALS->tims.start);
	}
	else
	{
	h=v;
	}
*/

if(t->flags & TR_ANALOGMASK)
	{
        Trptr te = GiveNextTrace(t);
        int ext = 0;

        while(te)
                {
                if(te->flags & TR_ANALOG_BLANK_STRETCH)
                        {
                        ext++;
                        te = GiveNextTrace(te);
                        }
		else
                        {
                        break;
                        }
                }

 	if((ext) && (GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) && (!GLOBALS->black_and_white))
                {
                XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                        TRUE,0, liney,
                        GLOBALS->wavewidth, GLOBALS->fontheight * ext);
                }

	draw_vptr_trace_analog(t, v, which, ext);

	GLOBALS->tims.start+=GLOBALS->shift_timebase;
	GLOBALS->tims.end+=GLOBALS->shift_timebase;
	return;
	}

GLOBALS->color_active_in_filter = 1;

for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>GLOBALS->tims.end)||(tim>GLOBALS->tims.last)) break;

_x0=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x0<-1)
	{
	_x0=-1;
	}
	else
if(_x0>GLOBALS->wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
/* h2tim= */ tim=(h2->time); /* scan-build */
if(tim>GLOBALS->tims.last) tim=GLOBALS->tims.last;
	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1;
_x1=(tim - GLOBALS->tims.start) * GLOBALS->pxns;
if(_x1<-1)
	{
	_x1=-1;
	}
	else
if(_x1>GLOBALS->wavewidth)
	{
	_x1=GLOBALS->wavewidth;
	}

/* draw trans */
type = vtype2(t,h);

if(_x0>-1) {
wave_rgb_t gltype, gtype;

switch(lasttype)
	{
	case AN_X:	gltype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; break;
	case AN_U:	gltype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; break;
	default:	gltype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1; break;
	}
switch(type)
	{
	case AN_X:	gtype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1; break;
	case AN_U:	gtype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1; break;
	default:	gtype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1; break;
	}

if(GLOBALS->use_roundcaps)
	{
	if (type == AN_Z)
		{
		if (lasttype != -1)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0-1, _y0,_x0,   yu);
			if(lasttype != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0, yu,_x0-1, _y1);
			}
		}
		else
		if (lasttype==AN_Z)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0+1, _y0,_x0,   yu);
			if(    type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0, yu,_x0+1, _y1);
			}
			else
			{
			if (lasttype != type)
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0-1, _y0,_x0,   yu);
				if(lasttype != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gltype,_x0, yu,_x0-1, _y1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0+1, _y0,_x0,   yu);
				if(    type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0, yu,_x0+1, _y1);
				}
				else
				{
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0-2, _y0,_x0+2, _y1);
				XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0+2, _y0,_x0-2, _y1);
				}
			}
		}
		else
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, gtype,_x0, _y0,_x0, _y1);
		}
}

if(_x0!=_x1)
	{
	if (type == AN_Z)
		{
		if(GLOBALS->use_roundcaps)
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,_x0+1, yu,_x1-1, yu);
			}
			else
			{
			XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,_x0, yu,_x1, yu);
			}
		}
		else
		{
		if((type != AN_X) && (type != AN_U))
			{
			c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
			}
			else
			{
			c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
			}

	if(GLOBALS->use_roundcaps)
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0+2, _y0,_x1-2, _y0);
		if(type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0+2, _y1,_x1-2, _y1);
		if(type == AN_1) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0+2, _y1+1,_x1-2, _y1+1);
		}
		else
		{
		XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0, _y0,_x1, _y0);
		if(type != AN_0) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0, _y1,_x1, _y1);
		if(type == AN_1) XXX_gdk_draw_line(GLOBALS->cr_wavepixmap_wavewindow_c_1, c,_x0, _y1+1,_x1, _y1+1);
		}


if(_x0<0) _x0=0;	/* fixup left margin */

	if((width=_x1-_x0)>GLOBALS->vector_padding)
		{
		char *ascii2;

		ascii=convert_ascii(t,h);

		ascii2 = ascii;
		if(*ascii == '?')
			{
			wave_rgb_t cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = XXX_get_gc_from_name(ascii+1);
				if(cb.a != 0.0)
					{
					ascii2 =  srch_for_color + 1;
					if(!GLOBALS->black_and_white) XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, cb, TRUE, _x0+1, _y1+1, width-1, (_y0-1) - (_y1+1) + 1);
					GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1 = 1;
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}

		if((_x1>=GLOBALS->wavewidth)||(font_engine_string_measure(GLOBALS->wavefont, ascii2)+GLOBALS->vector_padding<=width))
			{
			XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1,GLOBALS->wavefont,&GLOBALS->rgb_gc.gc_value_wavewindow_c_1,_x0+2,ytext,ascii2);
			}
		else
			{
			char *mod;

			mod=bsearch_trunc(ascii2,width-GLOBALS->vector_padding);
			if(mod)
				{
				*mod='+';
				*(mod+1)=0;

				XXX_font_engine_draw_string(GLOBALS->cr_wavepixmap_wavewindow_c_1,GLOBALS->wavefont,&GLOBALS->rgb_gc.gc_value_wavewindow_c_1,_x0+2,ytext,ascii2);
				}
			}

		}
		else if(GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1)
		{
		/* char *ascii2; */ /* scan-build */

		ascii=convert_ascii(t,h);

		/* ascii2 = ascii; */ /* scan-build */
		if(*ascii == '?')
			{
			wave_rgb_t cb;
			char *srch_for_color = strchr(ascii+1, '?');
			if(srch_for_color)
				{
				*srch_for_color = 0;
				cb = XXX_get_gc_from_name(ascii+1);
				if(cb.a != 0.0)
					{
					/* ascii2 =  srch_for_color + 1; */
					if(memcmp(&GLOBALS->rgb_gc.gc_back_wavewindow_c_1, &GLOBALS->rgb_gc_white, sizeof(wave_rgb_t)))
						{
						if(!GLOBALS->black_and_white) XXX_gdk_draw_rectangle(GLOBALS->cr_wavepixmap_wavewindow_c_1, cb, TRUE, _x0, _y1+1, width, (_y0-1) - (_y1+1) + 1);
						}
					}
					else
					{
					*srch_for_color = '?'; /* replace name as color is a miss */
					}
				}
			}
		}
	}
	}
	else
	{
	newtime=(((gdouble)(_x1+WAVE_OPT_SKIP))*GLOBALS->nspx)+GLOBALS->tims.start/*+GLOBALS->shift_timebase*/;	/* skip to next pixel */
	h3=bsearch_vector(t->n.vec,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

if(ascii) { free_2(ascii); ascii=NULL; }
lasttype=type;
h=h->next;
}

GLOBALS->color_active_in_filter = 0;

GLOBALS->tims.start+=GLOBALS->shift_timebase;
GLOBALS->tims.end+=GLOBALS->shift_timebase;
}
