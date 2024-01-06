/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <math.h>
#include <string.h>
#include "gtk23compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "bsearch.h"
#include "color.h"
#include "strace.h"
#include "debug.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "pipeio.h"

/*
 * filters mutually exclusive with file/translate/process filters
 */
static char *lzremoval(char *s)
{
    char *p = s;

    if (*p) {
        while ((*p == '0') && *(p + 1)) {
            p++;
        }
    }

    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }

    return (s);
}

/*
 * file/translate/process filters
 */
static char *dofilter(GwTrace *t, char *s)
{
    GLOBALS->xl_file_filter[t->f_filter] = xl_splay(s, GLOBALS->xl_file_filter[t->f_filter]);

    if (!strcasecmp(s, GLOBALS->xl_file_filter[t->f_filter]->item)) {
        free_2(s);
        s = malloc_2(strlen(GLOBALS->xl_file_filter[t->f_filter]->trans) + 1);
        strcpy(s, GLOBALS->xl_file_filter[t->f_filter]->trans);
    }

    if ((*s == '?') && (!GLOBALS->color_active_in_filter)) {
        char *s2a;
        char *s2 = strchr(s + 1, '?');
        if (s2) {
            s2++;
            s2a = malloc_2(strlen(s2) + 1);
            strcpy(s2a, s2);
            free_2(s);
            s = s2a;
        }
    }

    return (s);
}

static char *edofilter(GwTrace *t, char *s)
{
    if (t->flags & TR_ENUM) {
        int filt = t->e_filter - 1;

#ifdef _WAVE_HAVE_JUDY
        PPvoid_t pv = JudyHSGet(GLOBALS->xl_enum_filter[filt], s, strlen(s));
        if (pv) {
            free_2(s);
            s = malloc_2(strlen(*pv) + 1);
            strcpy(s, *pv);
        }
#else
        GLOBALS->xl_enum_filter[filt] = xl_splay(s, GLOBALS->xl_enum_filter[filt]);

        if (!strcasecmp(s, GLOBALS->xl_enum_filter[filt]->item)) {
            free_2(s);
            s = malloc_2(strlen(GLOBALS->xl_enum_filter[filt]->trans) + 1);
            strcpy(s, GLOBALS->xl_enum_filter[filt]->trans);
        }
#endif
        else {
            char *zerofind = s;
            char *dst = s, *src;
            while (*zerofind == '0')
                zerofind++;
            if (zerofind != s) {
                src = (!*zerofind) ? (zerofind - 1) : zerofind;
                while (*src) {
                    *(dst++) = *(src++);
                }
                *dst = 0;
            }
        }
    }

    return (s);
}

static char *pdofilter(GwTrace *t, char *s)
{
    struct pipe_ctx *p = GLOBALS->proc_filter[t->p_filter];
    char buf[1025];
    int n;

    if (p) {
#if !defined __MINGW32__
        fputs(s, p->sout);
        fputc('\n', p->sout);
        fflush(p->sout);

        buf[0] = 0;

        n = fgets(buf, 1024, p->sin) ? strlen(buf) : 0;
        buf[n] = 0;
#else
        {
            BOOL bSuccess;
            DWORD dwWritten, dwRead;

            WriteFile(p->g_hChildStd_IN_Wr, s, strlen(s), &dwWritten, NULL);
            WriteFile(p->g_hChildStd_IN_Wr, "\n", 1, &dwWritten, NULL);

            for (n = 0; n < 1024; n++) {
                do {
                    bSuccess = ReadFile(p->g_hChildStd_OUT_Rd, buf + n, 1, &dwRead, NULL);
                    if ((!bSuccess) || (buf[n] == '\n')) {
                        goto ex;
                    }

                } while (buf[n] == '\r');
            }
        ex:
            buf[n] = 0;
        }

#endif

        if (n) {
            if (buf[n - 1] == '\n') {
                buf[n - 1] = 0;
                n--;
            }
        }

        if (buf[0]) {
            free_2(s);
            s = malloc_2(n + 1);
            strcpy(s, buf);
        }
    }

    if ((*s == '?') && (!GLOBALS->color_active_in_filter)) {
        char *s2a;
        char *s2 = strchr(s + 1, '?');
        if (s2) {
            s2++;
            s2a = malloc_2(strlen(s2) + 1);
            strcpy(s2a, s2);
            free_2(s);
            s = s2a;
        }
    }
    return (s);
}

/*
 * convert binary <=> gray/ffo/popcnt in place
 */
#define cvt_gray(f, p, n) \
    do { \
        if ((f) & (TR_GRAYMASK | TR_POPCNT | TR_FFO)) { \
            if ((f)&TR_BINGRAY) { \
                convert_bingray((p), (n)); \
            } \
            if ((f)&TR_GRAYBIN) { \
                convert_graybin((p), (n)); \
            } \
            if ((f)&TR_FFO) { \
                convert_ffo((p), (n)); \
            } \
            if ((f)&TR_POPCNT) { \
                convert_popcnt((p), (n)); \
            } \
        } \
    } while (0)

static void convert_graybin(char *pnt, int nbits)
{
    char kill_state = 0;
    char pch = GW_BIT_0;
    int i;

    for (i = 0; i < nbits; i++) {
        char ch = pnt[i];

        if (!kill_state) {
            switch (ch) {
                case GW_BIT_0:
                case GW_BIT_L:
                    if ((pch == GW_BIT_1) || (pch == GW_BIT_H)) {
                        pnt[i] = pch;
                    }
                    break;

                case GW_BIT_1:
                case GW_BIT_H:
                    if (pch == GW_BIT_1) {
                        pnt[i] = GW_BIT_0;
                    } else if (pch == GW_BIT_H) {
                        pnt[i] = GW_BIT_L;
                    }
                    break;

                default:
                    kill_state = 1;
                    break;
            }

            pch = pnt[i]; /* pch is xor accumulator */
        } else {
            pnt[i] = pch;
        }
    }
}

static void convert_bingray(char *pnt, int nbits)
{
    char kill_state = 0;
    char pch = GW_BIT_0;
    int i;

    for (i = 0; i < nbits; i++) {
        char ch = pnt[i];

        if (!kill_state) {
            switch (ch) {
                case GW_BIT_0:
                case GW_BIT_L:
                    if ((pch == GW_BIT_1) || (pch == GW_BIT_H)) {
                        pnt[i] = pch;
                    }
                    break;

                case GW_BIT_1:
                case GW_BIT_H:
                    if (pch == GW_BIT_1) {
                        pnt[i] = GW_BIT_0;
                    } else if (pch == GW_BIT_H) {
                        pnt[i] = GW_BIT_L;
                    }
                    break;

                default:
                    kill_state = 1;
                    break;
            }

            pch = ch; /* pch is previous character */
        } else {
            pnt[i] = pch;
        }
    }
}

static void convert_popcnt(char *pnt, int nbits)
{
    int i;
    unsigned int pop = 0;

    for (i = 0; i < nbits; i++) {
        char ch = pnt[i];

        switch (ch) {
            case GW_BIT_1:
            case GW_BIT_H:
                pop++;
                break;

            default:
                break;
        }
    }

    for (i = nbits - 1; i >= 0; i--) /* always requires less number of bits */
    {
        pnt[i] = (pop & 1) ? GW_BIT_1 : GW_BIT_0;
        pop >>= 1;
    }
}

static void convert_ffo(char *pnt, int nbits)
{
    int i;
    int ffo = -1;

    for (i = (nbits - 1); i >= 0; i--) {
        char ch = pnt[i];

        if ((ch == GW_BIT_1) || (ch == GW_BIT_H)) {
            ffo = (nbits - 1) - i;
            break;
        }
    }

    if (ffo >= 0) {
        for (i = nbits - 1; i >= 0; i--) /* always requires less number of bits */
        {
            pnt[i] = (ffo & 1) ? GW_BIT_1 : GW_BIT_0;
            ffo >>= 1;
        }
    } else {
        for (i = nbits - 1; i >= 0; i--) /* always requires less number of bits */
        {
            pnt[i] = GW_BIT_X;
        }
    }
}

static void dpr_e16(char *str, double d)
{
    char *buf16;
    char buf15[24];
    int l16;
    int l15;

    buf16 = str;
    buf16[23] = 0;
    l16 = snprintf(buf16, 24, "%.16g", d);
    if (l16 >= 18) {
        buf15[23] = 0;
        l15 = snprintf(buf15, 24, "%.15g", d);
        if ((l16 - l15) > 3) {
            strcpy(str, buf15);
        }
    }
}

static void cvt_fpsdec(GwTrace *t, GwTime val, char *os, int len, int nbits)
{
    (void)nbits; /* number of bits shouldn't be relevant here as we're going through a fraction */

    int shamt = t->t_fpdecshift;
    GwTime lpart = val >> shamt;
    GwTime rmsk = (GW_UTIME_CONSTANT(1) << shamt);
    GwTime rbit = (val >= 0) ? (val & (rmsk - 1)) : ((-val) & (rmsk - 1));
    double rfrac;
    int negflag = 0;
    char dbuf[32];
    char bigbuf[64];

    if (rmsk) {
        rfrac = (double)rbit / (double)rmsk;

        if (shamt) {
            if (lpart < 0) {
                if (rbit) {
                    lpart++;
                    if (!lpart)
                        negflag = 1;
                }
            }
        }

    } else {
        rfrac = 0.0;
    }

    dpr_e16(dbuf, rfrac); /* sprintf(dbuf, "%.16g", rfrac); */
    char *dot = strchr(dbuf, '.');

    if (dot && (dbuf[0] == '0')) {
        sprintf(bigbuf, "%s%" GW_TIME_FORMAT ".%s", negflag ? "-" : "", lpart, dot + 1);
        strncpy(os, bigbuf, len);
        os[len - 1] = 0;
    } else {
        sprintf(os, "%s%" GW_TIME_FORMAT, negflag ? "-" : "", lpart);
    }
}

static void cvt_fpsudec(GwTrace *t, GwTime val, char *os, int len)
{
    int shamt = t->t_fpdecshift;
    GwUTime lpart = ((GwUTime)val) >> shamt;
    GwTime rmsk = (GW_UTIME_CONSTANT(1) << shamt);
    GwTime rbit = (val & (rmsk - 1));
    double rfrac;
    char dbuf[32];
    char bigbuf[64];

    if (rmsk) {
        rfrac = (double)rbit / (double)rmsk;
    } else {
        rfrac = 0.0;
    }

    dpr_e16(dbuf, rfrac); /* sprintf(dbuf, "%.16g", rfrac);	*/
    char *dot = strchr(dbuf, '.');
    if (dot && (dbuf[0] == '0')) {
        sprintf(bigbuf, "%" GW_UTIME_FORMAT ".%s", lpart, dot + 1);
        strncpy(os, bigbuf, len);
        os[len - 1] = 0;
    } else {
        sprintf(os, "%" GW_UTIME_FORMAT, lpart);
    }
}

/*
 * convert trptr+vptr into an ascii string
 */
static char *convert_ascii_2(GwTrace *t, GwVectorEnt *v)
{
    TraceFlagsType flags;
    int nbits;
    unsigned char *bits;
    char *os, *pnt, *newbuff;
    int i, j, len;

    static const char xfwd[GW_BIT_COUNT] = AN_NORMAL;
    static const char xrev[GW_BIT_COUNT] = AN_INVERSE;
    const char *xtab;

    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);

    flags = t->flags;
    nbits = t->n.vec->nbits;
    bits = v->v;

    if (flags & TR_INVERT) {
        xtab = xrev;
    } else {
        xtab = xfwd;
    }

    if (flags & (TR_ZEROFILL | TR_ONEFILL)) {
        char whichfill = (flags & TR_ZEROFILL) ? GW_BIT_0 : GW_BIT_1;
        int msi = 0, lsi = 0, ok = 0;
        if ((t->name) && (nbits > 1)) {
            char *lbrack = strrchr(t->name, '[');
            if (lbrack) {
                int rc = sscanf(lbrack + 1, "%d:%d", &msi, &lsi);
                if (rc == 2) {
                    if (((msi - lsi + 1) == nbits) || ((lsi - msi + 1) == nbits)) {
                        ok = 1; /* to ensure sanity... */
                    }
                }
            }
        }

        if (ok) {
            if (msi > lsi) {
                if (lsi > 0) {
                    pnt = g_alloca(msi + 1);

                    memcpy(pnt, bits, nbits);

                    for (i = nbits; i < msi + 1; i++) {
                        pnt[i] = whichfill;
                    }

                    bits = (unsigned char *)pnt;
                    nbits = msi + 1;
                }
            } else {
                if (msi > 0) {
                    pnt = g_alloca(lsi + 1);

                    for (i = 0; i < msi; i++) {
                        pnt[i] = whichfill;
                    }

                    memcpy(pnt + i, bits, nbits);

                    bits = (unsigned char *)pnt;
                    nbits = lsi + 1;
                }
            }
        }
    }

    newbuff = (char *)malloc_2(nbits + 6); /* for justify */
    if (flags & TR_REVERSE) {
        char *fwdpnt, *revpnt;

        fwdpnt = (char *)bits;
        revpnt = newbuff + nbits + 6;
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(--revpnt) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
    } else {
        char *fwdpnt, *fwdpnt2;

        fwdpnt = (char *)bits;
        fwdpnt2 = newbuff;
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(fwdpnt2++) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
    }

    if (flags & TR_ASCII) {
        char *parse;
        int found = 0;

        free_2(newbuff);
        newbuff = (char *)malloc_2(nbits + 14); /* for justify */
        if (flags & TR_REVERSE) {
            char *fwdpnt, *revpnt;

            fwdpnt = (char *)bits;
            revpnt = newbuff + nbits + 14;
            for (i = 0; i < 7; i++)
                *(--revpnt) = xfwd[0];
            for (i = 0; i < nbits; i++) {
                *(--revpnt) = xtab[(int)(*(fwdpnt++))];
            }
            for (i = 0; i < 7; i++)
                *(--revpnt) = xfwd[0];
        } else {
            char *fwdpnt, *fwdpnt2;

            fwdpnt = (char *)bits;
            fwdpnt2 = newbuff;
            for (i = 0; i < 7; i++)
                *(fwdpnt2++) = xfwd[0];
            for (i = 0; i < nbits; i++) {
                *(fwdpnt2++) = xtab[(int)(*(fwdpnt++))];
            }
            for (i = 0; i < 7; i++)
                *(fwdpnt2++) = xfwd[0];
        }

        len = (nbits / 8) + 2 + 2; /* $xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '"';
        }

        parse = (flags & TR_RJUSTIFY) ? (newbuff + ((nbits + 7) & 7)) : (newbuff + 7);
        cvt_gray(flags, parse, (flags & TR_RJUSTIFY) ? ((nbits + 7) & ~7) : nbits);

        for (i = 0; i < nbits; i += 8) {
            unsigned long val;

            val = 0;
            for (j = 0; j < 8; j++) {
                val <<= 1;

                if ((parse[j] == GW_BIT_X) || (parse[j] == GW_BIT_Z) || (parse[j] == GW_BIT_W) ||
                    (parse[j] == GW_BIT_U) || (parse[j] == GW_BIT_DASH)) {
                    val = 1000; /* arbitrarily large */
                }
                if ((parse[j] == GW_BIT_1) || (parse[j] == GW_BIT_H)) {
                    val |= 1;
                }
            }

            if (val) {
                if (val > 0x7f || !isprint(val))
                    *pnt++ = '.';
                else
                    *pnt++ = val;
                found = 1;
            }

            parse += 8;
        }
        if (!found && !GLOBALS->show_base) {
            *(pnt++) = '"';
            *(pnt++) = '"';
        }

        if (GLOBALS->show_base) {
            *(pnt++) = '"';
        }
        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if ((flags & TR_HEX) || ((flags & (TR_DEC | TR_SIGNED)) && (nbits > 64) &&
                                    (!(flags & (TR_POPCNT | TR_FFO))))) {
        char *parse;

        len = (nbits / 4) + 2 + 1; /* $xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '$';
        }

        parse = (flags & TR_RJUSTIFY) ? (newbuff + ((nbits + 3) & 3)) : (newbuff + 3);
        cvt_gray(flags, parse, (flags & TR_RJUSTIFY) ? ((nbits + 3) & ~3) : nbits);

        for (i = 0; i < nbits; i += 4) {
            unsigned char val;

            val = 0;
            for (j = 0; j < 4; j++) {
                val <<= 1;

                if ((parse[j] == GW_BIT_1) || (parse[j] == GW_BIT_H)) {
                    val |= 1;
                } else if ((parse[j] == GW_BIT_0) || (parse[j] == GW_BIT_L)) {
                } else if (parse[j] == GW_BIT_X) {
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_X) {
                            char *thisbyt = parse + i + k;
                            char *lastbyt = newbuff + 3 + nbits - 1;
                            if ((lastbyt - thisbyt) >= 0)
                                match = 0;
                            break;
                        }
                    }
                    val = (match) ? 16 : 21;
                    break;
                } else if (parse[j] == GW_BIT_Z) {
                    int xover = 0;
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_Z) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            } else {
                                char *thisbyt = parse + i + k;
                                char *lastbyt = newbuff + 3 + nbits - 1;
                                if ((lastbyt - thisbyt) >= 0)
                                    match = 0;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = (match) ? 17 : 22;
                    break;
                } else if (parse[j] == GW_BIT_W) {
                    int xover = 0;
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_W) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            } else {
                                char *thisbyt = parse + i + k;
                                char *lastbyt = newbuff + 3 + nbits - 1;
                                if ((lastbyt - thisbyt) >= 0)
                                    match = 0;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = (match) ? 18 : 23;
                    break;
                } else if (parse[j] == GW_BIT_U) {
                    int xover = 0;
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_U) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            } else {
                                char *thisbyt = parse + i + k;
                                char *lastbyt = newbuff + 3 + nbits - 1;
                                if ((lastbyt - thisbyt) >= 0)
                                    match = 0;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = (match) ? 19 : 24;
                    break;
                } else if (parse[j] == GW_BIT_DASH) {
                    int xover = 0;
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_DASH) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = 20;
                    break;
                }
            }

            *(pnt++) = AN_HEX_STR[val];

            parse += 4;
        }

        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if (flags & TR_OCT) {
        char *parse;

        len = (nbits / 3) + 2 + 1; /* #xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '#';
        }

        parse = (flags & TR_RJUSTIFY) ? (newbuff + ((nbits % 3) ? (nbits % 3) : 3)) : (newbuff + 3);
        cvt_gray(flags, parse, (flags & TR_RJUSTIFY) ? (((nbits + 2) / 3) * 3) : nbits);

        for (i = 0; i < nbits; i += 3) {
            unsigned char val;

            val = 0;
            for (j = 0; j < 3; j++) {
                val <<= 1;

                if (parse[j] == GW_BIT_X) {
                    val = 8;
                    break;
                }
                if (parse[j] == GW_BIT_Z) {
                    val = 9;
                    break;
                }
                if (parse[j] == GW_BIT_W) {
                    val = 10;
                    break;
                }
                if (parse[j] == GW_BIT_U) {
                    val = 11;
                    break;
                }
                if (parse[j] == GW_BIT_DASH) {
                    val = 12;
                    break;
                }

                if ((parse[j] == GW_BIT_1) || (parse[j] == GW_BIT_H)) {
                    val |= 1;
                }
            }

            *(pnt++) = AN_OCT_STR[val];

            parse += 3;
        }

        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if (flags & TR_BIN) {
        char *parse;

        len = (nbits / 1) + 2 + 1; /* %xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '%';
        }

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        for (i = 0; i < nbits; i++) {
            *(pnt++) = gw_bit_to_char((int)(*(parse++)));
        }

        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if (flags & TR_SIGNED) {
        char *parse;
        GwTime val = 0;
        unsigned char fail = 0;

        len = 21; /* len+1 of 0x8000000000000000 expressed in decimal */
        os = (char *)calloc_2(1, len);

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        if ((parse[0] == GW_BIT_1) || (parse[0] == GW_BIT_H)) {
            val = GW_TIME_CONSTANT(-1);
        } else if ((parse[0] == GW_BIT_0) || (parse[0] == GW_BIT_L)) {
            val = GW_TIME_CONSTANT(0);
        } else {
            fail = 1;
        }

        if (!fail)
            for (i = 1; i < nbits; i++) {
                val <<= 1;

                if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                    val |= GW_TIME_CONSTANT(1);
                } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                    fail = 1;
                    break;
                }
            }

        if (!fail) {
            if ((flags & TR_FPDECSHIFT) && (t->t_fpdecshift)) {
                cvt_fpsdec(t, val, os, len, nbits);
            } else {
                if (!(flags & TR_TIME)) {
                    sprintf(os, "%" GW_TIME_FORMAT, val);
                } else {
                    free_2(os);
                    os = calloc_2(1, 128);
                    reformat_time(os, val, time_dimension);
                }
            }
        } else {
            strcpy(os, "XXX");
        }
    } else if (flags & TR_REAL) {
        char *parse;

        if ((nbits == 64) || (nbits == 32)) {
            GwUTime utt = GW_TIME_CONSTANT(0);
            double d;
            float f;
            uint32_t utt_32;

            parse = newbuff + 3;
            cvt_gray(flags, parse, nbits);

            for (i = 0; i < nbits; i++) {
                char ch = gw_bit_to_char((int)(*(parse++)));
                if ((ch == '0') || (ch == '1')) {
                    utt <<= 1;
                    if (ch == '1') {
                        utt |= GW_TIME_CONSTANT(1);
                    }
                } else {
                    goto rl_go_binary;
                }
            }

            os = /*pnt=*/(char *)calloc_2(1, 64); /* scan-build */
            if (nbits == 64) {
                memcpy(&d, &utt, sizeof(double));
                dpr_e16(os, d); /* sprintf(os, "%.16g", d); */
            } else {
                utt_32 = utt;
                memcpy(&f, &utt_32, sizeof(float));
                sprintf(os, "%f", f);
            }
        } else {
        rl_go_binary:
            len = (nbits / 1) + 2 + 1; /* %xxxxx */
            os = pnt = (char *)calloc_2(1, len);
            if (GLOBALS->show_base) {
                *(pnt++) = '%';
            }

            parse = newbuff + 3;
            cvt_gray(flags, parse, nbits);

            for (i = 0; i < nbits; i++) {
                *(pnt++) = gw_bit_to_char((int)(*(parse++)));
            }

            *(pnt) = 0x00; /* scan build : remove dead increment */
        }
    } else /* decimal when all else fails */
    {
        char *parse;
        GwUTime val = 0;
        unsigned char fail = 0;

        len = 21; /* len+1 of 0xffffffffffffffff expressed in decimal */
        os = (char *)calloc_2(1, len);

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        for (i = 0; i < nbits; i++) {
            val <<= 1;

            if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                val |= GW_TIME_CONSTANT(1);
            } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                fail = 1;
                break;
            }
        }

        if (!fail) {
            if ((flags & TR_FPDECSHIFT) && (t->t_fpdecshift)) {
                cvt_fpsudec(t, val, os, len);
            } else {
                if (!(flags & TR_TIME)) {
                    sprintf(os, "%" GW_UTIME_FORMAT, val);
                } else {
                    free_2(os);
                    os = calloc_2(1, 128);
                    reformat_time(os, val, time_dimension);
                }
            }
        } else {
            strcpy(os, "XXX");
        }
    }

    free_2(newbuff);
    return (os);
}

/*
 * convert trptr+hptr vectorstring into an ascii string
 */
char *convert_ascii_real(GwTrace *t, double *d)
{
    char *rv;

    if (t && (t->flags & TR_REAL2BITS) &&
        d) /* "real2bits" also allows other filters such as "bits2real" on top of it */
    {
        GwTrace t2;
        char vec[64];
        int i;
        guint64 swapmem;

        memcpy(&swapmem, d, sizeof(guint64));
        for (i = 0; i < 64; i++) {
            if (swapmem & (GW_UTIME_CONSTANT(1) << (63 - i))) {
                vec[i] = GW_BIT_1;
            } else {
                vec[i] = GW_BIT_0;
            }
        }

        memcpy(&t2, t, sizeof(GwTrace));

        t2.n.nd->msi = 63;
        t2.n.nd->lsi = 0;
        t2.flags &= ~(TR_REAL2BITS); /* to avoid possible recursion in the future */

        rv = convert_ascii_vec_2(&t2, vec);
    } else {
        rv = malloc_2(24); /* enough for .16e format */

        if (d) {
            dpr_e16(rv, *d); /* sprintf(rv,"%.16g",*d); */
        } else {
            strcpy(rv, "UNDEF");
        }
    }

    if (t) {
        if (!(t->f_filter | t->p_filter | t->e_filter)) {
            if (GLOBALS->lz_removal)
                lzremoval(rv);
        } else {
            if (t->e_filter) {
                rv = edofilter(t, rv);
            } else if (t->f_filter) {
                rv = dofilter(t, rv);
            } else {
                rv = pdofilter(t, rv);
            }
        }
    }

    return (rv);
}

char *convert_ascii_string(char *s)
{
    char *rv;

    if (s) {
        rv = (char *)malloc_2(strlen(s) + 1);
        strcpy(rv, s);
    } else {
        rv = (char *)malloc_2(6);
        strcpy(rv, "UNDEF");
    }
    return (rv);
}

static const unsigned char cvt_table[] = {
    GW_BIT_0 /* . */,    GW_BIT_X /* . */,    GW_BIT_Z /* . */,    GW_BIT_1 /* . */,
    GW_BIT_H /* . */,    GW_BIT_U /* . */,    GW_BIT_W /* . */,    GW_BIT_L /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /*   */, GW_BIT_DASH /* ! */, GW_BIT_DASH /* " */, GW_BIT_DASH /* # */,
    GW_BIT_DASH /* $ */, GW_BIT_DASH /* % */, GW_BIT_DASH /* & */, GW_BIT_DASH /* ' */,
    GW_BIT_DASH /* ( */, GW_BIT_DASH /* ) */, GW_BIT_DASH /* * */, GW_BIT_DASH /* + */,
    GW_BIT_DASH /* , */, GW_BIT_DASH /* - */, GW_BIT_DASH /* . */, GW_BIT_DASH /* / */,
    GW_BIT_0 /* 0 */,    GW_BIT_1 /* 1 */,    GW_BIT_DASH /* 2 */, GW_BIT_DASH /* 3 */,
    GW_BIT_DASH /* 4 */, GW_BIT_DASH /* 5 */, GW_BIT_DASH /* 6 */, GW_BIT_DASH /* 7 */,
    GW_BIT_DASH /* 8 */, GW_BIT_DASH /* 9 */, GW_BIT_DASH /* : */, GW_BIT_DASH /* ; */,
    GW_BIT_DASH /* < */, GW_BIT_DASH /* = */, GW_BIT_DASH /* > */, GW_BIT_DASH /* ? */,
    GW_BIT_DASH /* @ */, GW_BIT_DASH /* A */, GW_BIT_DASH /* B */, GW_BIT_DASH /* C */,
    GW_BIT_DASH /* D */, GW_BIT_DASH /* E */, GW_BIT_DASH /* F */, GW_BIT_DASH /* G */,
    GW_BIT_H /* H */,    GW_BIT_DASH /* I */, GW_BIT_DASH /* J */, GW_BIT_DASH /* K */,
    GW_BIT_L /* L */,    GW_BIT_DASH /* M */, GW_BIT_DASH /* N */, GW_BIT_DASH /* O */,
    GW_BIT_DASH /* P */, GW_BIT_DASH /* Q */, GW_BIT_DASH /* R */, GW_BIT_DASH /* S */,
    GW_BIT_DASH /* T */, GW_BIT_U /* U */,    GW_BIT_DASH /* V */, GW_BIT_W /* W */,
    GW_BIT_X /* X */,    GW_BIT_DASH /* Y */, GW_BIT_Z /* Z */,    GW_BIT_DASH /* [ */,
    GW_BIT_DASH /* \ */, GW_BIT_DASH /* ] */, GW_BIT_DASH /* ^ */, GW_BIT_DASH /* _ */,
    GW_BIT_DASH /* ` */, GW_BIT_DASH /* a */, GW_BIT_DASH /* b */, GW_BIT_DASH /* c */,
    GW_BIT_DASH /* d */, GW_BIT_DASH /* e */, GW_BIT_DASH /* f */, GW_BIT_DASH /* g */,
    GW_BIT_H /* h */,    GW_BIT_DASH /* i */, GW_BIT_DASH /* j */, GW_BIT_DASH /* k */,
    GW_BIT_L /* l */,    GW_BIT_DASH /* m */, GW_BIT_DASH /* n */, GW_BIT_DASH /* o */,
    GW_BIT_DASH /* p */, GW_BIT_DASH /* q */, GW_BIT_DASH /* r */, GW_BIT_DASH /* s */,
    GW_BIT_DASH /* t */, GW_BIT_U /* u */,    GW_BIT_DASH /* v */, GW_BIT_W /* w */,
    GW_BIT_X /* x */,    GW_BIT_DASH /* y */, GW_BIT_Z /* z */,    GW_BIT_DASH /* { */,
    GW_BIT_DASH /* | */, GW_BIT_DASH /* } */, GW_BIT_DASH /* ~ */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */,
    GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */, GW_BIT_DASH /* . */
};

int vtype(GwTrace *t, char *vec)
{
    int i, nbits;
    char pch, ch;

    if (vec == NULL)
        return (GW_BIT_X);

    nbits = t->n.nd->msi - t->n.nd->lsi;
    if (nbits < 0)
        nbits = -nbits;
    nbits++;
    pch = ch = cvt_table[(unsigned char)vec[0]];
    for (i = 1; i < nbits; i++) {
        ch = cvt_table[(unsigned char)vec[i]];
        if (ch != pch)
            goto miscompare;
    }

    return (ch);

miscompare:
    if ((pch == GW_BIT_X) || (pch == GW_BIT_U))
        return (pch);
    if (pch == GW_BIT_Z)
        return (GW_BIT_X);
    for (; i < nbits; i++) {
        ch = cvt_table[(unsigned char)vec[i]];
        if ((ch == GW_BIT_X) || (ch == GW_BIT_U))
            return (ch);
        if (ch == GW_BIT_Z)
            return (GW_BIT_X);
    }

    return (GW_BIT_COUNT);
}

int vtype2(GwTrace *t, GwVectorEnt *v)
{
    int i, nbits;
    char pch, ch;
    char *vec = (char *)v->v;

    if (!t->t_filter_converted) {
        if (vec == NULL)
            return (GW_BIT_X);
    } else {
        return (((vec == NULL) || (vec[0] == 0)) ? GW_BIT_Z : GW_BIT_COUNT);
    }

    nbits = t->n.vec->nbits;

    pch = ch = cvt_table[(unsigned char)vec[0]];
    for (i = 1; i < nbits; i++) {
        ch = cvt_table[(unsigned char)vec[i]];
        if (ch != pch)
            goto miscompare;
    }

    return (ch);

miscompare:
    if ((pch == GW_BIT_X) || (pch == GW_BIT_U))
        return (pch);
    if (pch == GW_BIT_Z)
        return (GW_BIT_X);
    for (; i < nbits; i++) {
        ch = cvt_table[(unsigned char)vec[i]];
        if ((ch == GW_BIT_X) || (ch == GW_BIT_U))
            return (ch);
        if (ch == GW_BIT_Z)
            return (GW_BIT_X);
    }

    return (GW_BIT_COUNT);
}

/*
 * convert trptr+hptr vectorstring into an ascii string
 */
char *convert_ascii_vec_2(GwTrace *t, char *vec)
{
    TraceFlagsType flags;
    int nbits;
    char *bits;
    char *os, *pnt, *newbuff;
    int i, j, len;
    const char *xtab;

    GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(GLOBALS->dump_file);

    static const char xfwd[GW_BIT_COUNT] = AN_NORMAL;
    static const char xrev[GW_BIT_COUNT] = AN_INVERSE;

    flags = t->flags;

    nbits = t->n.nd->msi - t->n.nd->lsi;
    if (nbits < 0)
        nbits = -nbits;
    nbits++;

    if (vec) {
        bits = vec;
        if (*vec > GW_BIT_MASK) /* convert as needed */
            for (i = 0; i < nbits; i++) {
                vec[i] = cvt_table[(unsigned char)vec[i]];
            }
    } else {
        pnt = bits = g_alloca(nbits);
        for (i = 0; i < nbits; i++) {
            *pnt++ = GW_BIT_X;
        }
    }

    if ((flags & (TR_ZEROFILL | TR_ONEFILL)) && (nbits > 1) && (t->n.nd->msi) && (t->n.nd->lsi)) {
        char whichfill = (flags & TR_ZEROFILL) ? GW_BIT_0 : GW_BIT_1;

        if (t->n.nd->msi > t->n.nd->lsi) {
            if (t->n.nd->lsi > 0) {
                pnt = g_alloca(t->n.nd->msi + 1);

                memcpy(pnt, bits, nbits);

                for (i = nbits; i < t->n.nd->msi + 1; i++) {
                    pnt[i] = whichfill;
                }

                bits = pnt;
                nbits = t->n.nd->msi + 1;
            }
        } else {
            if (t->n.nd->msi > 0) {
                pnt = g_alloca(t->n.nd->lsi + 1);

                for (i = 0; i < t->n.nd->msi; i++) {
                    pnt[i] = whichfill;
                }

                memcpy(pnt + i, bits, nbits);

                bits = pnt;
                nbits = t->n.nd->lsi + 1;
            }
        }
    }

    if (flags & TR_INVERT) {
        xtab = xrev;
    } else {
        xtab = xfwd;
    }

    newbuff = (char *)malloc_2(nbits + 6); /* for justify */
    if (flags & TR_REVERSE) {
        char *fwdpnt, *revpnt;

        fwdpnt = bits;
        revpnt = newbuff + nbits + 6;
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(--revpnt) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
    } else {
        char *fwdpnt, *fwdpnt2;

        fwdpnt = bits;
        fwdpnt2 = newbuff;
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(fwdpnt2++) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
    }

    if (flags & TR_ASCII) {
        char *parse;
        int found = 0;

        free_2(newbuff);
        newbuff = (char *)malloc_2(nbits + 14); /* for justify */
        if (flags & TR_REVERSE) {
            char *fwdpnt, *revpnt;

            fwdpnt = (char *)bits;
            revpnt = newbuff + nbits + 14;
            for (i = 0; i < 7; i++)
                *(--revpnt) = xfwd[0];
            for (i = 0; i < nbits; i++) {
                *(--revpnt) = xtab[(int)(*(fwdpnt++))];
            }
            for (i = 0; i < 7; i++)
                *(--revpnt) = xfwd[0];
        } else {
            char *fwdpnt, *fwdpnt2;

            fwdpnt = (char *)bits;
            fwdpnt2 = newbuff;
            for (i = 0; i < 7; i++)
                *(fwdpnt2++) = xfwd[0];
            for (i = 0; i < nbits; i++) {
                *(fwdpnt2++) = xtab[(int)(*(fwdpnt++))];
            }
            for (i = 0; i < 7; i++)
                *(fwdpnt2++) = xfwd[0];
        }

        len = (nbits / 8) + 2 + 2; /* $xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '"';
        }

        parse = (flags & TR_RJUSTIFY) ? (newbuff + ((nbits + 7) & 7)) : (newbuff + 7);
        cvt_gray(flags, parse, (flags & TR_RJUSTIFY) ? ((nbits + 7) & ~7) : nbits);

        for (i = 0; i < nbits; i += 8) {
            unsigned long val;

            val = 0;
            for (j = 0; j < 8; j++) {
                val <<= 1;

                if ((parse[j] == GW_BIT_X) || (parse[j] == GW_BIT_Z) || (parse[j] == GW_BIT_W) ||
                    (parse[j] == GW_BIT_U) || (parse[j] == GW_BIT_DASH)) {
                    val = 1000; /* arbitrarily large */
                }
                if ((parse[j] == GW_BIT_1) || (parse[j] == GW_BIT_H)) {
                    val |= 1;
                }
            }

            if (val) {
                if (val > 0x7f || !isprint(val))
                    *pnt++ = '.';
                else
                    *pnt++ = val;
                found = 1;
            }

            parse += 8;
        }
        if (!found && !GLOBALS->show_base) {
            *(pnt++) = '"';
            *(pnt++) = '"';
        }

        if (GLOBALS->show_base) {
            *(pnt++) = '"';
        }
        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if ((flags & TR_HEX) || ((flags & (TR_DEC | TR_SIGNED)) && (nbits > 64) &&
                                    (!(flags & (TR_POPCNT | TR_FFO))))) {
        char *parse;

        len = (nbits / 4) + 2 + 1; /* $xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '$';
        }

        parse = (flags & TR_RJUSTIFY) ? (newbuff + ((nbits + 3) & 3)) : (newbuff + 3);
        cvt_gray(flags, parse, (flags & TR_RJUSTIFY) ? ((nbits + 3) & ~3) : nbits);

        for (i = 0; i < nbits; i += 4) {
            unsigned char val;

            val = 0;
            for (j = 0; j < 4; j++) {
                val <<= 1;

                if ((parse[j] == GW_BIT_1) || (parse[j] == GW_BIT_H)) {
                    val |= 1;
                } else if ((parse[j] == GW_BIT_0) || (parse[j] == GW_BIT_L)) {
                } else if (parse[j] == GW_BIT_X) {
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_X) {
                            char *thisbyt = parse + i + k;
                            char *lastbyt = newbuff + 3 + nbits - 1;
                            if ((lastbyt - thisbyt) >= 0)
                                match = 0;
                            break;
                        }
                    }
                    val = (match) ? 16 : 21;
                    break;
                } else if (parse[j] == GW_BIT_Z) {
                    int xover = 0;
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_Z) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            } else {
                                char *thisbyt = parse + i + k;
                                char *lastbyt = newbuff + 3 + nbits - 1;
                                if ((lastbyt - thisbyt) >= 0)
                                    match = 0;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = (match) ? 17 : 22;
                    break;
                } else if (parse[j] == GW_BIT_W) {
                    int xover = 0;
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_W) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            } else {
                                char *thisbyt = parse + i + k;
                                char *lastbyt = newbuff + 3 + nbits - 1;
                                if ((lastbyt - thisbyt) >= 0)
                                    match = 0;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = (match) ? 18 : 23;
                    break;
                } else if (parse[j] == GW_BIT_U) {
                    int xover = 0;
                    int match = (j == 0) || ((parse + i + j) == (newbuff + 3));
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_U) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            } else {
                                char *thisbyt = parse + i + k;
                                char *lastbyt = newbuff + 3 + nbits - 1;
                                if ((lastbyt - thisbyt) >= 0)
                                    match = 0;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = (match) ? 19 : 24;
                    break;
                } else if (parse[j] == GW_BIT_DASH) {
                    int xover = 0;
                    int k;
                    for (k = j + 1; k < 4; k++) {
                        if (parse[k] != GW_BIT_DASH) {
                            if (parse[k] == GW_BIT_X) {
                                xover = 1;
                            }
                            break;
                        }
                    }

                    if (xover)
                        val = 21;
                    else
                        val = 20;
                    break;
                }
            }

            *(pnt++) = AN_HEX_STR[val];

            parse += 4;
        }

        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if (flags & TR_OCT) {
        char *parse;

        len = (nbits / 3) + 2 + 1; /* #xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '#';
        }

        parse = (flags & TR_RJUSTIFY) ? (newbuff + ((nbits % 3) ? (nbits % 3) : 3)) : (newbuff + 3);
        cvt_gray(flags, parse, (flags & TR_RJUSTIFY) ? (((nbits + 2) / 3) * 3) : nbits);

        for (i = 0; i < nbits; i += 3) {
            unsigned char val;

            val = 0;
            for (j = 0; j < 3; j++) {
                val <<= 1;

                if (parse[j] == GW_BIT_X) {
                    val = 8;
                    break;
                }
                if (parse[j] == GW_BIT_Z) {
                    val = 9;
                    break;
                }
                if (parse[j] == GW_BIT_W) {
                    val = 10;
                    break;
                }
                if (parse[j] == GW_BIT_U) {
                    val = 11;
                    break;
                }
                if (parse[j] == GW_BIT_DASH) {
                    val = 12;
                    break;
                }

                if ((parse[j] == GW_BIT_1) || (parse[j] == GW_BIT_H)) {
                    val |= 1;
                }
            }

            *(pnt++) = AN_OCT_STR[val];
            parse += 3;
        }

        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if (flags & TR_BIN) {
        char *parse;

        len = (nbits / 1) + 2 + 1; /* %xxxxx */
        os = pnt = (char *)calloc_2(1, len);
        if (GLOBALS->show_base) {
            *(pnt++) = '%';
        }

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        for (i = 0; i < nbits; i++) {
            *(pnt++) = gw_bit_to_char((int)(*(parse++)));
        }

        *(pnt) = 0x00; /* scan build : remove dead increment */
    } else if (flags & TR_SIGNED) {
        char *parse;
        GwTime val = 0;
        unsigned char fail = 0;

        len = 21; /* len+1 of 0x8000000000000000 expressed in decimal */
        os = (char *)calloc_2(1, len);

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        if ((parse[0] == GW_BIT_1) || (parse[0] == GW_BIT_H)) {
            val = GW_TIME_CONSTANT(-1);
        } else if ((parse[0] == GW_BIT_0) || (parse[0] == GW_BIT_L)) {
            val = GW_TIME_CONSTANT(0);
        } else {
            fail = 1;
        }

        if (!fail)
            for (i = 1; i < nbits; i++) {
                val <<= 1;

                if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                    val |= GW_TIME_CONSTANT(1);
                } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                    fail = 1;
                    break;
                }
            }

        if (!fail) {
            if ((flags & TR_FPDECSHIFT) && (t->t_fpdecshift)) {
                cvt_fpsdec(t, val, os, len, nbits);
            } else {
                if (!(flags & TR_TIME)) {
                    sprintf(os, "%" GW_TIME_FORMAT, val);
                } else {
                    free_2(os);
                    os = calloc_2(1, 128);
                    reformat_time(os, val, time_dimension);
                }
            }
        } else {
            strcpy(os, "XXX");
        }
    } else if (flags & TR_REAL) {
        char *parse;

        if ((nbits == 64) || (nbits == 32)) {
            GwUTime utt = GW_TIME_CONSTANT(0);
            double d;
            float f;
            uint32_t utt_32;

            parse = newbuff + 3;
            cvt_gray(flags, parse, nbits);

            for (i = 0; i < nbits; i++) {
                char ch = gw_bit_to_char((int)(*(parse++)));
                if ((ch == '0') || (ch == '1')) {
                    utt <<= 1;
                    if (ch == '1') {
                        utt |= GW_TIME_CONSTANT(1);
                    }
                } else {
                    goto rl_go_binary;
                }
            }

            os = /*pnt=*/(char *)calloc_2(1, 64); /* scan-build */

            if (nbits == 64) {
                memcpy(&d, &utt, sizeof(double));
                dpr_e16(os, d); /* sprintf(os, "%.16g", d); */
            } else {
                utt_32 = utt;
                memcpy(&f, &utt_32, sizeof(float));
                sprintf(os, "%f", f);
            }
        } else {
        rl_go_binary:
            len = (nbits / 1) + 2 + 1; /* %xxxxx */
            os = pnt = (char *)calloc_2(1, len);
            if (GLOBALS->show_base) {
                *(pnt++) = '%';
            }

            parse = newbuff + 3;
            cvt_gray(flags, parse, nbits);

            for (i = 0; i < nbits; i++) {
                *(pnt++) = gw_bit_to_char((int)(*(parse++)));
            }

            *(pnt) = 0x00; /* scan build : remove dead increment */
        }
    } else /* decimal when all else fails */
    {
        char *parse;
        GwUTime val = 0;
        unsigned char fail = 0;

        len = 21; /* len+1 of 0xffffffffffffffff expressed in decimal */
        os = (char *)calloc_2(1, len);

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        for (i = 0; i < nbits; i++) {
            val <<= 1;

            if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                val |= GW_TIME_CONSTANT(1);
            } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                fail = 1;
                break;
            }
        }

        if (!fail) {
            if ((flags & TR_FPDECSHIFT) && (t->t_fpdecshift)) {
                cvt_fpsudec(t, val, os, len);
            } else {
                if (!(flags & TR_TIME)) {
                    sprintf(os, "%" GW_UTIME_FORMAT, val);
                } else {
                    free_2(os);
                    os = calloc_2(1, 128);
                    reformat_time(os, val, time_dimension);
                }
            }
        } else {
            strcpy(os, "XXX");
        }
    }

    free_2(newbuff);
    return (os);
}

char *convert_ascii_vec(GwTrace *t, char *vec)
{
    char *s = convert_ascii_vec_2(t, vec);

    if (!(t->f_filter | t->p_filter | t->e_filter)) {
        if (GLOBALS->lz_removal)
            lzremoval(s);
    } else {
        if (t->e_filter) {
            s = edofilter(t, s);
        } else if (t->f_filter) {
            s = dofilter(t, s);
        } else {
            s = pdofilter(t, s);
        }
    }

    return (s);
}

char *convert_ascii(GwTrace *t, GwVectorEnt *v)
{
    char *s;

    if ((!t->t_filter_converted) && (!(v->flags & GW_HIST_ENT_FLAG_STRING))) {
        s = convert_ascii_2(t, v);
    } else {
        s = strdup_2((char *)v->v);

        if ((*s == '?') && (!GLOBALS->color_active_in_filter)) {
            char *s2a;
            char *s2 = strchr(s + 1, '?');
            if (s2) {
                s2++;
                s2a = malloc_2(strlen(s2) + 1);
                strcpy(s2a, s2);
                free_2(s);
                s = s2a;
            }
        }
    }

    if (!(t->f_filter | t->p_filter | t->e_filter)) {
        if (GLOBALS->lz_removal)
            lzremoval(s);
    } else {
        if (t->e_filter) {
            s = edofilter(t, s);
        } else if (t->f_filter) {
            s = dofilter(t, s);
        } else {
            s = pdofilter(t, s);
        }
    }

    return (s);
}

/*
 * convert trptr+hptr vectorstring into a real
 */
double convert_real_vec(GwTrace *t, char *vec)
{
    Ulong flags;
    int nbits;
    char *bits;
    char *pnt, *newbuff;
    int i;
    const char *xtab;
    double mynan = strtod("NaN", NULL);
    double retval = mynan;

    static const char xfwd[GW_BIT_COUNT] = AN_NORMAL;
    static const char xrev[GW_BIT_COUNT] = AN_INVERSE;

    flags = t->flags;

    nbits = t->n.nd->msi - t->n.nd->lsi;
    if (nbits < 0)
        nbits = -nbits;
    nbits++;

    if (vec) {
        bits = vec;
        if (*vec > GW_BIT_MASK) /* convert as needed */
            for (i = 0; i < nbits; i++) {
                vec[i] = cvt_table[(unsigned char)vec[i]];
            }
    } else {
        pnt = bits = g_alloca(nbits);
        for (i = 0; i < nbits; i++) {
            *pnt++ = GW_BIT_X;
        }
    }

    if (flags & TR_INVERT) {
        xtab = xrev;
    } else {
        xtab = xfwd;
    }

    newbuff = (char *)malloc_2(nbits + 6); /* for justify */
    if (flags & TR_REVERSE) {
        char *fwdpnt, *revpnt;

        fwdpnt = bits;
        revpnt = newbuff + nbits + 6;
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(--revpnt) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
    } else {
        char *fwdpnt, *fwdpnt2;

        fwdpnt = bits;
        fwdpnt2 = newbuff;
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(fwdpnt2++) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
    }

    if (flags & TR_REAL) {
        if ((nbits == 64) || (nbits == 32)) /* fail (NaN) otherwise */
        {
            char *parse;
            GwUTime val = 0;
            unsigned char fail = 0;
            uint32_t val_32;

            parse = newbuff + 3;
            for (i = 0; i < nbits; i++) {
                val <<= 1;

                if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                    val |= GW_TIME_CONSTANT(1);
                } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                    fail = 1;
                    break;
                }
            }

            if (!fail) {
                if (nbits == 64) {
                    memcpy(&retval, &val, sizeof(double)); /* otherwise strict-aliasing rules
                                                            *problem if retval = (double *)&val; */
                } else {
                    float f;
                    val_32 = val;
                    memcpy(&f, &val_32, sizeof(float)); /* otherwise strict-aliasing rules problem
                                                           if retval = *(double *)&val; */
                    retval = (double)f;
                }
            }
        }
    } else {
        if (flags & TR_SIGNED) {
            char *parse;
            GwTime val = 0;
            unsigned char fail = 0;

            parse = newbuff + 3;
            cvt_gray(flags, parse, nbits);

            if ((parse[0] == GW_BIT_1) || (parse[0] == GW_BIT_H)) {
                val = GW_TIME_CONSTANT(-1);
            } else if ((parse[0] == GW_BIT_0) || (parse[0] == GW_BIT_L)) {
                val = GW_TIME_CONSTANT(0);
            } else {
                fail = 1;
            }

            if (!fail)
                for (i = 1; i < nbits; i++) {
                    val <<= 1;

                    if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                        val |= GW_TIME_CONSTANT(1);
                    } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                        fail = 1;
                        break;
                    }
                }
            if (!fail) {
                retval = val;
            }
        } else /* decimal when all else fails */
        {
            char *parse;
            GwUTime val = 0;
            unsigned char fail = 0;

            parse = newbuff + 3;
            cvt_gray(flags, parse, nbits);

            for (i = 0; i < nbits; i++) {
                val <<= 1;

                if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                    val |= GW_TIME_CONSTANT(1);
                } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                    fail = 1;
                    break;
                }
            }
            if (!fail) {
                retval = val;
            }
        }
    }

    free_2(newbuff);
    return (retval);
}

/*
 * convert trptr+vptr into a real
 */
double convert_real(GwTrace *t, GwVectorEnt *v)
{
    Ulong flags;
    int nbits;
    unsigned char *bits;
    char *newbuff;
    int i;
    const char *xtab;
    double mynan = strtod("NaN", NULL);
    double retval = mynan;

    static const char xfwd[GW_BIT_COUNT] = AN_NORMAL;
    static const char xrev[GW_BIT_COUNT] = AN_INVERSE;

    flags = t->flags;
    nbits = t->n.vec->nbits;
    bits = v->v;

    if (flags & TR_INVERT) {
        xtab = xrev;
    } else {
        xtab = xfwd;
    }

    newbuff = (char *)malloc_2(nbits + 6); /* for justify */
    if (flags & TR_REVERSE) {
        char *fwdpnt, *revpnt;

        fwdpnt = (char *)bits;
        revpnt = newbuff + nbits + 6;
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(--revpnt) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(--revpnt) = xfwd[0];
    } else {
        char *fwdpnt, *fwdpnt2;

        fwdpnt = (char *)bits;
        fwdpnt2 = newbuff;
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
        for (i = 0; i < nbits; i++) {
            *(fwdpnt2++) = xtab[(int)(*(fwdpnt++))];
        }
        /* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
        for (i = 0; i < 3; i++)
            *(fwdpnt2++) = xfwd[0];
    }

    if (flags & TR_SIGNED) {
        char *parse;
        GwTime val = 0;
        unsigned char fail = 0;

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        if ((parse[0] == GW_BIT_1) || (parse[0] == GW_BIT_H)) {
            val = GW_TIME_CONSTANT(-1);
        } else if ((parse[0] == GW_BIT_0) || (parse[0] == GW_BIT_L)) {
            val = GW_TIME_CONSTANT(0);
        } else {
            fail = 1;
        }

        if (!fail)
            for (i = 1; i < nbits; i++) {
                val <<= 1;

                if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                    val |= GW_TIME_CONSTANT(1);
                } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                    fail = 1;
                    break;
                }
            }

        if (!fail) {
            retval = val;
        }
    } else /* decimal when all else fails */
    {
        char *parse;
        GwUTime val = 0;
        unsigned char fail = 0;

        parse = newbuff + 3;
        cvt_gray(flags, parse, nbits);

        for (i = 0; i < nbits; i++) {
            val <<= 1;

            if ((parse[i] == GW_BIT_1) || (parse[i] == GW_BIT_H)) {
                val |= GW_TIME_CONSTANT(1);
            } else if ((parse[i] != GW_BIT_0) && (parse[i] != GW_BIT_L)) {
                fail = 1;
                break;
            }
        }

        if (!fail) {
            retval = val;
        }
    }

    free_2(newbuff);
    return (retval);
}
