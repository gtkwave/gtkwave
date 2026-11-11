#include "gw-vcd-partial-loader.h"
#include "gw-dump-file.h"
#include "gw-loader.h"

/* Struct to hold symbol properties for JIT import */
typedef struct {
    int vartype;
    int size;
} GwVcdSymbolProperties;

#include "gw-vcd-file.h"
#include "gw-vcd-file-private.h"
#include "gw-util.h"
#include "gw-vlist-writer.h"
#include "gw-vlist-reader.h"
#include "gw-hash.h"
#include "vcd-keywords.h"
#include <stdio.h>
#include <fstapi.h>
#include <errno.h>

#define VCD_BSIZ 32768 /* size of getch() emulation buffer--this val should be ok */
#define VCD_INDEXSIZ (8 * 1024 * 1024)
// TODO: remove!
#define WAVE_T_WHICH_UNDEFINED_COMPNAME (-1)
#define WAVE_DECOMPRESSOR "gzip -cd " /* zcat alone doesn't cut it for AIX */

/* Special value encoding constants for scalar signals */
#define RCV_X (1 | (0 << 1))
#define RCV_Z (1 | (1 << 1))
#define RCV_H (1 | (2 << 1))
#define RCV_U (1 | (3 << 1))
#define RCV_W (1 | (4 << 1))
#define RCV_L (1 | (5 << 1))
#define RCV_D (1 | (6 << 1))



#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct vcdsymbol
{
    struct vcdsymbol *root;
    struct vcdsymbol *chain;
    GwSymbol *sym_chain;

    struct vcdsymbol *next;
    char *name;
    char *id;
    char *value;
    GwNode **narray;

    unsigned int nid;
    int msi, lsi;
    int size;

    unsigned char vartype;
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif

/* now the recoded "extra" values... */
#define RCV_X (1 | (0 << 1))
#define RCV_Z (1 | (1 << 1))
#define RCV_H (1 | (2 << 1))
#define RCV_U (1 | (3 << 1))
#define RCV_W (1 | (4 << 1))
#define RCV_L (1 | (5 << 1))
#define RCV_D (1 | (6 << 1))

struct _GwVcdPartialLoader
{
    GwLoader parent_instance;

    FILE *vcd_handle;
    gboolean is_compressed;
    off_t vcd_fsiz;

    gboolean header_over;
    gboolean is_streaming_mode;
    gboolean is_fully_initialized;

    /* NEW: Stream parsing members */
    GString *internal_buffer;
    gsize processed_offset;
    GwVcdFile *dump_file;

    /* Vlist writers for each signal */
    GHashTable *vlist_writers; /* Maps vcdsymbol id -> GwVlistWriter */
    GHashTable *vlist_import_positions; /* Maps vcdsymbol id -> gsize (import position) */
    GHashTable *vlist_symbol_properties; /* Maps vcdsymbol id -> GwVcdSymbolProperties */
    gboolean vlist_prepack;
    gint vlist_compression_level;
    GwVlist *time_vlist;
    unsigned int time_vlist_count;

    off_t vcdbyteno;
    char *vcdbuf;
    char *vst;
    char *vend;

    int error_count;
    gboolean err;

    GwTime current_time;
    GwTime current_time_raw;

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
    gint hash_cache;
    GwSymbol **sym_hash;

    char *varsplit;
    char *vsplitcurr;
    int var_prevch;

    gboolean already_backtracked;

    GSList *sym_chain;

    GwBlackoutRegions *blackout_regions;

    GwTime time_scale;
    GwTimeDimension time_dimension;
    GwTime start_time;
    GwTime end_time;
    GwTime global_time_offset;

    GwTreeNode *tree_root;

    guint numfacs;
    gchar *prev_hier_uncompressed_name;

    GwTreeNode *terminals_chain;
    GwTreeBuilder *tree_builder;

    GwDumpFile *live_dump_file_view; /* Live view with partial import */

    char *module_tree;
    int module_len_tree;

    gboolean has_escaped_names;
    guint warning_filesize;

    /* State for handling $dumpvars block */
    gboolean in_dumpvars_block;
};

G_DEFINE_TYPE(GwVcdPartialLoader, gw_vcd_partial_loader, GW_TYPE_LOADER)





/**/

static void malform_eof_fix(GwVcdPartialLoader *self)
{
    if (feof(self->vcd_handle)) {
        memset(self->vcdbuf, ' ', VCD_BSIZ);
        self->vst = self->vend;
    }
}

/**/

#undef VCD_BSEARCH_IS_PERFECT /* bsearch is imperfect under linux, but OK under AIX */

static void vcd_partial_build_symbols(GwVcdPartialLoader *self);
static void evcd_strcpy(char *dst, char *src);

// TODO: remove local copy of atoi_64
static GwTime atoi_64(const char *str)
{
    GwTime val = 0;
    unsigned char ch, nflag = 0;
    int consumed = 0;

    switch (*str) {
        case 'y':
        case 'Y':
            return (GW_TIME_CONSTANT(1));

        case 'o':
        case 'O':
            str++;
            ch = *str;
            if ((ch == 'n') || (ch == 'N'))
                return (GW_TIME_CONSTANT(1));
            else
                return (GW_TIME_CONSTANT(0));

        case 'n':
        case 'N':
            return (GW_TIME_CONSTANT(0));
            break;

        default:
            break;
    }

    while ((ch = *(str++))) {
        if ((ch >= '0') && (ch <= '9')) {
            val = (val * 10 + (ch & 15));
            consumed = 1;
        } else if ((ch == '-') && (val == 0) && (!nflag)) {
            nflag = 1;
            consumed = 1;
        } else if (consumed) {
            break;
        }
    }
    return (nflag ? (-val) : val);
}

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
static struct vcdsymbol *bsearch_vcd(GwVcdPartialLoader *self, char *key, int len)
{
    struct vcdsymbol **v;
    struct vcdsymbol *t;


    if (self->symbols_indexed != NULL) {
        unsigned int hsh = vcdid_hash(key, len);
        if (hsh >= self->vcd_minid && hsh <= self->vcd_maxid) {
            struct vcdsymbol *result = self->symbols_indexed[hsh - self->vcd_minid];
            return result;
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
static void create_sorted_table(GwVcdPartialLoader *self)
{
    struct vcdsymbol *v;
    struct vcdsymbol **pnt;
    unsigned int vcd_distance;

    g_clear_pointer(&self->symbols_sorted, g_free);
    g_clear_pointer(&self->symbols_indexed, g_free);

    if (self->numsyms > 0) {
        vcd_distance = self->vcd_maxid - self->vcd_minid + 1;


        if (vcd_distance <= VCD_INDEXSIZ) {
            self->symbols_indexed = g_new0(struct vcdsymbol *, vcd_distance);

            v = self->vcdsymroot;
            while (v) {
                if (self->symbols_indexed[v->nid - self->vcd_minid] == NULL) {
                    self->symbols_indexed[v->nid - self->vcd_minid] = v;
                }
                v = v->next;
            }
        } else {
            pnt = self->symbols_sorted = g_new0(struct vcdsymbol *, self->numsyms);
            v = self->vcdsymroot;
            while (v) {
                *(pnt++) = v;
                v = v->next;
            }

            qsort(self->symbols_sorted, self->numsyms, sizeof(struct vcdsymbol *), vcdsymcompare);
        }
    } else {
        g_test_message("create_sorted_table: No symbols to index");
    }
}

/*
 * add symbol to table.  no duplicate checking
 * is necessary as aet's are "correct."
 */
static GwSymbol *symadd(GwVcdPartialLoader *self, char *name, int hv)
{
    GwSymbol *s = g_new0(GwSymbol, 1);

    strcpy(s->name = g_malloc(strlen(name) + 1), name);
    s->sym_next = self->sym_hash[hv];
    self->sym_hash[hv] = s;

    return s;
}

/******************************************************************/

static void set_vcd_vartype(struct vcdsymbol *v, GwNode *n)
{
    unsigned char nvt;

    switch (v->vartype) {
        case V_EVENT:
            nvt = GW_VAR_TYPE_VCD_EVENT;
            break;
        case V_PARAMETER:
            nvt = GW_VAR_TYPE_VCD_PARAMETER;
            break;
        case V_INTEGER:
            nvt = GW_VAR_TYPE_VCD_INTEGER;
            break;
        case V_REAL:
            nvt = GW_VAR_TYPE_VCD_REAL;
            break;
        case V_REG:
            nvt = GW_VAR_TYPE_VCD_REG;
            break;
        case V_SUPPLY0:
            nvt = GW_VAR_TYPE_VCD_SUPPLY0;
            break;
        case V_SUPPLY1:
            nvt = GW_VAR_TYPE_VCD_SUPPLY1;
            break;
        case V_TIME:
            nvt = GW_VAR_TYPE_VCD_TIME;
            break;
        case V_TRI:
            nvt = GW_VAR_TYPE_VCD_TRI;
            break;
        case V_TRIAND:
            nvt = GW_VAR_TYPE_VCD_TRIAND;
            break;
        case V_TRIOR:
            nvt = GW_VAR_TYPE_VCD_TRIOR;
            break;
        case V_TRIREG:
            nvt = GW_VAR_TYPE_VCD_TRIREG;
            break;
        case V_TRI0:
            nvt = GW_VAR_TYPE_VCD_TRI0;
            break;
        case V_TRI1:
            nvt = GW_VAR_TYPE_VCD_TRI1;
            break;
        case V_WAND:
            nvt = GW_VAR_TYPE_VCD_WAND;
            break;
        case V_WIRE:
            nvt = GW_VAR_TYPE_VCD_WIRE;
            break;
        case V_WOR:
            nvt = GW_VAR_TYPE_VCD_WOR;
            break;
        case V_PORT:
            nvt = GW_VAR_TYPE_VCD_PORT;
            break;
        case V_STRINGTYPE:
            nvt = GW_VAR_TYPE_GEN_STRING;
            break;
        case V_BIT:
            nvt = GW_VAR_TYPE_SV_BIT;
            break;
        case V_LOGIC:
            nvt = GW_VAR_TYPE_SV_LOGIC;
            break;
        case V_INT:
            nvt = GW_VAR_TYPE_SV_INT;
            break;
        case V_SHORTINT:
            nvt = GW_VAR_TYPE_SV_SHORTINT;
            break;
        case V_LONGINT:
            nvt = GW_VAR_TYPE_SV_LONGINT;
            break;
        case V_BYTE:
            nvt = GW_VAR_TYPE_SV_BYTE;
            break;
        case V_ENUM:
            nvt = GW_VAR_TYPE_SV_ENUM;
            break;
        /* V_SHORTREAL as a type does not exist for VCD: is cast to V_REAL */
        default:
            nvt = GW_VAR_TYPE_UNSPECIFIED_DEFAULT;
            break;
    }
    n->vartype = nvt;
}



/*
 * single char get inlined/optimized
 */




static int getch_fetch(GwVcdPartialLoader *self)
{
    size_t rd;

    errno = 0;
    if (feof(self->vcd_handle)) {
        return (-1);
    }

    self->vcdbyteno += (self->vend - self->vcdbuf);
    memset(self->vcdbuf, 0, VCD_BSIZ);
    rd = fread(self->vcdbuf, sizeof(char), VCD_BSIZ, self->vcd_handle);
    self->vend = (self->vst = self->vcdbuf) + rd;

    if ((!rd) || (errno)) {
        return (-1);
    }

    if (self->vcd_fsiz > 0) {
        // TODO: update splash
        // splash_sync(self->vcdbyteno, self->vcd_fsiz); /* gnome 2.18 seems to set errno so splash
        //                                                                     moved here... */
    }

    return ((int)(*self->vst));
}

static inline signed char getch(GwVcdPartialLoader *self)
{
    signed char ch;
    if (self->vst == self->vend) {
        ch = getch_fetch(self);
    } else {
        ch = (signed char)*self->vst;
        if (ch == 0) {
            return -1;
        }
    }
    self->vst++;
    return (ch);
}

static inline signed char getch_peek(GwVcdPartialLoader *self)
{
    signed char ch;
    if (self->vst == self->vend) {
        ch = getch_fetch(self);
    } else {
        ch = (signed char)*self->vst;
        if (ch == 0) {
            return -1;
        }
    }
    /* no increment */
    return (ch);
}

static int getch_patched(GwVcdPartialLoader *self)
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
static int get_token(GwVcdPartialLoader *self)
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
            self->yytext = g_realloc(self->yytext, self->T_MAX_STR + 1);
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

static int get_vartoken_patched(GwVcdPartialLoader *self, int match_kw)
{
    int ch;
    int len = 0;

    if (!self->var_prevch) {
        for (;;) {
            ch = getch_patched(self);
            if (ch < 0) {
                g_free(self->varsplit);
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
            self->yytext = g_realloc(self->yytext, self->T_MAX_STR + 1);
        }
        ch = getch_patched(self);
        if (ch < 0) {
            g_free(self->varsplit);
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
                g_free(self->varsplit);
                self->varsplit = NULL;
            }
            return (vr);
        }
    }

    self->yylen = len;
    if (ch < 0) {
        g_free(self->varsplit);
        self->varsplit = NULL;
    }
    return V_STRING;
}

static int get_vartoken(GwVcdPartialLoader *self, int match_kw)
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
            self->yytext = g_realloc(self->yytext, self->T_MAX_STR + 1);
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
        vst = g_malloc(strlen(self->varsplit) + 1);
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

static int get_strtoken(GwVcdPartialLoader *self)
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
            self->yytext = g_realloc(self->yytext, self->T_MAX_STR + 1);
        }
        ch = getch(self);
        if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r') || (ch < 0))
            break;
    }
    self->yytext[len] = 0; /* terminator */

    self->yylen = len;
    return V_STRING;
}

static void sync_end(GwVcdPartialLoader *self)
{
    for (;;) {
        int tok = get_token(self);
        if (tok == T_END || tok == T_EOF) {
            break;
        }
    }
}

static void version_sync_end(GwVcdPartialLoader *self)
{
    for (;;) {
        int tok = get_token(self);
        if (tok == T_END || tok == T_EOF) {
            break;
        }

        // Turn off autocoalesce for Icarus.
        // see https://github.com/gtkwave/gtkwave/issues/331 for additional information
        if (strstr(self->yytext, "Icarus") != NULL) {
            gw_loader_set_autocoalesce(GW_LOADER(self), FALSE);
        }
    }
}

static void vcd_partial_parse_valuechange_scalar(GwVcdPartialLoader *self)
{
    struct vcdsymbol *v;

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

            if (n->mv.mvlfac_vlist_writer ==
                NULL) /* overloaded for vlist, numhist = last position used */
            {
                n->mv.mvlfac_vlist_writer =
                    gw_vlist_writer_new(self->vlist_compression_level, self->vlist_prepack);
                gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer,
                                            (unsigned int)'0'); /* represents single bit routine
                                                                 for decompression */
                gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, (unsigned int)v->vartype);
            }

            // Calculate actual time difference instead of step difference
            // Always calculate time delta as difference from previous time
            time_delta = (unsigned int)(self->current_time - n->last_time);
            n->last_time = self->current_time;

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

            gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, rcv);
        }
    } else {
        fprintf(stderr,
                "Near byte %d, Malformed VCD identifier\n",
                (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
        malform_eof_fix(self);
    }
}

static void process_binary_stream(GwVcdPartialLoader *self, GwVlistWriter *writer, const gchar *vector, gint vlen, struct vcdsymbol *v, unsigned int time_delta, GError **error)
{
    g_assert(v != NULL);
    g_assert(writer != NULL);
    g_assert(error != NULL && *error == NULL);


    // The writer is now created upfront and passed in. We just need to write the value.
    gw_vlist_writer_append_uv32(writer, time_delta);
    gw_vlist_writer_append_string(writer, vector);

    // Debug: check what was actually written to the vlist
    GwVlist *vlist = gw_vlist_writer_get_vlist(writer);
    if (vlist && vlist->size > 0) {
        // Create a temporary reader to see what's in the vlist
        GwVlistReader *temp_reader = gw_vlist_reader_new_from_writer(writer);
        g_object_unref(temp_reader);
    }
}

static void process_binary(GwVcdPartialLoader *self, gchar typ, const gchar *vector, gint vlen)
{
    struct vcdsymbol *v = bsearch_vcd(self, self->yytext, self->yylen);
    if (v == NULL) {
        fprintf(stderr,
                "Near byte %d, Unknown VCD identifier: '%s'\n",
                (int)(self->vcdbyteno + (self->vst - self->vcdbuf)),
                self->yytext + 1);
        malform_eof_fix(self);
    }

    GwNode *n = v->narray[0];
    unsigned int time_delta;

    if (n->mv.mvlfac_vlist_writer == NULL) /* overloaded for vlist, numhist = last position used */
    {
        unsigned char typ2 = toupper(typ);
        n->mv.mvlfac_vlist_writer =
            gw_vlist_writer_new(self->vlist_compression_level, self->vlist_prepack);

        if (v->vartype != V_REAL && v->vartype != V_STRINGTYPE) {
            if (typ2 == 'R' || typ2 == 'S') {
                typ2 = 'B'; /* ok, typical case...fix as 'r' on bits variable causes
                               recoder crash during trace extraction */
            }
        } else {
            if (typ2 == 'B') {
                typ2 = 'S'; /* should never be necessary...this is defensive */
            }
        }

        gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer,
                                    (unsigned int)toupper(typ2)); /* B/R/P/S for decompress */
        gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, (unsigned int)v->vartype);
        gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, (unsigned int)v->size);
    }

    // Calculate actual time difference instead of step difference
    // Always calculate time delta as difference from previous time
    time_delta = (unsigned int)(self->current_time - n->last_time);
    n->last_time = self->current_time;

    gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, time_delta);

    if (typ == 'b' || typ == 'B') {
        if (v->vartype != V_REAL && v->vartype != V_STRINGTYPE) {
            gw_vlist_writer_append_string(n->mv.mvlfac_vlist_writer, vector);
        } else {
            gw_vlist_writer_append_string(n->mv.mvlfac_vlist_writer, vector);
        }
    } else {
        if (v->vartype == V_REAL || v->vartype == V_STRINGTYPE || typ == 's' || typ == 'S') {
            gw_vlist_writer_append_string(n->mv.mvlfac_vlist_writer, vector);
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
            gw_vlist_writer_append_string(n->mv.mvlfac_vlist_writer, bits);
        }
    }
}

static void vcd_partial_parse_valuechange(GwVcdPartialLoader *self)
{
    unsigned char typ = self->yytext[0];
    switch (typ) {
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
            vcd_partial_parse_valuechange_scalar(self);
            break;

            /* encode everything else literally as a time delta + a string */
#ifndef STRICT_VCD_ONLY
        case 's':
        case 'S': {
            gchar *vector = g_alloca(self->yylen);
            gint vlen = fstUtilityEscToBin((unsigned char *)vector,
                                           (unsigned char *)(self->yytext + 1),
                                           self->yylen - 1);
            vector[vlen] = 0;

            get_strtoken(self);
            process_binary(self, typ, vector, vlen);
            break;
        }
#endif

        case 'b':
        case 'B':
        case 'r':
        case 'R': {
            gchar *vector = g_alloca(self->yylen);
            strcpy(vector, self->yytext + 1);
            gint vlen = self->yylen - 1;

            get_strtoken(self);

            process_binary(self, typ, vector, vlen);
            break;
        }

        case 'p':
        case 'P': {
            /* extract port dump value.. */
            gchar *vector = g_alloca(self->yylen);
            evcd_strcpy(vector, self->yytext + 1); /* convert to regular vcd */
            gint vlen = self->yylen - 1;

            get_strtoken(self); /* throw away 0_strength_component */
            get_strtoken(self); /* throw away 0_strength_component */
            get_strtoken(self); /* this is the id                  */

            // type = 'b', because it was already converted to regular VCD values
            process_binary(self, 'b', vector, vlen);
            break;
        }

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

static void fractional_timescale_fix(char *s)
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

    static const gchar *WAVE_SI_UNITS = " munpfaz";

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

static void vcd_partial_parse_timezero(GwVcdPartialLoader *self)
{
    int vtok = get_token(self);
    if (vtok == T_END || vtok == T_EOF) {
        return;
    }

    self->global_time_offset = atoi_64(self->yytext);

    sync_end(self);
}

static void vcd_partial_parse_timescale(GwVcdPartialLoader *self)
{
    int vtok;
    int i;
    char prefix = ' ';

    vtok = get_token(self);
    if (vtok == T_END || vtok == T_EOF) {
        return;
    }

    fractional_timescale_fix(self->yytext);
    self->time_scale = atoi_64(self->yytext);
    if (self->time_scale < 1) {
        self->time_scale = 1;
    }
    for (i = 0; i < self->yylen; i++) {
        if (self->yytext[i] < '0' || self->yytext[i] > '9') {
            prefix = self->yytext[i];
            break;
        }
    }
    if (prefix == ' ') {
        vtok = get_token(self);
        if (vtok == T_END || vtok == T_EOF) {
            return;
        }
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
            self->time_dimension = prefix;
            break;
        case 's':
            self->time_dimension = ' ';
            break;
        default: /* unknown */
            self->time_dimension = 'n';
            break;
    }

    sync_end(self);
}

static void vcd_partial_parse_scope(GwVcdPartialLoader *self)
{
    int tok = get_token(self);
    if (tok == T_END || tok == T_EOF) {
        return;
    }

    unsigned char ttype;
    switch (self->yytext[0]) {
        case 'm':
            ttype = GW_TREE_KIND_VCD_ST_MODULE;
            break;
        case 't':
            ttype = GW_TREE_KIND_VCD_ST_TASK;
            break;
        case 'f':
            ttype =
                (self->yytext[1] == 'u') ? GW_TREE_KIND_VCD_ST_FUNCTION : GW_TREE_KIND_VCD_ST_FORK;
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
                        ttype = (!strncmp(vht + 6, "roces", 5)) ? GW_TREE_KIND_VHDL_ST_PROCESS
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

    tok = get_token(self);
    if (tok == T_END || tok == T_EOF) {
        return;
    }

    GwTreeNode *scope = gw_tree_builder_push_scope(self->tree_builder, ttype, self->yytext);
    scope->t_which = -1;

    sync_end(self);
}

static void vcd_partial_parse_upscope(GwVcdPartialLoader *self)
{
    // TODO: add warning for upscope without scope
    gw_tree_builder_pop_scope(self->tree_builder);

    sync_end(self);
}

static gboolean vcd_partial_parse_var_evcd(GwVcdPartialLoader *self, struct vcdsymbol *v, gint *vtok)
{
    *vtok = get_vartoken(self, 1);
    if (*vtok == V_STRING) {
        v->size = atoi_64(self->yytext);
        if (!v->size)
            v->size = 1;
    } else if (*vtok == V_LB) {
        *vtok = get_vartoken(self, 1);
        if (*vtok == V_END || *vtok != V_STRING) {
            return FALSE;
        }
        v->msi = atoi_64(self->yytext);
        *vtok = get_vartoken(self, 0);
        if (*vtok == V_RB) {
            v->lsi = v->msi;
            v->size = 1;
        } else {
            if (*vtok != V_COLON) {
                return FALSE;
            }
            *vtok = get_vartoken(self, 0);
            if (*vtok != V_STRING) {
                return FALSE;
            }
            v->lsi = atoi_64(self->yytext);
            *vtok = get_vartoken(self, 0);
            if (*vtok != V_RB) {
                return FALSE;
            }

            if (v->msi > v->lsi) {
                v->size = v->msi - v->lsi + 1;
            } else {
                v->size = v->lsi - v->msi + 1;
            }
        }
    } else {
        return FALSE;
    }

    *vtok = get_strtoken(self);
    if (*vtok == V_END) {
        return FALSE;
    }
    v->id = g_malloc(self->yylen + 1);
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

    *vtok = get_vartoken(self, 0);
    if (*vtok != V_STRING) {
        return FALSE;
    }

    v->name = gw_tree_builder_get_symbol_name(self->tree_builder, self->yytext);

    if (self->pv != NULL) {
        if (!strcmp(self->prev_hier_uncompressed_name, v->name) &&
            gw_loader_is_autocoalesce(GW_LOADER(self)) && (!strchr(v->name, '\\'))) {
            self->pv->chain = v;
            v->root = self->rootv;
            if (self->pv == self->rootv) {
                self->pv->root = self->rootv;
            }
        } else {
            self->rootv = v;
        }

        g_free(self->prev_hier_uncompressed_name);
    } else {
        self->rootv = v;
    }

    self->pv = v;
    self->prev_hier_uncompressed_name = g_strdup(v->name);

    return TRUE;
}

static gboolean vcd_partial_parse_var_regular(GwVcdPartialLoader *self, struct vcdsymbol *v, int *vtok)
{
    *vtok = get_vartoken(self, 1);
    if (*vtok == V_END) {
        return FALSE;
    }
    v->size = atoi_64(self->yytext);
    *vtok = get_strtoken(self);
    if (*vtok == V_END) {
        return FALSE;
    }
    v->id = g_malloc(self->yylen + 1);
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

    *vtok = get_vartoken(self, 0);
    if (*vtok != V_STRING) {
        return FALSE;
    }

    v->name = gw_tree_builder_get_symbol_name(self->tree_builder, self->yytext);

    if (self->pv != NULL) {
        if (!strcmp(self->prev_hier_uncompressed_name, v->name)) {
            self->pv->chain = v;
            v->root = self->rootv;
            if (self->pv == self->rootv) {
                self->pv->root = self->rootv;
            }
        } else {
            self->rootv = v;
        }

        g_free(self->prev_hier_uncompressed_name);
    } else {
        self->rootv = v;
    }
    self->pv = v;
    self->prev_hier_uncompressed_name = g_strdup(v->name);

    *vtok = get_vartoken(self, 1);
    if (*vtok == V_END) {
        return TRUE;
    }

    if (*vtok != V_LB) {
        return FALSE;
    }
    *vtok = get_vartoken(self, 0);
    if (*vtok != V_STRING) {
        return FALSE;
    }
    v->msi = atoi_64(self->yytext);
    *vtok = get_vartoken(self, 0);
    if (*vtok == V_RB) {
        v->lsi = v->msi;
        return TRUE;
    }
    if (*vtok != V_COLON) {
        return FALSE;
    }
    *vtok = get_vartoken(self, 0);
    if (*vtok != V_STRING) {
        return FALSE;
    }
    v->lsi = atoi_64(self->yytext);
    *vtok = get_vartoken(self, 0);
    if (*vtok != V_RB) {
        return FALSE;
    }

    return TRUE;
}

static void vcd_partial_parse_var(GwVcdPartialLoader *self)
{
    // TODO: why was this disabled?
    // if ((self->header_over) && (0)) {
    //     fprintf(stderr,
    //             "$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, "
    //             "exiting.\n",
    //             (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
    //     vcd_exit(255);
    // } else {

    int vtok;
    struct vcdsymbol *v = NULL;

    self->var_prevch = 0;
    if (self->varsplit) {
        g_free(self->varsplit);
        self->varsplit = NULL;
    }
    vtok = get_vartoken(self, 1);
    if (vtok > V_STRINGTYPE)
        goto bail;

    v = g_new0(struct vcdsymbol, 1);
    v->vartype = vtok;
    v->msi = v->lsi = -1;

    if (vtok == V_PORT) {
        if (!vcd_partial_parse_var_evcd(self, v, &vtok)) {
            goto err;
        }
    } else {
        if (!vcd_partial_parse_var_regular(self, v, &vtok)) {
            goto err;
        }
    }

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
    v->narray = g_new0(GwNode *, 1);
    v->narray[0] = g_new0(GwNode, 1);
    v->narray[0]->head.time = -2;
    v->narray[0]->head.v.h_val = GW_BIT_X;

    if (self->vcdsymroot == NULL) {
        self->vcdsymroot = self->vcdsymcurr = v;
    } else {
        self->vcdsymcurr->next = v;
    }
    self->vcdsymcurr = v;
    self->numsyms++;

    goto bail;
err:
    if (v) {
        self->error_count++;
        if (v->name) {
            fprintf(stderr,
                    "Near byte %d, $VAR parse error encountered with '%s'\n",
                    (int)(self->vcdbyteno + (self->vst - self->vcdbuf)),
                    v->name);
            g_free(v->name);
        } else {
            fprintf(stderr,
                    "Near byte %d, $VAR parse error encountered\n",
                    (int)(self->vcdbyteno + (self->vst - self->vcdbuf)));
        }
        if (v->id)
            g_free(v->id);
        g_free(v);
        v = NULL;
        self->pv = NULL;
    }

bail:
    if (vtok != V_END)
        sync_end(self);
}

static GwTree *vcd_build_tree(GwVcdPartialLoader *self, GwFacs *facs);

static void _gw_vcd_partial_loader_initialize_dump_file(GwVcdPartialLoader *self)
{

    // 0. Create sorted symbol table for duplicate detection
    create_sorted_table(self);

    vcd_partial_build_symbols(self);

    if (self->sym_chain == NULL || self->numfacs == 0) {
        return; // No signals defined yet
    }

    // Create facs from the now-finalized symbol chain
    GwFacs *facs = gw_facs_new(self->numfacs);
    guint i = 0;
    for (GSList *iter = self->sym_chain; iter != NULL; iter = iter->next) {
        gw_facs_set(facs, i++, iter->data);
    }
    gw_facs_sort(facs); // IMPORTANT: Sort the facs by their full names

    // Build tree from tree builder to get proper scope hierarchy with tree kinds
    self->tree_root = gw_tree_builder_build(self->tree_builder);
    GwTree *tree = vcd_build_tree(self, facs);

    g_debug("Current time properties: scale=%" GW_TIME_FORMAT ", dimension=%d",
            self->time_scale, self->time_dimension);

    // Only create dump file when time properties are known
    if (self->time_scale == 0 || self->time_dimension == GW_TIME_DIMENSION_NONE) {
        g_debug("Time properties not yet known, cannot create dump file");
        return;
    }

    // Always create a new dump file with the updated tree and facs
    if (self->dump_file != NULL) {
        g_object_unref(self->dump_file);
    }

    g_debug("Creating dump file with time_scale=%" GW_TIME_FORMAT ", time_dimension=%d",
            self->time_scale, self->time_dimension);
    GwTimeRange *time_range = gw_time_range_new(self->start_time, self->end_time);
    self->dump_file = g_object_new(GW_TYPE_VCD_FILE,
                                            "tree", tree,
                                            "facs", facs,
                                            "blackout-regions", self->blackout_regions,
                                            "time-scale", self->time_scale,
                                            "time-dimension", self->time_dimension,
                                            "time-range", time_range,
                                            "global-time-offset", self->global_time_offset,
                                            "has-escaped-names", self->has_escaped_names,
                                            /* VCD specific properties are FALSE or NULL */
                                            "stems", NULL,
                                            "component-names", NULL,
                                            "enum-filters", NULL,
                                            "has-nonimplicit-directions", FALSE,
                                            "has-supplemental-datatypes", FALSE,
                                            "has-supplemental-vartypes", FALSE,
                                            "uses-vhdl-component-format", FALSE,
                                            NULL);
    g_object_unref(time_range);
    g_debug("Dump file created successfully: %p", self->dump_file);



    // The object takes ownership, so we don't need to unref tree and facs

    self->is_fully_initialized = TRUE;
}

// Handle streaming mode initialization when $enddefinitions is not present
static void _gw_vcd_partial_loader_initialize_streaming_mode(GwVcdPartialLoader *self)
{
    if (self->is_fully_initialized) {
        return; // Already initialized
    }

    // Check if we have enough information to initialize in streaming mode
    if (self->time_scale != 0 && self->numfacs > 0 &&
        gw_tree_builder_get_current_scope(self->tree_builder) == NULL) {

        _gw_vcd_partial_loader_initialize_dump_file(self);
        self->is_streaming_mode = TRUE;
    }
}

static void vcd_partial_parse_enddefinitions(GwVcdPartialLoader *self, GError **error)
{
    self->header_over = TRUE; /* do symbol table management here */


    if (!self->is_fully_initialized) {
        _gw_vcd_partial_loader_initialize_dump_file(self);
    }

    create_sorted_table(self);
    if (self->symbols_sorted == NULL && self->symbols_indexed == NULL) {
        g_set_error(error,
                    GW_DUMP_FILE_ERROR,
                    GW_DUMP_FILE_ERROR_NO_SYMBOLS,
                    "No symbols in VCD file");
        return;
    }

    // TODO: report more detailed error
    if (self->error_count > 0) {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN, "VCD parse error");
        return;
    }
}

static void vcd_partial_parse_string(GwVcdPartialLoader *self)
{
    if (!self->header_over) {
        self->header_over = TRUE; /* do symbol table management here */
        create_sorted_table(self);
        if (self->symbols_sorted == NULL && self->symbols_indexed == NULL) {
            return;
        }
    }

    /* catchall for events when header over */
    if (self->yytext[0] == '#') {
        GwTime tim;
        GwTime *tt;

        tim = atoi_64(self->yytext + 1);

        if (self->start_time < 0) {
            self->start_time = tim;
        } else {
            /* backtracking fix */
            if (tim < self->current_time) {
                if (!self->already_backtracked) {
                    self->already_backtracked = TRUE;
                    fprintf(stderr, "VCDLOAD | Time backtracking detected in VCD file!\n");
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
        if (self->end_time < tim)
            self->end_time = tim; /* in case of malformed vcd files */
        // DEBUG(fprintf(stderr, "#%" GW_TIME_FORMAT "\n", tim));

        tt = gw_vlist_alloc(&self->time_vlist, FALSE, self->vlist_compression_level);
        *tt = tim;
        self->time_vlist_count++;
    } else {
        if (self->time_vlist_count) {
            /* OK, otherwise fix for System C which doesn't emit time zero... */
        } else {
            GwTime tim = GW_TIME_CONSTANT(0);
            GwTime *tt;

            self->start_time = self->current_time = self->end_time = tim;

            tt = gw_vlist_alloc(&self->time_vlist, FALSE, self->vlist_compression_level);
            *tt = tim;
            self->time_vlist_count = 1;
        }
        vcd_partial_parse_valuechange(self);
    }
}



/*******************************************************************************/

static GwSymbol *symfind_unsorted(GwVcdPartialLoader *self, char *s)
{
    int hv = gw_hash(s);

    for (GwSymbol *iter = self->sym_hash[hv]; iter != NULL; iter = iter->sym_next) {
        if (strcmp(iter->name, s) == 0) {
            return iter; /* in table already */
        }
    }

    return NULL; /* not found, add here if you want to add*/
}

static void treenamefix_str(char *s, char delimiter)
{
    while (*s) {
        if (*s == VCD_HIERARCHY_DELIMITER)
            *s = delimiter;
        s++;
    }
}

static void vcd_partial_build_symbols(GwVcdPartialLoader *self)
{
    struct vcdsymbol *v, *vprime;
    GSList *sym_chain = NULL;

    // The full hierarchical names have already been created in _vcd_partial_handle_var.
    // This function now primarily handles aliasing/duplicate IDs and vector chaining.

    v = self->vcdsymroot;
    while (v) {
        // Find the corresponding GwSymbol created during initial parsing
        GwSymbol *s = v->sym_chain;
        if (!s) {
            v = v->next;
            continue;
        }

        // Check for duplicate IDs (aliasing)
        if ((vprime = bsearch_vcd(self, v->id, strlen(v->id))) != v && vprime != NULL) {
            if (v->size == vprime->size) {
                // This is an alias. We need to point our node's data to the original's.
                GwNode *n = s->n;
                GwNode *n2 = vprime->sym_chain->n;
                n->curr = (GwHistEnt *)n2; // Point to the original's history
                n->numhist = n2->numhist;
            } else {
                fprintf(stderr, "ERROR: Duplicate IDs with differing width: %s %s\n", v->name, vprime->name);
            }
        }

        // Re-link the vector chains
        if ((v->size == 1) && (v->root)) {
            sym_chain = g_slist_prepend(sym_chain, s);
        }

        v = v->next;
    }

    // Process the vector chain links
    if (sym_chain != NULL) {
        for (GSList *iter = sym_chain; iter != NULL; iter = iter->next) {
            GwSymbol *s = iter->data;
            struct vcdsymbol *v_root = (struct vcdsymbol *)s->vec_root;
            struct vcdsymbol *v_chain = (struct vcdsymbol *)s->vec_chain;

            if (v_root) s->vec_root = v_root->sym_chain;
            if (v_chain) s->vec_chain = v_chain->sym_chain;
        }
        g_slist_free(sym_chain);
    }
}

/*******************************************************************************/



static GwFacs *vcd_sortfacs(GwVcdPartialLoader *self)
{
    GwFacs *facs = gw_facs_new(self->numfacs);

    GSList *iter = self->sym_chain;
    for (guint i = 0; i < self->numfacs; i++) {
        GwSymbol *fac = iter->data;
        gw_facs_set(facs, i, fac);

        iter = g_slist_delete_link(iter, iter);
    }

    gw_facs_sort(facs);

    return facs;
}

/*
 * extract the next part of the name in the flattened
 * hierarchy name.  return ptr to next name if it exists
 * else NULL
 */
static const char *get_module_name(GwVcdPartialLoader *self, const char *s)
{
    char ch;
    char *pnt;
    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

    pnt = self->module_tree;

    for (;;) {
        ch = *(s++);

        if (ch == delimiter &&
            *s != '\0') /* added && null check to allow possible . at end of name */
        {
            *(pnt) = 0;
            self->module_len_tree = pnt - self->module_tree;
            return (s);
        }

        if (!(*(pnt++) = ch)) {
            self->module_len_tree = pnt - self->module_tree;
            return (NULL); /* nothing left to extract */
        }
    }
}

static void build_tree_from_name(GwVcdPartialLoader *self,
                                 GwTreeNode **tree_root,
                                 const char *s,
                                 int which)
{
    GwTreeNode *t, *nt;
    GwTreeNode *tchain = NULL, *tchain_iter;
    GwTreeNode *prevt;


    if (s == NULL || !s[0])
        return;

    t = *tree_root;

    if (t) {
        prevt = NULL;
        while (s) {
        rs:
            s = get_module_name(self, s);

            if (s && t &&
                !strcmp(t->name, self->module_tree)) /* ajb 300616 added "s &&" to cover case where
                                                        we can have hierarchy + final name are same,
                                                        see A.B.C.D notes elsewhere in this file */
            {

                prevt = t;
                t = t->child;
                continue;
            } else if (s && t) {
                //g_test_message("  Module mismatch: t->name=%s, self->module_tree=%s", t->name, self->module_tree);
            }

            tchain = tchain_iter = t;
            if (s && t) {
                nt = t->next;
                while (nt) {
                    if (nt && !strcmp(nt->name, self->module_tree)) {
                        /* move to front to speed up next compare if in same hier during build */
                        if (prevt) {
                            tchain_iter->next = nt->next;
                            nt->next = tchain;
                            prevt->child = nt;
                        }

                        prevt = nt;
                        t = nt->child;
                        goto rs;
                    }

                    tchain_iter = nt;
                    nt = nt->next;
                }
            }

            nt = gw_tree_node_new(0, self->module_tree);

            if (s) {
                nt->t_which = WAVE_T_WHICH_UNDEFINED_COMPNAME;

                if (prevt) /* make first in chain */
                {
                    nt->next = prevt->child;
                    prevt->child = nt;
                } else /* make second in chain as it's toplevel */
                {
                    nt->next = tchain->next;
                    tchain->next = nt;
                }
            } else {
                nt->child = prevt; /* parent */
                nt->t_which = which;
                nt->next = self->terminals_chain;
                self->terminals_chain = nt;

                return;
            }

            /* blindly clone fac from next part of hier on down */
            t = nt;
            while (s) {
                s = get_module_name(self, s);
                nt = gw_tree_node_new(0, self->module_tree);

                if (s) {
                    nt->t_which = WAVE_T_WHICH_UNDEFINED_COMPNAME;
                    t->child = nt;
                    t = nt;
                } else {
                    nt->child = t; /* parent */
                    nt->t_which = which;
                    nt->next = self->terminals_chain;
                    self->terminals_chain = nt;
                }
            }
        }
    } else {
        /* blindly create first fac in the tree (only ever called once) */
        while (s) {
            s = get_module_name(self, s);

            nt = gw_tree_node_new(0, self->module_tree);

            if (!s)
                nt->t_which = which;
            else
                nt->t_which = WAVE_T_WHICH_UNDEFINED_COMPNAME;

            if (*tree_root != NULL && t != NULL) {
                t->child = nt;
                t = nt;
            } else {
                t = nt;
                *tree_root = t;
            }
        }
    }
}



static void treenamefix(GwTreeNode *t, char delimiter)
{
    GwTreeNode *tnext;
    if (t->child)
        treenamefix(t->child, delimiter);

    tnext = t->next;

    while (tnext) {
        if (tnext->child)
            treenamefix(tnext->child, delimiter);
        treenamefix_str(tnext->name, delimiter);
        tnext = tnext->next;
    }

    treenamefix_str(t->name, delimiter);
}

// This function is now only used in initialization
static GwTree *vcd_build_tree(GwVcdPartialLoader *self, GwFacs *facs)
{
    // TODO: replace module_tree by GString to dynamically allocate enough memory
    self->module_tree = g_malloc0(65536);

    gchar delimiter = VCD_HIERARCHY_DELIMITER;

    for (guint i = 0; i < gw_facs_get_length(facs); i++) {
        GwSymbol *fac = gw_facs_get(facs, i);

        char *n = fac->name;
        build_tree_from_name(self, &self->tree_root, n, i);

        if (self->has_escaped_names) {
            char *subst, ch;
            subst = fac->name;
            while ((ch = (*subst))) {
                if (ch == VCD_HIERARCHY_DELIMITER) {
                    *subst = delimiter;
                } /* restore back to normal */
                subst++;
            }
        }
    }

    GwTree *tree = gw_tree_new(g_steal_pointer(&self->tree_root));
    if (self->terminals_chain != NULL) {
        gw_tree_graft(tree, self->terminals_chain);
    }
    gw_tree_sort(tree);



    if (self->has_escaped_names) {
        treenamefix(gw_tree_get_root(tree), delimiter);
    }

    return tree;
}

/*******************************************************************************/



static void gw_vcd_partial_loader_finalize(GObject *object)
{
    GwVcdPartialLoader *self = GW_VCD_PARTIAL_LOADER(object);

    g_free(self->sym_hash);
    if (self->vlist_writers != NULL) {
        g_hash_table_destroy(self->vlist_writers);
        self->vlist_writers = NULL;
    }
    if (self->vlist_symbol_properties != NULL) {
        g_hash_table_destroy(self->vlist_symbol_properties);
        self->vlist_symbol_properties = NULL;
    }
    if (self->vlist_import_positions != NULL) {
        g_hash_table_destroy(self->vlist_import_positions);
        self->vlist_import_positions = NULL;
    }


    G_OBJECT_CLASS(gw_vcd_partial_loader_parent_class)->finalize(object);
}

static GwDumpFile *
gw_vcd_partial_loader_load(GwLoader *loader G_GNUC_UNUSED, const gchar *fname G_GNUC_UNUSED, GError **error)
{
    // This loader is for streams only. Using it for direct file loading is an error.
    g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                "GwVcdPartialLoader does not support direct file loading. Use the feed() API instead.");
    return NULL;
}

static void gw_vcd_partial_loader_class_init(GwVcdPartialLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GwLoaderClass *loader_class = GW_LOADER_CLASS(klass); // Get the parent class

    object_class->finalize = gw_vcd_partial_loader_finalize;

    // Assign our (non-functional for now) load function to the parent class vtable
    loader_class->load = gw_vcd_partial_loader_load;
}

static void gw_vcd_partial_loader_init(GwVcdPartialLoader *self)
{
    self->current_time = -1;
    self->start_time = -1;
    self->end_time = -1;
    self->time_scale = 1;
    self->time_dimension = GW_TIME_DIMENSION_NANO;

    self->T_MAX_STR = 1024;
    self->yytext = g_malloc(self->T_MAX_STR + 1);
    self->vcd_minid = G_MAXUINT;
    self->vcd_maxid = 0;
    self->tree_builder = gw_tree_builder_new(gw_loader_get_hierarchy_delimiter(GW_LOADER(self)));
    self->blackout_regions = gw_blackout_regions_new();
    self->numfacs = 0;

    self->vlist_compression_level = Z_DEFAULT_COMPRESSION;

    self->sym_hash = g_new0(GwSymbol *, GW_HASH_PRIME);
    self->warning_filesize = 256;

    /* NEW: Initialize stateful parsing members */
    self->internal_buffer = g_string_new("");
    self->processed_offset = 0;
    self->is_streaming_mode = FALSE;
    self->tree_root = NULL; // Initialize tree_root to avoid stale pointers

    /* Create the single dump file instance that will be updated live */
    self->dump_file = NULL; // Will be created when time properties are known

    /* Initialize vlist writers hash table */
    self->vlist_writers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

    /* Initialize vlist import positions hash table */
    self->vlist_import_positions = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    /* Initialize symbol properties hash table */
    self->vlist_symbol_properties = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    /* Initialize time vlist */

    /* Initialize dumpvars state */
    self->in_dumpvars_block = FALSE;
    self->time_vlist = gw_vlist_create(sizeof(GwTime));
    self->time_vlist_count = 0;
}






// Forward declarations for handler functions
static void _vcd_partial_handle_timescale(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error);
static void _vcd_partial_handle_timezero(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error);
static void _vcd_partial_handle_scope(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error);
static void _vcd_partial_handle_var(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error);
static void _vcd_partial_handle_time_change(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error);
static void _vcd_partial_handle_value_change(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error);

// State machine for handling $dumpvars block
static void _vcd_partial_enter_dumpvars_state(GwVcdPartialLoader *self)
{
    g_debug("Entering dumpvars state");
    self->in_dumpvars_block = TRUE;
    self->current_time = 0;
    self->time_vlist_count = 0;
}

static void _vcd_partial_exit_dumpvars_state(GwVcdPartialLoader *self)
{
    g_debug("Exiting dumpvars state");
    self->in_dumpvars_block = FALSE;
}

static void _vcd_partial_handle_dumpvars(GwVcdPartialLoader *self, const gchar *token, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    (void)token; // Unused parameter

    // Enter dumpvars state - subsequent value changes will be at time 0
    _vcd_partial_enter_dumpvars_state(self);
}

static void _vcd_partial_handle_end(GwVcdPartialLoader *self, const gchar *token, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);
    (void)token; // Unused parameter

    // If we're in dumpvars state, exit it
    if (self->in_dumpvars_block) {
        _vcd_partial_exit_dumpvars_state(self);
    }
}

static void process_vcd_token(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Trim whitespace from token
    const gchar *trimmed_token = token;
    gsize trimmed_len = len;

    // Trim leading whitespace
    while (trimmed_len > 0 && g_ascii_isspace(*trimmed_token)) {
        trimmed_token++;
        trimmed_len--;
    }

    // Trim trailing whitespace
    while (trimmed_len > 0 && g_ascii_isspace(trimmed_token[trimmed_len - 1])) {
        trimmed_len--;
    }

    if (trimmed_len == 0) {
        return; // Empty token after trimming
    }

    // Dispatch based on token type
    if (g_str_has_prefix(trimmed_token, "$timescale")) {
        _vcd_partial_handle_timescale(self, trimmed_token, trimmed_len, error);
    } else if (g_str_has_prefix(trimmed_token, "$timezero")) {
        _vcd_partial_handle_timezero(self, trimmed_token, trimmed_len, error);
    } else if (g_str_has_prefix(trimmed_token, "$scope")) {
        g_debug("Processing scope command: %.*s", (int)trimmed_len, trimmed_token);
        _vcd_partial_handle_scope(self, trimmed_token, trimmed_len, error);
    } else if (g_str_has_prefix(trimmed_token, "$var")) {
        _vcd_partial_handle_var(self, trimmed_token, trimmed_len, error);
    } else if (g_str_has_prefix(trimmed_token, "$upscope")) {
        // Handle upscope by popping scope from tree builder
        g_debug("Processing upscope command");
        gw_tree_builder_pop_scope(self->tree_builder);
    } else if (g_str_has_prefix(trimmed_token, "$enddefinitions")) {
        // Handle enddefinitions
        g_debug("Processing enddefinitions command");
        vcd_partial_parse_enddefinitions(self, error);
    } else if (g_str_has_prefix(trimmed_token, "$date")) {
        // Handle date - just ignore for now
        g_debug("Ignoring date command");
    } else if (g_str_has_prefix(trimmed_token, "$version")) {
        // Handle version - just ignore for now
        g_debug("Ignoring version command");
    } else if (g_str_has_prefix(trimmed_token, "$comment")) {
        // Handle comment - just ignore for now
        g_debug("Ignoring comment command");
    } else if (g_str_has_prefix(trimmed_token, "$dumpvars")) {
        _vcd_partial_handle_dumpvars(self, trimmed_token, error);
    } else if (g_str_has_prefix(trimmed_token, "$end")) {
        _vcd_partial_handle_end(self, trimmed_token, error);
    } else if (self->in_dumpvars_block) {
        // Inside dumpvars block, treat all lines as value changes at time 0
        _vcd_partial_handle_value_change(self, trimmed_token, trimmed_len, error);
    } else if (trimmed_token[0] == '#') {
        _vcd_partial_handle_time_change(self, trimmed_token, trimmed_len, error);
    } else {
        _vcd_partial_handle_value_change(self, trimmed_token, trimmed_len, error);
    }
}

static void _vcd_partial_handle_timescale(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Extract timescale value and unit from token like "$timescale 1ns $end"
    int time_val = 0;
    char time_unit[16] = {0};

    // Use sscanf to parse the timescale - look for the actual content between $timescale and $end
    const char *content_start = strstr(token, "$timescale");
    if (content_start) {
        content_start += strlen("$timescale");
        const char *content_end = strstr(content_start, "$end");
        if (content_end) {
            // Extract the content between $timescale and $end
            gsize content_len = content_end - content_start;
            char *content = g_strndup(content_start, content_len);
            char *trimmed_content = g_strstrip(content);

            // Apply fractional timescale fix if needed
            char *timescale_content = g_strdup(trimmed_content);
            fractional_timescale_fix(timescale_content);

            if (sscanf(timescale_content, "%d%s", &time_val, time_unit) == 2) {
        // Parse the time unit
        GwTimeDimension dimension = GW_TIME_DIMENSION_NANO;

        if (g_str_has_prefix(time_unit, "s")) {
            dimension = GW_TIME_DIMENSION_BASE;
        } else if (g_str_has_prefix(time_unit, "ms")) {
            dimension = GW_TIME_DIMENSION_MILLI;
        } else if (g_str_has_prefix(time_unit, "us")) {
            dimension = GW_TIME_DIMENSION_MICRO;
        } else if (g_str_has_prefix(time_unit, "ns")) {
            dimension = GW_TIME_DIMENSION_NANO;
        } else if (g_str_has_prefix(time_unit, "ps")) {
            dimension = GW_TIME_DIMENSION_PICO;
        } else if (g_str_has_prefix(time_unit, "fs")) {
            dimension = GW_TIME_DIMENSION_FEMTO;
        } else {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Unknown time unit: %s", time_unit);
            return;
        }

        // Update loader state
        self->time_scale = time_val;
        self->time_dimension = dimension;

        // Timescale is stored in loader state for later use


            } else {
                g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                           "Failed to parse timescale content: %s", trimmed_content);
                g_free(content);
                return;
            }
            g_free(timescale_content);
            g_free(content);
        } else {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Missing $end in timescale command");
            return;
        }
    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Failed to parse timescale: %.*s", (int)len, token);
    }
}

static void _vcd_partial_handle_timezero(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Extract timezero value from token like "$timezero -2 $end"
    GwTime timezero_value = 0;

    // Use sscanf to parse the timezero - look for the actual content between $timezero and $end
    const char *content_start = strstr(token, "$timezero");
    if (content_start) {
        content_start += strlen("$timezero");
        const char *content_end = strstr(content_start, "$end");
        if (content_end) {
            // Extract the content between $timezero and $end
            gsize content_len = content_end - content_start;
            char *content = g_strndup(content_start, content_len);
            char *trimmed_content = g_strstrip(content);

            if (sscanf(trimmed_content, "%" "ld" "", &timezero_value) == 1) {
                // Apply timescale scaling to the timezero value
                self->global_time_offset = timezero_value * self->time_scale;
            } else {
                g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                           "Failed to parse timezero content: %s", trimmed_content);
                g_free(content);
                return;
            }
            g_free(content);
        } else {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Missing $end in timezero command");
            return;
        }
    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Failed to parse timezero: %.*s", (int)len, token);
    }
}

static void _vcd_partial_handle_scope(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Parse scope type and name from token like "$scope module test $end"
    char scope_type[32] = {0};
    char scope_name[256] = {0};

    // Extract content between $scope and $end
    const char *content_start = strstr(token, "$scope");
    if (content_start) {
        content_start += strlen("$scope");
        const char *content_end = strstr(content_start, "$end");
        if (content_end) {
            gsize content_len = content_end - content_start;
            char *content = g_strndup(content_start, content_len);
            char *trimmed_content = g_strstrip(content);

            // Manual parsing instead of sscanf
            char *space = strchr(trimmed_content, ' ');
            if (!space) {
                g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                           "Failed to parse scope content: %s", trimmed_content);
                g_free(content);
                return;
            }
            gsize type_len = space - trimmed_content;
            if (type_len >= sizeof(scope_type)) {
                type_len = sizeof(scope_type) - 1;
            }
            g_strlcpy(scope_type, trimmed_content, type_len + 1);
            g_strlcpy(scope_name, g_strstrip(space + 1), sizeof(scope_name));
            g_free(content);
        } else {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Missing $end in scope command");
            return;
        }
    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Failed to parse scope: %.*s", (int)len, token);
        return;
    }

    g_debug("Parsing scope: type='%s', name='%s'", scope_type, scope_name);

    // Map scope type to tree kind
    unsigned char ttype = GW_TREE_KIND_UNKNOWN;

    if (strcmp(scope_type, "module") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_MODULE;
    } else if (strcmp(scope_type, "task") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_TASK;
    } else if (strcmp(scope_type, "function") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_FUNCTION;
    } else if (strcmp(scope_type, "fork") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_FORK;
    } else if (strcmp(scope_type, "begin") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_BEGIN;
    } else if (strcmp(scope_type, "struct") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_STRUCT;
    } else if (strcmp(scope_type, "union") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_UNION;
    } else if (strcmp(scope_type, "class") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_CLASS;
    } else if (strcmp(scope_type, "interface") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_INTERFACE;
    } else if (strcmp(scope_type, "program") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_PROGRAM;
    } else if (strcmp(scope_type, "package") == 0) {
        ttype = GW_TREE_KIND_VCD_ST_PACKAGE;
    } else if (g_str_has_prefix(scope_type, "vhdl_")) {
        // Handle VHDL scope types
        if (g_str_has_prefix(scope_type + 5, "architecture")) {
            ttype = GW_TREE_KIND_VHDL_ST_ARCHITECTURE;
        } else if (g_str_has_prefix(scope_type + 5, "record")) {
            ttype = GW_TREE_KIND_VHDL_ST_RECORD;
        } else if (g_str_has_prefix(scope_type + 5, "block")) {
            ttype = GW_TREE_KIND_VHDL_ST_BLOCK;
        } else if (g_str_has_prefix(scope_type + 5, "generate")) {
            ttype = GW_TREE_KIND_VHDL_ST_GENERATE;
        } else if (g_str_has_prefix(scope_type + 5, "process")) {
            ttype = GW_TREE_KIND_VHDL_ST_PROCESS;
        } else if (g_str_has_prefix(scope_type + 5, "procedure")) {
            ttype = GW_TREE_KIND_VHDL_ST_PROCEDURE;
        } else if (g_str_has_prefix(scope_type + 5, "function")) {
            ttype = GW_TREE_KIND_VHDL_ST_FUNCTION;
        }
    }

    // Push the scope to the tree builder
    g_debug("Pushing scope: %s (type: %d, tree builder: %p)", scope_name, ttype, self->tree_builder);
    GwTreeNode *scope = gw_tree_builder_push_scope(self->tree_builder, ttype, scope_name);
    if (scope) {
        g_debug("Scope pushed successfully: %p", scope);
    } else {
        g_debug("Failed to push scope");
    }
    if (scope) {
        scope->t_which = -1;

    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Failed to parse scope: %.*s", (int)len, token);
    }
}

static void _vcd_partial_handle_upscope(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    g_debug("Popping scope (tree builder: %p)", self->tree_builder);
    gw_tree_builder_pop_scope(self->tree_builder);
}

static void _vcd_partial_handle_var(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Parse variable definition from token like "$var wire 1 ! signal_name $end"
    char var_type[32] = {0};
    char var_size_str[32] = {0};
    char var_id[32] = {0};
    char var_name[256] = {0};

    // Extract content between $var and $end
    const char *content_start = strstr(token, "$var");
    if (content_start) {
        content_start += strlen("$var");
        const char *content_end = strstr(content_start, "$end");
        if (content_end) {
            gsize content_len = content_end - content_start;
            char *content = g_strndup(content_start, content_len);
            char *trimmed_content = g_strstrip(content);

            if (sscanf(trimmed_content, "%31s %31s %31s %255s", var_type, var_size_str, var_id, var_name) < 4) {
                g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                           "Failed to parse variable content: %s", trimmed_content);
                g_free(content);
                return;
            }
            g_free(content);
        } else {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Missing $end in var command");
            return;
        }
    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Failed to parse variable: %.*s", (int)len, token);
        return;
    }

    // Parse variable size
    int var_size = atoi(var_size_str);

    // Map variable type to internal representation
    unsigned char vartype = 0;
    if (g_str_has_prefix(var_type, "wire")) {
        vartype = V_WIRE;
    } else if (g_str_has_prefix(var_type, "reg")) {
        vartype = V_REG;
    } else if (g_str_has_prefix(var_type, "integer")) {
        vartype = V_INTEGER;
    } else if (g_str_has_prefix(var_type, "real")) {
        vartype = V_REAL;
    } else if (g_str_has_prefix(var_type, "event")) {
        vartype = V_EVENT;
    } else if (g_str_has_prefix(var_type, "parameter")) {
        vartype = V_PARAMETER;
    } else if (g_str_has_prefix(var_type, "string")) {
        vartype = V_STRINGTYPE;
    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Unknown variable type: %s", var_type);
        return;
    }

    // Validate variable size - allow size 0 for string variables
    if (var_size <= 0 && vartype != V_STRINGTYPE) {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Invalid variable size: %s", var_size_str);
        return;
    }

    // Create vcdsymbol structure
    struct vcdsymbol *v = g_new0(struct vcdsymbol, 1);
    v->vartype = vartype;
    v->size = var_size;

    // Debug output for string variables
    if (vartype == V_STRINGTYPE) {
      //  g_debug("Creating string variable: name=%s, id=%s, size=%d, vartype=%d", var_name, var_id, var_size, vartype);
    }
    if (vartype == V_REAL || vartype == V_STRINGTYPE) {
        v->msi = 0;
        v->lsi = 0;
    } else {
        v->msi = var_size - 1;
        v->lsi = 0;
    }
    v->id = g_strdup(var_id);
    v->nid = vcdid_hash(var_id, strlen(var_id));

    // Update min/max IDs for efficient symbol lookup
    if (v->nid < self->vcd_minid) {
        //g_test_message("Updating vcd_minid: %u -> %u", self->vcd_minid, v->nid);
        self->vcd_minid = v->nid;
    }
    if (v->nid > self->vcd_maxid) {
       // g_test_message("Updating vcd_maxid: %u -> %u", self->vcd_maxid, v->nid);
        self->vcd_maxid = v->nid;
    }

    // Generate the full hierarchical name NOW, while the tree_builder scope is correct.
    if ((vartype != V_REAL) && (vartype != V_STRINGTYPE) && (vartype != V_INTEGER) && var_size > 1) {
        v->name = gw_tree_builder_get_symbol_name_with_two_indices(self->tree_builder, var_name, v->msi, v->lsi);
    } else {
        v->name = gw_tree_builder_get_symbol_name(self->tree_builder, var_name);
    }

    // Add to symbol list
    if (self->vcdsymroot == NULL) {
        self->vcdsymroot = self->vcdsymcurr = v;
    } else {
        self->vcdsymcurr->next = v;
        self->vcdsymcurr = v;
    }
    self->numsyms++;

    // Create initial node for the variable
    v->narray = g_new0(GwNode *, 1);
    v->narray[0] = g_new0(GwNode, 1);
    v->narray[0]->nname = g_strdup(v->name); // The node also needs the full name

    // Initialize the embedded head entry (decorator) with time=-2
    v->narray[0]->head.time = -2;
    v->narray[0]->head.v.h_val = GW_BIT_X;

    // Create and link both t=-2 and t=-1 entries as separate GwHistEnt structures
    GwHistEnt *h_minus_2 = g_new0(GwHistEnt, 1);
    h_minus_2->time = -2;
    h_minus_2->next = NULL;

    GwHistEnt *h_minus_1 = g_new0(GwHistEnt, 1);
    h_minus_1->time = -1;
    h_minus_1->next = NULL;

    // Set appropriate decorator values for t=-2 based on signal type
    if (v->vartype == V_REAL) {
        h_minus_2->flags = GW_HIST_ENT_FLAG_REAL;
        // For t=-2, use a specific NAN pattern that displays as 'x' instead of 'nan'
        // Create a quiet NAN with a specific payload that gtkwave recognizes as 'x'
        union {
            double d;
            guint64 u;
        } nan_union;
        nan_union.u = 0x7ff8000000000001ULL; // Quiet NAN with specific payload
        h_minus_2->v.h_double = nan_union.d;
    } else if (v->vartype == V_STRINGTYPE) {
        h_minus_2->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
        h_minus_2->v.h_vector = g_strdup("x"); // Represents 'x' for t=-2
    } else {
        h_minus_2->v.h_val = GW_BIT_X; // Represents 'x' for scalars and vectors
    }

    // Set appropriate decorator values for t=-1 based on signal type
    if (v->vartype == V_REAL) {
        h_minus_1->flags = GW_HIST_ENT_FLAG_REAL;
        // For t=-1, use a regular NAN that displays as 'nan'
        // Use union to avoid strict-aliasing warning
        union {
            double d;
            guint64 u;
        } nan_union;
        nan_union.d = 0.0 / 0.0; // Represents 'nan' (positive NAN)
        // Clear the sign bit to ensure positive NAN
        nan_union.u &= ~(1ULL << 63);
        h_minus_1->v.h_double = nan_union.d;
    } else if (v->vartype == V_STRINGTYPE) {
        h_minus_1->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
        h_minus_1->v.h_vector = g_strdup("?"); // Represents '?' for t=-1
    } else {
        h_minus_1->v.h_val = GW_BIT_X; // Represents 'x' for scalars and vectors
    }

    // Link the placeholders in the history chain
    h_minus_2->next = h_minus_1;
    v->narray[0]->head.next = h_minus_2; // Start chain with t=-2
    v->narray[0]->curr = h_minus_1; // CRITICAL: curr must point to t=-1 entry
    v->narray[0]->numhist = 0; // Start with 0 entries (placeholders are not counted in numhist)
    v->narray[0]->last_time = -1; // Initialize last_time for delta calculations
    v->narray[0]->last_time_raw = -1; // Initialize last_time_raw for delta calculations

    // Set node metadata from vcdsymbol
    set_vcd_vartype(v, v->narray[0]);
    v->narray[0]->vardt = GW_VAR_DATA_TYPE_NONE;
    v->narray[0]->vardir = GW_VAR_DIR_IMPLICIT;
    v->narray[0]->varxt = 0;
    v->narray[0]->extvals = (v->vartype == V_REAL || v->vartype == V_STRINGTYPE || v->size > 1) ? 1 : 0;
    v->narray[0]->msi = v->msi;
    v->narray[0]->lsi = v->lsi;

    // Create symbol for the signal
    GwSymbol *symbol = g_new0(GwSymbol, 1);
    symbol->name = g_strdup(v->name);
    symbol->n = v->narray[0];
    symbol->vec_root = (GwSymbol *)v->root;

    // Store the link from vcdsymbol to GwSymbol for later name updates
    v->sym_chain = symbol;

    // Add to symbol chain for later processing
    self->sym_chain = g_slist_append(self->sym_chain, symbol);
    self->numfacs++;

    // Create vlist writer for this signal and store it in the hash table
    GwVlistWriter *writer = gw_vlist_writer_new(self->vlist_compression_level, self->vlist_prepack);
    gw_vlist_writer_set_live_mode(writer, TRUE);

    // Debug: log what type header we're writing
    g_test_message("Creating vlist writer for signal '%s': size=%d, vartype=%d, writing header type=%c",
                  v->name, v->size, v->vartype,
                  (v->size == 1 && v->vartype != V_REAL && v->vartype != V_STRINGTYPE) ? '0' :
                  ((v->vartype == V_REAL) ? 'R' : (v->vartype == V_STRINGTYPE) ? 'S' : 'B'));

    // Write the vlist header
    if (v->size == 1 && v->vartype != V_REAL && v->vartype != V_STRINGTYPE) {
        // Scalar header
        gw_vlist_writer_append_uv32(writer, (unsigned int)'0');
        gw_vlist_writer_append_uv32(writer, (unsigned int)v->vartype);
    } else {
        // Vector/Real/String header
        unsigned char typ2 = (v->vartype == V_REAL) ? 'R' : (v->vartype == V_STRINGTYPE) ? 'S' : 'B';
        gw_vlist_writer_append_uv32(writer, (unsigned int)typ2);
        gw_vlist_writer_append_uv32(writer, (unsigned int)v->vartype);
        gw_vlist_writer_append_uv32(writer, (unsigned int)v->size);
    }

    // Store writer in hash table
    g_hash_table_insert(self->vlist_writers, g_strdup(var_id), writer);

    // Store symbol properties for JIT import
    GwVcdSymbolProperties *props = g_new0(GwVcdSymbolProperties, 1);
    props->vartype = v->vartype;
    props->size = v->size;
    g_hash_table_insert(self->vlist_symbol_properties, g_strdup(var_id), props);

    // Initialize import position for this signal (vlist type will be set later)
    // Store as combined value with vlist_type=0 in upper 32 bits
    guint64 *combined_value = g_new(guint64, 1);
    *combined_value = ((guint64)0 << 32) | 0;
    g_hash_table_insert(self->vlist_import_positions, g_strdup(var_id), combined_value);
    //g_test_message("Initialized import position for symbol %s: combined_value=%lu", var_id, *combined_value);

}

static void _vcd_partial_handle_time_change(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Parse time value from token like "#100" or "#12345"
    GwTime time_value = 0;

    // Skip the '#' character and parse the time
    if (sscanf(token + 1, "%" "ld" "", &time_value) == 1) {
        // Store raw time value for delta calculations
        self->current_time_raw = time_value;
        // Apply timescale scaling to the time value and add global time offset
        self->current_time = (time_value * self->time_scale) + self->global_time_offset;

        // Update start and end time tracking
        if (self->start_time == -1 || time_value < self->start_time) {
            self->start_time = time_value;
        }
        if (time_value > self->end_time) {
            self->end_time = time_value;
        }

        // Add time to time_vlist for tracking
        // Time values are tracked by count, the actual time list is built when needed
        self->time_vlist_count++;

        g_debug("Time change parsed: time=%" GW_TIME_FORMAT ", scale=%" GW_TIME_FORMAT ", dimension=%d",
                time_value, self->time_scale, self->time_dimension);

    } else {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Failed to parse time change: %.*s", (int)len, token);
    }
}

static void _vcd_partial_handle_value_change(GwVcdPartialLoader *self, const gchar *token, gsize len, GError **error)
{
    g_assert(token != NULL);
    g_assert(error != NULL && *error == NULL);

    // Parse value change from token like "1!" or "b1010 #"
    if (len < 2) {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Invalid value change: %.*s", (int)len, token);
        return;
    }

    // Check if this is a value change inside a dumpvars section
    // If we're in dumpvars state, force time to 0 for this transition only
    GwTime original_time = self->current_time;
    if (self->in_dumpvars_block) {
        self->current_time = 0;
    }

    // Parse value change from token like "1!" or "b1010 #"
    if (len < 2) {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Invalid value change: %.*s", (int)len, token);
        return;
    }

    char value_char = token[0];
    char identifier[32] = {0};
    char value_buffer[256] = {0};

    // Handle different value formats
    if (value_char == 'b' || value_char == 'B' || value_char == 'r' || value_char == 'R' || value_char == 's' || value_char == 'S') {
        // Binary, real, or string value: format is "bVALUE IDENTIFIER", "rVALUE IDENTIFIER", or "sVALUE IDENTIFIER"
        // Find the space separating value from identifier
        const char *space_pos = strchr(token + 1, ' ');
        if (space_pos != NULL) {
            // Extract value (everything between first char and space)
            gsize value_len = space_pos - (token + 1);
            g_strlcpy(value_buffer, token + 1, MIN(value_len + 1, sizeof(value_buffer)));

            // Extract identifier (everything after space, excluding newline)
            const char *id_start = space_pos + 1;
            gsize id_len = 0;
            for (gsize i = 0; id_start[i] != '\0' && id_start[i] != '\n' && id_start[i] != '\r'; i++) {
                if (i < sizeof(identifier) - 1) {
                    identifier[i] = id_start[i];
                    id_len++;
                }
            }
            identifier[id_len] = '\0';
        } else {
            // No space found, treat everything after value_char as identifier (fallback)
            gsize id_len = 0;
            for (gsize i = 1; i < len && i < sizeof(identifier); i++) {
                if (token[i] == '\n' || token[i] == '\r' || token[i] == '\0') {
                    break;
                }
                identifier[id_len++] = token[i];
            }
            identifier[id_len] = '\0';
        }
    } else {
        // Scalar value: format is "VALUEIDENTIFIER" (e.g., "1!")
        // Extract identifier (everything after the value character, excluding newline and VCD identifier chars)
        gsize id_len = 0;
        for (gsize i = 1; i < len; i++) {
            if (token[i] == '\n' || token[i] == '\r' || token[i] == '\0' || token[i] == '%') {
                break;
            }
            id_len++;
        }
        g_strlcpy(identifier, token + 1, MIN(id_len + 1, sizeof(identifier)));
    }



    // Look up symbol by identifier
    struct vcdsymbol *symbol = NULL;
    struct vcdsymbol *curr = self->vcdsymroot;
    while (curr != NULL) {
        if (g_strcmp0(curr->id, identifier) == 0) {
            symbol = curr;
            break;
        }
        curr = curr->next;
    }

    if (symbol == NULL) {
        // Check if identifier is empty (just newline)
        if (identifier[0] == '\0') {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Empty symbol identifier in value change: '%.*s'", (int)len, token);
        } else {
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Unknown symbol identifier: '%s' in token: '%.*s'", identifier, (int)len, token);
        }
        return;
    }

    // Get the vlist writer for this symbol from the hash table
    GwVlistWriter *writer = g_hash_table_lookup(self->vlist_writers, identifier);
    if (writer == NULL) {
        g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                   "Could not find vlist writer for symbol ID: %s", identifier);
        return;
    }

    // Calculate time delta from last change for this signal (counter-based for vlist)
    g_assert(symbol != NULL);
    g_assert(symbol->narray != NULL);
    g_assert(symbol->narray[0] != NULL);

    // Calculate time delta from last change for this signal
    // Use raw time values (without global offset) for delta calculation to avoid overflow
    unsigned int time_delta;
    if (symbol->narray[0]->last_time_raw == -1) {
        // First transition for this signal: time_delta = current_time_raw - (-1)
        time_delta = (unsigned int)(self->current_time_raw - symbol->narray[0]->last_time_raw);
    } else {
        time_delta = (unsigned int)(self->current_time_raw - symbol->narray[0]->last_time_raw);
    }

    // Store original last_time for debug message before updating
    GwTime original_last_time = symbol->narray[0]->last_time;
    GwTime original_last_time_raw = symbol->narray[0]->last_time_raw;

    // Update the node's time tracking (both raw and scaled)
    symbol->narray[0]->last_time = self->current_time;
    symbol->narray[0]->last_time_raw = self->current_time_raw;

    g_test_message("Processing value change for symbol %s (id: %s), time: %" GW_TIME_FORMAT ", last_time: %" GW_TIME_FORMAT ", time_delta: %u, value: %c, writer=%p, vartype=%d, size=%d",
            symbol->name, identifier, self->current_time, original_last_time, time_delta, value_char, writer, symbol->vartype, symbol->size);







    // Handle different value types
    switch (value_char) {
        case '0':
            if (writer != NULL) {
                guint32 accum = (time_delta << 2) | (0 << 1);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'l':
        case 'L':
            if (writer != NULL) {
                guint32 accum = RCV_L | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case '1':
            if (writer != NULL) {
                guint32 accum = (time_delta << 2) | (1 << 1);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'u':
        case 'U':
            if (writer != NULL) {
                guint32 accum = RCV_U | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'w':
        case 'W':
            if (writer != NULL) {
                guint32 accum = RCV_W | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'x':
        case 'X':
            if (writer != NULL) {
                guint32 accum = RCV_X | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case '-':
            if (writer != NULL) {
                guint32 accum = RCV_D | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'h':
        case 'H':
            if (writer != NULL) {
                guint32 accum = RCV_H | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'z':
        case 'Z':
            if (writer != NULL) {
                guint32 accum = RCV_Z | (time_delta << 4);
                g_test_message("WRITING to scalar VList for %s: time_delta=%u, accum=0x%x", identifier, time_delta, accum);
                gw_vlist_writer_append_uv32(writer, accum);
            }
            // Restore original time after processing value change (if we modified it for dumpvars)
            if (self->in_dumpvars_block) {
                self->current_time = original_time;
            }
            break;
        case 'b':
        case 'B':
        case 'r':
        case 'R':
        case 's':
        case 'S':
            // Handle binary, real, and string values
            {
                gchar *vector = g_alloca(len + 1); /* +1 for null terminator */
                gint vlen;

                if (value_char == 'b' || value_char == 'B' || value_char == 'r' || value_char == 'R') {
                    // For binary and real values, use the extracted value
                    if (value_buffer[0] != '\0') {
                        strcpy(vector, value_buffer);
                        vlen = strlen(value_buffer);
                    } else {
                        // Fallback: use everything after value_char, but limit to available space
                        g_strlcpy(vector, token + 1, len + 1);
                        vlen = strlen(vector);
                    }

                    // Restore original time after processing value change (if we modified it for dumpvars)
                    if (self->in_dumpvars_block) {
                        self->current_time = original_time;
                    }
                } else {
                    // For string values, extract the value part (before VCD identifier)
                    g_debug("Processing string value: token=%s, len=%zu", token, len);
                    const char *value_start = token + 1;
                    const char *id_start = strchr(value_start, ' ');
                    if (id_start != NULL) {
                        // String value is before the space, identifier after
                        gsize value_len = id_start - value_start;
                        g_strlcpy(vector, value_start, MIN(value_len + 1, len + 1));
                    } else {
                        // No space found, use everything after 's' but before newline/%
                        gsize value_len = 0;
                        for (gsize i = 1; i < len; i++) {
                            if (token[i] == '\n' || token[i] == '\r' || token[i] == '%') {
                                break;
                            }
                            value_len++;
                        }
                        g_strlcpy(vector, value_start, MIN(value_len + 1, len + 1));
                    }
                    vlen = strlen(vector);
                }

                g_test_message("Calling process_binary_stream for %s (id: %s) with vector=%s, writer=%p",
                              symbol->name, identifier, vector, writer);
                process_binary_stream(self, writer, vector, vlen, symbol, time_delta, error);
            }
            break;
        default:
            g_set_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN,
                       "Unknown value type: %c", value_char);
    }


}




static void _gw_vcd_partial_loader_parse_buffer(GwVcdPartialLoader *self, GError **error)
{
    g_assert(error != NULL && *error == NULL);

    const gchar *buffer = self->internal_buffer->str;
    gsize buffer_len = self->internal_buffer->len;
    gsize pos = 0;

    while (pos < buffer_len) {
        // Skip leading whitespace
        while (pos < buffer_len && g_ascii_isspace(buffer[pos])) {
            pos++;
        }
        if (pos >= buffer_len) break;

        gsize token_start = pos;
        gssize token_end_pos = -1;

        // Check if this is a VCD command starting with '$'
        if (buffer[pos] == '$') {
            // Special handling for $dumpvars block - treat each line separately
            if (g_str_has_prefix(buffer + pos, "$dumpvars")) {
                // For $dumpvars, process it as a single token, then handle the block contents line by line
                const char *newline = memchr(buffer + pos, '\n', buffer_len - pos);
                if (newline == NULL) {
                    // No newline found - incomplete line, wait for more data
                    break;
                }
                token_end_pos = (newline - buffer) + 1;
            } else {
                // Look for complete $command...$end block for other commands
                const char *end_marker = NULL;
                const char *search_ptr = buffer + pos;

                // Search for "$end" with proper termination
                while ((search_ptr = strstr(search_ptr, "$end")) != NULL) {
                    // Check if "$end" is properly terminated (whitespace, newline, or end of buffer)
                    gsize end_pos = search_ptr - buffer;
                    if (end_pos + 4 >= buffer_len ||
                        g_ascii_isspace(buffer[end_pos + 4]) ||
                        buffer[end_pos + 4] == '\0') {
                        end_marker = search_ptr;
                        token_end_pos = end_pos + 4;
                        break;
                    }
                    search_ptr += 4; // Continue searching
                }

                if (end_marker == NULL) {
                    // Incomplete $ command, wait for more data
                    break;
                }
            }
        } else {
            // Regular line - find the next newline
            const char *newline = memchr(buffer + pos, '\n', buffer_len - pos);
            if (newline == NULL) {
                // No newline found - incomplete line, wait for more data
                break;
            }
            token_end_pos = (newline - buffer) + 1;
        }

        if (token_end_pos == -1 || (gsize)token_end_pos > buffer_len) {
            // Incomplete token, wait for more data
            break;
        }

        // Process the complete token
        gsize token_len = token_end_pos - token_start;
        process_vcd_token(self, buffer + token_start, token_len, error);

        if (*error != NULL) {
            // Stop parsing on error
            self->processed_offset = token_end_pos;
            return;
        }

        pos = token_end_pos;
    }

    self->processed_offset = pos;
}

GwVcdPartialLoader *gw_vcd_partial_loader_new(void)
{
    return g_object_new(GW_TYPE_VCD_PARTIAL_LOADER, NULL);
}

/* NEW: Feed function implementation */
gboolean gw_vcd_partial_loader_feed(GwVcdPartialLoader *self, const gchar *data, gsize len, GError **error)
{
    g_return_val_if_fail(GW_IS_VCD_PARTIAL_LOADER(self), FALSE);
    g_return_val_if_fail(data != NULL, FALSE);

    /* Append new data to our internal buffer */
    g_string_append_len(self->internal_buffer, data, len);

    /* Call the parser to process as much of the buffer as it can */
    _gw_vcd_partial_loader_parse_buffer(self, error);

    /* Clean up processed data from buffer to save memory */
    if (self->processed_offset > 0) {
        g_string_erase(self->internal_buffer, 0, self->processed_offset);
        self->processed_offset = 0;
    }

    /* Check for errors */
    if (error != NULL && *error != NULL) {
        return FALSE;
    }

    return TRUE;
}


GwDumpFile *gw_vcd_partial_loader_get_dump_file(GwVcdPartialLoader *self)
{
    g_return_val_if_fail(GW_IS_VCD_PARTIAL_LOADER(self), NULL);

    /* Build tree and facs from current state */
    if (self->vcdsymroot == NULL) {
        return NULL; // No signals defined yet
    }

    // Detect streaming mode (no $enddefinitions but header is complete)
    g_test_message("Checking streaming mode: header_over=%d, time_scale=%" GW_TIME_FORMAT ", numfacs=%u, current_scope=%p",
                   self->header_over, self->time_scale, self->numfacs, gw_tree_builder_get_current_scope(self->tree_builder));
    if (!self->is_streaming_mode) {
        // Check if we have all components of a complete header
        if (self->time_scale != 0 && self->numfacs > 0 &&
            gw_tree_builder_get_current_scope(self->tree_builder) == NULL) {
            self->is_streaming_mode = TRUE;
            g_test_message("Detected streaming mode - enabling state preservation");
            // Initialize for streaming mode
            _gw_vcd_partial_loader_initialize_streaming_mode(self);
        }
    }

    // Handle streaming mode initialization when $enddefinitions is not present
    if (!self->is_fully_initialized && self->is_streaming_mode) {
        _gw_vcd_partial_loader_initialize_streaming_mode(self);
    }


    // --- CRITICAL POST-PROCESSING STEPS ---
    // These steps are essential for creating properly named and searchable symbols

    // Check if initialization is complete
    if (!self->is_fully_initialized) {
        return NULL; // Can't get a file until initialization is complete
    }

    // Get the facs from the existing dump file


    // Perform Just-in-Time Partial Import for each signal.
    // Get list of symbol IDs first to avoid iteration issues
    GList *symbol_ids = g_hash_table_get_keys(self->vlist_symbol_properties);
    GList *iter;




    for (iter = symbol_ids; iter != NULL; iter = iter->next) {
        const gchar *symbol_id = (const gchar *)iter->data;
        GwVlistWriter *writer = g_hash_table_lookup(self->vlist_writers, symbol_id);

        // Get symbol properties from stored hash table
        GwVcdSymbolProperties *props = g_hash_table_lookup(self->vlist_symbol_properties, symbol_id);

        // Find the corresponding GwSymbol using the fast, indexed search function
        GwSymbol *fac_symbol = NULL;
        struct vcdsymbol *vcd_sym = bsearch_vcd(self, (char *)symbol_id, strlen(symbol_id));

        if (vcd_sym && vcd_sym->sym_chain) {
            fac_symbol = vcd_sym->sym_chain;
        } else {
        }

        if (fac_symbol && fac_symbol->n) {
            GwNode *node = fac_symbol->n;

            // Get the current import position and vlist type for this signal
            guint64 *combined_value_ptr = g_hash_table_lookup(self->vlist_import_positions, symbol_id);
            if (combined_value_ptr == NULL) {
                continue;
            }
            guint64 combined_value = *combined_value_ptr;
            gsize last_pos = combined_value & 0xFFFFFFFF;
            guint32 vlist_type = (combined_value >> 32) & 0xFFFFFFFF;
            GwVlist *vlist = gw_vlist_writer_get_vlist(writer);

            // If new data has been written, process it
            guint vlist_total_size = gw_vlist_size(vlist);
            if (vlist && vlist_total_size > last_pos) {
                g_test_message("IMPORT: Processing %s, last_pos=%zu, vlist_size=%u", symbol_id, last_pos, vlist_total_size);

                GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
                gw_vlist_reader_set_position(reader, last_pos);

                // Get the last absolute time from the node's history, or use -1 if not found
                GwTime current_time = -1;
                if (node->curr != NULL) {
                    current_time = node->curr->time;
                }

                // Process header if this is the first read
                if (last_pos == 0 && vlist->size >= 1) {
                    vlist_type = gw_vlist_reader_read_uv32(reader);
                    g_test_message("JIT IMPORT: First read for %s, vlist_type from vlist=%c (0x%02x)",
                                  symbol_id, (char)vlist_type, vlist_type);
                    if (vlist_type == '0') {
                        gw_vlist_reader_read_uv32(reader); // Skip vartype
                    } else {
                        gw_vlist_reader_read_uv32(reader); // Skip vartype
                        gw_vlist_reader_read_uv32(reader); // Skip size
                    }
                    // Store the vlist type for future reads
                    guint64 *combined_value_ptr = g_new(guint64, 1);
                    *combined_value_ptr = ((guint64)vlist_type << 32) | 0;
                    g_hash_table_insert(self->vlist_import_positions, g_strdup(symbol_id), combined_value_ptr);
                } else {
                    // For subsequent reads, we need to know the vlist type
                    // It's stored in the upper 32 bits of the import position
                    vlist_type = (combined_value >> 32) & 0xFFFFFFFF;
                    g_test_message("JIT IMPORT: Subsequent read for %s, vlist_type=%c (0x%02x)",
                                  symbol_id, (char)vlist_type, vlist_type);
                }

                // Process value changes
                gboolean harray_needs_invalidation = FALSE;
                if (vlist_type == '0') {
                    // Scalar value processing
                    static const GwBit EXTRA_VALUES[] = { GW_BIT_X, GW_BIT_Z, GW_BIT_H, GW_BIT_U, GW_BIT_W, GW_BIT_L, GW_BIT_DASH, GW_BIT_X };
                    while (!gw_vlist_reader_is_done(reader)) {
                        guint32 accum = gw_vlist_reader_read_uv32(reader);
                        GwBit bit;
                        guint32 time_delta;

                        if (!(accum & 1)) {
                            time_delta = accum >> 2;
                            bit = accum & 2 ? GW_BIT_1 : GW_BIT_0;
                        } else {
                            time_delta = accum >> 4;
                            guint index = (accum >> 1) & 7;
                            bit = EXTRA_VALUES[index];
                        }

                        current_time += time_delta;

                        GwHistEnt *hent = g_new0(GwHistEnt, 1);
                        hent->time = current_time;
                        hent->v.h_val = bit;

                        // The node already has t=-2 and t=-1 placeholders, so we need to
                        // insert after the current last entry (which is at t=-1)
                        node->curr->next = hent;
                        node->curr = hent;
                        node->numhist++; // Increment numhist for each new transition
                        harray_needs_invalidation = TRUE;
                    }
                } else if (vlist_type == 'B' || vlist_type == 'R' || vlist_type == 'S') {
                    // Vector, Real, and String value processing - all use string format

                    while (!gw_vlist_reader_is_done(reader)) {
                        guint32 time_delta = gw_vlist_reader_read_uv32(reader);
                        current_time += time_delta;

                        const gchar *value_str = gw_vlist_reader_read_string(reader);

                        GwHistEnt *hent = g_new0(GwHistEnt, 1);
                        hent->time = current_time;

                        if (vlist_type == 'R') {
                            // Real value
                            hent->flags = GW_HIST_ENT_FLAG_REAL;
                            hent->v.h_double = g_ascii_strtod(value_str, NULL);
                        } else if (vlist_type == 'S') {
                            // String value
                            hent->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
                            hent->v.h_vector = (char *)g_strdup(value_str);
                        } else {
                            // Vector value (type 'B')
                            g_test_message("JIT IMPORT: Allocating h_vector for %s, size=%d, value_str='%s'",
                                          symbol_id, props ? props->size : -1, value_str);
                            hent->v.h_vector = g_malloc(props->size);
                            gint val_len = strlen(value_str);
                            gint copy_len = MIN(val_len, props->size);
                            gint offset = (val_len > props->size) ? (val_len - props->size) : 0;
                            gint pad_len = props->size - copy_len;

                            // Pad with '0' on the left (MSB)
                            for (gint i = 0; i < pad_len; i++) {
                                hent->v.h_vector[i] = GW_BIT_0;
                            }

                            // Copy the actual value
                            for (gint i = 0; i < copy_len; i++) {
                                hent->v.h_vector[pad_len + i] = gw_bit_from_char(value_str[i + offset]);
                            }
                            g_test_message("JIT IMPORT: Allocated h_vector=%p for %s",
                                          hent->v.h_vector, symbol_id);
                        }

                        // value_str is managed by the reader, do not free

                        // The node already has t=-2 and t=-1 placeholders, so we need to
                        // insert after the current last entry (which is at t=-1)
                        node->curr->next = hent;
                        node->curr = hent;
                        node->numhist++; // Increment numhist for each new transition
                        harray_needs_invalidation = TRUE;
                    }
                } else if (vlist_type == 'R' || vlist_type == 'S') {
                    // Real/String value processing - use gw_vlist_reader_read_string()
                    while (!gw_vlist_reader_is_done(reader)) {
                        guint32 time_delta = gw_vlist_reader_read_uv32(reader);
                        const gchar *value_str = gw_vlist_reader_read_string(reader);

                        current_time += time_delta;

                        GwHistEnt *hent = g_new0(GwHistEnt, 1);
                        hent->time = current_time;

                        if (vlist_type == 'R') {
                            hent->flags = GW_HIST_ENT_FLAG_REAL;
                            hent->v.h_double = g_strtod(value_str, NULL);
                        } else { // vlist_type == 'S'
                            hent->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
                            hent->v.h_vector = g_strdup(value_str);
                        }

                        // The node already has t=-2 and t=-1 placeholders, so we need to
                        // insert after the current last entry (which is at t=-1)
                        node->curr->next = hent;
                        node->curr = hent;
                        node->numhist++; // Increment numhist for each new transition
                        harray_needs_invalidation = TRUE;
                    }
                } else {
                    g_test_message("JIT IMPORT: Unsupported vlist type '%c' for symbol %s", vlist_type, symbol_id);
                }

                // Invalidate harray once after adding all new entries for this signal
                // This ensures bsearch_node will rebuild it with the correct size on next access
                if (harray_needs_invalidation && node->harray) {
                    node->harray = NULL;

                    // If this node is an expanded vector, invalidate its children too.
                    // Use reference counting to ensure safe access to expand_info
                    if (node->expand_info) {
                        GwExpandInfo *einfo = node->expand_info;
                        
                        // Acquire a reference to prevent freeing while we're using it
                        gw_expand_info_acquire(einfo);
                        
                        // Only proceed if all these conditions are met:
                        // 1. einfo pointer is non-NULL
                        // 2. narray pointer is non-NULL
                        // 3. width is reasonable (1-1024)
                        // 4. All child pointers in narray are non-NULL
                        gboolean all_children_valid = TRUE;
                        if (einfo && einfo->narray && einfo->width > 0 && einfo->width <= 1024) {
                            // Check that all child pointers are non-NULL before proceeding
                            for (int i = 0; i < einfo->width; i++) {
                                if (!einfo->narray[i]) {
                                    all_children_valid = FALSE;
                                    break;
                                }
                            }
                            
                            if (all_children_valid) {
                                g_debug("Propagating harray invalidation from '%s' to %d children.", node->nname, einfo->width);
                                for (int i = 0; i < einfo->width; i++) {
                                    einfo->narray[i]->harray = NULL;
                                }
                            } else {
                                g_debug("Expand info for '%s' has NULL child pointers, skipping propagation", node->nname);
                            }
                        } else {
                            g_debug("Expand info for '%s' appears invalid (einfo=%p, narray=%p, width=%d), skipping propagation", 
                                   node->nname, einfo, einfo ? einfo->narray : NULL, einfo ? einfo->width : -1);
                        }
                        
                        // Release the reference
                        gw_expand_info_release(einfo);
                    }
                }

                // Store both the import position and vlist type for future reads
                // Use the current reader position instead of vlist size for accurate positioning
                guint64 *combined_value_ptr = g_new(guint64, 1);
                *combined_value_ptr = ((guint64)vlist_type << 32) | gw_vlist_reader_get_position(reader);
                g_hash_table_insert(self->vlist_import_positions, g_strdup(symbol_id), combined_value_ptr);

                // Store the last absolute time for this signal for the next import

                g_object_unref(reader);
            }else{

                g_test_message("JIT IMPORT: Signal '%s' (ID: %s) has NO new data in vlist. numhist is currently %d.",
                                           fac_symbol->n->nname, symbol_id, node->numhist);
            }
        }
    }



    // Update the time range of the persistent dump file
    if (self->end_time > self->start_time) {
        GwTimeRange *time_range = gw_time_range_new(self->start_time, self->end_time);
        gw_dump_file_set_time_range(GW_DUMP_FILE(self->dump_file), time_range);
        g_object_unref(time_range);
    } else if (self->start_time == -1 && self->end_time == -1) {
        // Header-only parsing - no time values yet, set default time range
        GwTimeRange *time_range = gw_time_range_new(0, 1000); // Default 0-1000 time units
        gw_dump_file_set_time_range(GW_DUMP_FILE(self->dump_file), time_range);
        g_object_unref(time_range);
        }

        // Clean up the symbol IDs list
    g_list_free(symbol_ids);

    // --- Just-in-Time Partial Import (Always Runs) ---
    // This block runs on every call to import new data
    GwFacs *facs = gw_dump_file_get_facs(GW_DUMP_FILE(self->dump_file));
        guint num_facs = gw_facs_get_length(facs);

        for (guint i = 0; i < num_facs; i++) {
            GwSymbol *sym = gw_facs_get(facs, i);
            GwNode *node = sym->n;

            // Find the corresponding vcdsymbol for the current GwSymbol to get its ID.
            // This is a slow linear search, but is acceptable for a fix-up pass.
            struct vcdsymbol *vcd_sym = NULL;
            struct vcdsymbol *iter_vcd = self->vcdsymroot;
            while(iter_vcd) {
                if (iter_vcd->sym_chain == sym) {
                    vcd_sym = iter_vcd;
                    break;
                }
                iter_vcd = iter_vcd->next;
            }

            if (vcd_sym) {
                // Use the ID to find the *primary* symbol for that ID.
                struct vcdsymbol *original_vcd_sym = bsearch_vcd(self, vcd_sym->id, strlen(vcd_sym->id));

                // If the primary symbol is different from the current one, we have an alias.
                if (original_vcd_sym && original_vcd_sym != vcd_sym) {

                    GwNode *original_node = original_vcd_sym->sym_chain->n;

                    // If the history counts don't match, the alias is out of sync. Fix it.
                    if (node->numhist != original_node->numhist) {
                        node->head = original_node->head;
                        node->curr = original_node->curr;
                        node->numhist = original_node->numhist;
                    }
                }
            }
        }
    // Update the time range of the persistent dump file




    /* Return the single dump file instance that's always kept up to date */
    return GW_DUMP_FILE(self->dump_file);


}

/**
 * gw_vcd_partial_loader_is_header_parsed:
 * @self: A #GwVcdPartialLoader.
 *
 * Checks if the VCD header has been fully parsed. This is useful for interactive
 * sessions where the header must be parsed before building the user interface.
 *
 * Returns: %TRUE if the header has been parsed, %FALSE otherwise.
 */
gboolean gw_vcd_partial_loader_is_header_parsed(GwVcdPartialLoader *self)
{
    g_return_val_if_fail(GW_IS_VCD_PARTIAL_LOADER(self), FALSE);

    return self->header_over;
}
