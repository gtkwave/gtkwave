#include <gtkwave.h>

void region_to_string(GwTime start, GwTime end, gpointer data)
{
    GString *output = data;

    if (output->len > 0) {
        g_string_append(output, ", ");
    }

    g_string_append_printf(output, "%" GW_TIME_FORMAT "-%" GW_TIME_FORMAT, start, end);
}

static void assert_blackout_regions(GwBlackoutRegions *blackout_regions,
                                    const guint expected_count,
                                    const char *expected_string)
{
    g_assert_cmpint(gw_blackout_regions_length(blackout_regions), ==, expected_count);

    GString *string = g_string_new(NULL);

    gw_blackout_regions_foreach(blackout_regions, region_to_string, string);

    g_assert_cmpstr(string->str, ==, expected_string);

    g_string_free(string, TRUE);
}

static void test_add(void)
{
    GwBlackoutRegions *regions = gw_blackout_regions_new();
    assert_blackout_regions(regions, 0, "");

    gw_blackout_regions_add(regions, 1, 2);
    assert_blackout_regions(regions, 1, "1-2");

    gw_blackout_regions_add(regions, 4, 5);
    assert_blackout_regions(regions, 2, "4-5, 1-2");

    gw_blackout_regions_add(regions, 6, 7);
    assert_blackout_regions(regions, 3, "6-7, 4-5, 1-2");

    g_object_unref(regions);
}

static void test_add_dumponoff(void)
{
    GwBlackoutRegions *regions = gw_blackout_regions_new();
    assert_blackout_regions(regions, 0, "");

    gw_blackout_regions_add_dumpoff(regions, 1);
    assert_blackout_regions(regions, 0, "");

    gw_blackout_regions_add_dumpon(regions, 2);
    assert_blackout_regions(regions, 1, "1-2");

    // dumpon commands without previous dumpoff should be ignored.
    gw_blackout_regions_add_dumpon(regions, 3);
    assert_blackout_regions(regions, 1, "1-2");

    gw_blackout_regions_add_dumpoff(regions, 4);
    assert_blackout_regions(regions, 1, "1-2");

    // Repeated dumpoff commands shouldn't change the start time.
    gw_blackout_regions_add_dumpoff(regions, 5);
    assert_blackout_regions(regions, 1, "1-2");

    gw_blackout_regions_add_dumpon(regions, 6);
    assert_blackout_regions(regions, 2, "4-6, 1-2");

    g_object_unref(regions);
}

static void test_scale(void)
{
    GwBlackoutRegions *regions = gw_blackout_regions_new();

    gw_blackout_regions_add(regions, 1, 2);
    gw_blackout_regions_add(regions, 3, 4);
    gw_blackout_regions_add(regions, 10, 20);

    gw_blackout_regions_scale(regions, 10);

    assert_blackout_regions(regions, 3, "100-200, 30-40, 10-20");

    g_object_unref(regions);
}

static void test_contains(void)
{
    GwBlackoutRegions *regions = gw_blackout_regions_new();

    gw_blackout_regions_add(regions, 1, 2);
    gw_blackout_regions_add(regions, 4, 6);

    g_assert_false(gw_blackout_regions_contains(regions, 0));
    g_assert_true(gw_blackout_regions_contains(regions, 1));
    g_assert_false(gw_blackout_regions_contains(regions, 2));
    g_assert_false(gw_blackout_regions_contains(regions, 3));
    g_assert_true(gw_blackout_regions_contains(regions, 4));
    g_assert_true(gw_blackout_regions_contains(regions, 5));
    g_assert_false(gw_blackout_regions_contains(regions, 6));
    g_assert_false(gw_blackout_regions_contains(regions, 7));

    g_object_unref(regions);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/blackout_regions/add", test_add);
    g_test_add_func("/blackout_regions/add_dumponoff", test_add_dumponoff);
    g_test_add_func("/blackout_regions/scale", test_scale);
    g_test_add_func("/blackout_regions/contains", test_contains);

    return g_test_run();
}