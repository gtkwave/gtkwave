#include <gtkwave.h>
#include <stdio.h>

static void test_uv32_encoding_debug(void)
{
    printf("=== Testing UV32 encoding/decoding ===\n");
    
    // Create a writer with no compression
    GwVlistWriter *writer = gw_vlist_writer_new(0, FALSE);
    printf("Writer created with compression level 0\n");
    
    // Test encoding a simple value
    guint32 test_value = 12345;
    printf("Encoding value: %u\n", test_value);
    gw_vlist_writer_append_uv32(writer, test_value);
    
    // Finish the writer to get the vlist
    GwVlist *vlist = gw_vlist_writer_finish(writer);
    printf("Vlist created: %p\n", (void *)vlist);
    
    if (vlist != NULL) {
        // Check the raw bytes in the vlist
        guint size = gw_vlist_size(vlist);
        printf("Vlist size: %u bytes\n", size);
        
        for (guint i = 0; i < size; i++) {
            guint8 *byte = gw_vlist_locate(vlist, i);
            printf("Byte %u: 0x%02x (%u)\n", i, *byte, *byte);
        }
        
        // Create a reader and decode the value
        GwVlistReader *reader = gw_vlist_reader_new(vlist, FALSE);
        guint32 decoded_value = gw_vlist_reader_read_uv32(reader);
        printf("Decoded value: %u\n", decoded_value);
        
        g_assert_cmpint(decoded_value, ==, test_value);
        
        g_object_unref(reader);
        gw_vlist_destroy(vlist);
    }
    
    g_object_unref(writer);
    printf("=== UV32 test completed ===\n\n");
}

static void test_uv32_multiple_values(void)
{
    printf("=== Testing multiple UV32 values ===\n");
    
    GwVlistWriter *writer = gw_vlist_writer_new(0, FALSE);
    
    // Write multiple values
    guint32 values[] = {12345, 67890, 11111, 42, 255};
    guint num_values = sizeof(values) / sizeof(values[0]);
    
    for (guint i = 0; i < num_values; i++) {
        printf("Writing value %u: %u\n", i, values[i]);
        gw_vlist_writer_append_uv32(writer, values[i]);
    }
    
    GwVlist *vlist = gw_vlist_writer_finish(writer);
    
    if (vlist != NULL) {
        // Check raw bytes
        guint size = gw_vlist_size(vlist);
        printf("Total vlist size: %u bytes\n", size);
        
        for (guint i = 0; i < size; i++) {
            guint8 *byte = gw_vlist_locate(vlist, i);
            printf("Byte %u: 0x%02x\n", i, *byte);
        }
        
        // Read back values
        GwVlistReader *reader = gw_vlist_reader_new(vlist, FALSE);
        
        for (guint i = 0; i < num_values; i++) {
            guint32 decoded_value = gw_vlist_reader_read_uv32(reader);
            printf("Read value %u: %u (expected: %u)\n", i, decoded_value, values[i]);
            g_assert_cmpint(decoded_value, ==, values[i]);
        }
        
        g_object_unref(reader);
        gw_vlist_destroy(vlist);
    }
    
    g_object_unref(writer);
    printf("=== Multiple values test completed ===\n\n");
}

int main(int argc, char *argv[])
{
    printf("Starting UV32 debug tests...\n\n");
    
    test_uv32_encoding_debug();
    test_uv32_multiple_values();
    
    printf("All UV32 debug tests completed successfully!\n");
    return 0;
}