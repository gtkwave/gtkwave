#pragma once

struct _GwVcdFile
{
    GwDumpFile parent_instance;

    struct vlist_t *time_vlist;
    gboolean is_prepacked;

    GwTime start_time;
    GwTime end_time;

    GwHistEntFactory *hist_ent_factory;
};
