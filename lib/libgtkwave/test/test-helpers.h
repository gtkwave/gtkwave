#pragma once

#include <gtkwave.h>
#include <glib.h>

/**
 * assert_signal_history_matches:
 * @expected_node: The node with expected history (from original loader)
 * @actual_node: The node with actual history (from partial loader)
 *
 * Compares the signal history of two nodes, asserting that they are identical.
 * This includes comparing all transition times, values, and flags.
 */
void assert_signal_history_matches(GwNode *expected_node, GwNode *actual_node);

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
gchar get_last_value_as_char(GwDumpFile *dump, const gchar *signal_name);

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
void assert_history_matches_up_to_time(GwNode *expected_node, GwNode *actual_node, GwTime max_time);

/**
 * assert_dump_files_equivalent:
 * @expected_dump: Dump file from original loader (ground truth)
 * @actual_dump: Dump file from partial loader (test subject)
 *
 * Compares two dump files to ensure they contain equivalent signal data.
 * This includes comparing symbol counts, names, and signal histories.
 */
void assert_dump_files_equivalent(GwDumpFile *expected_dump, GwDumpFile *actual_dump);