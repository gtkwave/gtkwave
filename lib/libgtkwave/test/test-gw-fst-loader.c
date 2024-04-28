#include <gtkwave.h>

static void test_enum()
{
    GwLoader *loader = gw_fst_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/enum.fst", &error);
    g_assert_no_error(error);
    g_object_unref(loader);

    g_assert_true(gw_dump_file_import_all(file, NULL));

    GwFacs *facs = gw_dump_file_get_facs(file);
    g_assert_cmpint(gw_facs_get_length(facs), ==, 2);

    GwSymbol *v1 = gw_facs_get(facs, 0);
    g_assert_cmpstr(v1->name, ==, "v1[1:0]");
    guint e1 = gw_dump_file_get_enum_filter_for_node(file, v1->n);
    g_assert_cmpint(e1, !=, 0);

    GwSymbol *v2 = gw_facs_get(facs, 1);
    g_assert_cmpstr(v2->name, ==, "v2[3:0]");
    guint e2 = gw_dump_file_get_enum_filter_for_node(file, v2->n);
    g_assert_cmpint(e2, !=, 0);

    g_object_unref(file);
}

static void test_error_file_not_found()
{
    GwLoader *loader = gw_fst_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/does_not_exist.fst", &error);
    g_assert_null(file);
    g_assert_error(error, GW_DUMP_FILE_ERROR, GW_DUMP_FILE_ERROR_UNKNOWN);

    g_object_unref(loader);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/fst_loader/enum", test_enum);
    g_test_add_func("/fst_loader/error_file_not_found", test_error_file_not_found);

    return g_test_run();
}
