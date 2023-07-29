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
#include "signal_list.h"
#include "wave_view.h"

#if !defined _ISOC99_SOURCE
#define _ISOC99_SOURCE 1
#endif
#include <math.h>

#define WAVE_CAIRO_050_OFFSET (GLOBALS->cairo_050_offset)

static void rendertimebar(cairo_t *cr);
static void rendertraces(cairo_t *cr);
static void rendertimes(cairo_t *cr);

static const GdkModifierType bmask[4] = {0,
                                         GDK_BUTTON1_MASK,
                                         0,
                                         GDK_BUTTON3_MASK}; /* button 1, 3 press/rel encodings */
#ifndef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
static const GdkEventMask m_bmask[4] = {0,
                                        GDK_BUTTON1_MOTION_MASK,
                                        0,
                                        GDK_BUTTON3_MOTION_MASK}; /* button 1, 3 motion encodings */
#endif

/******************************************************************/

/******************************************************************/
static int gesture_filter_set = 0; /* to prevent floods of gesture events before next draw */
static int gesture_filter_cnt = 0; /* to prevent floods of gesture events before next draw */
static int gesture_in_zoom = 0; /* for suppression of filtering of redraws: indicates zoom state
                                   change, service_hslider() decrements this */

#ifdef WAVE_ALLOW_SLIDER_ZOOM
static void (*draw_slider_p)(GtkStyle *style,
                             cairo_t *cr,
                             GtkStateType state_type,
                             GtkShadowType shadow_type,
                             GtkWidget *widget,
                             const gchar *detail,
                             gint x,
                             gint y,
                             gint width,
                             gint height,
                             GtkOrientation orientation) =
    NULL; /* This is intended to be global...only needed once per toolkit */

static void draw_slider(GtkStyle *style,
                        cairo_t *cr,
                        GtkStateType state_type,
                        GtkShadowType shadow_type,
                        GtkWidget *widget,
                        const gchar *detail,
                        gint x,
                        gint y,
                        gint width,
                        gint height,
                        GtkOrientation orientation)
{
    if ((GLOBALS) && (widget == GLOBALS->hscroll_wavewindow_c_2)) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        GLOBALS->str_wid_x = x - allocation.x;
        GLOBALS->str_wid_width = width;
        GLOBALS->str_wid_bigw = allocation.width;
        GLOBALS->str_wid_height = height;
    }

    draw_slider_p(style,
                  cr,
                  state_type,
                  shadow_type,
                  widget,
                  detail,
                  x,
                  y,
                  width,
                  height,
                  orientation);
}

static gint slider_bpr(GtkWidget *widget, GdkEventButton *event)
{
    (void)widget;

    int xi = event->x;
    int xl = GLOBALS->str_wid_x;
    int xr = GLOBALS->str_wid_x + GLOBALS->str_wid_width;

    if ((xi > (xr - 8)) && (xi < (xr + 8))) {
        GLOBALS->str_wid_state = 1;
        return (TRUE);
    } else if ((xi < (xl + 8)) && (xi > (xl - 8))) {
        GLOBALS->str_wid_state = -1;
        return (TRUE);
    }

    return (FALSE);
}

static gint slider_brr(GtkWidget *widget, GdkEventButton *event)
{
    (void)widget;
    (void)event;

    GLOBALS->str_wid_state = 0;
    return (FALSE);
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

    if (event->is_hint) {
        WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
    } else {
        x = event->x; /* scan-build */
        y = event->y; /* scan-build */
        state = event->state;
    }

    if ((GLOBALS->str_wid_state) && (!(state & (GDK_BUTTON1_MASK | GDK_BUTTON3_MASK)))) {
        GLOBALS->str_wid_state = 0;
    }

    if (GLOBALS->str_wid_state == 1) {
        my_x = event->x - GLOBALS->str_wid_height;
        xmax = GLOBALS->str_wid_bigw - (2 * GLOBALS->str_wid_height) - GLOBALS->str_wid_slider;
        if (xmax > 1.0) {
            ratio = my_x / xmax;
            r_margin =
                (gdouble)(GLOBALS->tims.last - GLOBALS->tims.first) * ratio + GLOBALS->tims.first;
            if ((r_margin > GLOBALS->tims.start) && (r_margin <= GLOBALS->tims.last)) {
                service_dragzoom(GLOBALS->tims.start, r_margin);
            }
            return (TRUE);
        }
    } else if (GLOBALS->str_wid_state == -1) {
        my_x = event->x - GLOBALS->str_wid_height;
        xmax = GLOBALS->str_wid_bigw - (2 * GLOBALS->str_wid_height) - GLOBALS->str_wid_slider;
        if (xmax > 1.0) {
            ratio = my_x / xmax;
            l_margin =
                (gdouble)(GLOBALS->tims.last - GLOBALS->tims.first) * ratio + GLOBALS->tims.first;
            r_margin = GLOBALS->tims.end;
            if ((l_margin >= GLOBALS->tims.first) && (l_margin < GLOBALS->tims.end)) {
                if (r_margin > GLOBALS->tims.last)
                    r_margin = GLOBALS->tims.last;
                service_dragzoom(l_margin, r_margin);
            }
            return (TRUE);
        }
    }

    return (FALSE);
}
#endif

/******************************************************************/

void XXX_gdk_draw_line(cairo_t *cr, wave_rgb_t gc, gint _x1, gint _y1, gint _x2, gint _y2)
{
    cairo_set_source_rgba(cr, gc.r, gc.g, gc.b, gc.a);
    cairo_move_to(cr, _x1 + WAVE_CAIRO_050_OFFSET, _y1 + WAVE_CAIRO_050_OFFSET);
    cairo_line_to(cr, _x2 + WAVE_CAIRO_050_OFFSET, _y2 + WAVE_CAIRO_050_OFFSET);
    cairo_stroke(cr);
}

void XXX_gdk_set_color(cairo_t *cr, wave_rgb_t gc)
{
    cairo_set_source_rgba(cr, gc.r, gc.g, gc.b, gc.a);
}

void XXX_gdk_draw_line2(cairo_t *cr, gint _x1, gint _y1, gint _x2, gint _y2)
{
    cairo_move_to(cr, _x1 + WAVE_CAIRO_050_OFFSET, _y1 + WAVE_CAIRO_050_OFFSET);
    cairo_line_to(cr, _x2 + WAVE_CAIRO_050_OFFSET, _y2 + WAVE_CAIRO_050_OFFSET);
}

void XXX_gdk_draw_rectangle(cairo_t *cr,
                            wave_rgb_t gc,
                            gboolean filled,
                            gint _x1,
                            gint _y1,
                            gint _w,
                            gint _h)
{
    cairo_set_source_rgba(cr, gc.r, gc.g, gc.b, gc.a);
    if (filled) {
        cairo_rectangle(cr, _x1, _y1, _w, _h);
        cairo_fill(cr);
    } else {
        cairo_rectangle(cr, _x1 + WAVE_CAIRO_050_OFFSET, _y1 + WAVE_CAIRO_050_OFFSET, _w, _h);
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

    if (!GLOBALS->cursor_snap)
        return (marker);

    /* potential snapping to a named marker time */
    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        if (GLOBALS->named_markers[i] != -1) {
            TimeType dlt;

            if ((GLOBALS->named_markers[i] >= GLOBALS->tims.start) &&
                (GLOBALS->named_markers[i] <= GLOBALS->tims.end) &&
                (GLOBALS->named_markers[i] <= GLOBALS->tims.last)) {
                if (marker < GLOBALS->named_markers[i]) {
                    dlt = GLOBALS->named_markers[i] - marker;
                } else {
                    dlt = marker - GLOBALS->named_markers[i];
                }

                if (dlt < closest_named) {
                    closest_named = dlt;
                    closest_which = i;
                }
            }
        }
    }

    GtkAllocation allocation;
    gtk_widget_get_allocation(GLOBALS->wavearea, &allocation);

    num_traces_displayable = allocation.height / (GLOBALS->fontheight);
    num_traces_displayable--; /* for the time trace that is always there */

    y -= GLOBALS->fontheight;
    if (y < 0)
        y = 0;
    y /= GLOBALS->fontheight; /* y now indicates the trace in question */
    if (y > num_traces_displayable)
        y = num_traces_displayable;

    t = gw_signal_list_get_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), 0);
    for (i = 0; i < y; i++) {
        if (!t)
            goto bot;
        t = GiveNextTrace(t);
    }

    if (!t)
        goto bot;
    if ((t->flags & (/*TR_BLANK|*/ TR_EXCLUDE))) /* TR_BLANK removed because of transaction handling
                                                    below... */
    {
        t = NULL;
        goto bot;
    }

    if (t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) /* seek to real xact trace if present... */
    {
        Trptr tscan = t;
        int bcnt = 0;
        while ((tscan) && (tscan = GivePrevTrace(tscan))) {
            if (!(tscan->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                if (tscan->flags & TR_TTRANSLATED) {
                    break; /* found it */
                } else {
                    tscan = NULL;
                }
            } else {
                bcnt++; /* bcnt is number of blank traces */
            }
        }

        if ((tscan) && (tscan->vector)) {
            bvptr bv = tscan->n.vec;
            do {
                bv = bv->transaction_chain; /* correlate to blank trace */
            } while (bv && (bcnt--));
            if (bv) {
                memcpy(&t_trans, tscan, sizeof(TraceEnt)); /* substitute into a synthetic trace */
                t_trans.n.vec = bv;
                t_trans.vector = 1;

                t_trans.name = bv->bvname;
                if (GLOBALS->hier_max_level)
                    t_trans.name = hier_extract(t_trans.name, GLOBALS->hier_max_level);

                t = &t_trans;
                goto process_trace;
            }
        }
    }

    if ((t->flags & TR_BLANK)) {
        t = NULL;
        goto bot;
    }

    if (t->flags & TR_ANALOG_BLANK_STRETCH) /* seek to real analog trace if present... */
    {
        while ((t) && (t = GivePrevTrace(t))) {
            if (!(t->flags & TR_ANALOG_BLANK_STRETCH)) {
                if (t->flags & TR_ANALOGMASK) {
                    break; /* found it */
                } else {
                    t = NULL;
                }
            }
        }
    }
    if (!t)
        goto bot;

process_trace:
    if (t->vector) {
        vptr v = bsearch_vector(t->n.vec, marker - t->shift);
        vptr v2 = v ? v->next : NULL;

        if ((!v) || (!v2))
            goto bot; /* should never happen */

        lft = v->time;
        rgh = v2->time;
    } else {
        hptr h = bsearch_node(t->n.nd, marker - t->shift);
        hptr h2 = h ? h->next : NULL;

        if ((!h) || (!h2))
            goto bot; /* should never happen */

        lft = h->time;
        rgh = h2->time;
    }

    lftinv = (lft < (GLOBALS->tims.start - t->shift)) || (lft >= (GLOBALS->tims.end - t->shift)) ||
             (lft >= (GLOBALS->tims.last - t->shift));
    rghinv = (rgh < (GLOBALS->tims.start - t->shift)) || (rgh >= (GLOBALS->tims.end - t->shift)) ||
             (rgh >= (GLOBALS->tims.last - t->shift));

    xlft = (lft + t->shift - GLOBALS->tims.start) * GLOBALS->pxns;
    xrgh = (rgh + t->shift - GLOBALS->tims.start) * GLOBALS->pxns;

    xlftd = xlft - x;
    if (xlftd < (gdouble)0.0)
        xlftd = ((gdouble)0.0) - xlftd;

    xrghd = xrgh - x;
    if (xrghd < (gdouble)0.0)
        xrghd = ((gdouble)0.0) - xrghd;

    if (xlftd <= xrghd) {
        if ((!lftinv) && (xlftd <= GLOBALS->cursor_snap)) {
            if (closest_which >= 0) {
                if ((closest_named * GLOBALS->pxns) < xlftd) {
                    marker = GLOBALS->named_markers[closest_which];
                    goto xit;
                }
            }

            marker = lft + t->shift;
            goto xit;
        }
    } else {
        if ((!rghinv) && (xrghd <= GLOBALS->cursor_snap)) {
            if (closest_which >= 0) {
                if ((closest_named * GLOBALS->pxns) < xrghd) {
                    marker = GLOBALS->named_markers[closest_which];
                    goto xit;
                }
            }

            marker = rgh + t->shift;
            goto xit;
        }
    }

bot:
    if (closest_which >= 0) {
        if ((closest_named * GLOBALS->pxns) <= GLOBALS->cursor_snap) {
            marker = GLOBALS->named_markers[closest_which];
        }
    }

xit:
    GLOBALS->mouseover_counter = -1;
    move_mouseover(t, xold, yold, marker);
    return (marker);
}

static void render_individual_named_marker(cairo_t *cr, int i, wave_rgb_t gc, int blackout)
{
    gdouble pixstep;
    gint xl;
    TimeType t;

    if ((t = GLOBALS->named_markers[i]) != -1) {
        if ((t >= GLOBALS->tims.start) && (t <= GLOBALS->tims.last) && (t <= GLOBALS->tims.end)) {
            /* this needs to be here rather than outside the loop as gcc does some
               optimizations that cause it to calculate slightly different from the marker if it's
               not here */
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);

            xl = ((gdouble)(t - GLOBALS->tims.start)) / pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                static const double dashed1[] = {5.0, 3.0};
                char nbuff[16];
                make_bijective_marker_id_string(nbuff, i);

                cairo_set_dash(cr, dashed1, sizeof(dashed1) / sizeof(dashed1[0]), 0);
                XXX_gdk_draw_line(cr, gc, xl, GLOBALS->fontheight - 1, xl, GLOBALS->waveheight - 1);
                cairo_set_dash(cr, dashed1, 0, 0);

                if ((!GLOBALS->marker_names[i]) || (!GLOBALS->marker_names[i][0])) {
                    XXX_font_engine_draw_string(
                        cr,
                        GLOBALS->wavefont_smaller,
                        &gc,
                        xl - (font_engine_string_measure(GLOBALS->wavefont_smaller, nbuff) >> 1) +
                            WAVE_CAIRO_050_OFFSET,
                        GLOBALS->fontheight - 2 + WAVE_CAIRO_050_OFFSET,
                        nbuff);
                } else {
                    int width = font_engine_string_measure(GLOBALS->wavefont_smaller,
                                                           GLOBALS->marker_names[i]);
                    if (blackout) /* blackout background so text is legible if overlaid with other
                                     marker labels */
                    {
                        XXX_gdk_draw_rectangle(
                            cr,
                            GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1,
                            TRUE,
                            xl - (width >> 1),
                            GLOBALS->fontheight - 2 - GLOBALS->wavefont_smaller->ascent,
                            width,
                            GLOBALS->wavefont_smaller->ascent + GLOBALS->wavefont_smaller->descent);
                    }

                    XXX_font_engine_draw_string(cr,
                                                GLOBALS->wavefont_smaller,
                                                &gc,
                                                xl - (width >> 1) + WAVE_CAIRO_050_OFFSET,
                                                GLOBALS->fontheight - 2 + WAVE_CAIRO_050_OFFSET,
                                                GLOBALS->marker_names[i]);
                }
            }
        }
    }
}

static void draw_named_markers(cairo_t *cr)
{
    int i;

    for (i = 0; i < WAVE_NUM_NAMED_MARKERS; i++) {
        if (i != GLOBALS->named_marker_lock_idx) {
            render_individual_named_marker(cr, i, GLOBALS->rgb_gc.gc_mark_wavewindow_c_1, 0);
        }
    }

    if (GLOBALS->named_marker_lock_idx >= 0) {
        render_individual_named_marker(cr,
                                       GLOBALS->named_marker_lock_idx,
                                       GLOBALS->rgb_gc.gc_umark_wavewindow_c_1,
                                       1);
    }
}

static void sync_marker(void)
{
    if ((GLOBALS->tims.prevmarker == -1) && (GLOBALS->tims.marker != -1)) {
        GLOBALS->signalwindow_width_dirty = 1;
    } else if ((GLOBALS->tims.marker == -1) && (GLOBALS->tims.prevmarker != -1)) {
        GLOBALS->signalwindow_width_dirty = 1;
    }
    GLOBALS->tims.prevmarker = GLOBALS->tims.marker;

    /* additional case for race conditions with MaxSignalLength */
    if (((GLOBALS->tims.resizemarker == -1) || (GLOBALS->tims.resizemarker2 == -1)) &&
        (GLOBALS->tims.resizemarker != GLOBALS->tims.resizemarker2)) {
        GLOBALS->signalwindow_width_dirty = 1;
    }
}

static void draw_marker_partitions(cairo_t *cr)
{
    (void)cr;

    g_printerr("draw_marker_partitions disabled\n");
    //     draw_marker();

    // #ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
    //     GdkDrawingContext *gdc;
    // #endif
    //     cairo_t *cr =
    //         XXX_gdk_cairo_create(XXX_GDK_DRAWABLE(gtk_widget_get_window(GLOBALS->wavearea)),
    //         &gdc);
    //     wavewindow_paint(cr);

    // #ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
    //     gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->wavearea), gdc);
    // #else
    //     cairo_destroy(cr);
    // #endif

    //     draw_marker();
}

static void renderblackout(cairo_t *cr)
{
    gfloat pageinc;
    TimeType lhs, rhs, lclip, rclip;
    struct blackout_region_t *bt = GLOBALS->blackout_regions;

    if (bt) {
        pageinc = (gfloat)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
        lhs = GLOBALS->tims.start;
        rhs = pageinc + lhs;

        while (bt) {
            if ((bt->bend < lhs) || (bt->bstart > rhs)) {
                /* nothing, out of bounds */
            } else {
                lclip = bt->bstart;
                rclip = bt->bend;

                if (lclip < lhs)
                    lclip = lhs;
                else if (lclip > rhs)
                    lclip = rhs;

                if (rclip < lhs)
                    rclip = lhs;

                lclip -= lhs;
                rclip -= lhs;
                if (rclip > ((GLOBALS->wavewidth + 1) * GLOBALS->nspx))
                    rclip = (GLOBALS->wavewidth + 1) * (GLOBALS->nspx);

                XXX_gdk_draw_rectangle(cr,
                                       GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1,
                                       TRUE,
                                       (((gdouble)lclip) * GLOBALS->pxns),
                                       GLOBALS->fontheight,
                                       (((gdouble)(rclip - lclip)) * GLOBALS->pxns),
                                       GLOBALS->waveheight - GLOBALS->fontheight);
            }

            bt = bt->next;
        }
    }
}

static void service_hslider(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    DEBUG(printf("Wave HSlider Moved\n"));

    if (GLOBALS->wavewidth < 1) {
        return;
    }

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    TimeType old_start = GLOBALS->tims.start;
#endif
    GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);

    if (!GLOBALS->tims.timecache) {
        GLOBALS->tims.start = time_trunc(gtk_adjustment_get_value(hadj));
    } else {
        GLOBALS->tims.start = time_trunc(GLOBALS->tims.timecache);
        GLOBALS->tims.timecache = 0; /* reset */
    }

    if (GLOBALS->tims.start < GLOBALS->tims.first)
        GLOBALS->tims.start = GLOBALS->tims.first;
    else if (GLOBALS->tims.start > GLOBALS->tims.last)
        GLOBALS->tims.start = GLOBALS->tims.last;

    GLOBALS->tims.laststart = GLOBALS->tims.start;

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    if ((gesture_in_zoom) || (!GLOBALS->swipe_init_time) ||
        (GLOBALS->wavearea_gesture_swipe_velocity_x == 0.0) ||
        (old_start != GLOBALS->tims.start)) /* cut down on redundant draws when swiping
                                             */
#endif
    {
        gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));
    }
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    if (gesture_in_zoom)
        gesture_in_zoom--;
#endif
}

#ifndef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
static
#endif
    void
    service_vslider(GtkAdjustment *sadj, gpointer data)
{
    (void)data;

    sync_marker();

    gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));

    GLOBALS->old_wvalue = gtk_adjustment_get_value(sadj);
}

void button_press_release_common(void)
{
    MaxSignalLength();

    {
        char signalwindow_width_dirty = GLOBALS->signalwindow_width_dirty;
        sync_marker();
        if (!signalwindow_width_dirty && GLOBALS->signalwindow_width_dirty) {
            MaxSignalLength_2(1);
        }
    }

    gw_signal_list_force_redraw(GW_SIGNAL_LIST(GLOBALS->signalarea));
}

static void button_motion_common(gint xin, gint yin, int pressrel, int is_button_2)
{
    gdouble x, offset, pixstep;
    TimeType newcurr;

    if (xin < 0)
        xin = 0;
    if (xin > (GLOBALS->wavewidth - 1))
        xin = (GLOBALS->wavewidth - 1);

    x = xin; /* for pix time calc */

    pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
    newcurr = (TimeType)(offset = ((gdouble)GLOBALS->tims.start) + (x * pixstep));

    if (offset - newcurr > 0.5) /* round to nearest integer ns */
    {
        newcurr++;
    }

    if (newcurr > GLOBALS->tims.last) /* sanity checking */
    {
        newcurr = GLOBALS->tims.last;
    }
    if (newcurr > GLOBALS->tims.end) {
        newcurr = GLOBALS->tims.end;
    }
    if (newcurr < GLOBALS->tims.start) {
        newcurr = GLOBALS->tims.start;
    }

    newcurr = time_trunc(newcurr);
    if (newcurr < 0)
        newcurr = GLOBALS->min_time; /* prevents marker from disappearing? */

    if (!is_button_2) {
        update_markertime(GLOBALS->tims.marker = cook_markertime(newcurr, xin, yin));
        if (GLOBALS->tims.lmbcache < 0)
            GLOBALS->tims.lmbcache = time_trunc(newcurr);

        // draw_marker_partitions(cr);

        if ((pressrel) || (GLOBALS->constant_marker_update)) {
            button_press_release_common();
        }
    } else {
        GLOBALS->tims.baseline = ((GLOBALS->tims.baseline < 0) || (is_button_2 < 0))
                                     ? cook_markertime(newcurr, xin, yin)
                                     : -1;
        update_basetime(GLOBALS->tims.baseline);
        update_markertime(GLOBALS->tims.marker);
        // gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));
    }

    gtk_widget_queue_draw(GLOBALS->wavearea);
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

    if (event->is_hint) {
        WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
        WAVE_GDK_GET_POINTER_COPY;
    } else {
        x = event->x;
        y = event->y;
        state = event->state;
    }

    do {
        scrolled = 0;
        if (state &
            bmask[GLOBALS->in_button_press_wavewindow_c_1]) /* needed for retargeting in AIX/X11 */
        {
            if (x < 0) {
                if (GLOBALS->wave_scrolling)
                    if (GLOBALS->tims.start > GLOBALS->tims.first) {
                        if (GLOBALS->nsperframe < 10) {
                            GLOBALS->tims.start -= GLOBALS->nsperframe;
                        } else {
                            GLOBALS->tims.start -= (GLOBALS->nsperframe / 10);
                        }
                        if (GLOBALS->tims.start < GLOBALS->tims.first)
                            GLOBALS->tims.start = GLOBALS->tims.first;
                        gtk_adjustment_set_value(
                            GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                            GLOBALS->tims.marker =
                                time_trunc(GLOBALS->tims.timecache = GLOBALS->tims.start));

                        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
                        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                              "value_changed");
                        scrolled = 1;
                    }
                x = 0;
            } else if (x > GLOBALS->wavewidth) {
                if (GLOBALS->wave_scrolling)
                    if (GLOBALS->tims.start != GLOBALS->tims.last) {
                        gfloat pageinc;

                        pageinc = (gfloat)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);

                        if (GLOBALS->nsperframe < 10) {
                            GLOBALS->tims.start += GLOBALS->nsperframe;
                        } else {
                            GLOBALS->tims.start += (GLOBALS->nsperframe / 10);
                        }

                        if (GLOBALS->tims.start > GLOBALS->tims.last - pageinc + 1)
                            GLOBALS->tims.start = time_trunc(GLOBALS->tims.last - pageinc + 1);
                        if (GLOBALS->tims.start < GLOBALS->tims.first)
                            GLOBALS->tims.start = GLOBALS->tims.first;

                        GLOBALS->tims.marker = time_trunc(GLOBALS->tims.start + pageinc);
                        if (GLOBALS->tims.marker > GLOBALS->tims.last)
                            GLOBALS->tims.marker = GLOBALS->tims.last;

                        gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                                 GLOBALS->tims.timecache = GLOBALS->tims.start);

                        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed");
                        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                              "value_changed");
                        scrolled = 1;
                    }
                x = GLOBALS->wavewidth - 1;
            }
        } else if ((state & GDK_BUTTON2_MASK) && (GLOBALS->tims.baseline >= 0)) {
            button_motion_common(x, y, 0, -1); /* neg one says don't clear tims.baseline */
        }

        pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
        newcurr = GLOBALS->tims.start + (offset = x * pixstep);
        if ((offset - ((int)offset)) > 0.5) /* round to nearest integer ns */
        {
            newcurr++;
        }

        if (newcurr > GLOBALS->tims.last)
            newcurr = GLOBALS->tims.last;

        if (newcurr != GLOBALS->prevtim_wavewindow_c_1) {
            update_currenttime(time_trunc(newcurr));
            GLOBALS->prevtim_wavewindow_c_1 = newcurr;
        }

        if (state & bmask[GLOBALS->in_button_press_wavewindow_c_1]) {
            button_motion_common(x, y, 0, 0);
        }

        /* warp selected signals if CTRL is pressed */
#ifdef MAC_INTEGRATION
        if ((event->state & GDK_MOD2_MASK) && (state & GDK_BUTTON1_MASK))
#else
        if ((event->state & GDK_CONTROL_MASK) && (state & GDK_BUTTON1_MASK))
#endif
        {
            int warp = 0;
            Trptr t = (GLOBALS->use_gestures > 0) ? NULL : GLOBALS->traces.first;
            TimeType gt, delta;

            while (t) {
                if (t->flags & TR_HIGHLIGHT) {
                    warp++;

                    if (!t->shift_drag_valid) {
                        t->shift_drag = t->shift;
                        t->shift_drag_valid = 1;
                    }

                    gt = t->shift_drag + (GLOBALS->tims.marker - GLOBALS->tims.lmbcache);

                    if (gt < 0) {
                        delta = GLOBALS->tims.first - GLOBALS->tims.last;
                        if (gt < delta)
                            gt = delta;
                    } else if (gt > 0) {
                        delta = GLOBALS->tims.last - GLOBALS->tims.first;
                        if (gt > delta)
                            gt = delta;
                    }
                    t->shift = gt;
                }

                t = t->t_next;
            }

            if (warp) {
                /* commented out to reduce on visual noise...

                                        GLOBALS->signalwindow_width_dirty = 1;
                                        MaxSignalLength(  );

                ...commented out to reduce on visual noise */

                gw_signal_list_force_redraw(GW_SIGNAL_LIST(GLOBALS->signalarea));
                gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));
            }
        }

        if (scrolled) /* make sure counters up top update.. */
        {
            gtk_events_pending_gtk_main_iteration();
        }

        WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
        WAVE_GDK_GET_POINTER_COPY;

    } while ((scrolled) && (state & bmask[GLOBALS->in_button_press_wavewindow_c_1]));

    return (TRUE);
}

static void alternate_y_scroll(int delta)
{
    printf("WARNING: alternalte_y_scroll disabled\n");
#if 0
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

g_signal_emit_by_name (wadj, "changed"); /* force bar update */
g_signal_emit_by_name (wadj, "value_changed"); /* force text update */
#endif
}

/*
 * Sane code starts here... :)
 * TomB 05Feb2012
 */

#define SANE_INCREMENT 0.25
/* don't want to increment a whole page thereby completely losing where I am... */

void alt_move_left(gboolean fine_scroll)
{
    TimeType ntinc, ntfrac;

    ntinc = (TimeType)(((gdouble)GLOBALS->wavewidth) *
                       GLOBALS->nspx); /* really don't need this var but the speed of ui code is
                                          human dependent.. */
    ntfrac = ntinc * GLOBALS->page_divisor * (SANE_INCREMENT / (fine_scroll ? 8.0 : 1.0));
    if (!ntfrac)
        ntfrac = 1;

    if ((GLOBALS->tims.start - ntfrac) > GLOBALS->tims.first)
        GLOBALS->tims.timecache = GLOBALS->tims.start - ntfrac;
    else
        GLOBALS->tims.timecache = GLOBALS->tims.first;

    gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache);
    time_update();

    DEBUG(printf("Alternate move left\n"));
}

void alt_move_right(gboolean fine_scroll)
{
    TimeType ntinc, ntfrac;

    ntinc = (TimeType)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
    ntfrac = ntinc * GLOBALS->page_divisor * (SANE_INCREMENT / (fine_scroll ? 8.0 : 1.0));
    if (!ntfrac)
        ntfrac = 1;

    if ((GLOBALS->tims.start + ntfrac) < (GLOBALS->tims.last - ntinc + 1)) {
        GLOBALS->tims.timecache = GLOBALS->tims.start + ntfrac;
    } else {
        GLOBALS->tims.timecache = GLOBALS->tims.last - ntinc + 1;
        if (GLOBALS->tims.timecache < GLOBALS->tims.first)
            GLOBALS->tims.timecache = GLOBALS->tims.first;
    }

    gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache);
    time_update();

    DEBUG(printf("Alternate move right\n"));
}

void alt_zoom_out(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    TimeType middle = 0, width;
    TimeType marker = GLOBALS->cached_currenttimeval_currenttime_c_1;
    /* Zoom on mouse cursor, not marker */

    if (GLOBALS->do_zoom_center) {
        if ((marker < 0) || (marker < GLOBALS->tims.first) || (marker > GLOBALS->tims.last)) {
            if (GLOBALS->tims.end > GLOBALS->tims.last)
                GLOBALS->tims.end = GLOBALS->tims.last;

            middle = (GLOBALS->tims.start / 2) + (GLOBALS->tims.end / 2);
            if ((GLOBALS->tims.start & 1) && (GLOBALS->tims.end & 1))
                middle++;
        } else {
            middle = marker;
        }
    }

    GLOBALS->tims.prevzoom = GLOBALS->tims.zoom;

#ifdef WAVE_CTRL_SCROLL_ZOOM_FACTOR
    GLOBALS->tims.zoom -= WAVE_CTRL_SCROLL_ZOOM_FACTOR;
#else
    GLOBALS->tims.zoom--;
#endif
    calczoom(GLOBALS->tims.zoom);

    if (GLOBALS->do_zoom_center) {
        width = (TimeType)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
        GLOBALS->tims.start = time_trunc(middle - (width / 2));
        if (GLOBALS->tims.start + width > GLOBALS->tims.last)
            GLOBALS->tims.start = time_trunc(GLOBALS->tims.last - width);

        if (GLOBALS->tims.start < GLOBALS->tims.first)
            GLOBALS->tims.start = GLOBALS->tims.first;

        gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                 GLOBALS->tims.timecache = GLOBALS->tims.start);
    } else {
        GLOBALS->tims.timecache = 0;
    }

    fix_wavehadj();

    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed"); /* force zoom update */
    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                          "value_changed"); /* force zoom update */

    DEBUG(printf("Alternate Zoom out\n"));
}

void alt_zoom_in(GtkWidget *text, gpointer data)
{
    (void)text;
    (void)data;

    if (GLOBALS->tims.zoom < 0) /* otherwise it's ridiculous and can cause */
    { /* overflow problems in the scope          */
        TimeType middle = 0, width;
        TimeType marker = GLOBALS->cached_currenttimeval_currenttime_c_1;
        /* Zoom on mouse cursor, not marker */

        if (GLOBALS->do_zoom_center) {
            if ((marker < 0) || (marker < GLOBALS->tims.first) || (marker > GLOBALS->tims.last)) {
                if (GLOBALS->tims.end > GLOBALS->tims.last)
                    GLOBALS->tims.end = GLOBALS->tims.last;

                middle = (GLOBALS->tims.start / 2) + (GLOBALS->tims.end / 2);
                if ((GLOBALS->tims.start & 1) && (GLOBALS->tims.end & 1))
                    middle++;
            } else {
                middle = marker;
            }
        }

        GLOBALS->tims.prevzoom = GLOBALS->tims.zoom;

#ifdef WAVE_CTRL_SCROLL_ZOOM_FACTOR
        GLOBALS->tims.zoom += WAVE_CTRL_SCROLL_ZOOM_FACTOR;
#else
        GLOBALS->tims.zoom++;
#endif
        calczoom(GLOBALS->tims.zoom);

        if (GLOBALS->do_zoom_center) {
            width = (TimeType)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
            GLOBALS->tims.start = time_trunc(middle - (width / 2));
            if (GLOBALS->tims.start + width > GLOBALS->tims.last)
                GLOBALS->tims.start = time_trunc(GLOBALS->tims.last - width);

            if (GLOBALS->tims.start < GLOBALS->tims.first)
                GLOBALS->tims.start = GLOBALS->tims.first;

            gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                     GLOBALS->tims.timecache = GLOBALS->tims.start);
        } else {
            GLOBALS->tims.timecache = 0;
        }

        fix_wavehadj();

        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                              "changed"); /* force zoom update */
        g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                              "value_changed"); /* force zoom update */

        DEBUG(printf("Alternate zoom in\n"));
    }
}

static gint scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
    (void)widget;

    GtkAllocation allocation;
    gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

    int num_traces_displayable = (allocation.height) / (GLOBALS->fontheight);
    num_traces_displayable--;

    DEBUG(printf("Mouse Scroll Event\n"));

    if (GLOBALS->alt_wheel_mode) {
        /* TomB mouse wheel handling */
#ifdef MAC_INTEGRATION
        if (event->state & GDK_MOD2_MASK)
#else
        if (event->state & GDK_CONTROL_MASK)
#endif
        {
            /* CTRL+wheel - zoom in/out around current mouse cursor position */
            if (event->direction == GDK_SCROLL_UP)
                alt_zoom_in(NULL, 0);
            else if (event->direction == GDK_SCROLL_DOWN)
                alt_zoom_out(NULL, 0);
        } else if (event->state & GDK_MOD1_MASK) {
            /* ALT+wheel - edge left/right mode */
            if (event->direction == GDK_SCROLL_UP)
                service_left_edge(NULL, 0);
            else if (event->direction == GDK_SCROLL_DOWN)
                service_right_edge(NULL, 0);
        } else {
            /* wheel alone - scroll part of a page along */
            if (event->direction == GDK_SCROLL_UP)
                alt_move_left((event->state & GDK_SHIFT_MASK) != 0); /* finer scroll if shift */
            else if (event->direction == GDK_SCROLL_DOWN)
                alt_move_right((event->state & GDK_SHIFT_MASK) != 0); /* finer scroll if shift */
        }
    } else {
        /* Original 3.3.31 mouse wheel handling */
        switch (event->direction) {
            case GDK_SCROLL_UP:
                if (GLOBALS->use_scrollwheel_as_y) {
                    if (event->state & GDK_SHIFT_MASK) {
                        alternate_y_scroll(-num_traces_displayable);
                    } else {
                        alternate_y_scroll(-1);
                    }
                } else {
#ifdef MAC_INTEGRATION
                    if (event->state & GDK_MOD2_MASK)
#else
                    if (event->state & GDK_CONTROL_MASK)
#endif
                        service_left_shift(NULL, 0);
                    else if (event->state & GDK_MOD1_MASK)
                        service_zoom_out(NULL, 0);
                    else
                        service_left_page(NULL, 0);
                }
                break;
            case GDK_SCROLL_DOWN:
                if (GLOBALS->use_scrollwheel_as_y) {
                    if (event->state & GDK_SHIFT_MASK) {
                        alternate_y_scroll(num_traces_displayable);
                    } else {
                        alternate_y_scroll(1);
                    }
                }

                {
#ifdef MAC_INTEGRATION
                    if (event->state & GDK_MOD2_MASK)
#else
                    if (event->state & GDK_CONTROL_MASK)
#endif
                        service_right_shift(NULL, 0);
                    else if (event->state & GDK_MOD1_MASK)
                        service_zoom_in(NULL, 0);
                    else
                        service_right_page(NULL, 0);
                }
                break;

            default:
                break;
        }
    }
    return (TRUE);
}

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
    if ((event->button == 1) ||
        ((event->button == 3) && (!GLOBALS->in_button_press_wavewindow_c_1))) {
        GLOBALS->in_button_press_wavewindow_c_1 = event->button;

        DEBUG(printf("Button Press Event\n"));
        GLOBALS->prev_markertime = GLOBALS->tims.marker;
        button_motion_common(event->x, event->y, 1, 0);
        GLOBALS->tims.timecache = GLOBALS->tims.start;

#ifdef WAVE_ALLOW_GTK3_SEAT_VS_POINTER_GRAB_UNGRAB
        GdkDisplay *display = gdk_display_get_default();
        GdkSeat *seat = gdk_display_get_default_seat(display);
        gdk_seat_grab(seat,
                      gtk_widget_get_window(widget),
                      GDK_SEAT_CAPABILITY_ALL_POINTING,
                      FALSE,
                      NULL,
                      (GLOBALS->use_gestures > 0)
                          ? NULL
                          : (GdkEvent *)event, /* GdkEvent is a union with type as 1st element */
                      NULL,
                      NULL);
#else
        gdk_pointer_grab(
            gtk_widget_get_window(widget),
            FALSE,
            m_bmask[GLOBALS->in_button_press_wavewindow_c_1] | /* key up on motion for button
                                                                  pressed ONLY */
                GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_RELEASE_MASK,
            NULL,
            NULL,
            event->time);
#endif

#ifdef MAC_INTEGRATION
        if ((event->state & GDK_MOD2_MASK) && (event->button == 1))
#else
        if ((event->state & GDK_CONTROL_MASK) && (event->button == 1))
#endif
        {
            Trptr t = (GLOBALS->use_gestures > 0) ? NULL : GLOBALS->traces.first;

            while (t) {
                if ((t->flags & TR_HIGHLIGHT) && (!t->shift_drag_valid)) {
                    t->shift_drag = t->shift; /* cache old value */
                    t->shift_drag_valid = 1;
                }
                t = t->t_next;
            }
        }
    } else if (event->button == 2) {
        if (!GLOBALS->button2_debounce_flag) {
            GLOBALS->button2_debounce_flag = 1; /* cleared by mouseover_timer() interrupt */
            button_motion_common(event->x, event->y, 1, 1);
        }
    }

    return (TRUE);
}

static gint button_release_event(GtkWidget *widget, GdkEventButton *event)
{
    (void)widget;

    if ((event->button) && (event->button == GLOBALS->in_button_press_wavewindow_c_1)) {
        GLOBALS->in_button_press_wavewindow_c_1 = 0;

        DEBUG(printf("Button Release Event\n"));
        button_motion_common(event->x, event->y, 1, 0);

        /* warp selected signals if CTRL is pressed */
        if (event->button == 1) {
            int warp = 0;
            Trptr t = (GLOBALS->use_gestures > 0) ? NULL : GLOBALS->traces.first;
#ifdef MAC_INTEGRATION
            if (event->state & GDK_MOD2_MASK)
#else
            if (event->state & GDK_CONTROL_MASK)
#endif
            {
                TimeType gt, delta;

                while (t) {
                    if (t->flags & TR_HIGHLIGHT) {
                        warp++;
                        gt = (t->shift_drag_valid ? t->shift_drag : t->shift) +
                             (GLOBALS->tims.marker - GLOBALS->tims.lmbcache);

                        if (gt < 0) {
                            delta = GLOBALS->tims.first - GLOBALS->tims.last;
                            if (gt < delta)
                                gt = delta;
                        } else if (gt > 0) {
                            delta = GLOBALS->tims.last - GLOBALS->tims.first;
                            if (gt > delta)
                                gt = delta;
                        }
                        t->shift = gt;

                        t->flags &= (~TR_HIGHLIGHT);
                    }

                    t->shift_drag_valid = 0;
                    t = t->t_next;
                }
            } else /* back out warp and keep highlighting */
            {
                while (t) {
                    if (t->shift_drag_valid) {
                        t->shift = t->shift_drag;
                        t->shift_drag_valid = 0;
                        warp++;
                    }
                    t = t->t_next;
                }
            }

            if (warp) {
                GLOBALS->signalwindow_width_dirty = 1;
                redraw_signals_and_waves();
            }
        }

        GLOBALS->tims.timecache = GLOBALS->tims.start;

        XXX_gdk_pointer_ungrab(event->time);

        if (event->button == 3) /* oh yeah, dragzoooooooom! */
        {
            service_dragzoom(GLOBALS->tims.lmbcache, GLOBALS->tims.marker);
        }

        GLOBALS->tims.lmbcache = -1;
        update_markertime(GLOBALS->tims.marker);
    }

    GLOBALS->mouseover_counter = 11;
    move_mouseover(NULL, 0, 0, LLDescriptor(0));
    GLOBALS->tims.timecache = 0;

    if (GLOBALS->prev_markertime == LLDescriptor(-1)) {
        gw_signal_list_force_redraw(GW_SIGNAL_LIST(GLOBALS->signalarea));
    }

    return (TRUE);
}

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT

static void wavearea_zero_out_swipe_velocity(void)
{
    GLOBALS->wavearea_gesture_swipe_velocity_x = 0.0;
    if (GLOBALS->swipe_init_time) {
        g_date_time_unref(GLOBALS->swipe_init_time);
        GLOBALS->swipe_init_time = NULL;
    }
}

void wavearea_pressed_event(GtkGestureMultiPress *gesture,
                            gint n_press,
                            gdouble x,
                            gdouble y,
                            gpointer user_data)
{
    (void)gesture;
    (void)n_press;
    (void)user_data;
    GdkEventButton ev;

    if (gesture_filter_cnt) {
        return;
    }

    wavearea_zero_out_swipe_velocity();

    memset(&ev, 0, sizeof(GdkEventButton));
    ev.button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    ev.x = x;
    ev.y = y;

    if ((n_press != 2) || (ev.button != 1)) {
        button_press_event(GLOBALS->wavearea, &ev);
    } else {
        delete_unnamed_marker(NULL, 0, NULL); /* double click gesture deletes primary marker */
    }
}

void wavearea_released_event(GtkGestureMultiPress *gesture,
                             gint n_press,
                             gdouble x,
                             gdouble y,
                             gpointer user_data)
{
    (void)gesture;
    (void)n_press;
    (void)user_data;
    GdkEventButton ev;

    memset(&ev, 0, sizeof(GdkEventButton));
    ev.button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    ev.x = x;
    ev.y = y;

    button_release_event(GLOBALS->wavearea, &ev);
}

void wavearea_long_pressed_event(GtkGestureMultiPress *gesture,
                                 gdouble x,
                                 gdouble y,
                                 gpointer user_data)
{
    (void)gesture;
    (void)user_data;
    GdkEventButton ev;

    wavearea_zero_out_swipe_velocity();

    memset(&ev, 0, sizeof(GdkEventButton));
    ev.button = 2;
    ev.x = x;
    ev.y = y;

    button_press_event(GLOBALS->wavearea, &ev);
}

void wavearea_drag_begin_event(GtkGestureDrag *gesture,
                               gdouble start_x,
                               gdouble start_y,
                               gpointer user_data)
{
    (void)gesture;
    (void)user_data;

    GLOBALS->wavearea_drag_start_x = start_x;
    GLOBALS->wavearea_drag_start_y = start_y;

    if (gesture_filter_cnt) {
        GLOBALS->wavearea_drag_start_x = GLOBALS->wavearea_drag_start_y = -1.0;
        return;
    }
}

void wavearea_drag_update_event(GtkGestureDrag *gesture,
                                gdouble offset_x,
                                gdouble offset_y,
                                gpointer user_data)
{
    (void)gesture;
    (void)user_data;
    GdkEventMotion ev;

    if (GLOBALS->wavearea_drag_start_x < 0.0)
        return;

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        if (gesture_filter_set)
            return; /* to prevent floods of drag update events in wayland */
        gesture_filter_set = 1;
    }
#endif

    memset(&ev, 0, sizeof(GdkEventMotion));
    ev.is_hint = 0;
#ifdef WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON
    ev.state = (gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) == 3)
                   ? GDK_BUTTON3_MASK
                   : GDK_BUTTON1_MASK;
#else
    ev.state = GDK_BUTTON1_MASK;
#endif
    ev.x = GLOBALS->wavearea_drag_start_x + offset_x;
    ev.y = GLOBALS->wavearea_drag_start_y + offset_y;
    ev.window = gtk_widget_get_window(GLOBALS->wavearea);

    GLOBALS->wavearea_drag_active = 1;

    motion_notify_event(GLOBALS->wavearea, &ev);

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
        gtk_widget_queue_draw(GLOBALS->wavearea);
#endif
}

void wavearea_drag_end_event(GtkGestureDrag *gesture,
                             gdouble offset_x,
                             gdouble offset_y,
                             gpointer user_data)
{
    wavearea_drag_update_event(gesture, offset_x, offset_y, user_data);
    GLOBALS->wavearea_drag_active = 0;
}
#endif

void make_sigarea_gcs(GtkWidget *signalarea)
{
    (void)signalarea;

    if (!GLOBALS->made_sgc_contexts_wavewindow_c_1) {
        gboolean dark = GLOBALS->use_dark;

        if (!dark) {
            g_object_get(gtk_settings_get_default(),
                         "gtk-application-prefer-dark-theme",
                         &dark,
                         NULL);
            GLOBALS->use_dark = dark;
        }

        GLOBALS->rgb_gc_white =
            dark ? XXX_alloc_color(GLOBALS->color_black) : XXX_alloc_color(GLOBALS->color_white);
        GLOBALS->rgb_gc_black =
            dark ? XXX_alloc_color(GLOBALS->color_white) : XXX_alloc_color(GLOBALS->color_black);
        GLOBALS->rgb_gc.gc_ltgray =
            dark ? XXX_alloc_color(GLOBALS->color_black) : XXX_alloc_color(GLOBALS->color_ltgray);
        GLOBALS->rgb_gc.gc_normal = XXX_alloc_color(GLOBALS->color_normal);
        GLOBALS->rgb_gc.gc_mdgray =
            dark ? XXX_alloc_color(GLOBALS->color_dkgray) : XXX_alloc_color(GLOBALS->color_mdgray);
        GLOBALS->rgb_gc.gc_dkgray =
            dark ? XXX_alloc_color(GLOBALS->color_white) : XXX_alloc_color(GLOBALS->color_dkgray);
        GLOBALS->rgb_gc.gc_dkblue = XXX_alloc_color(GLOBALS->color_dkblue);
        GLOBALS->rgb_gc.gc_brkred = XXX_alloc_color(GLOBALS->color_brkred);
        GLOBALS->rgb_gc.gc_ltblue = XXX_alloc_color(GLOBALS->color_ltblue);
        GLOBALS->rgb_gc.gc_gmstrd = XXX_alloc_color(GLOBALS->color_gmstrd);

        GLOBALS->made_sgc_contexts_wavewindow_c_1 = ~0;
    }
}

/*
 * screengrab vs normal rendering gcs...
 */
void force_screengrab_gcs(void)
{
    GLOBALS->black_and_white = 1;

    GLOBALS->rgb_gc.gc_ltgray = GLOBALS->rgb_gc_white;
    GLOBALS->rgb_gc.gc_normal = GLOBALS->rgb_gc_white;
    GLOBALS->rgb_gc.gc_mdgray = GLOBALS->rgb_gc_white;
    GLOBALS->rgb_gc.gc_dkgray = GLOBALS->rgb_gc_white;
    GLOBALS->rgb_gc.gc_dkblue = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_brkred = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_ltblue = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_gmstrd = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_back_wavewindow_c_1 = GLOBALS->rgb_gc_white;
    GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_grid_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_grid2_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_time_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1 = GLOBALS->rgb_gc_white;
    GLOBALS->rgb_gc.gc_value_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_low_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_high_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_trans_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_mid_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_x_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_mark_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_umark_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_0_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_1_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_u_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_w_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1 = GLOBALS->rgb_gc_black;
    GLOBALS->rgb_gc.gc_dash_wavewindow_c_1 = GLOBALS->rgb_gc_black;
}

void force_normal_gcs(void)
{
    GLOBALS->black_and_white = 0;

    memcpy(&GLOBALS->rgb_gc, &GLOBALS->rgb_gccache, sizeof(struct wave_rgbmaster_t));
}

#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
static gboolean wave_vslider_gtc(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
    if (GLOBALS && GLOBALS->wave_vslider_valid && GLOBALS->wave_vslider &&
        GLOBALS->signal_hslider) {
        GtkAdjustment *wadj = GTK_ADJUSTMENT(GLOBALS->wave_vslider);
        GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);

        gtk_adjustment_set_page_size(wadj, GLOBALS->wave_vslider_page_size);
        gtk_adjustment_set_page_increment(wadj, GLOBALS->wave_vslider_page_increment);
        gtk_adjustment_set_step_increment(wadj, GLOBALS->wave_vslider_step_increment);
        gtk_adjustment_set_lower(wadj, GLOBALS->wave_vslider_lower);
        gtk_adjustment_set_upper(wadj, GLOBALS->wave_vslider_upper);
        gtk_adjustment_set_value(wadj, GLOBALS->wave_vslider_value);

        GLOBALS->wave_vslider_valid = 0;

        g_signal_emit_by_name(wadj, "changed"); /* force bar update */
        g_signal_emit_by_name(wadj, "value_changed"); /* force text update */
        g_signal_emit_by_name(hadj, "changed"); /* force bar update */
    }

    return (G_SOURCE_CONTINUE);
}
#endif

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
static int wavearea_zoom_get_gesture_xy_points(GtkGesture *gesture,
                                               gdouble *x1p,
                                               gdouble *y1p,
                                               gdouble *x2p,
                                               gdouble *y2p)
{
    GList *sequences;
    int rc = 0;

    if (gesture) {
        sequences = gtk_gesture_get_sequences(gesture);
        if (sequences) {
            if (sequences->next) {
                rc = 1;
                gdouble x1, y1, x2, y2;

                gtk_gesture_get_point(gesture, sequences->data, &x1, &y1);
                gtk_gesture_get_point(gesture, sequences->next->data, &x2, &y2);

                if (x1 < x2) /* order coordinates so x1 is leftmost point */
                {
                    *x1p = x1;
                    *y1p = y1;
                    *x2p = x2;
                    *y2p = y2;
                } else {
                    *x1p = x2;
                    *y1p = y2;
                    *x2p = x1;
                    *y2p = y1;
                }
            }

            g_list_free(sequences);
        }
    }

    return (rc);
}
#endif

static void wavearea_zoom_begin_event(GtkGesture *gesture,
                                      GdkEventSequence *sequence,
                                      gpointer user_data)
{
    (void)sequence;
    (void)user_data;
    gdouble x1, y1, x2, y2;

    gesture_in_zoom = 1;
    GLOBALS->wavearea_gesture_initial_zoom = GLOBALS->tims.zoom;

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
    if (wavearea_zoom_get_gesture_xy_points(gesture, &x1, &y1, &x2, &y2)) {
        GLOBALS->wavearea_gesture_initial_x1tim = GLOBALS->tims.start + (x1 * GLOBALS->nspx);

        GLOBALS->wavearea_gesture_initial_zoom_x_distance = x2 - x1;
        if (GLOBALS->wavearea_gesture_initial_zoom_x_distance < 1.0)
            GLOBALS->wavearea_gesture_initial_zoom_x_distance =
                1.0; /* min resolution is one pixel */
    } else {
        TimeType middle = (GLOBALS->tims.start / 2) + (GLOBALS->tims.end / 2);
        GLOBALS->wavearea_gesture_initial_x1tim = middle;

        GLOBALS->wavearea_gesture_initial_zoom_x_distance = 1.0; /* min resolution is one pixel */
    }

    if (GLOBALS->wavearea_gesture_initial_x1tim < GLOBALS->tims.first)
        GLOBALS->wavearea_gesture_initial_x1tim = GLOBALS->tims.first;
    if (GLOBALS->wavearea_gesture_initial_x1tim > GLOBALS->tims.last)
        GLOBALS->wavearea_gesture_initial_x1tim = GLOBALS->tims.last;

#endif

#ifdef WAVE_GTK3_GESTURE_ZOOM_USES_GTK_PHASE_CAPTURE
    gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
#endif
}

static void wavearea_zoom_scale_changed_event(GtkGestureZoom *controller,
                                              gdouble scale,
                                              gpointer user_data)
{
    (void)controller;
    gdouble zb, ls, lzb, r, z0;
#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
    gdouble x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

    if (gesture_filter_set)
        return; /* to prevent floods of zoom events */
    gesture_filter_set = 1;
#endif

    gesture_in_zoom = 1;

    zb = GLOBALS->zoombase;
    lzb = log(zb);

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
    {
        GtkGesture *gesture = (GtkGesture *)user_data;
        gdouble dist;
        if (wavearea_zoom_get_gesture_xy_points(gesture, &x1, &y1, &x2, &y2)) {
            dist = x2 - x1;
            if (dist < 1.0)
                dist = 1.0; /* min resolution is one pixel */
        } else {
            dist = 1.0; /* min resolution is one pixel */
        }

        if (GLOBALS->wavearea_gesture_initial_zoom_x_distance < 1.0)
            GLOBALS->wavearea_gesture_initial_zoom_x_distance =
                1.0; /* min resolution is one pixel */
        scale = GLOBALS->wavearea_gesture_initial_zoom_x_distance / dist;
    }
#else
    {
        if (scale < 0.00001)
            scale = 0.00001;
        scale = (1.0 / scale); /* matches version for WAVE_GTK3_GESTURE_ZOOM_IS_1D which is
                                  initial_distance/distance instead of
                                  distance/priv->initial_distance from gtk */
    }
#endif

    ls = log(scale);

    if ((lzb != 0.0) && (scale > 0.0)) {
        r = ls / lzb;
        z0 = GLOBALS->wavearea_gesture_initial_zoom - r;
        if ((z0 <= 0.0) && (isnormal(z0))) {
            /* printf("XXX %e %e\n", scale, z0); */

#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
            {
                TimeType width, new_x1tim;

                calczoom(GLOBALS->tims.zoom = z0);
                width = (TimeType)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);
                GLOBALS->tims.start =
                    GLOBALS->wavearea_gesture_initial_x1tim - (x1 * GLOBALS->nspx);

                new_x1tim = GLOBALS->tims.start + (x1 * GLOBALS->nspx);
                GLOBALS->tims.start += (GLOBALS->wavearea_gesture_initial_x1tim - new_x1tim);

                GLOBALS->tims.marker = new_x1tim;
                GLOBALS->tims.baseline = GLOBALS->tims.start + (x2 * GLOBALS->nspx);
                GLOBALS->tims.lmbcache = -1;
                GLOBALS->in_button_press_wavewindow_c_1 = 0;

                if (GLOBALS->tims.start + width > GLOBALS->tims.last)
                    GLOBALS->tims.start = time_trunc(GLOBALS->tims.last - width);
                if (GLOBALS->tims.start < GLOBALS->tims.first)
                    GLOBALS->tims.start = GLOBALS->tims.first;
                gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                         GLOBALS->tims.timecache = GLOBALS->tims.start);

                fix_wavehadj();

                g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                      "changed"); /* force zoom update */
                g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                      "value_changed"); /* force zoom update */
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
static void wavearea_zoom_update_event(GtkGestureZoom *gesture,
                                       GdkEventSequence *sequence,
                                       gpointer user_data)
{
    (void)sequence;

    wavearea_zoom_scale_changed_event(gesture,
                                      gtk_gesture_zoom_get_scale_delta(gesture),
                                      user_data);
}
#endif

static void wavearea_zoom_end_event(GtkGestureZoom *gesture,
                                    GdkEventSequence *sequence,
                                    gpointer user_data)
{
    (void)gesture;
    (void)sequence;
    (void)user_data;

    GLOBALS->tims.marker = -1;
    GLOBALS->tims.baseline = -1;
    GLOBALS->tims.lmbcache = -1;
    GLOBALS->in_button_press_wavewindow_c_1 = 0;

    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider), "changed"); /* force zoom update */
    g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                          "value_changed"); /* force zoom update */

    gesture_in_zoom = 1;

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        /* nothing for wayland */
    } else
#endif
    {
        gesture_filter_cnt = 10; /* to prevent mouse pointer from flying all over at zoom release */
    }
}

void wavearea_swipe_event(GtkGestureSwipe *gesture,
                          gdouble velocity_x,
                          gdouble velocity_y,
                          gpointer user_data)
{
    (void)gesture;
    (void)user_data;
    (void)velocity_y;

    if (gesture_filter_cnt) {
        wavearea_zero_out_swipe_velocity();
        return;
    }

    GLOBALS->wavearea_gesture_swipe_velocity_x = velocity_x;

    if (GLOBALS->swipe_init_time) {
        g_date_time_unref(GLOBALS->swipe_init_time);
    }
    GLOBALS->swipe_init_time = g_date_time_new_now_utc();

    GLOBALS->swipe_init_start = GLOBALS->tims.start;
}

void wavearea_swipe_update_event(GtkGesture *gesture,
                                 GdkEventSequence *sequence,
                                 gpointer user_data)
{
    (void)gesture;
    (void)sequence;
    (void)user_data;
    gdouble velocity_x, velocity_y;

    gboolean rc = gtk_gesture_swipe_get_velocity(GTK_GESTURE_SWIPE(GLOBALS->wavearea_gesture_swipe),
                                                 &velocity_x,
                                                 &velocity_y);
    if (rc && !gesture_filter_cnt) {
        GLOBALS->wavearea_gesture_swipe_velocity_x = velocity_x;
    }
}

static gboolean wavearea_swipe_tick(GtkWidget *widget,
                                    GdkFrameClock *frame_clock,
                                    gpointer user_data)
{
    (void)widget;
    (void)frame_clock;
    (void)user_data;

    if (gesture_filter_cnt > 0) /* for X11 */
    {
        wavearea_zero_out_swipe_velocity();
        gesture_filter_cnt--;
        return (G_SOURCE_CONTINUE);
    }

    if (GLOBALS->swipe_init_time) {
        GDateTime *gd2 = g_date_time_new_now_utc();
        GTimeSpan elapsed = g_date_time_difference(gd2, GLOBALS->swipe_init_time);
        gdouble decaying = 0.0;
        g_date_time_unref(gd2);
        if (elapsed < WAVE_GTK3_SWIPE_MICROSECONDS) {
            decaying = exp(-elapsed / ((gdouble)WAVE_GTK3_SWIPE_TIME_CONSTANT));
        }

        if (GLOBALS->wavearea_gesture_swipe_velocity_x < -1.0) {
            GtkAdjustment *hadj;
            gfloat inc;
            TimeType ntinc, pageinc;

            if (!GLOBALS->wavearea_drag_active) {
                TimeType dest = GLOBALS->swipe_init_start -
                                (GLOBALS->wavearea_gesture_swipe_velocity_x * GLOBALS->nspx *
                                 WAVE_GTK3_SWIPE_VEL_VS_DIST_FACTOR) *
                                    (1.0 - decaying);
                /* gdouble vp = -GLOBALS->wavearea_gesture_swipe_velocity_x; */
                hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
                ntinc = inc = dest - gtk_adjustment_get_value(hadj);
                if (ntinc) {
                    pageinc = (TimeType)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);

                    if (dest <= (GLOBALS->tims.last - pageinc))
                        gtk_adjustment_set_value(hadj, GLOBALS->tims.timecache = dest);
                    else {
                        gtk_adjustment_set_value(hadj,
                                                 GLOBALS->tims.timecache =
                                                     GLOBALS->tims.last - pageinc);
                        elapsed = WAVE_GTK3_SWIPE_MICROSECONDS;
                    }

                    if (GLOBALS->tims.timecache < GLOBALS->tims.first)
                        GLOBALS->tims.timecache = GLOBALS->tims.first;

                    time_update();
                }
            }

            if (elapsed >= WAVE_GTK3_SWIPE_MICROSECONDS) {
                wavearea_zero_out_swipe_velocity();
                GLOBALS->tims.baseline = -1;
                GLOBALS->tims.lmbcache = -1;
                GLOBALS->in_button_press_wavewindow_c_1 = 0;
            }
        }

        if (GLOBALS->wavearea_gesture_swipe_velocity_x > 1.0) {
            GtkAdjustment *hadj;

            if (!GLOBALS->wavearea_drag_active) {
                TimeType dest = GLOBALS->swipe_init_start -
                                (GLOBALS->wavearea_gesture_swipe_velocity_x * GLOBALS->nspx *
                                 WAVE_GTK3_SWIPE_VEL_VS_DIST_FACTOR) *
                                    (1.0 - decaying);
                /* gdouble vp = GLOBALS->wavearea_gesture_swipe_velocity_x; */
                hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);

                if (dest >= GLOBALS->tims.first)
                    gtk_adjustment_set_value(hadj, dest);
                else {
                    gtk_adjustment_set_value(hadj, GLOBALS->tims.first);
                    ;
                    elapsed = WAVE_GTK3_SWIPE_MICROSECONDS;
                }

                if (dest >= GLOBALS->tims.first)
                    GLOBALS->tims.timecache = dest;
                else {
                    GLOBALS->tims.timecache = GLOBALS->tims.first;
                    elapsed = WAVE_GTK3_SWIPE_MICROSECONDS;
                }

                time_update();
            }

            if (elapsed >= WAVE_GTK3_SWIPE_MICROSECONDS) {
                wavearea_zero_out_swipe_velocity();
                GLOBALS->tims.baseline = -1;
                GLOBALS->tims.lmbcache = -1;
                GLOBALS->in_button_press_wavewindow_c_1 = 0;
            }
        }
    }

    GdkWindow *gw = gtk_widget_get_window(GLOBALS->wavearea);

    if (gw) {
        gdouble x = 0, pixstep, offset;
        TimeType newcurr = 0;
        GdkModifierType state = 0;
        gint xi = 0, yi = 0;

        int dummy_x, dummy_y;
        get_window_xypos(&dummy_x, &dummy_y);

        WAVE_GDK_GET_POINTER(gtk_widget_get_window(GLOBALS->wavearea), &x, &y, &xi, &yi, &state);
        WAVE_GDK_GET_POINTER_COPY_XONLY;

        if ((x >= 0) && (x < GLOBALS->wavewidth)) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            newcurr = GLOBALS->tims.start + (offset = x * pixstep);
            if ((offset - ((int)offset)) > 0.5) /* round to nearest integer ns */
            {
                newcurr++;
            }

            if (newcurr > GLOBALS->tims.last)
                newcurr = GLOBALS->tims.last;

            if (newcurr != GLOBALS->prevtim_wavewindow_c_1) {
                update_currenttime(time_trunc(newcurr));
                GLOBALS->prevtim_wavewindow_c_1 = newcurr;
            }
        }
    }

    return (G_SOURCE_CONTINUE);
}

#endif

GtkWidget *create_wavewindow(void)
{
#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_DEPRECATED_API
    /* this removes the warning generated from XXX_gtk_table_attach() on
     * GLOBALS->vscroll_wavewindow_c_1 below */
    gtk_container_set_resize_mode(GTK_CONTAINER(table), GTK_RESIZE_IMMEDIATE);
#endif

    GLOBALS->wavearea = gw_wave_view_new();
    gtk_widget_set_hexpand(GLOBALS->wavearea, TRUE);
    gtk_widget_set_vexpand(GLOBALS->wavearea, TRUE);

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    if (GLOBALS->use_gestures < 0) /* <0 means "maybe (if available, enable)" */
    {
        GdkDisplay *display = gdk_display_get_default();
        GdkSeat *seat = gdk_display_get_default_seat(display);
        GdkSeatCapabilities gsc = gdk_seat_get_capabilities(seat);
        if (gsc & GDK_SEAT_CAPABILITY_TOUCH) {
            printf("GTKWAVE | Touch screen detected, enabling gestures.\n");
            GLOBALS->use_gestures = 1;
        } else {
            GLOBALS->use_gestures = 0;
        }
    }

    if (GLOBALS->use_gestures) {
        /* so far is mutually exclusive with existing motion/button action below */
        GtkGesture *gs;

        GLOBALS->wavearea_gesture_initial_zoom = GLOBALS->tims.zoom;
        gs = gtk_gesture_zoom_new(GLOBALS->wavearea);
        gtkwave_signal_connect(gs, "begin", G_CALLBACK(wavearea_zoom_begin_event), gs);
        gtkwave_signal_connect(gs, "end", G_CALLBACK(wavearea_zoom_end_event), gs);
#ifdef WAVE_GTK3_GESTURE_ZOOM_USES_GTK_PHASE_CAPTURE
        gtkwave_signal_connect(gs, "update", G_CALLBACK(wavearea_zoom_update_event), gs);
        gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(gs), GTK_PHASE_CAPTURE);
#else
        gtkwave_signal_connect(gs,
                               "scale-changed",
                               G_CALLBACK(wavearea_zoom_scale_changed_event),
                               gs);
#endif

        gs = gtk_gesture_multi_press_new(GLOBALS->wavearea);
        gtkwave_signal_connect(gs, "pressed", G_CALLBACK(wavearea_pressed_event), NULL);
        gtkwave_signal_connect(gs, "released", G_CALLBACK(wavearea_released_event), NULL);
#ifdef WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gs), 0);
#endif

        gs = gtk_gesture_long_press_new(GLOBALS->wavearea);
        gtkwave_signal_connect(gs, "pressed", G_CALLBACK(wavearea_long_pressed_event), NULL);

        gs = gtk_gesture_drag_new(GLOBALS->wavearea);
        gtkwave_signal_connect(gs, "drag_begin", G_CALLBACK(wavearea_drag_begin_event), NULL);
        gtkwave_signal_connect(gs, "drag_update", G_CALLBACK(wavearea_drag_update_event), NULL);
        gtkwave_signal_connect(gs, "drag_end", G_CALLBACK(wavearea_drag_end_event), NULL);
#ifdef WAVE_ALLOW_GTK3_GESTURE_MIDDLE_RIGHT_BUTTON
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gs), 0);
#endif

        GLOBALS->wavearea_gesture_swipe = gtk_gesture_swipe_new(GLOBALS->wavearea);
        gtkwave_signal_connect(GLOBALS->wavearea_gesture_swipe,
                               "swipe",
                               G_CALLBACK(wavearea_swipe_event),
                               GLOBALS);
        gtkwave_signal_connect(GLOBALS->wavearea_gesture_swipe,
                               "update",
                               G_CALLBACK(wavearea_swipe_update_event),
                               GLOBALS);
        gtk_widget_add_tick_callback(GTK_WIDGET(GLOBALS->wavearea),
                                     wavearea_swipe_tick,
                                     NULL,
                                     NULL);
    } else
#endif
    {
        gtkwave_signal_connect(GLOBALS->wavearea,
                               "motion_notify_event",
                               G_CALLBACK(motion_notify_event),
                               NULL);
        gtkwave_signal_connect(GLOBALS->wavearea,
                               "button_press_event",
                               G_CALLBACK(button_press_event),
                               NULL);
        gtkwave_signal_connect(GLOBALS->wavearea,
                               "button_release_event",
                               G_CALLBACK(button_release_event),
                               NULL);
    }

    gtkwave_signal_connect(GLOBALS->wavearea, "scroll_event", G_CALLBACK(scroll_event), NULL);
    gtk_widget_set_can_focus(GTK_WIDGET(GLOBALS->wavearea), TRUE);

    GtkAdjustment *vadj = gw_signal_list_get_vadjustment(GW_SIGNAL_LIST(GLOBALS->signalarea));
    gtkwave_signal_connect(vadj, "value_changed", G_CALLBACK(service_vslider), NULL);
    GLOBALS->vscroll_wavewindow_c_1 = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, vadj);

/* this moves GLOBALS->wave_vslider updates out from signalarea_configure_event where it was causing
 * size allocation warnings */
#ifdef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
    gtk_widget_add_tick_callback(GTK_WIDGET(GLOBALS->vscroll_wavewindow_c_1),
                                 wave_vslider_gtc,
                                 NULL,
                                 NULL);
#endif

    GLOBALS->wave_hslider = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
    gtkwave_signal_connect(GLOBALS->wave_hslider,
                           "value_changed",
                           G_CALLBACK(service_hslider),
                           NULL);
    GLOBALS->hscroll_wavewindow_c_2 = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, hadj);

#ifdef WAVE_ALLOW_SLIDER_ZOOM
    if (GLOBALS->enable_slider_zoom) {
        GValue gvalue;

        if (!draw_slider_p) {
            GtkStyle *gs = gtk_widget_get_style(GLOBALS->hscroll_wavewindow_c_2);
            draw_slider_p = GTK_STYLE_GET_CLASS(gs)->draw_slider;
            GTK_STYLE_GET_CLASS(gs)->draw_slider = draw_slider;
        }

        memset(&gvalue, 0, sizeof(GValue));
        g_value_init(&gvalue, G_TYPE_INT);
        gtk_widget_style_get_property(GLOBALS->hscroll_wavewindow_c_2,
                                      "min-slider-length",
                                      &gvalue);

        gtkwave_signal_connect(GLOBALS->hscroll_wavewindow_c_2,
                               "button_press_event",
                               G_CALLBACK(slider_bpr),
                               NULL);
        gtkwave_signal_connect(GLOBALS->hscroll_wavewindow_c_2,
                               "button_release_event",
                               G_CALLBACK(slider_brr),
                               NULL);
        gtkwave_signal_connect(GLOBALS->hscroll_wavewindow_c_2,
                               "motion_notify_event",
                               G_CALLBACK(slider_mnr),
                               NULL);
    }
#endif

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), GLOBALS->wavearea, 0, 0, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            GLOBALS->vscroll_wavewindow_c_1,
                            GLOBALS->wavearea,
                            GTK_POS_RIGHT,
                            1,
                            1);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            GLOBALS->hscroll_wavewindow_c_2,
                            GLOBALS->wavearea,
                            GTK_POS_BOTTOM,
                            1,
                            1);

    gtk_widget_show_all(grid);

    return grid;
}

/**********************************************/

void populateBuffer(Trptr t, char *altname, char *buf)
{
    char *ptr = buf;
    char *tname = altname ? altname : t->name;

    if (HasWave(t)) {
        if (tname) {
            strcpy(ptr, tname);
            ptr = ptr + strlen(ptr);

            if ((tname) && (t->shift)) {
                ptr[0] = '`';
                reformat_time(ptr + 1, t->shift, GLOBALS->time_dimension);
                ptr = ptr + strlen(ptr + 1) + 1;
                strcpy(ptr, "\'");
#ifdef WAVE_ARRAY_SUPPORT
                ptr = ptr + strlen(ptr); /* really needed for aet2 only */
#endif
            }

#ifdef WAVE_ARRAY_SUPPORT
            if ((!t->vector) && (t->n.nd) && (t->n.nd->array_height)) {
                sprintf(ptr, "{%d}", t->n.nd->this_row);
                /* ptr = ptr + strlen(ptr); */ /* scan-build */
            }
#endif
        }

        if (IsGroupBegin(t)) {
            char *pch;
            ptr = buf;
            if (IsClosed(t)) {
                pch = strstr(ptr, "[-]");
                if (pch) {
                    memcpy(pch, "[+]", 3);
                }
            } else {
                pch = strstr(ptr, "[+]");
                if (pch) {
                    memcpy(pch, "[-]", 3);
                }
            }
        }
    } else {
        if (tname) {
            if (IsGroupEnd(t)) {
                strcpy(ptr, "} ");
                ptr = ptr + strlen(ptr);
            }

            strcpy(ptr, tname);
            ptr = ptr + strlen(ptr);

            if (IsGroupBegin(t)) {
                if (IsClosed(t) && IsCollapsed(t->t_match)) {
                    strcpy(ptr, " {}");
                } else {
                    strcpy(ptr, " {");
                }
                /* ptr = ptr + strlen(ptr); */ /* scan-build */
            }
        }
    }

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
        gtk_widget_queue_draw(GLOBALS->signalarea);
#endif
}

/***************************************************************************/

void MaxSignalLength(void)
{
    Trptr t;
    int len = 0, maxlen = 0;
    int vlen = 0, vmaxlen = 0;
    char buf[2048];
    char dirty_kick;
    bvptr bv;
    Trptr tscan;

    DEBUG(printf("signalwindow_width_dirty: %d\n", GLOBALS->signalwindow_width_dirty));

    if ((!GLOBALS->signalwindow_width_dirty) && (GLOBALS->use_nonprop_fonts))
        return;

    dirty_kick = GLOBALS->signalwindow_width_dirty;
    GLOBALS->signalwindow_width_dirty = 0;

    t = GLOBALS->traces.first;

    while (t) {
        char *subname = NULL;
        bv = NULL;
        tscan = NULL;

        if (t->flags &
            (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) /* seek to real xact trace if present... */
        {
            int bcnt = 0;
            tscan = t;
            while ((tscan) && (tscan = GivePrevTrace(tscan))) {
                if (!(tscan->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                    if (tscan->flags & TR_TTRANSLATED) {
                        break; /* found it */
                    } else {
                        tscan = NULL;
                    }
                } else {
                    bcnt++; /* bcnt is number of blank traces */
                }
            }

            if ((tscan) && (tscan->vector)) {
                bv = tscan->n.vec;
                do {
                    bv = bv->transaction_chain; /* correlate to blank trace */
                } while (bv && (bcnt--));
                if (bv) {
                    subname = bv->bvname;
                    if (GLOBALS->hier_max_level)
                        subname = hier_extract(subname, GLOBALS->hier_max_level);
                }
            }
        }

        populateBuffer(t, subname, buf);

        if (!bv && (t->flags &
                    (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) /* for "comment" style blank traces */
        {
            if (t->name || subname) {
                len = font_engine_string_measure(GLOBALS->signalfont, buf);
                if (len > maxlen)
                    maxlen = len;
            }

            if (t->asciivalue) {
                free_2(t->asciivalue);
                t->asciivalue = NULL;
            }
            t = GiveNextTrace(t);
        } else if (t->name || subname) {
            len = font_engine_string_measure(GLOBALS->signalfont, buf);
            if (len > maxlen)
                maxlen = len;

            if ((GLOBALS->tims.marker != -1) && (!(t->flags & TR_EXCLUDE))) {
                t->asciitime = GLOBALS->tims.marker;
                if (t->asciivalue) {
                    free_2(t->asciivalue);
                }
                t->asciivalue = NULL;

                if (bv || t->vector) {
                    char *str, *str2;
                    vptr v;
                    Trptr ts;
                    TraceEnt t_temp;

                    if (bv) {
                        ts = &t_temp;
                        memcpy(ts, tscan, sizeof(TraceEnt));
                        ts->vector = 1;
                        ts->n.vec = bv;
                    } else {
                        ts = t;
                        bv = t->n.vec;
                    }

                    v = bsearch_vector(bv, GLOBALS->tims.marker - ts->shift);
                    str = convert_ascii(ts, v);
                    if (str) {
                        str2 = (char *)malloc_2(strlen(str) + 2);
                        *str2 = '=';
                        strcpy(str2 + 1, str);
                        free_2(str);

                        vlen = font_engine_string_measure(GLOBALS->signalfont, str2);
                        t->asciivalue = str2;
                    } else {
                        vlen = 0;
                        t->asciivalue = NULL;
                    }
                } else {
                    char *str;
                    hptr h_ptr;
                    if ((h_ptr = bsearch_node(t->n.nd, GLOBALS->tims.marker - t->shift))) {
                        if (!t->n.nd->extvals) {
                            unsigned char h_val = h_ptr->v.h_val;

                            str = (char *)calloc_2(1, 3 * sizeof(char));
                            str[0] = '=';
                            if (t->n.nd->vartype == ND_VCD_EVENT) {
                                h_val = (h_ptr->time >= GLOBALS->tims.first) &&
                                                ((GLOBALS->tims.marker - GLOBALS->shift_timebase) ==
                                                 h_ptr->time)
                                            ? AN_1
                                            : AN_0; /* generate impulse */
                            }

                            if (t->flags & TR_INVERT) {
                                str[1] = AN_STR_INV[h_val];
                            } else {
                                str[1] = AN_STR[h_val];
                            }
                            t->asciivalue = str;
                            vlen = font_engine_string_measure(GLOBALS->signalfont, str);
                        } else {
                            char *str2;

                            if (h_ptr->flags & HIST_REAL) {
                                if (!(h_ptr->flags & HIST_STRING)) {
#ifdef WAVE_HAS_H_DOUBLE
                                    str = convert_ascii_real(t, &h_ptr->v.h_double);
#else
                                    str = convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
                                } else {
                                    str = convert_ascii_string((char *)h_ptr->v.h_vector);
                                }
                            } else {
                                str = convert_ascii_vec(t, h_ptr->v.h_vector);
                            }

                            if (str) {
                                str2 = (char *)malloc_2(strlen(str) + 2);
                                *str2 = '=';
                                strcpy(str2 + 1, str);

                                free_2(str);

                                vlen = font_engine_string_measure(GLOBALS->signalfont, str2);
                                t->asciivalue = str2;
                            } else {
                                vlen = 0;
                                t->asciivalue = NULL;
                            }
                        }
                    } else {
                        vlen = 0;
                        t->asciivalue = NULL;
                    }
                }

                if (vlen > vmaxlen) {
                    vmaxlen = vlen;
                }
            }

            t = GiveNextTrace(t);
        } else {
            t = GiveNextTrace(t);
        }
    }

    GLOBALS->max_signal_name_pixel_width = maxlen;
    GLOBALS->signal_pixmap_width = maxlen + 6; /* 2 * 3 pixel pad */
    if (GLOBALS->tims.marker != -1) {
        GLOBALS->signal_pixmap_width += (vmaxlen + 6);
        if (GLOBALS->signal_pixmap_width > 32767)
            GLOBALS->signal_pixmap_width = 32767; /* fixes X11 protocol limitation crash */
    }

    GLOBALS->tims.resizemarker2 = GLOBALS->tims.resizemarker;
    GLOBALS->tims.resizemarker = GLOBALS->tims.marker;

    if (GLOBALS->signal_pixmap_width < 60)
        GLOBALS->signal_pixmap_width = 60;

    MaxSignalLength_2(dirty_kick);
}

void MaxSignalLength_2(char dirty_kick)
{
    if (!GLOBALS->in_button_press_wavewindow_c_1) {
        if (!GLOBALS->do_resize_signals) {
            int os;
            os = 48;
            if (GLOBALS->initial_signal_window_width > os) {
                os = GLOBALS->initial_signal_window_width;
            }

            if (GLOBALS->signalwindow) {
                GtkAllocation allocation;
                gtk_widget_get_allocation(GLOBALS->signalwindow, &allocation);

                /* printf("VALUES: %d %d %d\n", GLOBALS->initial_signal_window_width,
                 * allocation.width, GLOBALS->max_signal_name_pixel_width); */
                if (GLOBALS->first_unsized_signals && GLOBALS->max_signal_name_pixel_width != 0) {
                    GLOBALS->first_unsized_signals = 0;
                    gtk_paned_set_position(GTK_PANED(GLOBALS->panedwindow),
                                           GLOBALS->max_signal_name_pixel_width + 30);
                } else {
                    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow), os + 30, -1);
                }
            }
        } else if ((GLOBALS->do_resize_signals) && (GLOBALS->signalwindow)) {
            int oldusize;
            int rs;

            if (GLOBALS->initial_signal_window_width > GLOBALS->max_signal_name_pixel_width) {
                rs = GLOBALS->initial_signal_window_width;
            } else {
                rs = GLOBALS->max_signal_name_pixel_width;
            }

            GtkAllocation allocation;
            gtk_widget_get_allocation(GLOBALS->signalwindow, &allocation);

            oldusize = allocation.width;
            if ((oldusize != rs) ||
                (dirty_kick)) { /* keep signalwindow from expanding arbitrarily large */
                int wx, wy;
                get_window_size(&wx, &wy);

                if ((3 * rs) < (2 * wx)) /* 2/3 width max */
                {
                    int os;
                    os = rs;
                    os = (os < 48) ? 48 : os;
                    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow), os + 30, -1);
                } else {
                    int os;
                    os = 48;
                    if (GLOBALS->initial_signal_window_width > os) {
                        os = GLOBALS->initial_signal_window_width;
                    }

                    gtk_widget_set_size_request(GTK_WIDGET(GLOBALS->signalwindow), os + 30, -1);
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

    if (!t)
        return;
    if ((t->asciivalue) && (t->asciitime == GLOBALS->tims.marker))
        return;

    if (t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) /* seek to real xact trace if present... */
    {
        int bcnt = 0;
        tscan = t;
        while ((tscan) && (tscan = GivePrevTrace(tscan))) {
            if (!(tscan->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                if (tscan->flags & TR_TTRANSLATED) {
                    break; /* found it */
                } else {
                    tscan = NULL;
                }
            } else {
                bcnt++; /* bcnt is number of blank traces */
            }
        }

        if ((tscan) && (tscan->vector)) {
            bv = tscan->n.vec;
            do {
                bv = bv->transaction_chain; /* correlate to blank trace */
            } while (bv && (bcnt--));
            if (bv) {
                /* nothing, we just want to set bv */
            }
        }
    }

    if ((t->name || bv) && (bv || !(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)))) {
        GLOBALS->shift_timebase = t->shift;
        DEBUG(printf("UpdateSigValue: %s\n", t->name));

        if ((GLOBALS->tims.marker != -1) && (!(t->flags & TR_EXCLUDE))) {
            t->asciitime = GLOBALS->tims.marker;
            if (t->asciivalue)
                free_2(t->asciivalue);

            if (bv || t->vector) {
                char *str, *str2;
                vptr v;
                Trptr ts;
                TraceEnt t_temp;

                if (bv) {
                    ts = &t_temp;
                    memcpy(ts, tscan, sizeof(TraceEnt));
                    ts->vector = 1;
                    ts->n.vec = bv;
                } else {
                    ts = t;
                    bv = t->n.vec;
                }

                v = bsearch_vector(bv, GLOBALS->tims.marker - ts->shift);
                str = convert_ascii(ts, v);
                if (str) {
                    str2 = (char *)malloc_2(strlen(str) + 2);
                    *str2 = '=';
                    strcpy(str2 + 1, str);
                    free_2(str);

                    t->asciivalue = str2;
                } else {
                    t->asciivalue = NULL;
                }
            } else {
                char *str;
                hptr h_ptr;
                if ((h_ptr = bsearch_node(t->n.nd, GLOBALS->tims.marker - t->shift))) {
                    if (!t->n.nd->extvals) {
                        unsigned char h_val = h_ptr->v.h_val;
                        if (t->n.nd->vartype == ND_VCD_EVENT) {
                            h_val = (h_ptr->time >= GLOBALS->tims.first) &&
                                            ((GLOBALS->tims.marker - GLOBALS->shift_timebase) ==
                                             h_ptr->time)
                                        ? AN_1
                                        : AN_0; /* generate impulse */
                        }

                        str = (char *)calloc_2(1, 3 * sizeof(char));
                        str[0] = '=';
                        if (t->flags & TR_INVERT) {
                            str[1] = AN_STR_INV[h_val];
                        } else {
                            str[1] = AN_STR[h_val];
                        }
                        t->asciivalue = str;
                    } else {
                        char *str2;

                        if (h_ptr->flags & HIST_REAL) {
                            if (!(h_ptr->flags & HIST_STRING)) {
#ifdef WAVE_HAS_H_DOUBLE
                                str = convert_ascii_real(t, &h_ptr->v.h_double);
#else
                                str = convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
                            } else {
                                str = convert_ascii_string((char *)h_ptr->v.h_vector);
                            }
                        } else {
                            str = convert_ascii_vec(t, h_ptr->v.h_vector);
                        }

                        if (str) {
                            str2 = (char *)malloc_2(strlen(str) + 2);
                            *str2 = '=';
                            strcpy(str2 + 1, str);
                            free_2(str);

                            t->asciivalue = str2;
                        } else {
                            t->asciivalue = NULL;
                        }
                    }
                } else {
                    t->asciivalue = NULL;
                }
            }
        }
    }
}

/***************************************************************************/

void calczoom(gdouble z0)
{
    gdouble ppf, frame;
    ppf = ((gdouble)(GLOBALS->pixelsperframe = 200));
    frame = pow(GLOBALS->zoombase, -z0);

    if (frame > ((gdouble)MAX_HISTENT_TIME / (gdouble)4.0)) {
        GLOBALS->nsperframe = ((gdouble)MAX_HISTENT_TIME / (gdouble)4.0);
    } else if (frame < (gdouble)1.0) {
        GLOBALS->nsperframe = 1.0;
    } else {
        GLOBALS->nsperframe = frame;
    }

    GLOBALS->hashstep = 10.0;

    if (GLOBALS->zoom_pow10_snap)
        if (GLOBALS->nsperframe > 10.0) {
            TimeType nsperframe2;
            gdouble p = 10.0;
            gdouble scale;
            int l;
            l = (int)((log(GLOBALS->nsperframe) / log(p)) + 0.5); /* nearest power of 10 */
            nsperframe2 = pow(p, (gdouble)l);

            scale = (gdouble)nsperframe2 / (gdouble)GLOBALS->nsperframe;
            ppf *= scale;
            GLOBALS->pixelsperframe = ppf;

            GLOBALS->nsperframe = nsperframe2;
            GLOBALS->hashstep = ppf / 10.0;
        }

    GLOBALS->nspx = GLOBALS->nsperframe / ppf;
    GLOBALS->pxns = ppf / GLOBALS->nsperframe;

    time_trunc_set(); /* map nspx to rounding value */

    DEBUG(printf("Zoom: %e Pixelsperframe: %d, nsperframe: %e\n",
                 z0,
                 (int)GLOBALS->pixelsperframe,
                 (float)GLOBALS->nsperframe));
}
