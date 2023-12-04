/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/*
 * vcd.c 			23jan99ajb
 * evcd parts 			29jun99ajb
 * profiler optimizations 	15jul99ajb
 * more profiler optimizations	25jan00ajb
 * finsim parameter fix		26jan00ajb
 * vector rechaining code	03apr00ajb
 * multiple var section code	06apr00ajb
 * fix for duplicate nets	19dec00ajb
 * support for alt hier seps	23dec00ajb
 * fix for rcs identifiers	16jan01ajb
 * coredump fix for bad VCD	04apr02ajb
 * min/maxid speedup            27feb03ajb
 * bugfix on min/maxid speedup  06jul03ajb
 * escaped hier modification    20feb06ajb
 * added real_parameter vartype 04aug06ajb
 * recoder using vlists         17aug06ajb
 * code cleanup                 04sep06ajb
 * added in/out port vartype    31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 * VCD fastloading		05mar09ajb
 * Backtracking fix             16oct18ajb
 */
#include <config.h>
#include "globals.h"
#include "vcd.h"
#include "vlist.h"
#include "lx2.h"
#include "hierpack.h"

struct _VcdFile
{
    struct vlist_t *time_vlist;

    GwTime start_time;
    GwTime end_time;
};

typedef struct
{
    VcdFile *file;

    FILE *vcd_handle;
    gboolean is_compressed;
    off_t vcd_fsiz;

    gboolean header_over;

    unsigned int time_vlist_count;

    off_t vcdbyteno;
    char *vcdbuf;
    char *vst;
    char *vend;

    int error_count;
    gboolean err;

    GwTime current_time;

    struct vcdsymbol *pv;
    struct vcdsymbol *rootv;

    int T_MAX_STR;
    char *yytext;
    int yylen;

    struct vcdsymbol *vcdsymroot;
    struct vcdsymbol *vcdsymcurr;

    int numsyms;
    struct vcdsymbol **symbols_sorted;
    struct vcdsymbol **symbols_indexed;

    guint vcd_minid;
    guint vcd_maxid;
    guint vcd_hash_max;
    gboolean vcd_hash_kill;

    char *varsplit;
    char *vsplitcurr;
    int var_prevch;

    gboolean already_backtracked;

    GSList *sym_chain;
} VcdLoader;

/**/

static void malform_eof_fix(VcdLoader *self)
{
    if (feof(self->vcd_handle)) {
        memset(self->vcdbuf, ' ', VCD_BSIZ);
        self->vst = self->vend;
    }
}

/**/

static void vlist_packer_emit_uv32(struct vlist_packer_t **vl, unsigned int v)
{
    unsigned int nxt;

    while ((nxt = v >> 7)) {
        vlist_packer_alloc(*vl, v & 0x7f);
        v = nxt;
    }

    vlist_packer_alloc(*vl, (v & 0x7f) | 0x80);
}

static void vlist_packer_emit_string(struct vlist_packer_t **vl, const char *s)
{
    while (*s) {
        vlist_packer_alloc(*vl, *s);
        s++;
    }
    vlist_packer_alloc(*vl, 0);
}

static void vlist_packer_emit_mvl9_string(struct vlist_packer_t **vl, const char *s)
{
    unsigned int recoded_bit;
    unsigned char which = 0;
    unsigned char accum = 0;

    while (*s) {
        switch (*s) {
            case '0':
                recoded_bit = AN_0;
                break;
            case '1':
                recoded_bit = AN_1;
                break;
            case 'x':
            case 'X':
                recoded_bit = AN_X;
                break;
            case 'z':
            case 'Z':
                recoded_bit = AN_Z;
                break;
            case 'h':
            case 'H':
                recoded_bit = AN_H;
                break;
            case 'u':
            case 'U':
                recoded_bit = AN_U;
                break;
            case 'w':
            case 'W':
                recoded_bit = AN_W;
                break;
            case 'l':
            case 'L':
                recoded_bit = AN_L;
                break;
            default:
                recoded_bit = AN_DASH;
                break;
        }

        if (!which) {
            accum = (recoded_bit << 4);
            which = 1;
        } else {
            accum |= recoded_bit;
            vlist_packer_alloc(*vl, accum);
            which = accum = 0;
        }
        s++;
    }

    recoded_bit = AN_MSK; /* XXX : this is assumed it is going to remain a 4 bit max quantity! */
    if (!which) {
        accum = (recoded_bit << 4);
    } else {
        accum |= recoded_bit;
    }

    vlist_packer_alloc(*vl, accum);
}

/**/

static void vlist_emit_uv32(struct vlist_t **vl, unsigned int v)
{
    unsigned int nxt;
    char *pnt;

    if (GLOBALS->vlist_prepack) {
        vlist_packer_emit_uv32((struct vlist_packer_t **)vl, v);
        return;
    }

    while ((nxt = v >> 7)) {
        pnt = vlist_alloc(vl, 1);
        *pnt = (v & 0x7f);
        v = nxt;
    }

    pnt = vlist_alloc(vl, 1);
    *pnt = (v & 0x7f) | 0x80;
}

static void vlist_emit_string(struct vlist_t **vl, const char *s)
{
    char *pnt;

    if (GLOBALS->vlist_prepack) {
        vlist_packer_emit_string((struct vlist_packer_t **)vl, s);
        return;
    }

    while (*s) {
        pnt = vlist_alloc(vl, 1);
        *pnt = *s;
        s++;
    }
    pnt = vlist_alloc(vl, 1);
    *pnt = 0;
}

static void vlist_emit_mvl9_string(struct vlist_t **vl, const char *s)
{
    char *pnt;
    unsigned int recoded_bit;
    unsigned char which;
    unsigned char accum;

    if (GLOBALS->vlist_prepack) {
        vlist_packer_emit_mvl9_string((struct vlist_packer_t **)vl, s);
        return;
    }

    which = accum = 0;

    while (*s) {
        switch (*s) {
            case '0':
                recoded_bit = AN_0;
                break;
            case '1':
                recoded_bit = AN_1;
                break;
            case 'x':
            case 'X':
                recoded_bit = AN_X;
                break;
            case 'z':
            case 'Z':
                recoded_bit = AN_Z;
                break;
            case 'h':
            case 'H':
                recoded_bit = AN_H;
                break;
            case 'u':
            case 'U':
                recoded_bit = AN_U;
                break;
            case 'w':
            case 'W':
                recoded_bit = AN_W;
                break;
            case 'l':
            case 'L':
                recoded_bit = AN_L;
                break;
            default:
                recoded_bit = AN_DASH;
                break;
        }

        if (!which) {
            accum = (recoded_bit << 4);
            which = 1;
        } else {
            accum |= recoded_bit;
            pnt = vlist_alloc(vl, 1);
            *pnt = accum;
            which = accum = 0;
        }
        s++;
    }

    recoded_bit = AN_MSK; /* XXX : this is assumed it is going to remain a 4 bit max quantity! */
    if (!which) {
        accum = (recoded_bit << 4);
    } else {
        accum |= recoded_bit;
    }

    pnt = vlist_alloc(vl, 1);
    *pnt = accum;
}

/**/

#undef VCD_BSEARCH_IS_PERFECT /* bsearch is imperfect under linux, but OK under AIX */

static void add_histent(VcdFile *self, GwTime time, GwNode *n, char ch, int regadd, char *vector);
static void vcd_build_symbols(VcdLoader *self);
static void vcd_cleanup(VcdLoader *self);
static void evcd_strcpy(char *dst, char *src);

/******************************************************************/

enum Tokens
{
    T_VAR,
    T_END,
    T_SCOPE,
    T_UPSCOPE,
    T_COMMENT,
    T_DATE,
    T_DUMPALL,
    T_DUMPOFF,
    T_DUMPON,
    T_DUMPVARS,
    T_ENDDEFINITIONS,
    T_DUMPPORTS,
    T_DUMPPORTSOFF,
    T_DUMPPORTSON,
    T_DUMPPORTSALL,
    T_TIMESCALE,
    T_VERSION,
    T_VCDCLOSE,
    T_TIMEZERO,
    T_EOF,
    T_STRING,
    T_UNKNOWN_KEY
};

static const char *tokens[] = {"var",
                               "end",
                               "scope",
                               "upscope",
                               "comment",
                               "date",
                               "dumpall",
                               "dumpoff",
                               "dumpon",
                               "dumpvars",
                               "enddefinitions",
                               "dumpports",
                               "dumpportsoff",
                               "dumpportson",
                               "dumpportsall",
                               "timescale",
                               "version",
                               "vcdclose",
                               "timezero",
                               "",
                               "",
                               ""};

#define NUM_TOKENS 19

#define T_GET \
    tok = get_token(self); \
    if ((tok == T_END) || (tok == T_EOF)) \
        break;

/******************************************************************/

static unsigned int vcdid_hash(char *s, int len)
{
    unsigned int val = 0;
    int i;

    s += (len - 1);

    for (i = 0; i < len; i++) {
        val *= 94;
        val += (((unsigned char)*s) - 32);
        s--;
    }

    return (val);
}

/******************************************************************/

/*
 * bsearch compare
 */
static int vcdsymbsearchcompare(const void *s1, const void *s2)
{
    char *v1;
    struct vcdsymbol *v2;

    v1 = (char *)s1;
    v2 = *((struct vcdsymbol **)s2);

    return (strcmp(v1, v2->id));
}

/*
 * actual bsearch
 */
static struct vcdsymbol *bsearch_vcd(VcdLoader *self, char *key, int len)
{
    struct vcdsymbol **v;
    struct vcdsymbol *t;

    if (self->symbols_indexed != NULL) {
        unsigned int hsh = vcdid_hash(key, len);
        if (hsh >= self->vcd_minid && hsh <= self->vcd_maxid) {
            return (self->symbols_indexed[hsh - self->vcd_minid]);
        }

        return NULL;
    }

    if (self->symbols_sorted != NULL) {
        v = (struct vcdsymbol **)bsearch(key,
                                         self->symbols_sorted,
                                         self->numsyms,
                                         sizeof(struct vcdsymbol *),
                                         vcdsymbsearchcompare);

        if (v) {
#ifndef VCD_BSEARCH_IS_PERFECT
            for (;;) {
                t = *v;

                if ((v == self->symbols_sorted) || (strcmp((*(--v))->id, key))) {
                    return (t);
                }
            }
#else
            return (*v);
#endif
        } else {
            return (NULL);
        }
    } else {
        if (!self->err) {
            fprintf(stderr,
                    "Near byte %d, VCD search table NULL..is this a VCD file?\n",
                    (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
            self->err = TRUE;
        }
        return (NULL);
    }
}

/*
 * sort on vcdsymbol pointers
 */
static int vcdsymcompare(const void *s1, const void *s2)
{
    struct vcdsymbol *v1, *v2;

    v1 = *((struct vcdsymbol **)s1);
    v2 = *((struct vcdsymbol **)s2);

    return (strcmp(v1->id, v2->id));
}

/*
 * create sorted (by id) table
 */
static void create_sorted_table(VcdLoader *self)
{
    struct vcdsymbol *v;
    struct vcdsymbol **pnt;
    unsigned int vcd_distance;

    g_clear_pointer(&self->symbols_sorted, free_2);
    g_clear_pointer(&self->symbols_indexed, free_2);

    if (self->numsyms > 0) {
        vcd_distance = self->vcd_maxid - self->vcd_minid + 1;

        if ((vcd_distance <= VCD_INDEXSIZ) || !self->vcd_hash_kill) {
            self->symbols_indexed = calloc_2(vcd_distance, sizeof(struct vcdsymbol *));

            /* printf("%d symbols span ID range of %d, using indexing... hash_kill = %d\n",
             * self->numsyms, vcd_distance, GLOBALS->vcd_hash_kill);  */

            v = self->vcdsymroot;
            while (v) {
                if (self->symbols_indexed[v->nid - self->vcd_minid] == NULL) {
                    self->symbols_indexed[v->nid - self->vcd_minid] = v;
                }
                v = v->next;
            }
        } else {
            pnt = self->symbols_sorted = calloc_2(self->numsyms, sizeof(struct vcdsymbol *));
            v = self->vcdsymroot;
            while (v) {
                *(pnt++) = v;
                v = v->next;
            }

            qsort(self->symbols_sorted, self->numsyms, sizeof(struct vcdsymbol *), vcdsymcompare);
        }
    }
}

/******************************************************************/

static unsigned int vlist_emit_finalize(VcdLoader *self)
{
    struct vcdsymbol *v /* , *vprime */; /* scan-build */
    struct vlist_t *vlist;
    char vlist_prepack = GLOBALS->vlist_prepack;
    int cnt = 0;

    v = self->vcdsymroot;
    while (v) {
        GwNode *n = v->narray[0];

        set_vcd_vartype(v, n);

        if (n->mv.mvlfac_vlist) {
            if (vlist_prepack) {
                vlist_packer_finalize(n->mv.mvlfac_packer_vlist);
                vlist = n->mv.mvlfac_packer_vlist->v;
                free_2(n->mv.mvlfac_packer_vlist);
                n->mv.mvlfac_vlist = vlist;
                vlist_freeze(&n->mv.mvlfac_vlist);
            } else {
                vlist_freeze(&n->mv.mvlfac_vlist);
            }
        } else {
            n->mv.mvlfac_vlist = vlist_prepack ? ((struct vlist_t *)vlist_packer_create())
                                               : vlist_create(sizeof(char));

            if ((/* vprime= */ bsearch_vcd(self, v->id, strlen(v->id))) ==
                v) /* hash mish means dup net */ /* scan-build */
            {
                switch (v->vartype) {
                    case V_REAL:
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, 'R');
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, 0);
                        vlist_emit_string(&n->mv.mvlfac_vlist, "NaN");
                        break;

                    case V_STRINGTYPE:
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, 'S');
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, 0);
                        vlist_emit_string(&n->mv.mvlfac_vlist, "UNDEF");
                        break;

                    default:
                        if (v->size == 1) {
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)'0');
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, RCV_X);
                        } else {
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, 'B');
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
                            vlist_emit_uv32(&n->mv.mvlfac_vlist, 0);
                            vlist_emit_mvl9_string(&n->mv.mvlfac_vlist, "x");
                        }
                        break;
                }
            }

            if (vlist_prepack) {
                vlist_packer_finalize(n->mv.mvlfac_packer_vlist);
                vlist = n->mv.mvlfac_packer_vlist->v;
                free_2(n->mv.mvlfac_packer_vlist);
                n->mv.mvlfac_vlist = vlist;
                vlist_freeze(&n->mv.mvlfac_vlist);
            } else {
                vlist_freeze(&n->mv.mvlfac_vlist);
            }
        }
        v = v->next;
        cnt++;
    }

    return (cnt);
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(VcdLoader *self)
{
    self->vcdbuf = calloc_2(1, VCD_BSIZ);
    self->vst = self->vcdbuf;
    self->vend = self->vcdbuf;
}

static void getch_free(VcdLoader *self)
{
    free_2(self->vcdbuf);
    self->vcdbuf = NULL;
    self->vst = NULL;
    self->vend = NULL;
}

static int getch_fetch(VcdLoader *self)
{
    size_t rd;

    errno = 0;
    if (feof(self->vcd_handle))
        return (-1);

    self->vcdbyteno += (self->vend - self->vcdbuf);
    rd = fread(self->vcdbuf, sizeof(char), VCD_BSIZ, self->vcd_handle);
    self->vend = (self->vst = self->vcdbuf) + rd;

    if ((!rd) || (errno))
        return (-1);

    if (self->vcd_fsiz > 0) {
        splash_sync(self->vcdbyteno, self->vcd_fsiz); /* gnome 2.18 seems to set errno so splash
                                                                            moved here... */
    }

    return ((int)(*self->vst));
}

static inline signed char getch(VcdLoader *self)
{
    signed char ch = (self->vst != self->vend) ? ((int)(*self->vst)) : (getch_fetch(self));
    self->vst++;
    return (ch);
}

static inline signed char getch_peek(VcdLoader *self)
{
    signed char ch = (self->vst != self->vend) ? ((int)(*self->vst)) : (getch_fetch(self));
    /* no increment */
    return (ch);
}

static int getch_patched(VcdLoader *self)
{
    char ch;

    ch = *self->vsplitcurr;
    if (!ch) {
        return (-1);
    } else {
        self->vsplitcurr++;
        return ((int)ch);
    }
}

/*
 * simple tokenizer
 */
static int get_token(VcdLoader *self)
{
    int ch;
    int i, len = 0;
    int is_string = 0;
    char *yyshadow;

    for (;;) {
        ch = getch(self);
        if (ch < 0)
            return (T_EOF);
        if (ch <= ' ')
            continue; /* val<=' ' is a quick whitespace check      */
        break; /* (take advantage of fact that vcd is text) */
    }
    if (ch == '$') {
        self->yytext[len++] = ch;
        for (;;) {
            ch = getch(self);
            if (ch < 0)
                return (T_EOF);
            if (ch <= ' ')
                continue;
            break;
        }
    } else {
        is_string = 1;
    }

    for (self->yytext[len++] = ch;; self->yytext[len++] = ch) {
        if (len == self->T_MAX_STR) {
            self->T_MAX_STR *= 2;
            self->yytext = realloc_2(self->yytext, self->T_MAX_STR + 1);
        }
        ch = getch(self);
        if (ch <= ' ')
            break;
    }
    self->yytext[len] = 0; /* terminator */
    self->yylen = len;

    if (is_string) {
        return (T_STRING);
    }

    yyshadow = self->yytext;
    do {
        yyshadow++;
        for (i = 0; i < NUM_TOKENS; i++) {
            if (!strcmp(yyshadow, tokens[i])) {
                return (i);
            }
        }

    } while (*yyshadow == '$'); /* fix for RCS ids in version strings */

    return T_UNKNOWN_KEY;
}

static int get_vartoken_patched(VcdLoader *self, int match_kw)
{
    int ch;
    int len = 0;

    if (!self->var_prevch) {
        for (;;) {
            ch = getch_patched(self);
            if (ch < 0) {
                free_2(self->varsplit);
                self->varsplit = NULL;
                return (V_END);
            }
            if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r'))
                continue;
            break;
        }
    } else {
        ch = self->var_prevch;
        self->var_prevch = 0;
    }

    if (ch == '[')
        return (V_LB);
    if (ch == ':')
        return (V_COLON);
    if (ch == ']')
        return (V_RB);

    for (self->yytext[len++] = ch;; self->yytext[len++] = ch) {
        if (len == self->T_MAX_STR) {
            self->T_MAX_STR *= 2;
            self->yytext = realloc_2(self->yytext, self->T_MAX_STR + 1);
        }
        ch = getch_patched(self);
        if (ch < 0) {
            free_2(self->varsplit);
            self->varsplit = NULL;
            break;
        }
        if ((ch == ':') || (ch == ']')) {
            self->var_prevch = ch;
            break;
        }
    }
    self->yytext[len] = 0; /* terminator */

    if (match_kw) {
        int vr = vcd_keyword_code(self->yytext, len);
        if (vr != V_STRING) {
            if (ch < 0) {
                free_2(self->varsplit);
                self->varsplit = NULL;
            }
            return (vr);
        }
    }

    self->yylen = len;
    if (ch < 0) {
        free_2(self->varsplit);
        self->varsplit = NULL;
    }
    return V_STRING;
}

static int get_vartoken(VcdLoader *self, int match_kw)
{
    int ch;
    int len = 0;

    if (self->varsplit) {
        int rc = get_vartoken_patched(self, match_kw);
        if (rc != V_END)
            return (rc);
        self->var_prevch = 0;
    }

    if (!self->var_prevch) {
        for (;;) {
            ch = getch(self);
            if (ch < 0)
                return (V_END);
            if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r'))
                continue;
            break;
        }
    } else {
        ch = self->var_prevch;
        self->var_prevch = 0;
    }

    if (ch == '[')
        return (V_LB);
    if (ch == ':')
        return (V_COLON);
    if (ch == ']')
        return (V_RB);

    if (ch == '#') /* for MTI System Verilog '$var reg 64 >w #implicit-var###VarElem:ram_di[0.0]
                      [63:0] $end' style declarations */
    { /* debussy simply escapes until the space */
        self->yytext[len++] = '\\';
    }

    for (self->yytext[len++] = ch;; self->yytext[len++] = ch) {
        if (len == self->T_MAX_STR) {
            self->T_MAX_STR *= 2;
            self->yytext = realloc_2(self->yytext, self->T_MAX_STR + 1);
        }

        ch = getch(self);
        if (ch == ' ') {
            if (match_kw)
                break;
            if (getch_peek(self) == '[') {
                ch = getch(self);
                self->varsplit = self->yytext + len; /* keep looping so we get the *last* one */
                continue;
            }
        }

        if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r') || (ch < 0))
            break;
        if ((ch == '[') && (self->yytext[0] != '\\')) {
            self->varsplit = self->yytext + len; /* keep looping so we get the *last* one */
        } else if (((ch == ':') || (ch == ']')) && (!self->varsplit) && (self->yytext[0] != '\\')) {
            self->var_prevch = ch;
            break;
        }
    }

    self->yytext[len] = 0; /* absolute terminator */
    if ((self->varsplit) && (self->yytext[len - 1] == ']')) {
        char *vst;
        vst = malloc_2(strlen(self->varsplit) + 1);
        strcpy(vst, self->varsplit);

        *self->varsplit = 0x00; /* zero out var name at the left bracket */
        len = self->varsplit - self->yytext;

        self->varsplit = self->vsplitcurr = vst;
        self->var_prevch = 0;
    } else {
        self->varsplit = NULL;
    }

    if (match_kw) {
        int vr = vcd_keyword_code(self->yytext, len);
        if (vr != V_STRING) {
            return (vr);
        }
    }

    self->yylen = len;
    return V_STRING;
}

static int get_strtoken(VcdLoader *self)
{
    int ch;
    int len = 0;

    if (!self->var_prevch) {
        for (;;) {
            ch = getch(self);
            if (ch < 0)
                return (V_END);
            if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r'))
                continue;
            break;
        }
    } else {
        ch = self->var_prevch;
        self->var_prevch = 0;
    }

    for (self->yytext[len++] = ch;; self->yytext[len++] = ch) {
        if (len == self->T_MAX_STR) {
            self->T_MAX_STR *= 2;
            self->yytext = realloc_2(self->yytext, self->T_MAX_STR + 1);
        }
        ch = getch(self);
        if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r') || (ch < 0))
            break;
    }
    self->yytext[len] = 0; /* terminator */

    self->yylen = len;
    return V_STRING;
}

static void sync_end(VcdLoader *self, const char *hdr)
{
    int tok;

    if (hdr) {
        DEBUG(fprintf(stderr, "%s", hdr));
    }
    for (;;) {
        tok = get_token(self);
        if ((tok == T_END) || (tok == T_EOF))
            break;
        if (hdr) {
            DEBUG(fprintf(stderr, " %s", GLOBALS->yytext_vcd_recoder_c_3));
        }
    }
    if (hdr) {
        DEBUG(fprintf(stderr, "\n"));
    }
}

static int version_sync_end(VcdLoader *self, const char *hdr)
{
    int tok;
    int rc = 0;

    if (hdr) {
        DEBUG(fprintf(stderr, "%s", hdr));
    }
    for (;;) {
        tok = get_token(self);
        if ((tok == T_END) || (tok == T_EOF))
            break;
        if (hdr) {
            DEBUG(fprintf(stderr, " %s", GLOBALS->yytext_vcd_recoder_c_3));
        }
        /* turn off autocoalesce for Icarus */
        if (strstr(self->yytext, "Icarus") != NULL) {
            GLOBALS->autocoalesce = 0;
            rc = 1;
        }
    }
    if (hdr) {
        DEBUG(fprintf(stderr, "\n"));
    }
    return (rc);
}

static void parse_valuechange(VcdLoader *self)
{
    struct vcdsymbol *v;
    char *vector;
    int vlen;
    unsigned char typ;

    switch ((typ = self->yytext[0])) {
        /* encode bits as (time delta<<4) + (enum AnalyzerBits value) */
        case '0':
        case '1':
        case 'x':
        case 'X':
        case 'z':
        case 'Z':
        case 'h':
        case 'H':
        case 'u':
        case 'U':
        case 'w':
        case 'W':
        case 'l':
        case 'L':
        case '-':
            if (self->yylen > 1) {
                v = bsearch_vcd(self, self->yytext + 1, self->yylen - 1);
                if (!v) {
                    fprintf(stderr,
                            "Near byte %d, Unknown VCD identifier: '%s'\n",
                            (int)(self->vcdbyteno + (self->vst - self->vcdbuf)),
                            self->yytext + 1);
                    malform_eof_fix(self);
                } else {
                    GwNode *n = v->narray[0];
                    unsigned int time_delta;
                    unsigned int rcv;

                    if (!n->mv
                             .mvlfac_vlist) /* overloaded for vlist, numhist = last position used */
                    {
                        n->mv.mvlfac_vlist = (GLOBALS->vlist_prepack)
                                                 ? ((struct vlist_t *)vlist_packer_create())
                                                 : vlist_create(sizeof(char));
                        vlist_emit_uv32(&n->mv.mvlfac_vlist,
                                        (unsigned int)'0'); /* represents single bit routine for
                                                               decompression */
                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
                    }

                    time_delta = self->time_vlist_count - (unsigned int)n->numhist;
                    n->numhist = self->time_vlist_count;

                    switch (self->yytext[0]) {
                        case '0':
                        case '1':
                            rcv = ((self->yytext[0] & 1) << 1) | (time_delta << 2);
                            break; /* pack more delta bits in for 0/1 vchs */

                        case 'x':
                        case 'X':
                            rcv = RCV_X | (time_delta << 4);
                            break;
                        case 'z':
                        case 'Z':
                            rcv = RCV_Z | (time_delta << 4);
                            break;
                        case 'h':
                        case 'H':
                            rcv = RCV_H | (time_delta << 4);
                            break;
                        case 'u':
                        case 'U':
                            rcv = RCV_U | (time_delta << 4);
                            break;
                        case 'w':
                        case 'W':
                            rcv = RCV_W | (time_delta << 4);
                            break;
                        case 'l':
                        case 'L':
                            rcv = RCV_L | (time_delta << 4);
                            break;
                        default:
                            rcv = RCV_D | (time_delta << 4);
                            break;
                    }

                    vlist_emit_uv32(&n->mv.mvlfac_vlist, rcv);
                }
            } else {
                fprintf(stderr,
                        "Near byte %d, Malformed VCD identifier\n",
                        (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
                malform_eof_fix(self);
            }
            break;

            /* encode everything else literally as a time delta + a string */
#ifndef STRICT_VCD_ONLY
        case 's':
        case 'S':
            vector = g_alloca(self->yylen);
            vlen = fstUtilityEscToBin((unsigned char *)vector,
                                      (unsigned char *)(self->yytext + 1),
                                      self->yylen - 1);
            vector[vlen] = 0;

            get_strtoken(self);
            goto process_binary;
#endif
        case 'b':
        case 'B':
        case 'r':
        case 'R':
            vector = g_alloca(self->yylen);
            strcpy(vector, self->yytext + 1);
            vlen = self->yylen - 1;

            get_strtoken(self);
        process_binary:
            v = bsearch_vcd(self, self->yytext, self->yylen);
            if (!v) {
                fprintf(stderr,
                        "Near byte %d, Unknown VCD identifier: '%s'\n",
                        (int)(self->vcdbyteno + (self->vst - self->vcdbuf)),
                        self->yytext + 1);
                malform_eof_fix(self);
            } else {
                GwNode *n = v->narray[0];
                unsigned int time_delta;

                if (!n->mv.mvlfac_vlist) /* overloaded for vlist, numhist = last position used */
                {
                    unsigned char typ2 = toupper(typ);
                    n->mv.mvlfac_vlist = (GLOBALS->vlist_prepack)
                                             ? ((struct vlist_t *)vlist_packer_create())
                                             : vlist_create(sizeof(char));

                    if ((v->vartype != V_REAL) && (v->vartype != V_STRINGTYPE)) {
                        if ((typ2 == 'R') || (typ2 == 'S')) {
                            typ2 = 'B'; /* ok, typical case...fix as 'r' on bits variable causes
                                           recoder crash during trace extraction */
                        }
                    } else {
                        if (typ2 == 'B') {
                            typ2 = 'S'; /* should never be necessary...this is defensive */
                        }
                    }

                    vlist_emit_uv32(&n->mv.mvlfac_vlist,
                                    (unsigned int)toupper(typ2)); /* B/R/P/S for decompress */
                    vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
                    vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
                }

                time_delta = self->time_vlist_count - (unsigned int)n->numhist;
                n->numhist = self->time_vlist_count;

                vlist_emit_uv32(&n->mv.mvlfac_vlist, time_delta);

                if ((typ == 'b') || (typ == 'B')) {
                    if ((v->vartype != V_REAL) && (v->vartype != V_STRINGTYPE)) {
                        vlist_emit_mvl9_string(&n->mv.mvlfac_vlist, vector);
                    } else {
                        vlist_emit_string(&n->mv.mvlfac_vlist, vector);
                    }
                } else {
                    if ((v->vartype == V_REAL) || (v->vartype == V_STRINGTYPE) || (typ == 's') ||
                        (typ == 'S')) {
                        vlist_emit_string(&n->mv.mvlfac_vlist, vector);
                    } else {
                        char *bits = g_alloca(v->size + 1);
                        int i, j, k = 0;

                        memset(bits, 0x0, v->size + 1);

                        for (i = 0; i < vlen; i++) {
                            for (j = 0; j < 8; j++) {
                                bits[k++] = ((vector[i] >> (7 - j)) & 1) | '0';
                                if (k >= v->size)
                                    goto bit_term;
                            }
                        }

                    bit_term:
                        vlist_emit_mvl9_string(&n->mv.mvlfac_vlist, bits);
                    }
                }
            }
            break;

        case 'p':
        case 'P':
            /* extract port dump value.. */
            vector = g_alloca(self->yylen);
            evcd_strcpy(vector, self->yytext + 1); /* convert to regular vcd */
            vlen = self->yylen - 1;

            get_strtoken(self); /* throw away 0_strength_component */
            get_strtoken(self); /* throw away 0_strength_component */
            get_strtoken(self); /* this is the id                  */

            typ = 'b'; /* convert to regular vcd */
            goto process_binary; /* store string literally */

        default:
            break;
    }
}

static void evcd_strcpy(char *dst, char *src)
{
    static const char *evcd = "DUNZduLHXTlh01?FAaBbCcf";
    static const char *vcd = "01xz0101xz0101xzxxxxxxz";

    char ch;
    int i;

    while ((ch = *src)) {
        for (i = 0; i < 23; i++) {
            if (evcd[i] == ch) {
                *dst = vcd[i];
                break;
            }
        }
        if (i == 23)
            *dst = 'x';

        src++;
        dst++;
    }

    *dst = 0; /* null terminate destination */
}

static void vcd_parse(VcdLoader *self)
{
    int tok;
    unsigned char ttype;
    int disable_autocoalesce = 0;

    for (;;) {
        switch (get_token(self)) {
            case T_COMMENT:
                sync_end(self, "COMMENT:");
                break;
            case T_DATE:
                sync_end(self, "DATE:");
                break;
            case T_VERSION:
                disable_autocoalesce = version_sync_end(self, "VERSION:");
                break;
            case T_TIMEZERO: {
                int vtok = get_token(self);
                if ((vtok == T_END) || (vtok == T_EOF))
                    break;
                GLOBALS->global_time_offset = atoi_64(self->yytext);

                DEBUG(fprintf(stderr,
                              "TIMEZERO: %" GW_TIME_FORMAT "\n",
                              GLOBALS->global_time_offset));
                sync_end(self, NULL);
            } break;
            case T_TIMESCALE: {
                int vtok;
                int i;
                char prefix = ' ';

                vtok = get_token(self);
                if ((vtok == T_END) || (vtok == T_EOF))
                    break;
                fractional_timescale_fix(self->yytext);
                GLOBALS->time_scale = atoi_64(self->yytext);
                if (!GLOBALS->time_scale)
                    GLOBALS->time_scale = 1;
                for (i = 0; i < self->yylen; i++) {
                    if (self->yytext[i] < '0' || self->yytext[i] > '9') {
                        prefix = self->yytext[i];
                        break;
                    }
                }
                if (prefix == ' ') {
                    vtok = get_token(self);
                    if ((vtok == T_END) || (vtok == T_EOF))
                        break;
                    prefix = self->yytext[0];
                }
                switch (prefix) {
                    case ' ':
                    case 'm':
                    case 'u':
                    case 'n':
                    case 'p':
                    case 'f':
                    case 'a':
                    case 'z':
                        GLOBALS->time_dimension = prefix;
                        break;
                    case 's':
                        GLOBALS->time_dimension = ' ';
                        break;
                    default: /* unknown */
                        GLOBALS->time_dimension = 'n';
                        break;
                }

                DEBUG(fprintf(stderr,
                              "TIMESCALE: %" GW_TIME_FORMAT " %cs\n",
                              GLOBALS->time_scale,
                              GLOBALS->time_dimension));
                sync_end(self, NULL);
            } break;
            case T_SCOPE:
                T_GET;
                {
                    switch (self->yytext[0]) {
                        case 'm':
                            ttype = GW_TREE_KIND_VCD_ST_MODULE;
                            break;
                        case 't':
                            ttype = GW_TREE_KIND_VCD_ST_TASK;
                            break;
                        case 'f':
                            ttype = (self->yytext[1] == 'u') ? GW_TREE_KIND_VCD_ST_FUNCTION
                                                             : GW_TREE_KIND_VCD_ST_FORK;
                            break;
                        case 'b':
                            ttype = GW_TREE_KIND_VCD_ST_BEGIN;
                            break;
                        case 'g':
                            ttype = GW_TREE_KIND_VCD_ST_GENERATE;
                            break;
                        case 's':
                            ttype = GW_TREE_KIND_VCD_ST_STRUCT;
                            break;
                        case 'u':
                            ttype = GW_TREE_KIND_VCD_ST_UNION;
                            break;
                        case 'c':
                            ttype = GW_TREE_KIND_VCD_ST_CLASS;
                            break;
                        case 'i':
                            ttype = GW_TREE_KIND_VCD_ST_INTERFACE;
                            break;
                        case 'p':
                            ttype = (self->yytext[1] == 'r') ? GW_TREE_KIND_VCD_ST_PROGRAM
                                                             : GW_TREE_KIND_VCD_ST_PACKAGE;
                            break;

                        case 'v': {
                            char *vht = self->yytext;
                            if (!strncmp(vht, "vhdl_", 5)) {
                                switch (vht[5]) {
                                    case 'a':
                                        ttype = GW_TREE_KIND_VHDL_ST_ARCHITECTURE;
                                        break;
                                    case 'r':
                                        ttype = GW_TREE_KIND_VHDL_ST_RECORD;
                                        break;
                                    case 'b':
                                        ttype = GW_TREE_KIND_VHDL_ST_BLOCK;
                                        break;
                                    case 'g':
                                        ttype = GW_TREE_KIND_VHDL_ST_GENERATE;
                                        break;
                                    case 'i':
                                        ttype = GW_TREE_KIND_VHDL_ST_GENIF;
                                        break;
                                    case 'f':
                                        ttype = (vht[6] == 'u') ? GW_TREE_KIND_VHDL_ST_FUNCTION
                                                                : GW_TREE_KIND_VHDL_ST_GENFOR;
                                        break;
                                    case 'p':
                                        ttype = (!strncmp(vht + 6, "roces", 5))
                                                    ? GW_TREE_KIND_VHDL_ST_PROCESS
                                                    : GW_TREE_KIND_VHDL_ST_PROCEDURE;
                                        break;
                                    default:
                                        ttype = GW_TREE_KIND_UNKNOWN;
                                        break;
                                }
                            } else {
                                ttype = GW_TREE_KIND_UNKNOWN;
                            }
                        } break;

                        default:
                            ttype = GW_TREE_KIND_UNKNOWN;
                            break;
                    }
                }
                T_GET;
                if (tok != T_END && tok != T_EOF) {
                    struct slist *s;
                    s = (struct slist *)calloc_2(1, sizeof(struct slist));
                    s->len = self->yylen;
                    s->str = (char *)malloc_2(self->yylen + 1);
                    strcpy(s->str, self->yytext);
                    s->mod_tree_parent = GLOBALS->mod_tree_parent;

                    allocate_and_decorate_module_tree_node(ttype,
                                                           self->yytext,
                                                           NULL,
                                                           self->yylen,
                                                           0,
                                                           0,
                                                           0);

                    if (GLOBALS->slistcurr) {
                        GLOBALS->slistcurr->next = s;
                        GLOBALS->slistcurr = s;
                    } else {
                        GLOBALS->slistcurr = GLOBALS->slistroot = s;
                    }

                    build_slisthier();
                    DEBUG(fprintf(stderr, "SCOPE: %s\n", GLOBALS->slisthier));
                }
                sync_end(self, NULL);
                break;
            case T_UPSCOPE:
                if (GLOBALS->slistroot) {
                    struct slist *s;

                    GLOBALS->mod_tree_parent = GLOBALS->slistcurr->mod_tree_parent;
                    s = GLOBALS->slistroot;
                    if (!s->next) {
                        free_2(s->str);
                        free_2(s);
                        GLOBALS->slistroot = GLOBALS->slistcurr = NULL;
                    } else
                        for (;;) {
                            if (!s->next->next) {
                                free_2(s->next->str);
                                free_2(s->next);
                                s->next = NULL;
                                GLOBALS->slistcurr = s;
                                break;
                            }
                            s = s->next;
                        }
                    build_slisthier();
                    DEBUG(fprintf(stderr, "SCOPE: %s\n", GLOBALS->slisthier));
                } else {
                    GLOBALS->mod_tree_parent = NULL;
                }
                sync_end(self, NULL);
                break;
            case T_VAR:
                if ((self->header_over) && (0)) {
                    fprintf(
                        stderr,
                        "$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, "
                        "exiting.\n",
                        (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
                    vcd_exit(255);
                } else {
                    int vtok;
                    struct vcdsymbol *v = NULL;

                    self->var_prevch = 0;
                    if (self->varsplit) {
                        free_2(self->varsplit);
                        self->varsplit = NULL;
                    }
                    vtok = get_vartoken(self, 1);
                    if (vtok > V_STRINGTYPE)
                        goto bail;

                    v = (struct vcdsymbol *)calloc_2(1, sizeof(struct vcdsymbol));
                    v->vartype = vtok;
                    v->msi = v->lsi = -1;

                    if (vtok == V_PORT) {
                        vtok = get_vartoken(self, 1);
                        if (vtok == V_STRING) {
                            v->size = atoi_64(self->yytext);
                            if (!v->size)
                                v->size = 1;
                        } else if (vtok == V_LB) {
                            vtok = get_vartoken(self, 1);
                            if (vtok == V_END)
                                goto err;
                            if (vtok != V_STRING)
                                goto err;
                            v->msi = atoi_64(self->yytext);
                            vtok = get_vartoken(self, 0);
                            if (vtok == V_RB) {
                                v->lsi = v->msi;
                                v->size = 1;
                            } else {
                                if (vtok != V_COLON)
                                    goto err;
                                vtok = get_vartoken(self, 0);
                                if (vtok != V_STRING)
                                    goto err;
                                v->lsi = atoi_64(self->yytext);
                                vtok = get_vartoken(self, 0);
                                if (vtok != V_RB)
                                    goto err;

                                if (v->msi > v->lsi) {
                                    v->size = v->msi - v->lsi + 1;
                                } else {
                                    v->size = v->lsi - v->msi + 1;
                                }
                            }
                        } else
                            goto err;

                        vtok = get_strtoken(self);
                        if (vtok == V_END)
                            goto err;
                        v->id = (char *)malloc_2(self->yylen + 1);
                        strcpy(v->id, self->yytext);
                        v->nid = vcdid_hash(self->yytext, self->yylen);

                        if (v->nid == (self->vcd_hash_max + 1)) {
                            self->vcd_hash_max = v->nid;
                        } else if ((v->nid > 0) && (v->nid <= self->vcd_hash_max)) {
                            /* general case with aliases */
                        } else {
                            self->vcd_hash_kill = TRUE;
                        }

                        if (v->nid < self->vcd_minid) {
                            self->vcd_minid = v->nid;
                        }
                        if (v->nid > self->vcd_maxid) {
                            self->vcd_maxid = v->nid;
                        }

                        vtok = get_vartoken(self, 0);
                        if (vtok != V_STRING)
                            goto err;
                        if (GLOBALS->slisthier_len) {
                            v->name =
                                (char *)malloc_2(GLOBALS->slisthier_len + 1 + self->yylen + 1);
                            strcpy(v->name, GLOBALS->slisthier);
                            strcpy(v->name + GLOBALS->slisthier_len, GLOBALS->vcd_hier_delimeter);
                            if (GLOBALS->alt_hier_delimeter) {
                                strcpy_vcdalt(v->name + GLOBALS->slisthier_len + 1,
                                              self->yytext,
                                              GLOBALS->alt_hier_delimeter);
                            } else {
                                if ((strcpy_delimfix(v->name + GLOBALS->slisthier_len + 1,
                                                     self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd = (char *)malloc_2(GLOBALS->slisthier_len + 1 +
                                                                self->yylen + 2);
                                    strcpy(sd, GLOBALS->slisthier);
                                    strcpy(sd + GLOBALS->slisthier_len,
                                           GLOBALS->vcd_hier_delimeter);
                                    sd[GLOBALS->slisthier_len + 1] = '\\';
                                    strcpy(sd + GLOBALS->slisthier_len + 2,
                                           v->name + GLOBALS->slisthier_len + 1);
                                    free_2(v->name);
                                    v->name = sd;
                                }
                            }
                        } else {
                            v->name = (char *)malloc_2(self->yylen + 1);
                            if (GLOBALS->alt_hier_delimeter) {
                                strcpy_vcdalt(v->name, self->yytext, GLOBALS->alt_hier_delimeter);
                            } else {
                                if ((strcpy_delimfix(v->name, self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd = (char *)malloc_2(self->yylen + 2);
                                    sd[0] = '\\';
                                    strcpy(sd + 1, v->name);
                                    free_2(v->name);
                                    v->name = sd;
                                }
                            }
                        }

                        if (self->pv != NULL) {
                            if (!strcmp(GLOBALS->prev_hier_uncompressed_name, v->name) &&
                                !disable_autocoalesce && (!strchr(v->name, '\\'))) {
                                self->pv->chain = v;
                                v->root = self->rootv;
                                if (self->pv == self->rootv) {
                                    self->pv->root = self->rootv;
                                }
                            } else {
                                self->rootv = v;
                            }

                            free_2(GLOBALS->prev_hier_uncompressed_name);
                        } else {
                            self->rootv = v;
                        }

                        self->pv = v;
                        GLOBALS->prev_hier_uncompressed_name = strdup_2(v->name);
                    } else /* regular vcd var, not an evcd port var */
                    {
                        vtok = get_vartoken(self, 1);
                        if (vtok == V_END)
                            goto err;
                        v->size = atoi_64(self->yytext);
                        vtok = get_strtoken(self);
                        if (vtok == V_END)
                            goto err;
                        v->id = (char *)malloc_2(self->yylen + 1);
                        strcpy(v->id, self->yytext);
                        v->nid = vcdid_hash(self->yytext, self->yylen);

                        if (v->nid == (self->vcd_hash_max + 1)) {
                            self->vcd_hash_max = v->nid;
                        } else if ((v->nid > 0) && (v->nid <= self->vcd_hash_max)) {
                            /* general case with aliases */
                        } else {
                            self->vcd_hash_kill = 1;
                        }

                        if (v->nid < self->vcd_minid) {
                            self->vcd_minid = v->nid;
                        }
                        if (v->nid > self->vcd_maxid) {
                            self->vcd_maxid = v->nid;
                        }

                        vtok = get_vartoken(self, 0);
                        if (vtok != V_STRING)
                            goto err;

                        if (GLOBALS->slisthier_len) {
                            v->name =
                                (char *)malloc_2(GLOBALS->slisthier_len + 1 + self->yylen + 1);
                            strcpy(v->name, GLOBALS->slisthier);
                            strcpy(v->name + GLOBALS->slisthier_len, GLOBALS->vcd_hier_delimeter);
                            if (GLOBALS->alt_hier_delimeter) {
                                strcpy_vcdalt(v->name + GLOBALS->slisthier_len + 1,
                                              self->yytext,
                                              GLOBALS->alt_hier_delimeter);
                            } else {
                                if ((strcpy_delimfix(v->name + GLOBALS->slisthier_len + 1,
                                                     self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd = (char *)malloc_2(GLOBALS->slisthier_len + 1 +
                                                                self->yylen + 2);
                                    strcpy(sd, GLOBALS->slisthier);
                                    strcpy(sd + GLOBALS->slisthier_len,
                                           GLOBALS->vcd_hier_delimeter);
                                    sd[GLOBALS->slisthier_len + 1] = '\\';
                                    strcpy(sd + GLOBALS->slisthier_len + 2,
                                           v->name + GLOBALS->slisthier_len + 1);
                                    free_2(v->name);
                                    v->name = sd;
                                }
                            }
                        } else {
                            v->name = (char *)malloc_2(self->yylen + 1);
                            if (GLOBALS->alt_hier_delimeter) {
                                strcpy_vcdalt(v->name, self->yytext, GLOBALS->alt_hier_delimeter);
                            } else {
                                if ((strcpy_delimfix(v->name, self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd = (char *)malloc_2(self->yylen + 2);
                                    sd[0] = '\\';
                                    strcpy(sd + 1, v->name);
                                    free_2(v->name);
                                    v->name = sd;
                                }
                            }
                        }

                        if (self->pv != NULL) {
                            if (!strcmp(GLOBALS->prev_hier_uncompressed_name, v->name)) {
                                self->pv->chain = v;
                                v->root = self->rootv;
                                if (self->pv == self->rootv) {
                                    self->pv->root = self->rootv;
                                }
                            } else {
                                self->rootv = v;
                            }

                            free_2(GLOBALS->prev_hier_uncompressed_name);
                        } else {
                            self->rootv = v;
                        }
                        self->pv = v;
                        GLOBALS->prev_hier_uncompressed_name = strdup_2(v->name);

                        vtok = get_vartoken(self, 1);
                        if (vtok == V_END)
                            goto dumpv;
                        if (vtok != V_LB)
                            goto err;
                        vtok = get_vartoken(self, 0);
                        if (vtok != V_STRING)
                            goto err;
                        v->msi = atoi_64(self->yytext);
                        vtok = get_vartoken(self, 0);
                        if (vtok == V_RB) {
                            v->lsi = v->msi;
                            goto dumpv;
                        }
                        if (vtok != V_COLON)
                            goto err;
                        vtok = get_vartoken(self, 0);
                        if (vtok != V_STRING)
                            goto err;
                        v->lsi = atoi_64(self->yytext);
                        vtok = get_vartoken(self, 0);
                        if (vtok != V_RB)
                            goto err;
                    }

                dumpv:
                    if (v->size == 0) {
                        if (v->vartype != V_EVENT) {
                            if (v->vartype != V_STRINGTYPE) {
                                v->vartype = V_REAL;
                            }
                        } else {
                            v->size = 1;
                        }

                    } /* MTI fix */

                    if ((v->vartype == V_REAL) || (v->vartype == V_STRINGTYPE)) {
                        v->size = 1; /* override any data we parsed in */
                        v->msi = v->lsi = 0;
                    } else if ((v->size > 1) && (v->msi <= 0) && (v->lsi <= 0)) {
                        if (v->vartype == V_EVENT) {
                            v->size = 1;
                        } else {
                            /* any criteria for the direction here? */
                            v->msi = v->size - 1;
                            v->lsi = 0;
                        }
                    } else if ((v->msi > v->lsi) && ((v->msi - v->lsi + 1) != v->size)) {
                        if ((v->vartype != V_EVENT) && (v->vartype != V_PARAMETER)) {
                            if ((v->msi - v->lsi + 1) > v->size) /* if() is 2d add */
                            {
                                v->msi = v->size - 1;
                                v->lsi = 0;
                            }
                            /* all this formerly was goto err; */
                        } else {
                            v->size = v->msi - v->lsi + 1;
                        }
                    } else if ((v->lsi >= v->msi) && ((v->lsi - v->msi + 1) != v->size)) {
                        if ((v->vartype != V_EVENT) && (v->vartype != V_PARAMETER)) {
                            if ((v->lsi - v->msi + 1) > v->size) /* if() is 2d add */
                            {
                                v->lsi = v->size - 1;
                                v->msi = 0;
                            }
                            /* all this formerly was goto err; */
                        } else {
                            v->size = v->lsi - v->msi + 1;
                        }
                    }

                    /* initial conditions */
                    v->narray = calloc_2(1, sizeof(GwNode *));
                    v->narray[0] = calloc_2(1, sizeof(GwNode));
                    v->narray[0]->head.time = -1;
                    v->narray[0]->head.v.h_val = AN_X;

                    if (self->vcdsymroot == NULL) {
                        self->vcdsymroot = self->vcdsymcurr = v;
                    } else {
                        self->vcdsymcurr->next = v;
                    }
                    self->vcdsymcurr = v;
                    self->numsyms++;

                    if (GLOBALS->vcd_save_handle) {
                        if (v->msi == v->lsi) {
                            if ((v->vartype == V_REAL) || (v->vartype == V_STRINGTYPE)) {
                                fprintf(GLOBALS->vcd_save_handle, "%s\n", v->name);
                            } else {
                                if (v->msi >= 0) {
                                    fprintf(GLOBALS->vcd_save_handle, "%s[%d]\n", v->name, v->msi);
                                } else {
                                    fprintf(GLOBALS->vcd_save_handle, "%s\n", v->name);
                                }
                            }
                        } else {
                            fprintf(GLOBALS->vcd_save_handle,
                                    "%s[%d:%d]\n",
                                    v->name,
                                    v->msi,
                                    v->lsi);
                        }
                    }

                    goto bail;
                err:
                    if (v) {
                        self->error_count++;
                        if (v->name) {
                            fprintf(stderr,
                                    "Near byte %d, $VAR parse error encountered with '%s'\n",
                                    (int)(self->vcdbyteno + (self->vst - self->vcdbuf)),
                                    v->name);
                            free_2(v->name);
                        } else {
                            fprintf(stderr,
                                    "Near byte %d, $VAR parse error encountered\n",
                                    (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
                        }
                        if (v->id)
                            free_2(v->id);
                        free_2(v);
                        v = NULL;
                        self->pv = NULL;
                    }

                bail:
                    if (vtok != V_END)
                        sync_end(self, NULL);
                    break;
                }
            case T_ENDDEFINITIONS:
                self->header_over = TRUE; /* do symbol table management here */
                create_sorted_table(self);
                if (self->symbols_sorted == NULL && self->symbols_indexed == NULL) {
                    fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
                    vcd_exit(255);
                }
                if (self->error_count > 0) {
                    fprintf(stderr,
                            "\n%d VCD parse errors encountered, exiting.\n",
                            self->error_count);
                    vcd_exit(255);
                }

                break;
            case T_STRING:
                if (!self->header_over) {
                    self->header_over = TRUE; /* do symbol table management here */
                    create_sorted_table(self);
                    if (self->symbols_sorted == NULL && self->symbols_indexed == NULL) {
                        break;
                    }
                }
                {
                    /* catchall for events when header over */
                    if (self->yytext[0] == '#') {
                        GwTime tim;
                        GwTime *tt;

                        tim = atoi_64(self->yytext + 1);

                        if (self->file->start_time < 0) {
                            self->file->start_time = tim;
                        } else {
                            /* backtracking fix */
                            if (tim < self->current_time) {
                                if (!self->already_backtracked) {
                                    self->already_backtracked = TRUE;
                                    fprintf(stderr,
                                            "VCDLOAD | Time backtracking detected in VCD file!\n");
                                }
                            }
#if 0
						if(tim < GLOBALS->current_time_vcd_recoder_c_3) /* avoid backtracking time counts which can happen on malformed files */
							{
							tim = GLOBALS->current_time_vcd_recoder_c_3;
							}
#endif
                        }

                        self->current_time = tim;
                        if (self->file->end_time < tim)
                            self->file->end_time = tim; /* in case of malformed vcd files */
                        DEBUG(fprintf(stderr, "#%" GW_TIME_FORMAT "\n", tim));

                        tt = vlist_alloc(&self->file->time_vlist, 0);
                        *tt = tim;
                        self->time_vlist_count++;
                    } else {
                        if (self->time_vlist_count) {
                            /* OK, otherwise fix for System C which doesn't emit time zero... */
                        } else {
                            GwTime tim = GW_TIME_CONSTANT(0);
                            GwTime *tt;

                            self->file->start_time = self->current_time = self->file->end_time =
                                tim;

                            tt = vlist_alloc(&self->file->time_vlist, 0);
                            *tt = tim;
                            self->time_vlist_count = 1;
                        }
                        parse_valuechange(self);
                    }
                }
                break;
            case T_DUMPALL: /* dump commands modify vals anyway so */
            case T_DUMPPORTSALL:
                break; /* just loop through..                 */
            case T_DUMPOFF:
            case T_DUMPPORTSOFF:
                gw_blackout_regions_add_dumpoff(GLOBALS->blackout_regions, self->current_time);
                break;
            case T_DUMPON:
            case T_DUMPPORTSON:
                gw_blackout_regions_add_dumpon(GLOBALS->blackout_regions, self->current_time);
                break;
            case T_DUMPVARS:
            case T_DUMPPORTS:
                if (self->current_time < 0) {
                    self->file->start_time = self->current_time = self->file->end_time = 0;
                }
                break;
            case T_VCDCLOSE:
                sync_end(self, "VCDCLOSE:");
                break; /* next token will be '#' time related followed by $end */
            case T_END: /* either closure for dump commands or */
                break; /* it's spurious                       */
            case T_UNKNOWN_KEY:
                sync_end(self, NULL); /* skip over unknown keywords */
                break;
            case T_EOF:
                gw_blackout_regions_add_dumpon(GLOBALS->blackout_regions, self->current_time);

                self->pv = NULL;
                if (GLOBALS->prev_hier_uncompressed_name) {
                    free_2(GLOBALS->prev_hier_uncompressed_name);
                    GLOBALS->prev_hier_uncompressed_name = NULL;
                }

                return;
            default:
                DEBUG(fprintf(stderr, "UNKNOWN TOKEN\n"));
        }
    }
}

/*******************************************************************************/

void add_histent(VcdFile *self, GwTime tim, GwNode *n, char ch, int regadd, char *vector)
{
    GwHistEnt *he;
    char heval;

    if (!vector) {
        if (!n->curr) {
            he = histent_calloc();
            he->time = -1;
            he->v.h_val = AN_X;

            n->curr = he;
            n->head.next = he;

            add_histent(self, tim, n, ch, regadd, vector);
        } else {
            if (regadd) {
                tim *= (GLOBALS->time_scale);
            }

            if (ch == '0')
                heval = AN_0;
            else if (ch == '1')
                heval = AN_1;
            else if ((ch == 'x') || (ch == 'X'))
                heval = AN_X;
            else if ((ch == 'z') || (ch == 'Z'))
                heval = AN_Z;
            else if ((ch == 'h') || (ch == 'H'))
                heval = AN_H;
            else if ((ch == 'u') || (ch == 'U'))
                heval = AN_U;
            else if ((ch == 'w') || (ch == 'W'))
                heval = AN_W;
            else if ((ch == 'l') || (ch == 'L'))
                heval = AN_L;
            else
                /* if(ch=='-') */ heval = AN_DASH; /* default */

            if ((n->curr->v.h_val != heval) || (tim == self->start_time) ||
                (n->vartype == GW_VAR_TYPE_VCD_EVENT) ||
                (GLOBALS->vcd_preserve_glitches)) /* same region == go skip */
            {
                if (n->curr->time == tim) {
                    DEBUG(printf("Warning: Glitch at time [%" GW_TIME_FORMAT
                                 "] Signal [%p], Value [%c->%c].\n",
                                 tim,
                                 n,
                                 AN_STR[n->curr->v.h_val],
                                 ch));
                    n->curr->v.h_val = heval; /* we have a glitch! */

                    if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                        n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                    }
                } else {
                    he = histent_calloc();
                    he->time = tim;
                    he->v.h_val = heval;

                    n->curr->next = he;
                    if (n->curr->v.h_val == heval) {
                        n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                    }
                    n->curr = he;
                    GLOBALS->regions += regadd;
                }
            }
        }
    } else {
        switch (ch) {
            case 's': /* string */
            {
                if (!n->curr) {
                    he = histent_calloc();
                    he->flags = (GW_HIST_ENT_FLAG_STRING | GW_HIST_ENT_FLAG_REAL);
                    he->time = -1;
                    he->v.h_vector = NULL;

                    n->curr = he;
                    n->head.next = he;

                    add_histent(self, tim, n, ch, regadd, vector);
                } else {
                    if (regadd) {
                        tim *= (GLOBALS->time_scale);
                    }

                    if (n->curr->time == tim) {
                        DEBUG(printf("Warning: String Glitch at time [%" GW_TIME_FORMAT
                                     "] Signal [%p].\n",
                                     tim,
                                     n));
                        if (n->curr->v.h_vector)
                            free_2(n->curr->v.h_vector);
                        n->curr->v.h_vector = vector; /* we have a glitch! */

                        if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                            n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                        }
                    } else {
                        he = histent_calloc();
                        he->flags = (GW_HIST_ENT_FLAG_STRING | GW_HIST_ENT_FLAG_REAL);
                        he->time = tim;
                        he->v.h_vector = vector;

                        n->curr->next = he;
                        n->curr = he;
                        GLOBALS->regions += regadd;
                    }
                }
                break;
            }
            case 'g': /* real number */
            {
                if (!n->curr) {
                    he = histent_calloc();
                    he->flags = GW_HIST_ENT_FLAG_REAL;
                    he->time = -1;
                    he->v.h_double = strtod("NaN", NULL);

                    n->curr = he;
                    n->head.next = he;

                    add_histent(self, tim, n, ch, regadd, vector);
                } else {
                    if (regadd) {
                        tim *= (GLOBALS->time_scale);
                    }

                    if ((vector && (n->curr->v.h_double != *(double *)vector)) ||
                        (tim == self->start_time) || (GLOBALS->vcd_preserve_glitches) ||
                        (GLOBALS->vcd_preserve_glitches_real)) /* same region == go skip */
                    {
                        if (n->curr->time == tim) {
                            DEBUG(printf("Warning: Real number Glitch at time [%" GW_TIME_FORMAT
                                         "] Signal [%p].\n",
                                         tim,
                                         n));
                            n->curr->v.h_double = *((double *)vector);
                            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                            }
                        } else {
                            he = histent_calloc();
                            he->flags = GW_HIST_ENT_FLAG_REAL;
                            he->time = tim;
                            he->v.h_double = *((double *)vector);
                            n->curr->next = he;
                            n->curr = he;
                            GLOBALS->regions += regadd;
                        }
                    } else {
                    }
                    free_2(vector);
                }
                break;
            }
            default: {
                if (!n->curr) {
                    he = histent_calloc();
                    he->time = -1;
                    he->v.h_vector = NULL;

                    n->curr = he;
                    n->head.next = he;

                    add_histent(self, tim, n, ch, regadd, vector);
                } else {
                    if (regadd) {
                        tim *= (GLOBALS->time_scale);
                    }

                    if ((n->curr->v.h_vector && vector && (strcmp(n->curr->v.h_vector, vector))) ||
                        (tim == self->start_time) || (!n->curr->v.h_vector) ||
                        (GLOBALS->vcd_preserve_glitches)) /* same region == go skip */
                    {
                        if (n->curr->time == tim) {
                            DEBUG(printf("Warning: Glitch at time [%" GW_TIME_FORMAT
                                         "] Signal [%p], Value [%c->%c].\n",
                                         tim,
                                         n,
                                         AN_STR[n->curr->v.h_val],
                                         ch));
                            if (n->curr->v.h_vector)
                                free_2(n->curr->v.h_vector);
                            n->curr->v.h_vector = vector; /* we have a glitch! */

                            if (!(n->curr->flags & GW_HIST_ENT_FLAG_GLITCH)) {
                                n->curr->flags |= GW_HIST_ENT_FLAG_GLITCH; /* set the glitch flag */
                            }
                        } else {
                            he = histent_calloc();
                            he->time = tim;
                            he->v.h_vector = vector;

                            n->curr->next = he;
                            n->curr = he;
                            GLOBALS->regions += regadd;
                        }
                    } else {
                        free_2(vector);
                    }
                }
                break;
            }
        }
    }
}

/*******************************************************************************/

static void vcd_build_symbols(VcdLoader *self)
{
    int j;
    int max_slen = -1;
    GSList *sym_chain = NULL;
    int duphier = 0;
    char hashdirty;
    struct vcdsymbol *v, *vprime;
    char *str = g_alloca(1); /* quiet scan-build null pointer warning below */
#ifdef _WAVE_HAVE_JUDY
    int ss_len, longest = 0;
#endif

    v = self->vcdsymroot;
    while (v) {
        int msi;
        int delta;

        {
            int slen;
            int substnode;

            msi = v->msi;
            delta = ((v->lsi - v->msi) < 0) ? -1 : 1;
            substnode = 0;

            slen = strlen(v->name);
            str = (slen > max_slen) ? (g_alloca((max_slen = slen) + 32))
                                    : (str); /* more than enough */
            strcpy(str, v->name);

            if ((v->msi >= 0) || (v->msi != v->lsi)) {
                strcpy(str + slen, GLOBALS->vcd_hier_delimeter);
                slen++;
            }

            if ((vprime = bsearch_vcd(self, v->id, strlen(v->id))) !=
                v) /* hash mish means dup net */
            {
                if (v->size != vprime->size) {
                    fprintf(stderr,
                            "ERROR: Duplicate IDs with differing width: %s %s\n",
                            v->name,
                            vprime->name);
                } else {
                    substnode = 1;
                }
            }

            if ((v->size == 1) && (v->vartype != V_REAL) && (v->vartype != V_STRINGTYPE)) {
                struct symbol *s = NULL;

                for (j = 0; j < v->size; j++) {
                    if (v->msi >= 0) {
                        sprintf(str + slen - 1, "[%d]", msi);
                    }

                    hashdirty = 0;
                    if (symfind(str, NULL)) {
                        char *dupfix = (char *)malloc_2(max_slen + 32);
#ifndef _WAVE_HAVE_JUDY
                        hashdirty = 1;
#endif
                        DEBUG(fprintf(stderr, "Warning: %s is a duplicate net name.\n", str));

                        do
                            sprintf(dupfix,
                                    "$DUP%d%s%s",
                                    duphier++,
                                    GLOBALS->vcd_hier_delimeter,
                                    str);
                        while (symfind(dupfix, NULL));

                        strcpy(str, dupfix);
                        free_2(dupfix);
                        duphier = 0; /* reset for next duplicate resolution */
                    }
                    /* fallthrough */
                    {
                        s = symadd(str, hashdirty ? hash(str) : GLOBALS->hashcache);
#ifdef _WAVE_HAVE_JUDY
                        ss_len = strlen(str);
                        if (ss_len >= longest) {
                            longest = ss_len + 1;
                        }
#endif
                        s->n = v->narray[j];
                        if (substnode) {
                            GwNode *n;
                            GwNode *n2;

                            n = s->n;
                            n2 = vprime->narray[j];
                            /* nname stays same */
                            /* n->head=n2->head; */
                            /* n->curr=n2->curr; */
                            n->curr = (GwHistEnt *)n2;
                            /* harray calculated later */
                            n->numhist = n2->numhist;
                        }

#ifndef _WAVE_HAVE_JUDY
                        s->n->nname = s->name;
#endif
                        self->sym_chain = g_slist_prepend(self->sym_chain, s);

                        GLOBALS->numfacs++;
                        DEBUG(fprintf(stderr, "Added: %s\n", str));
                    }
                    msi += delta;
                }

                if ((j == 1) && (v->root)) {
                    s->vec_root = (struct symbol *)v->root; /* these will get patched over */
                    s->vec_chain = (struct symbol *)v->chain; /* these will get patched over */
                    v->sym_chain = s;

                    sym_chain = g_slist_prepend(sym_chain, s);
                }
            } else /* atomic vector */
            {
                if ((v->vartype != V_REAL) && (v->vartype != V_STRINGTYPE) &&
                    (v->vartype != V_INTEGER) && (v->vartype != V_PARAMETER))
                /* if((v->vartype!=V_REAL)&&(v->vartype!=V_STRINGTYPE)) */
                {
                    sprintf(str + slen - 1, "[%d:%d]", v->msi, v->lsi);
                    /* 2d add */
                    if ((v->msi > v->lsi) && ((v->msi - v->lsi + 1) != v->size)) {
                        if ((v->vartype != V_EVENT) && (v->vartype != V_PARAMETER)) {
                            v->msi = v->size - 1;
                            v->lsi = 0;
                        }
                    } else if ((v->lsi >= v->msi) && ((v->lsi - v->msi + 1) != v->size)) {
                        if ((v->vartype != V_EVENT) && (v->vartype != V_PARAMETER)) {
                            v->lsi = v->size - 1;
                            v->msi = 0;
                        }
                    }
                } else {
                    *(str + slen - 1) = 0;
                }

                hashdirty = 0;
                if (symfind(str, NULL)) {
                    char *dupfix = (char *)malloc_2(max_slen + 32);
#ifndef _WAVE_HAVE_JUDY
                    hashdirty = 1;
#endif
                    DEBUG(fprintf(stderr, "Warning: %s is a duplicate net name.\n", str));

                    do
                        sprintf(dupfix, "$DUP%d%s%s", duphier++, GLOBALS->vcd_hier_delimeter, str);
                    while (symfind(dupfix, NULL));

                    strcpy(str, dupfix);
                    free_2(dupfix);
                    duphier = 0; /* reset for next duplicate resolution */
                }
                /* fallthrough */
                {
                    struct symbol *s;

                    s = symadd(str,
                               hashdirty ? hash(str)
                                         : GLOBALS->hashcache); /* cut down on double lookups.. */
#ifdef _WAVE_HAVE_JUDY
                    ss_len = strlen(str);
                    if (ss_len >= longest) {
                        longest = ss_len + 1;
                    }
#endif
                    s->n = v->narray[0];
                    if (substnode) {
                        GwNode *n;
                        GwNode *n2;

                        n = s->n;
                        n2 = vprime->narray[0];
                        /* nname stays same */
                        /* n->head=n2->head; */
                        /* n->curr=n2->curr; */
                        n->curr = (GwHistEnt *)n2;
                        /* harray calculated later */
                        n->numhist = n2->numhist;
                        n->extvals = n2->extvals;
                        n->msi = n2->msi;
                        n->lsi = n2->lsi;
                    } else {
                        s->n->msi = v->msi;
                        s->n->lsi = v->lsi;

                        s->n->extvals = 1;
                    }

#ifndef _WAVE_HAVE_JUDY
                    s->n->nname = s->name;
#endif
                    self->sym_chain = g_slist_prepend(self->sym_chain, s);

                    GLOBALS->numfacs++;
                    DEBUG(fprintf(stderr, "Added: %s\n", str));
                }
            }
        }

        v = v->next;
    }

#ifdef _WAVE_HAVE_JUDY
    {
        Pvoid_t PJArray = GLOBALS->sym_judy;
        PPvoid_t PPValue;
        char *Index = calloc_2(1, longest);

        for (PPValue = JudySLFirst(PJArray, (uint8_t *)Index, PJE0); PPValue != (PPvoid_t)NULL;
             PPValue = JudySLNext(PJArray, (uint8_t *)Index, PJE0)) {
            struct symbol *s = *(struct symbol **)PPValue;
            s->name = strdup_2(Index);
            s->n->nname = s->name;
        }

        free_2(Index);
    }
#endif

    if (sym_chain != NULL) {
        for (GSList *iter = sym_chain; iter != NULL; iter = iter->next) {
            struct symbol *s = iter->data;

            s->vec_root = ((struct vcdsymbol *)s->vec_root)->sym_chain;
            if ((struct vcdsymbol *)s->vec_chain != NULL) {
                s->vec_chain = ((struct vcdsymbol *)s->vec_chain)->sym_chain;
            }

            DEBUG(printf("Link: ('%s') '%s' -> '%s'\n",
                         sym_curr->val->vec_root->name,
                         sym_curr->val->name,
                         sym_curr->val->vec_chain ? sym_curr->val->vec_chain->name : "(END)"));
        }

        g_slist_free(sym_chain);
    }
}

/*******************************************************************************/

static void vcd_cleanup(VcdLoader *self)
{
    struct slist *s, *s2;
    struct vcdsymbol *v, *vt;

    g_clear_pointer(&self->symbols_indexed, free_2);
    g_clear_pointer(&self->symbols_sorted, free_2);

    v = self->vcdsymroot;
    while (v) {
        if (v->name)
            free_2(v->name);
        if (v->id)
            free_2(v->id);
        if (v->narray)
            free_2(v->narray);
        vt = v;
        v = v->next;
        free_2(vt);
    }
    self->vcdsymroot = NULL;
    self->vcdsymcurr = NULL;

    if (GLOBALS->slisthier) {
        free_2(GLOBALS->slisthier);
        GLOBALS->slisthier = NULL;
    }
    s = GLOBALS->slistroot;
    while (s) {
        s2 = s->next;
        if (s->str)
            free_2(s->str);
        free_2(s);
        s = s2;
    }

    GLOBALS->slistroot = GLOBALS->slistcurr = NULL;
    GLOBALS->slisthier_len = 0;

    if (self->is_compressed) {
        pclose(self->vcd_handle);
    } else {
        fclose(self->vcd_handle);
    }
    self->vcd_handle = NULL;

    g_clear_pointer(&self->yytext, free_2);
}

/*******************************************************************************/

GwTime vcd_recoder_main(char *fname)
{
    GLOBALS->vcd_hier_delimeter[0] = GLOBALS->hier_delimeter;

    GLOBALS->blackout_regions = gw_blackout_regions_new();

    errno = 0; /* reset in case it's set for some reason */

    if (!GLOBALS->hier_was_explicitly_set) /* set default hierarchy split char */
    {
        GLOBALS->hier_delimeter = '.';
    }

    VcdLoader loader = {0};
    VcdLoader *self = &loader;
    self->current_time = -1;
    self->file = g_new0(VcdFile, 1);
    self->file->start_time = -1;
    self->file->end_time = -1;

    self->T_MAX_STR = 1024;
    self->yytext = malloc_2(self->T_MAX_STR + 1);
    self->vcd_minid = G_MAXUINT;

    if (suffix_check(fname, ".gz") || suffix_check(fname, ".zip")) {
        char *str;
        int dlen;
        dlen = strlen(WAVE_DECOMPRESSOR);
        str = g_alloca(strlen(fname) + dlen + 1);
        strcpy(str, WAVE_DECOMPRESSOR);
        strcpy(str + dlen, fname);
        self->vcd_handle = popen(str, "r");
        self->is_compressed = ~0;
    } else {
        if (strcmp("-vcd", fname)) {
            self->vcd_handle = fopen(fname, "rb");

            if (self->vcd_handle) {
                fseeko(self->vcd_handle, 0, SEEK_END); /* do status bar for vcd load */
                self->vcd_fsiz = ftello(self->vcd_handle);
                fseeko(self->vcd_handle, 0, SEEK_SET);
            }

            if (GLOBALS->vcd_warning_filesize < 0)
                GLOBALS->vcd_warning_filesize = VCD_SIZE_WARN;

            if (GLOBALS->vcd_warning_filesize)
                if (self->vcd_fsiz > (GLOBALS->vcd_warning_filesize * (1024 * 1024))) {
                    if (!GLOBALS->vlist_prepack) {
                        fprintf(
                            stderr,
                            "Warning! File size is %d MB.  This might fail in recoding.\n"
                            "Consider converting it to the FST database format instead.  (See the\n"
                            "vcd2fst(1) manpage for more information.)\n"
                            "To disable this warning, set rc variable vcd_warning_filesize to "
                            "zero.\n"
                            "Alternatively, use the -o, --optimize command line option to convert "
                            "to FST\n"
                            "or the -g, --giga command line option to use dynamically compressed "
                            "memory.\n\n",
                            (int)(self->vcd_fsiz / (1024 * 1024)));
                    } else {
                        fprintf(stderr,
                                "VCDLOAD | File size is %d MB, using vlist prepacking.\n\n",
                                (int)(self->vcd_fsiz / (1024 * 1024)));
                    }
                }
        } else {
            GLOBALS->splash_disable = 1;
            self->vcd_handle = stdin;
        }
        self->is_compressed = 0;
    }

    if (self->vcd_handle == NULL) {
        fprintf(stderr,
                "Error opening %s .vcd file '%s'.\n",
                self->is_compressed ? "compressed" : "",
                fname);
        perror("Why");
        vcd_exit(255);
    }

    /* SPLASH */ splash_create();

    sym_hash_initialize(GLOBALS);
    getch_alloc(self); /* alloc membuff for vcd getch buffer */

    build_slisthier();

    self->file->time_vlist = vlist_create(sizeof(GwTime));

    vcd_parse(self);
    if (self->varsplit) {
        free_2(self->varsplit);
        self->varsplit = NULL;
    }

    vlist_freeze(&self->file->time_vlist);

    vlist_emit_finalize(self);

    if (self->symbols_sorted == NULL && self->symbols_indexed == NULL) {
        fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
        vcd_exit(255);
    }

    if (GLOBALS->vcd_save_handle) {
        fclose(GLOBALS->vcd_save_handle);
        GLOBALS->vcd_save_handle = NULL;
    }

    fprintf(stderr,
            "[%" GW_TIME_FORMAT "] start time.\n[%" GW_TIME_FORMAT "] end time.\n",
            self->file->start_time * GLOBALS->time_scale,
            self->file->end_time * GLOBALS->time_scale);

    if (self->vcd_fsiz > 0) {
        splash_sync(self->vcd_fsiz, self->vcd_fsiz);
    } else if (self->is_compressed) {
        splash_sync(1, 1);
    }
    self->vcd_fsiz = 0;

    GLOBALS->min_time = self->file->start_time * GLOBALS->time_scale;
    GLOBALS->max_time = self->file->end_time * GLOBALS->time_scale;
    GLOBALS->global_time_offset = GLOBALS->global_time_offset * GLOBALS->time_scale;

    if ((GLOBALS->min_time == GLOBALS->max_time) && (GLOBALS->max_time == GW_TIME_CONSTANT(-1))) {
        fprintf(stderr, "VCD times range is equal to zero.  Exiting.\n");
        vcd_exit(255);
    }

    vcd_build_symbols(self);
    vcd_sortfacs(self->sym_chain);
    vcd_cleanup(self);

    getch_free(self); /* free membuff for vcd getch buffer */

    gw_blackout_regions_scale(GLOBALS->blackout_regions, GLOBALS->time_scale);

    /* is_vcd=~0; */
    GLOBALS->is_lx2 = LXT2_IS_VLIST;
    GLOBALS->vcd_file = self->file;

    /* SPLASH */ splash_finalize();
    return (GLOBALS->max_time);
}

/*******************************************************************************/

void vcd_import_masked(VcdFile *self)
{
    /* nothing */
    (void)self;
}

void vcd_set_fac_process_mask(VcdFile *self, GwNode *np)
{
    if (np && np->mv.mvlfac_vlist) {
        import_vcd_trace(self, np);
    }
}

#define vlist_locate_import(x, y) \
    ((GLOBALS->vlist_prepack) ? ((depacked) + (y)) : vlist_locate((x), (y)))

void import_vcd_trace(VcdFile *self, GwNode *np)
{
    struct vlist_t *v = np->mv.mvlfac_vlist;
    int len = 1;
    unsigned int list_size;
    unsigned char vlist_type;
    /* unsigned int vartype = 0; */ /* scan-build */
    unsigned int vlist_pos = 0;
    unsigned char *chp;
    unsigned int time_idx = 0;
    GwTime *curtime_pnt;
    unsigned char arr[5];
    int arr_pos;
    unsigned int accum;
    unsigned char ch;
    double *d;
    unsigned char *depacked = NULL;

    if (!v)
        return;
    vlist_uncompress(&v);

    if (GLOBALS->vlist_prepack) {
        depacked = vlist_packer_decompress(v, &list_size);
        vlist_destroy(v);
    } else {
        list_size = vlist_size(v);
    }

    if (!list_size) {
        len = 1;
        vlist_type = '!'; /* possible alias */
    } else {
        chp = vlist_locate_import(v, vlist_pos++);
        if (chp) {
            switch ((vlist_type = (*chp & 0x7f))) {
                case '0':
                    len = 1;
                    chp = vlist_locate_import(v, vlist_pos++);
                    if (!chp) {
                        fprintf(stderr,
                                "Internal error file '%s' line %d, exiting.\n",
                                __FILE__,
                                __LINE__);
                        exit(255);
                    }
                    /* vartype = (unsigned int)(*chp & 0x7f); */ /*scan-build */
                    break;

                case 'B':
                case 'R':
                case 'S':
                    chp = vlist_locate_import(v, vlist_pos++);
                    if (!chp) {
                        fprintf(stderr,
                                "Internal error file '%s' line %d, exiting.\n",
                                __FILE__,
                                __LINE__);
                        exit(255);
                    }
                    /* vartype = (unsigned int)(*chp & 0x7f); */ /* scan-build */

                    arr_pos = accum = 0;

                    do {
                        chp = vlist_locate_import(v, vlist_pos++);
                        if (!chp)
                            break;
                        ch = *chp;
                        arr[arr_pos++] = ch;
                    } while (!(ch & 0x80));

                    for (--arr_pos; arr_pos >= 0; arr_pos--) {
                        ch = arr[arr_pos];
                        accum <<= 7;
                        accum |= (unsigned int)(ch & 0x7f);
                    }

                    len = accum;

                    break;

                default:
                    fprintf(stderr, "Unsupported vlist type '%c', exiting.", vlist_type);
                    vcd_exit(255);
                    break;
            }
        } else {
            len = 1;
            vlist_type = '!'; /* possible alias */
        }
    }

    if (vlist_type == '0') /* single bit */
    {
        while (vlist_pos < list_size) {
            unsigned int delta, bitval;
            char ascval;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            if (!(accum & 1)) {
                delta = accum >> 2;
                bitval = (accum >> 1) & 1;
                ascval = '0' + bitval;
            } else {
                delta = accum >> 4;
                bitval = (accum >> 1) & 7;
                ascval = RCV_STR[bitval];
            }
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed bitwise signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            add_histent(self, *curtime_pnt, np, ascval, 1, NULL);
        }

        add_histent(self, MAX_HISTENT_TIME - 1, np, 'x', 0, NULL);
        add_histent(self, MAX_HISTENT_TIME, np, 'z', 0, NULL);
    } else if (vlist_type == 'B') /* bit vector, port type was converted to bit vector already */
    {
        char *sbuf = malloc_2(len + 1);
        int dst_len;
        char *vector;

        while (vlist_pos < list_size) {
            unsigned int delta;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            delta = accum;
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed 'b' signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            dst_len = 0;
            for (;;) {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                if ((ch >> 4) == AN_MSK)
                    break;
                if (dst_len == len) {
                    if (len != 1)
                        memmove(sbuf, sbuf + 1, dst_len - 1);
                    dst_len--;
                }
                sbuf[dst_len++] = AN_STR[ch >> 4];
                if ((ch & AN_MSK) == AN_MSK)
                    break;
                if (dst_len == len) {
                    if (len != 1)
                        memmove(sbuf, sbuf + 1, dst_len - 1);
                    dst_len--;
                }
                sbuf[dst_len++] = AN_STR[ch & AN_MSK];
            }

            if (len == 1) {
                add_histent(self, *curtime_pnt, np, sbuf[0], 1, NULL);
            } else {
                vector = malloc_2(len + 1);
                if (dst_len < len) {
                    unsigned char extend = (sbuf[0] == '1') ? '0' : sbuf[0];
                    memset(vector, extend, len - dst_len);
                    memcpy(vector + (len - dst_len), sbuf, dst_len);
                } else {
                    memcpy(vector, sbuf, len);
                }

                vector[len] = 0;
                add_histent(self, *curtime_pnt, np, 0, 1, vector);
            }
        }

        if (len == 1) {
            add_histent(self, MAX_HISTENT_TIME - 1, np, 'x', 0, NULL);
            add_histent(self, MAX_HISTENT_TIME, np, 'z', 0, NULL);
        } else {
            add_histent(self, MAX_HISTENT_TIME - 1, np, 'x', 0, (char *)calloc_2(1, sizeof(char)));
            add_histent(self, MAX_HISTENT_TIME, np, 'z', 0, (char *)calloc_2(1, sizeof(char)));
        }

        free_2(sbuf);
    } else if (vlist_type == 'R') /* real */
    {
        char *sbuf = malloc_2(64);
        int dst_len;
        char *vector;

        while (vlist_pos < list_size) {
            unsigned int delta;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            delta = accum;
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed 'r' signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            dst_len = 0;
            do {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                sbuf[dst_len++] = ch;
            } while (ch);

            vector = malloc_2(sizeof(double));
            sscanf(sbuf, "%lg", (double *)vector);
            add_histent(self, *curtime_pnt, np, 'g', 1, (char *)vector);
        }

        d = malloc_2(sizeof(double));
        *d = 1.0;
        add_histent(self, MAX_HISTENT_TIME - 1, np, 'g', 0, (char *)d);

        d = malloc_2(sizeof(double));
        *d = 0.0;
        add_histent(self, MAX_HISTENT_TIME, np, 'g', 0, (char *)d);

        free_2(sbuf);
    } else if (vlist_type == 'S') /* string */
    {
        char *sbuf = malloc_2(list_size); /* being conservative */
        int dst_len;
        char *vector;

        while (vlist_pos < list_size) {
            unsigned int delta;

            arr_pos = accum = 0;

            do {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                arr[arr_pos++] = ch;
            } while (!(ch & 0x80));

            for (--arr_pos; arr_pos >= 0; arr_pos--) {
                ch = arr[arr_pos];
                accum <<= 7;
                accum |= (unsigned int)(ch & 0x7f);
            }

            delta = accum;
            time_idx += delta;

            curtime_pnt = vlist_locate(self->time_vlist, time_idx ? time_idx - 1 : 0);
            if (!curtime_pnt) {
                fprintf(stderr,
                        "GTKWAVE | malformed 's' signal data for '%s' after time_idx = %d\n",
                        np->nname,
                        time_idx - delta);
                exit(255);
            }

            dst_len = 0;
            do {
                chp = vlist_locate_import(v, vlist_pos++);
                if (!chp)
                    break;
                ch = *chp;
                sbuf[dst_len++] = ch;
            } while (ch);

            vector = malloc_2(dst_len + 1);
            strcpy(vector, sbuf);
            add_histent(self, *curtime_pnt, np, 's', 1, (char *)vector);
        }

        d = malloc_2(sizeof(double));
        *d = 1.0;
        add_histent(self, MAX_HISTENT_TIME - 1, np, 'g', 0, (char *)d);

        d = malloc_2(sizeof(double));
        *d = 0.0;
        add_histent(self, MAX_HISTENT_TIME, np, 'g', 0, (char *)d);

        free_2(sbuf);
    } else if (vlist_type == '!') /* error in loading */
    {
        GwNode *n2 = (GwNode *)np->curr;

        if ((n2) &&
            (n2 != np)) /* keep out any possible infinite recursion from corrupt pointer bugs */
        {
            import_vcd_trace(self, n2);

            if (GLOBALS->vlist_prepack) {
                vlist_packer_decompress_destroy((char *)depacked);
            } else {
                vlist_destroy(v);
            }
            np->mv.mvlfac_vlist = NULL;

            np->head = n2->head;
            np->curr = n2->curr;
            return;
        }

        fprintf(stderr, "Error in decompressing vlist for '%s', exiting.\n", np->nname);
        vcd_exit(255);
    }

    if (GLOBALS->vlist_prepack) {
        vlist_packer_decompress_destroy((char *)depacked);
    } else {
        vlist_destroy(v);
    }
    np->mv.mvlfac_vlist = NULL;
}
