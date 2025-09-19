#include <gtkwave.h>
#include <glib/gstdio.h>
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

    // --- 4. Compare the outputs ---
    g_test_message("Comparing generated output with golden reference...");
    g_assert_cmpstr(actual_dump_str, ==, expected_dump_str);

    g_test_message("Equivalence test passed!");

    // --- Cleanup ---
    g_free(actual_dump_str);
    g_free(expected_dump_str);
    g_object_unref(partial_loader);
}

static void test_vcd_equivalence_streaming(void)
{
    const char *vcd_filepath = "files/equivalence.vcd";
    const char *golden_dump_filepath = "files/equivalence.vcd.dump";
    GError *error = NULL;

    g_test_message("Testing VCD equivalence with streaming...");

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

    g_test_message("Partial loader streaming completed successfully");

    // --- 2. Convert the resulting dump file to string format ---
    gchar *actual_dump_str = dump_file_to_string(actual_dump);
    g_assert_nonnull(actual_dump_str);

    // --- 3. Read the golden reference dump file ---
    gchar *expected_dump_str = read_file_contents(golden_dump_filepath);
    g_assert_nonnull(expected_dump_str);

    // --- 4. Compare the outputs ---
    g_test_message("Comparing streaming output with golden reference...");
    g_assert_cmpstr(actual_dump_str, ==, expected_dump_str);

    g_test_message("Streaming equivalence test passed!");

    // --- Cleanup ---
    g_free(actual_dump_str);
    g_free(expected_dump_str);
    g_object_unref(partial_loader);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    
    g_test_add_func("/vcd_partial_loader/equivalence", test_vcd_equivalence_full);
    g_test_add_func("/vcd_partial_loader/equivalence_streaming", test_vcd_equivalence_streaming);
    
    return g_test_run();
}