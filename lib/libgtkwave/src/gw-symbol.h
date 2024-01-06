#pragma once

#include "gw-types.h"
#include "gw-node.h"

struct _GwSymbol
{
    // #ifndef _WAVE_HAVE_JUDY
    GwSymbol *sym_next; /* for hash chain, judy uses sym_judy in globals */
    // #endif

    GwSymbol *vec_root;
    GwSymbol *vec_chain;
    char *name;
    GwNode *n;

    // #ifndef _WAVE_HAVE_JUDY
    char s_selected; /* for the clist object */
    // #endif
};
