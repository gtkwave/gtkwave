#include <gtkwave.h>

static void test_basic(void)
{
    GwProject *project = gw_project_new();
    g_assert_true(GW_IS_PROJECT(project));

    g_assert_true(GW_IS_MARKER(gw_project_get_cursor(project)));
    g_assert_true(GW_IS_MARKER(gw_project_get_primary_marker(project)));
    g_assert_true(GW_IS_MARKER(gw_project_get_baseline_marker(project)));
    g_assert_true(GW_IS_MARKER(gw_project_get_ghost_marker(project)));

    g_assert_true(GW_IS_NAMED_MARKERS(gw_project_get_named_markers(project)));

    g_object_unref(project);
}

void on_unnamed_marker_changed(GwProject *project, guint *counter)
{
    g_assert_true(GW_IS_PROJECT(project));

    (*counter)++;
}

static void test_markers_changed_signal(void)
{
    GwProject *project = gw_project_new();

    guint counter = 0;
    g_signal_connect(project,
                     "unnamed-marker-changed",
                     G_CALLBACK(on_unnamed_marker_changed),
                     &counter);
    g_assert_cmpint(counter, ==, 0);

    GwMarker *primary_marker = gw_project_get_primary_marker(project);
    GwMarker *baseline_marker = gw_project_get_baseline_marker(project);
    GwMarker *ghost_marker = gw_project_get_ghost_marker(project);
    g_assert_cmpint(counter, ==, 0);

    gw_marker_set_position(primary_marker, 10);
    g_assert_cmpint(counter, ==, 1);
    gw_marker_set_position(baseline_marker, 10);
    g_assert_cmpint(counter, ==, 2);
    gw_marker_set_position(ghost_marker, 10);
    g_assert_cmpint(counter, ==, 3);

    // GwNamedMarkers *markers = gw_project_get_named_markers(project);
    // GwMarker *named_marker = gw_named_markers_get(markers, 10);
    // gw_marker_set_position(named_marker, 20);
    // g_assert_cmpint(counter, ==, 4);

    g_object_unref(project);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/project/basic", test_basic);
    g_test_add_func("/project/markers_changed_signal", test_markers_changed_signal);

    return g_test_run();
}