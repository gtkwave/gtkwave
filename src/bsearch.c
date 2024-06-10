/*
 * Copyright (c) Tony Bybell 1999-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "analyzer.h"
#include "currenttime.h"
#include "symbol.h"
#include "bsearch.h"
#include "strace.h"
#include <ctype.h>

static int compar_timechain(const void *s1, const void *s2)
{
    GwTime key, obj, delta;
    GwTime *cpos;
    int rv;

    key = *((GwTime *)s1);
    obj = *(cpos = (GwTime *)s2);

    if ((obj <= key) && (obj > GLOBALS->max_compare_time_tc_bsearch_c_1)) {
        GLOBALS->max_compare_time_tc_bsearch_c_1 = obj;
        GLOBALS->max_compare_pos_tc_bsearch_c_1 = cpos;
    }

    delta = key - obj;
    if (delta < 0)
        rv = -1;
    else if (delta > 0)
        rv = 1;
    else
        rv = 0;

    return (rv);
}

int bsearch_timechain(GwTime key)
{
    GLOBALS->max_compare_time_tc_bsearch_c_1 = -2;
    GLOBALS->max_compare_pos_tc_bsearch_c_1 = NULL;

    if (!GLOBALS->strace_ctx->timearray)
        return (-1);

    if (bsearch(&key,
                GLOBALS->strace_ctx->timearray,
                GLOBALS->strace_ctx->timearray_size,
                sizeof(GwTime),
                compar_timechain)) {
        /* nothing, all side effects are in bsearch */
    }

    if ((!GLOBALS->max_compare_pos_tc_bsearch_c_1) ||
        (GLOBALS->max_compare_time_tc_bsearch_c_1 < GLOBALS->shift_timebase)) {
        GLOBALS->max_compare_pos_tc_bsearch_c_1 =
            GLOBALS->strace_ctx->timearray; /* aix bsearch fix */
    }

    return (GLOBALS->max_compare_pos_tc_bsearch_c_1 - GLOBALS->strace_ctx->timearray);
}

/*****************************************************************************************/

static int compar_histent(const void *s1, const void *s2)
{
    GwTime key, obj, delta;
    GwHistEnt *cpos;
    int rv;

    key = *((GwTime *)s1);
    obj = (cpos = (*((GwHistEnt **)s2)))->time;

    if ((obj <= key) && (obj > GLOBALS->max_compare_time_bsearch_c_1)) {
        GLOBALS->max_compare_time_bsearch_c_1 = obj;
        GLOBALS->max_compare_pos_bsearch_c_1 = cpos;
        GLOBALS->max_compare_index = (GwHistEnt **)s2;
    }

    delta = key - obj;
    if (delta < 0)
        rv = -1;
    else if (delta > 0)
        rv = 1;
    else
        rv = 0;

    return (rv);
}

GwHistEnt *bsearch_node(GwNode *n, GwTime key)
{
    GLOBALS->max_compare_time_bsearch_c_1 = -2;
    GLOBALS->max_compare_pos_bsearch_c_1 = NULL;
    GLOBALS->max_compare_index = NULL;

    if (bsearch(&key, n->harray, n->numhist, sizeof(GwHistEnt *), compar_histent)) {
        /* nothing, all side effects are in bsearch */
    }

    if ((!GLOBALS->max_compare_pos_bsearch_c_1) ||
        (GLOBALS->max_compare_time_bsearch_c_1 < GW_TIME_CONSTANT(0))) {
        GLOBALS->max_compare_pos_bsearch_c_1 = n->harray[1]; /* aix bsearch fix */
        GLOBALS->max_compare_index = &(n->harray[1]);
    }

    while (GLOBALS->max_compare_pos_bsearch_c_1->next) /* non-RoSync dumper deglitching fix */
    {
        if (GLOBALS->max_compare_pos_bsearch_c_1->time !=
            GLOBALS->max_compare_pos_bsearch_c_1->next->time)
            break;
        GLOBALS->max_compare_pos_bsearch_c_1 = GLOBALS->max_compare_pos_bsearch_c_1->next;
    }

    return (GLOBALS->max_compare_pos_bsearch_c_1);
}

/*****************************************************************************************/

static int compar_vectorent(const void *s1, const void *s2)
{
    GwTime key, obj, delta;
    GwVectorEnt *cpos;
    int rv;

    key = *((GwTime *)s1);
    /* obj=(cpos=(*((GwVectorEnt **)s2)))->time+GLOBALS->shift_timebase; */

    obj = (cpos = (*((GwVectorEnt **)s2)))->time;

    if ((obj <= key) && (obj > GLOBALS->vmax_compare_time_bsearch_c_1)) {
        GLOBALS->vmax_compare_time_bsearch_c_1 = obj;
        GLOBALS->vmax_compare_pos_bsearch_c_1 = cpos;
        GLOBALS->vmax_compare_index = (GwVectorEnt **)s2;
    }

    delta = key - obj;
    if (delta < 0)
        rv = -1;
    else if (delta > 0)
        rv = 1;
    else
        rv = 0;

    return (rv);
}

GwVectorEnt *bsearch_vector(GwBitVector *b, GwTime key)
{
    GLOBALS->vmax_compare_time_bsearch_c_1 = -2;
    GLOBALS->vmax_compare_pos_bsearch_c_1 = NULL;
    GLOBALS->vmax_compare_index = NULL;

    if (bsearch(&key, b->vectors, b->numregions, sizeof(GwVectorEnt *), compar_vectorent)) {
        /* nothing, all side effects are in bsearch */
    }

    if ((!GLOBALS->vmax_compare_pos_bsearch_c_1) ||
        (GLOBALS->vmax_compare_time_bsearch_c_1 < GW_TIME_CONSTANT(0))) {
        /* ignore warning: array index of '1' indexes past the end of an array (that contains 1
         * elements) [-Warray-bounds] */
        /* because this array is allocated with size > that declared in the structure definition via
         * end of structure malloc padding */
        GLOBALS->vmax_compare_pos_bsearch_c_1 = b->vectors[1]; /* aix bsearch fix */
        GLOBALS->vmax_compare_index = &(b->vectors[1]);
    }

    while (GLOBALS->vmax_compare_pos_bsearch_c_1->next) /* non-RoSync dumper deglitching fix */
    {
        if (GLOBALS->vmax_compare_pos_bsearch_c_1->time !=
            GLOBALS->vmax_compare_pos_bsearch_c_1->next->time)
            break;
        GLOBALS->vmax_compare_pos_bsearch_c_1 = GLOBALS->vmax_compare_pos_bsearch_c_1->next;
    }

    return (GLOBALS->vmax_compare_pos_bsearch_c_1);
}

/*****************************************************************************************/

static int compar_trunc(const void *s1, const void *s2)
{
    char *str;
    char vcache[2];
    int key, obj;

    str = (char *)s2;
    key = *((int *)s1);

    vcache[0] = *str;
    vcache[1] = *(str + 1);
    *str = '+';
    *(str + 1) = 0;
    obj = font_engine_string_measure(GLOBALS->wavefont, GLOBALS->trunc_asciibase_bsearch_c_1);
    *str = vcache[0];
    *(str + 1) = vcache[1];

    if ((obj <= key) && (obj > GLOBALS->maxlen_trunc)) {
        GLOBALS->maxlen_trunc = obj;
        GLOBALS->maxlen_trunc_pos_bsearch_c_1 = str;
    }

    return (key - obj);
}

char *bsearch_trunc(char *ascii, int maxlen)
{
    int len;

    if ((maxlen <= 0) || (!ascii) || (!(len = strlen(ascii))))
        return (NULL);

    GLOBALS->maxlen_trunc_pos_bsearch_c_1 = NULL;

    if (GLOBALS->wavefont->is_mono) {
        int adjusted_len = maxlen / GLOBALS->wavefont->mono_width;
        if (adjusted_len)
            adjusted_len--;
        if (GLOBALS->wavefont->mono_width <= maxlen) {
            GLOBALS->maxlen_trunc_pos_bsearch_c_1 = ascii + adjusted_len;
        }
    } else {
        GLOBALS->maxlen_trunc = 0;

        if (bsearch(&maxlen,
                    GLOBALS->trunc_asciibase_bsearch_c_1 = ascii,
                    len,
                    sizeof(char),
                    compar_trunc)) {
            /* nothing, all side effects are in bsearch */
        }
    }

    return (GLOBALS->maxlen_trunc_pos_bsearch_c_1);
}

char *bsearch_trunc_print(char *ascii, int maxlen)
{
    int len;

    if ((maxlen <= 0) || (!ascii) || (!(len = strlen(ascii))))
        return (NULL);

    GLOBALS->maxlen_trunc = 0;
    GLOBALS->maxlen_trunc_pos_bsearch_c_1 = NULL;

    if (bsearch(&maxlen,
                GLOBALS->trunc_asciibase_bsearch_c_1 = ascii,
                len,
                sizeof(char),
                compar_trunc)) {
        /* nothing, all side effects are in bsearch */
    }

    return (GLOBALS->maxlen_trunc_pos_bsearch_c_1);
}

/*****************************************************************************************/

static int compar_facs(const void *key, const void *v2)
{
    GwSymbol *s2;
    int rc;
    char *s3;

    s2 = *((GwSymbol **)v2);
    s3 = s2->name;
    rc = sigcmp((char *)key, s3);

    /* if(was_packed) free_2(s3); ...not needed with HIER_DEPACK_STATIC */

    return (rc);
}

// TODO: move to GwFacs
GwSymbol *bsearch_facs(char *ascii, unsigned int *rows_return)
{
    GwSymbol **rc;
    int len;

    if ((!ascii) || (!(len = strlen(ascii))))
        return (NULL);
    if (rows_return) {
        *rows_return = 0;
    }

    GwFacs *facs = gw_dump_file_get_facs(GLOBALS->dump_file);
    GwSymbol **facs_array = gw_facs_get_array(facs);
    guint numfacs = gw_facs_get_length(facs);

    if (ascii[len - 1] == '}') {
        int i;

        for (i = len - 2; i >= 2; i--) {
            if (isdigit((int)(unsigned char)ascii[i]))
                continue;
            if (ascii[i] == '{') {
                char *tsc = g_alloca(i + 1);
                memcpy(tsc, ascii, i + 1);
                tsc[i] = 0;

                rc =
                    (GwSymbol **)bsearch(tsc, facs_array, numfacs, sizeof(GwSymbol *), compar_facs);
                if (rc) {
                    unsigned int whichrow = atoi(&ascii[i + 1]);
                    if (rows_return)
                        *rows_return = whichrow;

                    return (*rc);
                }
            }
            break; /* fallthrough to normal handler */
        }
    }

    rc = (GwSymbol **)bsearch(ascii, facs_array, numfacs, sizeof(GwSymbol *), compar_facs);
    if (rc)
        return (*rc);
    else
        return (NULL);
}
