/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */
#if defined _AIX
#pragma alloca
#endif

#include <config.h>
#include "globals.h"
#include "tree.h"
#include "tree_component.h"
#include "vcd.h"

enum TreeBuildTypes
{
    MAKETREE_FLATTEN,
    MAKETREE_LEAF,
    MAKETREE_NODE
};

#ifdef WAVE_USE_STRUCT_PACKING
GwTreeNode *talloc_2(size_t siz)
{
    if (GLOBALS->talloc_pool_base) {
        if ((siz + GLOBALS->talloc_idx) <= WAVE_TALLOC_POOL_SIZE) {
            unsigned char *m = GLOBALS->talloc_pool_base + GLOBALS->talloc_idx;
            GLOBALS->talloc_idx += siz;
            return ((GwTreeNode *)m);
        } else if (siz >= WAVE_TALLOC_ALTREQ_SIZE) {
            return (calloc_2(1, siz));
        }
    }

    GLOBALS->talloc_pool_base = calloc_2(1, WAVE_TALLOC_POOL_SIZE);
    GLOBALS->talloc_idx = 0;
    return (talloc_2(siz));
}
#endif

/*
 * init pointers needed for n-way tree
 */
void init_tree(void)
{
    GLOBALS->module_tree_c_1 = (char *)malloc_2(GLOBALS->longestname + 1);
}

/*
 * extract the next part of the name in the flattened
 * hierarchy name.  return ptr to next name if it exists
 * else NULL
 */
static const char *get_module_name(const char *s)
{
    char ch;
    char *pnt;

    pnt = GLOBALS->module_tree_c_1;

    for (;;) {
        ch = *(s++);

        if (((ch == GLOBALS->hier_delimeter) || (ch == '|')) &&
            (*s)) /* added && null check to allow possible . at end of name */
        {
            *(pnt) = 0;
            GLOBALS->module_len_tree_c_1 = pnt - GLOBALS->module_tree_c_1;
            return (s);
        }

        if (!(*(pnt++) = ch)) {
            GLOBALS->module_len_tree_c_1 = pnt - GLOBALS->module_tree_c_1;
            return (NULL); /* nothing left to extract */
        }
    }
}

/*
 * generate unique hierarchy pointer faster that sprintf("%p")
 * use 7-bit string to generate less characters
 */
#ifdef _WAVE_HAVE_JUDY
static int gen_hier_string(char *dest, void *pnt)
{
    uintptr_t p = (uintptr_t)(pnt);
    char *dest_copy = dest;

    while (p) {
        *(dest++) = (p & 0x7f) | 0x80;
        p >>= 7;
    }
    *(dest++) = '.';

    return (dest - dest_copy);
}
#endif

/*
 * decorated module cleanup (if judy active)
 */
int decorated_module_cleanup(void)
{
#ifdef _WAVE_HAVE_JUDY
    if (GLOBALS->sym_tree) {
        JudySLFreeArray(&GLOBALS->sym_tree, PJE0);
    }

    if (GLOBALS->sym_tree_addresses) {
        int rcValue;
        Word_t Index = 0;

        for (rcValue = Judy1First(GLOBALS->sym_tree_addresses, &Index, PJE0); rcValue != 0;
             rcValue = Judy1Next(GLOBALS->sym_tree_addresses, &Index, PJE0)) {
            ((GwTreeNode *)Index)->children_in_gui = 0;
        }

        Judy1FreeArray(&GLOBALS->sym_tree_addresses, PJE0);
    }

#endif
    return (1);
}

/*
 * decorated module add
 */
void allocate_and_decorate_module_tree_node(GwTreeNode **tree_root,
                                            unsigned char ttype,
                                            const char *scopename,
                                            const char *compname,
                                            uint32_t scopename_len,
                                            uint32_t compname_len,
                                            uint32_t t_stem,
                                            uint32_t t_istem)
{
    GwTreeNode *t;
    int mtyp = WAVE_T_WHICH_UNDEFINED_COMPNAME;
#ifdef _WAVE_HAVE_JUDY
    char str[2048];
#endif

    if (compname && compname[0] && strcmp(scopename, compname)) {
        int ix = add_to_comp_name_table(compname, compname_len);
        if (ix) {
            ix--;
            mtyp = WAVE_T_WHICH_COMPNAME_START - ix;
        }
    }

    if (*tree_root != NULL) {
        if (GLOBALS->mod_tree_parent) {
#ifdef _WAVE_HAVE_JUDY
            if (GLOBALS->mod_tree_parent->children_in_gui) {
                PPvoid_t PPValue;
                /* find with judy */
                int len = gen_hier_string(str, GLOBALS->mod_tree_parent);
                strcpy(str + len, scopename);
                PPValue = JudySLIns(&GLOBALS->sym_tree, (uint8_t *)str, PJE0);
                if (*PPValue) {
                    GLOBALS->mod_tree_parent = *PPValue;
                    return;
                }

                t = talloc_2(sizeof(GwTreeNode) + scopename_len + 1);
                *PPValue = t;
                goto t_allocated;
            } else {
                int dep = 0;
#endif
                t = GLOBALS->mod_tree_parent->child;
                while (t) {
                    if (!strcmp(t->name, scopename)) {
                        GLOBALS->mod_tree_parent = t;
                        return;
                    }
                    t = t->next;
#ifdef _WAVE_HAVE_JUDY
                    dep++;
#endif
                }

#ifdef _WAVE_HAVE_JUDY
                if (dep >= FST_TREE_SEARCH_NEXT_LIMIT) {
                    PPvoid_t PPValue;
                    int len = gen_hier_string(str, GLOBALS->mod_tree_parent);
                    GLOBALS->mod_tree_parent->children_in_gui = 1; /* "borrowed" for tree build */
                    t = GLOBALS->mod_tree_parent->child;

                    Judy1Set((Pvoid_t)&GLOBALS->sym_tree_addresses,
                             (Word_t)GLOBALS->mod_tree_parent,
                             PJE0);
                    /* assemble judy based on scopename + GLOBALS->mod_tree_parent pnt */
                    while (t) {
                        strcpy(str + len, t->name);
                        PPValue = JudySLIns(&GLOBALS->sym_tree, (uint8_t *)str, PJE0);
                        *PPValue = t;

                        t = t->next;
                    }

                    strcpy(str + len, scopename);
                    PPValue = JudySLIns(&GLOBALS->sym_tree, (uint8_t *)str, PJE0);
                    t = talloc_2(sizeof(GwTreeNode) + scopename_len + 1);
                    *PPValue = t;
                    goto t_allocated;
                }
            }
#endif

            t = talloc_2(sizeof(GwTreeNode) + scopename_len + 1);
#ifdef _WAVE_HAVE_JUDY
        t_allocated:
#endif
            strcpy(t->name, scopename);
            t->kind = ttype;
            t->t_which = mtyp;
            t->t_stem = t_stem;
            t->t_istem = t_istem;

            if (GLOBALS->mod_tree_parent->child) {
                t->next = GLOBALS->mod_tree_parent->child;
            }
            GLOBALS->mod_tree_parent->child = t;
            GLOBALS->mod_tree_parent = t;
        } else {
            t = *tree_root;
            while (t) {
                if (!strcmp(t->name, scopename)) {
                    GLOBALS->mod_tree_parent = t;
                    return;
                }
                t = t->next;
            }

            t = talloc_2(sizeof(GwTreeNode) + scopename_len + 1);
            strcpy(t->name, scopename);
            t->kind = ttype;
            t->t_which = mtyp;
            t->t_stem = t_stem;
            t->t_istem = t_istem;

            t->next = *tree_root;
            *tree_root = t;
            GLOBALS->mod_tree_parent = t;
        }
    } else {
        t = talloc_2(sizeof(GwTreeNode) + scopename_len + 1);
        strcpy(t->name, scopename);
        t->kind = ttype;
        t->t_which = mtyp;
        t->t_stem = t_stem;
        t->t_istem = t_istem;

        *tree_root = t;
        GLOBALS->mod_tree_parent = t;
    }
}

/*
 * unswizzle extended names in tree
 */
void treenamefix_str(char *s)
{
    while (*s) {
        if (*s == VCDNAM_ESCAPE)
            *s = GLOBALS->hier_delimeter;
        s++;
    }
}

void treenamefix(GwTreeNode *t)
{
    GwTreeNode *tnext;
    if (t->child)
        treenamefix(t->child);

    tnext = t->next;

    while (tnext) {
        if (tnext->child)
            treenamefix(tnext->child);
        treenamefix_str(tnext->name);
        tnext = tnext->next;
    }

    treenamefix_str(t->name);
}

/*
 * for debugging purposes only
 */
void treedebug(GwTreeNode *t, char *s)
{
    while (t) {
        char *s2;

        s2 = (char *)malloc_2(strlen(s) + strlen(t->name) + 2);
        strcpy(s2, s);
        strcat(s2, ".");
        strcat(s2, t->name);

        if (t->child) {
            treedebug(t->child, s2);
        }

        if (t->t_which >=
            0) /* for when valid netnames like A.B.C, A.B.C.D exist (not legal excluding texsim) */
        /* otherwise this would be an 'else' */
        {
            printf("%3d) %s\n", t->t_which, s2);
        }

        free_2(s2);
        t = t->next;
    }
}

/*
 * return least significant member name of a hierarchy
 * (used for tree and hier vec_root search hits)
 */
char *leastsig_hiername(char *nam)
{
    char *t, *pnt = NULL;
    char ch;

    if (nam) {
        t = nam;
        while ((ch = *(t++))) {
            if (ch == GLOBALS->hier_delimeter)
                pnt = t;
        }
    }

    return (pnt ? pnt : nam);
}

/**********************************/
/* Experimental treesorting code  */
/* (won't directly work with lxt2 */
/* because alias hier is after    */
/* fac hier so fix with partial   */
/* mergesort...)                  */
/**********************************/

/*
 * sort the hier tree..should be faster than
 * moving numfacs longer strings around
 */

void order_facs_from_treesort_2(GwTreeNode *t)
{
    while (t) {
        if (t->child) {
            order_facs_from_treesort_2(t->child);
        }

        if (t->t_which >=
            0) /* for when valid netnames like A.B.C, A.B.C.D exist (not legal excluding texsim) */
        /* otherwise this would be an 'else' */
        {
            GLOBALS->facs2_tree_c_1[GLOBALS->facs2_pos_tree_c_1] = GLOBALS->facs[t->t_which];
            t->t_which = GLOBALS->facs2_pos_tree_c_1--;
        }

        t = t->next;
    }
}

void order_facs_from_treesort(GwTreeNode *t, void *v)
{
    GwSymbol ***f =
        (GwSymbol ***)v; /* eliminate compiler warning in tree.h as symbol.h refs tree.h */

    GLOBALS->facs2_tree_c_1 = (GwSymbol **)malloc_2(GLOBALS->numfacs * sizeof(GwSymbol *));
    GLOBALS->facs2_pos_tree_c_1 = GLOBALS->numfacs - 1;
    order_facs_from_treesort_2(t);

    if (GLOBALS->facs2_pos_tree_c_1 >= 0) {
        fprintf(stderr,
                "Internal Error: GLOBALS->facs2_pos_tree_c_1 = %d\n",
                GLOBALS->facs2_pos_tree_c_1);
        fprintf(stderr,
                "[This is usually the result of multiply defined facilities such as a hierarchy "
                "name also being used as a signal at the same level of scope.]\n");
        exit(255);
    }

    free_2(*f);
    *f = GLOBALS->facs2_tree_c_1;
    GLOBALS->facs2_tree_c_1 = NULL;
}

void build_tree_from_name(GwTreeNode **tree_root, const char *s, int which)
{
    GwTreeNode *t, *nt;
    GwTreeNode *tchain = NULL, *tchain_iter;
    GwTreeNode *prevt;
#ifdef _WAVE_HAVE_JUDY
    PPvoid_t PPValue = NULL;
    char str[2048];
#endif

    if (s == NULL || !s[0])
        return;

    t = *tree_root;

    if (t) {
        prevt = NULL;
        while (s) {
        rs:
            s = get_module_name(s);

            if (s && t &&
                !strcmp(t->name,
                        GLOBALS->module_tree_c_1)) /* ajb 300616 added "s &&" to cover case where we
                                                      can have hierarchy + final name are same, see
                                                      A.B.C.D notes elsewhere in this file */
            {
                prevt = t;
                t = t->child;
                continue;
            }

#ifdef _WAVE_HAVE_JUDY
        rescan:
            if (prevt && prevt->children_in_gui) {
                /* find with judy */
                int len = gen_hier_string(str, prevt);
                strcpy(str + len, GLOBALS->module_tree_c_1);
                PPValue = JudySLIns(&GLOBALS->sym_tree, (uint8_t *)str, PJE0);
                if (*PPValue) {
                    t = *PPValue;
                    prevt = t;
                    t = t->child;
                    continue;
                }

                goto construct;
            }
#endif

            tchain = tchain_iter = t;
            if (s && t) {
#ifdef _WAVE_HAVE_JUDY
                int dep = 0;
#endif
                nt = t->next;
                while (nt) {
                    if (nt && !strcmp(nt->name, GLOBALS->module_tree_c_1)) {
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
#ifdef _WAVE_HAVE_JUDY
                    dep++;
#endif
                }

#ifdef _WAVE_HAVE_JUDY
                if (prevt && (dep >= FST_TREE_SEARCH_NEXT_LIMIT)) {
                    int len = gen_hier_string(str, prevt);
                    prevt->children_in_gui = 1; /* "borrowed" for tree build */
                    t = prevt->child;

                    Judy1Set((Pvoid_t)&GLOBALS->sym_tree_addresses, (Word_t)prevt, PJE0);
                    /* assemble judy based on scopename + prevt pnt */
                    while (t) {
                        strcpy(str + len, t->name);
                        PPValue = JudySLIns(&GLOBALS->sym_tree, (uint8_t *)str, PJE0);
                        *PPValue = t;

                        t = t->next;
                    }

                    goto rescan; /* this level of hier is built, now do insert */
                }
#endif
            }

#ifdef _WAVE_HAVE_JUDY
        construct:
#endif
            nt = (GwTreeNode *)talloc_2(sizeof(GwTreeNode) + GLOBALS->module_len_tree_c_1 + 1);
            memcpy(nt->name, GLOBALS->module_tree_c_1, GLOBALS->module_len_tree_c_1);

            if (s) {
                nt->t_which = WAVE_T_WHICH_UNDEFINED_COMPNAME;

#ifdef _WAVE_HAVE_JUDY
                if (prevt && prevt->children_in_gui) {
                    *PPValue = nt;
                }
#endif

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
                nt->next = GLOBALS->terminals_tchain_tree_c_1;
                GLOBALS->terminals_tchain_tree_c_1 = nt;
                return;
            }

            /* blindly clone fac from next part of hier on down */
            t = nt;
            while (s) {
                s = get_module_name(s);

                nt = (GwTreeNode *)talloc_2(sizeof(GwTreeNode) + GLOBALS->module_len_tree_c_1 + 1);
                memcpy(nt->name, GLOBALS->module_tree_c_1, GLOBALS->module_len_tree_c_1);

                if (s) {
                    nt->t_which = WAVE_T_WHICH_UNDEFINED_COMPNAME;
                    t->child = nt;
                    t = nt;
                } else {
                    nt->child = t; /* parent */
                    nt->t_which = which;
                    nt->next = GLOBALS->terminals_tchain_tree_c_1;
                    GLOBALS->terminals_tchain_tree_c_1 = nt;
                }
            }
        }
    } else {
        /* blindly create first fac in the tree (only ever called once) */
        while (s) {
            s = get_module_name(s);

            nt = (GwTreeNode *)talloc_2(sizeof(GwTreeNode) + GLOBALS->module_len_tree_c_1 + 1);
            memcpy(nt->name, GLOBALS->module_tree_c_1, GLOBALS->module_len_tree_c_1);

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

/* ######################## */
/* ## compatibility code ## */
/* ######################## */

/*
 * tree widgets differ between GTK2 and GTK1 so we need two different
 * maketree() routines
 */

/*
 * GTK2: build the tree.
 */
static void XXX_maketree_nodes(GwTreeNode *t2, GtkTreeIter *iter)
{
    char *tmp, *tmp2, *tmp3;
    gchar *text[1];
    GdkPixbuf *pxb;

    if (t2->t_which >= 0) {
        if (GLOBALS->facs[t2->t_which]->vec_root) {
            if (GLOBALS->autocoalesce) {
                if (GLOBALS->facs[t2->t_which]->vec_root != GLOBALS->facs[t2->t_which]) {
                    return; /* was return(NULL) */
                }

                tmp2 = makename_chain(GLOBALS->facs[t2->t_which]);
                tmp3 = leastsig_hiername(tmp2);
                tmp = g_alloca(strlen(tmp3) + 4);
                strcpy(tmp, "[] ");
                strcpy(tmp + 3, tmp3);
                free_2(tmp2);
            } else {
                tmp = g_alloca(strlen(t2->name) + 4);
                strcpy(tmp, "[] ");
                strcpy(tmp + 3, t2->name);
            }
        } else {
            tmp = t2->name;
        }
    } else {
        if (t2->t_which == WAVE_T_WHICH_UNDEFINED_COMPNAME) {
            tmp = t2->name;
        } else {
            int thidx = -t2->t_which + WAVE_T_WHICH_COMPNAME_START;
            if ((thidx >= 0) && (thidx < GLOBALS->comp_name_serial)) {
                char *sc = GLOBALS->comp_name_idx[thidx];
                int tlen = strlen(t2->name) + 2 + 1 + strlen(sc) + 1 + 1;
                tmp = g_alloca(tlen);
                if (!GLOBALS->is_vhdl_component_format) {
                    sprintf(tmp, "%s  (%s)", t2->name, sc);
                } else {
                    sprintf(tmp, "%s  : %s", t2->name, sc);
                }
            } else {
                tmp = t2->name; /* should never get a value out of range here! */
            }
        }
    }

    text[0] = tmp;

    pxb = gw_hierarchy_icons_get(GLOBALS->hierarchy_icons, t2->kind);

    gtk_tree_store_set(GLOBALS->treestore_main,
                       iter,
                       XXX_NAME_COLUMN,
                       text[0],
                       XXX_TREE_COLUMN,
                       t2,
                       XXX_PXB_COLUMN,
                       pxb,
                       -1);
}

void XXX_maketree2(GtkTreeIter *subtree, GwTreeNode *t, int depth)
{
    GwTreeNode *t2;

#ifndef WAVE_DISABLE_FAST_TREE
    if (depth > 1)
        return;
#endif

    t2 = t;
    while (t2) {
#ifndef WAVE_DISABLE_FAST_TREE
        if (depth < 1)
#endif
        {
            t2->children_in_gui = 1;
        }

        if (t2->child) {
            int blacklist = 0;

            if (GLOBALS->exclhiermask) {
                uint64_t exclone = 1;
                if ((exclone << t2->kind) & GLOBALS->exclhiermask)
                    blacklist = 1;
            }

            if (GLOBALS->exclinstname) {
                JRB str = jrb_find_str(GLOBALS->exclinstname, t2->name);
                if (str)
                    blacklist = 1;
            }

            if (GLOBALS->exclcompname) {
                int thidx = -t2->t_which + WAVE_T_WHICH_COMPNAME_START;
                char *sc = ((thidx >= 0) && (thidx < GLOBALS->comp_name_serial))
                               ? GLOBALS->comp_name_idx[thidx]
                               : t2->name;

                JRB str = jrb_find_str(GLOBALS->exclcompname, sc);
                if (str)
                    blacklist = 1;
            }

            if (!blacklist) {
                GtkTreeIter iter;
                gtk_tree_store_prepend(GLOBALS->treestore_main, &iter, subtree);

                XXX_maketree_nodes(t2, &iter);
                XXX_maketree2(&iter, t2->child, depth + 1);
            }
        }

        t2 = t2->next;
    }
}

void XXX_maketree(GtkTreeIter *subtree, GwTreeNode *t)
{
    GtkCellRenderer *renderer_t;
    GtkCellRenderer *renderer_p;
    GtkTreeViewColumn *column;

    GLOBALS->treestore_main = gtk_tree_store_new(XXX_NUM_COLUMNS, /* Total number of columns */
                                                 G_TYPE_STRING, /* name */
                                                 G_TYPE_POINTER, /* tree */
                                                 GDK_TYPE_PIXBUF); /* pixbuf */

    XXX_maketree2(subtree, t, 0);

    GLOBALS->treeview_main = gtk_tree_view_new_with_model(GTK_TREE_MODEL(GLOBALS->treestore_main));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(GLOBALS->treeview_main), FALSE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(GLOBALS->treeview_main), TRUE);

    column = gtk_tree_view_column_new();
    renderer_t = gtk_cell_renderer_text_new();
    renderer_p = gtk_cell_renderer_pixbuf_new();

    gtk_tree_view_column_pack_start(column, renderer_p, FALSE);
    gtk_tree_view_column_pack_end(column, renderer_t, FALSE);

    gtk_tree_view_column_add_attribute(column, renderer_t, "text", XXX_NAME_COLUMN);

    gtk_tree_view_column_add_attribute(column, renderer_p, "pixbuf", XXX_PXB_COLUMN);

    gtk_tree_view_append_column(GTK_TREE_VIEW(GLOBALS->treeview_main), column);

    gtk_widget_show(GLOBALS->treeview_main);
}

/*
 * SST Exclusion filtering for XXX_maketree2() above
 */
#define SST_EXCL_MESS "SSTEXCL | "

enum sst_excl_mode
{
    SST_EXCL_NONE,
    SST_EXCL_HIER,
    SST_EXCL_COMP,
    SST_EXCL_INST
};

void sst_exclusion_loader(void)
{
    JRB str;
    Jval jv;

    int dummy = 0;

    if (GLOBALS->sst_exclude_filename) {
        FILE *f = fopen(GLOBALS->sst_exclude_filename, "rb");
        int exclmode = SST_EXCL_NONE;
        uint64_t exclhier = 0;
        uint64_t exclone = 1;

        if (!f) {
            fprintf(stderr,
                    SST_EXCL_MESS "Could not open '%s' SST exclusion file!\n",
                    GLOBALS->sst_exclude_filename);
            fprintf(stderr, SST_EXCL_MESS);
            perror("Why");
            return;
        }

        fprintf(stderr, SST_EXCL_MESS "Processing '%s'.\n", GLOBALS->sst_exclude_filename);

        while (!feof(f)) {
            char *iline = fgetmalloc(f);
            if (iline) {
                char *p = iline;
                char *e;

                while (*p) {
                    if (isspace(*p))
                        p++;
                    else
                        break;
                }

                e = p;
                while (*e) {
                    if (isspace(*e)) {
                        *e = 0;
                        break;
                    }
                    e++;
                }

                switch (*p) {
                    case '#':
                        break;
                    case '/':
                        break;

                    case '[':
                        if (!strcmp(p, "[hiertype]")) {
                            exclmode = SST_EXCL_HIER;
                        } else if (!strcmp(p, "[compname]")) {
                            exclmode = SST_EXCL_COMP;
                        } else if (!strcmp(p, "[instname]")) {
                            exclmode = SST_EXCL_INST;
                        } else {
                            exclmode = SST_EXCL_NONE;
                        }
                        break;

                    default:
                        switch (exclmode) {
                            case SST_EXCL_HIER: /* this if/else chain is good enough for an init
                                                   script */
                                if (!strcmp(p, "VCD_ST_MODULE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_MODULE;
                                } else if (!strcmp(p, "VCD_ST_TASK")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_TASK;
                                } else if (!strcmp(p, "VCD_ST_FUNCTION")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_FUNCTION;
                                } else if (!strcmp(p, "VCD_ST_BEGIN")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_BEGIN;
                                } else if (!strcmp(p, "VCD_ST_FORK")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_FORK;
                                } else if (!strcmp(p, "VCD_ST_GENERATE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_GENERATE;
                                } else if (!strcmp(p, "VCD_ST_STRUCT")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_STRUCT;
                                } else if (!strcmp(p, "VCD_ST_UNION")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_UNION;
                                } else if (!strcmp(p, "VCD_ST_CLASS")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_CLASS;
                                } else if (!strcmp(p, "VCD_ST_INTERFACE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_INTERFACE;
                                } else if (!strcmp(p, "VCD_ST_PACKAGE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_PACKAGE;
                                } else if (!strcmp(p, "VCD_ST_PROGRAM")) {
                                    exclhier |= exclone << GW_TREE_KIND_VCD_ST_PROGRAM;
                                } else if (!strcmp(p, "VHDL_ST_DESIGN")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_DESIGN;
                                } else if (!strcmp(p, "VHDL_ST_BLOCK")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_BLOCK;
                                } else if (!strcmp(p, "VHDL_ST_GENIF")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_GENIF;
                                } else if (!strcmp(p, "VHDL_ST_GENFOR")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_GENFOR;
                                } else if (!strcmp(p, "VHDL_ST_INSTANCE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_INSTANCE;
                                } else if (!strcmp(p, "VHDL_ST_PACKAGE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_PACKAGE;
                                }

                                else if (!strcmp(p, "VHDL_ST_SIGNAL")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_SIGNAL;
                                } else if (!strcmp(p, "VHDL_ST_PORTIN")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_PORTIN;
                                } else if (!strcmp(p, "VHDL_ST_PORTOUT")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_PORTOUT;
                                } else if (!strcmp(p, "VHDL_ST_PORTINOUT")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_PORTINOUT;
                                } else if (!strcmp(p, "VHDL_ST_BUFFER")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_BUFFER;
                                } else if (!strcmp(p, "VHDL_ST_LINKAGE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_LINKAGE;
                                }

                                else if (!strcmp(p, "VHDL_ST_ARCHITECTURE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_ARCHITECTURE;
                                } else if (!strcmp(p, "VHDL_ST_FUNCTION")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_FUNCTION;
                                } else if (!strcmp(p, "VHDL_ST_PROCEDURE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_PROCEDURE;
                                } else if (!strcmp(p, "VHDL_ST_RECORD")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_RECORD;
                                } else if (!strcmp(p, "VHDL_ST_PROCESS")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_PROCESS;
                                } else if (!strcmp(p, "VHDL_ST_GENERATE")) {
                                    exclhier |= exclone << GW_TREE_KIND_VHDL_ST_GENERATE;
                                }
                                break;

                            case SST_EXCL_COMP:
                                if (!GLOBALS->exclcompname) {
                                    GLOBALS->exclcompname = make_jrb();
                                }
                                str = jrb_find_str(GLOBALS->exclcompname, p);
                                jv.i = dummy++;
                                if (!str)
                                    jrb_insert_str(GLOBALS->exclcompname, strdup_2(p), jv);
                                break;

                            case SST_EXCL_INST:
                                if (!GLOBALS->exclinstname) {
                                    GLOBALS->exclinstname = make_jrb();
                                }
                                str = jrb_find_str(GLOBALS->exclinstname, p);
                                jv.i = dummy++;
                                if (!str)
                                    jrb_insert_str(GLOBALS->exclinstname, strdup_2(p), jv);
                                break;

                            default:
                                break;
                        }
                        break;
                }

                free_2(iline);
            }

            GLOBALS->exclhiermask |= exclhier;
        }

        fclose(f);
    }
}

/* Get the highest signal from T.  */
GwTreeNode *fetchhigh(GwTreeNode *t)
{
    while (t->child)
        t = t->child;
    return (t);
}

/* Get the lowest signal from T.  */
GwTreeNode *fetchlow(GwTreeNode *t)
{
    if (t->child) {
        t = t->child;

        for (;;) {
            while (t->next)
                t = t->next;
            if (t->child)
                t = t->child;
            else
                break;
        }
    }
    return (t);
}

void recurse_fetch_high_low(GwTreeNode *t)
{
top:
    if (t->t_which >= 0) {
        if (t->t_which > GLOBALS->fetchhigh)
            GLOBALS->fetchhigh = t->t_which;
        if (GLOBALS->fetchlow < 0) {
            GLOBALS->fetchlow = t->t_which;
        } else if (t->t_which < GLOBALS->fetchlow) {
            GLOBALS->fetchlow = t->t_which;
        }
    }

    if (t->child) {
        recurse_fetch_high_low(t->child);
    }

    if (t->next) {
        t = t->next;
        goto top;
    }
}
