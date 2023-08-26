#include <gtkwave.h>

static void test_basic(void)
{
    GwProject *project = gw_project_new();
    g_assert_true(GW_IS_PROJECT(project));

    g_assert_true(GW_IS_NAMED_MARKERS(gw_project_get_named_markers(project)));

    g_object_unref(project);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/project/basic", test_basic);

    return g_test_run();
}