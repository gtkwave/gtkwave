#pragma once

struct _GwVcdFile
{
    GwDumpFile parent_instance;

    gboolean preserve_glitches;
    gboolean preserve_glitches_real;

    GwVlist *time_vlist;
    gboolean is_prepacked;

    GwTime start_time;
    GwTime end_time;

    GwHistEntFactory *hist_ent_factory;
};

// The unit separator control character is used to represent the hierarchy
// delimiter internally.
#define VCD_HIERARCHY_DELIMITER '\x1F'
#define VCD_HIERARCHY_DELIMITER_STR "\x1F"