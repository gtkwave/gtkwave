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
 * assert_dump_files_equivalent:
 * @expected_dump: Dump file from original loader (ground truth)
 * @actual_dump: Dump file from partial loader (test subject)
 *
 * Compares two dump files to ensure they contain equivalent signal data.
 * This includes comparing symbol counts, names, and signal histories.
 */
void assert_dump_files_equivalent(GwDumpFile *expected_dump, GwDumpFile *actual_dump);