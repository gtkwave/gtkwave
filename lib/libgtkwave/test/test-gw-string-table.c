#include <gtkwave.h>

static void test_duplicates(void)
{
    GwStringTable *strings = gw_string_table_new();

    g_assert_cmpint(gw_string_table_add(strings, "string1"), ==, 0);
    g_assert_cmpint(gw_string_table_add(strings, "string2"), ==, 1);

    // Adding duplicates should return the same indices.

    g_assert_cmpint(gw_string_table_add(strings, "string1"), ==, 0);
    g_assert_cmpint(gw_string_table_add(strings, "string2"), ==, 1);

    g_object_unref(strings);
}

static void test_freeze(void)
{
    GwStringTable *strings = gw_string_table_new();

    g_assert_cmpint(gw_string_table_add(strings, "string1"), ==, 0);
    g_assert_cmpint(gw_string_table_add(strings, "string2"), ==, 1);

    g_assert_cmpstr(gw_string_table_get(strings, 0), ==, "string1");
    g_assert_cmpstr(gw_string_table_get(strings, 1), ==, "string2");

    // _get shouldn't be affected by calling _freeze.

    gw_string_table_freeze(strings);

    g_assert_cmpstr(gw_string_table_get(strings, 0), ==, "string1");
    g_assert_cmpstr(gw_string_table_get(strings, 1), ==, "string2");

    g_object_unref(strings);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/string-table/duplicates", test_duplicates);
    g_test_add_func("/string-table/freeze", test_freeze);

    return g_test_run();
}