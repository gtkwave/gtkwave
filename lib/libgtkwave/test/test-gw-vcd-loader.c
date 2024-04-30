#include <gtkwave.h>

static void test_error_common(const gchar *filename, GQuark error_domain, gint error_code)
{
    GwLoader *loader = gw_vcd_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, filename, &error);
    g_assert_null(file);
    g_assert_error(error, error_domain, error_code);

    g_object_unref(loader);
}

static void test_error_file_not_found()
{
    test_error_common("files/does_not_exist.vcd", GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN);
}

static void test_error_empty()
{
    test_error_common("files/error_empty.vcd", GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_NO_SYMBOLS);
}

static void test_error_no_symbols()
{
    test_error_common("files/error_no_symbols.vcd", GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_NO_SYMBOLS);
}

static void test_error_no_transitions()
{
    test_error_common("files/error_no_transitions.vcd", GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_NO_TRANSITIONS);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vcd_loader/error_file_not_found", test_error_file_not_found);
    g_test_add_func("/vcd_loader/error_empty", test_error_empty);
    g_test_add_func("/vcd_loader/error_no_symbols", test_error_no_symbols);
    g_test_add_func("/vcd_loader/error_no_transitions", test_error_no_transitions);

    return g_test_run();
}
