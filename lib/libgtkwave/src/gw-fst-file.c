#include <fstapi.h>
#include "gw-fst-file.h"
#include "gw-fst-file-private.h"

#define FST_RDLOAD "FSTLOAD | "

G_DEFINE_TYPE(GwFstFile, gw_fst_file, GW_TYPE_DUMP_FILE)

static void gw_fst_file_import_trace(GwFstFile *self, GwNode *np);
static void gw_fst_file_set_fac_process_mask(GwFstFile *self, GwNode *np);
static void gw_fst_file_import_masked(GwFstFile *self);

static void gw_fst_file_dispose(GObject *object)
{
    GwFstFile *self = GW_FST_FILE(object);

    g_clear_object(&self->hist_ent_factory);

    G_OBJECT_CLASS(gw_fst_file_parent_class)->dispose(object);
}

static void gw_fst_file_finalize(GObject *object)
{
    GwFstFile *self = GW_FST_FILE(object);

    g_clear_pointer(&self->fst_reader, fstReaderClose);
    g_clear_pointer(&self->subvar_jrb, jrb_free_tree);
    g_clear_pointer(&self->synclock_jrb, jrb_free_tree);
    g_clear_pointer(&self->enum_nptrs_jrb, jrb_free_tree);

    G_OBJECT_CLASS(gw_fst_file_parent_class)->finalize(object);
}

static gboolean gw_fst_file_import_traces(GwDumpFile *dump_file, GwNode **nodes, GError **error)
{
    GwFstFile *self = GW_FST_FILE(dump_file);
    (void)error;

    for (GwNode **iter = nodes; *iter != NULL; iter++) {
        GwNode *node = *iter;

        gw_fst_file_set_fac_process_mask(self, node);
    }
    gw_fst_file_import_masked(self);

    return TRUE;
}

static guint gw_fst_file_get_enum_filter_for_node(GwDumpFile *dump_file, GwNode *node)
{
    GwFstFile *self = GW_FST_FILE(dump_file);

    if (self->enum_nptrs_jrb == NULL) {
        return 0;
    }

    JRB enum_nptr = jrb_find_vptr(self->enum_nptrs_jrb, node);

    return enum_nptr != NULL ? enum_nptr->val.ui : 0;
}

static void gw_fst_file_class_init(GwFstFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GwDumpFileClass *dump_file_class = GW_DUMP_FILE_CLASS(klass);

    object_class->dispose = gw_fst_file_dispose;
    object_class->finalize = gw_fst_file_finalize;

    dump_file_class->import_traces = gw_fst_file_import_traces;
    dump_file_class->get_enum_filter_for_node = gw_fst_file_get_enum_filter_for_node;
}

static void gw_fst_file_init(GwFstFile *self)
{
    self->hist_ent_factory = gw_hist_ent_factory_new();
}

/*
 * conversion from evcd -> vcd format
 */
static void evcd_memcpy(char *dst, const char *src, int len)
{
    static const char *evcd = "DUNZduLHXTlh01?FAaBbCcf";
    static const char *vcd = "01xz0101xz0101xzxxxxxxz";

    char ch;
    int i, j;

    for (j = 0; j < len; j++) {
        ch = *src;
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
}

/*
 * fst callback (only does bits for now)
 */
static void fst_callback2(void *user_callback_data_pointer,
                          uint64_t tim,
                          fstHandle txidx,
                          const unsigned char *value,
                          uint32_t plen)
{
    GwFstFile *self = user_callback_data_pointer;

    fstHandle facidx = self->mvlfacs_rvs_alias[--txidx];
    GwHistEnt *htemp;
    GwLx2Entry *l2e = &self->fst_table[facidx];
    GwFac *f = &self->mvlfacs[facidx];

    // TODO: report progress
    // self->busycnt++;
    // if (self->busycnt == WAVE_BUSY_ITER) {
    //     busy_window_refresh();
    //     self->busycnt = 0;
    // }

    /* fprintf(stderr, "%lld %d '%s'\n", tim, facidx, value); */

    if (!(f->flags & (GW_FAC_FLAG_DOUBLE | GW_FAC_FLAG_STRING))) {
        unsigned char vt = GW_VAR_TYPE_UNSPECIFIED_DEFAULT;
        if (f->working_node) {
            vt = f->working_node->vartype;
        }

        if (f->len > 1) {
            char *h_vector = g_malloc(f->len);
            if (vt != GW_VAR_TYPE_VCD_PORT) {
                memcpy(h_vector, value, f->len);
            } else {
                evcd_memcpy(h_vector, (const char *)value, f->len);
            }

            for (gint i = 0; i < f->len; i++) {
                h_vector[i] = gw_bit_from_char(h_vector[i]);
            }

            if ((l2e->histent_curr) &&
                (l2e->histent_curr->v.h_vector)) /* remove duplicate values */
            {
                if ((!memcmp(l2e->histent_curr->v.h_vector, h_vector, f->len)) &&
                    (!self->preserve_glitches)) {
                    g_free(h_vector);
                    return;
                }
            }

            htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            htemp->v.h_vector = h_vector;
        } else {
            unsigned char h_val;

            if (vt != GW_VAR_TYPE_VCD_PORT) {
                h_val = gw_bit_from_char(*value);
            } else {
                char membuf[1];
                evcd_memcpy(membuf, (const char *)value, 1);
                switch (*membuf) {
                    case '0':
                        h_val = GW_BIT_0;
                        break;
                    case '1':
                        h_val = GW_BIT_1;
                        break;
                    case 'Z':
                    case 'z':
                        h_val = GW_BIT_Z;
                        break;
                    default:
                        h_val = GW_BIT_X;
                        break;
                }
            }

            if ((vt != GW_VAR_TYPE_VCD_EVENT) && (l2e->histent_curr)) /* remove duplicate values */
            {
                if ((l2e->histent_curr->v.h_val == h_val) && (!self->preserve_glitches)) {
                    return;
                }
            }

            htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            htemp->v.h_val = h_val;
        }
    } else if (f->flags & GW_FAC_FLAG_DOUBLE) {
        if ((l2e->histent_curr) && (l2e->histent_curr->v.h_vector)) /* remove duplicate values */
        {
            if (!memcmp(&l2e->histent_curr->v.h_double, value, sizeof(double))) {
                if ((!self->preserve_glitches) && (!self->preserve_glitches_real)) {
                    return;
                }
            }
        }

        /* if(fstReaderIterBlocksSetNativeDoublesOnCallback is disabled...)

        double *d = double_slab_calloc();
        sscanf(value, "%lg", d);
        htemp = histent_calloc();
        htemp->v.h_vector = (char *)d;

        otherwise...
        */

        htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        memcpy(&htemp->v.h_double, value, sizeof(double));
        htemp->flags = GW_HIST_ENT_FLAG_REAL;
    } else /* string */
    {
        unsigned char *s = g_malloc(plen + 1);
        uint32_t pidx;

        for (pidx = 0; pidx < plen; pidx++) {
            unsigned char ch = value[pidx];

#if 0
		/* for now do not convert to printable unless done in vcd + lxt loaders also */
		if((ch < ' ') || (ch > '~'))
			{
			ch = '.';
			}
#endif

            s[pidx] = ch;
        }
        s[pidx] = 0;

        if ((l2e->histent_curr) && (l2e->histent_curr->v.h_vector)) /* remove duplicate values */
        {
            if ((!strcmp(l2e->histent_curr->v.h_vector, (const char *)value)) &&
                (!self->preserve_glitches)) {
                g_free(s);
                return;
            }
        }

        htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        htemp->v.h_vector = (char *)s;
        htemp->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
    }

    htemp->time = (tim) * (self->time_scale);

    if (l2e->histent_curr) /* scan-build : was l2e->histent_head */
    {
        l2e->histent_curr->next = htemp; /* scan-build : this is ok given how it's used */
        l2e->histent_curr = htemp;
    } else {
        l2e->histent_head = l2e->histent_curr = htemp;
    }

    l2e->numtrans++;
}

static void fst_callback(void *user_callback_data_pointer,
                         uint64_t tim,
                         fstHandle txidx,
                         const unsigned char *value)
{
    fst_callback2(user_callback_data_pointer, tim, txidx, value, 0);
}

/*
 * this is the black magic that handles aliased signals...
 */
static void fst_resolver(GwNode *np, GwNode *resolve)
{
    np->extvals = resolve->extvals;
    np->msi = resolve->msi;
    np->lsi = resolve->lsi;
    memcpy(&np->head, &resolve->head, sizeof(GwHistEnt));
    np->curr = resolve->curr;
    np->harray = resolve->harray;
    np->numhist = resolve->numhist;
    np->mv.mvlfac = NULL;
}

/*
 * actually import a fst trace but don't do it if it's already been imported
 */
static void gw_fst_file_import_trace(GwFstFile *self, GwNode *np)
{
    GwHistEnt *htemp;
    GwHistEnt *htempx = NULL;
    GwHistEnt *histent_tail;
    int len, i;
    GwFac *f;
    int txidx;
    GwNode *nold = np;

    if (!(f = np->mv.mvlfac))
        return; /* already imported */

    txidx = f - self->mvlfacs;
    if (np->mv.mvlfac->flags & GW_FAC_FLAG_ALIAS) {
        /* this is to map to fstHandles, so even non-aliased are remapped */
        txidx = self->mvlfacs[txidx].node_alias;
        txidx = self->mvlfacs_rvs_alias[txidx];
        np = self->mvlfacs[txidx].working_node;

        if (!(f = np->mv.mvlfac)) {
            fst_resolver(nold, np);
            return; /* already imported */
        }
    }

    if (!(f->flags & GW_FAC_FLAG_SYNVEC)) /* block debug message for synclk */
    {
        fprintf(stderr, "Import: %s\n", np->nname); /* normally this never happens */
    }

    /* new stuff */
    len = np->mv.mvlfac->len;

    /* check here for array height in future */

    if (!(f->flags & GW_FAC_FLAG_SYNVEC)) {
        fstReaderSetFacProcessMask(self->fst_reader, self->mvlfacs[txidx].node_alias + 1);
        fstReaderIterBlocks2(self->fst_reader, fst_callback, fst_callback2, self, NULL);
        fstReaderClrFacProcessMask(self->fst_reader, self->mvlfacs[txidx].node_alias + 1);
    }

    histent_tail = htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
    if (len > 1) {
        htemp->v.h_vector = g_malloc(len);
        for (i = 0; i < len; i++)
            htemp->v.h_vector[i] = GW_BIT_Z;
    } else {
        htemp->v.h_val = GW_BIT_Z; /* z */
    }
    htemp->time = GW_TIME_MAX;

    htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
    if (len > 1) {
        if (!(f->flags & GW_FAC_FLAG_DOUBLE)) {
            if (!(f->flags & GW_FAC_FLAG_STRING)) {
                htemp->v.h_vector = g_malloc(len);
                for (i = 0; i < len; i++)
                    htemp->v.h_vector[i] = GW_BIT_X;
            } else {
                htemp->v.h_vector = g_strdup("UNDEF");
                htemp->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
            }
        } else {
            htemp->v.h_double = strtod("NaN", NULL);
            htemp->flags = GW_HIST_ENT_FLAG_REAL;
        }
        htempx = htemp;
    } else {
        htemp->v.h_val = GW_BIT_X; /* x */
        htempx = htemp;
    }
    htemp->time = GW_TIME_MAX - 1;
    htemp->next = histent_tail;

    if (self->fst_table[txidx].histent_curr) {
        self->fst_table[txidx].histent_curr->next = htemp;
        htemp = self->fst_table[txidx].histent_head;
    }

    if (!(f->flags & (GW_FAC_FLAG_DOUBLE | GW_FAC_FLAG_STRING))) {
        if (len > 1) {
            np->head.v.h_vector = g_malloc(len);
            for (i = 0; i < len; i++)
                np->head.v.h_vector[i] = GW_BIT_X;
        } else {
            np->head.v.h_val = GW_BIT_X; /* x */
        }
    } else {
        np->head.flags = GW_HIST_ENT_FLAG_REAL;
        if (f->flags & GW_FAC_FLAG_STRING) {
            np->head.flags |= GW_HIST_ENT_FLAG_STRING;
        }
    }

    {
        GwHistEnt *htemp2 = gw_hist_ent_factory_alloc(self->hist_ent_factory);
        htemp2->time = -1;
        if (len > 1) {
            htemp2->v.h_vector = htempx->v.h_vector;
            htemp2->flags = htempx->flags;
        } else {
            htemp2->v.h_val = htempx->v.h_val;
        }
        htemp2->next = htemp;
        htemp = htemp2;
        self->fst_table[txidx].numtrans++;
    }

    np->head.time = -2;
    np->head.next = htemp;
    np->numhist = self->fst_table[txidx].numtrans + 2 /*endcap*/ + 1 /*frontcap*/;

    memset(&self->fst_table[txidx], 0, sizeof(GwLx2Entry)); /* zero it out */

    np->curr = histent_tail;
    np->mv.mvlfac = NULL; /* it's imported and cached so we can forget it's an mvlfac now */

    if (nold != np) {
        fst_resolver(nold, np);
    }
}

/*
 * decompress [m b xs xe valstring]... format string into trace
 */
static void expand_synvec(GwFstFile *self, int txidx, const char *s)
{
    char *scopy = NULL;
    char *pnt, *pnt2;
    double m, b;
    uint64_t xs, xe, xi;
    char *vs;
    uint64_t tim;
    uint64_t tim_max;
    int vslen;
    int vspnt;
    unsigned char value[2] = {0, 0};
    unsigned char pval = 0;

    scopy = g_strdup(s);
    vs = g_malloc0(strlen(s) + 1); /* will never be as big as original string */
    pnt = scopy;

    while (*pnt) {
        if (*pnt != '[') {
            pnt++;
            continue;
        }
        pnt++;

        pnt2 = strchr(pnt, ']');
        if (!pnt2)
            break;
        *pnt2 = 0;

        /* printf("PNT: %s\n", pnt); */
        int rc = sscanf(pnt, "%lg %lg %" SCNu64 " %" SCNu64 " %s", &m, &b, &xs, &xe, vs);
        if (rc == 5) {
            vslen = strlen(vs);
            vspnt = 0;

            tim_max = 0;
            for (xi = xs; xi <= xe; xi++) {
                tim = (xi * m) + b;
                /* fprintf(stderr, "#%"PRIu64" '%c'\n", tim, vs[vspnt]); */
                value[0] = vs[vspnt];
                if (value[0] != pval) /* collapse new == old value transitions so new is ignored */
                {
                    if ((tim >= tim_max) || (xi == xs)) {
                        fst_callback2(self, tim, txidx, value, 0);
                        tim_max = tim;
                    }
                    pval = value[0];
                }
                vspnt++;
                vspnt = (vspnt == vslen) ? 0 : vspnt; /* modulus on repeating clock */
            }
        } else {
            break;
        }

        pnt = pnt2 + 1;
    }

    g_free(vs);
    g_free(scopy);
}

/*
 * pre-import many traces at once so function above doesn't have to iterate...
 */
static void gw_fst_file_set_fac_process_mask(GwFstFile *self, GwNode *np)
{
    GwFac *f;
    int txidx;

    if (!(f = np->mv.mvlfac))
        return; /* already imported */

    txidx = f - self->mvlfacs;

    if (np->mv.mvlfac->flags & GW_FAC_FLAG_ALIAS) {
        txidx = self->mvlfacs[txidx].node_alias;
        txidx = self->mvlfacs_rvs_alias[txidx];
        GwNode *nold = np;
        np = self->mvlfacs[txidx].working_node;

        if (!(np->mv.mvlfac)) {
            fst_resolver(nold, np);
            return; /* already imported */
        }
    }

    if (np->mv.mvlfac->flags & GW_FAC_FLAG_SYNVEC) {
        JRB fi = jrb_find_int(self->synclock_jrb, txidx);
        if (fi) {
            expand_synvec(self, self->mvlfacs[txidx].node_alias + 1, fi->val.s);
            gw_fst_file_import_trace(self, np);
            return; /* import_fst_trace() will construct the trailer */
        }
    }

    /* check here for array height in future */
    {
        fstReaderSetFacProcessMask(self->fst_reader, self->mvlfacs[txidx].node_alias + 1);
        self->fst_table[txidx].np = np;
    }
}

static void gw_fst_file_import_masked(GwFstFile *self)
{
    unsigned int txidxi;
    int i, cnt;
    GwHistEnt *htempx = NULL;

    cnt = 0;
    for (txidxi = 0; txidxi < self->fst_maxhandle; txidxi++) {
        if (fstReaderGetFacProcessMask(self->fst_reader, txidxi + 1)) {
            cnt++;
        }
    }

    if (!cnt) {
        return;
    }

    if (cnt > 100) {
        fprintf(stderr, FST_RDLOAD "Extracting %d traces\n", cnt);
    }

    // TODO: report progress
    // set_window_busy(NULL);

    fstReaderIterBlocks2(self->fst_reader, fst_callback, fst_callback2, self, NULL);

    // TODO: report progress
    // set_window_idle(NULL);

    for (txidxi = 0; txidxi < self->fst_maxhandle; txidxi++) {
        if (fstReaderGetFacProcessMask(self->fst_reader, txidxi + 1)) {
            int txidx = self->mvlfacs_rvs_alias[txidxi];
            GwHistEnt *htemp, *histent_tail;
            GwFac *f = &self->mvlfacs[txidx];
            int len = f->len;
            GwNode *np = self->fst_table[txidx].np;

            histent_tail = htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            if (len > 1) {
                htemp->v.h_vector = g_malloc(len);
                for (i = 0; i < len; i++) {
                    if (f->flags & GW_FAC_FLAG_STRING) {
                        htemp->v.h_vector[i] = 0;
                        htemp->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
                    } else {
                        htemp->v.h_vector[i] = GW_BIT_Z;
                    }
                }
            } else {
                htemp->v.h_val = GW_BIT_Z; /* z */
            }
            htemp->time = GW_TIME_MAX;

            htemp = gw_hist_ent_factory_alloc(self->hist_ent_factory);
            if (len > 1) {
                if (!(f->flags & GW_FAC_FLAG_DOUBLE)) {
                    if (!(f->flags & GW_FAC_FLAG_STRING)) {
                        htemp->v.h_vector = g_malloc(len);
                        for (i = 0; i < len; i++)
                            htemp->v.h_vector[i] = GW_BIT_X;
                    } else {
                        htemp->v.h_vector = g_strdup("UNDEF");
                        htemp->flags = GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING;
                    }
                    htempx = htemp;
                } else {
                    htemp->v.h_double = strtod("NaN", NULL);
                    htemp->flags = GW_HIST_ENT_FLAG_REAL;
                    htempx = htemp;
                }
            } else {
                htemp->v.h_val = GW_BIT_X; /* x */
                htempx = htemp;
            }
            htemp->time = GW_TIME_MAX - 1;
            htemp->next = histent_tail;

            if (self->fst_table[txidx].histent_curr) {
                self->fst_table[txidx].histent_curr->next = htemp;
                htemp = self->fst_table[txidx].histent_head;
            }

            if (!(f->flags & (GW_FAC_FLAG_DOUBLE | GW_FAC_FLAG_STRING))) {
                if (len > 1) {
                    np->head.v.h_vector = g_malloc(len);
                    for (i = 0; i < len; i++)
                        np->head.v.h_vector[i] = GW_BIT_X;
                } else {
                    np->head.v.h_val = GW_BIT_X; /* x */
                }
            } else {
                np->head.flags = GW_HIST_ENT_FLAG_REAL;
                if (f->flags & GW_FAC_FLAG_STRING) {
                    np->head.flags |= GW_HIST_ENT_FLAG_STRING;
                }
            }

            {
                GwHistEnt *htemp2 = gw_hist_ent_factory_alloc(self->hist_ent_factory);
                htemp2->time = -1;
                if (len > 1) {
                    htemp2->v.h_vector = htempx->v.h_vector;
                    htemp2->flags = htempx->flags;
                } else {
                    htemp2->v.h_val = htempx->v.h_val;
                }
                htemp2->next = htemp;
                htemp = htemp2;
                self->fst_table[txidx].numtrans++;
            }

            np->head.time = -2;
            np->head.next = htemp;
            np->numhist = self->fst_table[txidx].numtrans + 2 /*endcap*/ + 1 /*frontcap*/;

            memset(&self->fst_table[txidx], 0, sizeof(GwLx2Entry)); /* zero it out */

            np->curr = histent_tail;
            np->mv.mvlfac = NULL; /* it's imported and cached so we can forget it's an mvlfac now */
            fstReaderClrFacProcessMask(self->fst_reader, txidxi + 1);
        }
    }
}

gchar *gw_fst_file_get_subvar(GwFstFile *self, gint index)
{
    g_return_val_if_fail(self != NULL, NULL);

    return self->subvar_pnt[index];
}
