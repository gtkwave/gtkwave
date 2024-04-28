#include <gtkwave.h>
#include "test-util.h"

static void test_basic()
{
    GwLoader *loader = gw_vcd_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/basic.vcd", &error);
    g_assert_no_error(error);
    g_object_unref(loader);

    common_basic_vcd_and_fst_test(file);

    g_object_unref(file);
}

static void test_error_file_not_found()
{
    GwLoader *loader = gw_vcd_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/does_not_exist.vcd", &error);
    g_assert_null(file);
    g_assert_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN);

    g_object_unref(loader);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vcd_loader/basic", test_basic);
    g_test_add_func("/vcd_loader/error_file_not_found", test_error_file_not_found);

    return g_test_run();
}
