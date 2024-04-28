#include <gtkwave.h>

static gint write_test_data(GwVlistWriter *writer)
{
    g_assert_true(GW_IS_VLIST_WRITER(writer));

    gint size = 0;

    for (guint32 i = 0; i < 1000; i++) {
        gw_vlist_writer_append_uv32(writer, i / 10);
        size++;
    }

    gw_vlist_writer_append_string(writer, "String");
    size += strlen("String") + 1;

    gw_vlist_writer_append_mvl9_string(writer, "1xz");
    size += 2;

    return size;
}

static void check_test_data(GBytes *bytes, gint expected_size)
{
    g_assert_cmpint(g_bytes_get_size(bytes), ==, expected_size);

    const guint8 *data = g_bytes_get_data(bytes, NULL);

    guint32 i;
    for (i = 0; i < 1000; i++) {
        g_assert_cmpint(data[i], ==, 0x80 | (i / 10));
    }

    g_assert_cmpstr((const gchar *)&data[i], ==, "String");
    i += strlen("String") + 1;

    g_assert_cmpint(data[i++], ==, GW_BIT_1 << 4 | GW_BIT_X);
    g_assert_cmpint(data[i++], ==, GW_BIT_Z << 4 | GW_BIT_MASK);

    g_assert_cmpint(i, ==, expected_size);
}

GBytes *vlist_to_bytes(GwVlist *vlist)
{
    GByteArray *array = g_byte_array_new();

    guint size = gw_vlist_size(vlist);
    for (guint i = 0; i < size; i++) {
        const guint8 *byte = gw_vlist_locate(vlist, i);
        g_byte_array_append(array, byte, 1);
    }

    return g_byte_array_free_to_bytes(array);
}

static void test_not_packed(void)
{
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    gint expected_size = write_test_data(writer);

    GwVlist *vlist = gw_vlist_writer_finish(writer);
    g_object_unref(writer);

    gw_vlist_uncompress(&vlist);

    GBytes *data = vlist_to_bytes(vlist);
    check_test_data(data, expected_size);

    gw_vlist_destroy(vlist);
}

static void test_packed(void)
{
    GwVlistWriter *writer = gw_vlist_writer_new(-1, TRUE);
    gint expected_size = write_test_data(writer);

    GwVlist *vlist = gw_vlist_writer_finish(writer);
    g_object_unref(writer);

    gw_vlist_uncompress(&vlist);

    g_assert_cmpint(gw_vlist_size(vlist), <, expected_size);

    guint data_len = 0;
    guchar *data = gw_vlist_packer_decompress(vlist, &data_len);

    GBytes *data_bytes = g_bytes_new(data, data_len);
    check_test_data(data_bytes, expected_size);
    g_bytes_unref(data_bytes);

    // TODO: free data
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vlist_writer/not_packed", test_not_packed);
    g_test_add_func("/vlist_writer/packed", test_packed);

    return g_test_run();
}
