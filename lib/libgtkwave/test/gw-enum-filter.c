#include <gtkwave.h>

static void test_basic(void)
{
    GwEnumFilter *filter = gw_enum_filter_new();
    gw_enum_filter_insert(filter, "000", "a");
    gw_enum_filter_insert(filter, "001", "b");
    gw_enum_filter_insert(filter, "100", "c");
    gw_enum_filter_insert(filter, "zx-", "d");

    g_assert_cmpstr(gw_enum_filter_lookup(filter, "000"), ==, "a");
    g_assert_cmpstr(gw_enum_filter_lookup(filter, "001"), ==, "b");
    g_assert_cmpstr(gw_enum_filter_lookup(filter, "100"), ==, "c");

    // The value should be compared case insensitively.
    g_assert_cmpstr(gw_enum_filter_lookup(filter, "zx-"), ==, "d");
    g_assert_cmpstr(gw_enum_filter_lookup(filter, "ZX-"), ==, "d");

    g_assert_null(gw_enum_filter_lookup(filter, "111"));

    g_object_unref(filter);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/enum_filter/basic", test_basic);

    return g_test_run();
}