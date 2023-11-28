#include <config.h>
#include "cairo.h"
#include "gw-wave-view.h"
#include "gw-wave-view-private.h"
#include "gw-wave-view-traces.h"
#include "globals.h"
#include "wavewindow.h"

G_DEFINE_TYPE(GwWaveView, gw_wave_view, GTK_TYPE_DRAWING_AREA)

static void update_dual(void)
{
    if (GLOBALS->dual_ctx == NULL || GLOBALS->dual_race_lock) {
        return;
    }

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    GwMarker *baseline_marker = gw_project_get_baseline_marker(GLOBALS->project);

    // TODO: don't use sentinel values for disabled markers
    GwTime primary_pos =
        gw_marker_is_enabled(primary_marker) ? gw_marker_get_position(primary_marker) : -1;
    GwTime baseline_pos =
        gw_marker_is_enabled(baseline_marker) ? gw_marker_get_position(baseline_marker) : -1;

    GLOBALS->dual_ctx[GLOBALS->dual_id].zoom = GLOBALS->tims.zoom;
    GLOBALS->dual_ctx[GLOBALS->dual_id].marker = primary_pos;
    GLOBALS->dual_ctx[GLOBALS->dual_id].baseline = baseline_pos;
    GLOBALS->dual_ctx[GLOBALS->dual_id].left_margin_time = GLOBALS->tims.start;
    GLOBALS->dual_ctx[GLOBALS->dual_id].use_new_times = 1;
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

static void draw_marker(GwWaveView *self, cairo_t *cr, GwWaveformColors *colors)
{
    (void)self;

    gdouble pixstep;
    gint xl;

    cairo_set_line_width(cr, GLOBALS->cr_line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

    int m1x_wavewindow_c_1 = -1;
    int m2x_wavewindow_c_1 = -1;

    GwMarker *baseline_marker = gw_project_get_baseline_marker(GLOBALS->project);
    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    GwMarker *ghost_marker = gw_project_get_ghost_marker(GLOBALS->project);

    if (gw_marker_is_enabled(baseline_marker)) {
        GwTime baseline_pos = gw_marker_get_position(baseline_marker);
        if (baseline_pos >= GLOBALS->tims.start && baseline_pos <= GLOBALS->tims.last &&
            baseline_pos <= GLOBALS->tims.end) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            xl = ((gdouble)(baseline_pos - GLOBALS->tims.start)) / pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                XXX_gdk_draw_line(cr,
                                  colors->marker_baseline,
                                  xl,
                                  GLOBALS->fontheight - 1,
                                  xl,
                                  GLOBALS->waveheight - 1);
            }
        }
    }

    if (gw_marker_is_enabled(primary_marker)) {
        GwTime primary_pos = gw_marker_get_position(primary_marker);
        if (primary_pos >= GLOBALS->tims.start && primary_pos <= GLOBALS->tims.last &&
            primary_pos <= GLOBALS->tims.end) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            xl = ((gdouble)(primary_pos - GLOBALS->tims.start)) / pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                XXX_gdk_draw_line(cr,
                                  colors->marker_primary,
                                  xl,
                                  GLOBALS->fontheight - 1,
                                  xl,
                                  GLOBALS->waveheight - 1);
                m1x_wavewindow_c_1 = xl;
            }
        }
    }

    if (GLOBALS->enable_ghost_marker && GLOBALS->in_button_press_wavewindow_c_1 &&
        gw_marker_is_enabled(ghost_marker)) {
        GwTime ghost_pos = gw_marker_get_position(ghost_marker);
        if ((ghost_pos >= GLOBALS->tims.start) && (ghost_pos <= GLOBALS->tims.last) &&
            (ghost_pos <= GLOBALS->tims.end)) {
            pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
            xl = ((gdouble)(ghost_pos - GLOBALS->tims.start)) / pixstep; /* snap to integer */
            if ((xl >= 0) && (xl < GLOBALS->wavewidth)) {
                XXX_gdk_draw_line(cr,
                                  colors->marker_primary,
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

static void renderhash(GwWaveView *self, cairo_t *cr, GwWaveformColors *colors, int x, GwTime tim)
{
    (void)self;

    GwTime rborder;
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

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, colors->grid.r, colors->grid.g, colors->grid.b, colors->grid.a);
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

static void rendertimes(GwWaveView *self, cairo_t *cr, GwWaveformColors *colors)
{
    int lastx = -1000; /* arbitrary */
    int x, lenhalf;
    GwTime tim, rem;
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
        const GwColor *gc;

        if (!s_ctx_iter) {
            gc = &colors->grid;
            cairo_set_dash(cr, dashed1, 0, 0);
        } else {
            gc = &colors->grid2;
            /* gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT,
             * GDK_JOIN_BEVEL); */
            cairo_set_dash(cr, dashed1, sizeof(dashed1) / sizeof(dashed1[0]), 0);
        }

        cairo_set_source_rgba(cr, gc->r, gc->g, gc->b, gc->a);

        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];

        if (GLOBALS->strace_ctx->timearray) {
            int pos, pos2;
            GwTime *t, tm;
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
        GwTime rhs =
            (GLOBALS->tims.end > GLOBALS->tims.last) ? GLOBALS->tims.last : GLOBALS->tims.end;
        GwTime low_x = (GLOBALS->tims.start - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
        GwTime high_x = (rhs - GLOBALS->ruler_origin) / GLOBALS->ruler_step;
        GwTime iter_x, tm;
        int y = GLOBALS->fontheight + 2;
        int oldx = -1;

        cairo_set_source_rgba(cr, colors->grid.r, colors->grid.g, colors->grid.b, colors->grid.a);

        for (iter_x = low_x; iter_x <= high_x; iter_x++) {
            tm = GLOBALS->ruler_step * iter_x + GLOBALS->ruler_origin;
            x = (tm - GLOBALS->tims.start) * GLOBALS->pxns;
            if (oldx == x) {
                gdouble xd, offset, pixstep;
                GwTime newcurr;

                xd = x + 1; /* for pix time calc */

                pixstep = ((gdouble)GLOBALS->nsperframe) / ((gdouble)GLOBALS->pixelsperframe);
                newcurr = (GwTime)(offset = ((gdouble)GLOBALS->tims.start) + (xd * pixstep));

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

    DEBUG(printf("Ruler Start time: %" GW_TIME_FORMAT ", Finish time: %" GW_TIME_FORMAT "\n",
                 GLOBALS->tims.start,
                 GLOBALS->tims.end));

    x = 0;
    realx = 0;
    if (tim) {
        rem = tim % ((GwTime)GLOBALS->nsperframe);
        if (rem) {
            tim = tim - GLOBALS->nsperframe - rem;
            x = -GLOBALS->pixelsperframe - ((rem * GLOBALS->pixelsperframe) / GLOBALS->nsperframe);
            realx =
                -GLOBALS->pixelsperframe - ((rem * GLOBALS->pixelsperframe) / GLOBALS->nsperframe);
        }
    }

    for (;;) {
        renderhash(self, cr, colors, realx, tim);

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
                                        &colors->timebar_text,
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

static void rendertimebar(GwWaveView *self, cairo_t *cr, GwWaveformColors *colors)
{
    gint width = gtk_widget_get_allocated_width(GTK_WIDGET(self));

    XXX_gdk_draw_rectangle(cr, colors->timebar_background, TRUE, 0, -1, width, GLOBALS->fontheight);
    rendertimes(self, cr, colors);
}

typedef struct
{
    cairo_t *cr;
    GwWaveformColors *colors;
} RenderBlackoutData;

static void renderblackout(GwTime start, GwTime end, gpointer user_data)
{
    RenderBlackoutData *data = user_data;

    gdouble pageinc = GLOBALS->wavewidth * GLOBALS->nspx;
    GwTime lhs = GLOBALS->tims.start;
    GwTime rhs = pageinc + lhs;

    if (start > rhs || end < lhs) {
        // skip out of bounds regions
        return;
    }

    GwTime lclip = start;
    GwTime rclip = end;

    if (lclip < lhs) {
        lclip = lhs;
    } else if (lclip > rhs) {
        lclip = rhs;
    }

    if (rclip < lhs) {
        rclip = lhs;
    }

    lclip -= lhs;
    rclip -= lhs;
    if (rclip > ((GLOBALS->wavewidth + 1) * GLOBALS->nspx)) {
        rclip = (GLOBALS->wavewidth + 1) * (GLOBALS->nspx);
    }

    XXX_gdk_draw_rectangle(data->cr,
                           data->colors->fill_x,
                           TRUE,
                           (((gdouble)lclip) * GLOBALS->pxns),
                           GLOBALS->fontheight,
                           (((gdouble)(rclip - lclip)) * GLOBALS->pxns),
                           GLOBALS->waveheight - GLOBALS->fontheight);
}

typedef struct
{
    GwWaveView *widget;
    cairo_t *cr;
    GwColor color;
} DrawNamedMarkerData;

static void draw_named_marker(gpointer data, gpointer user_data)
{
    GwMarker *marker = data;
    DrawNamedMarkerData *draw_data = user_data;

    if (!gw_marker_is_enabled(marker)) {
        return;
    }

    GwTime t = gw_marker_get_position(marker);
    if (t < GLOBALS->tims.start || t > GLOBALS->tims.last || t > GLOBALS->tims.end) {
        return;
    }

    gdouble pixstep = GLOBALS->nsperframe / GLOBALS->pixelsperframe;
    gint x = (t - GLOBALS->tims.start) / pixstep; /* snap to integer */

    if (x < 0 || x > gtk_widget_get_allocated_width(GTK_WIDGET(draw_data->widget))) {
        return;
    }

    cairo_t *cr = draw_data->cr;

    static const double DASHES[] = {5.0, 3.0};
    cairo_set_dash(cr, DASHES, G_N_ELEMENTS(DASHES), 0.0);

    XXX_gdk_draw_line(cr, draw_data->color, x, GLOBALS->fontheight - 1, x, GLOBALS->waveheight - 1);
    cairo_set_dash(cr, DASHES, 0, 0);

    const gchar *name = gw_marker_get_display_name(marker);
    gint text_width = font_engine_string_measure(GLOBALS->wavefont_smaller, name);
    XXX_font_engine_draw_string(cr,
                                GLOBALS->wavefont_smaller,
                                &draw_data->color,
                                x - text_width / 2,
                                GLOBALS->fontheight - 2,
                                name);

    // if ((!GLOBALS->marker_names[i]) || (!GLOBALS->marker_names[i][0])) {
    //     XXX_font_engine_draw_string(
    //         cr,
    //         GLOBALS->wavefont_smaller,
    //         &gc,
    //         x - (font_engine_string_measure(GLOBALS->wavefont_smaller, nbuff) >> 1) + offset,
    //         GLOBALS->fontheight - 2 + offset,
    //         nbuff);
    // } else {
    //     int width = font_engine_string_measure(GLOBALS->wavefont_smaller,
    //     GLOBALS->marker_names[i]); if (blackout) /* blackout background so text is legible if
    //     overlaid with other
    //                      marker labels */
    //     {
    //         XXX_gdk_draw_rectangle(cr,
    //                                GLOBALS->rgb_gc.gc_timeb_wavewindow_c_1,
    //                                TRUE,
    //                                x - (width >> 1),
    //                                GLOBALS->fontheight - 2 - GLOBALS->wavefont_smaller->ascent,
    //                                width,
    //                                GLOBALS->wavefont_smaller->ascent +
    //                                    GLOBALS->wavefont_smaller->descent);
    //     }

    //     XXX_font_engine_draw_string(cr,
    //                                 GLOBALS->wavefont_smaller,
    //                                 &gc,
    //                                 xl - (width >> 1) + offset,
    //                                 GLOBALS->fontheight - 2 + offset,
    //                                 GLOBALS->marker_names[i]);
    // }
}

static void draw_named_markers(GwWaveView *self, cairo_t *cr, GwWaveformColors *colors)
{
    (void)self;

    DrawNamedMarkerData data = {
        .widget = self,
        .cr = cr,
        .color = colors->marker_named,
    };

    GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
    gw_named_markers_foreach(markers, draw_named_marker, &data);

    // TODO: draw locked marker on top
    // if (GLOBALS->named_marker_lock_idx >= 0) {
    //     render_individual_named_marker(cr,
    //                                    GLOBALS->named_marker_lock_idx,
    //                                    GLOBALS->rgb_gc.gc_umark_wavewindow_c_1,
    //                                    1);
    // }
}

static gboolean gw_wave_view_draw(GtkWidget *widget, cairo_t *cr)
{
    GwWaveView *self = GW_WAVE_VIEW(widget);

    gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
    /* struct Global *g_old = GLOBALS; */

    set_GLOBALS((*GLOBALS->contexts)[page_num]);

    GwWaveformColors *colors = gw_color_theme_get_waveform_colors(GLOBALS->color_theme);
    if (GLOBALS->black_and_white) {
        colors = gw_waveform_colors_new_black_and_white();
    }

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

        if (GLOBALS->blackout_regions != NULL) {
            RenderBlackoutData data = {.cr = traces_cr, .colors = colors};

            gw_blackout_regions_foreach(GLOBALS->blackout_regions, renderblackout, &data);
        }

        if (GLOBALS->disable_antialiasing) {
            cairo_set_antialias(traces_cr, CAIRO_ANTIALIAS_NONE);
        }
        gw_wave_view_render_traces(self, traces_cr);

        cairo_destroy(traces_cr);

        // gdouble time = g_timer_elapsed(timer, NULL);
        // g_printerr("Draw: %f\n", time);
        // g_timer_destroy(timer);

        self->dirty = FALSE;
    }

    cairo_set_source_rgba(cr,
                          colors->background.r,
                          colors->background.g,
                          colors->background.b,
                          colors->background.a);
    cairo_paint(cr);

    rendertimebar(self, cr, colors);

    cairo_set_source_surface(cr, self->traces_surface, 0.0, 0.0);
    cairo_paint(cr);

    cairo_set_line_width(cr, GLOBALS->cr_line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

    draw_marker(self, cr, colors);
    draw_named_markers(self, cr, colors);

    /* seems to cause a conflict flipping back so don't! */
    /* set_GLOBALS(g_old); */

    update_dual();

    // TODO: check
    // if (gesture_filter_set)
    //     gesture_filter_set = 0;

    if (GLOBALS->black_and_white) {
        g_free(colors);
    }

    return FALSE;
}

static void gw_wave_view_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GwWaveView *self = GW_WAVE_VIEW(widget);
    int scale;

    GTK_WIDGET_CLASS(gw_wave_view_parent_class)->size_allocate(widget, allocation);

    if (GLOBALS->wavewidth == allocation->width && GLOBALS->waveheight == allocation->height &&
        self->traces_surface != NULL) {
        return;
    }

    GLOBALS->wavewidth = allocation->width;
    GLOBALS->waveheight = allocation->height;

    g_clear_pointer(&self->traces_surface, cairo_surface_destroy);

    scale = gtk_widget_get_scale_factor(widget);

    self->traces_surface = gdk_window_create_similar_image_surface(gtk_widget_get_window(widget),
                                                                   CAIRO_FORMAT_ARGB32,
                                                                   allocation->width * scale,
                                                                   allocation->height * scale,
                                                                   scale);

    self->dirty = TRUE;
}

static void gw_wave_view_class_init(GwWaveViewClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    widget_class->configure_event = gw_wave_view_configure_event;
    widget_class->size_allocate = gw_wave_view_size_allocate;
    widget_class->draw = gw_wave_view_draw;
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
