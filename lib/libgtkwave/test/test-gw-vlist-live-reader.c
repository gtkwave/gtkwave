#include <gtkwave.h>

static void test_live_reader_basic(void)
{
    // Create a writer and enable live mode
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    gw_vlist_writer_set_live_mode(writer, TRUE);
    
    // Create reader from writer before any data is written
    // Ensure writer is fully constructed before creating reader
    g_object_ref_sink(writer);
    GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
    g_assert_nonnull(reader);
    
    // Verify no data available initially
    g_assert_true(gw_vlist_reader_is_done(reader));
    g_assert_cmpint(gw_vlist_reader_next(reader), ==, -1);
    
    // Write some data
    gw_vlist_writer_append_uv32(writer, 12345);
    
    // Reader should now see the data
    g_assert_false(gw_vlist_reader_is_done(reader));
    
    guint32 value = gw_vlist_reader_read_uv32(reader);


    g_assert_cmpint(value, ==, 12345);
    
    // Should be at end again
    g_assert_true(gw_vlist_reader_is_done(reader));
    g_assert_cmpint(gw_vlist_reader_next(reader), ==, -1);
    
    // Write more data
    gw_vlist_writer_append_uv32(writer, 67890);
    gw_vlist_writer_append_uv32(writer, 11111);
    

    // Reader should see new data
    g_assert_false(gw_vlist_reader_is_done(reader));
    value = gw_vlist_reader_read_uv32(reader);

    g_assert_cmpint(value, ==, 67890);
    value = gw_vlist_reader_read_uv32(reader);

    g_assert_cmpint(value, ==, 11111);
    g_assert_true(gw_vlist_reader_is_done(reader));
    
    // Clean up - reader first, then writer
    g_object_unref(reader);
    g_object_unref(writer);
}

static void test_live_reader_strings(void)
{
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    gw_vlist_writer_set_live_mode(writer, TRUE);
    
    // Ensure writer is fully constructed before creating reader
    g_object_ref_sink(writer);
    GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
    
    // Write strings
    gw_vlist_writer_append_string(writer, "Hello");
    gw_vlist_writer_append_string(writer, "World");
    

    // Read strings
    const gchar *str = gw_vlist_reader_read_string(reader);


    g_assert_cmpstr(str, ==, "Hello");
    
    str = gw_vlist_reader_read_string(reader);


    g_assert_cmpstr(str, ==, "World");
    
    // Clean up - reader first, then writer
    g_object_unref(reader);
    g_object_unref(writer);
}

static void test_live_reader_mixed_data(void)
{
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    gw_vlist_writer_set_live_mode(writer, TRUE);
    
    // Ensure writer is fully constructed before creating reader
    g_object_ref_sink(writer);
    GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
    
    // Write mixed data types
    gw_vlist_writer_append_uv32(writer, 42);
    gw_vlist_writer_append_string(writer, "test");
    gw_vlist_writer_append_uv32(writer, 99);
    gw_vlist_writer_append_mvl9_string(writer, "1x");
    
    // Read back in same order
    guint32 value = gw_vlist_reader_read_uv32(reader);

    g_assert_cmpint(value, ==, 42);
    
    const gchar *str = gw_vlist_reader_read_string(reader);

    g_assert_cmpstr(str, ==, "test");
    
    value = gw_vlist_reader_read_uv32(reader);

    g_assert_cmpint(value, ==, 99);
    
    // MVL9 string reading would need specific handling, but we can at least read the bytes
    gint byte;
    byte = gw_vlist_reader_next(reader);
    g_assert_cmpint(byte, >=, 0); // Should read first byte of MVL9 string
    
    // Clean up - reader first, then writer
    g_object_unref(reader);
    g_object_unref(writer);
}

static void test_live_reader_error_cases(void)
{
    // Test that live mode requires prepack=FALSE
    GwVlistWriter *writer = gw_vlist_writer_new(-1, TRUE);
    
    // This should fail because prepack is TRUE
    g_test_expect_message(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*failed*");
    gw_vlist_writer_set_live_mode(writer, TRUE);
    g_test_assert_expected_messages();
    
    g_object_unref(writer);
    
    // Test creating reader from non-live writer
    writer = gw_vlist_writer_new(-1, FALSE);
    // writer is not in live mode, so this should fail
    g_test_expect_message(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*failed*");
    // Ensure writer is fully constructed before creating reader
    g_object_ref_sink(writer);
    GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
    g_test_assert_expected_messages();
    g_assert_null(reader);
    
    g_object_unref(writer);
}

static void test_backward_compatibility(void)
{
    // Verify that normal writer behavior is unchanged
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    
    // Normal usage should work exactly as before
    gw_vlist_writer_append_uv32(writer, 123);
    gw_vlist_writer_append_string(writer, "normal");
    
    GwVlist *vlist = gw_vlist_writer_finish(writer);
    g_assert_nonnull(vlist);
    
    GwVlistReader *reader = gw_vlist_reader_new(vlist, FALSE);
    guint32 value = gw_vlist_reader_read_uv32(reader);
    g_assert_cmpint(value, ==, 123);
    
    const gchar *str = gw_vlist_reader_read_string(reader);
    g_assert_cmpstr(str, ==, "normal");
    
    // Clean up - reader first, then writer
    // The reader owns the vlist and will destroy it automatically
    g_object_unref(reader);
    g_object_unref(writer);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/vlist_live_reader/basic", test_live_reader_basic);
    g_test_add_func("/vlist_live_reader/strings", test_live_reader_strings);
    g_test_add_func("/vlist_live_reader/mixed_data", test_live_reader_mixed_data);
    g_test_add_func("/vlist_live_reader/error_cases", test_live_reader_error_cases);
    g_test_add_func("/vlist_live_reader/backward_compatibility", test_backward_compatibility);

    return g_test_run();
}