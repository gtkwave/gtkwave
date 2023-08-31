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

static void test_find_closest(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(4);

    // No enabled marker

    GwTime delta = 123;
    g_assert_null(gw_named_markers_find_closest(markers, 10, &delta));
    g_assert_cmpint(delta, ==, 123);

    // One enabled marker

    GwMarker *marker1 = gw_named_markers_get(markers, 2);
    gw_marker_set_position(marker1, 10);
    gw_marker_set_enabled(marker1, TRUE);

    delta = 123;
    g_assert_true(gw_named_markers_find_closest(markers, 10, &delta) == marker1);
    g_assert_cmpint(delta, ==, 0);
    g_assert_true(gw_named_markers_find_closest(markers, 11, &delta) == marker1);
    g_assert_cmpint(delta, ==, 1);
    g_assert_true(gw_named_markers_find_closest(markers, 8, &delta) == marker1);
    g_assert_cmpint(delta, ==, -2);

    // Two enabled marker

    GwMarker *marker2 = gw_named_markers_get(markers, 1);
    gw_marker_set_position(marker2, 20);
    gw_marker_set_enabled(marker2, TRUE);

    g_assert_true(gw_named_markers_find_closest(markers, 9, &delta) == marker1);
    g_assert_cmpint(delta, ==, -1);
    g_assert_true(gw_named_markers_find_closest(markers, 10, &delta) == marker1);
    g_assert_cmpint(delta, ==, 0);
    g_assert_true(gw_named_markers_find_closest(markers, 11, &delta) == marker1);
    g_assert_cmpint(delta, ==, 1);
    g_assert_true(gw_named_markers_find_closest(markers, 19, &delta) == marker2);
    g_assert_cmpint(delta, ==, -1);
    g_assert_true(gw_named_markers_find_closest(markers, 20, &delta) == marker2);
    g_assert_cmpint(delta, ==, 0);
    g_assert_true(gw_named_markers_find_closest(markers, 21, &delta) == marker2);
    g_assert_cmpint(delta, ==, 1);

    // If the absolute distance to two markers is identical the one with the
    // lower index will be returned.

    g_assert_true(gw_named_markers_find_closest(markers, 15, &delta) == marker2);
    g_assert_cmpint(delta, ==, -5);

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

static void on_changed(GwNamedMarkers *markers, guint *counter)
{
    g_assert_true(GW_IS_NAMED_MARKERS(markers));
    (*counter)++;
}

static void test_changed_signal(void)
{
    GwNamedMarkers *markers = gw_named_markers_new(5);

    guint counter = 0;
    g_signal_connect(markers, "changed", G_CALLBACK(on_changed), &counter);
    g_assert_cmpint(counter, ==, 0);

    GwMarker *marker = gw_named_markers_get(markers, 3);
    gw_marker_set_position(marker, 100);
    g_assert_cmpint(counter, ==, 1);
    gw_marker_set_position(marker, 100);
    g_assert_cmpint(counter, ==, 1);

    GwMarker *marker0 = gw_named_markers_get(markers, 0);
    GwMarker *marker1 = gw_named_markers_get(markers, 1);
    gw_marker_set_position(marker0, 101);
    gw_marker_set_position(marker1, 102);
    g_assert_cmpint(counter, ==, 3);

    g_object_unref(markers);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/named_markers/get", test_get);
    g_test_add_func("/named_markers/names", test_names);
    g_test_add_func("/named_markers/find_first_disabled", test_find_first_disabled);
    g_test_add_func("/named_markers/find", test_find);
    g_test_add_func("/named_markers/find_closest", test_find_closest);
    g_test_add_func("/named_markers/foreach", test_foreach);
    g_test_add_func("/named_markers/changed_signal", test_changed_signal);

    return g_test_run();
}
