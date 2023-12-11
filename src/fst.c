/*
 * Copyright (c) Tony Bybell 2009-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <stdio.h>
#include "fstapi.h"

#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "fstapi.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"
#include "fst.h"
#include "fst_util.h"
#include "gw-fst-file.h"
#include "gw-fst-file-private.h"

typedef struct
{
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

    struct lx2_entry *fst_table;

    GwFac *mvlfacs;
    fstHandle *mvlfacs_rvs_alias;

    JRB subvar_jrb;
    guint subvar_jrb_count;
    gboolean subvar_jrb_count_locked;

    JRB synclock_jrb;
} FstLoader;

#define FST_RDLOAD "FSTLOAD | "

#define VZT_RD_SYM_F_BITS (0)
#define VZT_RD_SYM_F_INTEGER (1 << 0)
#define VZT_RD_SYM_F_DOUBLE (1 << 1)
#define VZT_RD_SYM_F_STRING (1 << 2)
#define VZT_RD_SYM_F_ALIAS (1 << 3)
#define VZT_RD_SYM_F_SYNVEC \
    (1 << 17) /* reader synthesized vector in alias sec'n from non-adjacent vectorizing */

/******************************************************************/

/*
 * doubles going into histent structs are NEVER freed so this is OK..
 * (we are allocating as many entries that fit in 4k minus the size of the two
 * bookkeeping void* pointers found in the malloc_2/free_2 routines in
 * debug.c)
 */
#ifdef _WAVE_HAVE_JUDY
#define FST_DOUBLE_GRANULARITY ((4 * 1024) / sizeof(double))
#else
#define FST_DOUBLE_GRANULARITY (((4 * 1024) - (2 * sizeof(void *))) / sizeof(double))
#endif

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

static void handle_scope(FstLoader *self, struct fstHierScope *scope)
{
    void *fst_reader = self->fst_reader;

    self->scope_name = fstReaderPushScope(fst_reader, scope->name, GLOBALS->mod_tree_parent);
    self->scope_name_len = fstReaderGetCurrentScopeLen(fst_reader);

    unsigned char ttype = fst_scope_type_to_gw_tree_kind(scope->typ);

    allocate_and_decorate_module_tree_node(ttype,
                                           scope->name,
                                           scope->component,
                                           scope->name_length,
                                           scope->component_length,
                                           self->next_var_stem,
                                           self->next_var_istem);
    self->next_var_stem = 0;
    self->next_var_istem = 0;
}

static void handle_upscope(FstLoader *self)
{
    void *fst_reader = self->fst_reader;

    GLOBALS->mod_tree_parent = fstReaderGetCurrentScopeUserInfo(fst_reader);
    self->scope_name = fstReaderPopScope(fst_reader);
    self->scope_name_len = fstReaderGetCurrentScopeLen(fst_reader);
}

static void handle_var(FstLoader *self,
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

static void handle_supvar(FstLoader *self,
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

    GLOBALS->supplemental_datatypes_encountered = 1;
    if (*svt != FST_SVT_NONE && *svt != FST_SVT_VHDL_SIGNAL) {
        GLOBALS->supplemental_vartypes_encountered = 1;
    }
}

static void handle_sourceistem(FstLoader *self, struct fstHierAttr *attr)
{
    uint32_t istem_path_number = (uint32_t)attr->arg_from_name;
    uint32_t istem_line_number = (uint32_t)attr->arg;

    self->next_var_istem = gw_stems_add_istem(self->stems, istem_path_number, istem_line_number);
}

static void handle_sourcestem(FstLoader *self, struct fstHierAttr *attr)
{
    uint32_t stem_path_number = (uint32_t)attr->arg_from_name;
    uint32_t stem_line_number = (uint32_t)attr->arg;

    self->next_var_stem = gw_stems_add_stem(self->stems, stem_path_number, stem_line_number);
}

static void handle_pathname(FstLoader *self, struct fstHierAttr *attr)
{
    const gchar *path = attr->name;
    guint64 index = attr->arg;

    // Check that path index has the expected value.
    // TODO: add warnings
    if (path != NULL && index == gw_stems_get_next_path_index(self->stems)) {
        gw_stems_add_path(self->stems, path);
    }
}

static void handle_valuelist(FstLoader *self, struct fstHierAttr *attr)
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

static void handle_enumtable(FstLoader *self, struct fstHierAttr *attr)
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

    uint32_t ie;
#ifdef _WAVE_HAVE_JUDY
    Pvoid_t e = (Pvoid_t)NULL;
    for (ie = 0; ie < fe->elem_count; ie++) {
        PPvoid_t pv = JudyHSIns(&e, fe->val_arr[ie], strlen(fe->val_arr[ie]), NULL);
        if (*pv) {
            free_2(*pv);
        }
        *pv = (void *)strdup_2(fe->literal_arr[ie]);
    }
#else
    struct xl_tree_node *e = NULL;

    for (ie = 0; ie < fe->elem_count; ie++) {
        e = xl_insert(fe->val_arr[ie], e, fe->literal_arr[ie]);
    }
#endif

    if (GLOBALS->xl_enum_filter) {
        GLOBALS->num_xl_enum_filter++;
        GLOBALS->xl_enum_filter =
            realloc_2(GLOBALS->xl_enum_filter,
                      GLOBALS->num_xl_enum_filter * sizeof(struct xl_tree_node *));
    } else {
        GLOBALS->num_xl_enum_filter++;
        GLOBALS->xl_enum_filter = malloc_2(sizeof(struct xl_tree_node *));
    }

    GLOBALS->xl_enum_filter[GLOBALS->num_xl_enum_filter - 1] = e;

    if ((unsigned int)GLOBALS->num_xl_enum_filter != attr->arg) {
        // TODO: convert into error/warning
        fprintf(stderr,
                FST_RDLOAD "Internal error, nonsequential enum tables "
                           "definition encountered, exiting.\n");
        exit(255);
    }

    fstUtilityFreeEnumTable(fe);
}

static void handle_attr(FstLoader *self,
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

static struct fstHier *extractNextVar(FstLoader *self,
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

static void fst_append_graft_chain(int len, char *nam, int which, GwTree *par)
{
    GwTree *t = talloc_2(sizeof(GwTree) + len + 1);

    memcpy(t->name, nam, len + 1);
    t->t_which = which;

    t->child = par;
    t->next = GLOBALS->terminals_tchain_tree_c_1;
    GLOBALS->terminals_tchain_tree_c_1 = t;
}

static GwBlackoutRegions *load_blackout_regions(FstLoader *self)
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
 * mainline
 */
GwDumpFile *fst_main(char *fname, char *skip_start, char *skip_end)
{
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
    GwTree *npar = NULL;
    char **f_name = NULL;
    int *f_name_len = NULL, *f_name_max_len = NULL;
    int allowed_to_autocoalesce;
    unsigned int nnam_max = 0;

    int f_name_build_buf_len = 128;
    char *f_name_build_buf = malloc_2(f_name_build_buf_len + 1);

    void *fst_reader = fstReaderOpen(fname);
    if (fst_reader == NULL) {
        /* look at self->fst_reader in caller for success status... */
        return NULL;
    }
    /* SPLASH */ splash_create();

    FstLoader loader = {0};
    FstLoader *self = &loader;
    self->fst_reader = fst_reader;
    self->stems = gw_stems_new();

    allowed_to_autocoalesce = (strstr(fstReaderGetVersionString(fst_reader), "Icarus") == NULL);
    if (!allowed_to_autocoalesce) {
        GLOBALS->autocoalesce = 0;
    }

    GwTimeScaleAndDimension *scale =
        gw_time_scale_and_dimension_from_exponent(fstReaderGetTimescale(fst_reader));
    self->time_dimension = scale->dimension;
    self->time_scale = scale->scale;
    g_free(scale);

    f_name = calloc_2(F_NAME_MODULUS + 1, sizeof(char *));
    f_name_len = calloc_2(F_NAME_MODULUS + 1, sizeof(int));
    f_name_max_len = calloc_2(F_NAME_MODULUS + 1, sizeof(int));

    nnam_max = 16;
    nnam = malloc_2(nnam_max + 1);

    if (fstReaderGetFileType(fst_reader) == FST_FT_VHDL) {
        GLOBALS->is_vhdl_component_format = 1;
    }

    self->subvar_jrb = make_jrb(); /* only used for attributes such as generated in VHDL, etc. */
    self->synclock_jrb = make_jrb(); /* only used for synthetic clocks */

    GLOBALS->numfacs = fstReaderGetVarCount(fst_reader);
    self->mvlfacs = calloc_2(GLOBALS->numfacs, sizeof(GwFac));
    sym_block = calloc_2(GLOBALS->numfacs, sizeof(GwSymbol));
    node_block = calloc_2(GLOBALS->numfacs, sizeof(GwNode));
    GLOBALS->facs = malloc_2(GLOBALS->numfacs * sizeof(GwSymbol *));
    self->mvlfacs_rvs_alias = calloc_2(GLOBALS->numfacs, sizeof(fstHandle));

    hier_auto_enable(); /* enable if greater than threshold */

    init_facility_pack();

    fprintf(stderr, FST_RDLOAD "Processing %d facs.\n", GLOBALS->numfacs);
    /* SPLASH */ splash_sync(1, 5);

    self->first_cycle = fstReaderGetStartTime(fst_reader) * self->time_scale;
    self->last_cycle = fstReaderGetEndTime(fst_reader) * self->time_scale;
    self->total_cycles = self->last_cycle - self->first_cycle + 1;
    GwTime global_time_offset = fstReaderGetTimezero(fst_reader) * self->time_scale;

    GwBlackoutRegions *blackout_regions = load_blackout_regions(self);

    /* do your stuff here..all useful info has been initialized by now */

    if (!GLOBALS->hier_was_explicitly_set) /* set default hierarchy split char */
    {
        GLOBALS->hier_delimeter = '.';
    }

    for (i = 0; i < GLOBALS->numfacs; i++) {
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
            fstReaderIterateHierRewind(fst_reader);
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
                GLOBALS->nonimplicit_direction_encountered = 1;
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

            if (!GLOBALS->do_hier_compress) {
                str = malloc_2(len + 1);
            } else {
                if (len > f_name_build_buf_len) {
                    free_2(f_name_build_buf);
                    f_name_build_buf = malloc_2((f_name_build_buf_len = len) + 1);
                }
                str = f_name_build_buf;
            }

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
                if (!GLOBALS->do_hier_compress) {
                    str = malloc_2(len + 1);
                } else {
                    if (len > f_name_build_buf_len) {
                        free_2(f_name_build_buf);
                        f_name_build_buf = malloc_2((f_name_build_buf_len = len) + 1);
                    }
                    str = f_name_build_buf;
                }

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
                if (!GLOBALS->do_hier_compress) {
                    str = malloc_2(len + 1);
                } else {
                    if (len > f_name_build_buf_len) {
                        free_2(f_name_build_buf);
                        f_name_build_buf = malloc_2((f_name_build_buf_len = len) + 1);
                    }
                    str = f_name_build_buf;
                }

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

        GLOBALS->facs[i] = &sym_block[i];
        n = &node_block[i];

        if (self->queued_xl_enum_filter != 0) {
            Jval jv;
            jv.ui = self->queued_xl_enum_filter;
            self->queued_xl_enum_filter = 0;

            if (!GLOBALS->enum_nptrs_jrb)
                GLOBALS->enum_nptrs_jrb = make_jrb();
            jrb_insert_vptr(GLOBALS->enum_nptrs_jrb, n, jv);
        }

        if (GLOBALS->do_hier_compress) {
            n->nname = compress_facility((unsigned char *)s->name, longest_nam_candidate);
            /* free_2(s->name); ...removed as f_name_build_buf is now used */
            s->name = n->nname;
        } else {
            n->nname = s->name;
        }

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
        n->head.v.h_val = AN_X;
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

    if (numvars != GLOBALS->numfacs) {
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

    decorated_module_cleanup(); /* ...also now in gtk2_treesearch.c */
    freeze_facility_pack();
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
    treegraft(&GLOBALS->treeroot);

    /* SPLASH */ splash_sync(3, 5);

    fprintf(stderr, FST_RDLOAD "Sorting facility hierarchy tree.\n");
    treesort(GLOBALS->treeroot, NULL);
    /* SPLASH */ splash_sync(4, 5);
    order_facs_from_treesort(GLOBALS->treeroot, &GLOBALS->facs);

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

    GLOBALS->min_time = self->first_cycle;
    GLOBALS->max_time = self->last_cycle;
    GLOBALS->is_lx2 = LXT2_IS_FST;

    if (skip_start || skip_end) {
        GwTime b_start, b_end;

        if (!skip_start)
            b_start = GLOBALS->min_time;
        else
            b_start = unformat_time(skip_start, self->time_dimension);
        if (!skip_end)
            b_end = GLOBALS->max_time;
        else
            b_end = unformat_time(skip_end, self->time_dimension);

        if (b_start < GLOBALS->min_time)
            b_start = GLOBALS->min_time;
        else if (b_start > GLOBALS->max_time)
            b_start = GLOBALS->max_time;

        if (b_end < GLOBALS->min_time)
            b_end = GLOBALS->min_time;
        else if (b_end > GLOBALS->max_time)
            b_end = GLOBALS->max_time;

        if (b_start > b_end) {
            GwTime tmp_time = b_start;
            b_start = b_end;
            b_end = tmp_time;
        }

        fstReaderSetLimitTimeRange(fst_reader, b_start, b_end);
        GLOBALS->min_time = b_start;
        GLOBALS->max_time = b_end;
    }

    /* to avoid bin -> ascii -> bin double swap */
    fstReaderIterBlocksSetNativeDoublesOnCallback(fst_reader, 1);

    /* SPLASH */ splash_finalize();

    // clang-format off
    GwFstFile *dump_file = g_object_new(GW_TYPE_FST_FILE,
                                        "blackout-regions", blackout_regions,
                                        "stems", self->stems,
                                        "time-dimension", self->time_dimension,
                                        "global-time-offset", global_time_offset,
                                        NULL);
    // clang-format on

    dump_file->fst_reader = g_steal_pointer(&self->fst_reader);
    dump_file->fst_maxhandle = numvars;
    dump_file->fst_table = calloc_2(GLOBALS->numfacs, sizeof(struct lx2_entry));
    dump_file->mvlfacs = g_steal_pointer(&self->mvlfacs);
    dump_file->mvlfacs_rvs_alias = g_steal_pointer(&self->mvlfacs_rvs_alias);
    dump_file->subvar_jrb = g_steal_pointer(&self->subvar_jrb);
    dump_file->subvar_pnt = subvar_pnt;
    dump_file->synclock_jrb = g_steal_pointer(&self->synclock_jrb);
    dump_file->time_scale = self->time_scale;

    g_object_unref(blackout_regions);
    g_object_unref(self->stems);

    return GW_DUMP_FILE(dump_file);
}
