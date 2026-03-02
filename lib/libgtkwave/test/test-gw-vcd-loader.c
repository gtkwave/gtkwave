#include <gtkwave.h>
#include <glib.h>

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

static void test_equivalence_vcd_loading(void)
{
    const char *vcd_filepath = "files/equivalence.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD loader with equivalence.vcd file");

    // Load with the original loader
    GwLoader *loader = gw_vcd_loader_new();
    GwDumpFile *dump = gw_loader_load(loader, vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(dump);
    g_object_unref(loader);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(dump, &error));
    g_assert_no_error(error);

    // Check basic properties
    GwFacs *facs = gw_dump_file_get_facs(dump);
    g_assert_nonnull(facs);
    
    guint num_signals = gw_facs_get_length(facs);
    g_test_message("VCD loader found %u signals", num_signals);
    g_assert_cmpint(num_signals, ==, 4);

    // Check each signal
    for (guint i = 0; i < num_signals; i++) {
        GwSymbol *symbol = gw_facs_get(facs, i);
        g_assert_nonnull(symbol);
        g_assert_nonnull(symbol->n);
        
        g_test_message("Signal %d: %s, numhist=%d", i, symbol->name, symbol->n->numhist);
        g_assert_cmpint(symbol->n->numhist, >, 0);
    }

    g_object_unref(dump);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vcd_loader/error_file_not_found", test_error_file_not_found);
    g_test_add_func("/vcd_loader/error_empty", test_error_empty);
    g_test_add_func("/vcd_loader/error_no_symbols", test_error_no_symbols);
    g_test_add_func("/vcd_loader/error_no_transitions", test_error_no_transitions);
    g_test_add_func("/vcd_loader/equivalence_loading", test_equivalence_vcd_loading);

    return g_test_run();
}
