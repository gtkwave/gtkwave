#include "test-helpers.h"

/**
 * assert_signal_history_matches:
 * @expected_node: The node with expected history (from original loader)
 * @actual_node: The node with actual history (from partial loader)
 *
 * Compares the signal history of two nodes, asserting that they are identical.
 * This includes comparing all transition times, values, and flags.
 */
void assert_signal_history_matches(GwNode *expected_node, GwNode *actual_node)
{
    g_assert_nonnull(expected_node);
    g_assert_nonnull(actual_node);

    // Get the first history entries (skip the head which is just a placeholder)
    GwHistEnt *expected_hist = expected_node->head.next;
    GwHistEnt *actual_hist = actual_node->head.next;

    guint expected_count = 0;
    guint actual_count = 0;

    // Count transitions in both histories
    for (GwHistEnt *he = expected_hist; he != NULL; he = he->next) {
        expected_count++;
    }
    for (GwHistEnt *he = actual_hist; he != NULL; he = he->next) {
        actual_count++;
    }

    g_test_message("Comparing histories: expected %u transitions, actual %u transitions", 
                   expected_count, actual_count);

    // Compare each transition
    while (expected_hist != NULL && actual_hist != NULL) {
        g_test_message("Comparing transition at time %" GW_TIME_FORMAT, expected_hist->time);
        
        // Assert that the time of the transition is identical
        g_assert_cmpint(actual_hist->time, ==, expected_hist->time);

        // Assert that the flags are identical
        g_assert_cmpint(actual_hist->flags, ==, expected_hist->flags);

        // Compare values based on type
        if (expected_hist->flags & GW_HIST_ENT_FLAG_STRING) {
            g_assert_cmpstr(actual_hist->v.h_vector, ==, expected_hist->v.h_vector);
        } else if (expected_hist->flags & GW_HIST_ENT_FLAG_REAL) {
            g_assert_cmpfloat(actual_hist->v.h_double, ==, expected_hist->v.h_double);
        } else if (expected_node->extvals) { 
            // Vector of bits - compare the entire vector
            int width = ABS(expected_node->msi - expected_node->lsi) + 1;
            g_assert_cmpmem(actual_hist->v.h_vector, width, expected_hist->v.h_vector, width);
        } else { 
            // Single bit
            g_assert_cmpint(actual_hist->v.h_val, ==, expected_hist->v.h_val);
        }

        expected_hist = expected_hist->next;
        actual_hist = actual_hist->next;
    }

    // Assert that both lists ended at the same time (i.e., they have the same length)
    g_assert_null(expected_hist);
    g_assert_null(actual_hist);
}

/**
 * assert_dump_files_equivalent:
 * @expected_dump: Dump file from original loader (ground truth)
 * @actual_dump: Dump file from partial loader (test subject)
 *
 * Compares two dump files to ensure they contain equivalent signal data.
 * This includes comparing symbol counts, names, and signal histories.
 */
void assert_dump_files_equivalent(GwDumpFile *expected_dump, GwDumpFile *actual_dump)
{
    g_assert_nonnull(expected_dump);
    g_assert_nonnull(actual_dump);

    GwFacs *expected_facs = gw_dump_file_get_facs(expected_dump);
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);

    g_assert_nonnull(expected_facs);
    g_assert_nonnull(actual_facs);

    // Assert basic properties are the same
    guint expected_count = gw_facs_get_length(expected_facs);
    guint actual_count = gw_facs_get_length(actual_facs);
    
    g_test_message("Comparing dump files: expected %u symbols, actual %u symbols", 
                   expected_count, actual_count);
    
    g_assert_cmpint(actual_count, ==, expected_count);

    // Compare time scale and dimension
    g_assert_cmpint(gw_dump_file_get_time_scale(actual_dump), ==, 
                    gw_dump_file_get_time_scale(expected_dump));
    g_assert_cmpint(gw_dump_file_get_time_dimension(actual_dump), ==, 
                    gw_dump_file_get_time_dimension(expected_dump));

    // Deep compare every single signal
    for (guint i = 0; i < expected_count; i++) {
        GwSymbol *expected_symbol = gw_facs_get(expected_facs, i);
        g_assert_nonnull(expected_symbol);
        
        // Look up the same symbol by name in the new dump file
        GwSymbol *actual_symbol = gw_dump_file_lookup_symbol(actual_dump, expected_symbol->name);
        
        g_test_message("Comparing signal: %s", expected_symbol->name);
        g_assert_nonnull(actual_symbol);
        g_assert_cmpstr(actual_symbol->name, ==, expected_symbol->name);

        // Use our helper to compare their value histories
        assert_signal_history_matches(expected_symbol->n, actual_symbol->n);
    }
}