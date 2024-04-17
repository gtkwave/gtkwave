#include "gw-vcd-loader.h"
#include "gw-vcd-file.h"
#include "gw-vcd-file-private.h"
#include "gw-util.h"
#include "gw-hash.h"
#include "vcd-keywords.h"
#include <stdio.h>
#include <fstapi.h>

#define VCD_BSIZ 32768 /* size of getch() emulation buffer--this val should be ok */
#define VCD_INDEXSIZ (8 * 1024 * 1024)
// TODO: remove VCDNAM_ESCAPE
#define VCDNAM_ESCAPE 1
// TODO: remove!
#define WAVE_T_WHICH_UNDEFINED_COMPNAME (-1)
#define WAVE_DECOMPRESSOR "gzip -cd " /* zcat alone doesn't cut it for AIX */

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct vcdsymbol
{
    struct vcdsymbol *root, *chain;
    GwSymbol *sym_chain;

    struct vcdsymbol *next;
    char *name;
    char *id;
    char *value;
    GwNode **narray;
    GwHistEnt **tr_array; /* points to synthesized trailers (which can move) */
    GwHistEnt **app_array; /* points to hptr to append to (which can move) */

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

typedef struct
{
    gchar *name;
    GwTreeNode *mod_tree_parent;
} VcdScope;

struct _GwVcdLoader
{
    GwLoader parent_instance;

    FILE *vcd_handle;
    gboolean is_compressed;
    off_t vcd_fsiz;

    gboolean header_over;

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

    GQueue *scopes;
    GString *name_prefix;

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
    GwTreeNode *mod_tree_parent;

    char *module_tree;
    int module_len_tree;

    gboolean has_escaped_names;
    guint warning_filesize;
};

G_DEFINE_TYPE(GwVcdLoader, gw_vcd_loader, GW_TYPE_LOADER)

enum
{
    PROP_VLIST_PREPACK = 1,
    PROP_VLIST_COMPRESSION_LEVEL,
    PROP_WARNING_FILESIZE,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

// TODO: improve error handling
static void vcd_exit(int status)
{
    exit(status);

    // old implementation:
    // if (GLOBALS->vcd_jmp_buf) {
    //     splash_finalize();
    //     longjmp(*(GLOBALS->vcd_jmp_buf), x);
    // } else {
    //     exit(x);
    // }
}

/**/

static void malform_eof_fix(GwVcdLoader *self)
{
    if (feof(self->vcd_handle)) {
        memset(self->vcdbuf, ' ', VCD_BSIZ);
        self->vst = self->vend;
    }
}

/**/

#undef VCD_BSEARCH_IS_PERFECT /* bsearch is imperfect under linux, but OK under AIX */

static void vcd_build_symbols(GwVcdLoader *self);
static void vcd_cleanup(GwVcdLoader *self);
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
static struct vcdsymbol *bsearch_vcd(GwVcdLoader *self, char *key, int len)
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
static void create_sorted_table(GwVcdLoader *self)
{
    struct vcdsymbol *v;
    struct vcdsymbol **pnt;
    unsigned int vcd_distance;

    g_clear_pointer(&self->symbols_sorted, g_free);
    g_clear_pointer(&self->symbols_indexed, g_free);

    if (self->numsyms > 0) {
        vcd_distance = self->vcd_maxid - self->vcd_minid + 1;

        if ((vcd_distance <= VCD_INDEXSIZ) || !self->vcd_hash_kill) {
            self->symbols_indexed = g_new0(struct vcdsymbol *, vcd_distance);

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
            pnt = self->symbols_sorted = g_new0(struct vcdsymbol *, self->numsyms);
            v = self->vcdsymroot;
            while (v) {
                *(pnt++) = v;
                v = v->next;
            }

            qsort(self->symbols_sorted, self->numsyms, sizeof(struct vcdsymbol *), vcdsymcompare);
        }
    }
}

/*
 * add symbol to table.  no duplicate checking
 * is necessary as aet's are "correct."
 */
static GwSymbol *symadd(GwVcdLoader *self, char *name, int hv)
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

static unsigned int vlist_emit_finalize(GwVcdLoader *self)
{
    struct vcdsymbol *v /* , *vprime */; /* scan-build */
    int cnt = 0;

    v = self->vcdsymroot;
    while (v) {
        GwNode *n = v->narray[0];

        set_vcd_vartype(v, n);

        if (n->mv.mvlfac_vlist_writer == NULL) {
            GwVlistWriter *writer =
                gw_vlist_writer_new(self->vlist_compression_level, self->vlist_prepack);
            n->mv.mvlfac_vlist_writer = writer;

            if ((/* vprime= */ bsearch_vcd(self, v->id, strlen(v->id))) ==
                v) /* hash mish means dup net */ /* scan-build */
            {
                switch (v->vartype) {
                    case V_REAL:
                        gw_vlist_writer_append_uv32(writer, 'R');
                        gw_vlist_writer_append_uv32(writer, (unsigned int)v->vartype);
                        gw_vlist_writer_append_uv32(writer, (unsigned int)v->size);
                        gw_vlist_writer_append_uv32(writer, 0);
                        gw_vlist_writer_append_string(writer, "NaN");
                        break;

                    case V_STRINGTYPE:
                        gw_vlist_writer_append_uv32(writer, 'S');
                        gw_vlist_writer_append_uv32(writer, (unsigned int)v->vartype);
                        gw_vlist_writer_append_uv32(writer, (unsigned int)v->size);
                        gw_vlist_writer_append_uv32(writer, 0);
                        gw_vlist_writer_append_string(writer, "UNDEF");
                        break;

                    default:
                        if (v->size == 1) {
                            gw_vlist_writer_append_uv32(writer, (unsigned int)'0');
                            gw_vlist_writer_append_uv32(writer, (unsigned int)v->vartype);
                            gw_vlist_writer_append_uv32(writer, RCV_X);
                        } else {
                            gw_vlist_writer_append_uv32(writer, 'B');
                            gw_vlist_writer_append_uv32(writer, (unsigned int)v->vartype);
                            gw_vlist_writer_append_uv32(writer, (unsigned int)v->size);
                            gw_vlist_writer_append_uv32(writer, 0);
                            gw_vlist_writer_append_mvl9_string(writer, "x");
                        }
                        break;
                }
            }
        }

        GwVlistWriter *writer = n->mv.mvlfac_vlist_writer;
        n->mv.mvlfac_vlist = gw_vlist_writer_finish(writer);
        g_object_unref(writer);

        v = v->next;
        cnt++;
    }

    return (cnt);
}

/******************************************************************/

static void update_name_prefix(GwVcdLoader *self)
{
    g_string_truncate(self->name_prefix, 0);

    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

    for (GList *iter = self->scopes->head; iter != NULL; iter = iter->next) {
        VcdScope *scope = iter->data;

        if (self->name_prefix->len > 0) {
            g_string_append_c(self->name_prefix, delimiter);
        }

        g_string_append(self->name_prefix, scope->name);
    }
}

static void push_scope(GwVcdLoader *self, const gchar *name, GwTreeNode *tree_parent)
{
    VcdScope *scope = g_new0(VcdScope, 1);
    scope->name = g_strdup(self->yytext);
    scope->mod_tree_parent = tree_parent;

    g_queue_push_tail(self->scopes, scope);

    update_name_prefix(self);
}

static GwTreeNode *pop_scope(GwVcdLoader *self)
{
    VcdScope *scope = g_queue_pop_tail(self->scopes);
    g_assert_nonnull(scope);

    GwTreeNode *tree_parent = scope->mod_tree_parent;

    g_free(scope->name);
    g_free(scope);

    update_name_prefix(self);

    return tree_parent;
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(GwVcdLoader *self)
{
    self->vcdbuf = g_malloc0(VCD_BSIZ);
    self->vst = self->vcdbuf;
    self->vend = self->vcdbuf;
}

static void getch_free(GwVcdLoader *self)
{
    g_free(self->vcdbuf);
    self->vcdbuf = NULL;
    self->vst = NULL;
    self->vend = NULL;
}

static int getch_fetch(GwVcdLoader *self)
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
        // TODO: update splash
        // splash_sync(self->vcdbyteno, self->vcd_fsiz); /* gnome 2.18 seems to set errno so splash
        //                                                                     moved here... */
    }

    return ((int)(*self->vst));
}

static inline signed char getch(GwVcdLoader *self)
{
    signed char ch = (self->vst != self->vend) ? ((int)(*self->vst)) : (getch_fetch(self));
    self->vst++;
    return (ch);
}

static inline signed char getch_peek(GwVcdLoader *self)
{
    signed char ch = (self->vst != self->vend) ? ((int)(*self->vst)) : (getch_fetch(self));
    /* no increment */
    return (ch);
}

static int getch_patched(GwVcdLoader *self)
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
static int get_token(GwVcdLoader *self)
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

static int get_vartoken_patched(GwVcdLoader *self, int match_kw)
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

static int get_vartoken(GwVcdLoader *self, int match_kw)
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

static int get_strtoken(GwVcdLoader *self)
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

static void sync_end(GwVcdLoader *self, const char *hdr)
{
    int tok;

    if (hdr) {
        // DEBUG(fprintf(stderr, "%s", hdr));
    }
    for (;;) {
        tok = get_token(self);
        if ((tok == T_END) || (tok == T_EOF))
            break;
        // if (hdr) {
        //     DEBUG(fprintf(stderr, " %s", self->yytext));
        // }
    }
    // if (hdr) {
    //     DEBUG(fprintf(stderr, "\n"));
    // }
}

static int version_sync_end(GwVcdLoader *self, const char *hdr)
{
    int tok;
    int rc = 0;

    // if (hdr) {
    //     DEBUG(fprintf(stderr, "%s", hdr));
    // }
    for (;;) {
        tok = get_token(self);
        if ((tok == T_END) || (tok == T_EOF))
            break;
        // if (hdr) {
        //     DEBUG(fprintf(stderr, " %s", self->yytext));
        // }

        // TODO: wait for reply to https://github.com/gtkwave/gtkwave/issues/331
        // /* turn off autocoalesce for Icarus */
        // if (strstr(self->yytext, "Icarus") != NULL) {
        //     GLOBALS->autocoalesce = 0;
        //     rc = 1;
        // }
    }
    // if (hdr) {
    //     DEBUG(fprintf(stderr, "\n"));
    // }
    return (rc);
}

static void parse_valuechange(GwVcdLoader *self)
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

                    if (n->mv.mvlfac_vlist_writer ==
                        NULL) /* overloaded for vlist, numhist = last position used */
                    {
                        n->mv.mvlfac_vlist_writer =
                            gw_vlist_writer_new(self->vlist_compression_level, self->vlist_prepack);
                        gw_vlist_writer_append_uv32(
                            n->mv.mvlfac_vlist_writer,
                            (unsigned int)'0'); /* represents single bit routine
                                                 for decompression */
                        gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer,
                                                    (unsigned int)v->vartype);
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

                    gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, rcv);
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

                if (n->mv.mvlfac_vlist_writer ==
                    NULL) /* overloaded for vlist, numhist = last position used */
                {
                    unsigned char typ2 = toupper(typ);
                    n->mv.mvlfac_vlist_writer =
                        gw_vlist_writer_new(self->vlist_compression_level, self->vlist_prepack);

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

                    gw_vlist_writer_append_uv32(
                        n->mv.mvlfac_vlist_writer,
                        (unsigned int)toupper(typ2)); /* B/R/P/S for decompress */
                    gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer,
                                                (unsigned int)v->vartype);
                    gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, (unsigned int)v->size);
                }

                time_delta = self->time_vlist_count - (unsigned int)n->numhist;
                n->numhist = self->time_vlist_count;

                gw_vlist_writer_append_uv32(n->mv.mvlfac_vlist_writer, time_delta);

                if ((typ == 'b') || (typ == 'B')) {
                    if ((v->vartype != V_REAL) && (v->vartype != V_STRINGTYPE)) {
                        gw_vlist_writer_append_mvl9_string(n->mv.mvlfac_vlist_writer, vector);
                    } else {
                        gw_vlist_writer_append_string(n->mv.mvlfac_vlist_writer, vector);
                    }
                } else {
                    if ((v->vartype == V_REAL) || (v->vartype == V_STRINGTYPE) || (typ == 's') ||
                        (typ == 'S')) {
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
                        gw_vlist_writer_append_mvl9_string(n->mv.mvlfac_vlist_writer, bits);
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

static int strcpy_delimfix(GwVcdLoader *self, char *too, char *from)
{
    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

    char ch;
    int found = 0;

    do {
        ch = *(from++);
        if (ch == delimiter) {
            ch = VCDNAM_ESCAPE;
            found = 1;
        }
    } while ((*(too++) = ch));

    if (found) {
        self->has_escaped_names = TRUE;
    }

    return (found);
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

static void strcpy_vcdalt(GwVcdLoader *self, char *too, char *from)
{
    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));
    gchar alt_delimiter = gw_loader_get_alternate_hierarchy_delimiter(GW_LOADER(self));

    char ch;

    do {
        ch = *(from++);
        if (ch == alt_delimiter) {
            ch = delimiter;
        }
    } while ((*(too++) = ch));
}

static void vcd_parse(GwVcdLoader *self)
{
    int tok;
    unsigned char ttype;
    int disable_autocoalesce = 0;

    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));
    gchar alt_delimiter = gw_loader_get_alternate_hierarchy_delimiter(GW_LOADER(self));

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
                self->global_time_offset = atoi_64(self->yytext);

                // DEBUG(fprintf(stderr, "TIMEZERO: %" GW_TIME_FORMAT "\n",
                // self->global_time_offset));
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
                        self->time_dimension = prefix;
                        break;
                    case 's':
                        self->time_dimension = ' ';
                        break;
                    default: /* unknown */
                        self->time_dimension = 'n';
                        break;
                }

                // DEBUG(fprintf(stderr,
                //               "TIMESCALE: %" GW_TIME_FORMAT " %cs\n",
                //               self->time_scale,
                //               self->time_dimension));
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
                    push_scope(self, self->yytext, self->mod_tree_parent);

                    allocate_and_decorate_module_tree_node(&self->tree_root,
                                                           ttype,
                                                           self->yytext,
                                                           -1,
                                                           0,
                                                           0,
                                                           &self->mod_tree_parent);

                    // DEBUG(fprintf(stderr, "SCOPE: %s\n", self->name_prefix->str));
                }
                sync_end(self, NULL);
                break;
            case T_UPSCOPE:
                if (!g_queue_is_empty(self->scopes)) {
                    self->mod_tree_parent = pop_scope(self);
                    // DEBUG(fprintf(stderr, "SCOPE: %s\n", self->name_prefix->str));
                } else {
                    self->mod_tree_parent = NULL;
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

                        vtok = get_vartoken(self, 0);
                        if (vtok != V_STRING)
                            goto err;
                        if (self->name_prefix->len > 0) {
                            v->name = g_malloc(self->name_prefix->len + 1 + self->yylen + 1);
                            strcpy(v->name, self->name_prefix->str);
                            v->name[self->name_prefix->len] = delimiter;
                            if (alt_delimiter != '\0') {
                                strcpy_vcdalt(self,
                                              v->name + self->name_prefix->len + 1,
                                              self->yytext);
                            } else {
                                if ((strcpy_delimfix(self,
                                                     v->name + self->name_prefix->len + 1,
                                                     self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd =
                                        g_malloc(self->name_prefix->len + 1 + self->yylen + 2);
                                    strcpy(sd, self->name_prefix->str);
                                    sd[self->name_prefix->len] = delimiter;
                                    sd[self->name_prefix->len + 1] = '\\';
                                    strcpy(sd + self->name_prefix->len + 2,
                                           v->name + self->name_prefix->len + 1);
                                    g_free(v->name);
                                    v->name = sd;
                                }
                            }
                        } else {
                            v->name = g_malloc(self->yylen + 1);
                            if (alt_delimiter != '\0') {
                                strcpy_vcdalt(self, v->name, self->yytext);
                            } else {
                                if ((strcpy_delimfix(self, v->name, self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd = g_malloc(self->yylen + 2);
                                    sd[0] = '\\';
                                    strcpy(sd + 1, v->name);
                                    g_free(v->name);
                                    v->name = sd;
                                }
                            }
                        }

                        if (self->pv != NULL) {
                            if (!strcmp(self->prev_hier_uncompressed_name, v->name) &&
                                !disable_autocoalesce && (!strchr(v->name, '\\'))) {
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
                    } else /* regular vcd var, not an evcd port var */
                    {
                        vtok = get_vartoken(self, 1);
                        if (vtok == V_END)
                            goto err;
                        v->size = atoi_64(self->yytext);
                        vtok = get_strtoken(self);
                        if (vtok == V_END)
                            goto err;
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

                        vtok = get_vartoken(self, 0);
                        if (vtok != V_STRING)
                            goto err;

                        if (self->name_prefix->len > 0) {
                            v->name = g_malloc(self->name_prefix->len + 1 + self->yylen + 1);
                            strcpy(v->name, self->name_prefix->str);
                            v->name[self->name_prefix->len] = delimiter;
                            if (alt_delimiter != '\0') {
                                strcpy_vcdalt(self,
                                              v->name + self->name_prefix->len + 1,
                                              self->yytext);
                            } else {
                                if ((strcpy_delimfix(self,
                                                     v->name + self->name_prefix->len + 1,
                                                     self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd =
                                        g_malloc(self->name_prefix->len + 1 + self->yylen + 2);
                                    strcpy(sd, self->name_prefix->str);
                                    sd[self->name_prefix->len] = delimiter;
                                    sd[self->name_prefix->len + 1] = '\\';
                                    strcpy(sd + self->name_prefix->len + 2,
                                           v->name + self->name_prefix->len + 1);
                                    g_free(v->name);
                                    v->name = sd;
                                }
                            }
                        } else {
                            v->name = g_malloc(self->yylen + 1);
                            if (alt_delimiter != '\0') {
                                strcpy_vcdalt(self, v->name, self->yytext);
                            } else {
                                if ((strcpy_delimfix(self, v->name, self->yytext)) &&
                                    (self->yytext[0] != '\\')) {
                                    char *sd = g_malloc(self->yylen + 2);
                                    sd[0] = '\\';
                                    strcpy(sd + 1, v->name);
                                    g_free(v->name);
                                    v->name = sd;
                                }
                            }
                        }

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
                        sync_end(self, NULL);
                }
                break;
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

                        if (self->start_time < 0) {
                            self->start_time = tim;
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
                        if (self->end_time < tim)
                            self->end_time = tim; /* in case of malformed vcd files */
                        // DEBUG(fprintf(stderr, "#%" GW_TIME_FORMAT "\n", tim));

                        tt =
                            gw_vlist_alloc(&self->time_vlist, FALSE, self->vlist_compression_level);
                        *tt = tim;
                        self->time_vlist_count++;
                    } else {
                        if (self->time_vlist_count) {
                            /* OK, otherwise fix for System C which doesn't emit time zero... */
                        } else {
                            GwTime tim = GW_TIME_CONSTANT(0);
                            GwTime *tt;

                            self->start_time = self->current_time = self->end_time = tim;

                            tt = gw_vlist_alloc(&self->time_vlist,
                                                FALSE,
                                                self->vlist_compression_level);
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
                gw_blackout_regions_add_dumpoff(self->blackout_regions, self->current_time);
                break;
            case T_DUMPON:
            case T_DUMPPORTSON:
                gw_blackout_regions_add_dumpon(self->blackout_regions, self->current_time);
                break;
            case T_DUMPVARS:
            case T_DUMPPORTS:
                if (self->current_time < 0) {
                    self->start_time = self->current_time = self->end_time = 0;
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
                gw_blackout_regions_add_dumpon(self->blackout_regions, self->current_time);

                self->pv = NULL;
                if (self->prev_hier_uncompressed_name) {
                    g_free(self->prev_hier_uncompressed_name);
                    self->prev_hier_uncompressed_name = NULL;
                }

                return;
            default: {
                // DEBUG(fprintf(stderr, "UNKNOWN TOKEN\n"));
            }
        }
    }
}

/*******************************************************************************/

static GwSymbol *symfind_unsorted(GwVcdLoader *self, char *s)
{
    int hv = gw_hash(s);

    for (GwSymbol *iter = self->sym_hash[hv]; iter != NULL; iter = iter->sym_next) {
        if (strcmp(iter->name, s) == 0) {
            return iter; /* in table already */
        }
    }

    return NULL; /* not found, add here if you want to add*/
}

static void vcd_build_symbols(GwVcdLoader *self)
{
    int j;
    int max_slen = -1;
    GSList *sym_chain = NULL;
    int duphier = 0;
    char hashdirty;
    struct vcdsymbol *v, *vprime;
    char *str = g_alloca(1); /* quiet scan-build null pointer warning below */
    // #ifdef _WAVE_HAVE_JUDY
    //     int ss_len, longest = 0;
    // #endif

    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

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
                str[slen] = delimiter;
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
                GwSymbol *s = NULL;

                for (j = 0; j < v->size; j++) {
                    if (v->msi >= 0) {
                        sprintf(str + slen - 1, "[%d]", msi);
                    }

                    hashdirty = 0;
                    if (symfind_unsorted(self, str)) {
                        char *dupfix = g_malloc(max_slen + 32);
                        // #ifndef _WAVE_HAVE_JUDY
                        hashdirty = 1;
                        // #endif
                        // DEBUG(fprintf(stderr, "Warning: %s is a duplicate net name.\n", str));

                        do {
                            sprintf(dupfix, "$DUP%d%c%s", duphier++, delimiter, str);
                        } while (symfind_unsorted(self, dupfix));

                        strcpy(str, dupfix);
                        g_free(dupfix);
                        duphier = 0; /* reset for next duplicate resolution */
                    }
                    /* fallthrough */
                    {
                        if (hashdirty) {
                            self->hash_cache = gw_hash(str);
                        }
                        s = symadd(self, str, self->hash_cache);

                        // #ifdef _WAVE_HAVE_JUDY
                        //                         ss_len = strlen(str);
                        //                         if (ss_len >= longest) {
                        //                             longest = ss_len + 1;
                        //                         }
                        // #endif
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

                        // #ifndef _WAVE_HAVE_JUDY
                        s->n->nname = s->name;
                        // #endif
                        self->sym_chain = g_slist_prepend(self->sym_chain, s);

                        self->numfacs++;
                        // DEBUG(fprintf(stderr, "Added: %s\n", str));
                    }
                    msi += delta;
                }

                if ((j == 1) && (v->root)) {
                    s->vec_root = (GwSymbol *)v->root; /* these will get patched over */
                    s->vec_chain = (GwSymbol *)v->chain; /* these will get patched over */
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
                if (symfind_unsorted(self, str)) {
                    char *dupfix = g_malloc(max_slen + 32);
                    // #ifndef _WAVE_HAVE_JUDY
                    hashdirty = 1;
                    // #endif
                    // DEBUG(fprintf(stderr, "Warning: %s is a duplicate net name.\n", str));

                    do {
                        sprintf(dupfix, "$DUP%d%c%s", duphier++, delimiter, str);

                    } while (symfind_unsorted(self, dupfix));

                    strcpy(str, dupfix);
                    g_free(dupfix);
                    duphier = 0; /* reset for next duplicate resolution */
                }
                /* fallthrough */
                {
                    /* cut down on double lookups.. */
                    if (hashdirty) {
                        self->hash_cache = gw_hash(str);
                    }
                    GwSymbol *s = symadd(self, str, self->hash_cache);

                    // #ifdef _WAVE_HAVE_JUDY
                    //                     ss_len = strlen(str);
                    //                     if (ss_len >= longest) {
                    //                         longest = ss_len + 1;
                    //                     }
                    // #endif
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

                    // #ifndef _WAVE_HAVE_JUDY
                    s->n->nname = s->name;
                    // #endif
                    self->sym_chain = g_slist_prepend(self->sym_chain, s);

                    self->numfacs++;
                    // DEBUG(fprintf(stderr, "Added: %s\n", str));
                }
            }
        }

        v = v->next;
    }

    // TODO: reenable judy support
    // #ifdef _WAVE_HAVE_JUDY
    //     {
    //         Pvoid_t PJArray = GLOBALS->sym_judy;
    //         PPvoid_t PPValue;
    //         char *Index = calloc_2(1, longest);
    //
    //         for (PPValue = JudySLFirst(PJArray, (uint8_t *)Index, PJE0); PPValue !=
    //         (PPvoid_t)NULL;
    //              PPValue = JudySLNext(PJArray, (uint8_t *)Index, PJE0)) {
    //             GwSymbol *s = *(GwSymbol **)PPValue;
    //             s->name = strdup_2(Index);
    //             s->n->nname = s->name;
    //         }
    //
    //         free_2(Index);
    //     }
    // #endif

    if (sym_chain != NULL) {
        for (GSList *iter = sym_chain; iter != NULL; iter = iter->next) {
            GwSymbol *s = iter->data;

            s->vec_root = ((struct vcdsymbol *)s->vec_root)->sym_chain;
            if ((struct vcdsymbol *)s->vec_chain != NULL) {
                s->vec_chain = ((struct vcdsymbol *)s->vec_chain)->sym_chain;
            }

            // DEBUG(printf("Link: ('%s') '%s' -> '%s'\n",
            //              sym_curr->val->vec_root->name,
            //              sym_curr->val->name,
            //              sym_curr->val->vec_chain ? sym_curr->val->vec_chain->name : "(END)"));
        }

        g_slist_free(sym_chain);
    }
}

/*******************************************************************************/

static void vcd_cleanup(GwVcdLoader *self)
{
    struct vcdsymbol *v, *vt;

    g_clear_pointer(&self->symbols_indexed, g_free);
    g_clear_pointer(&self->symbols_sorted, g_free);

    v = self->vcdsymroot;
    while (v) {
        g_free(v->name);
        g_free(v->id);
        g_free(v->narray);
        vt = v;
        v = v->next;
        g_free(vt);
    }
    self->vcdsymroot = NULL;
    self->vcdsymcurr = NULL;

    g_queue_free_full(self->scopes, g_free);
    g_string_free(self->name_prefix, TRUE);
    self->scopes = NULL;
    self->name_prefix = NULL;

    if (self->is_compressed) {
        pclose(self->vcd_handle);
    } else {
        fclose(self->vcd_handle);
    }
    self->vcd_handle = NULL;

    g_clear_pointer(&self->yytext, g_free);
}

static GwFacs *vcd_sortfacs(GwVcdLoader *self)
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
static const char *get_module_name(GwVcdLoader *self, const char *s)
{
    char ch;
    char *pnt;

    pnt = self->module_tree;

    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

    for (;;) {
        ch = *(s++);

        if (((ch == delimiter) || (ch == '|')) &&
            (*s)) /* added && null check to allow possible . at end of name */
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

static void build_tree_from_name(GwVcdLoader *self,
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

/*
 * unswizzle extended names in tree
 */
static void treenamefix_str(char *s, char delimiter)
{
    while (*s) {
        if (*s == VCDNAM_ESCAPE)
            *s = delimiter;
        s++;
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

static GwTree *vcd_build_tree(GwVcdLoader *self, GwFacs *facs)
{
    // TODO: replace module_tree by GString to dynamically allocate enough memory
    self->module_tree = g_malloc0(65536);

    gchar delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

    for (guint i = 0; i < gw_facs_get_length(facs); i++) {
        GwSymbol *fac = gw_facs_get(facs, i);

        char *n = fac->name;
        build_tree_from_name(self, &self->tree_root, n, i);

        if (self->has_escaped_names) {
            char *subst, ch;
            subst = fac->name;
            while ((ch = (*subst))) {
                if (ch == VCDNAM_ESCAPE) {
                    *subst = delimiter;
                } /* restore back to normal */
                subst++;
            }
        }
    }

    GwTree *tree = gw_tree_new(g_steal_pointer(&self->tree_root));
    gw_tree_graft(tree, self->terminals_chain);
    gw_tree_sort(tree);

    if (self->has_escaped_names) {
        treenamefix(gw_tree_get_root(tree), delimiter);
    }

    return tree;
}

/*******************************************************************************/

static void gw_vcd_loader_finalize(GObject *object)
{
    GwVcdLoader *self = GW_VCD_LOADER(object);

    g_free(self->sym_hash);

    G_OBJECT_CLASS(gw_vcd_loader_parent_class)->finalize(object);
}

static GwDumpFile *gw_vcd_loader_load(GwLoader *loader, const gchar *fname, GError **error)
{
    g_return_val_if_fail(fname != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    GwVcdLoader *self = GW_VCD_LOADER(loader);

    errno = 0; /* reset in case it's set for some reason */

    if (g_str_has_suffix(fname, ".gz") || g_str_has_suffix(fname, ".zip")) {
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

            if (self->warning_filesize > 0 &&
                self->vcd_fsiz > self->warning_filesize * 1024 * 1024) {
                if (!self->vlist_prepack) {
                    fprintf(stderr,
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
            // TODO: Fix splash update
            // GLOBALS->splash_disable = 1;
            self->vcd_handle = stdin;
        }
        self->is_compressed = 0;
    }

    if (self->vcd_handle == NULL) {
        g_set_error(error,
                    GW_DUMP_FILE_ERROR,
                    GW_DUMP_FILE_ERROR_UNKNOWN,
                    "Error opening %s .vcd file '%s'.\n",
                    self->is_compressed ? "compressed" : "",
                    fname);
        return FALSE;
    }

    // TODO: update splash
    // /* SPLASH */ splash_create();

    getch_alloc(self); /* alloc membuff for vcd getch buffer */

    update_name_prefix(self);

    self->time_vlist = gw_vlist_create(sizeof(GwTime));

    vcd_parse(self);
    if (self->varsplit) {
        g_free(self->varsplit);
        self->varsplit = NULL;
    }

    gw_vlist_freeze(&self->time_vlist, self->vlist_compression_level);

    vlist_emit_finalize(self);

    if (self->symbols_sorted == NULL && self->symbols_indexed == NULL) {
        fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
        vcd_exit(255);
    }

    fprintf(stderr,
            "[%" GW_TIME_FORMAT "] start time.\n[%" GW_TIME_FORMAT "] end time.\n",
            self->start_time * self->time_scale,
            self->end_time * self->time_scale);

    // TODO: udpate splash
    // if (self->vcd_fsiz > 0) {
    //     splash_sync(self->vcd_fsiz, self->vcd_fsiz);
    // } else if (self->is_compressed) {
    //     splash_sync(1, 1);
    // }
    self->vcd_fsiz = 0;

    GwTime min_time = self->start_time * self->time_scale;
    GwTime max_time = self->end_time * self->time_scale;
    self->global_time_offset *= self->time_scale;

    if (min_time == max_time && max_time == GW_TIME_CONSTANT(-1)) {
        fprintf(stderr, "VCD times range is equal to zero.  Exiting.\n");
        vcd_exit(255);
    }

    vcd_build_symbols(self);
    GwFacs *facs = vcd_sortfacs(self);
    GwTree *tree = vcd_build_tree(self, facs);
    vcd_cleanup(self);

    getch_free(self); /* free membuff for vcd getch buffer */

    gw_blackout_regions_scale(self->blackout_regions, self->time_scale);

    // TODO: update splash
    // /* SPLASH */ splash_finalize();

    GwTimeRange *time_range = gw_time_range_new(min_time, max_time);

    // clang-format off
    GwVcdFile *dump_file = g_object_new(GW_TYPE_VCD_FILE,
                                        "tree", tree,
                                        "facs", facs,
                                        "blackout-regions", self->blackout_regions,
                                        "time-scale", self->time_scale,
                                        "time-dimension", self->time_dimension,
                                        "time-range", time_range,
                                        "global-time-offset", self->global_time_offset,
                                        "has-escaped-names", self->has_escaped_names,
                                        NULL);
    // clang-format on

    g_object_unref(self->blackout_regions);

    dump_file->start_time = self->start_time;
    dump_file->end_time = self->end_time;
    dump_file->time_vlist = self->time_vlist;
    dump_file->is_prepacked = self->vlist_prepack;

    dump_file->preserve_glitches = gw_loader_is_preserve_glitches(loader);
    dump_file->preserve_glitches_real = gw_loader_is_preserve_glitches_real(loader);

    g_object_unref(tree);
    g_object_unref(time_range);

    return GW_DUMP_FILE(dump_file);
}

static void gw_vcd_loader_set_property(GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
    GwVcdLoader *self = GW_VCD_LOADER(object);

    switch (property_id) {
        case PROP_VLIST_PREPACK:
            gw_vcd_loader_set_vlist_prepack(self, g_value_get_boolean(value));
            break;

        case PROP_VLIST_COMPRESSION_LEVEL:
            gw_vcd_loader_set_vlist_compression_level(self, g_value_get_int(value));
            break;

        case PROP_WARNING_FILESIZE:
            gw_vcd_loader_set_warning_filesize(self, g_value_get_uint(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_vcd_loader_get_property(GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
    GwVcdLoader *self = GW_VCD_LOADER(object);

    switch (property_id) {
        case PROP_VLIST_PREPACK:
            g_value_set_boolean(value, gw_vcd_loader_is_vlist_prepack(self));
            break;

        case PROP_VLIST_COMPRESSION_LEVEL:
            g_value_set_int(value, gw_vcd_loader_get_vlist_compression_level(self));
            break;

        case PROP_WARNING_FILESIZE:
            g_value_set_uint(value, gw_vcd_loader_get_warning_filesize(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_vcd_loader_class_init(GwVcdLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GwLoaderClass *loader_class = GW_LOADER_CLASS(klass);

    object_class->finalize = gw_vcd_loader_finalize;
    object_class->set_property = gw_vcd_loader_set_property;
    object_class->get_property = gw_vcd_loader_get_property;

    loader_class->load = gw_vcd_loader_load;

    properties[PROP_VLIST_PREPACK] =
        g_param_spec_boolean("vlist-prepack",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_VLIST_COMPRESSION_LEVEL] =
        g_param_spec_int("vlist-compresion-level",
                         NULL,
                         NULL,
                         Z_DEFAULT_COMPRESSION /* -1 */,
                         Z_BEST_COMPRESSION /* 9 */,
                         Z_DEFAULT_COMPRESSION,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_WARNING_FILESIZE] =
        g_param_spec_uint("warning-filesize",
                          NULL,
                          NULL,
                          0,
                          G_MAXUINT,
                          0,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_vcd_loader_init(GwVcdLoader *self)
{
    self->current_time = -1;
    self->start_time = -1;
    self->end_time = -1;
    self->time_scale = 1;
    self->time_dimension = GW_TIME_DIMENSION_NANO;

    self->T_MAX_STR = 1024;
    self->yytext = g_malloc(self->T_MAX_STR + 1);
    self->vcd_minid = G_MAXUINT;
    self->scopes = g_queue_new();
    self->name_prefix = g_string_new(NULL);
    self->blackout_regions = gw_blackout_regions_new();

    self->vlist_compression_level = Z_DEFAULT_COMPRESSION;

    self->sym_hash = g_new0(GwSymbol *, GW_HASH_PRIME);
    self->warning_filesize = 256;
}

GwLoader *gw_vcd_loader_new(void)
{
    return g_object_new(GW_TYPE_VCD_LOADER, NULL);
}

void gw_vcd_loader_set_vlist_prepack(GwVcdLoader *self, gboolean vlist_prepack)
{
    g_return_if_fail(GW_IS_VCD_LOADER(self));

    vlist_prepack = !!vlist_prepack;

    if (self->vlist_prepack != vlist_prepack) {
        self->vlist_prepack = vlist_prepack;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VLIST_PREPACK]);
    }
}

gboolean gw_vcd_loader_is_vlist_prepack(GwVcdLoader *self)
{
    g_return_val_if_fail(GW_IS_VCD_LOADER(self), FALSE);

    return self->vlist_prepack;
}

void gw_vcd_loader_set_vlist_compression_level(GwVcdLoader *self, gint level)
{
    g_return_if_fail(GW_IS_VCD_LOADER(self));

    level = CLAMP(level, Z_DEFAULT_COMPRESSION, Z_BEST_COMPRESSION);

    if (self->vlist_compression_level != level) {
        self->vlist_compression_level = level;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VLIST_COMPRESSION_LEVEL]);
    }
}

gint gw_vcd_loader_get_vlist_compression_level(GwVcdLoader *self)
{
    g_return_val_if_fail(GW_IS_VCD_LOADER(self), FALSE);

    return self->vlist_compression_level;
}

void gw_vcd_loader_set_warning_filesize(GwVcdLoader *self, guint warning_filesize)
{
    g_return_if_fail(GW_IS_VCD_LOADER(self));

    if (self->warning_filesize != warning_filesize) {
        self->warning_filesize = warning_filesize;

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_WARNING_FILESIZE]);
    }
}

guint gw_vcd_loader_get_warning_filesize(GwVcdLoader *self)
{
    g_return_val_if_fail(GW_IS_VCD_LOADER(self), FALSE);

    return self->warning_filesize;
}