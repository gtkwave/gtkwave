#include <config.h>
#include <gtkwave.h>
#include "globals.h"
#include "gw-vcd-loader.h"
#include "gw-fst-loader.h"
#include "gw-fst-file.h"
#include "gw-fst-file-private.h"

static void set_common_settings(GwLoader *loader)
{
    const Settings *global_settings = &GLOBALS->settings;

    gw_loader_set_preserve_glitches(loader, global_settings->preserve_glitches);
    gw_loader_set_preserve_glitches_real(loader, global_settings->preserve_glitches_real);
}

// TODO: remove
GwDumpFile *vcd_recoder_main(char *fname)
{
    const Settings *global_settings = &GLOBALS->settings;

    GwLoader *loader = gw_vcd_loader_new();
    set_common_settings(loader);
    gw_vcd_loader_set_vlist_prepack(GW_VCD_LOADER(loader), global_settings->vlist_prepack);

    GwDumpFile *file = gw_loader_load(loader, fname, NULL); // TODO: use error

    g_object_unref(loader);

    return file;
}

// TODO: remove
GwDumpFile *fst_main(char *fname, char *skip_start, char *skip_end)
{
    GwLoader *loader = gw_fst_loader_new();
    set_common_settings(loader);

    GwDumpFile *file = gw_loader_load(loader, fname, NULL); // TODO: use error

    g_object_unref(loader);

    // TODO: move to loader
    if (skip_start || skip_end) {
        GwTime b_start, b_end;
        GwTimeDimension time_dimension = gw_dump_file_get_time_dimension(file);

        if (!skip_start)
            b_start = GLOBALS->min_time;
        else
            b_start = unformat_time(skip_start, time_dimension);
        if (!skip_end)
            b_end = GLOBALS->max_time;
        else
            b_end = unformat_time(skip_end, time_dimension);

        if (b_start < GLOBALS->min_time)
            b_start = GLOBALS->min_time;
        else if (b_start > GLOBALS->max_time)
            b_start = GLOBALS->max_time;

        if (b_end < GLOBALS->min_time)
            b_end = GLOBALS->min_time;
        else if (b_end > GLOBALS->max_time)
            b_end = GLOBALS->max_time;

        if (b_start > b_end) {
            GwTime tmp_time = b_start;
            b_start = b_end;
            b_end = tmp_time;
        }

        fstReaderSetLimitTimeRange(GW_FST_FILE(file)->fst_reader, b_start, b_end);
        GLOBALS->min_time = b_start;
        GLOBALS->max_time = b_end;
    }

    return file;
}