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
 * added in/out port vartype    31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 * Backtracking fix             16oct18ajb
 */

/* AIX may need this for alloca to work */
#if defined _AIX
#pragma alloca
#endif

#include <config.h>
#include "globals.h"
#include "vcd.h"

void strcpy_vcdalt(char *too, char *from, char delim)
{
    char ch;

    do {
        ch = *(from++);
        if (ch == delim) {
            ch = GLOBALS->hier_delimeter;
        }
    } while ((*(too++) = ch));
}

int strcpy_delimfix(char *too, char *from)
{
    char ch;
    int found = 0;

    do {
        ch = *(from++);
        if (ch == GLOBALS->hier_delimeter) {
            ch = VCDNAM_ESCAPE;
            found = 1;
        }
    } while ((*(too++) = ch));

    if (found)
        GLOBALS->escaped_names_found_vcd_c_1 = found;

    return (found);
}

/*
 * histent structs are NEVER freed so this is OK..
 * (we are allocating as many entries that fit in 64k minus the size of the two
 * bookkeeping void* pointers found in the malloc_2/free_2 routines in
 * debug.c--unless Judy, then can dispense with pointer subtraction)
 */
#ifdef _WAVE_HAVE_JUDY
#define VCD_HISTENT_GRANULARITY ((64 * 1024) / sizeof(GwHistEnt))
#else
#define VCD_HISTENT_GRANULARITY (((64 * 1024) - (2 * sizeof(void *))) / sizeof(GwHistEnt))
#endif

GwHistEnt *histent_calloc(void)
{
    if (GLOBALS->he_curr_vcd_c_1 == GLOBALS->he_fini_vcd_c_1) {
        GLOBALS->he_curr_vcd_c_1 = calloc_2(VCD_HISTENT_GRANULARITY, sizeof(GwHistEnt));
        GLOBALS->he_fini_vcd_c_1 = GLOBALS->he_curr_vcd_c_1 + VCD_HISTENT_GRANULARITY;
    }

    return (GLOBALS->he_curr_vcd_c_1++);
}

char *build_slisthier(void)
{
    struct slist *s;
    int len = 0;

    if (GLOBALS->slisthier) {
        free_2(GLOBALS->slisthier);
    }

    if (!GLOBALS->slistroot) {
        GLOBALS->slisthier_len = 0;
        GLOBALS->slisthier = (char *)malloc_2(1);
        *GLOBALS->slisthier = 0;
        return (GLOBALS->slisthier);
    }

    s = GLOBALS->slistroot;
    len = 0;
    while (s) {
        len += s->len + (s->next ? 1 : 0);
        s = s->next;
    }

    GLOBALS->slisthier = (char *)malloc_2((GLOBALS->slisthier_len = len) + 1);
    s = GLOBALS->slistroot;
    len = 0;
    while (s) {
        strcpy(GLOBALS->slisthier + len, s->str);
        len += s->len;
        if (s->next) {
            strcpy(GLOBALS->slisthier + len, GLOBALS->vcd_hier_delimeter);
            len++;
        }
        s = s->next;
    }

    return (GLOBALS->slisthier);
}

void append_vcd_slisthier(const char *str)
{
    struct slist *s;
    s = (struct slist *)calloc_2(1, sizeof(struct slist));
    s->len = strlen(str);
    s->str = (char *)malloc_2(s->len + 1);
    strcpy(s->str, str);

    if (GLOBALS->slistcurr) {
        GLOBALS->slistcurr->next = s;
        GLOBALS->slistcurr = s;
    } else {
        GLOBALS->slistcurr = GLOBALS->slistroot = s;
    }

    build_slisthier();
    DEBUG(fprintf(stderr, "SCOPE: %s\n", GLOBALS->slisthier));
}

void set_vcd_vartype(struct vcdsymbol *v, GwNode *n)
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

void vcd_sortfacs(GSList *sym_chain)
{
    int i;

    GLOBALS->facs = (struct symbol **)malloc_2(GLOBALS->numfacs * sizeof(struct symbol *));

    GSList *iter = sym_chain;
    for (i = 0; i < GLOBALS->numfacs; i++) {
        GLOBALS->facs[i] = iter->data;

        char *subst = GLOBALS->facs[i]->name;
        int len = strlen(subst);
        if (len > GLOBALS->longestname) {
            GLOBALS->longestname = len;
        }

        iter = g_slist_delete_link(iter, iter);
    }

    /* quicksort(facs,0,numfacs-1); */ /* quicksort deprecated because it degenerates on sorted
                                          traces..badly.  very badly. */
    wave_heapsort(GLOBALS->facs, GLOBALS->numfacs);

    GLOBALS->facs_are_sorted = 1;

    init_tree();
    for (i = 0; i < GLOBALS->numfacs; i++) {
        char *n = GLOBALS->facs[i]->name;
        build_tree_from_name(n, i);

        if (GLOBALS->escaped_names_found_vcd_c_1) {
            char *subst, ch;
            subst = GLOBALS->facs[i]->name;
            while ((ch = (*subst))) {
                if (ch == VCDNAM_ESCAPE) {
                    *subst = GLOBALS->hier_delimeter;
                } /* restore back to normal */
                subst++;
            }
        }
    }
    treegraft(&GLOBALS->treeroot);
    treesort(GLOBALS->treeroot, NULL);

    if (GLOBALS->escaped_names_found_vcd_c_1) {
        treenamefix(GLOBALS->treeroot);
    }
}
