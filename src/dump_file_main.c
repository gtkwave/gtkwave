#include <config.h>
#include <gtkwave.h>
#include "globals.h"
#include "gw-vcd-loader.h"
#include "gw-ghw-loader.h"
#include "gw-fst-loader.h"
#include "gw-fst-file.h"
#include "gw-fst-file-private.h"

static void set_common_settings(GwLoader *loader)
{
    const Settings *global_settings = &GLOBALS->settings;

    gw_loader_set_preserve_glitches(loader, global_settings->preserve_glitches);
    gw_loader_set_preserve_glitches_real(loader, global_settings->preserve_glitches_real);

    if (!GLOBALS->hier_was_explicitly_set) {
        GLOBALS->hier_delimeter = '.';
    }
    gw_loader_set_hierarchy_delimiter(loader, GLOBALS->hier_delimeter);
    gw_loader_set_alternate_hierarchy_delimiter(loader, GLOBALS->alt_hier_delimeter);
}

// TODO: remove
GwDumpFile *vcd_recoder_main(char *fname)
{
    const Settings *global_settings = &GLOBALS->settings;

    GwLoader *loader = gw_vcd_loader_new();
    set_common_settings(loader);
    gw_vcd_loader_set_vlist_prepack(GW_VCD_LOADER(loader), global_settings->vlist_prepack);
    gw_vcd_loader_set_vlist_compression_level(GW_VCD_LOADER(loader),
                                              global_settings->vlist_compression_level);

    GwDumpFile *file = gw_loader_load(loader, fname, NULL); // TODO: use error

    g_object_unref(loader);

    return file;
}

// TODO: remove
GwDumpFile *ghw_main(char *fname)
{
    GwLoader *loader = gw_ghw_loader_new();
    set_common_settings(loader);

    GwDumpFile *file = gw_loader_load(loader, fname, NULL); // TODO: use error

    g_object_unref(loader);

    return file;
}

// TODO: remove
GwDumpFile *fst_main(char *fname, char *skip_start, char *skip_end)
{
    GwLoader *loader = gw_fst_loader_new();
    set_common_settings(loader);

    gw_fst_loader_set_start_time(GW_FST_LOADER(loader), skip_start);
    gw_fst_loader_set_end_time(GW_FST_LOADER(loader), skip_end);

    GwDumpFile *file = gw_loader_load(loader, fname, NULL); // TODO: use error

    g_object_unref(loader);

    return file;
}
