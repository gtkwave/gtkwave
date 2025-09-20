#include "vcd_partial_adapter.h"
#include "gw-vcd-partial-loader.h"
#include "gw-shared-memory.h"
#include "globals.h"
#include "wavewindow.h"
#include "analyzer.h"
#include "treesearch.h"
#include <unistd.h>
#include <string.h>
#include "savefile.h"

#define WAVE_PARTIAL_VCD_RING_BUFFER_SIZE (1024 * 1024)

static GwVcdPartialLoader *the_loader = NULL;
static guint the_timer_id = 0;
static GwSharedMemory *shm = NULL;
static guint8 *shm_data = NULL;
static gsize consume_offset = 0;

// Helper functions for reading from shared memory
static guint8 get_8(guint8 *base, gssize offset)
{
    (void)base; // Parameter is unused in this context
    return shm_data[offset % WAVE_PARTIAL_VCD_RING_BUFFER_SIZE];
}

static guint32 get_32(guint8 *base, gssize offset)
{
    guint32 val = 0;
    val |= (get_8(base, offset)     << 24);
    val |= (get_8(base, offset + 1) << 16);
    val |= (get_8(base, offset + 2) << 8);
    val |= (get_8(base, offset + 3));
    return val;
}

// Timer callback that reads from shared memory and feeds the partial loader
// Timer callback to import signals after UI initialization
static gboolean refresh_signal_store_idle(gpointer user_data)
{
    fprintf(stderr, "DEBUG: Refreshing signal store for interactive mode...\n");
    fprintf(stderr, "DEBUG: sig_store_treesearch_gtk2_c_1: %p, dnd_sigview: %p\n",
            GLOBALS->sig_store_treesearch_gtk2_c_1, GLOBALS->dnd_sigview);

    // Get the complete tree from the dump file (includes newly imported signals)
    GwTree *tree = gw_dump_file_get_tree(GLOBALS->dump_file);
    if (tree) {
        GwTreeNode *tree_root = gw_tree_get_root(tree);
        if (tree_root) {
            fprintf(stderr, "DEBUG: Updating signal root to: %p\n", tree_root);
            // Update the signal root to the complete tree from dump file
            GLOBALS->sig_root_treesearch_gtk2_c_1 = tree_root;

            // Repopulate the signal store with updated signal list
            fill_sig_store();
            fprintf(stderr, "DEBUG: Signal store refreshed successfully\n");

            // Force the tree view to refresh
            if (GLOBALS->dnd_sigview) {
                fprintf(stderr, "DEBUG: Forcing tree view refresh\n");

                // Check if tree view is properly connected to the signal store
                GtkTreeModel *current_model = gtk_tree_view_get_model(GTK_TREE_VIEW(GLOBALS->dnd_sigview));
                fprintf(stderr, "DEBUG: Tree view model: %p, Signal store: %p\n", current_model, GLOBALS->sig_store_treesearch_gtk2_c_1);

                if (current_model != GTK_TREE_MODEL(GLOBALS->sig_store_treesearch_gtk2_c_1)) {
                    fprintf(stderr, "DEBUG: WARNING: Tree view is not connected to current signal store!\n");
                    fprintf(stderr, "DEBUG: Reconnecting tree view to signal store\n");
                    gtk_tree_view_set_model(GTK_TREE_VIEW(GLOBALS->dnd_sigview), GTK_TREE_MODEL(GLOBALS->sig_store_treesearch_gtk2_c_1));
                }

                gtk_widget_queue_draw(GLOBALS->dnd_sigview);

                // Also refresh the entire UI to ensure changes are visible
                redraw_signals_and_waves();

                // Force complete UI rebuild by triggering a reload-like operation
                // This ensures the SST pane is properly refreshed like in normal mode
                if (GLOBALS->expanderwindow && GLOBALS->sstpane && GLOBALS->sst_vpaned) {
                    fprintf(stderr, "DEBUG: Forcing complete SST pane refresh\n");

                    // Store current state
                    gboolean was_expanded = gtk_expander_get_expanded(GTK_EXPANDER(GLOBALS->expanderwindow));
                    gint pane_pos = gtk_paned_get_position(GTK_PANED(GLOBALS->sst_vpaned));

                    // Hide and show to force refresh
                    gtk_widget_hide(GLOBALS->expanderwindow);
                    gtk_widget_show(GLOBALS->expanderwindow);

                    // Restore state
                    gtk_expander_set_expanded(GTK_EXPANDER(GLOBALS->expanderwindow), was_expanded);
                    gtk_paned_set_position(GTK_PANED(GLOBALS->sst_vpaned), pane_pos);

                    // Force complete redraw of the entire UI
                    gtk_widget_queue_draw(GTK_WIDGET(GLOBALS->mainwindow));
                }
            }
        } else {
            fprintf(stderr, "DEBUG: Tree root is NULL\n");
        }
    } else {
        fprintf(stderr, "DEBUG: Tree is NULL\n");
    }

    return G_SOURCE_REMOVE;
}

static gboolean import_signals_timeout_callback(gpointer user_data)
{
    fprintf(stderr, "DEBUG: Importing signals into UI...\n");
    fprintf(stderr, "DEBUG: Dump file pointer: %p\n", GLOBALS->dump_file);
    if (GLOBALS->dump_file) {
        GwFacs *facs = gw_dump_file_get_facs(GLOBALS->dump_file);
        fprintf(stderr, "DEBUG: Facs pointer: %p\n", facs);
        if (facs) {
            fprintf(stderr, "DEBUG: Number of signals: %u\n", gw_facs_get_length(facs));
        }
    }
    //analyzer_import_all_signals();
    fprintf(stderr, "DEBUG: Signal import completed\n");

    // Schedule signal store refresh on the main thread to ensure UI updates properly
    // This ensures the signal store is populated after the UI is built and the tree is available
    fprintf(stderr, "DEBUG: Checking if signal store refresh can be scheduled\n");
    fprintf(stderr, "DEBUG: dump_file: %p, sig_store_treesearch_gtk2_c_1: %p\n",
            GLOBALS->dump_file, GLOBALS->sig_store_treesearch_gtk2_c_1);

    if (GLOBALS->dump_file && GLOBALS->sig_store_treesearch_gtk2_c_1) {
        fprintf(stderr, "DEBUG: Scheduling signal store refresh via g_idle_add\n");

        // Get the complete tree from the dump file to ensure it's available
        GwTree *tree = gw_dump_file_get_tree(GLOBALS->dump_file);
        if (tree) {
            GwTreeNode *tree_root = gw_tree_get_root(tree);
            if (tree_root) {
                fprintf(stderr, "DEBUG: Tree root available: %p, scheduling refresh\n", tree_root);
                g_idle_add(refresh_signal_store_idle, NULL);
            } else {
                fprintf(stderr, "DEBUG: Tree root is NULL, cannot refresh signal store\n");
            }
        } else {
            fprintf(stderr, "DEBUG: Tree is NULL, cannot refresh signal store\n");
        }
    } else {
        fprintf(stderr, "DEBUG: Cannot refresh signal store - missing required globals\n");
        if (!GLOBALS->dump_file) {
            fprintf(stderr, "DEBUG: dump_file is NULL\n");
        }
        if (!GLOBALS->sig_store_treesearch_gtk2_c_1) {
            fprintf(stderr, "DEBUG: sig_store_treesearch_gtk2_c_1 is NULL\n");
        }
    }

    return G_SOURCE_REMOVE; // Run only once
}

static gboolean kick_timeout_callback(gpointer user_data)
{
    if (!the_loader || !shm_data) {
        the_timer_id = 0;
        return G_SOURCE_REMOVE;
    }

    gboolean data_processed = FALSE;

    // Process all available data in the shared memory
    while (get_8(shm_data, consume_offset) != 0) {
        guint32 len = get_32(shm_data, consume_offset + 1);

        if (len > 0 && len < 32768) { // Reasonable size limit
            gchar *data_chunk = g_malloc(len + 1);

            for (guint32 i = 0; i < len; i++) {
                data_chunk[i] = get_8(shm_data, consume_offset + 5 + i);
            }
            data_chunk[len] = '\0';

            // Feed the data to the partial loader
            GError *error = NULL;
            gboolean success = gw_vcd_partial_loader_feed(the_loader, data_chunk, len, &error);

            if (!success) {
                fprintf(stderr, "Partial loader feed error: %s\n", error ? error->message : "Unknown error");
                if (error) g_error_free(error);
                g_free(data_chunk);
                the_timer_id = 0;
                return G_SOURCE_REMOVE;
            }

            g_free(data_chunk);
            data_processed = TRUE;
        }

        // Mark as consumed and move to next block
        shm_data[consume_offset % WAVE_PARTIAL_VCD_RING_BUFFER_SIZE] = 0;
        consume_offset += (5 + len);
    }

    // If we processed data, update the UI
    if (data_processed) {
        // Update the global dump file reference
        GLOBALS->dump_file = gw_vcd_partial_loader_get_dump_file(the_loader);

        if (GLOBALS->dump_file) {
            // Update the time range
            GwTimeRange *range = gw_dump_file_get_time_range(GLOBALS->dump_file);
            if (range) {
                GLOBALS->tims.last = gw_time_range_get_end(range);
            }

            // Redraw the UI
            fix_wavehadj();
            update_time_box();
            redraw_signals_and_waves();
        }
    }

    return G_SOURCE_CONTINUE;
}

// Helper function to synchronously parse VCD header
static gboolean parse_vcd_header_synchronously(void)
{
    fprintf(stderr, "DEBUG: Synchronously parsing VCD header...\n");

    // Block and read until the header is fully parsed
    while (!gw_vcd_partial_loader_is_header_parsed(the_loader)) {
        // Check if there's a block ready in the ring buffer
        if (get_8(shm_data, consume_offset) != 0) {
            guint32 len = get_32(shm_data, consume_offset + 1);

            if (len > 0 && len < 32768) { // Reasonable size limit
                gchar *header_chunk = g_malloc(len + 1);

                for (guint32 i = 0; i < len; i++) {
                    header_chunk[i] = get_8(shm_data, consume_offset + 5 + i);
                }
                header_chunk[len] = '\0';

                // Feed the header chunk to the loader
                GError *error = NULL;
                gboolean success = gw_vcd_partial_loader_feed(the_loader, header_chunk, len, &error);

                if (!success) {
                    fprintf(stderr, "Header parse error: %s\n", error ? error->message : "Unknown error");
                    if (error) g_error_free(error);
                    g_free(header_chunk);
                    return FALSE;
                }

                g_free(header_chunk);

                // Mark as consumed and move to next block
                shm_data[consume_offset % WAVE_PARTIAL_VCD_RING_BUFFER_SIZE] = 0;
                consume_offset += (5 + len);
            }
        } else {
            // No data yet, sleep briefly to avoid busy-waiting
            g_usleep(10000); // 10ms
        }
    }

    fprintf(stderr, "DEBUG: VCD header parsed successfully\n");
    return TRUE;
}

GwDumpFile *vcd_partial_adapter_main(const gchar *shm_id)
{
    fprintf(stderr, "DEBUG: vcd_partial_adapter_main called with SHM ID: %s\n", shm_id);
    vcd_partial_adapter_cleanup(); // Clean any previous instance

    // Open the shared memory segment
    GError *error = NULL;
    fprintf(stderr, "DEBUG: Attempting to open SHM segment '%s'\n", shm_id);
    shm = gw_shared_memory_open(shm_id, &error);
    if (!shm) {
        fprintf(stderr, "Error: Could not open SHM segment '%s': %s\n",
                shm_id, error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return NULL;
    }
    fprintf(stderr, "DEBUG: Successfully opened SHM segment\n");

    shm_data = (guint8 *)gw_shared_memory_get_data(shm);
    consume_offset = 0;
    fprintf(stderr, "DEBUG: SHM data pointer: %p, size: %zu\n", shm_data, gw_shared_memory_get_size(shm));

    // Create the partial loader
    the_loader = gw_vcd_partial_loader_new();
    fprintf(stderr, "DEBUG: Partial loader created: %p\n", the_loader);
    if (!the_loader) {
        fprintf(stderr, "Error: Failed to create VCD partial loader\n");
        gw_shared_memory_free(shm);
        shm = NULL;
        shm_data = NULL;
        return NULL;
    }

    // SYNCHRONOUSLY PARSE THE VCD HEADER BEFORE RETURNING
    if (!parse_vcd_header_synchronously()) {
        fprintf(stderr, "Error: Failed to parse VCD header\n");
        vcd_partial_adapter_cleanup();
        return NULL;
    }

    // Start the timer to periodically check for new data (value changes)
    the_timer_id = g_timeout_add(100, kick_timeout_callback, NULL); // Check every 100ms
    fprintf(stderr, "DEBUG: Timer started with ID: %u\n", the_timer_id);

    // Return the now-valid dump file with complete signal hierarchy
    GwDumpFile *dump_file = gw_vcd_partial_loader_get_dump_file(the_loader);
    fprintf(stderr, "DEBUG: Dump file after header parse: %p\n", dump_file);

    // Schedule signal import after UI is fully initialized
    fprintf(stderr, "DEBUG: Scheduling signal import after UI initialization...\n");
    g_timeout_add(500, import_signals_timeout_callback, NULL); // Increased to 500ms

    return dump_file;
}

void vcd_partial_adapter_cleanup(void)
{
    if (the_timer_id > 0) {
        g_source_remove(the_timer_id);
        the_timer_id = 0;
    }

    if (shm) {
        gw_shared_memory_free(shm);
        shm = NULL;
        shm_data = NULL;
    }

    if (the_loader) {
        g_object_unref(the_loader);
        the_loader = NULL;
    }

    consume_offset = 0;
}
