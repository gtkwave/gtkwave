#include <config.h>
#include "cairo.h"
#include "wave_view.h"
#include "wave_view_traces.h"
#include "globals.h"
#include "wavewindow.h"

struct _GwWaveView
{
    GtkDrawingArea parent_instance;

    cairo_surface_t *traces_surface;

    gboolean dirty;
};

G_DEFINE_TYPE(GwWaveView, gw_wave_view, GTK_TYPE_DRAWING_AREA)

static const int wave_rgb_rainbow[WAVE_NUM_RAINBOW] = WAVE_RAINBOW_RGB;

static void update_dual(void)
{
    if (GLOBALS->dual_ctx && !GLOBALS->dual_race_lock) {
        GLOBALS->dual_ctx[GLOBALS->dual_id].zoom = GLOBALS->tims.zoom;
        GLOBALS->dual_ctx[GLOBALS->dual_id].marker = GLOBALS->tims.marker;
        GLOBALS->dual_ctx[GLOBALS->dual_id].baseline = GLOBALS->tims.baseline;
        GLOBALS->dual_ctx[GLOBALS->dual_id].left_margin_time = GLOBALS->tims.start;
        GLOBALS->dual_ctx[GLOBALS->dual_id].use_new_times = 1;
    }
}

gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
    (void)event;

    if ((!widget) || (!gtk_widget_get_window(widget)))
        return (TRUE);

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    DEBUG(printf("WaveWin Configure Event h: %d, w: %d\n", allocation.height, allocation.width));

    if (GLOBALS->timestart_from_savefile_valid) {
        gfloat pageinc = (gfloat)(((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);

        if ((GLOBALS->timestart_from_savefile >= GLOBALS->tims.first) &&
            (GLOBALS->timestart_from_savefile <= (GLOBALS->tims.last - pageinc))) {
            GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
            gtk_adjustment_set_value(hadj, (gdouble)(GLOBALS->timestart_from_savefile));
            fix_wavehadj();
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "value_changed"); /* force zoom update */
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "changed"); /* force zoom update */
        }
        GLOBALS->timestart_from_savefile_valid--; /* for previous gtk toolkit versions could set
                                                     this directly to zero */
    }

    if (GLOBALS->wavewidth > 1) {
        if ((!GLOBALS->do_initial_zoom_fit) || (GLOBALS->do_initial_zoom_fit_used)) {
            calczoom(GLOBALS->tims.zoom);
            fix_wavehadj();
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "value_changed"); /* force zoom update */
            g_signal_emit_by_name(GTK_ADJUSTMENT(GLOBALS->wave_hslider),
                                  "changed"); /* force zoom update */
        } else {
            GLOBALS->do_initial_zoom_fit_used = 1;
            service_zoom_fit(NULL, NULL);
        }
    }

    /* tims.timecache=tims.laststart; */
    return (TRUE);
}

static gboolean gw_wave_view_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
    gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
    struct Global *g_old = GLOBALS;

    set_GLOBALS((*GLOBALS->contexts)[page_num]);

    configure_event(widget, event);

    set_GLOBALS(g_old);

    return FALSE;
}

static void draw_marker(GwWaveView *self, cairo_t *cr)
{
    (void)self;

    gdouble pixstep;
    gint xl;

    cairo_set_line_width(cr, GLOBALS->cr_line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

    int m1x_wavewindow_c_1 = -1;
    int m2x_wavewindow_c_1 = -1;

    if (GLOBALS->tims.baseline >= 0) {
        if ((GLOBALS->tims.baseline >= GLOBALS->tims.start) &&
            (GLOBALS->tims.baseline <= GLOBALS->tims.last) &&
            (GLOBALS->tims.baseline <= GLOBALS->tims.end)) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            xl = ((gdouble)(GLOBALS->tims.baseline - GLOBALS->tims.start)) /
                 pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                XXX_gdk_draw_line(cr,
                                  GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1,
                                  xl,
                                  GLOBALS->fontheight - 1,
                                  xl,
                                  GLOBALS->waveheight - 1);
            }
        }
    }

    if (GLOBALS->tims.marker >= 0) {
        if ((GLOBALS->tims.marker >= GLOBALS->tims.start) &&
            (GLOBALS->tims.marker <= GLOBALS->tims.last) &&
            (GLOBALS->tims.marker <= GLOBALS->tims.end)) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            xl = ((gdouble)(GLOBALS->tims.marker - GLOBALS->tims.start)) /
                 pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                XXX_gdk_draw_line(cr,
                                  GLOBALS->rgb_gc.gc_umark_wavewindow_c_1,
                                  xl,
                                  GLOBALS->fontheight - 1,
                                  xl,
                                  GLOBALS->waveheight - 1);
                m1x_wavewindow_c_1 = xl;
            }
        }
    }

    if ((GLOBALS->enable_ghost_marker) && (GLOBALS->in_button_press_wavewindow_c_1) &&
        (GLOBALS->tims.lmbcache >= 0)) {
        if ((GLOBALS->tims.lmbcache >= GLOBALS->tims.start) &&
            (GLOBALS->tims.lmbcache <= GLOBALS->tims.last) &&
            (GLOBALS->tims.lmbcache <= GLOBALS->tims.end)) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            xl = ((gdouble)(GLOBALS->tims.lmbcache - GLOBALS->tims.start)) /
                 pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                XXX_gdk_draw_line(cr,
                                  GLOBALS->rgb_gc.gc_umark_wavewindow_c_1,
                                  xl,
                                  GLOBALS->fontheight - 1,
                                  xl,
                                  GLOBALS->waveheight - 1);
                m2x_wavewindow_c_1 = xl;
            }
        }
    }

    if (m1x_wavewindow_c_1 > m2x_wavewindow_c_1) /* ensure m1x <= m2x for partitioned refresh */
    {
        gint t;

        t = m1x_wavewindow_c_1;
        m1x_wavewindow_c_1 = m2x_wavewindow_c_1;
        m2x_wavewindow_c_1 = t;
    }

    if (m1x_wavewindow_c_1 == -1) {
        m1x_wavewindow_c_1 =
            m2x_wavewindow_c_1; /* make both markers same if no ghost marker or v.v. */
    }
}

static void renderhash(GwWaveView *self, cairo_t *cr, int x, TimeType tim)
{
    (void)self;

    TimeType rborder;
    int fhminus2;
    int rhs;
    gdouble dx;
    gdouble hashoffset;
    int iter = 0;
    int s_ctx_iter;
    int timearray_encountered = (GLOBALS->ruler_step != 0);

    fhminus2 = GLOBALS->fontheight - 2;

    WAVE_STRACE_ITERATOR(s_ctx_iter)
    {
        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
        if (GLOBALS->strace_ctx->timearray) {
            timearray_encountered = 1;
            break;
        }
    }

    cairo_set_source_rgba(cr,
                          GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.r,
                          GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.g,
                          GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.b,
                          GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.a);
    cairo_move_to(cr, x + GLOBALS->cairo_050_offset, GLOBALS->cairo_050_offset);
    cairo_line_to(
        cr,
        x + GLOBALS->cairo_050_offset,
        (((!timearray_encountered) && (GLOBALS->display_grid) && (GLOBALS->enable_vert_grid))
             ? GLOBALS->waveheight
             : fhminus2) +
            GLOBALS->cairo_050_offset);
    cairo_stroke(cr);

    if (tim == GLOBALS->tims.last)
        return;

    rborder = (GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
    DEBUG(printf("Rborder: %lld, Wavewidth: %d\n", rborder, GLOBALS->wavewidth));
    if (rborder > GLOBALS->wavewidth)
        rborder = GLOBALS->wavewidth;
    if ((rhs = x + GLOBALS->pixelsperframe) > rborder)
        rhs = rborder;

    cairo_move_to(cr,
                  x + GLOBALS->cairo_050_offset,
                  GLOBALS->wavecrosspiece + GLOBALS->cairo_050_offset);
    cairo_line_to(cr,
                  rhs + GLOBALS->cairo_050_offset,
                  GLOBALS->wavecrosspiece + GLOBALS->cairo_050_offset);
    cairo_stroke(cr);

    dx = x + (hashoffset = GLOBALS->hashstep);
    x = dx;

    while ((hashoffset < GLOBALS->pixelsperframe) && (x <= rhs) && (iter < 9)) {
        cairo_move_to(cr,
                      x + GLOBALS->cairo_050_offset,
                      GLOBALS->wavecrosspiece + GLOBALS->cairo_050_offset);
        cairo_line_to(cr, x + GLOBALS->cairo_050_offset, fhminus2 + GLOBALS->cairo_050_offset);
        cairo_stroke(cr);

        hashoffset += GLOBALS->hashstep;
        dx = dx + GLOBALS->hashstep;
        if ((GLOBALS->pixelsperframe != 200) || (GLOBALS->hashstep != 10.0))
            iter++; /* fix any roundoff errors */
        x = dx;
    }
}

static void rendertimes(GwWaveView *self, cairo_t *cr)
{
    int lastx = -1000; /* arbitrary */
    int x, lenhalf;
    TimeType tim, rem;
    char timebuff[32];
    char prevover = 0;
    gdouble realx;
    int s_ctx_iter;
    int timearray_encountered = 0;
    static const double dashed1[] = {8.0, 8.0};

    tim = GLOBALS->tims.start;

    GLOBALS->tims.end = GLOBALS->tims.start + (((gdouble)GLOBALS->wavewidth) * GLOBALS->nspx);

    /***********/
    WAVE_STRACE_ITERATOR_FWD(s_ctx_iter)
    {
        wave_rgb_t gc;

        if (!s_ctx_iter) {
            gc = GLOBALS->rgb_gc.gc_grid_wavewindow_c_1;
            cairo_set_dash(cr, dashed1, 0, 0);
        } else {
            gc = GLOBALS->rgb_gc.gc_grid2_wavewindow_c_1;
            /* gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT,
             * GDK_JOIN_BEVEL); */
            cairo_set_dash(cr, dashed1, sizeof(dashed1) / sizeof(dashed1[0]), 0);
        }

        cairo_set_source_rgba(cr, gc.r, gc.g, gc.b, gc.a);

        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];

        if (GLOBALS->strace_ctx->timearray) {
            int pos, pos2;
            TimeType *t, tm;
            int y = GLOBALS->fontheight + 2;
            int oldx = -1;

            timearray_encountered = 1;

            pos = bsearch_timechain(GLOBALS->tims.start);
        top:
            if ((pos >= 0) && (pos < GLOBALS->strace_ctx->timearray_size)) {
                t = GLOBALS->strace_ctx->timearray + pos;
                for (; pos < GLOBALS->strace_ctx->timearray_size; t++, pos++) {
                    tm = *t;
                    if (tm >= GLOBALS->tims.start) {
                        if (tm <= GLOBALS->tims.end) {
                            x = (tm - GLOBALS->tims.start) * GLOBALS->pxns;
                            if (oldx == x) {
                                pos2 = bsearch_timechain(GLOBALS->tims.start +
                                                         (((gdouble)(x + 1)) * GLOBALS->nspx));
                                if (pos2 > pos) {
                                    pos = pos2;
                                    goto top;
                                } else
                                    continue;
                            }
                            oldx = x;

                            cairo_move_to(cr,
                                          x + GLOBALS->cairo_050_offset,
                                          y + GLOBALS->cairo_050_offset);
                            cairo_line_to(cr,
                                          x + GLOBALS->cairo_050_offset,
                                          GLOBALS->waveheight + GLOBALS->cairo_050_offset);
                            cairo_stroke(cr);
                        } else {
                            break;
                        }
                    }
                }
            }
        }

        if (s_ctx_iter) {
            /* gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL); */
        }
    }

    GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = 0];
    /***********/

    cairo_set_dash(cr, dashed1, 0, 0);

    if (GLOBALS->ruler_step && !timearray_encountered) {
        TimeType rhs =
            (GLOBALS->tims.end > GLOBALS->tims.last) ? GLOBALS->tims.last : GLOBALS->tims.end;
        TimeType low_x = (GLOBALS->tims.start - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
        TimeType high_x = (rhs - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
        TimeType iter_x, tm;
        int y = GLOBALS->fontheight + 2;
        int oldx = -1;

        cairo_set_source_rgba(cr,
                              GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.r,
                              GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.g,
                              GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.b,
                              GLOBALS->rgb_gc.gc_grid_wavewindow_c_1.a);

        for (iter_x = low_x; iter_x <= high_x; iter_x++) {
            tm = GLOBALS->ruler_step * iter_x + GLOBALS->ruler_origin;
            x = (tm - GLOBALS->tims.start) * GLOBALS->pxns;
            if (oldx == x) {
                gdouble xd, offset, pixstep;
                TimeType newcurr;

                xd = x + 1; /* for pix time calc */

                pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
                newcurr = (TimeType)(offset = ((gdouble)GLOBALS->tims.start) + (xd * pixstep));

                if (offset - newcurr > 0.5) /* round to nearest integer ns */
                {
                    newcurr++;
                }

                low_x = (newcurr - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
                if (low_x <= iter_x)
                    low_x = (iter_x + 1);
                iter_x = low_x;
                tm = GLOBALS->ruler_step * iter_x + GLOBALS->ruler_origin;
                x = (tm - GLOBALS->tims.start) * GLOBALS->pxns;
            }

            if (x >= GLOBALS->wavewidth)
                break;
            oldx = x;

            cairo_move_to(cr, x + GLOBALS->cairo_050_offset, y + GLOBALS->cairo_050_offset);
            cairo_line_to(cr,
                          x + GLOBALS->cairo_050_offset,
                          GLOBALS->waveheight + GLOBALS->cairo_050_offset);
            cairo_stroke(cr);
        }
    }

    /***********/

    DEBUG(printf("Ruler Start time: " TTFormat ", Finish time: " TTFormat "\n",
                 GLOBALS->tims.start,
                 GLOBALS->tims.end));

    x = 0;
    realx = 0;
    if (tim) {
        rem = tim % ((TimeType)GLOBALS->nsperframe);
        if (rem) {
            tim = tim - GLOBALS->nsperframe - rem;
            x = -GLOBALS->pixelsperframe - ((rem * GLOBALS->pixelsperframe) / GLOBALS->nsperframe);
            realx =
                -GLOBALS->pixelsperframe - ((rem * GLOBALS->pixelsperframe) / GLOBALS->nsperframe);
        }
    }

    for (;;) {
        renderhash(self, cr, realx, tim);

        if (tim + GLOBALS->global_time_offset) {
            if (tim != GLOBALS->min_time) {
                reformat_time(timebuff,
                              time_trunc(tim) + GLOBALS->global_time_offset,
                              GLOBALS->time_dimension);
            } else {
                timebuff[0] = 0;
            }
        } else {
            strcpy(timebuff, "0");
        }

        lenhalf = font_engine_string_measure(GLOBALS->wavefont, timebuff) >> 1;

        if ((x - lenhalf >= lastx) || (GLOBALS->pixelsperframe >= 200)) {
            XXX_font_engine_draw_string(cr,
                                        GLOBALS->wavefont,
                                        &GLOBALS->rgb_gc.gc_time_wavewindow_c_1,
                                        x - lenhalf + GLOBALS->cairo_050_offset,
                                        GLOBALS->wavefont->ascent - 1 + GLOBALS->cairo_050_offset,
                                        timebuff);
            lastx = x + lenhalf;
        }

        tim += GLOBALS->nsperframe;
        x += GLOBALS->pixelsperframe;
        realx += GLOBALS->pixelsperframe;
        if ((prevover) || (tim > GLOBALS->tims.last))
            break;
        if (x >= GLOBALS->wavewidth)
            prevover = 1;
    }
}

static void rendertimebar(GwWaveView *self, cairo_t *cr)
{
    gint width = gtk_widget_get_allocated_width(GTK_WIDGET(self));

    XXX_gdk_draw_rectangle(cr,
                           GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1,
                           TRUE,
                           0,
                           -1,
                           width,
                           GLOBALS->fontheight);
    rendertimes(self, cr);
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

static void render_individual_named_marker(cairo_t *cr, int i, wave_rgb_t gc, int blackout)
{
    gdouble pixstep;
    gint xl;
    TimeType t;

    gdouble offset = GLOBALS->cairo_050_offset;

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
                            offset,
                        GLOBALS->fontheight - 2 + offset,
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
                                                xl - (width >> 1) + offset,
                                                GLOBALS->fontheight - 2 + offset,
                                                GLOBALS->marker_names[i]);
                }
            }
        }
    }
}

static void draw_named_markers(GwWaveView *self, cairo_t *cr)
{
    (void)self;

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

static gboolean gw_wave_view_draw(GtkWidget *widget, cairo_t *cr)
{
    GwWaveView *self = GW_WAVE_VIEW(widget);

    gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
    /* struct Global *g_old = GLOBALS; */

    set_GLOBALS((*GLOBALS->contexts)[page_num]);

    if (self->dirty) {
        // GTimer *timer = g_timer_new();

        GLOBALS->tims.end = GLOBALS->tims.start + GLOBALS->nspx * GLOBALS->wavewidth;

        cairo_t *traces_cr = cairo_create(self->traces_surface);

        cairo_set_operator(traces_cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(traces_cr, 0.0, 0.0, 0.0, 0.0);
        cairo_paint(traces_cr);
        cairo_set_operator(traces_cr, CAIRO_OPERATOR_OVER);

        cairo_set_line_width(traces_cr, GLOBALS->cr_line_width);
        cairo_set_line_cap(traces_cr, CAIRO_LINE_CAP_SQUARE);

        renderblackout(traces_cr);

        if (GLOBALS->disable_antialiasing) {
            cairo_set_antialias(traces_cr, CAIRO_ANTIALIAS_NONE);
        }
        
        rendertraces(traces_cr);

        cairo_destroy(traces_cr);

        // gdouble time = g_timer_elapsed(timer, NULL);
        // g_printerr("Draw: %f\n", time);
        // g_timer_destroy(timer);

        self->dirty = FALSE;
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    rendertimebar(self, cr);

    cairo_set_source_surface(cr, self->traces_surface, 0.0, 0.0);
    cairo_paint(cr);

    cairo_set_line_width(cr, GLOBALS->cr_line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

    draw_marker(self, cr);
    draw_named_markers(self, cr);

    /* seems to cause a conflict flipping back so don't! */
    /* set_GLOBALS(g_old); */

    update_dual();

    // TODO: check
    // if (gesture_filter_set)
    //     gesture_filter_set = 0;

    return FALSE;
}

static void gw_wave_view_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GwWaveView *self = GW_WAVE_VIEW(widget);

    GTK_WIDGET_CLASS(gw_wave_view_parent_class)->size_allocate(widget, allocation);

    if (GLOBALS->wavewidth == allocation->width && GLOBALS->waveheight == allocation->height &&
        self->traces_surface != NULL) {
        return;
    }

    GLOBALS->wavewidth = allocation->width;
    GLOBALS->waveheight = allocation->height;

    g_clear_pointer(&self->traces_surface, cairo_surface_destroy);

    self->traces_surface =
        gdk_window_create_similar_image_surface(gtk_widget_get_window(widget),
                                                CAIRO_FORMAT_ARGB32,
                                                allocation->width,
                                                allocation->height,
                                                gtk_widget_get_scale_factor(widget));

    self->dirty = TRUE;
}

static void gw_wave_view_class_init(GwWaveViewClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    widget_class->configure_event = gw_wave_view_configure_event;
    widget_class->size_allocate = gw_wave_view_size_allocate;
    widget_class->draw = gw_wave_view_draw;
}

static void init_rgb_gc(void)
{
    if (GLOBALS->made_gc_contexts_wavewindow_c_1) {
        return;
    }
    GLOBALS->rgb_gc.gc_back_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_back);
    GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_baseline);
    GLOBALS->rgb_gc.gc_grid_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_grid);
    GLOBALS->rgb_gc.gc_grid2_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_grid2);
    GLOBALS->rgb_gc.gc_time_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_time);
    GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_timeb);
    GLOBALS->rgb_gc.gc_value_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_value);
    GLOBALS->rgb_gc.gc_low_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_low);
    GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_highfill);
    GLOBALS->rgb_gc.gc_high_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_high);
    GLOBALS->rgb_gc.gc_trans_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_trans);
    GLOBALS->rgb_gc.gc_mid_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_mid);
    GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_xfill);
    GLOBALS->rgb_gc.gc_x_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_x);
    GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_vbox);
    GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_vtrans);
    GLOBALS->rgb_gc.gc_mark_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_mark);
    GLOBALS->rgb_gc.gc_umark_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_umark);

    GLOBALS->rgb_gc.gc_0_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_0);
    GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_1fill);
    GLOBALS->rgb_gc.gc_1_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_1);
    GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_ufill);
    GLOBALS->rgb_gc.gc_u_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_u);
    GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_wfill);
    GLOBALS->rgb_gc.gc_w_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_w);
    GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_dashfill);
    GLOBALS->rgb_gc.gc_dash_wavewindow_c_1 = XXX_alloc_color(GLOBALS->color_dash);

    GLOBALS->made_gc_contexts_wavewindow_c_1 = ~0;

    memcpy(&GLOBALS->rgb_gccache, &GLOBALS->rgb_gc, sizeof(struct wave_rgbmaster_t));

    /* add rainbow colors */
    for (int i = 0; i < WAVE_NUM_RAINBOW; i++) {
        int col = wave_rgb_rainbow[i];

        GLOBALS->rgb_gc_rainbow[i * 2] = XXX_alloc_color(col);
        col >>= 1;
        col &= 0x007F7F7F;
        GLOBALS->rgb_gc_rainbow[i * 2 + 1] = XXX_alloc_color(col);
    }
}

static void gw_wave_view_init(GwWaveView *self)
{
    gtk_widget_set_events(GTK_WIDGET(self),
                          GDK_SCROLL_MASK | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
                              | GDK_TOUCH_MASK | GDK_TABLET_PAD_MASK | GDK_TOUCHPAD_GESTURE_MASK
#endif
    );

    init_rgb_gc();

    self->dirty = TRUE;
}

GtkWidget *gw_wave_view_new(void)
{
    return g_object_new(GW_TYPE_WAVE_VIEW, NULL);
}

void gw_wave_view_force_redraw(GwWaveView *self)
{
    g_return_if_fail(GW_IS_WAVE_VIEW(self));

    self->dirty = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(self));
}
