#include <gtkwave.h>

static void test_common(gint compression_level)
{
    GwVlist *vlist = gw_vlist_create(1);
    g_assert_cmpint(gw_vlist_size(vlist), ==, 0);

    for (gint i = 0; i < 100; i++) {
        char *t = gw_vlist_alloc(&vlist, FALSE, compression_level);
        *t = i;
    }
    g_assert_cmpint(gw_vlist_size(vlist), ==, 100);

    gw_vlist_freeze(&vlist, compression_level);

    if (compression_level > 0) {
        gw_vlist_uncompress(&vlist);
    }

    for (gint i = 0; i < 100; i++) {
        char *t = gw_vlist_locate(vlist, i);
        g_assert_cmpint(*t, ==, i);
    }

    gw_vlist_destroy(vlist);
}

static void test_uncompressed(void)
{
    test_common(0);
}

static void test_compressed(void)
{
    test_common(9);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vlist/uncompressed", test_uncompressed);
    g_test_add_func("/vlist/compressed", test_compressed);

    return g_test_run();
}
