#include <gtkwave.h>

static void test_blackout_regions(void)
{
    GwBlackoutRegions *regions = gw_blackout_regions_new();

    GwDumpFile *dump_file = g_object_new(GW_TYPE_DUMP_FILE, "blackout-regions", regions, NULL);
    g_object_unref(regions);

    g_assert_true(GW_IS_BLACKOUT_REGIONS(gw_dump_file_get_blackout_regions(dump_file)));
    g_assert_true(gw_dump_file_get_blackout_regions(dump_file) == regions);

    g_object_unref(dump_file);

    // get_blackout_regions must always return a valid object.

    dump_file = g_object_new(GW_TYPE_DUMP_FILE, NULL);

    g_assert_true(GW_IS_BLACKOUT_REGIONS(gw_dump_file_get_blackout_regions(dump_file)));

    g_object_unref(dump_file);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/dump-file/blackout-regions", test_blackout_regions);

    return g_test_run();
}