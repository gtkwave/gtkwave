#include <gtkwave.h>

static void test_basic(void)
{
    // Generate packable data with repeating values
    static const gint DATA_SIZE = 100;
    guint8 *data = g_malloc(DATA_SIZE);
    for (gint i = 0; i < DATA_SIZE; i++) {
        data[i] = i / 10;
    }

    GwVlistPacker *packer = gw_vlist_packer_new(4);
    for (gint i = 0; i < DATA_SIZE; i++) {
        gw_vlist_packer_alloc(packer, data[i]);
    }

    GwVlist *packed_vlist = gw_vlist_packer_finalize_and_free(packer);
    g_assert_cmpint(gw_vlist_size(packed_vlist), <, DATA_SIZE);

    guint decompressed_size = 0;
    guchar *decompressed_data = gw_vlist_packer_decompress(packed_vlist, &decompressed_size);

    g_assert_cmpint(decompressed_size, ==, DATA_SIZE);
    g_assert_cmpmem(decompressed_data, decompressed_size, data, DATA_SIZE);

    gw_vlist_destroy(packed_vlist);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vlist_packer/basic", test_basic);

    return g_test_run();
}
