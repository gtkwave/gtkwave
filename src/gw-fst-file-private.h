#pragma once

#include <gtkwave.h>
#include <fstapi.h>
#include <jrb.h>

struct _GwFstFile
{
    GwDumpFile parent_instance;

    void *fst_reader;

    fstHandle fst_maxhandle;

    struct lx2_entry *fst_table;

    GwFac *mvlfacs;
    fstHandle *mvlfacs_rvs_alias;

    JRB subvar_jrb;
    char **subvar_pnt;

    JRB synclock_jrb;

    GwTime time_scale;

    GwHistEntFactory *hist_ent_factory;

    int busycnt;
};

