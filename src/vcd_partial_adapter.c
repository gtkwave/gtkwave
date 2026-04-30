#include "vcd_partial_adapter.h"
#include "gw-vcd-partial-loader.h"
#include "gw-shared-memory.h"
#include "globals.h"
#include "wavewindow.h"
#include "analyzer.h"
#include "treesearch.h"
#include "timeentry.h"
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
      //  printf("WE HAVE PROCESSED!\n");
        // Update the global dump file reference
        GLOBALS->dump_file = gw_vcd_partial_loader_get_dump_file(the_loader);

#if 0
        /* List all signals from the dump file for debugging */
       if (GLOBALS->dump_file) {
            GwFacs *facs = gw_dump_file_get_facs(GLOBALS->dump_file);
            if (facs) {
                guint num_facs = gw_facs_get_length(facs);
                printf("DUMP FILE SIGNALS: count=%u\n", num_facs);
                for (guint fi = 0; fi < num_facs; fi++) {
                    GwSymbol *sym = gw_facs_get(facs, fi);
                    if (!sym) {
                        printf("  %u: <null symbol>\n", fi);
                        continue;
                    }
                    GwNode *n = sym->n;
                    if (!n) {
                        printf("  %u: <symbol without node>\n", fi);
                        continue;
                    }
                    const char *name = n->nname ? n->nname : "<unnamed>";
                    printf("  %u: %s (node=%p, numhist=%d)\n", fi, name, (void*)n, n->numhist);

                    /* If this is the signal we're interested in, print its first two history values (positive-time entries) */
                    if (strcmp(name, "mysim.sine_wave") == 0) {
                        int printed = 0;
                        for (GwHistEnt *he = &n->head; he && printed < 2; he = he->next) {
                            if (he->time < 0) /* skip placeholder entries */ continue;

                            if (he->flags & GW_HIST_ENT_FLAG_REAL) {
                                printf("    hist %d: time=%" GW_TIME_FORMAT " real=%f\n", printed + 1, he->time, he->v.h_double);
                            } else if (he->flags & GW_HIST_ENT_FLAG_STRING) {
                                printf("    hist %d: time=%" GW_TIME_FORMAT " string=%s\n", printed + 1, he->time, he->v.h_vector ? he->v.h_vector : "<null>");
                            } else if (he->v.h_vector) {
                                /* vector value stored as a C string of bits/characters */
                                printf("    hist %d: time=%" GW_TIME_FORMAT " vector=%s\n", printed + 1, he->time, he->v.h_vector);
                            } else {
                                printf("    hist %d: time=%" GW_TIME_FORMAT " val=%u\n", printed + 1, he->time, (unsigned)he->v.h_val);
                            }
                            printed++;
                        }
                        if (printed == 0) {
                            printf("    mysim.sine_wave: no positive-time history entries available\n");
                        } else if (printed == 1) {
                            printf("    mysim.sine_wave: only 1 positive-time history entry found\n");
                        }
                    }
                }
            } else {
                printf("DUMP FILE: no facs available\n");
            }
       }
#endif
            // *** Re-expansion logic for updated vector nodes ***
            // Iterate through all traces currently displayed in the wave window
            for (GwTrace *t = GLOBALS->traces.first; t; t = t->t_next) {
                // Check if the trace is for a node (not a blank/comment) and the node has been expanded
                if (!t->vector && t->n.nd && t->n.nd->expand_info) {
                    GwNode *parent_node = t->n.nd;
                    
                    // Check if harray is NULL, which means the node has new data and needs re-expansion
                    if (parent_node->harray == NULL) {
                        g_debug("Re-expanding trace '%s' due to new data.", parent_node->nname);
                        
                        // Store pointer to old expansion info
                        GwExpandInfo *old_expand_info = parent_node->expand_info;
                        
                        // Temporarily clear parent's expand_info pointer to prevent
                        // gw_node_expand() from trying to free it (which would free
                        // the child nodes that are still referenced by traces)
                        parent_node->expand_info = NULL;
                        
                        // Re-run expansion - this creates new children
                        GwExpandInfo *new_expand_info = gw_node_expand(parent_node);
                        
                        if (new_expand_info) {
                            // Find the child traces in the UI and update them to point to the new nodes
                            // Child traces are the traces immediately following the parent trace
                            GwTrace *child_trace = t->t_next;
                            int child_count = 0;
                            
                            // Walk through child traces (they follow the parent until we hit a non-child)
                            while (child_trace && child_count < new_expand_info->width) {
                                // Check if this trace is actually a child of our parent
                                // Children should have the parent's name as a prefix
                                if (!child_trace->vector && child_trace->n.nd && 
                                    child_trace->n.nd->expansion && 
                                    child_trace->n.nd->expansion->parent == parent_node) {
                                    
                                    // Update the child trace to point to the new child node
                                    child_trace->n.nd = new_expand_info->narray[child_count];
                                    
                                    // Mark the trace for redraw
                                    child_trace->interactive_vector_needs_regeneration = 1;
                                    
                                    child_count++;
                                } else {
                                    // Hit a non-child trace, stop looking
                                    break;
                                }
                                
                                child_trace = child_trace->t_next;
                            }
                            
                            // Release the old expand_info using reference counting
                            // The VCD partial loader may still have a reference to it via acquire/release,
                            // so it won't be freed until all references are released
                            if (old_expand_info) {
                                gw_expand_info_release(old_expand_info);
                            }
                        } else {
                            // Re-expansion failed, restore the old expand_info pointer
                            parent_node->expand_info = old_expand_info;
                        }
                    }
                }
            }
            
            // Update the time range
            GwTimeRange *range = gw_dump_file_get_time_range(GLOBALS->dump_file);
            if (range) {
                GLOBALS->tims.last = gw_time_range_get_end(range);
            }

            // Update the time entry fields (From/To) to reflect new time range
            update_endcap_times_for_partial_vcd();

            // Redraw the UI
            fix_wavehadj();
            update_time_box();
            redraw_signals_and_waves();
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
