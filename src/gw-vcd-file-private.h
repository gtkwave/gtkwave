#pragma once

struct _GwVcdFile
{
    GwDumpFile parent_instance;

    gboolean preserve_glitches;
    gboolean preserve_glitches_real;

    struct vlist_t *time_vlist;
    gboolean is_prepacked;

    GwTime start_time;
    GwTime end_time;

    GwHistEntFactory *hist_ent_factory;
};
