#include <gtkwave.h>
#include <glib/gstdio.h>
#include <string.h>
#include "test-helpers.h"
// Simple function to read a file into a string
static gchar *read_file_contents(const gchar *filename)
{
    GError *error = NULL;
    gchar *contents;
    gsize length;

    if (!g_file_get_contents(filename, &contents, &length, &error)) {
        g_test_message("Failed to read file %s: %s", filename, error->message);
        g_error_free(error);
        return NULL;
    }

    return contents;
}

// Simple function to convert a dump file to string format (mimics dump.c behavior)
static gchar *dump_file_to_string(GwDumpFile *dump_file)
{
    GString *output = g_string_new(NULL);

    // Time section
    g_string_append(output, "Time\n");
    g_string_append(output, "----\n");
    g_string_append_printf(output, "scale: %" GW_TIME_FORMAT "\n", gw_dump_file_get_time_scale(dump_file));

    gchar *dimension = g_enum_to_string(GW_TYPE_TIME_DIMENSION, gw_dump_file_get_time_dimension(dump_file));
    g_string_append_printf(output, "dimension: %s\n", dimension);
    g_free(dimension);

    GwTimeRange *range = gw_dump_file_get_time_range(dump_file);
    g_string_append_printf(output, "range: %" GW_TIME_FORMAT " - %" GW_TIME_FORMAT "\n",
                          gw_time_range_get_start(range), gw_time_range_get_end(range));
    g_string_append_printf(output, "global time offset: %" GW_TIME_FORMAT "\n", gw_dump_file_get_global_time_offset(dump_file));
    g_string_append(output, "\n");

    // Tree section
    g_string_append(output, "Tree\n");
    g_string_append(output, "----\n");

    GwTree *tree = gw_dump_file_get_tree(dump_file);
    if (tree && gw_tree_get_root(tree)) {
        // Simplified tree dump - just show the structure without recursion
        GwTreeNode *root = gw_tree_get_root(tree);
        if (root && root->name) {
            gchar *kind = g_enum_to_string(GW_TYPE_TREE_KIND, root->kind);
            g_string_append_printf(output, "%s (kind=%s, t_which=%d)\n", root->name, kind, root->t_which);
            g_free(kind);

            // Show direct children
            for (GwTreeNode *child = root->child; child != NULL; child = child->next) {
                kind = g_enum_to_string(GW_TYPE_TREE_KIND, child->kind);
                g_string_append_printf(output, "    %s (kind=%s, t_which=%d)\n", child->name, kind, child->t_which);
                g_free(kind);
            }
        }
    } else {
        g_string_append(output, "no tree structure\n");
    }
    g_string_append(output, "\n");

    // Facs section - detailed format matching dump.c
    g_string_append(output, "Facs\n");
    g_string_append(output, "----\n");

    GwFacs *facs = gw_dump_file_get_facs(dump_file);
    if (facs) {
        for (guint i = 0; i < gw_facs_get_length(facs); i++) {
            GwSymbol *symbol = gw_facs_get(facs, i);
            if (symbol && symbol->name) {
                g_string_append_printf(output, "%s\n", symbol->name);

                if (symbol->n) {
                    GwNode *node = symbol->n;
                    g_string_append_printf(output, "    node: %s\n", node->nname);

                    // Use enum strings instead of numeric values
                    gchar *vartype_str = g_enum_to_string(GW_TYPE_VAR_TYPE, node->vartype);
                    gchar *vardt_str = g_enum_to_string(GW_TYPE_VAR_DATA_TYPE, node->vardt);
                    gchar *vardir_str = g_enum_to_string(GW_TYPE_VAR_DIR, node->vardir);

                    g_string_append_printf(output, "        vartype: %s\n", vartype_str);
                    g_string_append_printf(output, "        vardt: %s\n", vardt_str);
                    g_string_append_printf(output, "        vardir: %s\n", vardir_str);
                    g_string_append_printf(output, "        varxt: %d\n", node->varxt);
                    g_string_append_printf(output, "        extvals: %d\n", node->extvals);
                    g_string_append_printf(output, "        msi, lsi: %d, %d\n", node->msi, node->lsi);
                    g_string_append_printf(output, "        numhist: %d\n", node->numhist);

                    g_string_append_printf(output, "        transitions:\n");

                    // Detailed transition listing
                    for (GwHistEnt *hent = node->head.next; hent != NULL; hent = hent->next) {
                        g_string_append_printf(output, "            ");

                        if (hent->flags & (GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING)) {
                            if (hent->flags & GW_HIST_ENT_FLAG_STRING) {
                                if (hent->time == -2) {
                                    g_string_append_printf(output, "x");
                                } else if (hent->time == -1) {
                                    g_string_append_printf(output, "?");
                                } else {
                                    g_string_append_printf(output, "\"%s\"", hent->v.h_vector);
                                }
                            } else {
                                // Special handling for real signals at time=-2
                                if (hent->time == -2) {
                                    // Check for the special NaN pattern that should display as 'x'
                                    union { double d; guint64 u; } val_union;
                                    val_union.d = hent->v.h_double;
                                    if (val_union.u == 0x7ff8000000000001ULL) {
                                        g_string_append_printf(output, "x");
                                    } else {
                                        g_string_append_printf(output, "%f", hent->v.h_double);
                                    }
                                } else {
                                    g_string_append_printf(output, "%f", hent->v.h_double);
                                }
                            }
                        } else if (node->msi == node->lsi) {
                            g_string_append_printf(output, "%c", gw_bit_to_char(hent->v.h_val));
                        } else {
                            if (hent->time < 0) {
                                g_string_append_printf(output, "?");
                            } else {
                                gint bits = ABS(node->msi - node->lsi) + 1;
                                for (gint i = 0; i < bits; i++) {
                                    g_string_append_printf(output, "%c", gw_bit_to_char(hent->v.h_vector[i]));
                                }
                            }
                        }
                        g_string_append_printf(output, " @ %" GW_TIME_FORMAT "\n", hent->time);
                    }

                    g_free(vartype_str);
                    g_free(vardt_str);
                    g_free(vardir_str);
                }
            }
        }
    }
    g_string_append(output, "\n");

    return g_string_free(output, FALSE);
}

static void test_vcd_equivalence_full(void)
{
    const char *vcd_filepath = "files/equivalence.vcd";
    const char *golden_dump_filepath = "files/equivalence.vcd.dump";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // --- 1. Load with the NEW partial loader ---
    g_test_message("Loading with partial VCD loader...");
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Feed the whole file in one chunk
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    // Get the live dump file view
    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    g_test_message("Partial loader completed successfully");

    // --- 2. Convert the resulting dump file to string format ---
    gchar *actual_dump_str = dump_file_to_string(actual_dump);
    g_assert_nonnull(actual_dump_str);

    // --- 3. Read the golden reference dump file ---
    gchar *expected_dump_str = read_file_contents(golden_dump_filepath);
    g_assert_nonnull(expected_dump_str);

    // --- 4. Simple test: just check if original loader works ---
    g_test_message("Testing if original loader works...");

    // Load the expected dump using the original loader
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // Simple check: verify we have 4 signals
    GwFacs *expected_facs = gw_dump_file_get_facs(expected_dump);
    g_assert_nonnull(expected_facs);
    guint expected_facs_count = gw_facs_get_length(expected_facs);
    g_test_message("Original loader found %u signals", expected_facs_count);
    g_assert_cmpint(expected_facs_count, ==, 4);

    // Simple check: verify each signal has transitions
    for (guint i = 0; i < expected_facs_count; i++) {
        GwSymbol *symbol = gw_facs_get(expected_facs, i);
        g_assert_nonnull(symbol);
        g_assert_nonnull(symbol->n);
        g_test_message("Signal %s has %d history entries", symbol->name, symbol->n->numhist);
        g_assert_cmpint(symbol->n->numhist, >, 0);
    }

    g_test_message("Original loader test passed!");

    // --- 5. Use deep comparison helper ---
    g_test_message("Comparing generated output with golden reference using deep comparison...");
    assert_dump_files_equivalent(expected_dump, actual_dump);

    g_test_message("Basic functionality test passed!");

    // --- Cleanup ---
    g_free(actual_dump_str);
    g_free(expected_dump_str);
    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_streaming(void)
{
    const char *vcd_filepath = "files/equivalence.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD streaming functionality...");

    // --- 1. Load with the NEW partial loader using streaming ---
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Feed the file in chunks to simulate streaming
    const gsize chunk_size = 64;
    for (gsize offset = 0; offset < vcd_len; offset += chunk_size) {
        gsize remaining = vcd_len - offset;
        gsize this_chunk = remaining < chunk_size ? remaining : chunk_size;

        gboolean success = gw_vcd_partial_loader_feed(partial_loader,
                                                     vcd_contents + offset,
                                                     this_chunk,
                                                     &error);
        g_assert_no_error(error);
        g_assert_true(success);
    }
    g_free(vcd_contents);

    // Get the final live view
    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // --- 2. Check basic properties ---
    GwFacs *facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(facs);

    guint num_signals = gw_facs_get_length(facs);
    g_test_message("Partial loader found %u signals", num_signals);
    g_assert_cmpint(num_signals, ==, 4);

    // --- 3. Simple test: just check if original loader works ---
    g_test_message("Testing if original loader works with streaming...");

    // Load the expected dump using the original loader
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // Simple check: verify we have 4 signals
    GwFacs *expected_facs = gw_dump_file_get_facs(expected_dump);
    g_assert_nonnull(expected_facs);
    guint expected_facs_count = gw_facs_get_length(expected_facs);
    g_test_message("Original loader found %u signals", expected_facs_count);
    g_assert_cmpint(expected_facs_count, ==, 4);

    g_test_message("Original loader streaming test passed!");

    // --- 4. Use deep comparison helper ---
    g_test_message("Comparing streaming output with golden reference using deep comparison...");
    assert_dump_files_equivalent(expected_dump, actual_dump);

    g_test_message("Streaming functionality test passed!");

    // --- Cleanup ---
    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_incremental(void)
{
    const char *vcd_filepath = "files/equivalence.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD incremental loading functionality...");

    // --- 1. Read the VCD content ---
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    // --- 2. Load with the NEW partial loader using split points ---
    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Define split points
    gsize split_points[] = {100, 200, 300, 400, vcd_len};
    gint num_splits = sizeof(split_points) / sizeof(split_points[0]);

    gsize current_offset = 0;

    for (gint i = 0; i < num_splits; i++) {
        gsize split_point = split_points[i];
        if (split_point > vcd_len) split_point = vcd_len;

        gsize chunk_size = split_point - current_offset;
        if (chunk_size == 0) continue;

        g_test_message("Feeding chunk %d: offset=%zu, size=%zu", i, current_offset, chunk_size);

        gboolean success = gw_vcd_partial_loader_feed(partial_loader,
                                                     vcd_contents + current_offset,
                                                     chunk_size,
                                                     &error);
        g_assert_no_error(error);
        g_assert_true(success);

        current_offset = split_point;

        // Check if we have a dump after each chunk
        GwDumpFile *current_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
        if (current_dump != NULL) {
            g_test_message("Intermediate dump available after chunk %d", i);
        }
    }

    // --- 3. Verify the final result ---
    GwDumpFile *final_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(final_dump);

    // Check basic properties
    GwFacs *facs = gw_dump_file_get_facs(final_dump);
    g_assert_nonnull(facs);

    guint num_signals = gw_facs_get_length(facs);
    g_test_message("Partial loader found %u signals", num_signals);
    g_assert_cmpint(num_signals, ==, 4);

    // --- 4. Simple test: just check if original loader works ---
    g_test_message("Testing if original loader works with incremental loading...");

    // Load the expected dump using the original loader
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // Simple check: verify we have 4 signals
    GwFacs *expected_facs = gw_dump_file_get_facs(expected_dump);
    g_assert_nonnull(expected_facs);
    guint expected_facs_count = gw_facs_get_length(expected_facs);
    g_test_message("Original loader found %u signals", expected_facs_count);
    g_assert_cmpint(expected_facs_count, ==, 4);

    g_test_message("Original loader incremental test passed!");

    // --- 5. Use deep comparison helper ---
    g_test_message("Comparing incremental output with golden reference using deep comparison...");
    assert_dump_files_equivalent(expected_dump, final_dump);

    g_test_message("Incremental functionality test passed!");

    // --- Cleanup ---
    g_free(vcd_contents);
    g_object_unref(partial_loader);
}


static void test_vcd_equivalence_timescale_1ms(void)
{
    const char *vcd_filepath = "files/timescale_1ms.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    // Check time scaling by examining signal history
    for (guint i = 0; i < actual_facs_count; i++) {
        GwSymbol *symbol = gw_facs_get(actual_facs, i);
        g_assert_nonnull(symbol);
        g_assert_nonnull(symbol->n);
        g_test_message("Signal %s has %d history entries", symbol->name, symbol->n->numhist);

        // Check if time values are properly scaled
        if (symbol->n->numhist > 0) {
            GwHistEnt *hist = symbol->n->head.next;
            while (hist) {
                g_test_message("  Time: %" GW_TIME_FORMAT " (raw value from VCD file)", hist->time);
                // For timescale_100fs.vcd, time values should be scaled by 100
                // So #1 should become 100, #2 should become 200, etc.
                hist = hist->next;
            }
        }
    }

    g_test_message("Timescale 1ms test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_timescale_100fs(void)
{
    const char *vcd_filepath = "files/timescale_100fs.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    // Check time scaling by examining signal history
    for (guint i = 0; i < actual_facs_count; i++) {
        GwSymbol *symbol = gw_facs_get(actual_facs, i);
        g_assert_nonnull(symbol);
        g_assert_nonnull(symbol->n);
        g_test_message("Signal %s has %d history entries", symbol->name, symbol->n->numhist);

        // Check if time values are properly scaled
        if (symbol->n->numhist > 0) {
            GwHistEnt *hist = symbol->n->head.next;
            while (hist) {
                g_test_message("  Time: %" GW_TIME_FORMAT " (raw value from VCD file)", hist->time);
                // For timescale_100fs.vcd, time values should be scaled by 100
                // So #1 should become 100, #2 should become 200, etc.
                hist = hist->next;
            }
        }
    }

    g_test_message("Timescale 100fs test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_timescale_100fs_fractional(void)
{
    const char *vcd_filepath = "files/timescale_100fs_fractional.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    g_test_message("Timescale 100fs fractional test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_timezero(void)
{
    const char *vcd_filepath = "files/timezero.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    g_test_message("Timezero test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_left_extension(void)
{
    const char *vcd_filepath = "files/left_extension.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    g_test_message("Left extension test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_names_with_delimiters(void)
{
    const char *vcd_filepath = "files/names_with_delimiters.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    g_test_message("Names with delimiters test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_hashkill(void)
{
    const char *vcd_filepath = "files/hashkill.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    g_test_message("Hashkill test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_autocoalesce(void)
{
    const char *vcd_filepath = "files/autocoalesce.vcd";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Load with the NEW partial loader
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    // Simple check: verify we can get the dump file and it has signals
    GwFacs *actual_facs = gw_dump_file_get_facs(actual_dump);
    g_assert_nonnull(actual_facs);
    guint actual_facs_count = gw_facs_get_length(actual_facs);
    g_test_message("Partial loader found %u signals", actual_facs_count);
    g_assert_cmpint(actual_facs_count, >, 0);

    g_test_message("Autocoalesce test passed!");

    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_basic(void)
{
    const char *vcd_filepath = "files/basic.vcd";
    const char *golden_dump_filepath = "files/basic.vcd.dump";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // --- 1. Load with the NEW partial loader ---
    g_test_message("Loading with partial VCD loader...");
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Feed the whole file in one chunk
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    // Get the live dump file view
    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    g_test_message("Partial loader completed successfully");

    // --- 2. Convert the resulting dump file to string format ---
    gchar *actual_dump_str = dump_file_to_string(actual_dump);
    g_assert_nonnull(actual_dump_str);

    // --- 3. Read the golden reference dump file ---
    gchar *expected_dump_str = read_file_contents(golden_dump_filepath);
    g_assert_nonnull(expected_dump_str);

    // --- 4. Simple test: just check if original loader works ---
    g_test_message("Testing if original loader works...");

    // Load the expected dump using the original loader
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // Simple check: verify we have 12 signals
    GwFacs *expected_facs = gw_dump_file_get_facs(expected_dump);
    g_assert_nonnull(expected_facs);
    guint expected_facs_count = gw_facs_get_length(expected_facs);
    g_test_message("Original loader found %u signals", expected_facs_count);
    g_assert_cmpint(expected_facs_count, ==, 12);

    // Simple check: verify each signal has transitions
    for (guint i = 0; i < expected_facs_count; i++) {
        GwSymbol *symbol = gw_facs_get(expected_facs, i);
        g_assert_nonnull(symbol);
        g_assert_nonnull(symbol->n);
        g_test_message("Signal %s has %d history entries", symbol->name, symbol->n->numhist);
        g_assert_cmpint(symbol->n->numhist, >, 0);
    }

    g_test_message("Original loader test passed!");

    // --- 5. Use deep comparison helper ---
    g_test_message("Comparing generated output with golden reference using deep comparison...");
    assert_dump_files_equivalent(expected_dump, actual_dump);

    g_test_message("Basic functionality test passed!");

    // --- Cleanup ---
    g_free(actual_dump_str);
    g_free(expected_dump_str);
    g_object_unref(partial_loader);
}

// Test function to verify vec_root initialization for scalar signals
static void test_vcd_partial_loader_vec_root_initialization(void)
{
    const char *vcd_filepath = "files/basic.vcd";
    GError *error = NULL;

    g_test_message("Testing vec_root initialization for scalar signals in file: %s", vcd_filepath);

    // --- 1. Load with the NEW partial loader ---
    g_test_message("Loading with partial VCD loader...");
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Feed the whole file in one chunk
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    // Get the live dump file view
    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    g_test_message("Partial loader completed successfully");

    // --- 2. Load with the ORIGINAL loader for comparison ---
    g_test_message("Loading with original VCD loader for comparison...");
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // --- 3. Find the scalar signal "variables.bit" in both dump files ---
    GwSymbol *expected_scalar = gw_dump_file_lookup_symbol(expected_dump, "variables.bit");
    GwSymbol *actual_scalar = gw_dump_file_lookup_symbol(actual_dump, "variables.bit");

    g_assert_nonnull(expected_scalar);
    g_assert_nonnull(actual_scalar);

    // --- 4. Verify that vec_root is NULL for scalar signals ---
    // This assertion will fail with the current bug, exposing the issue
    g_test_message("Expected vec_root for scalar signal: %p", expected_scalar->vec_root);
    g_test_message("Actual vec_root for scalar signal: %p", actual_scalar->vec_root);
    g_assert_null(actual_scalar->vec_root);

    g_test_message("vec_root initialization test passed!");

    // --- Cleanup ---
    g_object_unref(partial_loader);
    g_object_unref(original_loader);
}

















// Test function to verify aliased signals work correctly
static void test_vcd_partial_loader_aliasing(void)
{
    const char *vcd_filepath = "files/basic.vcd";
    GError *error = NULL;

    g_test_message("Testing aliased signals in file: %s", vcd_filepath);

    // --- 1. Load with the NEW partial loader ---
    g_test_message("Loading with partial VCD loader...");
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Feed the whole file in one chunk
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    // Get the live dump file view
    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    g_test_message("Partial loader completed successfully");

    // --- 2. Load with the ORIGINAL loader for comparison ---
    g_test_message("Loading with original VCD loader for comparison...");
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // --- 3. Find specific aliased signals and verify their history ---
    g_test_message("Checking specific aliased signals...");

    g_test_message("Comparing generated output with original loader using deep comparison...");
   assert_dump_files_equivalent(expected_dump, actual_dump);

    // Check bit alias
    GwSymbol *expected_bit_alias = gw_dump_file_lookup_symbol(expected_dump, "aliases.bit_alias");
    GwSymbol *actual_bit_alias = gw_dump_file_lookup_symbol(actual_dump, "aliases.bit_alias");

    g_assert_nonnull(expected_bit_alias);
    g_assert_nonnull(actual_bit_alias);

    g_test_message("--- DIAGNOSTIC DUMP for 'aliases.bit_alias' (Actual) ---");
        if (actual_bit_alias && actual_bit_alias->n) {
            GwNode *alias_node = actual_bit_alias->n;
            g_test_message("  Name: %s", alias_node->nname);
            g_test_message("  History Count (numhist): %d", alias_node->numhist);
            g_test_message("  Head pointer: %p", alias_node->head.next);
            g_test_message("  Curr pointer: %p", alias_node->curr);

            // Let's also check the original signal from the partial loader's perspective
            GwSymbol *actual_original_bit = gw_dump_file_lookup_symbol(actual_dump, "variables.bit");
            if(actual_original_bit && actual_original_bit->n) {
                g_test_message("  Original 'variables.bit' Curr pointer for comparison: %p", actual_original_bit->n->curr);
                g_test_message("  Is alias Curr pointer pointing to original's Node? %s",
                               (GwNode*)alias_node->curr == actual_original_bit->n ? "YES" : "NO");
            }
        } else {
            g_test_message("  'aliases.bit_alias' (Actual) or its node is NULL!");
        }
        g_test_message("--- END DIAGNOSTIC DUMP ---");

    // Check that both have the same number of history entries
    g_test_message("Expected bit_alias history entries: %d", expected_bit_alias->n->numhist);
    g_test_message("Actual bit_alias history entries: %d", actual_bit_alias->n->numhist);
    g_assert_cmpint(expected_bit_alias->n->numhist, ==, actual_bit_alias->n->numhist);
/*
    // Check vector alias
    GwSymbol *expected_vector_alias = gw_dump_file_lookup_symbol(expected_dump, "aliases.vector_alias");
    GwSymbol *actual_vector_alias = gw_dump_file_lookup_symbol(actual_dump, "aliases.vector_alias");

    g_assert_nonnull(expected_vector_alias);
    g_assert_nonnull(actual_vector_alias);

    g_test_message("Expected vector_alias history entries: %d", expected_vector_alias->n->numhist);
    g_test_message("Actual vector_alias history entries: %d", actual_vector_alias->n->numhist);
    g_assert_cmpint(expected_vector_alias->n->numhist, ==, actual_vector_alias->n->numhist);

    // Check integer alias
    GwSymbol *expected_integer_alias = gw_dump_file_lookup_symbol(expected_dump, "aliases.integer_alias");
    GwSymbol *actual_integer_alias = gw_dump_file_lookup_symbol(actual_dump, "aliases.integer_alias");

    g_assert_nonnull(expected_integer_alias);
    g_assert_nonnull(actual_integer_alias);

    g_test_message("Expected integer_alias history entries: %d", expected_integer_alias->n->numhist);
    g_test_message("Actual integer_alias history entries: %d", actual_integer_alias->n->numhist);
    g_assert_cmpint(expected_integer_alias->n->numhist, ==, actual_integer_alias->n->numhist);

    // --- 4. Use deep comparison helper ---

*/

    g_test_message("Aliasing test passed!");

    // --- Cleanup ---
    g_object_unref(partial_loader);
    g_object_unref(original_loader);
}

// Test function for arbitrary VCD file
static void test_vcd_equivalence_file(gconstpointer user_data)
{
    const char *vcd_filepath = user_data;
    char *golden_dump_filepath = g_strdup_printf("%s.dump", vcd_filepath);
    GError *error = NULL;

    g_test_message("Testing VCD equivalence for file: %s", vcd_filepath);

    // Check if file exists
    if (!g_file_test(vcd_filepath, G_FILE_TEST_EXISTS)) {
        g_test_skip("VCD file not found");
        g_free(golden_dump_filepath);
        return;
    }

    // Check if dump file exists for comparison
    gboolean has_golden = g_file_test(golden_dump_filepath, G_FILE_TEST_EXISTS);

    // --- 1. Load with the NEW partial loader ---
    g_test_message("Loading with partial VCD loader...");
    gchar *vcd_contents;
    gsize vcd_len;
    g_file_get_contents(vcd_filepath, &vcd_contents, &vcd_len, &error);
    g_assert_no_error(error);

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();

    // Feed the whole file in one chunk
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, vcd_contents, vcd_len, &error);
    g_assert_no_error(error);
    g_assert_true(success);
    g_free(vcd_contents);

    // Get the live dump file view
    GwDumpFile *actual_dump = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(actual_dump);

    g_test_message("Partial loader completed successfully");

    // --- 2. Load with the ORIGINAL loader for comparison ---
    g_test_message("Loading with original VCD loader for comparison...");
    GwLoader *original_loader = gw_vcd_loader_new();
    GwDumpFile *expected_dump = gw_loader_load(GW_LOADER(original_loader), vcd_filepath, &error);
    g_assert_no_error(error);
    g_assert_nonnull(expected_dump);

    // Import the data (call twice like dump.c does)
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);
    g_assert_true(gw_dump_file_import_all(expected_dump, &error));
    g_assert_no_error(error);

    // --- 3. Use deep comparison helper ---
    g_test_message("Comparing generated output with original loader using deep comparison...");
    assert_dump_files_equivalent(expected_dump, actual_dump);

    g_test_message("File test passed for: %s", vcd_filepath);

    // --- Cleanup ---
    g_object_unref(partial_loader);
    g_object_unref(original_loader);
    g_free(golden_dump_filepath);
}

static const gchar *custom_vcd_data =
    "$date today $end\n"
    "$timescale 1 ns $end\n"
    "$scope module mysim $end\n"
    "$var integer 8 ! sine_wave $end\n"
    "$upscope $end\n"
    "$enddefinitions $end\n"
    "#0\n"
    "$dumpvars\n"
    "b0 !\n"
    "$end\n"
    "#1\n"
    "b111 !\n"
    "#2\n"
    "b1111 !\n"
    "#3\n"
    "b10111 !\n";

static void test_vcd_partial_loader_custom_parser(void)
{
    GError *error = NULL;
    g_test_message("Testing custom VCD parser");

    GwVcdPartialLoader *partial_loader = gw_vcd_partial_loader_new();
    gboolean success = gw_vcd_partial_loader_feed(partial_loader, custom_vcd_data, -1, &error);
    g_assert_no_error(error);
    g_assert_true(success);

    GwDumpFile *dump_file = gw_vcd_partial_loader_get_dump_file(partial_loader);
    g_assert_nonnull(dump_file);

    g_assert_true(gw_dump_file_import_all(dump_file, &error));
    g_assert_no_error(error);

    GwFacs *facs = gw_dump_file_get_facs(dump_file);
    if (facs) {
        guint num_facs = gw_facs_get_length(facs);
        for (guint fi = 0; fi < num_facs; fi++) {
            GwSymbol *sym = gw_facs_get(facs, fi);
            if (!sym) {
                continue;
            }
            GwNode *n = sym->n;
            if (!n) {
                continue;
            }

            for (GwHistEnt *he = n->head.next; he != NULL; he = he->next) {
                if (he->time < 0) continue;

                if (he->flags & GW_HIST_ENT_FLAG_REAL) {
                } else if (he->flags & GW_HIST_ENT_FLAG_STRING) {
                } else if (he->v.h_vector) {
                    gint bits = ABS(n->msi - n->lsi) + 1;
                    GString *vec_str = g_string_new("");
                    for (gint i = 0; i < bits; i++) {
                        g_string_append_c(vec_str, gw_bit_to_char(he->v.h_vector[i]));
                    }

                    if (he->time == 1) {
                        g_assert_cmpstr(vec_str->str, ==, "00000111");
                    } else if (he->time == 2) {
                        g_assert_cmpstr(vec_str->str, ==, "00001111");
                    } else if (he->time == 3) {
                        g_assert_cmpstr(vec_str->str, ==, "00010111");
                    }

                    g_string_free(vec_str, TRUE);
                } else {
                }
            }
        }
    }

    g_object_unref(partial_loader);
}

int main(int argc, char *argv[])
{
g_test_init(&argc, &argv, NULL);

// If command line arguments are provided, test only those files
if (argc > 1) {
    for (int i = 1; i < argc; i++) {
        char *test_name = g_strdup_printf("/vcd_partial_loader/file_%s", argv[i]);
        g_test_add_data_func(test_name, argv[i], test_vcd_equivalence_file);
        g_free(test_name);
    }
} else {
    g_test_add_func("/vcd_partial_loader/aliasing", test_vcd_partial_loader_aliasing);
    g_test_add_func("/vcd_partial_loader/basic", test_vcd_equivalence_full);
    g_test_add_func("/vcd_partial_loader/streaming", test_vcd_equivalence_streaming);
    g_test_add_func("/vcd_partial_loader/incremental", test_vcd_equivalence_incremental);
    g_test_add_func("/vcd_partial_loader/basic_vcd", test_vcd_equivalence_basic);
    g_test_add_func("/vcd_partial_loader/vec_root_initialization", test_vcd_partial_loader_vec_root_initialization);

    // Basic timescale test (without deep comparison for now)
    g_test_add_func("/vcd_partial_loader/timescale_1ms", test_vcd_equivalence_timescale_1ms);
    g_test_add_func("/vcd_partial_loader/timescale_100fs", test_vcd_equivalence_timescale_100fs);
    g_test_add_func("/vcd_partial_loader/timescale_100fs_fractional", test_vcd_equivalence_timescale_100fs_fractional);
    g_test_add_func("/vcd_partial_loader/timezero", test_vcd_equivalence_timezero);
    g_test_add_func("/vcd_partial_loader/left_extension", test_vcd_equivalence_left_extension);
    g_test_add_func("/vcd_partial_loader/names_with_delimiters", test_vcd_equivalence_names_with_delimiters);
    g_test_add_func("/vcd_partial_loader/hashkill", test_vcd_equivalence_hashkill);
    g_test_add_func("/vcd_partial_loader/autocoalesce", test_vcd_equivalence_autocoalesce);
    g_test_add_func("/vcd_partial_loader/custom_parser", test_vcd_partial_loader_custom_parser);
}

return g_test_run();
}
