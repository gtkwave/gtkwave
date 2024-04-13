#include <gtkwave.h>

static void test_uncompressed(void)
{
    GwVlist *vlist = gw_vlist_create(1);
    g_assert_cmpint(gw_vlist_size(vlist), ==, 0);

    for (gint i = 0; i < 100; i++) {
        char *t = gw_vlist_alloc(&vlist, FALSE, 0);
        *t = i;
    }
    g_assert_cmpint(gw_vlist_size(vlist), ==, 100);

    gw_vlist_freeze(&vlist, 0);

    for (gint i = 0; i < 100; i++) {
        char *t = gw_vlist_locate(vlist, i);
        g_assert_cmpint(*t, ==, i);
    }

    gw_vlist_destroy(vlist);
}

static void test_compressed(void)
{
    GwVlist *vlist = gw_vlist_create(1);
    g_assert_cmpint(gw_vlist_size(vlist), ==, 0);

    for (gint i = 0; i < 100; i++) {
        char *t = gw_vlist_alloc(&vlist, TRUE, 4);
        *t = i;
    }
    g_assert_cmpint(gw_vlist_size(vlist), ==, 100);

    gw_vlist_freeze(&vlist, 4);

    gw_vlist_uncompress(&vlist);

    for (gint i = 0; i < 100; i++) {
        char *t = gw_vlist_locate(vlist, i);
        g_assert_cmpint(*t, ==, i);
    }

    gw_vlist_destroy(vlist);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vlist/uncompressed", test_uncompressed);
    g_test_add_func("/vlist/compressed", test_compressed);

    return g_test_run();
}
