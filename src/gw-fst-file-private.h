#pragma once

#include <gtkwave.h>
#include <fstapi.h>
#include <jrb.h>

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct
{
    GwHistEnt *histent_head;
    GwHistEnt *histent_curr;
    int numtrans;
    GwNode *np;
} GwLx2Entry;

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif

struct _GwFstFile
{
    GwDumpFile parent_instance;

    void *fst_reader;

    fstHandle fst_maxhandle;

    GwLx2Entry *fst_table;

    GwFac *mvlfacs;
    fstHandle *mvlfacs_rvs_alias;

    JRB subvar_jrb;
    char **subvar_pnt;

    JRB synclock_jrb;

    GwTime time_scale;

    GwHistEntFactory *hist_ent_factory;

    int busycnt;
};
