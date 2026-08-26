#include <gtkwave.h>
#include <stdio.h>

static void test_debug_writer_construction(void)
{
    printf("=== Testing writer construction ===\n");
    
    // Create a writer
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    printf("Writer created: %p\n", (void *)writer);
    
    // Check if writer is live (should be FALSE by default)
    gboolean is_live = gw_vlist_writer_get_is_live(writer);
    printf("Writer is_live: %d\n", is_live);
    
    // Check if writer is prepack (should be FALSE)
    gboolean is_prepack = gw_vlist_writer_get_prepack(writer);
    printf("Writer prepack: %d\n", is_prepack);
    
    // Get vlist from writer
    GwVlist *vlist = gw_vlist_writer_get_vlist(writer);
    printf("Writer vlist: %p\n", (void *)vlist);
    
    // Try to get size of vlist
    if (vlist != NULL) {
        guint size = gw_vlist_size(vlist);
        printf("Vlist size: %u\n", size);
    } else {
        printf("Vlist is NULL!\n");
    }
    
    // Enable live mode
    printf("Enabling live mode...\n");
    gw_vlist_writer_set_live_mode(writer, TRUE);
    is_live = gw_vlist_writer_get_is_live(writer);
    printf("Writer is_live after enabling: %d\n", is_live);
    
    // Try to create reader from writer
    printf("Creating reader from writer...\n");
    GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
    printf("Reader created: %p\n", (void *)reader);
    
    if (reader != NULL) {
        // Check reader properties
        printf("Reader vlist: %p\n", (void *)reader->vlist);
        printf("Reader size: %u\n", reader->size);
        printf("Reader position: %u\n", reader->position);
        printf("Reader live_writer_source: %p\n", (void *)reader->live_writer_source);
        
        // Try to read from empty reader
        printf("Testing reader_next on empty reader...\n");
        gint result = gw_vlist_reader_next(reader);
        printf("reader_next result: %d\n", result);
        
        g_object_unref(reader);
    }
    
    g_object_unref(writer);
    printf("=== Test completed ===\n\n");
}

static void test_debug_write_then_read(void)
{
    printf("=== Testing write then read ===\n");
    
    // Create a writer and enable live mode
    GwVlistWriter *writer = gw_vlist_writer_new(-1, FALSE);
    gw_vlist_writer_set_live_mode(writer, TRUE);
    g_object_ref_sink(writer); // Ensure writer is fully constructed
    
    // Create reader from writer
    GwVlistReader *reader = gw_vlist_reader_new_from_writer(writer);
    printf("Reader created: %p\n", (void *)reader);
    
    if (reader != NULL) {
        // Try to read from empty reader
        printf("Reading from empty reader...\n");
        gint result = gw_vlist_reader_next(reader);
        printf("reader_next result: %d\n", result);
        
        // Write some data
        printf("Writing data to writer...\n");
        gw_vlist_writer_append_uv32(writer, 12345);
        
        // Try to read again
        printf("Reading after writing...\n");
        result = gw_vlist_reader_next(reader);
        printf("reader_next result: %d\n", result);
        
        if (result != -1) {
            printf("Successfully read byte: %d\n", result);
            
            // Try to read UV32
            printf("Reading UV32...\n");
            guint32 value = gw_vlist_reader_read_uv32(reader);
            printf("UV32 value: %u\n", value);
        }
        
        g_object_unref(reader);
    }
    
    g_object_unref(writer);
    printf("=== Test completed ===\n\n");
}

int main(int argc, char *argv[])
{
    printf("Starting debug tests...\n\n");
    
    test_debug_writer_construction();
    test_debug_write_then_read();
    
    printf("All debug tests completed.\n");
    return 0;
}