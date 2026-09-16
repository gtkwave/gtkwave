#include <config.h>
#include <gtkwave.h>
#include "globals.h"
#include "gw-vcd-loader.h"
#include "gw-ghw-loader.h"
#include "gw-fst-loader.h"
#include "lx2.h"

static void set_common_settings(GwLoader *loader)
{
    const Settings *global_settings = &GLOBALS->settings;

    gw_loader_set_preserve_glitches(loader, global_settings->preserve_glitches);
    gw_loader_set_preserve_glitches_real(loader, global_settings->preserve_glitches_real);

    if (!GLOBALS->hier_was_explicitly_set) {
        GLOBALS->hier_delimeter = '.';
    }
    gw_loader_set_hierarchy_delimiter(loader, GLOBALS->hier_delimeter);
}

static GwDumpFile *load(GwLoader *loader, const gchar *fname)
{
    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, fname, &error);
    if (file == NULL) {
        g_printerr("Error loading %s: ", fname);
        if (error != NULL) {
            g_printerr("%s\n", error->message);
            g_error_free(error);
        } else {
            g_printerr("Unknown error\n");
        }
        exit(EXIT_FAILURE);
    }

    return file;
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
    gw_vcd_loader_set_warning_filesize(GW_VCD_LOADER(loader),
                                       global_settings->vcd_warning_filesize);

    GwDumpFile *file = load(loader, fname);

    g_object_unref(loader);

    GLOBALS->is_lx2 = LXT2_IS_VLIST;

    return file;
}

// TODO: remove
GwDumpFile *ghw_main(char *fname)
{
    GwLoader *loader = gw_ghw_loader_new();
    set_common_settings(loader);

    GwDumpFile *file = load(loader, fname);

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

    GwDumpFile *file = load(loader, fname);

    g_object_unref(loader);

    GLOBALS->is_lx2 = LXT2_IS_FST;

    return file;
}
