#include <gtkwave.h>

static void test_get(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(4);

    g_assert_true(GW_IS_MARKER(gw_named_markers_get(markers, 0)));
    g_assert_true(GW_IS_MARKER(gw_named_markers_get(markers, 1)));
    g_assert_true(GW_IS_MARKER(gw_named_markers_get(markers, 2)));
    g_assert_true(GW_IS_MARKER(gw_named_markers_get(markers, 3)));
    g_assert_null(gw_named_markers_get(markers, 4));

    g_object_unref(markers);
}

static void test_names(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(27);

    GwMarker *a = gw_named_markers_get(markers, 0);
    GwMarker *b = gw_named_markers_get(markers, 1);
    GwMarker *c = gw_named_markers_get(markers, 2);
    GwMarker *d = gw_named_markers_get(markers, 3);
    GwMarker *aa = gw_named_markers_get(markers, 26);

    g_assert_cmpstr(gw_marker_get_name(a), ==, "A");
    g_assert_cmpstr(gw_marker_get_name(b), ==, "B");
    g_assert_cmpstr(gw_marker_get_name(c), ==, "C");
    g_assert_cmpstr(gw_marker_get_name(d), ==, "D");
    g_assert_cmpstr(gw_marker_get_name(aa), ==, "AA");

    g_object_unref(markers);
}

static void test_find_first_disabled(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(4);

    GwMarker *marker;

    marker = gw_named_markers_find_first_disabled(markers);
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "A");

    gw_marker_set_enabled(marker, TRUE);

    marker = gw_named_markers_find_first_disabled(markers);
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "B");

    g_object_unref(markers);
}

static void test_find(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(4);

    // Enables markers:
    // - A @   0
    // - B @ 100
    // - C @ 200
    // - D @ 300
    for (guint i = 0; i < gw_named_markers_get_number_of_markers(markers); i++) {
        GwMarker *marker = gw_named_markers_get(markers, i);
        gw_marker_set_position(marker, 100 * i);
        gw_marker_set_enabled(marker, TRUE);
    }

    GwMarker *marker;

    marker = gw_named_markers_find(markers, 100);
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "B");

    marker = gw_named_markers_find(markers, 300);
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "D");

    marker = gw_named_markers_find(markers, 301);
    g_assert_null(marker);

    g_object_unref(markers);
}

static void append_marker_name(gpointer data, gpointer user_data)
{
    GwMarker *marker = data;
    GString *string = user_data;

    g_string_append(string, gw_marker_get_name(marker));
}

static void test_foreach(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(5);

    GString *string = g_string_new(NULL);
    gw_named_markers_foreach(markers, append_marker_name, string);

    g_assert_cmpstr(string->str, ==, "ABCDE");

    g_string_free(string, TRUE);
    g_object_unref(markers);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/named_markers/get", test_get);
    g_test_add_func("/named_markers/names", test_names);
    g_test_add_func("/named_markers/find_first_disabled", test_find_first_disabled);
    g_test_add_func("/named_markers/find", test_find);
    g_test_add_func("/named_markers/foreach", test_foreach);

    return g_test_run();
}