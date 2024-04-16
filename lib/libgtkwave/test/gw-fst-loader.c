#include <gtkwave.h>
#include "test-util.h"

static void test_basic()
{
    GwLoader *loader = gw_fst_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/basic.fst", &error);
    g_assert_no_error(error);
    g_object_unref(loader);

    common_basic_vcd_and_fst_test(file);

    g_object_unref(file);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vcd_loader/basic", test_basic);

    return g_test_run();
}
