#include <config.h>
#include <gtk/gtk.h>
#include "cairo.h"
#include "gw-wave-view.h"
#include "gw-wave-view-private.h"
#include "globals.h"
#include "signal_list.h"
#include "wavewindow.h"

static GwColor XXX_get_gc_from_name(const char *str)
{
    GwColor color = {0.0, 0.0, 0.0, 0.0};

    gw_color_init_from_x11_name(&color, str);

    return color;
}

typedef enum
{
    LINE_COLOR_X,
    LINE_COLOR_U,
    LINE_COLOR_W,
    LINE_COLOR_DASH,
    LINE_COLOR_TRANS,
    LINE_COLOR_0,
    LINE_COLOR_LOW,
    LINE_COLOR_MID,
    LINE_COLOR_1,
    LINE_COLOR_HIGH,
    LINE_COLOR_VBOX,
    N_LINE_COLORS,
} LineColor;

typedef struct
{
    gint x1;
    gint y1;
    gint x2;
    gint y2;
} Line;

typedef struct
{
    GwColor colors[N_LINE_COLORS];
    GArray *lines[N_LINE_COLORS];
} LineBuffer;

static LineBuffer *line_buffer_new(const GwWaveformColors *colors)
{
    LineBuffer *self = g_new0(LineBuffer, 1);
    self->colors[LINE_COLOR_X] = colors->stroke_x;
    self->colors[LINE_COLOR_U] = colors->stroke_u;
    self->colors[LINE_COLOR_W] = colors->stroke_w;
    self->colors[LINE_COLOR_DASH] = colors->stroke_dash;
    self->colors[LINE_COLOR_TRANS] = colors->stroke_transition;
    self->colors[LINE_COLOR_0] = colors->stroke_0;
    self->colors[LINE_COLOR_LOW] = colors->stroke_l;
    self->colors[LINE_COLOR_MID] = colors->stroke_z;
    self->colors[LINE_COLOR_1] = colors->stroke_1;
    self->colors[LINE_COLOR_HIGH] = colors->stroke_h;
    self->colors[LINE_COLOR_VBOX] = colors->stroke_vector;

    for (gint i = 0; i < N_LINE_COLORS; i++) {
        self->lines[i] = g_array_new(FALSE, FALSE, sizeof(Line));
    }

    return self;
}

static void line_buffer_free(LineBuffer *self)
{
    for (gint i = 0; i < N_LINE_COLORS; i++) {
        g_array_free(self->lines[i], TRUE);
    }
    g_free(self);
}

static void line_buffer_add(LineBuffer *self, LineColor color, gint x1, gint y1, gint x2, gint y2)
{
    Line line = (Line){
        .x1 = x1,
        .y1 = y1,
        .x2 = x2,
        .y2 = y2,
    };
    g_array_append_val(self->lines[color], line);
}

static void line_buffer_draw(LineBuffer *self, cairo_t *cr)
{
    gdouble offset = GLOBALS->cairo_050_offset;

    for (gint i = 0; i < N_LINE_COLORS; i++) {
        GwColor *color = &self->colors[i];
        GArray *lines = self->lines[i];

        if (lines->len == 0) {
            continue;
        }

        cairo_set_source_rgba(cr, color->r, color->g, color->b, color->a);

        for (guint j = 0; j < lines->len; j++) {
            Line *line = &g_array_index(lines, Line, j);

            cairo_move_to(cr, line->x1 + offset, line->y1 + offset);
            cairo_line_to(cr, line->x2 + offset, line->y2 + offset);
        }

        cairo_stroke(cr);
    }
}

static void draw_hptr_trace(GwWaveView *self,
                            cairo_t *cr,
                            GwWaveformColors *colors,
                            GwTrace *t,
                            GwHistEnt *h,
                            int which,
                            int dodraw,
                            int kill_grid);
static void draw_hptr_trace_vector(GwWaveView *self,
                                   cairo_t *cr,
                                   GwWaveformColors *colors,
                                   GwTrace *t,
                                   GwHistEnt *h,
                                   int which);
static void draw_vptr_trace(GwWaveView *self,
                            cairo_t *cr,
                            GwWaveformColors *colors,
                            GwTrace *t,
                            GwVectorEnt *v,
                            int which);

void gw_wave_view_render_traces(GwWaveView *self, cairo_t *cr)
{
    GwTrace *t = gw_signal_list_get_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), 0);
    if (t) {
        GwTrace *tback = t;
        GwHistEnt *h;
        GwVectorEnt *v;
        int i = 0, num_traces_displayable;
        int iback = 0;

        GtkAllocation allocation;
        gtk_widget_get_allocation(GLOBALS->wavearea, &allocation);

        num_traces_displayable = allocation.height / (GLOBALS->fontheight);
        num_traces_displayable--; /* for the time trace that is always there */

        /* ensure that transaction traces are visible even if the topmost traces are blanks */
        while (tback) {
            if (tback->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) {
                tback = GivePrevTrace(tback);
                iback--;
            } else if (tback->flags & TR_TTRANSLATED) {
                if (tback != t) {
                    t = tback;
                    i = iback;
                }
                break;
            } else {
                break;
            }
        }

        for (; ((i < num_traces_displayable) && (t)); i++) {
            gboolean uses_rainbow_color =
                t->t_color >= 1 && (t->t_color - 1) < GW_NUM_RAINBOW_COLORS;

            GwWaveformColors *colors = gw_color_theme_get_waveform_colors(GLOBALS->color_theme);
            if (GLOBALS->black_and_white) {
                colors = gw_waveform_colors_new_black_and_white();
            } else if (uses_rainbow_color) {
                colors = gw_waveform_colors_get_rainbow_variant(colors,
                                                                t->t_color - 1,
                                                                GLOBALS->keep_xz_colors);
            }

            if (!(t->flags & (TR_EXCLUDE | TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                GLOBALS->shift_timebase = t->shift;
                if (!t->vector) {
                    h = bsearch_node(t->n.nd, GLOBALS->tims.start - t->shift);
                    DEBUG(printf("Start time: %" GW_TIME_FORMAT ", Histent time: %" GW_TIME_FORMAT
                                 "\n",
                                 GLOBALS->tims.start,
                                 (h->time + GLOBALS->shift_timebase)));

                    if (i >= 0) {
                        if (!t->n.nd->extvals) {
                            draw_hptr_trace(self, cr, colors, t, h, i, 1, 0);
                        } else {
                            draw_hptr_trace_vector(self, cr, colors, t, h, i);
                        }
                    }
                } else {
                    GwTrace *t_orig;
                    GwTrace *tn;
                    GwBitVector *bv = t->n.vec;

                    v = bsearch_vector(bv, GLOBALS->tims.start - t->shift);
                    DEBUG(printf("Vector Trace: %s, %s\n", t->name, bv->bvname));
                    DEBUG(printf("Start time: %" GW_TIME_FORMAT ", Vectorent time: %" GW_TIME_FORMAT
                                 "\n",
                                 GLOBALS->tims.start,
                                 (v->time + GLOBALS->shift_timebase)));
                    if (i >= 0) {
                        draw_vptr_trace(self, cr, colors, t, v, i);
                    }

                    if ((bv->transaction_chain) && (t->flags & TR_TTRANSLATED)) {
                        t_orig = t;
                        for (;;) {
                            tn = GiveNextTrace(t);
                            bv = bv->transaction_chain;

                            if (bv && tn && (tn->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                                i++;
                                if (i < num_traces_displayable) {
                                    v = bsearch_vector(bv, GLOBALS->tims.start - t->shift);
                                    if (i >= 0) {
                                        draw_vptr_trace(self, cr, colors, t_orig, v, i);
                                    }
                                    t = tn;
                                    continue;
                                }
                            }
                            break;
                        }
                    }
                }
            } else {
                int kill_dodraw_grid = t->flags & TR_ANALOG_BLANK_STRETCH;

                if (kill_dodraw_grid) {
                    GwTrace *tn = GiveNextTrace(t);
                    if (!tn) {
                        kill_dodraw_grid = 0;
                    } else if (!(tn->flags & TR_ANALOG_BLANK_STRETCH)) {
                        kill_dodraw_grid = 0;
                    }
                }

                if (i >= 0) {
                    draw_hptr_trace(self, cr, colors, NULL, NULL, i, 0, kill_dodraw_grid);
                }
            }
            t = GiveNextTrace(t);
            /* bot:		1; */

            if (GLOBALS->black_and_white || uses_rainbow_color) {
                g_free(colors);
            }
        }
    }

    if (GLOBALS->traces.dirty) {
        char dbuf[32];
        sprintf(dbuf, "%d", GLOBALS->traces.total);
        GLOBALS->traces.dirty = 0;
        gtkwavetcl_setvar(WAVE_TCLCB_TRACES_UPDATED, dbuf, WAVE_TCLCB_TRACES_UPDATED_FLAGS);
    }
}

/*
 * draw single traces and use this for rendering the grid lines
 * for "excluded" traces
 */
static void draw_hptr_trace(GwWaveView *self,
                            cairo_t *cr,
                            GwWaveformColors *colors,
                            GwTrace *t,
                            GwHistEnt *h,
                            int which,
                            int dodraw,
                            int kill_grid)
{
    GwTime _x0, _x1, newtime;
    int _y0, _y1, yu, liney, ytext;
    GwTime tim, h2tim;
    GwHistEnt *h2;
    GwHistEnt *h3;
    char hval, h2val, invert;
    LineColor c;
    GwColor gcx, gcxf;
    char identifier_str[2];
    int is_event = t && t->n.nd && (t->n.nd->vartype == GW_VAR_TYPE_VCD_EVENT);

    LineBuffer *lines = line_buffer_new(colors);

    GLOBALS->tims.start -= GLOBALS->shift_timebase;
    GLOBALS->tims.end -= GLOBALS->shift_timebase;

    liney = ((which + 2) * GLOBALS->fontheight) - 2;
    if (((t) && (t->flags & TR_INVERT)) && (!is_event)) {
        _y0 = ((which + 1) * GLOBALS->fontheight) + 2;
        _y1 = liney - 2;
        invert = 1;
    } else {
        _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
        _y0 = liney - 2;
        invert = 0;
    }
    yu = (_y0 + _y1) / 2;
    ytext = yu - (GLOBALS->wavefont->ascent / 2) + GLOBALS->wavefont->ascent;

    if ((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
        (!GLOBALS->black_and_white) && (!kill_grid)) {
        XXX_gdk_draw_rectangle(cr,
                               colors->grid,
                               TRUE,
                               0,
                               liney - GLOBALS->fontheight,
                               GLOBALS->wavewidth,
                               GLOBALS->fontheight);
    } else if ((GLOBALS->display_grid) && (GLOBALS->enable_horiz_grid) && (!kill_grid)) {
        XXX_gdk_draw_line(cr,
                          colors->grid,
                          (GLOBALS->tims.start < GLOBALS->tims.first)
                              ? (GLOBALS->tims.first - GLOBALS->tims.start) * GLOBALS->pxns
                              : 0,
                          liney,
                          (GLOBALS->tims.last <= GLOBALS->tims.end)
                              ? (GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns
                              : GLOBALS->wavewidth - 1,
                          liney);
    }

    if ((h) && (GLOBALS->tims.start == h->time))
        if (h->v.h_val != AN_Z) {
            switch (h->v.h_val) {
                case AN_X:
                    c = LINE_COLOR_X;
                    break;
                case AN_U:
                    c = LINE_COLOR_U;
                    break;
                case AN_W:
                    c = LINE_COLOR_W;
                    break;
                case AN_DASH:
                    c = LINE_COLOR_DASH;
                    break;
                default:
                    c = (h->v.h_val == AN_X) ? LINE_COLOR_X : LINE_COLOR_TRANS;
            }
            line_buffer_add(lines, c, 0, _y0, 0, _y1);
        }

    if (dodraw && t)
        for (;;) {
            if (!h)
                break;
            tim = (h->time);

            if ((tim > GLOBALS->tims.end) || (tim > GLOBALS->tims.last))
                break;

            _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
            if (_x0 < -1) {
                _x0 = -1;
            } else if (_x0 > GLOBALS->wavewidth) {
                break;
            }

            h2 = h->next;
            if (!h2)
                break;
            h2tim = tim = (h2->time);
            if (tim > GLOBALS->tims.last)
                tim = GLOBALS->tims.last;
            else if (tim > GLOBALS->tims.end + 1)
                tim = GLOBALS->tims.end + 1;
            _x1 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
            if (_x1 < -1) {
                _x1 = -1;
            } else if (_x1 > GLOBALS->wavewidth) {
                _x1 = GLOBALS->wavewidth;
            }

            if (_x0 != _x1) {
                if (is_event) {
                    if (h->time >= GLOBALS->tims.first) {
                        line_buffer_add(lines, LINE_COLOR_W, _x0, _y0, _x0, _y1);
                        line_buffer_add(lines, LINE_COLOR_W, _x0, _y1, _x0 + 2, _y1 + 2);
                        line_buffer_add(lines, LINE_COLOR_W, _x0, _y1, _x0 - 2, _y1 + 2);
                    }
                    h = h->next;
                    continue;
                }

                hval = h->v.h_val;
                h2val = h2->v.h_val;

                switch (h2val) {
                    case AN_X:
                        c = LINE_COLOR_X;
                        break;
                    case AN_U:
                        c = LINE_COLOR_U;
                        break;
                    case AN_W:
                        c = LINE_COLOR_W;
                        break;
                    case AN_DASH:
                        c = LINE_COLOR_DASH;
                        break;
                    default:
                        c = (hval == AN_X) ? LINE_COLOR_X : LINE_COLOR_TRANS;
                }

                switch (hval) {
                    case AN_0: /* 0 */
                    case AN_L: /* L */
                        if (GLOBALS->fill_waveform && invert) {
                            switch (hval) {
                                case AN_0:
                                    gcxf = colors->fill_1;
                                    break;
                                case AN_L:
                                    gcxf = colors->fill_h;
                                    break;
                                default:
                                    g_warn_if_reached();
                                    break;
                            }
                            XXX_gdk_draw_rectangle(cr,
                                                   gcxf,
                                                   TRUE,
                                                   _x0 + 1,
                                                   _y0,
                                                   _x1 - _x0,
                                                   _y1 - _y0 + 1);
                        }
                        line_buffer_add(lines,
                                        (hval == AN_0) ? LINE_COLOR_0 : LINE_COLOR_LOW,
                                        _x0,
                                        _y0,
                                        _x1,
                                        _y0);

                        if (h2tim <= GLOBALS->tims.end)
                            switch (h2val) {
                                case AN_0:
                                case AN_L:
                                    break;

                                case AN_Z:
                                    line_buffer_add(lines, c, _x1, _y0, _x1, yu);
                                    break;
                                default:
                                    line_buffer_add(lines, c, _x1, _y0, _x1, _y1);
                                    break;
                            }
                        break;

                    case AN_X: /* X */
                    case AN_W: /* W */
                    case AN_U: /* U */
                    case AN_DASH: /* - */

                        identifier_str[1] = 0;
                        switch (hval) {
                            case AN_X:
                                c = LINE_COLOR_X;
                                gcx = colors->stroke_x;
                                gcxf = colors->fill_x;
                                identifier_str[0] = 0;
                                break;
                            case AN_W:
                                c = LINE_COLOR_W;
                                gcx = colors->stroke_w;
                                gcxf = colors->fill_w;
                                identifier_str[0] = 'W';
                                break;
                            case AN_U:
                                c = LINE_COLOR_U;
                                gcx = colors->stroke_u;
                                gcxf = colors->fill_u;
                                identifier_str[0] = 'U';
                                break;
                            default:
                                c = LINE_COLOR_DASH;
                                gcx = colors->stroke_dash;
                                gcxf = colors->fill_dash;
                                identifier_str[0] = '-';
                                break;
                        }

                        if (invert) {
                            XXX_gdk_draw_rectangle(cr,
                                                   gcx,
                                                   TRUE,
                                                   _x0 + 1,
                                                   _y0,
                                                   _x1 - _x0,
                                                   _y1 - _y0 + 1);
                        } else {
                            XXX_gdk_draw_rectangle(cr,
                                                   gcxf,
                                                   TRUE,
                                                   _x0 + 1,
                                                   _y1,
                                                   _x1 - _x0,
                                                   _y0 - _y1 + 1);
                        }

                        if (identifier_str[0]) {
                            int _x0_new = (_x0 >= 0) ? _x0 : 0;
                            int width;

                            if ((width = _x1 - _x0_new) > GLOBALS->vector_padding) {
                                if ((_x1 >= GLOBALS->wavewidth) ||
                                    (font_engine_string_measure(GLOBALS->wavefont, identifier_str) +
                                         GLOBALS->vector_padding <=
                                     width)) {
                                    XXX_font_engine_draw_string(cr,
                                                                GLOBALS->wavefont,
                                                                &colors->value_text,
                                                                _x0 + 2 + GLOBALS->cairo_050_offset,
                                                                ytext + GLOBALS->cairo_050_offset,
                                                                identifier_str);
                                }
                            }
                        }

                        line_buffer_add(lines, c, _x0, _y0, _x1, _y0);
                        line_buffer_add(lines, c, _x0, _y1, _x1, _y1);
                        if (h2tim <= GLOBALS->tims.end)
                            line_buffer_add(lines, c, _x1, _y0, _x1, _y1);
                        break;

                    case AN_Z: /* Z */
                        line_buffer_add(lines, LINE_COLOR_MID, _x0, yu, _x1, yu);
                        if (h2tim <= GLOBALS->tims.end)
                            switch (h2val) {
                                case AN_0:
                                case AN_L:
                                    line_buffer_add(lines, c, _x1, yu, _x1, _y0);
                                    break;
                                case AN_1:
                                case AN_H:
                                    line_buffer_add(lines, c, _x1, yu, _x1, _y1);
                                    break;
                                default:
                                    line_buffer_add(lines, c, _x1, _y0, _x1, _y1);
                                    break;
                            }
                        break;

                    case AN_1: /* 1 */
                    case AN_H: /* 1 */
                        if (GLOBALS->fill_waveform && !invert) {
                            switch (hval) {
                                case AN_1:
                                    gcxf = colors->fill_1;
                                    break;
                                case AN_H:
                                    gcxf = colors->fill_h;
                                    break;
                                default:
                                    g_warn_if_reached();
                                    break;
                            }
                            XXX_gdk_draw_rectangle(cr,
                                                   gcxf,
                                                   TRUE,
                                                   _x0 + 1,
                                                   _y1,
                                                   _x1 - _x0,
                                                   _y0 - _y1 + 1);
                        }
                        line_buffer_add(lines,
                                        (hval == AN_1) ? LINE_COLOR_1 : LINE_COLOR_HIGH,
                                        _x0,
                                        _y1,
                                        _x1,
                                        _y1);
                        if (h2tim <= GLOBALS->tims.end)
                            switch (h2val) {
                                case AN_1:
                                case AN_H:
                                    break;

                                case AN_0:
                                case AN_L:
                                    line_buffer_add(lines, c, _x1, _y1, _x1, _y0);
                                    break;
                                case AN_Z:
                                    line_buffer_add(lines, c, _x1, _y1, _x1, yu);
                                    break;
                                default:
                                    line_buffer_add(lines, c, _x1, _y0, _x1, _y1);
                                    break;
                            }
                        break;

                    default:
                        break;
                }
            } else {
                if (!is_event) {
                    line_buffer_add(lines, LINE_COLOR_TRANS, _x1, _y0, _x1, _y1);
                } else {
                    line_buffer_add(lines, LINE_COLOR_W, _x1, _y0, _x1, _y1);
                    line_buffer_add(lines, LINE_COLOR_W, _x0, _y1, _x0 + 2, _y1 + 2);
                    line_buffer_add(lines, LINE_COLOR_W, _x0, _y1, _x0 - 2, _y1 + 2);
                }
                newtime = (((gdouble)(_x1 + WAVE_OPT_SKIP)) * GLOBALS->nspx) +
                          GLOBALS->tims.start /*+GLOBALS->shift_timebase*/; /* skip to next pixel */
                h3 = bsearch_node(t->n.nd, newtime);
                if (h3->time > h->time) {
                    h = h3;
                    continue;
                }
            }

            if ((h->flags & GW_HIST_ENT_FLAG_GLITCH) && (GLOBALS->vcd_preserve_glitches)) {
                XXX_gdk_draw_rectangle(cr, colors->stroke_z, TRUE, _x1 - 1, yu - 1, 3, 3);
            }

            h = h->next;
        }

    line_buffer_draw(lines, cr);
    line_buffer_free(lines);

    GLOBALS->tims.start += GLOBALS->shift_timebase;
    GLOBALS->tims.end += GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_hptr_trace_vector_analog(GwWaveView *self,
                                          cairo_t *cr,
                                          GwWaveformColors *colors,
                                          GwTrace *t,
                                          GwHistEnt *h,
                                          int which,
                                          int num_extension)
{
    GwTime _x0, _x1, newtime;
    int _y0, _y1, yu, liney, yt0, yt1;
    GwTime tim, h2tim;
    GwHistEnt *h2;
    GwHistEnt *h3;
    int endcnt = 0;
    int type;
    /* int lasttype=-1; */ /* scan-build */
    GwColor c;
    GwColor ci = colors->marker_baseline;
    GwColor cnan = colors->stroke_u;
    GwColor cinf = colors->stroke_w;
    GwColor cfixed;
    double mynan = strtod("NaN", NULL);
    double tmin = mynan, tmax = mynan, tv, tv2;
    gint rmargin;
    int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
    int any_infs = 0, any_infp = 0, any_infm = 0;
    int skipcnt = 0;

    liney = ((which + 2 + num_extension) * GLOBALS->fontheight) - 2;
    _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
    _y0 = liney - 2;
    yu = (_y0 + _y1) / 2;

    if (t->flags & TR_ANALOG_FULLSCALE) /* otherwise use dynamic */
    {
        if ((!t->minmax_valid) || (t->d_num_ext != num_extension)) {
            h3 = &t->n.nd->head;
            for (;;) {
                if (!h3)
                    break;

                if ((h3->time >= GLOBALS->tims.first) && (h3->time <= GLOBALS->tims.last)) {
                    tv = mynan;
                    if (h3->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(h3->flags & GW_HIST_ENT_FLAG_STRING))
                            tv = h3->v.h_double;
                    } else {
                        if (h3->time <= GLOBALS->tims.last)
                            tv = convert_real_vec(t, h3->v.h_vector);
                    }

                    if (!isnan(tv) && !isinf(tv)) {
                        if (isnan(tmin) || tv < tmin)
                            tmin = tv;
                        if (isnan(tmax) || tv > tmax)
                            tmax = tv;
                    } else if (isinf(tv)) {
                        any_infs = 1;

                        if (tv > 0) {
                            any_infp = 1;
                        } else {
                            any_infm = 1;
                        }
                    }
                }
                h3 = h3->next;
            }

            if (isnan(tmin) || isnan(tmax)) {
                tmin = tmax = 0;
            }

            if (any_infs) {
                double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

                if (any_infp)
                    tmax = tmax + tdelta;
                if (any_infm)
                    tmin = tmin - tdelta;
            }

            if ((tmax - tmin) < 1e-20) {
                tmax = 1;
                tmin -= 0.5 * (_y1 - _y0);
            } else {
                tmax = (_y1 - _y0) / (tmax - tmin);
            }

            t->minmax_valid = 1;
            t->d_minval = tmin;
            t->d_maxval = tmax;
            t->d_num_ext = num_extension;
        } else {
            tmin = t->d_minval;
            tmax = t->d_maxval;
        }
    } else {
        h3 = h;
        for (;;) {
            if (!h3)
                break;
            tim = (h3->time);
            if (tim > GLOBALS->tims.end) {
                endcnt++;
                if (endcnt == 2)
                    break;
            }
            if (tim > GLOBALS->tims.last)
                break;

            _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
            if ((_x0 > GLOBALS->wavewidth) && (endcnt == 2)) {
                break;
            }

            tv = mynan;
            if (h3->flags & GW_HIST_ENT_FLAG_REAL) {
                if (!(h3->flags & GW_HIST_ENT_FLAG_STRING))
                    tv = h3->v.h_double;
            } else {
                if (h3->time <= GLOBALS->tims.last)
                    tv = convert_real_vec(t, h3->v.h_vector);
            }

            if (!isnan(tv) && !isinf(tv)) {
                if (isnan(tmin) || tv < tmin)
                    tmin = tv;
                if (isnan(tmax) || tv > tmax)
                    tmax = tv;
            } else if (isinf(tv)) {
                any_infs = 1;
                if (tv > 0) {
                    any_infp = 1;
                } else {
                    any_infm = 1;
                }
            }

            h3 = h3->next;
        }

        if (isnan(tmin) || isnan(tmax))
            tmin = tmax = 0;

        if (any_infs) {
            double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

            if (any_infp)
                tmax = tmax + tdelta;
            if (any_infm)
                tmin = tmin - tdelta;
        }

        if ((tmax - tmin) < 1e-20) {
            tmax = 1;
            tmin -= 0.5 * (_y1 - _y0);
        } else {
            tmax = (_y1 - _y0) / (tmax - tmin);
        }
    }

    if (GLOBALS->tims.last - GLOBALS->tims.start < GLOBALS->wavewidth) {
        rmargin = (GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
    } else {
        rmargin = GLOBALS->wavewidth;
    }

    /* now do the actual drawing */
    h3 = NULL;
    for (;;) {
        if (!h)
            break;
        tim = (h->time);
        if ((tim > GLOBALS->tims.end) || (tim > GLOBALS->tims.last))
            break;

        _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;

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

        h2 = h->next;
        if (!h2)
            break;
        h2tim = tim = (h2->time);
        if (tim > GLOBALS->tims.last)
            tim = GLOBALS->tims.last;
        /*	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1; */
        _x1 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;

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
        type = (!(h->flags & (GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING)))
                   ? vtype(t, h->v.h_vector)
                   : AN_COUNT;
        tv = tv2 = mynan;

        if (h->flags & GW_HIST_ENT_FLAG_REAL) {
            if (!(h->flags & GW_HIST_ENT_FLAG_STRING))
                tv = h->v.h_double;
        } else {
            if (h->time <= GLOBALS->tims.last)
                tv = convert_real_vec(t, h->v.h_vector);
        }

        if (h2->flags & GW_HIST_ENT_FLAG_REAL) {
            if (!(h2->flags & GW_HIST_ENT_FLAG_STRING))
                tv2 = h2->v.h_double;
        } else {
            if (h2->time <= GLOBALS->tims.last)
                tv2 = convert_real_vec(t, h2->v.h_vector);
        }

        if ((is_inf = isinf(tv))) {
            if (tv < 0) {
                yt0 = _y0;
            } else {
                yt0 = _y1;
            }
        } else if ((is_nan = isnan(tv))) {
            yt0 = yu;
        } else {
            yt0 = _y0 + (tv - tmin) * tmax;
        }

        if ((is_inf2 = isinf(tv2))) {
            if (tv2 < 0) {
                yt1 = _y0;
            } else {
                yt1 = _y1;
            }
        } else if ((is_nan2 = isnan(tv2))) {
            yt1 = yu;
        } else {
            yt1 = _y0 + (tv2 - tmin) * tmax;
        }

        if ((_x0 != _x1) ||
            (skipcnt < GLOBALS->analog_redraw_skip_count)) /* lower number = better performance */
        {
            if (_x0 == _x1) {
                skipcnt++;
            } else {
                skipcnt = 0;
            }

            if (type != AN_X) {
                c = colors->stroke_vector;
            } else {
                c = colors->stroke_x;
            }

            if (h->next) {
                if (h->next->time > GLOBALS->max_time) {
                    yt1 = yt0;
                }
            }

            cfixed = is_inf ? cinf : c;
            if ((is_nan2) && (h2tim > GLOBALS->max_time))
                is_nan2 = 0;

            /* clamp to top/bottom because of integer rounding errors */

            if (yt0 < _y1)
                yt0 = _y1;
            else if (yt0 > _y0)
                yt0 = _y0;

            if (yt1 < _y1)
                yt1 = _y1;
            else if (yt1 > _y0)
                yt1 = _y0;

            /* clipping... */
            {
                int coords[4];
                int rect[4];

                if (_x0 < INT_MIN) {
                    coords[0] = INT_MIN;
                } else if (_x0 > INT_MAX) {
                    coords[0] = INT_MAX;
                } else {
                    coords[0] = _x0;
                }

                if (_x1 < INT_MIN) {
                    coords[2] = INT_MIN;
                } else if (_x1 > INT_MAX) {
                    coords[2] = INT_MAX;
                } else {
                    coords[2] = _x1;
                }

                coords[1] = yt0;
                coords[3] = yt1;

                rect[0] = -10;
                rect[1] = _y1;
                rect[2] = GLOBALS->wavewidth + 10;
                rect[3] = _y0;

                if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) != TR_ANALOG_STEP) {
                    wave_lineclip(coords, rect);
                } else {
                    if (coords[0] < rect[0])
                        coords[0] = rect[0];
                    if (coords[2] < rect[0])
                        coords[2] = rect[0];

                    if (coords[0] > rect[2])
                        coords[0] = rect[2];
                    if (coords[2] > rect[2])
                        coords[2] = rect[2];

                    if (coords[1] < rect[1])
                        coords[1] = rect[1];
                    if (coords[3] < rect[1])
                        coords[3] = rect[1];

                    if (coords[1] > rect[3])
                        coords[1] = rect[3];
                    if (coords[3] > rect[3])
                        coords[3] = rect[3];
                }

                _x0 = coords[0];
                yt0 = coords[1];
                _x1 = coords[2];
                yt1 = coords[3];
            }
            /* ...clipping */

            if (is_nan || is_nan2) {
                if (is_nan) {
                    XXX_gdk_draw_rectangle(cr, cnan, TRUE, _x0, _y1, _x1 - _x0, _y0 - _y1);

                    if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) ==
                        (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) {
                        XXX_gdk_draw_line(cr, ci, _x1 - 1, yt1, _x1 + 1, yt1);
                        XXX_gdk_draw_line(cr, ci, _x1, yt1 - 1, _x1, yt1 + 1);

                        XXX_gdk_draw_line(cr, ci, _x0 - 1, _y0, _x0 + 1, _y0);
                        XXX_gdk_draw_line(cr, ci, _x0, _y0 - 1, _x0, _y0 + 1);

                        XXX_gdk_draw_line(cr, ci, _x0 - 1, _y1, _x0 + 1, _y1);
                        XXX_gdk_draw_line(cr, ci, _x0, _y1 - 1, _x0, _y1 + 1);
                    }
                }
                if (is_nan2) {
                    XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt0);

                    if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) ==
                        (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) {
                        XXX_gdk_draw_line(cr, cnan, _x1, yt1, _x1, yt0);

                        XXX_gdk_draw_line(cr, ci, _x1 - 1, _y0, _x1 + 1, _y0);
                        XXX_gdk_draw_line(cr, ci, _x1, _y0 - 1, _x1, _y0 + 1);

                        XXX_gdk_draw_line(cr, ci, _x1 - 1, _y1, _x1 + 1, _y1);
                        XXX_gdk_draw_line(cr, ci, _x1, _y1 - 1, _x1, _y1 + 1);
                    }
                }
            } else if ((t->flags & TR_ANALOG_INTERPOLATED) && !is_inf && !is_inf2) {
                if (t->flags & TR_ANALOG_STEP) {
                    XXX_gdk_draw_line(cr, ci, _x0 - 1, yt0, _x0 + 1, yt0);
                    XXX_gdk_draw_line(cr, ci, _x0, yt0 - 1, _x0, yt0 + 1);
                }

                if (rmargin != GLOBALS->wavewidth) /* the window is clipped in postscript */
                {
                    if ((yt0 == yt1) && ((_x0 > _x1) || (_x0 < 0))) {
                        XXX_gdk_draw_line(cr, cfixed, 0, yt0, _x1, yt1);
                    } else {
                        XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt1);
                    }
                } else {
                    XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt1);
                }
            } else
            /* if(t->flags & TR_ANALOG_STEP) */
            {
                XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt0);

                if (is_inf2)
                    cfixed = cinf;
                XXX_gdk_draw_line(cr, cfixed, _x1, yt0, _x1, yt1);

                if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) ==
                    (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) {
                    XXX_gdk_draw_line(cr, ci, _x0 - 1, yt0, _x0 + 1, yt0);
                    XXX_gdk_draw_line(cr, ci, _x0, yt0 - 1, _x0, yt0 + 1);
                }
            }
        } else {
            newtime = (((gdouble)(_x1 + WAVE_OPT_SKIP)) * GLOBALS->nspx) +
                      GLOBALS->tims.start /*+GLOBALS->shift_timebase*/; /* skip to next pixel */
            h3 = bsearch_node(t->n.nd, newtime);
            if (h3->time > h->time) {
                h = h3;
                /* lasttype=type; */ /* scan-build */
                continue;
            }
        }

        h = h->next;
        /* lasttype=type; */ /* scan-build */
    }
}

/*
 * draw hptr vectors (integer+real)
 */
static void draw_hptr_trace_vector(GwWaveView *self,
                                   cairo_t *cr,
                                   GwWaveformColors *colors,
                                   GwTrace *t,
                                   GwHistEnt *h,
                                   int which)
{
    GwTime _x0, _x1, newtime;
    int _y0, _y1, yu, liney, ytext;
    GwTime tim /* , h2tim */; /* scan-build */
    GwHistEnt *h2;
    GwHistEnt *h3;
    char *ascii = NULL;
    int type;

    GLOBALS->tims.start -= GLOBALS->shift_timebase;
    GLOBALS->tims.end -= GLOBALS->shift_timebase;

    liney = ((which + 2) * GLOBALS->fontheight) - 2;
    _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
    _y0 = liney - 2;
    yu = (_y0 + _y1) / 2;
    ytext = yu - (GLOBALS->wavefont->ascent / 2) + GLOBALS->wavefont->ascent;

    if ((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
        (!GLOBALS->black_and_white)) {
        GwTrace *tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
            XXX_gdk_draw_rectangle(cr,
                                   colors->grid,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        } else {
            XXX_gdk_draw_rectangle(cr,
                                   colors->grid,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        }
    } else if ((GLOBALS->display_grid) && (GLOBALS->enable_horiz_grid)) {
        GwTrace *tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
        } else {
            XXX_gdk_draw_line(cr,
                              colors->grid,
                              (GLOBALS->tims.start < GLOBALS->tims.first)
                                  ? (GLOBALS->tims.first - GLOBALS->tims.start) * GLOBALS->pxns
                                  : 0,
                              liney,
                              (GLOBALS->tims.last <= GLOBALS->tims.end)
                                  ? (GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns
                                  : GLOBALS->wavewidth - 1,
                              liney);
        }
    }

    if ((t->flags & TR_ANALOGMASK) &&
        (!(h->flags & GW_HIST_ENT_FLAG_STRING) || !(h->flags & GW_HIST_ENT_FLAG_REAL))) {
        GwTrace *te = GiveNextTrace(t);
        int ext = 0;

        while (te) {
            if (te->flags & TR_ANALOG_BLANK_STRETCH) {
                ext++;
                te = GiveNextTrace(te);
            } else {
                break;
            }
        }

        if ((ext) && (GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
            (!GLOBALS->black_and_white)) {
            XXX_gdk_draw_rectangle(cr,
                                   colors->grid,
                                   TRUE,
                                   0,
                                   liney,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight * ext);
        }

        draw_hptr_trace_vector_analog(self, cr, colors, t, h, which, ext);
        GLOBALS->tims.start += GLOBALS->shift_timebase;
        GLOBALS->tims.end += GLOBALS->shift_timebase;
        return;
    }

    GLOBALS->color_active_in_filter = 1;

    for (;;) {
        if (!h)
            break;
        tim = (h->time);
        if ((tim > GLOBALS->tims.end) || (tim > GLOBALS->tims.last))
            break;

        _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
        if (_x0 < -1) {
            _x0 = -1;
        } else if (_x0 > GLOBALS->wavewidth) {
            break;
        }

        h2 = h->next;
        if (!h2)
            break;
        /* h2tim= */ tim = (h2->time); /* scan-build */
        if (tim > GLOBALS->tims.last)
            tim = GLOBALS->tims.last;
        else if (tim > GLOBALS->tims.end + 1)
            tim = GLOBALS->tims.end + 1;
        _x1 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
        if (_x1 < -1) {
            _x1 = -1;
        } else if (_x1 > GLOBALS->wavewidth) {
            _x1 = GLOBALS->wavewidth;
        }

        /* draw trans */
        if (!(h->flags & (GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING))) {
            type = vtype(t, h->v.h_vector);
        } else {
            /* s\000 ID is special "z" case */
            type = AN_COUNT;

            if (h->flags & GW_HIST_ENT_FLAG_STRING) {
                if (h->v.h_vector) {
                    if (!h->v.h_vector[0]) {
                        type = AN_Z;
                    } else {
                        if (!strcmp(h->v.h_vector, "UNDEF")) {
                            type = AN_X;
                        }
                    }
                } else {
                    type = AN_X;
                }
            }
        }
        /* type = (!(h->flags&(GW_HIST_ENT_FLAG_REAL|GW_HIST_ENT_FLAG_STRING))) ?
         * vtype(t,h->v.h_vector) : AN_COUNT; */

        if (_x0 != _x1) {
            if (type == AN_Z) {
                gdouble offset = GLOBALS->cairo_050_offset;
                XXX_gdk_set_color(cr, colors->stroke_z);

                if (GLOBALS->use_roundcaps) {
                    cairo_move_to(cr, _x0 + 1 + offset, yu + offset);
                    cairo_line_to(cr, _x1 - 1 + offset, yu + offset);
                } else {
                    cairo_move_to(cr, _x0 + offset, yu + offset);
                    cairo_line_to(cr, _x1 + offset, yu + offset);
                }
                cairo_stroke(cr);
            } else {
                gdouble offset = GLOBALS->cairo_050_offset;
                GwColor color;
                if (type != AN_X && type != AN_U) {
                    color = colors->stroke_vector;
                } else {
                    color = colors->stroke_x;
                }

                GwTime width = _x1 - _x0;

                if (width == 1) {
                    XXX_gdk_set_color(cr, color);
                    cairo_move_to(cr, _x0 + offset, _y0 + offset);
                    cairo_line_to(cr, _x0 + offset, _y1 + offset);
                    cairo_stroke(cr);
                } else {
                    if (type == AN_1) {
                        GwColor c = colors->stroke_vector;
                        cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a / 3.0);
                    } else {
                        XXX_gdk_set_color(cr, color);
                    }

                    if (GLOBALS->use_roundcaps) {
                        if (width > 4) {
                            cairo_move_to(cr, _x0 + offset, yu + offset);
                            cairo_line_to(cr, _x0 + 2 + offset, _y0 + offset);
                            cairo_line_to(cr, _x1 - 2 + offset, _y0 + offset);
                            cairo_line_to(cr, _x1 + offset, yu + offset);
                        } else {
                            cairo_move_to(cr, _x0 + offset, yu + offset);
                            cairo_line_to(cr, _x0 + width / 2.0 + offset, _y0 + offset);
                            cairo_move_to(cr, _x0 + width / 2.0 + offset, _y0 + offset);
                            cairo_line_to(cr, _x1 + offset, yu + offset);
                        }
                    } else {
                        cairo_move_to(cr, _x0 + offset, yu + offset);
                        cairo_line_to(cr, _x0 + offset, _y0 + offset);
                        cairo_line_to(cr, _x1 + offset, _y0 + offset);
                        cairo_line_to(cr, _x1 + offset, yu + offset);
                    }
                    cairo_stroke(cr);

                    if (type == AN_0) {
                        GwColor c = colors->stroke_vector;
                        cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a / 3.0);
                    } else {
                        XXX_gdk_set_color(cr, color);
                    }

                    if (GLOBALS->use_roundcaps) {
                        if (width > 4) {
                            cairo_move_to(cr, _x0 + offset, yu + offset);
                            cairo_line_to(cr, _x0 + 2 + offset, _y1 + offset);
                            cairo_line_to(cr, _x1 - 2 + offset, _y1 + offset);
                            cairo_line_to(cr, _x1 + offset, yu + offset);
                        } else {
                            cairo_move_to(cr, _x0 + offset, yu + offset);
                            cairo_line_to(cr, _x0 + width / 2.0 + offset, _y1 + offset);
                            cairo_move_to(cr, _x0 + width / 2.0 + offset, _y1 + offset);
                            cairo_line_to(cr, _x1 + offset, yu + offset);
                        }
                    } else {
                        cairo_move_to(cr, _x0 + offset, yu + offset);
                        cairo_line_to(cr, _x0 + offset, _y1 + offset);
                        cairo_line_to(cr, _x1 + offset, _y1 + offset);
                        cairo_line_to(cr, _x1 + offset, yu + offset);
                    }
                    cairo_stroke(cr);
                }

                if (_x0 < 0)
                    _x0 = 0; /* fixup left margin */

                if ((width = _x1 - _x0) > GLOBALS->vector_padding) {
                    char *ascii2;

                    if (h->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(h->flags & GW_HIST_ENT_FLAG_STRING)) {
                            ascii = convert_ascii_real(t, &h->v.h_double);
                        } else {
                            ascii = convert_ascii_string((char *)h->v.h_vector);
                        }
                    } else {
                        ascii = convert_ascii_vec(t, h->v.h_vector);
                    }

                    ascii2 = ascii;
                    if (*ascii == '?') {
                        GwColor cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                ascii2 = srch_for_color + 1;
                                if (!gw_color_equal(&colors->background, &GW_COLOR_WHITE)) {
                                    if (!GLOBALS->black_and_white)
                                        XXX_gdk_draw_rectangle(cr,
                                                               cb,
                                                               TRUE,
                                                               _x0 + 1,
                                                               _y1 + 1,
                                                               width - 1,
                                                               (_y0 - 1) - (_y1 + 1) + 1);
                                }
                                GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1 = 1;
                            } else {
                                *srch_for_color = '?'; /* replace name as color is a miss */
                            }
                        }
                    }

                    if ((_x1 >= GLOBALS->wavewidth) ||
                        (font_engine_string_measure(GLOBALS->wavefont, ascii2) +
                             GLOBALS->vector_padding <=
                         width)) {
                        XXX_font_engine_draw_string(cr,
                                                    GLOBALS->wavefont,
                                                    &colors->value_text,
                                                    _x0 + 2 + GLOBALS->cairo_050_offset,
                                                    ytext + GLOBALS->cairo_050_offset,
                                                    ascii2);
                    } else {
                        char *mod;

                        mod = bsearch_trunc(ascii2, width - GLOBALS->vector_padding);
                        if (mod) {
                            *mod = '+';
                            *(mod + 1) = 0;

                            XXX_font_engine_draw_string(cr,
                                                        GLOBALS->wavefont,
                                                        &colors->value_text,
                                                        _x0 + 2 + GLOBALS->cairo_050_offset,
                                                        ytext + GLOBALS->cairo_050_offset,
                                                        ascii2);
                        }
                    }
                } else if (GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1) {
                    /* char *ascii2; */ /* scan-build */

                    if (h->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(h->flags & GW_HIST_ENT_FLAG_STRING)) {
                            ascii = convert_ascii_real(t, &h->v.h_double);
                        } else {
                            ascii = convert_ascii_string((char *)h->v.h_vector);
                        }
                    } else {
                        ascii = convert_ascii_vec(t, h->v.h_vector);
                    }

                    /* ascii2 = ascii; */ /* scan-build */
                    if (*ascii == '?') {
                        GwColor cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                /* ascii2 =  srch_for_color + 1; */ /* scan-build */
                                if (!gw_color_equal(&colors->background, &GW_COLOR_WHITE)) {
                                    if (!GLOBALS->black_and_white)
                                        XXX_gdk_draw_rectangle(cr,
                                                               cb,
                                                               TRUE,
                                                               _x0,
                                                               _y1 + 1,
                                                               width,
                                                               (_y0 - 1) - (_y1 + 1) + 1);
                                }
                            } else {
                                *srch_for_color = '?'; /* replace name as color is a miss */
                            }
                        }
                    }
                }
            }
        } else {
            newtime = (((gdouble)(_x1 + WAVE_OPT_SKIP)) * GLOBALS->nspx) +
                      GLOBALS->tims.start /*+GLOBALS->shift_timebase*/; /* skip to next pixel */
            h3 = bsearch_node(t->n.nd, newtime);
            if (h3->time > h->time) {
                h = h3;
                continue;
            }
        }

        if (ascii) {
            free_2(ascii);
            ascii = NULL;
        }
        h = h->next;
    }

    GLOBALS->color_active_in_filter = 0;

    GLOBALS->tims.start += GLOBALS->shift_timebase;
    GLOBALS->tims.end += GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_vptr_trace_analog(GwWaveView *self,
                                   cairo_t *cr,
                                   GwWaveformColors *colors,
                                   GwTrace *t,
                                   GwVectorEnt *v,
                                   int which,
                                   int num_extension)
{
    GwTime _x0, _x1, newtime;
    int _y0, _y1, yu, liney, yt0, yt1;
    GwTime tim, h2tim;
    GwVectorEnt *h;
    GwVectorEnt *h2;
    GwVectorEnt *h3;
    int endcnt = 0;
    int type;
    /* int lasttype=-1; */ /* scan-build */
    GwColor c;
    GwColor ci = colors->marker_baseline;
    GwColor cnan = colors->stroke_u;
    GwColor cinf = colors->stroke_w;
    GwColor cfixed;
    double mynan = strtod("NaN", NULL);
    double tmin = mynan, tmax = mynan, tv = 0.0, tv2;
    gint rmargin;
    int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
    int any_infs = 0, any_infp = 0, any_infm = 0;
    int skipcnt = 0;

    h = v;
    liney = ((which + 2 + num_extension) * GLOBALS->fontheight) - 2;
    _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
    _y0 = liney - 2;
    yu = (_y0 + _y1) / 2;

    if (t->flags & TR_ANALOG_FULLSCALE) /* otherwise use dynamic */
    {
        if ((!t->minmax_valid) || (t->d_num_ext != num_extension)) {
            h3 = t->n.vec->vectors[0];
            for (;;) {
                if (!h3)
                    break;

                if ((h3->time >= GLOBALS->tims.first) && (h3->time <= GLOBALS->tims.last)) {
                    /* tv = mynan; */ /* scan-build */

                    tv = convert_real(t, h3);
                    if (!isnan(tv) && !isinf(tv)) {
                        if (isnan(tmin) || tv < tmin)
                            tmin = tv;
                        if (isnan(tmax) || tv > tmax)
                            tmax = tv;
                    }
                } else if (isinf(tv)) {
                    any_infs = 1;

                    if (tv > 0) {
                        any_infp = 1;
                    } else {
                        any_infm = 1;
                    }
                }

                h3 = h3->next;
            }

            if (isnan(tmin) || isnan(tmax))
                tmin = tmax = 0;

            if (any_infs) {
                double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

                if (any_infp)
                    tmax = tmax + tdelta;
                if (any_infm)
                    tmin = tmin - tdelta;
            }

            if ((tmax - tmin) < 1e-20) {
                tmax = 1;
                tmin -= 0.5 * (_y1 - _y0);
            } else {
                tmax = (_y1 - _y0) / (tmax - tmin);
            }

            t->minmax_valid = 1;
            t->d_minval = tmin;
            t->d_maxval = tmax;
            t->d_num_ext = num_extension;
        } else {
            tmin = t->d_minval;
            tmax = t->d_maxval;
        }
    } else {
        h3 = h;
        for (;;) {
            if (!h3)
                break;
            tim = (h3->time);

            if (tim > GLOBALS->tims.end) {
                endcnt++;
                if (endcnt == 2)
                    break;
            }
            if (tim > GLOBALS->tims.last)
                break;

            _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
            if ((_x0 > GLOBALS->wavewidth) && (endcnt == 2)) {
                break;
            }

            tv = convert_real(t, h3);
            if (!isnan(tv) && !isinf(tv)) {
                if (isnan(tmin) || tv < tmin)
                    tmin = tv;
                if (isnan(tmax) || tv > tmax)
                    tmax = tv;
            } else if (isinf(tv)) {
                any_infs = 1;
                if (tv > 0) {
                    any_infp = 1;
                } else {
                    any_infm = 1;
                }
            }

            h3 = h3->next;
        }
        if (isnan(tmin) || isnan(tmax))
            tmin = tmax = 0;

        if (any_infs) {
            double tdelta = (tmax - tmin) * WAVE_INF_SCALING;

            if (any_infp)
                tmax = tmax + tdelta;
            if (any_infm)
                tmin = tmin - tdelta;
        }

        if ((tmax - tmin) < 1e-20) {
            tmax = 1;
            tmin -= 0.5 * (_y1 - _y0);
        } else {
            tmax = (_y1 - _y0) / (tmax - tmin);
        }
    }

    if (GLOBALS->tims.last - GLOBALS->tims.start < GLOBALS->wavewidth) {
        rmargin = (GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns;
    } else {
        rmargin = GLOBALS->wavewidth;
    }

    h3 = NULL;
    for (;;) {
        if (!h)
            break;
        tim = (h->time);
        if ((tim > GLOBALS->tims.end) || (tim > GLOBALS->tims.last))
            break;

        _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;

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

        h2 = h->next;
        if (!h2)
            break;
        h2tim = tim = (h2->time);
        if (tim > GLOBALS->tims.last)
            tim = GLOBALS->tims.last;
        /*	else if(tim>GLOBALS->tims.end+1) tim=GLOBALS->tims.end+1; */
        _x1 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;

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
        type = vtype2(t, h);
        tv = convert_real(t, h);
        tv2 = convert_real(t, h2);

        if ((is_inf = isinf(tv))) {
            if (tv < 0) {
                yt0 = _y0;
            } else {
                yt0 = _y1;
            }
        } else if ((is_nan = isnan(tv))) {
            yt0 = yu;
        } else {
            yt0 = _y0 + (tv - tmin) * tmax;
        }

        if ((is_inf2 = isinf(tv2))) {
            if (tv2 < 0) {
                yt1 = _y0;
            } else {
                yt1 = _y1;
            }
        } else if ((is_nan2 = isnan(tv2))) {
            yt1 = yu;
        } else {
            yt1 = _y0 + (tv2 - tmin) * tmax;
        }

        if ((_x0 != _x1) ||
            (skipcnt < GLOBALS->analog_redraw_skip_count)) /* lower number = better performance */
        {
            if (_x0 == _x1) {
                skipcnt++;
            } else {
                skipcnt = 0;
            }

            if (type != AN_X) {
                c = colors->stroke_vector;
            } else {
                c = colors->stroke_x;
            }

            if (h->next) {
                if (h->next->time > GLOBALS->max_time) {
                    yt1 = yt0;
                }
            }

            cfixed = is_inf ? cinf : c;
            if ((is_nan2) && (h2tim > GLOBALS->max_time))
                is_nan2 = 0;

            /* clamp to top/bottom because of integer rounding errors */

            if (yt0 < _y1)
                yt0 = _y1;
            else if (yt0 > _y0)
                yt0 = _y0;

            if (yt1 < _y1)
                yt1 = _y1;
            else if (yt1 > _y0)
                yt1 = _y0;

            /* clipping... */
            {
                int coords[4];
                int rect[4];

                if (_x0 < INT_MIN) {
                    coords[0] = INT_MIN;
                } else if (_x0 > INT_MAX) {
                    coords[0] = INT_MAX;
                } else {
                    coords[0] = _x0;
                }

                if (_x1 < INT_MIN) {
                    coords[2] = INT_MIN;
                } else if (_x1 > INT_MAX) {
                    coords[2] = INT_MAX;
                } else {
                    coords[2] = _x1;
                }

                coords[1] = yt0;
                coords[3] = yt1;

                rect[0] = -10;
                rect[1] = _y1;
                rect[2] = GLOBALS->wavewidth + 10;
                rect[3] = _y0;

                if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) != TR_ANALOG_STEP) {
                    wave_lineclip(coords, rect);
                } else {
                    if (coords[0] < rect[0])
                        coords[0] = rect[0];
                    if (coords[2] < rect[0])
                        coords[2] = rect[0];

                    if (coords[0] > rect[2])
                        coords[0] = rect[2];
                    if (coords[2] > rect[2])
                        coords[2] = rect[2];

                    if (coords[1] < rect[1])
                        coords[1] = rect[1];
                    if (coords[3] < rect[1])
                        coords[3] = rect[1];

                    if (coords[1] > rect[3])
                        coords[1] = rect[3];
                    if (coords[3] > rect[3])
                        coords[3] = rect[3];
                }

                _x0 = coords[0];
                yt0 = coords[1];
                _x1 = coords[2];
                yt1 = coords[3];
            }
            /* ...clipping */

            if (is_nan || is_nan2) {
                if (is_nan) {
                    XXX_gdk_draw_rectangle(cr, cnan, TRUE, _x0, _y1, _x1 - _x0, _y0 - _y1);

                    if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) ==
                        (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) {
                        XXX_gdk_draw_line(cr, ci, _x1 - 1, yt1, _x1 + 1, yt1);
                        XXX_gdk_draw_line(cr, ci, _x1, yt1 - 1, _x1, yt1 + 1);

                        XXX_gdk_draw_line(cr, ci, _x0 - 1, _y0, _x0 + 1, _y0);
                        XXX_gdk_draw_line(cr, ci, _x0, _y0 - 1, _x0, _y0 + 1);

                        XXX_gdk_draw_line(cr, ci, _x0 - 1, _y1, _x0 + 1, _y1);
                        XXX_gdk_draw_line(cr, ci, _x0, _y1 - 1, _x0, _y1 + 1);
                    }
                }
                if (is_nan2) {
                    XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt0);

                    if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) ==
                        (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) {
                        XXX_gdk_draw_line(cr, cnan, _x1, yt1, _x1, yt0);

                        XXX_gdk_draw_line(cr, ci, _x1 - 1, _y0, _x1 + 1, _y0);
                        XXX_gdk_draw_line(cr, ci, _x1, _y0 - 1, _x1, _y0 + 1);

                        XXX_gdk_draw_line(cr, ci, _x1 - 1, _y1, _x1 + 1, _y1);
                        XXX_gdk_draw_line(cr, ci, _x1, _y1 - 1, _x1, _y1 + 1);
                    }
                }
            } else if ((t->flags & TR_ANALOG_INTERPOLATED) && !is_inf && !is_inf2) {
                if (t->flags & TR_ANALOG_STEP) {
                    XXX_gdk_draw_line(cr, ci, _x0 - 1, yt0, _x0 + 1, yt0);
                    XXX_gdk_draw_line(cr, ci, _x0, yt0 - 1, _x0, yt0 + 1);
                }

                if (rmargin != GLOBALS->wavewidth) /* the window is clipped in postscript */
                {
                    if ((yt0 == yt1) && ((_x0 > _x1) || (_x0 < 0))) {
                        XXX_gdk_draw_line(cr, cfixed, 0, yt0, _x1, yt1);
                    } else {
                        XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt1);
                    }
                } else {
                    XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt1);
                }
            } else
            /* if(t->flags & TR_ANALOG_STEP) */
            {
                XXX_gdk_draw_line(cr, cfixed, _x0, yt0, _x1, yt0);

                if (is_inf2)
                    cfixed = cinf;
                XXX_gdk_draw_line(cr, cfixed, _x1, yt0, _x1, yt1);

                if ((t->flags & (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) ==
                    (TR_ANALOG_INTERPOLATED | TR_ANALOG_STEP)) {
                    XXX_gdk_draw_line(cr, ci, _x0 - 1, yt0, _x0 + 1, yt0);
                    XXX_gdk_draw_line(cr, ci, _x0, yt0 - 1, _x0, yt0 + 1);
                }
            }
        } else {
            newtime = (((gdouble)(_x1 + WAVE_OPT_SKIP)) * GLOBALS->nspx) +
                      GLOBALS->tims.start /*+GLOBALS->shift_timebase*/; /* skip to next pixel */
            h3 = bsearch_vector(t->n.vec, newtime);
            if (h3->time > h->time) {
                h = h3;
                /* lasttype=type; */
                continue;
            }
        }

        h = h->next;
        /* lasttype=type; */
    }

    GLOBALS->tims.start += GLOBALS->shift_timebase;
    GLOBALS->tims.end += GLOBALS->shift_timebase;
}

/*
 * draw vector traces
 */
static void draw_vptr_trace(GwWaveView *self,
                            cairo_t *cr,
                            GwWaveformColors *colors,
                            GwTrace *t,
                            GwVectorEnt *v,
                            int which)
{
    GwTime _x0, _x1, newtime, width;
    int _y0, _y1, yu, liney, ytext;
    GwTime tim /* , h2tim */; /* scan-build */
    GwVectorEnt *h;
    GwVectorEnt *h2;
    GwVectorEnt *h3;
    char *ascii = NULL;
    int type;
    int lasttype = -1;
    GwColor c;

    GLOBALS->tims.start -= GLOBALS->shift_timebase;
    GLOBALS->tims.end -= GLOBALS->shift_timebase;

    liney = ((which + 2) * GLOBALS->fontheight) - 2;
    _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
    _y0 = liney - 2;
    yu = (_y0 + _y1) / 2;
    ytext = yu - (GLOBALS->wavefont->ascent / 2) + GLOBALS->wavefont->ascent;

    if ((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
        (!GLOBALS->black_and_white)) {
        GwTrace *tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
            XXX_gdk_draw_rectangle(cr,
                                   colors->grid,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        } else {
            XXX_gdk_draw_rectangle(cr,
                                   colors->grid,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        }
    } else if ((GLOBALS->display_grid) && (GLOBALS->enable_horiz_grid)) {
        GwTrace *tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
        } else {
            XXX_gdk_draw_line(cr,
                              colors->grid,
                              (GLOBALS->tims.start < GLOBALS->tims.first)
                                  ? (GLOBALS->tims.first - GLOBALS->tims.start) * GLOBALS->pxns
                                  : 0,
                              liney,
                              (GLOBALS->tims.last <= GLOBALS->tims.end)
                                  ? (GLOBALS->tims.last - GLOBALS->tims.start) * GLOBALS->pxns
                                  : GLOBALS->wavewidth - 1,
                              liney);
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

    if (t->flags & TR_ANALOGMASK) {
        GwTrace *te = GiveNextTrace(t);
        int ext = 0;

        while (te) {
            if (te->flags & TR_ANALOG_BLANK_STRETCH) {
                ext++;
                te = GiveNextTrace(te);
            } else {
                break;
            }
        }

        if ((ext) && (GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
            (!GLOBALS->black_and_white)) {
            XXX_gdk_draw_rectangle(cr,
                                   colors->grid,
                                   TRUE,
                                   0,
                                   liney,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight * ext);
        }

        draw_vptr_trace_analog(self, cr, colors, t, v, which, ext);

        GLOBALS->tims.start += GLOBALS->shift_timebase;
        GLOBALS->tims.end += GLOBALS->shift_timebase;
        return;
    }

    GLOBALS->color_active_in_filter = 1;

    for (;;) {
        if (!h)
            break;
        tim = (h->time);
        if ((tim > GLOBALS->tims.end) || (tim > GLOBALS->tims.last))
            break;

        _x0 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
        if (_x0 < -1) {
            _x0 = -1;
        } else if (_x0 > GLOBALS->wavewidth) {
            break;
        }

        h2 = h->next;
        if (!h2)
            break;
        /* h2tim= */ tim = (h2->time); /* scan-build */
        if (tim > GLOBALS->tims.last)
            tim = GLOBALS->tims.last;
        else if (tim > GLOBALS->tims.end + 1)
            tim = GLOBALS->tims.end + 1;
        _x1 = (tim - GLOBALS->tims.start) * GLOBALS->pxns;
        if (_x1 < -1) {
            _x1 = -1;
        } else if (_x1 > GLOBALS->wavewidth) {
            _x1 = GLOBALS->wavewidth;
        }

        /* draw trans */
        type = vtype2(t, h);

        if (_x0 > -1) {
            GwColor gltype, gtype;

            switch (lasttype) {
                case AN_X:
                    gltype = colors->stroke_x;
                    break;
                case AN_U:
                    gltype = colors->stroke_u;
                    break;
                default:
                    gltype = colors->stroke_vector;
                    break;
            }
            switch (type) {
                case AN_X:
                    gtype = colors->stroke_x;
                    break;
                case AN_U:
                    gtype = colors->stroke_u;
                    break;
                default:
                    gtype = colors->stroke_vector;
                    break;
            }

            if (GLOBALS->use_roundcaps) {
                if (type == AN_Z) {
                    if (lasttype != -1) {
                        XXX_gdk_draw_line(cr, gltype, _x0 - 1, _y0, _x0, yu);
                        if (lasttype != AN_0)
                            XXX_gdk_draw_line(cr, gltype, _x0, yu, _x0 - 1, _y1);
                    }
                } else if (lasttype == AN_Z) {
                    XXX_gdk_draw_line(cr, gtype, _x0 + 1, _y0, _x0, yu);
                    if (type != AN_0)
                        XXX_gdk_draw_line(cr, gtype, _x0, yu, _x0 + 1, _y1);
                } else {
                    if (lasttype != type) {
                        XXX_gdk_draw_line(cr, gltype, _x0 - 1, _y0, _x0, yu);
                        if (lasttype != AN_0)
                            XXX_gdk_draw_line(cr, gltype, _x0, yu, _x0 - 1, _y1);
                        XXX_gdk_draw_line(cr, gtype, _x0 + 1, _y0, _x0, yu);
                        if (type != AN_0)
                            XXX_gdk_draw_line(cr, gtype, _x0, yu, _x0 + 1, _y1);
                    } else {
                        XXX_gdk_draw_line(cr, gtype, _x0 - 2, _y0, _x0 + 2, _y1);
                        XXX_gdk_draw_line(cr, gtype, _x0 + 2, _y0, _x0 - 2, _y1);
                    }
                }
            } else {
                XXX_gdk_draw_line(cr, gtype, _x0, _y0, _x0, _y1);
            }
        }

        if (_x0 != _x1) {
            if (type == AN_Z) {
                if (GLOBALS->use_roundcaps) {
                    XXX_gdk_draw_line(cr, colors->stroke_z, _x0 + 1, yu, _x1 - 1, yu);
                } else {
                    XXX_gdk_draw_line(cr, colors->stroke_z, _x0, yu, _x1, yu);
                }
            } else {
                if ((type != AN_X) && (type != AN_U)) {
                    c = colors->stroke_vector;
                } else {
                    c = colors->stroke_x;
                }

                if (GLOBALS->use_roundcaps) {
                    XXX_gdk_draw_line(cr, c, _x0 + 2, _y0, _x1 - 2, _y0);
                    if (type != AN_0)
                        XXX_gdk_draw_line(cr, c, _x0 + 2, _y1, _x1 - 2, _y1);
                    if (type == AN_1)
                        XXX_gdk_draw_line(cr, c, _x0 + 2, _y1 + 1, _x1 - 2, _y1 + 1);
                } else {
                    XXX_gdk_draw_line(cr, c, _x0, _y0, _x1, _y0);
                    if (type != AN_0)
                        XXX_gdk_draw_line(cr, c, _x0, _y1, _x1, _y1);
                    if (type == AN_1)
                        XXX_gdk_draw_line(cr, c, _x0, _y1 + 1, _x1, _y1 + 1);
                }

                if (_x0 < 0)
                    _x0 = 0; /* fixup left margin */

                if ((width = _x1 - _x0) > GLOBALS->vector_padding) {
                    char *ascii2;

                    ascii = convert_ascii(t, h);

                    ascii2 = ascii;
                    if (*ascii == '?') {
                        GwColor cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                ascii2 = srch_for_color + 1;
                                if (!GLOBALS->black_and_white)
                                    XXX_gdk_draw_rectangle(cr,
                                                           cb,
                                                           TRUE,
                                                           _x0 + 1,
                                                           _y1 + 1,
                                                           width - 1,
                                                           (_y0 - 1) - (_y1 + 1) + 1);
                                GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1 = 1;
                            } else {
                                *srch_for_color = '?'; /* replace name as color is a miss */
                            }
                        }
                    }

                    if ((_x1 >= GLOBALS->wavewidth) ||
                        (font_engine_string_measure(GLOBALS->wavefont, ascii2) +
                             GLOBALS->vector_padding <=
                         width)) {
                        XXX_font_engine_draw_string(cr,
                                                    GLOBALS->wavefont,
                                                    &colors->value_text,
                                                    _x0 + 2,
                                                    ytext,
                                                    ascii2);
                    } else {
                        char *mod;

                        mod = bsearch_trunc(ascii2, width - GLOBALS->vector_padding);
                        if (mod) {
                            *mod = '+';
                            *(mod + 1) = 0;

                            XXX_font_engine_draw_string(cr,
                                                        GLOBALS->wavefont,
                                                        &colors->value_text,
                                                        _x0 + 2,
                                                        ytext,
                                                        ascii2);
                        }
                    }

                } else if (GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1) {
                    /* char *ascii2; */ /* scan-build */

                    ascii = convert_ascii(t, h);

                    /* ascii2 = ascii; */ /* scan-build */
                    if (*ascii == '?') {
                        GwColor cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                /* ascii2 =  srch_for_color + 1; */
                                if (!gw_color_equal(&colors->background, &GW_COLOR_WHITE)) {
                                    if (!GLOBALS->black_and_white)
                                        XXX_gdk_draw_rectangle(cr,
                                                               cb,
                                                               TRUE,
                                                               _x0,
                                                               _y1 + 1,
                                                               width,
                                                               (_y0 - 1) - (_y1 + 1) + 1);
                                }
                            } else {
                                *srch_for_color = '?'; /* replace name as color is a miss */
                            }
                        }
                    }
                }
            }
        } else {
            newtime = (((gdouble)(_x1 + WAVE_OPT_SKIP)) * GLOBALS->nspx) +
                      GLOBALS->tims.start /*+GLOBALS->shift_timebase*/; /* skip to next pixel */
            h3 = bsearch_vector(t->n.vec, newtime);
            if (h3->time > h->time) {
                h = h3;
                lasttype = type;
                continue;
            }
        }

        if (ascii) {
            free_2(ascii);
            ascii = NULL;
        }
        lasttype = type;
        h = h->next;
    }

    GLOBALS->color_active_in_filter = 0;

    GLOBALS->tims.start += GLOBALS->shift_timebase;
    GLOBALS->tims.end += GLOBALS->shift_timebase;
}
