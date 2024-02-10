#include <config.h>
#include <fstapi.h>
#include "globals.h"
#include "lx2.h"
#include "tree_component.h"
#include "gw-fst-loader.h"
#include "gw-fst-file.h"
#include "gw-fst-file-private.h"
#include "gw-util.h"

static GwTreeKind fst_scope_type_to_gw_tree_kind(enum fstScopeType scope_type);
static GwVarType fst_var_type_to_gw_var_type(enum fstVarType var_type);
static GwVarDir fst_var_dir_to_gw_var_dir(enum fstVarDir var_dir);
static GwVarDataType fst_supplemental_data_type_to_gw_var_data_type(
    enum fstSupplementalDataType supplemental_data_type);

#define FST_RDLOAD "FSTLOAD | "

#define VZT_RD_SYM_F_BITS (0)
#define VZT_RD_SYM_F_INTEGER (1 << 0)
#define VZT_RD_SYM_F_DOUBLE (1 << 1)
#define VZT_RD_SYM_F_STRING (1 << 2)
#define VZT_RD_SYM_F_ALIAS (1 << 3)
#define VZT_RD_SYM_F_SYNVEC \
    (1 << 17) /* reader synthesized vector in alias sec'n from non-adjacent vectorizing */

struct _GwFstLoader
{
    GwLoader parent_instance;

    void *fst_reader;

    const char *scope_name;
    int scope_name_len;

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

    GwTreeNode *tree_root;

    // TODO: don't use strings for start and end time
    gchar *start_time;
    gchar *end_time;

    GwEnumFilterList *enum_filters;

    gboolean has_nonimplicit_directions;
    gboolean has_supplemental_datatypes;
    gboolean has_supplemental_vartypes;
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
    void *fst_reader = self->fst_reader;

    self->scope_name = fstReaderPushScope(fst_reader, scope->name, GLOBALS->mod_tree_parent);
    self->scope_name_len = fstReaderGetCurrentScopeLen(fst_reader);

    unsigned char ttype = fst_scope_type_to_gw_tree_kind(scope->typ);

    allocate_and_decorate_module_tree_node(&self->tree_root,
                                           ttype,
                                           scope->name,
                                           scope->component,
                                           scope->name_length,
                                           scope->component_length,
                                           self->next_var_stem,
                                           self->next_var_istem);
    self->next_var_stem = 0;
    self->next_var_istem = 0;
}

static void handle_upscope(GwFstLoader *self)
{
    void *fst_reader = self->fst_reader;

    GLOBALS->mod_tree_parent = fstReaderGetCurrentScopeUserInfo(fst_reader);
    self->scope_name = fstReaderPopScope(fst_reader);
    self->scope_name_len = fstReaderGetCurrentScopeLen(fst_reader);
}

static void handle_var(GwFstLoader *self,
                       struct fstHierVar *var,
                       gint *msb,
                       gint *lsb,
                       gchar **nam,
                       gint *namlen,
                       guint *nnam_max)
{
    char *pnt;
    char *lb_last = NULL;
    char *col_last = NULL;
    char *rb_last = NULL;

    if (var->name_length > (*nnam_max)) {
        free_2(*nam);
        *nam = malloc_2(((*nnam_max) = var->name_length) + 1);
    }

    char *s = *nam;
    const char *pnts = var->name;
    char *pntd = s;
    while (*pnts) {
        if (*pnts != ' ') {
            if (*pnts == '[') {
                lb_last = pntd;
                col_last = NULL;
                rb_last = NULL;
            } else if (*pnts == ':' && lb_last != NULL && col_last == NULL && rb_last == NULL) {
                col_last = pntd;
            } else if (*pnts == ']' && lb_last != NULL && rb_last == NULL) {
                rb_last = pntd;
            } else if (lb_last != NULL && rb_last == NULL &&
                       (g_ascii_isdigit(*pnts) || (*pnts == '-'))) {
            } else {
                lb_last = NULL;
                col_last = NULL;
                rb_last = NULL;
            }

            *(pntd++) = *pnts;
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
            char *lc_p = attr_pnt = strdup_2(attr->name);

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
                                                       strdup_2(attr_pnt ? attr_pnt : attr->name),
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
            free_2(attr_pnt);
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
        free_2(self->synclock_str);
    }
    self->synclock_str = strdup_2(attr->name);
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
    for (int i = 0; i < fe->elem_count; i++) {
        gw_enum_filter_insert(filter, fe->val_arr[i], fe->literal_arr[i]);
    }

    guint index = gw_enum_filter_list_add(self->enum_filters, filter);

    if (index + 1 != attr->arg) {
        // TODO: convert into error/warning
        fprintf(stderr,
                FST_RDLOAD "Internal error, nonsequential enum tables "
                           "definition encountered, exiting.\n");
        exit(255);
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

static void fst_append_graft_chain(int len, char *nam, int which, GwTreeNode *par)
{
    GwTreeNode *t = talloc_2(sizeof(GwTreeNode) + len + 1);

    memcpy(t->name, nam, len + 1);
    t->t_which = which;

    t->child = par;
    t->next = GLOBALS->terminals_tchain_tree_c_1;
    GLOBALS->terminals_tchain_tree_c_1 = t;
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

static GwDumpFile *gw_fst_loader_load(GwLoader *loader, const char *fname, GError **error)
{
    GwFstLoader *self = GW_FST_LOADER(loader);

    int i;
    GwNode *n;
    GwSymbol *s;
    GwSymbol *prevsymroot = NULL;
    GwSymbol *prevsym = NULL;
    int numalias = 0;
    int numvars = 0;
    GwSymbol *sym_block = NULL;
    GwNode *node_block = NULL;
    struct fstHier *h = NULL;
    int msb, lsb;
    char *nnam = NULL;
    GwTreeNode *npar = NULL;
    char **f_name = NULL;
    int *f_name_len = NULL, *f_name_max_len = NULL;
    int allowed_to_autocoalesce;
    unsigned int nnam_max = 0;

    int f_name_build_buf_len = 128;
    char *f_name_build_buf = malloc_2(f_name_build_buf_len + 1);

    self->fst_reader = fstReaderOpen(fname);
    if (self->fst_reader == NULL) {
        // TODO: set error
        return NULL;
    }
    /* SPLASH */ splash_create();

    allowed_to_autocoalesce =
        (strstr(fstReaderGetVersionString(self->fst_reader), "Icarus") == NULL);
    if (!allowed_to_autocoalesce) {
        GLOBALS->autocoalesce = 0;
    }

    GwTimeScaleAndDimension *scale =
        gw_time_scale_and_dimension_from_exponent(fstReaderGetTimescale(self->fst_reader));
    self->time_dimension = scale->dimension;
    self->time_scale = scale->scale;
    g_free(scale);

    f_name = calloc_2(F_NAME_MODULUS + 1, sizeof(char *));
    f_name_len = calloc_2(F_NAME_MODULUS + 1, sizeof(int));
    f_name_max_len = calloc_2(F_NAME_MODULUS + 1, sizeof(int));

    nnam_max = 16;
    nnam = malloc_2(nnam_max + 1);

    if (fstReaderGetFileType(self->fst_reader) == FST_FT_VHDL) {
        GLOBALS->is_vhdl_component_format = 1;
    }

    self->subvar_jrb = make_jrb(); /* only used for attributes such as generated in VHDL, etc. */
    self->synclock_jrb = make_jrb(); /* only used for synthetic clocks */

    uint numfacs = fstReaderGetVarCount(self->fst_reader);

    GwFacs *facs = gw_facs_new(numfacs);
    self->mvlfacs = calloc_2(numfacs, sizeof(GwFac));
    sym_block = calloc_2(numfacs, sizeof(GwSymbol));
    node_block = calloc_2(numfacs, sizeof(GwNode));
    self->mvlfacs_rvs_alias = calloc_2(numfacs, sizeof(fstHandle));

    fprintf(stderr, FST_RDLOAD "Processing %d facs.\n", numfacs);
    /* SPLASH */ splash_sync(1, 5);

    self->first_cycle = fstReaderGetStartTime(self->fst_reader) * self->time_scale;
    self->last_cycle = fstReaderGetEndTime(self->fst_reader) * self->time_scale;
    self->total_cycles = self->last_cycle - self->first_cycle + 1;
    GwTime global_time_offset = fstReaderGetTimezero(self->fst_reader) * self->time_scale;

    GwBlackoutRegions *blackout_regions = load_blackout_regions(self);

    /* do your stuff here..all useful info has been initialized by now */

    if (!GLOBALS->hier_was_explicitly_set) /* set default hierarchy split char */
    {
        GLOBALS->hier_delimeter = '.';
    }

    for (i = 0; i < numfacs; i++) {
        char buf[65537];
        char *str;
        GwFac *f;
        int hier_len, name_len, tlen;
        unsigned char nvt, nvd, ndt;
        int longest_nam_candidate = 0;
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

        npar = GLOBALS->mod_tree_parent;
        hier_len = self->scope_name ? self->scope_name_len : 0;
        if (hier_len) {
            tlen = hier_len + 1 + name_len;
            if (tlen > f_name_max_len[i & F_NAME_MODULUS]) {
                if (f_name[i & F_NAME_MODULUS])
                    free_2(f_name[i & F_NAME_MODULUS]);
                f_name_max_len[i & F_NAME_MODULUS] = tlen;
                fnam = malloc_2(tlen + 1);
            } else {
                fnam = f_name[i & F_NAME_MODULUS];
            }

            memcpy(fnam, self->scope_name, hier_len);
            fnam[hier_len] = GLOBALS->hier_delimeter;
            memcpy(fnam + hier_len + 1, nnam, name_len + 1);
        } else {
            tlen = name_len;
            if (tlen > f_name_max_len[i & F_NAME_MODULUS]) {
                if (f_name[i & F_NAME_MODULUS])
                    free_2(f_name[i & F_NAME_MODULUS]);
                f_name_max_len[i & F_NAME_MODULUS] = tlen;
                fnam = malloc_2(tlen + 1);
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
                    self->mvlfacs[i].flags = VZT_RD_SYM_F_INTEGER;
                    break;

                case FST_VT_VCD_REAL:
                case FST_VT_VCD_REAL_PARAMETER:
                case FST_VT_VCD_REALTIME:
                case FST_VT_SV_SHORTREAL:
                    self->mvlfacs[i].flags = VZT_RD_SYM_F_DOUBLE;
                    break;

                case FST_VT_GEN_STRING:
                    self->mvlfacs[i].flags = VZT_RD_SYM_F_STRING;
                    self->mvlfacs[i].len = 2;
                    break;

                default:
                    self->mvlfacs[i].flags = VZT_RD_SYM_F_BITS;
                    break;
            }
        } else /* convert any variable length records into strings */
        {
            nvt = GW_VAR_TYPE_GEN_STRING;
            nvd = GW_VAR_DIR_IMPLICIT;
            self->mvlfacs[i].flags = VZT_RD_SYM_F_STRING;
            self->mvlfacs[i].len = 2;
        }

        if (self->synclock_str != NULL) {
            if (self->mvlfacs[i].len == 1) /* currently only for single bit signals */
            {
                Jval syn_jv;

                /* special meaning for this in FST loader--means synthetic signal! */
                self->mvlfacs[i].flags |= VZT_RD_SYM_F_SYNVEC;
                syn_jv.s = self->synclock_str;
                jrb_insert_int(self->synclock_jrb, i, syn_jv);
            } else {
                free_2(self->synclock_str);
            }

            /* under malloc_2() control for true if() branch, so not lost */
            self->synclock_str = NULL;
        }

        if (h->u.var.is_alias) {
            self->mvlfacs[i].node_alias =
                h->u.var.handle - 1; /* subtract 1 to scale it with gtkwave-style numbering */
            self->mvlfacs[i].flags |= VZT_RD_SYM_F_ALIAS;
            numalias++;
        } else {
            self->mvlfacs_rvs_alias[numvars] = i;
            self->mvlfacs[i].node_alias = numvars;
            numvars++;
        }

        f = &self->mvlfacs[i];

        if ((f->len > 1) &&
            (!(f->flags & (VZT_RD_SYM_F_INTEGER | VZT_RD_SYM_F_DOUBLE | VZT_RD_SYM_F_STRING)))) {
            int len = sprintf_2_sdd(buf,
                                    f_name[(i)&F_NAME_MODULUS],
                                    node_block[i].msi,
                                    node_block[i].lsi);

            if (len_subst) /* preserve 2d in name, but make 1d internally */
            {
                node_block[i].msi = h->u.var.length - 1;
                node_block[i].lsi = 0;
            }

            longest_nam_candidate = len;

            str = malloc_2(len + 1);

            if (!GLOBALS->alt_hier_delimeter) {
                memcpy(str, buf, len + 1);
            } else {
                strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
            }
            s = &sym_block[i];
            symadd_name_exists_sym_exists(s, str, 0);
            prevsymroot = prevsym = NULL;

            len = sprintf_2_sdd(buf, nnam, node_block[i].msi, node_block[i].lsi);
            fst_append_graft_chain(len, buf, i, npar);
        } else {
            int gatecmp = (f->len == 1) &&
                          (!(f->flags &
                             (VZT_RD_SYM_F_INTEGER | VZT_RD_SYM_F_DOUBLE | VZT_RD_SYM_F_STRING))) &&
                          (node_block[i].msi != -1) && (node_block[i].lsi != -1);
            int revcmp = gatecmp && (i) &&
                         (f_name_len[(i)&F_NAME_MODULUS] == f_name_len[(i - 1) & F_NAME_MODULUS]) &&
                         (!memrevcmp(f_name_len[(i)&F_NAME_MODULUS],
                                     f_name[(i)&F_NAME_MODULUS],
                                     f_name[(i - 1) & F_NAME_MODULUS]));

            if (gatecmp) {
                int len = sprintf_2_sd(buf, f_name[(i)&F_NAME_MODULUS], node_block[i].msi);

                longest_nam_candidate = len;
                str = malloc_2(len + 1);

                if (!GLOBALS->alt_hier_delimeter) {
                    memcpy(str, buf, len + 1);
                } else {
                    strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
                }
                s = &sym_block[i];
                symadd_name_exists_sym_exists(s, str, 0);
                if ((allowed_to_autocoalesce) && (prevsym) && (revcmp) &&
                    (!strchr(f_name[(i)&F_NAME_MODULUS],
                             '\\'))) /* allow chaining for search functions.. */
                {
                    prevsym->vec_root = prevsymroot;
                    prevsym->vec_chain = s;
                    s->vec_root = prevsymroot;
                    prevsym = s;
                } else {
                    prevsymroot = prevsym = s;
                }

                len = sprintf_2_sd(buf, nnam, node_block[i].msi);
                fst_append_graft_chain(len, buf, i, npar);
            } else {
                int len = f_name_len[(i)&F_NAME_MODULUS];

                longest_nam_candidate = len;
                str = malloc_2(len + 1);

                if (!GLOBALS->alt_hier_delimeter) {
                    memcpy(str, f_name[(i)&F_NAME_MODULUS], len + 1);
                } else {
                    strcpy_vcdalt(str, f_name[(i)&F_NAME_MODULUS], GLOBALS->alt_hier_delimeter);
                }
                s = &sym_block[i];
                symadd_name_exists_sym_exists(s, str, 0);
                prevsymroot = prevsym = NULL;

                if (f->flags & VZT_RD_SYM_F_INTEGER) {
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

                fst_append_graft_chain(strlen(nnam), nnam, i, npar);
            }
        }

        if (longest_nam_candidate > GLOBALS->longestname)
            GLOBALS->longestname = longest_nam_candidate;

        gw_facs_set(facs, i, &sym_block[i]);
        n = &node_block[i];

        if (self->queued_xl_enum_filter != 0) {
            Jval jv;
            jv.ui = self->queued_xl_enum_filter;
            self->queued_xl_enum_filter = 0;

            if (!GLOBALS->enum_nptrs_jrb)
                GLOBALS->enum_nptrs_jrb = make_jrb();
            jrb_insert_vptr(GLOBALS->enum_nptrs_jrb, n, jv);
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

        if ((f->len > 1) || (f->flags & (VZT_RD_SYM_F_DOUBLE | VZT_RD_SYM_F_STRING))) {
            n->extvals = 1;
        }

        n->head.time = -1; /* mark 1st node as negative time */
        n->head.v.h_val = GW_BIT_X;
        s->n = n;
    } /* for(i) of facs parsing */

    if (f_name_max_len) {
        free_2(f_name_max_len);
        f_name_max_len = NULL;
    }
    if (nnam) {
        free_2(nnam);
        nnam = NULL;
    }
    if (f_name_build_buf) {
        free_2(f_name_build_buf);
        f_name_build_buf = NULL;
    }

    for (i = 0; i <= F_NAME_MODULUS; i++) {
        if (f_name[i]) {
            free_2(f_name[i]);
            f_name[i] = NULL;
        }
    }
    free_2(f_name);
    f_name = NULL;
    free_2(f_name_len);
    f_name_len = NULL;

    if (numvars != numfacs) {
        self->mvlfacs_rvs_alias = realloc_2(self->mvlfacs_rvs_alias, numvars * sizeof(fstHandle));
    }

    /* generate lookup table for typenames explicitly given as attributes */
    gchar **subvar_pnt = NULL;
    if (self->subvar_jrb_count > 0) {
        JRB subvar_jrb_node = NULL;
        subvar_pnt = calloc_2(self->subvar_jrb_count + 1, sizeof(char *));

        jrb_traverse(subvar_jrb_node, self->subvar_jrb)
        {
            subvar_pnt[subvar_jrb_node->val.ui] = subvar_jrb_node->key.s;
        }
    }

    gw_stems_shrink_to_fit(self->stems);

    iter_through_comp_name_table();

    fprintf(stderr,
            FST_RDLOAD "Built %d signal%s and %d alias%s.\n",
            numvars,
            (numvars == 1) ? "" : "s",
            numalias,
            (numalias == 1) ? "" : "es");

    /* SPLASH */ splash_sync(2, 5);
    fprintf(stderr, FST_RDLOAD "Building facility hierarchy tree.\n");

    init_tree();

    /* SPLASH */ splash_sync(3, 5);

    fprintf(stderr, FST_RDLOAD "Sorting facility hierarchy tree.\n");

    // TODO: add GwTree to GwDumpFile
    GwTree *tree = gw_tree_new(g_steal_pointer(&self->tree_root));
    gw_tree_graft(tree, GLOBALS->terminals_tchain_tree_c_1);
    gw_tree_sort(tree);

    /* SPLASH */ splash_sync(4, 5);
    gw_facs_order_from_tree(facs, tree);

    /* SPLASH */ splash_sync(5, 5);
    GLOBALS->facs_are_sorted = 1;

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

    GLOBALS->is_lx2 = LXT2_IS_FST;

    /* to avoid bin -> ascii -> bin double swap */
    fstReaderIterBlocksSetNativeDoublesOnCallback(self->fst_reader, 1);

    /* SPLASH */ splash_finalize();

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
                                        "enum-filters", self->enum_filters,
                                        "time-dimension", self->time_dimension,
                                        "time-range", time_range,
                                        "global-time-offset", global_time_offset,
                                        "tree", tree,
                                        "facs", facs,
                                        "has-nonimplicit-directions", self->has_nonimplicit_directions,
                                        "has-supplemental-datatypes", self->has_supplemental_datatypes,
                                        "has-supplemental-vartypes", self->has_supplemental_vartypes,
                                        NULL);
    // clang-format on

    dump_file->fst_reader = g_steal_pointer(&self->fst_reader);
    dump_file->fst_maxhandle = numvars;
    dump_file->fst_table = calloc_2(numfacs, sizeof(GwLx2Entry));
    dump_file->mvlfacs = g_steal_pointer(&self->mvlfacs);
    dump_file->mvlfacs_rvs_alias = g_steal_pointer(&self->mvlfacs_rvs_alias);
    dump_file->subvar_jrb = g_steal_pointer(&self->subvar_jrb);
    dump_file->subvar_pnt = subvar_pnt;
    dump_file->synclock_jrb = g_steal_pointer(&self->synclock_jrb);
    dump_file->time_scale = self->time_scale;

    g_object_unref(blackout_regions);
    g_object_unref(self->stems);
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
    self->stems = gw_stems_new();
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
