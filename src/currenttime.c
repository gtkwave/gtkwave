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
#include "symbol.h"
#include "wave_view.h"

static char *time_prefix = WAVE_SI_UNITS;

void fractional_timescale_fix(char *s)
{
    char buf[32], sfx[2];
    int i, len;
    int prefix_idx = 0;

    if (*s != '0') {
        char *dot = strchr(s, '.');
        char *src, *dst;
        if (dot) {
            char *pnt = dot + 1;
            int alpha_found = 0;
            while (*pnt) {
                if (isalpha(*pnt)) {
                    alpha_found = 1;
                    break;
                }
                pnt++;
            }

            if (alpha_found) {
                src = pnt;
                dst = dot;
                while (*src) {
                    *dst = *src;
                    dst++;
                    src++;
                }
                *dst = 0;
            }
        }
        return;
    }

    len = strlen(s);
    for (i = 0; i < len; i++) {
        if ((s[i] != '0') && (s[i] != '1') && (s[i] != '.')) {
            buf[i] = 0;
            prefix_idx = i;
            break;
        } else {
            buf[i] = s[i];
        }
    }

    if (!strcmp(buf, "0.1")) {
        strcpy(buf, "100");
    } else if (!strcmp(buf, "0.01")) {
        strcpy(buf, "10");
    } else if (!strcmp(buf, "0.001")) {
        strcpy(buf, "1");
    } else {
        return;
    }

    len = strlen(WAVE_SI_UNITS);
    for (i = 0; i < len - 1; i++) {
        if (s[prefix_idx] == WAVE_SI_UNITS[i])
            break;
    }

    sfx[0] = WAVE_SI_UNITS[i + 1];
    sfx[1] = 0;
    strcat(buf, sfx);
    strcat(buf, "s");
    /* printf("old time: '%s', new time: '%s'\n", s, buf); */
    strcpy(s, buf);
}

/* handles floating point values with units */
static TimeType unformat_time_complex(const char *s, char dim)
{
    int i, delta, rc;
    unsigned char ch = dim;
    double d = 0.0;
    const char *offs = NULL, *doffs = NULL;

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

    return ((TimeType)d);
}

/* handles integer values with units */
static TimeType unformat_time_simple(const char *buf, char dim)
{
    TimeType rval;
    const char *pnt;
    char *offs = NULL, *doffs;
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

TimeType unformat_time(const char *s, char dim)
{
    const char *compar = ".+eE";
    int compar_len = strlen(compar);
    int i;
    char *pnt;

    if ((*s == 'M') || (*s == 'm')) {
        if (bijective_marker_id_string_len(s + 1)) {
            unsigned int mkv = bijective_marker_id_string_hash(s + 1);
            if (mkv < WAVE_NUM_NAMED_MARKERS) {
                TimeType mkvt = GLOBALS->named_markers[mkv];
                if (mkvt != -1) {
                    return (mkvt);
                }
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

void reformat_time_simple(char *buf, TimeType val, char dim)
{
    char *pnt;
    int i, offset;

    if (val < LLDescriptor(0)) {
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
        sprintf(buf, TTFormat " %cs", val, time_prefix[i]);
    } else {
        sprintf(buf, TTFormat " sec", val);
    }
}

static int dim_to_exponent(char dim)
{
    char *pnt = strchr(time_prefix, (int)dim);
    if (pnt == NULL) {
        return 0;
    }

    return (pnt - time_prefix) * 3;
}

static gchar *reformat_time_2_scale_to_dimension(TimeType val, char dim, gboolean show_plus_sign)
{
    int value_exponent = dim_to_exponent(dim);
    int target_exponent = dim_to_exponent(GLOBALS->scale_to_time_dimension);

    double v = val;
    while (value_exponent > target_exponent) {
        v /= 1000.0;
        value_exponent -= 3;
    }
    while (value_exponent < target_exponent) {
        v *= 1000.0;
        value_exponent += 3;
    }

    gchar *str;
    if (GLOBALS->scale_to_time_dimension == 's') {
         str = g_strdup_printf("%.9g sec", v);
    } else {
        str = g_strdup_printf("%.9g %cs", v, GLOBALS->scale_to_time_dimension);
    }

    if (show_plus_sign && val >= 0) {
        gchar *t = g_strconcat("+", str, NULL);
        g_free(str);
        return t;
    } else {
        return str;
    }
}

static gchar *reformat_time_2(TimeType val, char dim, gboolean show_plus_sign)
{
    static const gunichar THIN_SPACE = 0x2009;

    if (GLOBALS->scale_to_time_dimension) {
        return reformat_time_2_scale_to_dimension(val, dim, show_plus_sign);
    }

    gboolean negative = val < 0;
    val = ABS(val);

    gchar *value_str = g_strdup_printf(TTFormat, val);
    gsize value_len = strlen(value_str);

    GString *str = g_string_new(NULL);
    if (negative) {
        g_string_append_c(str, '-');
    } else if (show_plus_sign) {
        g_string_append_c(str, '+');
    }

    gsize first_group_len = value_len % 3;
    if (first_group_len > 0) {
        g_string_append_len(str, value_str, first_group_len);
    }
    for (gsize group = 0; group < value_len / 3; group++) {
        if (group > 0 || first_group_len != 0) {
            g_string_append_unichar(str, THIN_SPACE);
        }
        g_string_append_len(str, &value_str[first_group_len + group * 3], 3);
    }

    g_string_append_c(str, ' ');

    if (dim != 0 && dim != ' ') {
        g_string_append_c(str, dim);
        g_string_append_c(str, 's');
    } else {
        g_string_append(str, "sec");
    }

    return g_string_free(str, FALSE);
}

void reformat_time(char *buf, TimeType val, char dim)
{
    char *pnt;
    int i, offset, offsetfix;

    if (val < LLDescriptor(0)) {
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
        sprintf(buf, TTFormat " %cs", val, time_prefix[i]);
    } else {
        sprintf(buf, TTFormat " sec", val);
    }
}

void reformat_time_as_frequency(char *buf, TimeType val, char dim)
{
    char *pnt;
    int offset;
    double k;

    static const double negpow[] =
        {1.0, 1.0e-3, 1.0e-6, 1.0e-9, 1.0e-12, 1.0e-15, 1.0e-18, 1.0e-21};

    pnt = strchr(time_prefix, (int)dim);
    if (pnt) {
        offset = pnt - time_prefix;
    } else
        offset = 0;

    if (val) {
        k = 1 / ((double)val * negpow[offset]);

        sprintf(buf, "%e Hz", k);
    } else {
        strcpy(buf, "-- Hz");
    }
}

static gboolean is_in_blackout_region(TimeType val)
{
    for (struct blackout_region_t *bt = GLOBALS->blackout_regions; bt != NULL; bt = bt->next) {
        if (val >= bt->bstart && val < bt->bend) {
            return TRUE;
        }
    }

    return FALSE;
}

static void update_markertime(void)
{
    TimeType val = GLOBALS->tims.marker;

    if (GLOBALS->anno_ctx) {
        if (val >= 0) {
            GLOBALS->anno_ctx->marker_set = 0; /* avoid race on update */

            if (!GLOBALS->ae2_time_xlate) {
                GLOBALS->anno_ctx->marker = val / GLOBALS->time_scale;
            } else {
                int rvs_xlate = bsearch_aetinfo_timechain(val);
                GLOBALS->anno_ctx->marker = ((TimeType)rvs_xlate) + GLOBALS->ae2_start_cyc;
            }

            reformat_time(GLOBALS->anno_ctx->time_string,
                          val + GLOBALS->global_time_offset,
                          GLOBALS->time_dimension);

            GLOBALS->anno_ctx->marker_set = 1;
        } else {
            GLOBALS->anno_ctx->marker_set = 0;
        }
    }

    gchar *text = NULL;

    if (!GLOBALS->use_maxtime_display) {
        if (val >= 0) {
            if (GLOBALS->tims.baseline >= 0) {
                val -= GLOBALS->tims.baseline; /* do delta instead */

                if (GLOBALS->use_frequency_delta) {
                    text = g_malloc0(40);
                    reformat_time_as_frequency(text, val, GLOBALS->time_dimension);
                } else {
                    text = reformat_time_2(val, GLOBALS->time_dimension, TRUE);
                }

                gchar *t = g_strconcat("B", text, NULL);
                g_free(text);
                text = t;
            } else if (GLOBALS->tims.lmbcache >= 0) {
                val -= GLOBALS->tims.lmbcache; /* do delta instead */

                if (GLOBALS->use_frequency_delta) {
                    reformat_time_as_frequency(GLOBALS->maxtext_currenttime_c_1,
                                               val,
                                               GLOBALS->time_dimension);
                } else {
                    text = reformat_time_2(val, GLOBALS->time_dimension, TRUE);
                }
            } else {
                text = reformat_time_2(val + GLOBALS->global_time_offset,
                                       GLOBALS->time_dimension,
                                       FALSE);
            }
        } else {
            text = g_strdup("--");
        }
    }

    if (text != NULL) {
        strncpy(GLOBALS->maxtext_currenttime_c_1, text, 40);
        g_free(text);
    }

    if (GLOBALS->named_marker_lock_idx > -1) {
        if (GLOBALS->tims.marker >= 0) {
            int ent_idx = GLOBALS->named_marker_lock_idx;

            if (GLOBALS->named_markers[ent_idx] != GLOBALS->tims.marker) {
                GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
                gw_wave_view_force_redraw(GW_WAVE_VIEW(GLOBALS->wavearea));
            }
        }
    }
}

void update_time_box(void)
{
    static const gchar *EM_DASH = "\xe2\x80\x94";

    GList *children = gtk_container_get_children(GTK_CONTAINER(GLOBALS->time_box));
    GtkWidget *marker_label = g_list_nth_data(children, 0);
    GtkWidget *marker_value = g_list_nth_data(children, 1);
    GtkWidget *cursor_label = g_list_nth_data(children, 3);
    GtkWidget *cursor_value = g_list_nth_data(children, 4);
    g_list_free(children);

    update_markertime();
    if (GLOBALS->use_maxtime_display) {
        gchar *text = reformat_time_2(GLOBALS->max_time + GLOBALS->global_time_offset,
                                      GLOBALS->time_dimension,
                                      FALSE);
        strncpy(GLOBALS->maxtext_currenttime_c_1, text, 40);
        g_free(text);

        gtk_label_set_text(GTK_LABEL(marker_label), "Max");
        gtk_label_set_text(GTK_LABEL(marker_value), GLOBALS->maxtext_currenttime_c_1);
    } else {
        gtk_label_set_text(GTK_LABEL(marker_label), "Marker");
        if (GLOBALS->tims.marker >= 0) {
            gtk_label_set_text(GTK_LABEL(marker_value), GLOBALS->maxtext_currenttime_c_1);
        } else {
            gtk_label_set_text(GTK_LABEL(marker_value), EM_DASH);
        }
    }

    gchar *text;
    if (GLOBALS->tims.baseline >= 0) {
        gtk_label_set_text(GTK_LABEL(cursor_label), "Base");
        text = reformat_time_2(GLOBALS->tims.baseline + GLOBALS->global_time_offset,
                               GLOBALS->time_dimension,
                               FALSE);
    } else {
        gtk_label_set_text(GTK_LABEL(cursor_label), "Cursor");

        text = reformat_time_2(GLOBALS->currenttime + GLOBALS->global_time_offset,
                               GLOBALS->time_dimension,
                               FALSE);

        if (is_in_blackout_region(GLOBALS->currenttime + GLOBALS->global_time_offset)) {
            gchar *last_space = g_strrstr(text, " ");
            if (last_space != NULL) {
                *last_space = '*';
            } else {
                g_assert_not_reached();
            }
        }
    }

    strncpy(GLOBALS->curtext_currenttime_c_1, text, 40);
    g_free(text);

    gtk_label_set_text(GTK_LABEL(cursor_value), GLOBALS->curtext_currenttime_c_1);
}

void update_currenttime(TimeType val)
{
    GLOBALS->cached_currenttimeval_currenttime_c_1 = val;

    if (GLOBALS->tims.baseline >= 0) {
        return;
    }

    GLOBALS->currenttime = val;

    update_time_box();
}

/* Create an entry box */
GtkWidget *create_time_box(void)
{
    GLOBALS->maxtext_currenttime_c_1 = malloc_2(40);
    GLOBALS->curtext_currenttime_c_1 = malloc_2(40);

    // Determine the maximum value length
    gchar buf[40] = {0};
    reformat_time(buf, GLOBALS->max_time, GLOBALS->time_dimension);
    gint max_length = strlen(buf) + 2; // two extra chars for sign and 'B' delta marker
    max_length = MAX(max_length, 17); // at least 17 chars to fit delta frequency

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "gw-time-box");

    GtkWidget *marker_label = gtk_label_new(NULL);
    gtk_label_set_width_chars(GTK_LABEL(marker_label), 6);
    gtk_label_set_xalign(GTK_LABEL(marker_label), 0.0);
    gtk_widget_set_valign(marker_label, GTK_ALIGN_BASELINE);
    gtk_box_pack_start(GTK_BOX(box), marker_label, FALSE, FALSE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(marker_label), "gw-time-box-label");

    GtkWidget *marker_value = gtk_label_new(NULL);
    gtk_label_set_width_chars(GTK_LABEL(marker_value), max_length);
    gtk_label_set_xalign(GTK_LABEL(marker_value), 1.0);
    gtk_widget_set_valign(marker_value, GTK_ALIGN_BASELINE);
    gtk_box_pack_start(GTK_BOX(box), marker_value, FALSE, FALSE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(marker_value), "gw-time-box-value");

    GtkWidget *separator = gtk_label_new("|");
    gtk_widget_set_valign(separator, GTK_ALIGN_BASELINE);
    gtk_box_pack_start(GTK_BOX(box), separator, FALSE, FALSE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(separator), "gw-time-box-separator");

    GtkWidget *cursor_label = gtk_label_new(NULL);
    gtk_label_set_width_chars(GTK_LABEL(cursor_label), 6);
    gtk_label_set_xalign(GTK_LABEL(cursor_label), 0.0);
    gtk_widget_set_valign(cursor_label, GTK_ALIGN_BASELINE);
    gtk_box_pack_start(GTK_BOX(box), cursor_label, FALSE, FALSE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(cursor_label), "gw-time-box-label");

    GtkWidget *cursor_value = gtk_label_new(NULL);
    gtk_label_set_width_chars(GTK_LABEL(cursor_value), max_length);
    gtk_label_set_xalign(GTK_LABEL(cursor_value), 1.0);
    gtk_widget_set_valign(cursor_value, GTK_ALIGN_BASELINE);
    gtk_box_pack_start(GTK_BOX(box), cursor_value, FALSE, FALSE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(cursor_value), "gw-time-box-value");

    return box;
}

TimeType time_trunc(TimeType t)
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
    TimeType compar = LLDescriptor(1000000000000000);

    for (; compar != 1; compar = compar / 10, gcompar = gcompar / ((gdouble)10.0)) {
        if (GLOBALS->nspx >= gcompar) {
            GLOBALS->time_trunc_val_currenttime_c_1 = compar;
            return;
        }
    }

    GLOBALS->time_trunc_val_currenttime_c_1 = 1;
}

/*
 * called by lxt/lxt2/vzt reader inits
 */
void exponent_to_time_scale(signed char scale)
{
    switch (scale) {
        case 2:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 's';
            break;
        case 1:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case 0:
            GLOBALS->time_dimension = 's';
            break;

        case -1:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'm';
            break;
        case -2:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -3:
            GLOBALS->time_dimension = 'm';
            break;

        case -4:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'u';
            break;
        case -5:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -6:
            GLOBALS->time_dimension = 'u';
            break;

        case -10:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'p';
            break;
        case -11:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -12:
            GLOBALS->time_dimension = 'p';
            break;

        case -13:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'f';
            break;
        case -14:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -15:
            GLOBALS->time_dimension = 'f';
            break;

        case -16:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'a';
            break;
        case -17:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -18:
            GLOBALS->time_dimension = 'a';
            break;

        case -19:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'z';
            break;
        case -20:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -21:
            GLOBALS->time_dimension = 'z';
            break;

        case -7:
            GLOBALS->time_scale = LLDescriptor(100);
            GLOBALS->time_dimension = 'n';
            break;
        case -8:
            GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -9:
        default:
            GLOBALS->time_dimension = 'n';
            break;
    }
}
