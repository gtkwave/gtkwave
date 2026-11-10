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

static GwNode *node_of(GwDumpFile *file, const gchar *name)
{
    GwFacs *facs = gw_dump_file_get_facs(file);
    for (guint i = 0; i < gw_facs_get_length(facs); i++) {
        GwSymbol *symbol = gw_facs_get(facs, i);
        if (g_strcmp0(symbol->name, name) == 0) {
            return symbol->n;
        }
    }
    g_error("fac '%s' not found", name);
}

static GwVarDir vardir_of(GwDumpFile *file, const gchar *name)
{
    return node_of(file, name)->vardir;
}

static void test_evcd_directions()
{
    GwLoader *loader = gw_vcd_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/evcd_directions.vcd", &error);
    g_assert_no_error(error);
    g_assert_nonnull(file);

    /* Direction is inferred from the EVCD state-char class (18.4.3.1), with the
     * in/out/inout keyword honored when present. */
    g_assert_true(gw_dump_file_has_nonimplicit_directions(file));
    g_assert_cmpint(vardir_of(file, "top.d_in"), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(vardir_of(file, "top.clk_in"), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(vardir_of(file, "top.q_out"), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(vardir_of(file, "top.bus_out[3:0]"), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(vardir_of(file, "top.io"), ==, GW_VAR_DIR_INOUT);

    /* Drive strength is captured per port (last seen) and propagated to the
     * node so the GUI can surface it. io ends with "pA 7 0", i.e. strength0=7
     * (supply) and strength1=0 (highz). */
    GwNode *io = node_of(file, "top.io");
    g_assert_true(io->evcd_strength_valid);
    g_assert_cmpint(io->evcd_strength0, ==, 7);
    g_assert_cmpint(io->evcd_strength1, ==, 0);
    g_assert_cmpstr(gw_evcd_strength_name(io->evcd_strength0), ==, "supply");
    g_assert_cmpstr(gw_evcd_strength_name(io->evcd_strength1), ==, "highz");

    g_object_unref(file);
    g_object_unref(loader);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vcd_loader/error_file_not_found", test_error_file_not_found);
    g_test_add_func("/vcd_loader/error_empty", test_error_empty);
    g_test_add_func("/vcd_loader/error_no_symbols", test_error_no_symbols);
    g_test_add_func("/vcd_loader/error_no_transitions", test_error_no_transitions);
    g_test_add_func("/vcd_loader/evcd_directions", test_evcd_directions);

    return g_test_run();
}
