#include <gtkwave.h>

static void test_getters(void)
{
    GwTimeRange *range = gw_time_range_new(10, 20);

    g_assert_cmpint(gw_time_range_get_start(range), ==, 10);
    g_assert_cmpint(gw_time_range_get_end(range), ==, 20);

    g_object_unref(range);
}

static void test_contains(void)
{
    GwTimeRange *range = gw_time_range_new(-2, 3);

    g_assert_false(gw_time_range_contains(range, -3));
    g_assert_true(gw_time_range_contains(range, -2));
    g_assert_true(gw_time_range_contains(range, -1));
    g_assert_true(gw_time_range_contains(range, 0));
    g_assert_true(gw_time_range_contains(range, 1));
    g_assert_true(gw_time_range_contains(range, 2));
    g_assert_true(gw_time_range_contains(range, 3));
    g_assert_false(gw_time_range_contains(range, 4));
    g_assert_false(gw_time_range_contains(range, 5));

    g_object_unref(range);
}

static void test_start_greater_than_end(void)
{
    GwTimeRange *range = gw_time_range_new(10, 2);

    g_assert_cmpint(gw_time_range_get_start(range), ==, 2);
    g_assert_cmpint(gw_time_range_get_end(range), ==, 10);

    g_object_unref(range);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/time_range/getters", test_getters);
    g_test_add_func("/time_range/contains", test_contains);
    g_test_add_func("/time_range/start_greater_than_end", test_start_greater_than_end);

    return g_test_run();
}
