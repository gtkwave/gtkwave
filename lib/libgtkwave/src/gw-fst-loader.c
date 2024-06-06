#include "gw-fst-loader.h"
#include "gw-fst-file.h"
#include "gw-fst-file-private.h"
#include "gw-util.h"
#include <fstapi.h>

static GwTreeKind fst_scope_type_to_gw_tree_kind(enum fstScopeType scope_type);
static GwVarType fst_var_type_to_gw_var_type(enum fstVarType var_type);
static GwVarDir fst_var_dir_to_gw_var_dir(enum fstVarDir var_dir);
static GwVarDataType fst_supplemental_data_type_to_gw_var_data_type(
    enum fstSupplementalDataType supplemental_data_type);

#define FST_RDLOAD "FSTLOAD | "

// TODO: remove!
#define WAVE_T_WHICH_UNDEFINED_COMPNAME (-1)
#define WAVE_T_WHICH_COMPNAME_START (-2)
#define F_NAME_MODULUS (3)

struct _GwFstLoader
{
    GwLoader parent_instance;

    void *fst_reader;

    guint32 next_var_stem;
    guint32 next_var_istem;

    GwTimeDimension time_dimension;
    GwTime time_scale;

    GwTime first_cycle;
    GwTime last_cycle;
    GwTime total_cycles;

    char *synclock_str;

    fstEnumHandle queued_xl_enum_filter;

    GwStems *stems;

    GwLx2Entry *fst_table;

    GwFac *mvlfacs;
    fstHandle *mvlfacs_rvs_alias;

    JRB subvar_jrb;
    guint subvar_jrb_count;
    gboolean subvar_jrb_count_locked;

    JRB synclock_jrb;
    JRB enum_nptrs_jrb;

    GwTreeBuilder *tree_builder;

    // TODO: don't use strings for start and end time
    gchar *start_time;
    gchar *end_time;

    GwEnumFilterList *enum_filters;

    gboolean has_nonimplicit_directions;
    gboolean has_supplemental_datatypes;
    gboolean has_supplemental_vartypes;

    GwTreeNode *terminals_chain;

    GwStringTable *component_names;
};

G_DEFINE_TYPE(GwFstLoader, gw_fst_loader, GW_TYPE_LOADER)

enum
{
    PROP_START_TIME = 1,
    PROP_END_TIME,
    N_PROPERTIES,
};

GParamSpec *properties[N_PROPERTIES];

/*
 * reverse equality mem compare
 */
static int memrevcmp(int i, const char *s1, const char *s2)
{
    i--;
    for (; i >= 0; i--) {
        if (s1[i] != s2[i])
            break;
    }

    return (i + 1);
}

/*
 * fast itoa for decimal numbers
 */
static char *itoa_2(int value, char *result)
{
    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= 10;
        *ptr++ = "9876543210123456789"[9 + (tmp_value - value * 10)];
    } while (value);

    if (tmp_value < 0)
        *ptr++ = '-';
    result = ptr;
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return (result);
}

/*
 * preformatted sprintf statements which remove parsing latency
 */
static int sprintf_2_sd(char *s, char *c, int d)
{
    char *s2 = s;

    while (*c) {
        *(s2++) = *(c++);
    }
    *(s2++) = '[';
    s2 = itoa_2(d, s2);
    *(s2++) = ']';
    *s2 = 0;

    return (s2 - s);
}

static int sprintf_2_sdd(char *s, char *c, int d, int d2)
{
    char *s2 = s;

    while (*c) {
        *(s2++) = *(c++);
    }
    *(s2++) = '[';
    s2 = itoa_2(d, s2);
    *(s2++) = ':';
    s2 = itoa_2(d2, s2);
    *(s2++) = ']';
    *s2 = 0;

    return (s2 - s);
}

/******************************************************************/

static void handle_scope(GwFstLoader *self, struct fstHierScope *scope)
{
    unsigned char ttype = fst_scope_type_to_gw_tree_kind(scope->typ);

    gint component_index = WAVE_T_WHICH_UNDEFINED_COMPNAME;

    if (scope->component != NULL && scope->component[0] != '\0' &&
        strcmp(scope->component, scope->name) != 0) {
        guint index = gw_string_table_add(self->component_names, scope->component);
        component_index = WAVE_T_WHICH_COMPNAME_START - index;
    }

    GwTreeNode *node = gw_tree_builder_push_scope(self->tree_builder, ttype, scope->name);
    node->t_which = component_index;
    node->t_stem = self->next_var_stem;
    node->t_istem = self->next_var_istem;

    self->next_var_stem = 0;
    self->next_var_istem = 0;
}

static void handle_upscope(GwFstLoader *self)
{
    gw_tree_builder_pop_scope(self->tree_builder);
}

static void handle_var(GwFstLoader *self,
                       struct fstHierVar *var,
                       gint *msb,
                       gint *lsb,
                       gchar **nam,
                       gint *namlen,
                       guint *nnam_max)
{
    (void)self;

    char *pnt;
    char *lb_last = NULL;
    char *col_last = NULL;
    char *rb_last = NULL;

    if (var->name_length > (*nnam_max)) {
        g_free(*nam);
        *nam = g_malloc(((*nnam_max) = var->name_length) + 1);
    }

    char *s = *nam;
    const char *pnts = var->name;
    char *pntd = s;

    int esc_sm = 0;
    while (*pnts) {
        if (*pnts == '\\') {
            esc_sm = 1;
        }

        if (*pnts != ' ') {
            if (*pnts == '[' && !esc_sm) {
                lb_last = pntd;
                col_last = NULL;
                rb_last = NULL;
            } else if (*pnts == ':' && lb_last != NULL && col_last == NULL && rb_last == NULL &&
                       !esc_sm) {
                col_last = pntd;
            } else if (*pnts == ']' && lb_last != NULL && rb_last == NULL && !esc_sm) {
                rb_last = pntd;
            } else if (lb_last != NULL && rb_last == NULL &&
                       (g_ascii_isdigit(*pnts) || (*pnts == '-')) && !esc_sm) {
            } else {
                lb_last = NULL;
                col_last = NULL;
                rb_last = NULL;
            }

            *(pntd++) = *pnts;
        } else {
            esc_sm = 0;
        }
        pnts++;
    }
    *pntd = 0;
    *namlen = pntd - s;

    if (!rb_last) {
        col_last = NULL;
        lb_last = NULL;
    }

    if (!lb_last) {
        *msb = *lsb = -1;
    } else if ((var->length > 1) && !col_last &&
               lb_last) /* add for NC arrays that don't add final explicit bitrange like VCS does */
    {
        lb_last = NULL;
        *msb = var->length - 1;
        *lsb = 0;
    } else {
        int sgna = 1, sgnb = 1;
        pnt = lb_last + 1;
        int acc = 0;
        while (g_ascii_isdigit(*pnt) || (*pnt == '-')) {
            if (*pnt != '-') {
                acc *= 10;
                acc += (*pnt - '0');
            } else {
                sgna = -1;
            }
            pnt++;
        }

        *msb = acc * sgna;
        if (!col_last) {
            *lsb = acc;
        } else {
            pnt = col_last + 1;
            acc = 0;
            while (g_ascii_isdigit(*pnt) || (*pnt == '-')) {
                if (*pnt != '-') {
                    acc *= 10;
                    acc += (*pnt - '0');
                } else {
                    sgnb = -1;
                }
                pnt++;
            }
            *lsb = acc * sgnb;
        }
    }

    if (lb_last) {
        *lb_last = 0;
        if ((lb_last - s) < (*namlen)) {
            *namlen = lb_last - s;
        }
    }
    *nam = s;
}

static void handle_supvar(GwFstLoader *self,
                          struct fstHierAttr *attr,
                          guint8 *sdt,
                          guint8 *svt,
                          guint *sxt)
{
    if (attr->name != NULL) {
        JRB subvar_jrb_node;
        char *attr_pnt;

        if (fstReaderGetFileType(self->fst_reader) == FST_FT_VHDL) {
            char *lc_p = attr_pnt = g_strdup(attr->name);

            while (*lc_p) {
                *lc_p = tolower(*lc_p); /* convert attrib name to lowercase for VHDL */
                lc_p++;
            }
        } else {
            attr_pnt = NULL;
        }

        /* sxt points to actual type name specified in FST file */
        subvar_jrb_node = jrb_find_str(self->subvar_jrb, attr_pnt ? attr_pnt : attr->name);
        if (subvar_jrb_node) {
            *sxt = subvar_jrb_node->val.ui;
        } else {
            Jval jv;

            if (self->subvar_jrb_count != WAVE_VARXT_MAX_ID) {
                *sxt = jv.ui = ++self->subvar_jrb_count;
                /* subvar_jrb_node = */ jrb_insert_str(self->subvar_jrb,
                                                       g_strdup(attr_pnt ? attr_pnt : attr->name),
                                                       jv);
            } else {
                sxt = 0;
                if (!self->subvar_jrb_count_locked) {
                    fprintf(stderr,
                            FST_RDLOAD "Max number (%d) of type attributes reached, "
                                       "please increase WAVE_VARXT_MAX_ID.\n",
                            WAVE_VARXT_MAX_ID);
                    self->subvar_jrb_count_locked = TRUE;
                }
            }
        }

        if (attr_pnt) {
            g_free(attr_pnt);
        }
    }

    *svt = attr->arg >> FST_SDT_SVT_SHIFT_COUNT;
    *sdt = attr->arg & (FST_SDT_ABS_MAX - 1);

    self->has_supplemental_datatypes = TRUE;
    if (*svt != FST_SVT_NONE && *svt != FST_SVT_VHDL_SIGNAL) {
        self->has_supplemental_vartypes = TRUE;
    }
}

static void handle_sourceistem(GwFstLoader *self, struct fstHierAttr *attr)
{
    uint32_t istem_path_number = (uint32_t)attr->arg_from_name;
    uint32_t istem_line_number = (uint32_t)attr->arg;

    self->next_var_istem = gw_stems_add_istem(self->stems, istem_path_number, istem_line_number);
}

static void handle_sourcestem(GwFstLoader *self, struct fstHierAttr *attr)
{
    uint32_t stem_path_number = (uint32_t)attr->arg_from_name;
    uint32_t stem_line_number = (uint32_t)attr->arg;

    self->next_var_stem = gw_stems_add_stem(self->stems, stem_path_number, stem_line_number);
}

static void handle_pathname(GwFstLoader *self, struct fstHierAttr *attr)
{
    const gchar *path = attr->name;
    guint64 index = attr->arg;

    // Check that path index has the expected value.
    // TODO: add warnings
    if (path != NULL && index == gw_stems_get_next_path_index(self->stems)) {
        gw_stems_add_path(self->stems, path);
    }
}

static void handle_valuelist(GwFstLoader *self, struct fstHierAttr *attr)
{
    if (attr->name == NULL) {
        return;
    }

    /* format is concatenations of [m b xs xe valstring] */
    if (self->synclock_str) {
        g_free(self->synclock_str);
    }
    self->synclock_str = g_strdup(attr->name);
}

static void handle_enumtable(GwFstLoader *self, struct fstHierAttr *attr)
{
    if (attr->name == NULL) {
        return;
    }
    /* consumed by next enum variable definition */
    self->queued_xl_enum_filter = attr->arg;

    if (attr->name_length == 0) {
        return;
    }

    /* currently fe->name is unused */
    struct fstETab *fe = fstUtilityExtractEnumTableFromString(attr->name);
    if (fe == NULL) {
        return;
    }

    GwEnumFilter *filter = gw_enum_filter_new();
    for (guint i = 0; i < fe->elem_count; i++) {
        gw_enum_filter_insert(filter, fe->val_arr[i], fe->literal_arr[i]);
    }

    guint index = gw_enum_filter_list_add(self->enum_filters, filter);

    if (index + 1 != attr->arg) {
        g_error("nonsequential enum tables definition encountered");
    }

    fstUtilityFreeEnumTable(fe);
    g_object_unref(filter);
}

static void handle_attr(GwFstLoader *self,
                        struct fstHierAttr *attr,
                        guint8 *sdt,
                        guint8 *svt,
                        guint *sxt)
{
    if (attr->typ != FST_AT_MISC) {
        return;
    }

    switch (attr->subtype) {
        case FST_MT_SUPVAR:
            handle_supvar(self, attr, sdt, svt, sxt);
            break;

        case FST_MT_SOURCEISTEM:
            handle_sourceistem(self, attr);
            break;

        case FST_MT_SOURCESTEM:
            handle_sourcestem(self, attr);
            break;

        case FST_MT_PATHNAME:
            handle_pathname(self, attr);
            break;

        case FST_MT_VALUELIST:
            handle_valuelist(self, attr);
            break;

        case FST_MT_ENUMTABLE:
            handle_enumtable(self, attr);
            break;

        default:
            // ignore
            break;
    }
}

static struct fstHier *extractNextVar(GwFstLoader *self,
                                      int *msb,
                                      int *lsb,
                                      char **nam,
                                      int *namlen,
                                      unsigned int *nnam_max)
{
    struct fstHier *h;
    guint8 sdt = FST_SDT_NONE;
    guint8 svt = FST_SVT_NONE;
    guint sxt = 0;

    void *fst_reader = self->fst_reader;

    while ((h = fstReaderIterateHier(fst_reader))) {
        switch (h->htyp) {
            case FST_HT_SCOPE:
                handle_scope(self, &h->u.scope);
                break;

            case FST_HT_UPSCOPE:
                handle_upscope(self);
                break;

            case FST_HT_VAR:
                handle_var(self, &h->u.var, msb, lsb, nam, namlen, nnam_max);

                h->u.var.svt_workspace = svt;
                h->u.var.sdt_workspace = sdt;
                h->u.var.sxt_workspace = sxt;

                return h;

            case FST_HT_ATTRBEGIN:
                // currently ignored for most cases except VHDL variable vartype/datatype creation
                handle_attr(self, &h->u.attr, &sdt, &svt, &sxt);
                break;

            case FST_HT_ATTREND:
                break;

            default:
                break;
        }
    }

    *namlen = 0;
    *nam = NULL;
    return (NULL);
}

static void fst_append_graft_chain(GwFstLoader *self, char *nam, int which, GwTreeNode *par)
{
    GwTreeNode *t = gw_tree_node_new(0, nam);
    t->t_which = which;

    t->child = par;
    t->next = self->terminals_chain;
    self->terminals_chain = t;
}

static GwBlackoutRegions *load_blackout_regions(GwFstLoader *self)
{
    void *fst_reader = self->fst_reader;

    GwBlackoutRegions *blackout_regions = gw_blackout_regions_new();

    guint32 num_activity_changes = fstReaderGetNumberDumpActivityChanges(fst_reader);
    for (guint32 activity_idx = 0; activity_idx < num_activity_changes; activity_idx++) {
        GwTime ct = fstReaderGetDumpActivityChangeTime(fst_reader, activity_idx) * self->time_scale;
        unsigned char ac = fstReaderGetDumpActivityChangeValue(fst_reader, activity_idx);

        if (ac == 1) {
            gw_blackout_regions_add_dumpon(blackout_regions, ct);
        } else {
            gw_blackout_regions_add_dumpoff(blackout_regions, ct);
        }
    }

    // Ensure that final blackout region is finished.
    gw_blackout_regions_add_dumpon(blackout_regions, self->last_cycle);

    return blackout_regions;
}

/*
 * atoi 64-bit version..
 * y/on     default to '1'
 * n/nonnum default to '0'
 */
static GwTime unformat_time_atoi_64(const char *str, const gchar **cont)
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
            *cont = str - 1;
            break;
        }
    }
    return (nflag ? (-val) : val);
}

static GwTime unformat_time_simple(const char *buf, char dim)
{
    GwTime rval;
    const char *pnt;
    const char *offs = NULL;
    const char *doffs;
    char ch;
    int i, ich, delta;

    static const char *TIME_UNITS = " munpfaz";

    const char *cont = NULL;
    rval = unformat_time_atoi_64(buf, &cont);
    if ((pnt = cont)) {
        while ((ch = *(pnt++))) {
            if ((ch == ' ') || (ch == '\t'))
                continue;

            ich = tolower((int)ch);
            if (ich == 's')
                ich = ' '; /* as in plain vanilla seconds */

            offs = strchr(TIME_UNITS, ich);
            break;
        }
    }

    if (!offs)
        return (rval);
    if ((dim == 'S') || (dim == 's')) {
        doffs = TIME_UNITS;
    } else {
        doffs = strchr(TIME_UNITS, (int)dim);
        if (!doffs)
            return (rval); /* should *never* happen */
    }

    delta = (doffs - TIME_UNITS) - (offs - TIME_UNITS);

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

// TODO: remove local copy of unformat_time
// TODO: use full version of unformat_time
static GwTime unformat_time(const char *buf, char dim)
{
    return unformat_time_simple(buf, dim);
}

static void gw_fst_loader_dispose(GObject *object)
{
    GwFstLoader *self = GW_FST_LOADER(object);

    g_clear_object(&self->tree_builder);

    G_OBJECT_CLASS(gw_fst_loader_parent_class)->dispose(object);
}

static GwDumpFile *gw_fst_loader_load(GwLoader *loader, const char *fname, GError **error)
{
    GwFstLoader *self = GW_FST_LOADER(loader);

    GwNode *n;
    GwSymbol *s;
    GwSymbol *prevsymroot = NULL;
    GwSymbol *prevsym = NULL;
    guint numalias = 0;
    guint numvars = 0;
    GwSymbol *sym_block = NULL;
    GwNode *node_block = NULL;
    struct fstHier *h = NULL;
    int msb, lsb;
    char *nnam = NULL;
    GwTreeNode *npar = NULL;
    char **f_name = NULL;
    int *f_name_len = NULL, *f_name_max_len = NULL;
    int allowed_to_autocoalesce = 1;
    unsigned int nnam_max = 0;

    int f_name_build_buf_len = 128;
    char *f_name_build_buf = g_malloc(f_name_build_buf_len + 1);

    self->fst_reader = fstReaderOpen(fname);
    if (self->fst_reader == NULL) {
        // TODO: report more detailed errors
        g_set_error(error,
                    GW_DUMP_FILE_ERROR,
                    GW_DUMP_FILE_ERROR_UNKNOWN,
                    "Failed to open FST file");
        return NULL;
    }

    // TODO: update splash
    // /* SPLASH */ splash_create();

    // TODO: wait for answer to https://github.com/gtkwave/gtkwave/issues/331
    // allowed_to_autocoalesce =
    //     (strstr(fstReaderGetVersionString(self->fst_reader), "Icarus") == NULL);
    // if (!allowed_to_autocoalesce) {
    //     GLOBALS->autocoalesce = 0;
    // }

    GwTimeScaleAndDimension *scale =
        gw_time_scale_and_dimension_from_exponent(fstReaderGetTimescale(self->fst_reader));
    self->time_dimension = scale->dimension;
    self->time_scale = scale->scale;
    g_free(scale);

    f_name = g_new0(char *, F_NAME_MODULUS + 1);
    f_name_len = g_new0(int, F_NAME_MODULUS + 1);
    f_name_max_len = g_new0(int, F_NAME_MODULUS + 1);

    nnam_max = 16;
    nnam = g_malloc(nnam_max + 1);

    self->subvar_jrb = make_jrb(); /* only used for attributes such as generated in VHDL, etc. */
    self->synclock_jrb = make_jrb(); /* only used for synthetic clocks */

    guint64 numfacs = fstReaderGetVarCount(self->fst_reader);

    GwFacs *facs = gw_facs_new(numfacs);
    self->mvlfacs = g_new0(GwFac, numfacs);
    sym_block = g_new0(GwSymbol, numfacs);
    node_block = g_new0(GwNode, numfacs);
    self->mvlfacs_rvs_alias = g_new0(fstHandle, numfacs);

    fprintf(stderr, FST_RDLOAD "Processing %lu facs.\n", numfacs);
    // TODO: update splash
    // /* SPLASH */ splash_sync(1, 5);

    self->first_cycle = fstReaderGetStartTime(self->fst_reader) * self->time_scale;
    self->last_cycle = fstReaderGetEndTime(self->fst_reader) * self->time_scale;
    self->total_cycles = self->last_cycle - self->first_cycle + 1;
    GwTime global_time_offset = fstReaderGetTimezero(self->fst_reader) * self->time_scale;

    GwBlackoutRegions *blackout_regions = load_blackout_regions(self);

    /* do your stuff here..all useful info has been initialized by now */

    char delimiter = gw_loader_get_hierarchy_delimiter(GW_LOADER(self));

    for (guint i = 0; i < numfacs; i++) {
        char buf[65537];
        char *str;
        GwFac *f;
        int hier_len, name_len, tlen;
        unsigned char nvt, nvd, ndt;
        char *fnam;
        int len_subst = 0;

        h = extractNextVar(self, &msb, &lsb, &nnam, &name_len, &nnam_max);
        if (!h) {
            /* this should never happen */
            fstReaderIterateHierRewind(self->fst_reader);
            h = extractNextVar(self, &msb, &lsb, &nnam, &name_len, &nnam_max);
            if (!h) {
                fprintf(stderr,
                        FST_RDLOAD "Exiting, missing or malformed names table encountered.\n");
                exit(255);
            }
        }

        npar = gw_tree_builder_get_current_scope(self->tree_builder);
        const gchar *name_prefix = gw_tree_builder_get_name_prefix(self->tree_builder);

        hier_len = name_prefix != NULL ? strlen(name_prefix) : 0;
        if (hier_len) {
            tlen = hier_len + 1 + name_len;
            if (tlen > f_name_max_len[i & F_NAME_MODULUS]) {
                if (f_name[i & F_NAME_MODULUS])
                    g_free(f_name[i & F_NAME_MODULUS]);
                f_name_max_len[i & F_NAME_MODULUS] = tlen;
                fnam = g_malloc(tlen + 1);
            } else {
                fnam = f_name[i & F_NAME_MODULUS];
            }

            memcpy(fnam, name_prefix, hier_len);
            fnam[hier_len] = delimiter;
            memcpy(fnam + hier_len + 1, nnam, name_len + 1);
        } else {
            tlen = name_len;
            if (tlen > f_name_max_len[i & F_NAME_MODULUS]) {
                if (f_name[i & F_NAME_MODULUS])
                    g_free(f_name[i & F_NAME_MODULUS]);
                f_name_max_len[i & F_NAME_MODULUS] = tlen;
                fnam = g_malloc(tlen + 1);
            } else {
                fnam = f_name[i & F_NAME_MODULUS];
            }

            memcpy(fnam, nnam, name_len + 1);
        }

        f_name[i & F_NAME_MODULUS] = fnam;
        f_name_len[i & F_NAME_MODULUS] = tlen;

        if ((h->u.var.length > 1) && (msb == -1) && (lsb == -1)) {
            node_block[i].msi = h->u.var.length - 1;
            node_block[i].lsi = 0;
        } else {
            unsigned int abslen = (msb >= lsb) ? (msb - lsb + 1) : (lsb - msb + 1);

            if ((h->u.var.length > abslen) && !(h->u.var.length % abslen)) /* check if 2d array */
            {
                /* printf("h->u.var.length: %d, abslen: %d '%s'\n", h->u.var.length, abslen, fnam);
                 */
                len_subst = 1;
            }

            node_block[i].msi = msb;
            node_block[i].lsi = lsb;
        }
        self->mvlfacs[i].len = h->u.var.length;

        if (h->u.var.length) {
            nvd = fst_var_dir_to_gw_var_dir(h->u.var.direction);
            if (nvd != GW_VAR_DIR_IMPLICIT) {
                self->has_nonimplicit_directions = TRUE;
            }

            nvt = fst_var_type_to_gw_var_type(h->u.var.typ);

            switch (h->u.var.typ) {
                case FST_VT_VCD_PARAMETER:
                case FST_VT_VCD_INTEGER:
                case FST_VT_SV_INT:
                case FST_VT_SV_SHORTINT:
                case FST_VT_SV_LONGINT:
                    self->mvlfacs[i].flags = GW_FAC_FLAG_INTEGER;
                    break;

                case FST_VT_VCD_REAL:
                case FST_VT_VCD_REAL_PARAMETER:
                case FST_VT_VCD_REALTIME:
                case FST_VT_SV_SHORTREAL:
                    self->mvlfacs[i].flags = GW_FAC_FLAG_DOUBLE;
                    break;

                case FST_VT_GEN_STRING:
                    self->mvlfacs[i].flags = GW_FAC_FLAG_STRING;
                    self->mvlfacs[i].len = 2;
                    break;

                default:
                    self->mvlfacs[i].flags = GW_FAC_FLAG_BITS;
                    break;
            }
        } else {
            /* convert any variable length records into strings */
            nvt = GW_VAR_TYPE_GEN_STRING;
            nvd = GW_VAR_DIR_IMPLICIT;
            self->mvlfacs[i].flags = GW_FAC_FLAG_STRING;
            self->mvlfacs[i].len = 2;
        }

        if (self->synclock_str != NULL) {
            if (self->mvlfacs[i].len == 1) /* currently only for single bit signals */
            {
                Jval syn_jv;

                /* special meaning for this in FST loader--means synthetic signal! */
                self->mvlfacs[i].flags |= GW_FAC_FLAG_SYNVEC;
                syn_jv.s = self->synclock_str;
                jrb_insert_int(self->synclock_jrb, i, syn_jv);
            } else {
                g_free(self->synclock_str);
            }

            /* under malloc_2() control for true if() branch, so not lost */
            self->synclock_str = NULL;
        }

        if (h->u.var.is_alias) {
            self->mvlfacs[i].node_alias =
                h->u.var.handle - 1; /* subtract 1 to scale it with gtkwave-style numbering */
            self->mvlfacs[i].flags |= GW_FAC_FLAG_ALIAS;
            numalias++;
        } else {
            self->mvlfacs_rvs_alias[numvars] = i;
            self->mvlfacs[i].node_alias = numvars;
            numvars++;
        }

        f = &self->mvlfacs[i];

        if ((f->len > 1) &&
            (!(f->flags & (GW_FAC_FLAG_INTEGER | GW_FAC_FLAG_DOUBLE | GW_FAC_FLAG_STRING)))) {
            int len = sprintf_2_sdd(buf,
                                    f_name[(i)&F_NAME_MODULUS],
                                    node_block[i].msi,
                                    node_block[i].lsi);

            if (len_subst) /* preserve 2d in name, but make 1d internally */
            {
                node_block[i].msi = h->u.var.length - 1;
                node_block[i].lsi = 0;
            }

            str = g_malloc(len + 1);
            memcpy(str, buf, len + 1);
            s = &sym_block[i];
            s->name = str;
            prevsymroot = prevsym = NULL;

            len = sprintf_2_sdd(buf, nnam, node_block[i].msi, node_block[i].lsi);
            fst_append_graft_chain(self, buf, i, npar);
        } else {
            int gatecmp =
                (f->len == 1) &&
                (!(f->flags & (GW_FAC_FLAG_INTEGER | GW_FAC_FLAG_DOUBLE | GW_FAC_FLAG_STRING))) &&
                (node_block[i].msi != -1) && (node_block[i].lsi != -1);
            int revcmp = gatecmp && (i) &&
                         (f_name_len[(i)&F_NAME_MODULUS] == f_name_len[(i - 1) & F_NAME_MODULUS]) &&
                         (!memrevcmp(f_name_len[(i)&F_NAME_MODULUS],
                                     f_name[(i)&F_NAME_MODULUS],
                                     f_name[(i - 1) & F_NAME_MODULUS]));

            if (gatecmp) {
                int len = sprintf_2_sd(buf, f_name[(i)&F_NAME_MODULUS], node_block[i].msi);

                str = g_malloc(len + 1);
                memcpy(str, buf, len + 1);
                s = &sym_block[i];
                s->name = str;
                if (allowed_to_autocoalesce && prevsym &&
                    revcmp) /* allow chaining for search functions.. */
                {
                    prevsym->vec_root = prevsymroot;
                    prevsym->vec_chain = s;
                    s->vec_root = prevsymroot;
                    prevsym = s;
                } else {
                    prevsymroot = prevsym = s;
                }

                len = sprintf_2_sd(buf, nnam, node_block[i].msi);
                fst_append_graft_chain(self, buf, i, npar);
            } else {
                int len = f_name_len[(i)&F_NAME_MODULUS];

                str = g_malloc(len + 1);
                memcpy(str, f_name[(i)&F_NAME_MODULUS], len + 1);
                s = &sym_block[i];
                s->name = str;
                prevsymroot = prevsym = NULL;

                if (f->flags & GW_FAC_FLAG_INTEGER) {
                    if (f->len != 0) {
                        node_block[i].msi = f->len - 1;
                        node_block[i].lsi = 0;
                        self->mvlfacs[i].len = f->len;
                    } else {
                        node_block[i].msi = 31;
                        node_block[i].lsi = 0;
                        self->mvlfacs[i].len = 32;
                    }
                }

                fst_append_graft_chain(self, nnam, i, npar);
            }
        }

        gw_facs_set(facs, i, &sym_block[i]);
        n = &node_block[i];

        if (self->queued_xl_enum_filter != 0) {
            Jval jv;
            jv.ui = self->queued_xl_enum_filter;
            self->queued_xl_enum_filter = 0;

            if (self->enum_nptrs_jrb == NULL) {
                self->enum_nptrs_jrb = make_jrb();
            }
            jrb_insert_vptr(self->enum_nptrs_jrb, n, jv);
        }

        n->nname = s->name;

        n->mv.mvlfac = &self->mvlfacs[i];
        self->mvlfacs[i].working_node = n;
        n->vardir = nvd;
        n->varxt = h->u.var.sxt_workspace;
        if ((h->u.var.svt_workspace == FST_SVT_NONE) && (h->u.var.sdt_workspace == FST_SDT_NONE)) {
            n->vartype = nvt;
        } else {
            switch (h->u.var.svt_workspace) {
                case FST_SVT_VHDL_SIGNAL:
                    nvt = GW_VAR_TYPE_VHDL_SIGNAL;
                    break;
                case FST_SVT_VHDL_VARIABLE:
                    nvt = GW_VAR_TYPE_VHDL_VARIABLE;
                    break;
                case FST_SVT_VHDL_CONSTANT:
                    nvt = GW_VAR_TYPE_VHDL_CONSTANT;
                    break;
                case FST_SVT_VHDL_FILE:
                    nvt = GW_VAR_TYPE_VHDL_FILE;
                    break;
                case FST_SVT_VHDL_MEMORY:
                    nvt = GW_VAR_TYPE_VHDL_MEMORY;
                    break;
                default:
                    break; /* keep what exists */
            }
            n->vartype = nvt;

            ndt = fst_supplemental_data_type_to_gw_var_data_type(h->u.var.sdt_workspace);

            n->vardt = ndt;
        }

        if ((f->len > 1) || (f->flags & (GW_FAC_FLAG_DOUBLE | GW_FAC_FLAG_STRING))) {
            n->extvals = 1;
        }

        n->head.time = -1; /* mark 1st node as negative time */
        n->head.v.h_val = GW_BIT_X;
        s->n = n;
    } /* for(i) of facs parsing */

    if (f_name_max_len) {
        g_free(f_name_max_len);
        f_name_max_len = NULL;
    }
    if (nnam) {
        g_free(nnam);
        nnam = NULL;
    }
    if (f_name_build_buf) {
        g_free(f_name_build_buf);
        f_name_build_buf = NULL;
    }

    for (guint i = 0; i <= F_NAME_MODULUS; i++) {
        if (f_name[i]) {
            g_free(f_name[i]);
            f_name[i] = NULL;
        }
    }
    g_free(f_name);
    f_name = NULL;
    g_free(f_name_len);
    f_name_len = NULL;

    if (numvars != numfacs) {
        self->mvlfacs_rvs_alias = g_realloc(self->mvlfacs_rvs_alias, numvars * sizeof(fstHandle));
    }

    /* generate lookup table for typenames explicitly given as attributes */
    gchar **subvar_pnt = NULL;
    if (self->subvar_jrb_count > 0) {
        JRB subvar_jrb_node = NULL;
        subvar_pnt = g_new0(char *, self->subvar_jrb_count + 1);

        jrb_traverse(subvar_jrb_node, self->subvar_jrb)
        {
            subvar_pnt[subvar_jrb_node->val.ui] = subvar_jrb_node->key.s;
        }
    }

    gw_stems_shrink_to_fit(self->stems);

    gw_string_table_freeze(self->component_names);

    fprintf(stderr,
            FST_RDLOAD "Built %d signal%s and %d alias%s.\n",
            numvars,
            (numvars == 1) ? "" : "s",
            numalias,
            (numalias == 1) ? "" : "es");

    // TODO: update splash
    // /* SPLASH */ splash_sync(2, 5);
    fprintf(stderr, FST_RDLOAD "Building facility hierarchy tree.\n");

    // TODO: update splash
    // /* SPLASH */ splash_sync(3, 5);

    fprintf(stderr, FST_RDLOAD "Sorting facility hierarchy tree.\n");

    GwTreeNode *root = gw_tree_builder_build(self->tree_builder);
    GwTree *tree = gw_tree_new(root);
    gw_tree_graft(tree, self->terminals_chain);
    gw_tree_sort(tree);

    // TODO: update splash
    // /* SPLASH */ splash_sync(4, 5);
    gw_facs_order_from_tree(facs, tree);

    // TODO: update splash
    // /* SPLASH */ splash_sync(5, 5);

#if 0
{
int num_dups = 0;
for(i=0;i<GLOBALS->numfacs-1;i++)
	{
	if(!strcmp(GLOBALS->facs[i]->name, GLOBALS->facs[i+1]->name))
		{
		fprintf(stderr, FST_RDLOAD"DUPLICATE FAC: '%s'\n", GLOBALS->facs[i]->name);
		num_dups++;
		}
	}

if(num_dups)
	{
	fprintf(stderr, FST_RDLOAD"Exiting, %d duplicate signals are present.\n", num_dups);
	exit(255);
	}
}
#endif

    /* to avoid bin -> ascii -> bin double swap */
    fstReaderIterBlocksSetNativeDoublesOnCallback(self->fst_reader, 1);

    // TODO: update splash
    // /* SPLASH */ splash_finalize();

    GwTimeRange *time_range;

    if (self->start_time || self->end_time) {
        GwTime b_start = self->first_cycle;
        GwTime b_end = self->last_cycle;

        if (self->start_time != NULL) {
            b_start = unformat_time(self->start_time, self->time_dimension);
        }

        if (self->end_time != NULL) {
            b_end = unformat_time(self->end_time, self->time_dimension);
        }

        b_start = CLAMP(b_start, self->first_cycle, self->last_cycle);
        b_end = CLAMP(b_end, self->first_cycle, self->last_cycle);

        if (b_start > b_end) {
            GwTime tmp_time = b_start;
            b_start = b_end;
            b_end = tmp_time;
        }

        fstReaderSetLimitTimeRange(self->fst_reader, b_start, b_end);

        time_range = gw_time_range_new(b_start, b_end);
    } else {
        time_range = gw_time_range_new(self->first_cycle, self->last_cycle);
    }

    // clang-format off
    GwFstFile *dump_file = g_object_new(GW_TYPE_FST_FILE,
                                        "blackout-regions", blackout_regions,
                                        "stems", self->stems,
                                        "component-names", self->component_names,
                                        "enum-filters", self->enum_filters,
                                        "time-dimension", self->time_dimension,
                                        "time-range", time_range,
                                        "global-time-offset", global_time_offset,
                                        "tree", tree,
                                        "facs", facs,
                                        "has-nonimplicit-directions", self->has_nonimplicit_directions,
                                        "has-supplemental-datatypes", self->has_supplemental_datatypes,
                                        "has-supplemental-vartypes", self->has_supplemental_vartypes,
                                        "uses-vhdl-component-format", fstReaderGetFileType(self->fst_reader) == FST_FT_VHDL,
                                        NULL);
    // clang-format on

    dump_file->fst_reader = g_steal_pointer(&self->fst_reader);
    dump_file->fst_maxhandle = numvars;
    dump_file->fst_table = g_new0(GwLx2Entry, numfacs);
    dump_file->mvlfacs = g_steal_pointer(&self->mvlfacs);
    dump_file->mvlfacs_rvs_alias = g_steal_pointer(&self->mvlfacs_rvs_alias);
    dump_file->subvar_jrb = g_steal_pointer(&self->subvar_jrb);
    dump_file->subvar_pnt = subvar_pnt;
    dump_file->synclock_jrb = g_steal_pointer(&self->synclock_jrb);
    dump_file->enum_nptrs_jrb = g_steal_pointer(&self->enum_nptrs_jrb);
    dump_file->time_scale = self->time_scale;

    g_object_unref(blackout_regions);
    g_object_unref(self->stems);
    g_object_unref(self->component_names);
    g_object_unref(self->enum_filters);
    g_object_unref(tree);
    g_object_unref(time_range);

    return GW_DUMP_FILE(dump_file);
}

static void gw_fst_loader_set_property(GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
    GwFstLoader *self = GW_FST_LOADER(object);

    switch (property_id) {
        case PROP_START_TIME:
            gw_fst_loader_set_start_time(self, g_value_get_string(value));
            break;

        case PROP_END_TIME:
            gw_fst_loader_set_end_time(self, g_value_get_string(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_fst_loader_class_init(GwFstLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GwLoaderClass *loader_class = GW_LOADER_CLASS(klass);

    object_class->dispose = gw_fst_loader_dispose;
    object_class->set_property = gw_fst_loader_set_property;

    loader_class->load = gw_fst_loader_load;

    properties[PROP_START_TIME] =
        gw_param_spec_time("start-time",
                           NULL,
                           NULL,
                           G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    properties[PROP_END_TIME] =
        gw_param_spec_time("end-time",
                           NULL,
                           NULL,
                           G_PARAM_WRITABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_fst_loader_init(GwFstLoader *self)
{
    self->tree_builder = gw_tree_builder_new('.'); // TODO: use prop
    self->stems = gw_stems_new();
    self->component_names = gw_string_table_new();
    self->enum_filters = gw_enum_filter_list_new();
}

GwLoader *gw_fst_loader_new(void)
{
    return g_object_new(GW_TYPE_FST_LOADER, NULL);
}

void gw_fst_loader_set_start_time(GwFstLoader *self, const gchar *start_time)
{
    g_return_if_fail(GW_IS_FST_LOADER(self));

    if (g_strcmp0(self->start_time, start_time) != 0) {
        self->start_time = g_strdup(start_time);

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_START_TIME]);
    }
}

void gw_fst_loader_set_end_time(GwFstLoader *self, const gchar *end_time)
{
    g_return_if_fail(GW_IS_FST_LOADER(self));

    if (g_strcmp0(self->end_time, end_time) != 0) {
        self->end_time = g_strdup(end_time);

        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_END_TIME]);
    }
}

static GwTreeKind fst_scope_type_to_gw_tree_kind(enum fstScopeType scope_type)
{
    switch (scope_type) {
        case FST_ST_VCD_MODULE:
            return GW_TREE_KIND_VCD_ST_MODULE;
        case FST_ST_VCD_TASK:
            return GW_TREE_KIND_VCD_ST_TASK;
        case FST_ST_VCD_FUNCTION:
            return GW_TREE_KIND_VCD_ST_FUNCTION;
        case FST_ST_VCD_BEGIN:
            return GW_TREE_KIND_VCD_ST_BEGIN;
        case FST_ST_VCD_FORK:
            return GW_TREE_KIND_VCD_ST_FORK;
        case FST_ST_VCD_GENERATE:
            return GW_TREE_KIND_VCD_ST_GENERATE;
        case FST_ST_VCD_STRUCT:
            return GW_TREE_KIND_VCD_ST_STRUCT;
        case FST_ST_VCD_UNION:
            return GW_TREE_KIND_VCD_ST_UNION;
        case FST_ST_VCD_CLASS:
            return GW_TREE_KIND_VCD_ST_CLASS;
        case FST_ST_VCD_INTERFACE:
            return GW_TREE_KIND_VCD_ST_INTERFACE;
        case FST_ST_VCD_PACKAGE:
            return GW_TREE_KIND_VCD_ST_PACKAGE;
        case FST_ST_VCD_PROGRAM:
            return GW_TREE_KIND_VCD_ST_PROGRAM;

        case FST_ST_VHDL_ARCHITECTURE:
            return GW_TREE_KIND_VHDL_ST_ARCHITECTURE;
        case FST_ST_VHDL_PROCEDURE:
            return GW_TREE_KIND_VHDL_ST_PROCEDURE;
        case FST_ST_VHDL_FUNCTION:
            return GW_TREE_KIND_VHDL_ST_FUNCTION;
        case FST_ST_VHDL_RECORD:
            return GW_TREE_KIND_VHDL_ST_RECORD;
        case FST_ST_VHDL_PROCESS:
            return GW_TREE_KIND_VHDL_ST_PROCESS;
        case FST_ST_VHDL_BLOCK:
            return GW_TREE_KIND_VHDL_ST_BLOCK;
        case FST_ST_VHDL_FOR_GENERATE:
            return GW_TREE_KIND_VHDL_ST_GENFOR;
        case FST_ST_VHDL_IF_GENERATE:
            return GW_TREE_KIND_VHDL_ST_GENIF;
        case FST_ST_VHDL_GENERATE:
            return GW_TREE_KIND_VHDL_ST_GENERATE;
        case FST_ST_VHDL_PACKAGE:
            return GW_TREE_KIND_VHDL_ST_PACKAGE;

        default:
            return GW_TREE_KIND_UNKNOWN;
    }
}

static GwVarType fst_var_type_to_gw_var_type(enum fstVarType var_type)
{
    switch (var_type) {
        case FST_VT_VCD_EVENT:
            return GW_VAR_TYPE_VCD_EVENT;
        case FST_VT_VCD_INTEGER:
            return GW_VAR_TYPE_VCD_INTEGER;
        case FST_VT_VCD_PARAMETER:
            return GW_VAR_TYPE_VCD_PARAMETER;
        case FST_VT_VCD_REAL:
            return GW_VAR_TYPE_VCD_REAL;
        case FST_VT_VCD_REAL_PARAMETER:
            return GW_VAR_TYPE_VCD_REAL_PARAMETER;
        case FST_VT_VCD_REALTIME:
            return GW_VAR_TYPE_VCD_REALTIME;
        case FST_VT_VCD_REG:
            return GW_VAR_TYPE_VCD_REG;
        case FST_VT_VCD_SUPPLY0:
            return GW_VAR_TYPE_VCD_SUPPLY0;
        case FST_VT_VCD_SUPPLY1:
            return GW_VAR_TYPE_VCD_SUPPLY1;
        case FST_VT_VCD_TIME:
            return GW_VAR_TYPE_VCD_TIME;
        case FST_VT_VCD_TRI:
            return GW_VAR_TYPE_VCD_TRI;
        case FST_VT_VCD_TRIAND:
            return GW_VAR_TYPE_VCD_TRIAND;
        case FST_VT_VCD_TRIOR:
            return GW_VAR_TYPE_VCD_TRIOR;
        case FST_VT_VCD_TRIREG:
            return GW_VAR_TYPE_VCD_TRIREG;
        case FST_VT_VCD_TRI0:
            return GW_VAR_TYPE_VCD_TRI0;
        case FST_VT_VCD_TRI1:
            return GW_VAR_TYPE_VCD_TRI1;
        case FST_VT_VCD_WAND:
            return GW_VAR_TYPE_VCD_WAND;
        case FST_VT_VCD_WIRE:
            return GW_VAR_TYPE_VCD_WIRE;
        case FST_VT_VCD_WOR:
            return GW_VAR_TYPE_VCD_WOR;
        case FST_VT_VCD_PORT:
            return GW_VAR_TYPE_VCD_PORT;

        case FST_VT_GEN_STRING:
            return GW_VAR_TYPE_GEN_STRING;

        case FST_VT_SV_BIT:
            return GW_VAR_TYPE_SV_BIT;
        case FST_VT_SV_LOGIC:
            return GW_VAR_TYPE_SV_LOGIC;
        case FST_VT_SV_INT:
            return GW_VAR_TYPE_SV_INT;
        case FST_VT_SV_SHORTINT:
            return GW_VAR_TYPE_SV_SHORTINT;
        case FST_VT_SV_LONGINT:
            return GW_VAR_TYPE_SV_LONGINT;
        case FST_VT_SV_BYTE:
            return GW_VAR_TYPE_SV_BYTE;
        case FST_VT_SV_ENUM:
            return GW_VAR_TYPE_SV_ENUM;
        case FST_VT_SV_SHORTREAL:
            return GW_VAR_TYPE_SV_SHORTREAL;

        default:
            return GW_VAR_TYPE_UNSPECIFIED_DEFAULT;
    }
}

static GwVarDir fst_var_dir_to_gw_var_dir(enum fstVarDir var_dir)
{
    switch (var_dir) {
        case FST_VD_INPUT:
            return GW_VAR_DIR_IN;
        case FST_VD_OUTPUT:
            return GW_VAR_DIR_OUT;
        case FST_VD_INOUT:
            return GW_VAR_DIR_INOUT;
        case FST_VD_BUFFER:
            return GW_VAR_DIR_BUFFER;
        case FST_VD_LINKAGE:
            return GW_VAR_DIR_LINKAGE;

        case FST_VD_IMPLICIT:
        default:
            return GW_VAR_DIR_IMPLICIT;
    }
}

static GwVarDataType fst_supplemental_data_type_to_gw_var_data_type(
    enum fstSupplementalDataType supplemental_data_type)
{
    switch (supplemental_data_type) {
        case FST_SDT_VHDL_BOOLEAN:
            return GW_VAR_DATA_TYPE_VHDL_BOOLEAN;
        case FST_SDT_VHDL_BIT:
            return GW_VAR_DATA_TYPE_VHDL_BIT;
        case FST_SDT_VHDL_BIT_VECTOR:
            return GW_VAR_DATA_TYPE_VHDL_BIT_VECTOR;
        case FST_SDT_VHDL_STD_ULOGIC:
            return GW_VAR_DATA_TYPE_VHDL_STD_ULOGIC;
        case FST_SDT_VHDL_STD_ULOGIC_VECTOR:
            return GW_VAR_DATA_TYPE_VHDL_STD_ULOGIC_VECTOR;
        case FST_SDT_VHDL_STD_LOGIC:
            return GW_VAR_DATA_TYPE_VHDL_STD_LOGIC;
        case FST_SDT_VHDL_STD_LOGIC_VECTOR:
            return GW_VAR_DATA_TYPE_VHDL_STD_LOGIC_VECTOR;
        case FST_SDT_VHDL_UNSIGNED:
            return GW_VAR_DATA_TYPE_VHDL_UNSIGNED;
        case FST_SDT_VHDL_SIGNED:
            return GW_VAR_DATA_TYPE_VHDL_SIGNED;
        case FST_SDT_VHDL_INTEGER:
            return GW_VAR_DATA_TYPE_VHDL_INTEGER;
        case FST_SDT_VHDL_REAL:
            return GW_VAR_DATA_TYPE_VHDL_REAL;
        case FST_SDT_VHDL_NATURAL:
            return GW_VAR_DATA_TYPE_VHDL_NATURAL;
        case FST_SDT_VHDL_POSITIVE:
            return GW_VAR_DATA_TYPE_VHDL_POSITIVE;
        case FST_SDT_VHDL_TIME:
            return GW_VAR_DATA_TYPE_VHDL_TIME;
        case FST_SDT_VHDL_CHARACTER:
            return GW_VAR_DATA_TYPE_VHDL_CHARACTER;
        case FST_SDT_VHDL_STRING:
            return GW_VAR_DATA_TYPE_VHDL_STRING;
        default:
            return GW_VAR_DATA_TYPE_NONE;
    }
}
