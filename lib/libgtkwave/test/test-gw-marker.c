#include <gtkwave.h>

static void test_position(void)
{
    GwMarker *marker = gw_marker_new("marker");
    g_assert_cmpint(gw_marker_get_position(marker), ==, 0);

    gw_marker_set_position(marker, 10);
    g_assert_cmpint(gw_marker_get_position(marker), ==, 10);

    gw_marker_set_position(marker, -20);
    g_assert_cmpint(gw_marker_get_position(marker), ==, -20);

    g_object_unref(marker);
}

static void test_enabled(void)
{
    GwMarker *marker = gw_marker_new("marker");
    g_assert_false(gw_marker_is_enabled(marker));

    gw_marker_set_enabled(marker, TRUE);
    g_assert_true(gw_marker_is_enabled(marker));

    gw_marker_set_enabled(marker, FALSE);
    g_assert_false(gw_marker_is_enabled(marker));

    g_object_unref(marker);
}

static void test_name_and_alias(void)
{
    GwMarker *marker = gw_marker_new("marker");
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "marker");
    g_assert_cmpstr(gw_marker_get_alias(marker), ==, NULL);
    g_assert_cmpstr(gw_marker_get_display_name(marker), ==, "marker");

    gw_marker_set_alias(marker, "alias");
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "marker");
    g_assert_cmpstr(gw_marker_get_alias(marker), ==, "alias");
    g_assert_cmpstr(gw_marker_get_display_name(marker), ==, "alias");

    gw_marker_set_alias(marker, NULL);
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "marker");
    g_assert_cmpstr(gw_marker_get_alias(marker), ==, NULL);
    g_assert_cmpstr(gw_marker_get_display_name(marker), ==, "marker");

    // An empty alias should be handled the same as NULL.

    gw_marker_set_alias(marker, "");
    g_assert_cmpstr(gw_marker_get_name(marker), ==, "marker");
    g_assert_cmpstr(gw_marker_get_alias(marker), ==, NULL);
    g_assert_cmpstr(gw_marker_get_display_name(marker), ==, "marker");

    g_object_unref(marker);
}

// get_name and get_display_name should never return NULL
static void test_nonnull_name(void)
{
    GwMarker *marker1 = g_object_new(GW_TYPE_MARKER, NULL);
    GwMarker *marker2 = g_object_new(GW_TYPE_MARKER, "name", NULL, NULL);

    g_assert_nonnull(gw_marker_get_name(marker1));
    g_assert_nonnull(gw_marker_get_name(marker2));
    g_assert_nonnull(gw_marker_get_display_name(marker1));
    g_assert_nonnull(gw_marker_get_display_name(marker2));

    g_object_unref(marker1);
    g_object_unref(marker2);
}

static void notify_increase(GObject *self, GParamSpec *pspec, gpointer user_data)
{
    (void)self;
    (void)pspec;

    guint *counter = user_data;
    (*counter)++;
}

static void test_notify_display_name(void)
{
    guint counter_alias = 0;
    guint counter_display_name = 0;

    GwMarker *marker = gw_marker_new("name");
    g_signal_connect(marker, "notify::alias", G_CALLBACK(notify_increase), &counter_alias);
    g_signal_connect(marker,
                     "notify::display-name",
                     G_CALLBACK(notify_increase),
                     &counter_display_name);

    g_assert_cmpint(counter_alias, ==, 0);
    g_assert_cmpint(counter_display_name, ==, 0);

    gw_marker_set_alias(marker, "alias");
    g_assert_cmpint(counter_alias, ==, 1);
    g_assert_cmpint(counter_display_name, ==, 1);

    gw_marker_set_alias(marker, "alias");
    g_assert_cmpint(counter_alias, ==, 1);
    g_assert_cmpint(counter_display_name, ==, 1);

    gw_marker_set_alias(marker, NULL);
    g_assert_cmpint(counter_alias, ==, 2);
    g_assert_cmpint(counter_display_name, ==, 2);

    g_object_unref(marker);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/marker/position", test_position);
    g_test_add_func("/marker/enabled", test_enabled);
    g_test_add_func("/marker/name_and_alias", test_name_and_alias);
    g_test_add_func("/marker/nonnull_name", test_nonnull_name);
    g_test_add_func("/marker/notify_display_name", test_notify_display_name);

    return g_test_run();
}