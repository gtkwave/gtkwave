#include <gtkwave.h>

static void test_add_stem(void)
{
    GwStems *stems = gw_stems_new();

    guint p1 = gw_stems_add_path(stems, "p1");
    guint p2 = gw_stems_add_path(stems, "p2");
    g_assert_cmpint(p1, ==, 1);
    g_assert_cmpint(p2, ==, 2);

    // The first index should be 1, because 0 is used as a marker for no stem
    g_assert_cmpint(gw_stems_add_stem(stems, p1, 1), ==, 1);
    g_assert_cmpint(gw_stems_add_stem(stems, p2, 2), ==, 2);

    GwStem stem = gw_stems_get_stem(stems, 1);
    g_assert_cmpstr(stem.path, ==, "p1");
    g_assert_cmpint(stem.line_number, ==, 1);

    stem = gw_stems_get_stem(stems, 2);
    g_assert_cmpstr(stem.path, ==, "p2");
    g_assert_cmpint(stem.line_number, ==, 2);
}

static void test_add_istem(void)
{
    GwStems *stems = gw_stems_new();

    // Adding stems shouldn't influence the istem indices.
    guint path2 = gw_stems_add_path(stems, "path2");
    gw_stems_add_stem(stems, path2, 1);
    gw_stems_add_stem(stems, path2, 2);

    guint path = gw_stems_add_path(stems, "path");

    // The first index should be 1, because 0 is used as a marker for no stem
    g_assert_cmpint(gw_stems_add_istem(stems, path, 3), ==, 1);
    g_assert_cmpint(gw_stems_add_istem(stems, path, 4), ==, 2);

    GwStem stem = gw_stems_get_istem(stems, 1);
    g_assert_cmpstr(stem.path, ==, "path");
    g_assert_cmpint(stem.line_number, ==, 3);

    stem = gw_stems_get_istem(stems, 2);
    g_assert_cmpstr(stem.path, ==, "path");
    g_assert_cmpint(stem.line_number, ==, 4);
}

static void test_is_path_index_valid(void)
{
    GwStems *stems = gw_stems_new();
    g_assert_false(gw_stems_is_path_index_valid(stems, 0));
    g_assert_false(gw_stems_is_path_index_valid(stems, 1));
    g_assert_false(gw_stems_is_path_index_valid(stems, 2));
    g_assert_false(gw_stems_is_path_index_valid(stems, 3));

    gw_stems_add_path(stems, "path0");
    g_assert_false(gw_stems_is_path_index_valid(stems, 0));
    g_assert_true(gw_stems_is_path_index_valid(stems, 1));
    g_assert_false(gw_stems_is_path_index_valid(stems, 2));
    g_assert_false(gw_stems_is_path_index_valid(stems, 3));

    gw_stems_add_path(stems, "path1");
    g_assert_false(gw_stems_is_path_index_valid(stems, 0));
    g_assert_true(gw_stems_is_path_index_valid(stems, 1));
    g_assert_true(gw_stems_is_path_index_valid(stems, 2));
    g_assert_false(gw_stems_is_path_index_valid(stems, 3));
}

static void test_get_next_path_index(void)
{
    GwStems *stems = gw_stems_new();
    g_assert_cmpint(gw_stems_get_next_path_index(stems), ==, 1);

    gw_stems_add_path(stems, "path0");
    g_assert_cmpint(gw_stems_get_next_path_index(stems), ==, 2);

    gw_stems_add_path(stems, "path1");
    g_assert_cmpint(gw_stems_get_next_path_index(stems), ==, 3);
}

static void test_is_empty_and_has_istems(void)
{
    GwStems *stems = gw_stems_new();
    g_assert_true(gw_stems_is_empty(stems));
    g_assert_false(gw_stems_has_istems(stems));

    // stems should still be empty if only a path is added
    guint p1 = gw_stems_add_path(stems, "p1");
    g_assert_true(gw_stems_is_empty(stems));

    gw_stems_add_stem(stems, p1, 1);
    g_assert_false(gw_stems_is_empty(stems));
    g_assert_false(gw_stems_has_istems(stems));

    stems = gw_stems_new();
    p1 = gw_stems_add_path(stems, "p1");
    g_assert_true(gw_stems_is_empty(stems));
    gw_stems_add_istem(stems, p1, 1);
    g_assert_false(gw_stems_is_empty(stems));
    g_assert_true(gw_stems_has_istems(stems));
    gw_stems_add_stem(stems, p1, 2);
    g_assert_false(gw_stems_is_empty(stems));
    g_assert_true(gw_stems_has_istems(stems));
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/stems/add_stem", test_add_stem);
    g_test_add_func("/stems/add_istem", test_add_istem);
    g_test_add_func("/stems/is_path_index_valid", test_is_path_index_valid);
    g_test_add_func("/stems/get_next_path_index", test_get_next_path_index);
    g_test_add_func("/stems/is_empty_and_has_istems", test_is_empty_and_has_istems);
    return g_test_run();
}
