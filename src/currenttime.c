/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "debug.h"
#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include "currenttime.h"
#include "gw-time-display.h"
#include "symbol.h"
#include "gw-wave-view.h"

static const char *time_prefix = WAVE_SI_UNITS;

/* handles floating point values with units */
static GwTime unformat_time_complex(const char *s, char dim)
{
    int i, delta, rc;
    unsigned char ch = dim;
    double d = 0.0;
    const char *offs = NULL;
    const char *doffs = NULL;

    rc = sscanf(s, "%lf %cs", &d, &ch);
    if (rc == 2) {
        ch = tolower(ch);
        if (ch == 's')
            ch = ' ';
        offs = strchr(time_prefix, ch);
        if (offs) {
            doffs = strchr(time_prefix, (int)dim);
            if (!doffs)
                doffs = offs; /* should *never* happen */

            delta = (doffs - time_prefix) - (offs - time_prefix);

            if (delta < 0) {
                for (i = delta; i < 0; i++) {
                    d = d / 1000;
                }
            } else {
                for (i = 0; i < delta; i++) {
                    d = d * 1000;
                }
            }
        }
    }

    return ((GwTime)d);
}

/* handles integer values with units */
static GwTime unformat_time_simple(const char *buf, char dim)
{
    GwTime rval;
    const char *pnt;
    const char *offs = NULL;
    const char *doffs;
    char ch;
    int i, ich, delta;

    rval = atoi_64(buf);
    if ((pnt = GLOBALS->atoi_cont_ptr)) {
        while ((ch = *(pnt++))) {
            if ((ch == ' ') || (ch == '\t'))
                continue;

            ich = tolower((int)ch);
            if (ich == 's')
                ich = ' '; /* as in plain vanilla seconds */

            offs = strchr(time_prefix, ich);
            break;
        }
    }

    if (!offs)
        return (rval);
    if ((dim == 'S') || (dim == 's')) {
        doffs = time_prefix;
    } else {
        doffs = strchr(time_prefix, (int)dim);
        if (!doffs)
            return (rval); /* should *never* happen */
    }

    delta = (doffs - time_prefix) - (offs - time_prefix);

    if (delta < 0) {
        for (i = delta; i < 0; i++) {
            rval = rval / 1000;
        }
    } else {
        for (i = 0; i < delta; i++) {
            rval = rval * 1000;
        }
    }

    return (rval);
}

GwTime unformat_time(const char *s, char dim)
{
    const char *compar = ".+eE";
    int compar_len = strlen(compar);
    int i;
    char *pnt;

    if ((*s == 'M') || (*s == 'm')) {
        if (bijective_marker_id_string_len(s + 1)) {
            guint mkv = bijective_marker_id_string_hash(s + 1);

            GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
            GwMarker *marker = gw_named_markers_get(markers, mkv);
            if (marker != NULL && gw_marker_is_enabled(marker)) {
                return gw_marker_get_position(marker);
            }
        }
    }

    for (i = 0; i < compar_len; i++) {
        if ((pnt = strchr(s, (int)compar[i]))) {
            if ((tolower((int)(unsigned char)pnt[0]) != 'e') &&
                (tolower((int)(unsigned char)pnt[1]) != 'c')) {
                return (unformat_time_complex(s, dim));
            }
        }
    }

    return (unformat_time_simple(s, dim));
}

void reformat_time_simple(char *buf, GwTime val, char dim)
{
    char *pnt;
    int i, offset;

    if (val < GW_TIME_CONSTANT(0)) {
        val = -val;
        buf[0] = '-';
        buf++;
    }

    pnt = strchr(time_prefix, (int)dim);
    if (pnt) {
        offset = pnt - time_prefix;
    } else
        offset = 0;

    for (i = offset; i > 0; i--) {
        if (val % 1000)
            break;
        val = val / 1000;
    }

    if (i) {
        sprintf(buf, "%" GW_TIME_FORMAT " %cs", val, time_prefix[i]);
    } else {
        sprintf(buf, "%" GW_TIME_FORMAT " sec", val);
    }
}

void reformat_time(char *buf, GwTime val, GwTimeDimension dim)
{
    const char *pnt;
    int i, offset, offsetfix;

    if (val < GW_TIME_CONSTANT(0)) {
        val = -val;
        buf[0] = '-';
        buf++;
    }

    pnt = strchr(time_prefix, (int)dim);
    if (pnt) {
        offset = pnt - time_prefix;
    } else
        offset = 0;

    for (i = offset; i > 0; i--) {
        if (val % 1000)
            break;
        val = val / 1000;
    }

    if (GLOBALS->scale_to_time_dimension) {
        if (GLOBALS->scale_to_time_dimension == 's') {
            pnt = time_prefix;
        } else {
            pnt = strchr(time_prefix, (int)GLOBALS->scale_to_time_dimension);
        }
        if (pnt) {
            offsetfix = pnt - time_prefix;
            if (offsetfix != i) {
                int j;
                int deltaexp = (offsetfix - i);
                gdouble gval = (gdouble)val;
                gdouble mypow = 1.0;

                if (deltaexp > 0) {
                    for (j = 0; j < deltaexp; j++) {
                        mypow *= 1000.0;
                    }
                } else {
                    for (j = 0; j > deltaexp; j--) {
                        mypow /= 1000.0;
                    }
                }

                gval *= mypow;

                if (GLOBALS->scale_to_time_dimension == 's') {
                    sprintf(buf, "%.9g sec", gval);
                } else {
                    sprintf(buf, "%.9g %cs", gval, GLOBALS->scale_to_time_dimension);
                }

                return;
            }
        }
    }

    if ((i) && (time_prefix)) /* scan-build on time_prefix, should not be necessary however */
    {
        sprintf(buf, "%" GW_TIME_FORMAT " %cs", val, time_prefix[i]);
    } else {
        sprintf(buf, "%" GW_TIME_FORMAT " sec", val);
    }
}

void update_time_box(void)
{
    if (GLOBALS->project == NULL || GLOBALS->dump_file == NULL) {
        return;
    }

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    GwTime time_scale = gw_dump_file_get_time_scale(GLOBALS->dump_file);
    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);

    if (GLOBALS->anno_ctx) {
        if (gw_marker_is_enabled(primary_marker)) {
            GwTime val = gw_marker_get_position(primary_marker);
            GwTime global_time_offset = gw_dump_file_get_global_time_offset(GLOBALS->dump_file);

            GLOBALS->anno_ctx->marker_set = 0; /* avoid race on update */
            GLOBALS->anno_ctx->marker = val / time_scale;

            reformat_time(GLOBALS->anno_ctx->time_string, val + global_time_offset, time_dimension);

            GLOBALS->anno_ctx->marker_set = 1;
        } else {
            GLOBALS->anno_ctx->marker_set = 0;
        }
    }

    // TODO: reenable marker locking
    // if (GLOBALS->named_marker_lock_idx > -1) {
    //     if (GLOBALS->tims.marker >= 0) {
    //         int ent_idx = GLOBALS->named_marker_lock_idx;

    //         if (GLOBALS->named_markers[ent_idx] != GLOBALS->tims.marker) {
    //             GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
    //             gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));
    //         }
    //     }
    // }
}

void update_currenttime(GwTime val)
{
    GLOBALS->cached_currenttimeval_currenttime_c_1 = val;

    GwMarker *baseline_marker = gw_project_get_baseline_marker(GLOBALS->project);
    GwMarker *cursor = gw_project_get_cursor(GLOBALS->project);

    if (gw_marker_is_enabled(baseline_marker)) {
        return;
    }

    gw_marker_set_position(cursor, val);

    update_time_box();
}

GwTime time_trunc(GwTime t)
{
    if (!GLOBALS->use_full_precision)
        if (GLOBALS->time_trunc_val_currenttime_c_1 != 1) {
            t = t / GLOBALS->time_trunc_val_currenttime_c_1;
            t = t * GLOBALS->time_trunc_val_currenttime_c_1;
            if (t < GLOBALS->tims.first)
                t = GLOBALS->tims.first;
        }

    return (t);
}

void time_trunc_set(void)
{
    gdouble gcompar = 1e15;
    GwTime compar = GW_TIME_CONSTANT(1000000000000000);

    for (; compar != 1; compar = compar / 10, gcompar = gcompar / ((gdouble)10.0)) {
        if (GLOBALS->nspx >= gcompar) {
            GLOBALS->time_trunc_val_currenttime_c_1 = compar;
            return;
        }
    }

    GLOBALS->time_trunc_val_currenttime_c_1 = 1;
}
