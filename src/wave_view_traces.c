#include <config.h>
#include <gtk/gtk.h>
#include "wave_view_traces.h"
#include "globals.h"
#include "signal_list.h"

static void draw_hptr_trace(cairo_t *cr, Trptr t, hptr h, int which, int dodraw, int kill_grid);
static void draw_hptr_trace_vector(cairo_t *cr, Trptr t, hptr h, int which);
static void draw_vptr_trace(cairo_t *cr, Trptr t, vptr v, int which);

static void gc_save(Trptr t, struct wave_rgbmaster_t *gc_sav)
{
    if ((!GLOBALS->black_and_white) && (t->t_color)) {
        int color = t->t_color;

        color--;

        memcpy(gc_sav, &GLOBALS->rgb_gc, sizeof(struct wave_rgbmaster_t));

        if (color < WAVE_NUM_RAINBOW) {
            XXX_set_alternate_gcs(GLOBALS->rgb_gc_rainbow[2 * color],
                                  GLOBALS->rgb_gc_rainbow[2 * color + 1]);
        }
    }
}

static void gc_restore(Trptr t, struct wave_rgbmaster_t *gc_sav)
{
    if ((!GLOBALS->black_and_white) && (t->t_color)) {
        memcpy(&GLOBALS->rgb_gc, gc_sav, sizeof(struct wave_rgbmaster_t));
    }
}

void rendertraces(cairo_t *cr)
{
    struct wave_rgbmaster_t gc_sav;

    Trptr t = gw_signal_list_get_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), 0);
    if (t) {
        Trptr tback = t;
        hptr h;
        vptr v;
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
            if (!(t->flags & (TR_EXCLUDE | TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                GLOBALS->shift_timebase = t->shift;
                if (!t->vector) {
                    h = bsearch_node(t->n.nd, GLOBALS->tims.start - t->shift);
                    DEBUG(printf("Start time: " TTFormat ", Histent time: " TTFormat "\n",
                                 GLOBALS->tims.start,
                                 (h->time + GLOBALS->shift_timebase)));

                    if (!t->n.nd->extvals) {
                        if (i >= 0) {
                            gc_save(t, &gc_sav);
                            draw_hptr_trace(cr, t, h, i, 1, 0);
                            gc_restore(t, &gc_sav);
                        }
                    } else {
                        if (i >= 0) {
                            gc_save(t, &gc_sav);
                            draw_hptr_trace_vector(cr, t, h, i);
                            gc_restore(t, &gc_sav);
                        }
                    }
                } else {
                    Trptr t_orig, tn;
                    bvptr bv = t->n.vec;

                    v = bsearch_vector(bv, GLOBALS->tims.start - t->shift);
                    DEBUG(printf("Vector Trace: %s, %s\n", t->name, bv->bvname));
                    DEBUG(printf("Start time: " TTFormat ", Vectorent time: " TTFormat "\n",
                                 GLOBALS->tims.start,
                                 (v->time + GLOBALS->shift_timebase)));
                    if (i >= 0) {
                        gc_save(t, &gc_sav);
                        draw_vptr_trace(cr, t, v, i);
                        gc_restore(t, &gc_sav);
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
                                        gc_save(t, &gc_sav);
                                        draw_vptr_trace(cr, t_orig, v, i);
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
            } else {
                int kill_dodraw_grid = t->flags & TR_ANALOG_BLANK_STRETCH;

                if (kill_dodraw_grid) {
                    Trptr tn = GiveNextTrace(t);
                    if (!tn) {
                        kill_dodraw_grid = 0;
                    } else if (!(tn->flags & TR_ANALOG_BLANK_STRETCH)) {
                        kill_dodraw_grid = 0;
                    }
                }

                if (i >= 0) {
                    gc_save(t, &gc_sav);
                    draw_hptr_trace(cr, NULL, NULL, i, 0, kill_dodraw_grid);
                    gc_restore(t, &gc_sav);
                }
            }
            t = GiveNextTrace(t);
            /* bot:		1; */
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
static void draw_hptr_trace(cairo_t *cr, Trptr t, hptr h, int which, int dodraw, int kill_grid)
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
                               GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                               TRUE,
                               0,
                               liney - GLOBALS->fontheight,
                               GLOBALS->wavewidth,
                               GLOBALS->fontheight);
    } else if ((GLOBALS->display_grid) && (GLOBALS->enable_horiz_grid) && (!kill_grid)) {
        XXX_gdk_draw_line(cr,
                          GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
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
                    c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                    break;
                case AN_U:
                    c = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                    break;
                case AN_W:
                    c = GLOBALS->rgb_gc.gc_w_wavewindow_c_1;
                    break;
                case AN_DASH:
                    c = GLOBALS->rgb_gc.gc_dash_wavewindow_c_1;
                    break;
                default:
                    c = (h->v.h_val == AN_X) ? GLOBALS->rgb_gc.gc_x_wavewindow_c_1
                                             : GLOBALS->rgb_gc.gc_trans_wavewindow_c_1;
            }
            XXX_gdk_draw_line(cr, c, 0, _y0, 0, _y1);
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
                        XXX_gdk_draw_line(cr,
                                          GLOBALS->rgb_gc.gc_w_wavewindow_c_1,
                                          _x0,
                                          _y0,
                                          _x0,
                                          _y1);
                        XXX_gdk_draw_line(cr,
                                          GLOBALS->rgb_gc.gc_w_wavewindow_c_1,
                                          _x0,
                                          _y1,
                                          _x0 + 2,
                                          _y1 + 2);
                        XXX_gdk_draw_line(cr,
                                          GLOBALS->rgb_gc.gc_w_wavewindow_c_1,
                                          _x0,
                                          _y1,
                                          _x0 - 2,
                                          _y1 + 2);
                    }
                    h = h->next;
                    continue;
                }

                hval = h->v.h_val;
                h2val = h2->v.h_val;

                switch (h2val) {
                    case AN_X:
                        c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                        break;
                    case AN_U:
                        c = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                        break;
                    case AN_W:
                        c = GLOBALS->rgb_gc.gc_w_wavewindow_c_1;
                        break;
                    case AN_DASH:
                        c = GLOBALS->rgb_gc.gc_dash_wavewindow_c_1;
                        break;
                    default:
                        c = (hval == AN_X) ? GLOBALS->rgb_gc.gc_x_wavewindow_c_1
                                           : GLOBALS->rgb_gc.gc_trans_wavewindow_c_1;
                }

                switch (hval) {
                    case AN_0: /* 0 */
                    case AN_L: /* L */
                        if (GLOBALS->fill_waveform && invert) {
                            switch (hval) {
                                case AN_0:
                                    gcxf = GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1;
                                    break;
                                case AN_L:
                                    gcxf = GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1;
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
                        XXX_gdk_draw_line(cr,
                                          (hval == AN_0) ? GLOBALS->rgb_gc.gc_0_wavewindow_c_1
                                                         : GLOBALS->rgb_gc.gc_low_wavewindow_c_1,
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
                                    XXX_gdk_draw_line(cr, c, _x1, _y0, _x1, yu);
                                    break;
                                default:
                                    XXX_gdk_draw_line(cr, c, _x1, _y0, _x1, _y1);
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
                                c = gcx = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                                gcxf = GLOBALS->rgb_gc.gc_xfill_wavewindow_c_1;
                                identifier_str[0] = 0;
                                break;
                            case AN_W:
                                c = gcx = GLOBALS->rgb_gc.gc_w_wavewindow_c_1;
                                gcxf = GLOBALS->rgb_gc.gc_wfill_wavewindow_c_1;
                                identifier_str[0] = 'W';
                                break;
                            case AN_U:
                                c = gcx = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                                gcxf = GLOBALS->rgb_gc.gc_ufill_wavewindow_c_1;
                                identifier_str[0] = 'U';
                                break;
                            default:
                                c = gcx = GLOBALS->rgb_gc.gc_dash_wavewindow_c_1;
                                gcxf = GLOBALS->rgb_gc.gc_dashfill_wavewindow_c_1;
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
                                    XXX_font_engine_draw_string(
                                        cr,
                                        GLOBALS->wavefont,
                                        &GLOBALS->rgb_gc.gc_value_wavewindow_c_1,
                                        _x0 + 2 + GLOBALS->cairo_050_offset,
                                        ytext + GLOBALS->cairo_050_offset,
                                        identifier_str);
                                }
                            }
                        }

                        XXX_gdk_draw_line(cr, gcx, _x0, _y0, _x1, _y0);
                        XXX_gdk_draw_line(cr, gcx, _x0, _y1, _x1, _y1);
                        if (h2tim <= GLOBALS->tims.end)
                            XXX_gdk_draw_line(cr, c, _x1, _y0, _x1, _y1);
                        break;

                    case AN_Z: /* Z */
                        XXX_gdk_draw_line(cr,
                                          GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,
                                          _x0,
                                          yu,
                                          _x1,
                                          yu);
                        if (h2tim <= GLOBALS->tims.end)
                            switch (h2val) {
                                case AN_0:
                                case AN_L:
                                    XXX_gdk_draw_line(cr, c, _x1, yu, _x1, _y0);
                                    break;
                                case AN_1:
                                case AN_H:
                                    XXX_gdk_draw_line(cr, c, _x1, yu, _x1, _y1);
                                    break;
                                default:
                                    XXX_gdk_draw_line(cr, c, _x1, _y0, _x1, _y1);
                                    break;
                            }
                        break;

                    case AN_1: /* 1 */
                    case AN_H: /* 1 */
                        if (GLOBALS->fill_waveform && !invert) {
                            switch (hval) {
                                case AN_1:
                                    gcxf = GLOBALS->rgb_gc.gc_1fill_wavewindow_c_1;
                                    break;
                                case AN_H:
                                    gcxf = GLOBALS->rgb_gc.gc_highfill_wavewindow_c_1;
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
                        XXX_gdk_draw_line(cr,
                                          (hval == AN_1) ? GLOBALS->rgb_gc.gc_1_wavewindow_c_1
                                                         : GLOBALS->rgb_gc.gc_high_wavewindow_c_1,
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
                                    XXX_gdk_draw_line(cr, c, _x1, _y1, _x1, _y0);
                                    break;
                                case AN_Z:
                                    XXX_gdk_draw_line(cr, c, _x1, _y1, _x1, yu);
                                    break;
                                default:
                                    XXX_gdk_draw_line(cr, c, _x1, _y0, _x1, _y1);
                                    break;
                            }
                        break;

                    default:
                        break;
                }
            } else {
                if (!is_event) {
                    XXX_gdk_draw_line(cr,
                                      GLOBALS->rgb_gc.gc_trans_wavewindow_c_1,
                                      _x1,
                                      _y0,
                                      _x1,
                                      _y1);
                } else {
                    XXX_gdk_draw_line(cr, GLOBALS->rgb_gc.gc_w_wavewindow_c_1, _x1, _y0, _x1, _y1);
                    XXX_gdk_draw_line(cr,
                                      GLOBALS->rgb_gc.gc_w_wavewindow_c_1,
                                      _x0,
                                      _y1,
                                      _x0 + 2,
                                      _y1 + 2);
                    XXX_gdk_draw_line(cr,
                                      GLOBALS->rgb_gc.gc_w_wavewindow_c_1,
                                      _x0,
                                      _y1,
                                      _x0 - 2,
                                      _y1 + 2);
                }
                newtime = (((gdouble)(_x1 + WAVE_OPT_SKIP)) * GLOBALS->nspx) +
                          GLOBALS->tims.start /*+GLOBALS->shift_timebase*/; /* skip to next pixel */
                h3 = bsearch_node(t->n.nd, newtime);
                if (h3->time > h->time) {
                    h = h3;
                    continue;
                }
            }

            if ((h->flags & HIST_GLITCH) && (GLOBALS->vcd_preserve_glitches)) {
                XXX_gdk_draw_rectangle(cr,
                                       GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,
                                       TRUE,
                                       _x1 - 1,
                                       yu - 1,
                                       3,
                                       3);
            }

            h = h->next;
        }

    GLOBALS->tims.start += GLOBALS->shift_timebase;
    GLOBALS->tims.end += GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_hptr_trace_vector_analog(cairo_t *cr,
                                          Trptr t,
                                          hptr h,
                                          int which,
                                          int num_extension)
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
                    if (h3->flags & HIST_REAL) {
#ifdef WAVE_HAS_H_DOUBLE
                        if (!(h3->flags & HIST_STRING))
                            tv = h3->v.h_double;
#else
                        if (!(h3->flags & HIST_STRING) && h3->v.h_vector)
                            tv = *(double *)h3->v.h_vector;
#endif
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
            if (h3->flags & HIST_REAL) {
#ifdef WAVE_HAS_H_DOUBLE
                if (!(h3->flags & HIST_STRING))
                    tv = h3->v.h_double;
#else
                if (!(h3->flags & HIST_STRING) && h3->v.h_vector)
                    tv = *(double *)h3->v.h_vector;
#endif
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
        type = (!(h->flags & (HIST_REAL | HIST_STRING))) ? vtype(t, h->v.h_vector) : AN_COUNT;
        tv = tv2 = mynan;

        if (h->flags & HIST_REAL) {
#ifdef WAVE_HAS_H_DOUBLE
            if (!(h->flags & HIST_STRING))
                tv = h->v.h_double;
#else
            if (!(h->flags & HIST_STRING) && h->v.h_vector)
                tv = *(double *)h->v.h_vector;
#endif
        } else {
            if (h->time <= GLOBALS->tims.last)
                tv = convert_real_vec(t, h->v.h_vector);
        }

        if (h2->flags & HIST_REAL) {
#ifdef WAVE_HAS_H_DOUBLE
            if (!(h2->flags & HIST_STRING))
                tv2 = h2->v.h_double;
#else
            if (!(h2->flags & HIST_STRING) && h2->v.h_vector)
                tv2 = *(double *)h2->v.h_vector;
#endif
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
                c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
            } else {
                c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
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
static void draw_hptr_trace_vector(cairo_t *cr, Trptr t, hptr h, int which)
{
    TimeType _x0, _x1, newtime, width;
    int _y0, _y1, yu, liney, ytext;
    TimeType tim /* , h2tim */; /* scan-build */
    hptr h2, h3;
    char *ascii = NULL;
    int type;
    int lasttype = -1;
    wave_rgb_t c;

    GLOBALS->tims.start -= GLOBALS->shift_timebase;
    GLOBALS->tims.end -= GLOBALS->shift_timebase;

    liney = ((which + 2) * GLOBALS->fontheight) - 2;
    _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
    _y0 = liney - 2;
    yu = (_y0 + _y1) / 2;
    ytext = yu - (GLOBALS->wavefont->ascent / 2) + GLOBALS->wavefont->ascent;

    if ((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
        (!GLOBALS->black_and_white)) {
        Trptr tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
            XXX_gdk_draw_rectangle(cr,
                                   GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        } else {
            XXX_gdk_draw_rectangle(cr,
                                   GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        }
    } else if ((GLOBALS->display_grid) && (GLOBALS->enable_horiz_grid)) {
        Trptr tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
        } else {
            XXX_gdk_draw_line(cr,
                              GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
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

    if ((t->flags & TR_ANALOGMASK) && (!(h->flags & HIST_STRING) || !(h->flags & HIST_REAL))) {
        Trptr te = GiveNextTrace(t);
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
                                   GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                                   TRUE,
                                   0,
                                   liney,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight * ext);
        }

        draw_hptr_trace_vector_analog(cr, t, h, which, ext);
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
        if (!(h->flags & (HIST_REAL | HIST_STRING))) {
            type = vtype(t, h->v.h_vector);
        } else {
            /* s\000 ID is special "z" case */
            type = AN_COUNT;

            if (h->flags & HIST_STRING) {
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
        /* type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.h_vector) : AN_COUNT; */
        if (_x0 > -1) {
            wave_rgb_t gltype, gtype;

            switch (lasttype) {
                case AN_X:
                    gltype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                    break;
                case AN_U:
                    gltype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                    break;
                default:
                    gltype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1;
                    break;
            }
            switch (type) {
                case AN_X:
                    gtype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                    break;
                case AN_U:
                    gtype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                    break;
                default:
                    gtype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1;
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
                    XXX_gdk_draw_line(cr,
                                      GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,
                                      _x0 + 1,
                                      yu,
                                      _x1 - 1,
                                      yu);
                } else {
                    XXX_gdk_draw_line(cr, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1, _x0, yu, _x1, yu);
                }
            } else {
                if ((type != AN_X) && (type != AN_U)) {
                    c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
                } else {
                    c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
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

                    if (h->flags & HIST_REAL) {
                        if (!(h->flags & HIST_STRING)) {
#ifdef WAVE_HAS_H_DOUBLE
                            ascii = convert_ascii_real(t, &h->v.h_double);
#else
                            ascii = convert_ascii_real(t, (double *)h->v.h_vector);
#endif
                        } else {
                            ascii = convert_ascii_string((char *)h->v.h_vector);
                        }
                    } else {
                        ascii = convert_ascii_vec(t, h->v.h_vector);
                    }

                    ascii2 = ascii;
                    if (*ascii == '?') {
                        wave_rgb_t cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                ascii2 = srch_for_color + 1;
                                if (memcmp(&GLOBALS->rgb_gc.gc_back_wavewindow_c_1,
                                           &GLOBALS->rgb_gc_white,
                                           sizeof(wave_rgb_t))) {
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
                                                    &GLOBALS->rgb_gc.gc_value_wavewindow_c_1,
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
                                                        &GLOBALS->rgb_gc.gc_value_wavewindow_c_1,
                                                        _x0 + 2 + GLOBALS->cairo_050_offset,
                                                        ytext + GLOBALS->cairo_050_offset,
                                                        ascii2);
                        }
                    }
                } else if (GLOBALS->fill_in_smaller_rgb_areas_wavewindow_c_1) {
                    /* char *ascii2; */ /* scan-build */

                    if (h->flags & HIST_REAL) {
                        if (!(h->flags & HIST_STRING)) {
#ifdef WAVE_HAS_H_DOUBLE
                            ascii = convert_ascii_real(t, &h->v.h_double);
#else
                            ascii = convert_ascii_real(t, (double *)h->v.h_vector);
#endif
                        } else {
                            ascii = convert_ascii_string((char *)h->v.h_vector);
                        }
                    } else {
                        ascii = convert_ascii_vec(t, h->v.h_vector);
                    }

                    /* ascii2 = ascii; */ /* scan-build */
                    if (*ascii == '?') {
                        wave_rgb_t cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                /* ascii2 =  srch_for_color + 1; */ /* scan-build */
                                if (memcmp(&GLOBALS->rgb_gc.gc_back_wavewindow_c_1,
                                           &GLOBALS->rgb_gc_white,
                                           sizeof(wave_rgb_t))) {
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
                lasttype = type;
                continue;
            }
        }

        if (ascii) {
            free_2(ascii);
            ascii = NULL;
        }
        h = h->next;
        lasttype = type;
    }

    GLOBALS->color_active_in_filter = 0;

    GLOBALS->tims.start += GLOBALS->shift_timebase;
    GLOBALS->tims.end += GLOBALS->shift_timebase;
}

/********************************************************************************************************/

static void draw_vptr_trace_analog(cairo_t *cr, Trptr t, vptr v, int which, int num_extension)
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
    double tmin = mynan, tmax = mynan, tv = 0.0, tv2;
    gint rmargin;
    int is_nan = 0, is_nan2 = 0, is_inf = 0, is_inf2 = 0;
    int any_infs = 0, any_infp = 0, any_infm = 0;
    int skipcnt = 0;

    ci = GLOBALS->rgb_gc.gc_baseline_wavewindow_c_1;

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
                c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
            } else {
                c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
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
static void draw_vptr_trace(cairo_t *cr, Trptr t, vptr v, int which)
{
    TimeType _x0, _x1, newtime, width;
    int _y0, _y1, yu, liney, ytext;
    TimeType tim /* , h2tim */; /* scan-build */
    vptr h, h2, h3;
    char *ascii = NULL;
    int type;
    int lasttype = -1;
    wave_rgb_t c;

    GLOBALS->tims.start -= GLOBALS->shift_timebase;
    GLOBALS->tims.end -= GLOBALS->shift_timebase;

    liney = ((which + 2) * GLOBALS->fontheight) - 2;
    _y1 = ((which + 1) * GLOBALS->fontheight) + 2;
    _y0 = liney - 2;
    yu = (_y0 + _y1) / 2;
    ytext = yu - (GLOBALS->wavefont->ascent / 2) + GLOBALS->wavefont->ascent;

    if ((GLOBALS->highlight_wavewindow) && (t) && (t->flags & TR_HIGHLIGHT) &&
        (!GLOBALS->black_and_white)) {
        Trptr tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
            XXX_gdk_draw_rectangle(cr,
                                   GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        } else {
            XXX_gdk_draw_rectangle(cr,
                                   GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                                   TRUE,
                                   0,
                                   liney - GLOBALS->fontheight,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight);
        }
    } else if ((GLOBALS->display_grid) && (GLOBALS->enable_horiz_grid)) {
        Trptr tn = GiveNextTrace(t);
        if ((t->flags & TR_ANALOGMASK) && (tn) && (tn->flags & TR_ANALOG_BLANK_STRETCH)) {
        } else {
            XXX_gdk_draw_line(cr,
                              GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
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
        Trptr te = GiveNextTrace(t);
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
                                   GLOBALS->rgb_gc.gc_grid_wavewindow_c_1,
                                   TRUE,
                                   0,
                                   liney,
                                   GLOBALS->wavewidth,
                                   GLOBALS->fontheight * ext);
        }

        draw_vptr_trace_analog(cr, t, v, which, ext);

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
            wave_rgb_t gltype, gtype;

            switch (lasttype) {
                case AN_X:
                    gltype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                    break;
                case AN_U:
                    gltype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                    break;
                default:
                    gltype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1;
                    break;
            }
            switch (type) {
                case AN_X:
                    gtype = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
                    break;
                case AN_U:
                    gtype = GLOBALS->rgb_gc.gc_u_wavewindow_c_1;
                    break;
                default:
                    gtype = GLOBALS->rgb_gc.gc_vtrans_wavewindow_c_1;
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
                    XXX_gdk_draw_line(cr,
                                      GLOBALS->rgb_gc.gc_mid_wavewindow_c_1,
                                      _x0 + 1,
                                      yu,
                                      _x1 - 1,
                                      yu);
                } else {
                    XXX_gdk_draw_line(cr, GLOBALS->rgb_gc.gc_mid_wavewindow_c_1, _x0, yu, _x1, yu);
                }
            } else {
                if ((type != AN_X) && (type != AN_U)) {
                    c = GLOBALS->rgb_gc.gc_vbox_wavewindow_c_1;
                } else {
                    c = GLOBALS->rgb_gc.gc_x_wavewindow_c_1;
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
                        wave_rgb_t cb;
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
                                                    &GLOBALS->rgb_gc.gc_value_wavewindow_c_1,
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
                                                        &GLOBALS->rgb_gc.gc_value_wavewindow_c_1,
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
                        wave_rgb_t cb;
                        char *srch_for_color = strchr(ascii + 1, '?');
                        if (srch_for_color) {
                            *srch_for_color = 0;
                            cb = XXX_get_gc_from_name(ascii + 1);
                            if (cb.a != 0.0) {
                                /* ascii2 =  srch_for_color + 1; */
                                if (memcmp(&GLOBALS->rgb_gc.gc_back_wavewindow_c_1,
                                           &GLOBALS->rgb_gc_white,
                                           sizeof(wave_rgb_t))) {
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
