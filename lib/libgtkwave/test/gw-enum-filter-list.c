#include <gtkwave.h>

static void test_basic(void)
{
    GwEnumFilterList *list = gw_enum_filter_list_new();

    GwEnumFilter *a = gw_enum_filter_new();
    GwEnumFilter *b = gw_enum_filter_new();
    GwEnumFilter *c = gw_enum_filter_new();

    g_assert_cmpint(gw_enum_filter_list_add(list, a), ==, 0);
    g_assert_cmpint(gw_enum_filter_list_add(list, b), ==, 1);
    g_assert_cmpint(gw_enum_filter_list_add(list, c), ==, 2);

    g_object_unref(a);
    g_object_unref(b);
    g_object_unref(c);

    g_assert_true(gw_enum_filter_list_get(list, 0) == a);
    g_assert_true(gw_enum_filter_list_get(list, 1) == b);
    g_assert_true(gw_enum_filter_list_get(list, 2) == c);
    g_assert_null(gw_enum_filter_list_get(list, 3));

    g_object_unref(list);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/enum_filter_list/basic", test_basic);

    return g_test_run();
}