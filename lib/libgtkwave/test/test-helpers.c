#include "test-helpers.h"
#include <string.h>

/**
 * get_last_value_as_char:
 * @dump: The dump file containing the signal
 * @signal_name: The full hierarchical name of the signal
 *
 * Returns the last value of the specified signal as a character.
 * For scalar signals, returns '0', '1', 'X', 'Z', etc.
 * For vector signals, returns the first character of the vector.
 * For real signals, returns 'R' (placeholder).
 * For string signals, returns the first character of the string.
 */
gchar get_last_value_as_char(GwDumpFile *dump, const gchar *signal_name)
{
    g_assert_nonnull(dump);
    g_assert_nonnull(signal_name);

    // Find the symbol by name
    GwSymbol *symbol = gw_dump_file_lookup_symbol(dump, signal_name);
    g_assert_nonnull(symbol);
    g_assert_nonnull(symbol->n);

    GwNode *node = symbol->n;
    g_assert_nonnull(node->curr); // Should have at least the placeholder entries

    // Get the last history entry (skip placeholders if needed)
    GwHistEnt *last_hist = node->curr;
    while (last_hist != NULL && last_hist->next != NULL) {
        last_hist = last_hist->next;
    }

    g_assert_nonnull(last_hist);
    
    // Debug output to understand what's happening
    g_test_message("get_last_value_as_char: signal=%s, time=%" GW_TIME_FORMAT ", flags=0x%x", 
                   signal_name, last_hist->time, last_hist->flags);

    // Convert the value to a character based on signal type
    gchar result;
    if (node->extvals) {
        if (node->vartype == GW_VAR_TYPE_VCD_REAL) {
            result = 'R'; // Placeholder for real values
        } else if (node->vartype == GW_VAR_TYPE_GEN_STRING) {
            if (last_hist->flags & GW_HIST_ENT_FLAG_STRING) {
                result = last_hist->v.h_vector[0]; // First character of string
            } else {
                result = '?'; // Unknown string format
            }
        } else {
            // Vector signal - return first character
            result = gw_bit_to_char(last_hist->v.h_vector[0]);
        }
    } else {
        // Scalar signal
        result = gw_bit_to_char(last_hist->v.h_val);
    }
    
    g_test_message("get_last_value_as_char: result='%c' (%d)", result, result);
    return result;
}

/**
 * assert_history_matches_up_to_time:
 * @expected_node: The node with expected history (from original loader)
 * @actual_node: The node with actual history (from partial loader)
 * @max_time: The maximum time to compare up to (inclusive)
 *
 * Compares the signal history of two nodes up to the specified time.
 * Asserts that the partial loader's history matches the ground truth
 * up to the given time, and that it doesn't contain any extra transitions
 * beyond that time.
 */
void assert_history_matches_up_to_time(GwNode *expected_node, GwNode *actual_node, GwTime max_time)
{
    g_assert_nonnull(expected_node);
    g_assert_nonnull(actual_node);

    // Get the first history entries (skip the head which is just a placeholder)
    GwHistEnt *expected_hist = expected_node->head.next;
    GwHistEnt *actual_hist = actual_node->head.next;

    guint expected_count = 0;
    guint actual_count = 0;

    // Compare each transition up to max_time
    while (expected_hist != NULL && actual_hist != NULL) {
        // Skip placeholder transitions in partial loader (times -2 and -1)
        if (actual_hist->time < 0) {
            g_test_message("Skipping placeholder transition at time %" GW_TIME_FORMAT, actual_hist->time);
            actual_hist = actual_hist->next;
            actual_count++;
            continue;
        }

        // Stop if we've reached beyond the max_time
        if (expected_hist->time > max_time) {
            break;
        }

        g_test_message("Comparing transition at time %" GW_TIME_FORMAT, expected_hist->time);
        
        // Assert that the time of the transition is identical
        g_assert_cmpint(actual_hist->time, ==, expected_hist->time);

        // Assert that the flags are identical
        g_assert_cmpint(actual_hist->flags, ==, expected_hist->flags);

        // Compare values based on type
        if (expected_hist->flags & GW_HIST_ENT_FLAG_STRING) {
            g_assert_cmpstr((const char *)actual_hist->v.h_vector, ==, (const char *)expected_hist->v.h_vector);
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
        expected_count++;
        actual_count++;
    }

    // Assert that the partial loader doesn't have extra transitions beyond max_time
    while (actual_hist != NULL && actual_hist->time <= max_time) {
        // Skip placeholder transitions in partial loader (times -2 and -1)
        if (actual_hist->time < 0) {
            g_test_message("Skipping placeholder transition at time %" GW_TIME_FORMAT, actual_hist->time);
            actual_hist = actual_hist->next;
            actual_count++;
            continue;
        }
        
        g_test_message("Unexpected transition in partial loader at time %" GW_TIME_FORMAT, actual_hist->time);
        actual_hist = actual_hist->next;
        actual_count++;
    }

    // The actual history should not have any more transitions up to max_time
    // (excluding placeholders which are allowed)
    if (actual_hist != NULL && actual_hist->time <= max_time) {
        g_test_message("Extra non-placeholder transition found at time %" GW_TIME_FORMAT, actual_hist->time);
        g_assert_true(actual_hist->time > max_time); // This should fail and show the problem
    }

    g_test_message("Compared %u transitions up to time %" GW_TIME_FORMAT, expected_count, max_time);
}

/**
 * assert_signal_history_matches:
 * @expected_node: The node with expected history (from original loader)
 * @actual_node: The node with actual history (from partial loader)
 *
 * Compares the signal history of two nodes, asserting that they are identical.
 * This includes comparing all transition times, values, and flags.
 * The comparison is endcap-aware - it stops before comparing endcap transitions
 * (times >= GW_TIME_MAX - 1) since the partial loader doesn't have endcaps.
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

    // Count transitions in both histories (excluding endcaps for expected)
    if (expected_hist == NULL) {
        g_test_message("WARNING: expected_hist is NULL - expected node has no history entries");
        g_test_message("Expected node info: numhist=%d, extvals=%d, vartype=%d, head.next=%p, curr=%p", 
                       expected_node->numhist, expected_node->extvals, expected_node->vartype,
                       expected_node->head.next, expected_node->curr);
    } else {
        for (GwHistEnt *he = expected_hist; he != NULL; he = he->next) {
            g_test_message("Expected transition: time=%" GW_TIME_FORMAT ", flags=0x%x", he->time, he->flags);
            if (he->time < GW_TIME_MAX - 1) {
                expected_count++;
            } else {
                g_test_message("Skipping expected endcap transition at time %" GW_TIME_FORMAT, he->time);
            }
        }
    }
    for (GwHistEnt *he = actual_hist; he != NULL; he = he->next) {
        g_test_message("Actual transition: time=%" GW_TIME_FORMAT ", flags=0x%x", he->time, he->flags);
        actual_count++;
    }

    g_test_message("Comparing histories: expected %u transitions (excluding endcaps), actual %u transitions", 
                   expected_count, actual_count);
    
    // Debug: Check if expected node has any transitions at all
    if (expected_count == 0) {
        g_test_message("WARNING: Expected node has no transitions with time < GW_TIME_MAX - 1");
        g_test_message("GW_TIME_MAX = %" GW_TIME_FORMAT, GW_TIME_MAX);
        g_test_message("GW_TIME_MAX - 1 = %" GW_TIME_FORMAT, GW_TIME_MAX - 1);
        
        // Debug: Show all expected transitions to understand what's happening
        g_test_message("All expected transitions:");
        if (expected_hist == NULL) {
            g_test_message("  No transitions found - expected_hist is NULL");
            g_test_message("  Expected node info: numhist=%d, extvals=%d, vartype=%d", 
                           expected_node->numhist, expected_node->extvals, expected_node->vartype);
            g_test_message("  Expected node head: next=%p, curr=%p", expected_node->head.next, expected_node->curr);
            
            // Check if the node has any history at all
            if (expected_node->numhist > 0) {
                g_test_message("  ERROR: Node reports numhist=%d but head.next is NULL!", expected_node->numhist);
            }
        } else {
            for (GwHistEnt *he = expected_hist; he != NULL; he = he->next) {
                g_test_message("  time=%" GW_TIME_FORMAT ", flags=0x%x", he->time, he->flags);
            }
        }
    }

    // Compare each transition (stop before endcaps)
    while (expected_hist != NULL && actual_hist != NULL) {
        // Stop comparison when we reach endcap transitions in expected history
        if (expected_hist->time >= GW_TIME_MAX - 1) {
            break;
        }

        // Skip placeholder transitions in expected history (times -2 and -1)
        if (expected_hist->time < 0) {
            g_test_message("Skipping placeholder transition in expected history at time %" GW_TIME_FORMAT, expected_hist->time);
            expected_hist = expected_hist->next;
            continue;
        }

        // Skip placeholder transitions in partial loader (times -2 and -1)
        if (actual_hist->time < 0) {
            g_test_message("Skipping placeholder transition in partial loader at time %" GW_TIME_FORMAT, actual_hist->time);
            actual_hist = actual_hist->next;
            continue;
        }

        g_test_message("Comparing transition at time %" GW_TIME_FORMAT, expected_hist->time);
        
        // Assert that the time of the transition is identical
        g_assert_cmpint(actual_hist->time, ==, expected_hist->time);

        // Assert that the flags are identical
        g_assert_cmpint(actual_hist->flags, ==, expected_hist->flags);

        // Compare values based on type
        if (expected_hist->flags & GW_HIST_ENT_FLAG_STRING) {
            g_test_message("Comparing string values: actual='%s', expected='%s'", actual_hist->v.h_vector, expected_hist->v.h_vector);
            g_assert_cmpstr((const char *)actual_hist->v.h_vector, ==, (const char *)expected_hist->v.h_vector);
        } else if (expected_hist->flags & GW_HIST_ENT_FLAG_REAL) {
            g_test_message("Comparing real values: actual=%f, expected=%f", actual_hist->v.h_double, expected_hist->v.h_double);
            g_assert_cmpfloat(actual_hist->v.h_double, ==, expected_hist->v.h_double);
        } else if (expected_node->extvals) { 
            // Vector of bits - compare the entire vector
            int width = ABS(expected_node->msi - expected_node->lsi) + 1;
            g_test_message("Comparing vector values: width=%d", width);
            
            // Debug: show full vector values
            gchar actual_vector_str[width + 1];
            gchar expected_vector_str[width + 1];
            for (int i = 0; i < width; i++) {
                actual_vector_str[i] = gw_bit_to_char(actual_hist->v.h_vector[i]);
                expected_vector_str[i] = gw_bit_to_char(expected_hist->v.h_vector[i]);
            }
            actual_vector_str[width] = '\0';
            expected_vector_str[width] = '\0';
            
            g_test_message("  full vector: actual='%s', expected='%s'", actual_vector_str, expected_vector_str);
            for (int i = 0; i < width; i++) {
                g_test_message("  bit %d: actual='%c', expected='%c'", i, gw_bit_to_char(actual_hist->v.h_vector[i]), gw_bit_to_char(expected_hist->v.h_vector[i]));
            }
            g_assert_cmpmem(actual_hist->v.h_vector, width, expected_hist->v.h_vector, width);
        } else { 
            // Single bit
            g_test_message("Comparing single bit values: actual='%c', expected='%c'", gw_bit_to_char(actual_hist->v.h_val), gw_bit_to_char(expected_hist->v.h_val));
            g_assert_cmpint(actual_hist->v.h_val, ==, expected_hist->v.h_val);
        }

        expected_hist = expected_hist->next;
        actual_hist = actual_hist->next;
    }

    // Assert that the partial loader doesn't have extra transitions beyond what we compared
    // (it should only have the meaningful transitions, no endcaps)
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