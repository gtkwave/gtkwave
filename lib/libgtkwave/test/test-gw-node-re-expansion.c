/*
 * Copyright (c) 2024 GTKWave Contributors
 *
 * Test case to verify that re-expanding a vector node correctly updates
 * its children with new history data, simulating a real-time update.
 */

#include <gtkwave.h>
#include <glib.h>

// Helper function to create a simple vector history entry
static GwHistEnt* create_vector_entry(GwTime time, const char* value_str) {
    GwHistEnt *he = g_new0(GwHistEnt, 1);
    he->time = time;
    int len = strlen(value_str);
    he->v.h_vector = g_new(char, len);
    for (int i = 0; i < len; i++) {
        he->v.h_vector[i] = gw_bit_from_char(value_str[i]);
    }
    return he;
}

static void test_node_re_expansion_updates_children(void)
{
    g_test_message("Setting up parent vector node and initial history...");

    // --- ARRANGE (Part 1): Initial Setup ---
    GwNode *parent = g_new0(GwNode, 1);
    parent->nname = g_strdup("test.vector[3:0]");
    parent->extvals = 1;
    parent->msi = 3;
    parent->lsi = 0;

    // Create initial history:
    // t=-2: x
    // t=10: 0001
    // t=20: 0010
    parent->head.time = -2;
    parent->head.v.h_val = GW_BIT_X;

    GwHistEnt *h1 = create_vector_entry(10, "0001");
    GwHistEnt *h2 = create_vector_entry(20, "0010");

    parent->head.next = h1;
    h1->next = h2;
    parent->curr = h2;
    parent->numhist = 3; // head, h1, h2

    // Manually build the harray that gw_node_expand() needs
    parent->harray = g_new(GwHistEnt*, 3);
    parent->harray[0] = &parent->head;
    parent->harray[1] = h1;
    parent->harray[2] = h2;


    // --- ACT (Part 1): Initial Expansion ---
    g_test_message("Performing initial expansion...");
    GwExpandInfo *expand_info1 = gw_node_expand(parent);

    // --- ASSERT (Part 1): Verify Initial Children ---
    g_assert_nonnull(expand_info1);
    g_assert_true(parent->expand_info == expand_info1); // Check parent link
    g_assert_cmpint(expand_info1->width, ==, 4);

    // Check child bit 0 (the LSB)
    GwNode *child_bit0 = expand_info1->narray[3]; // [3:0] -> index 3 is bit 0
    g_assert_nonnull(child_bit0);
    g_assert_cmpstr(child_bit0->nname, ==, "test.vector[0]");

    // Verify child bit 0's history: should have values 1 (at t=10) and 0 (at t=20)
    // Note: numhist might be 2 if initial 'x' is optimized out
    g_assert_cmpint(child_bit0->numhist, >=, 2);
    GwHistEnt *child_hist = child_bit0->head.next; // Skip head
    g_assert_nonnull(child_hist);
    g_assert_cmpint(child_hist->time, ==, 10);
    g_assert_cmpint(child_hist->v.h_val, ==, GW_BIT_1);
    child_hist = child_hist->next;
    g_assert_nonnull(child_hist);
    g_assert_cmpint(child_hist->time, ==, 20);
    g_assert_cmpint(child_hist->v.h_val, ==, GW_BIT_0);
    g_assert_null(child_hist->next); // Should be the end

    g_test_message("Initial expansion verified successfully.");


    // --- ARRANGE (Part 2): Simulate a real-time update ---
    g_test_message("Simulating real-time update: adding new history to parent...");
    GwHistEnt *h3 = create_vector_entry(30, "0101"); // bit 0 changes from 0 to 1
    parent->curr->next = h3; // Append to linked list
    parent->curr = h3;
    parent->numhist++;

    // Invalidate the parent's harray, which is the trigger for re-expansion.
    // The UI loop would then see this and call gw_node_expand again.
    g_free(parent->harray);
    parent->harray = NULL;


    // --- ACT (Part 2): Re-Expansion ---
    // The UI loop calls this. `gw_node_expand` should now see harray is NULL,
    // rebuild it from the updated linked list, and then perform the expansion.
    // It should also free the old `expand_info1` and its children.
    g_test_message("Performing re-expansion...");
    
    // Save a reference to one of the old child nodes to verify it gets replaced
    GwNode *old_child_bit0 = expand_info1->narray[3];
    
    GwExpandInfo *expand_info2 = gw_node_expand(parent);


    // --- ASSERT (Part 2): Verify Updated Children ---
    g_assert_nonnull(expand_info2);
    // expand_info1 has been freed, so we can't reliably compare pointers
    // Instead, verify that the parent's expand_info link was updated
    g_assert_true(parent->expand_info == expand_info2); // Parent link must be updated

    // Check the same child bit 0 again from the *new* expansion
    GwNode *new_child_bit0 = expand_info2->narray[3];
    g_assert_nonnull(new_child_bit0);
    // Verify that this is a different child node instance (new allocation)
    g_assert_true(new_child_bit0 != old_child_bit0);

    // Verify its history now includes the new value from t=30 (which is '1' for bit 0)
    g_test_message("Verifying updated history for child bit 0...");
    child_hist = new_child_bit0->head.next; // Start from beginning again
    g_assert_nonnull(child_hist);
    g_assert_cmpint(child_hist->time, ==, 10);
    g_assert_cmpint(child_hist->v.h_val, ==, GW_BIT_1);

    child_hist = child_hist->next;
    g_assert_nonnull(child_hist);
    g_assert_cmpint(child_hist->time, ==, 20);
    g_assert_cmpint(child_hist->v.h_val, ==, GW_BIT_0);

    // THIS IS THE CRITICAL ASSERTION
    child_hist = child_hist->next;
    if (child_hist == NULL) {
        g_test_fail_printf("New history entry at t=30 was not found in child!");
        return;
    }
    g_assert_cmpint(child_hist->time, ==, 30);
    g_assert_cmpint(child_hist->v.h_val, ==, GW_BIT_1); // Value of bit 0 in "0101"
    g_assert_null(child_hist->next); // Should now be the end

    g_test_message("Re-expansion and child update verified successfully.");


    // --- CLEANUP ---
    g_test_message("Cleaning up resources...");
    
    // Free the second (and final) expansion info and its children
    gw_expand_info_free_deep(expand_info2);
    parent->expand_info = NULL;

    // Free manually allocated history entries
    g_free(h1->v.h_vector);
    g_free(h1);
    g_free(h2->v.h_vector);
    g_free(h2);
    g_free(h3->v.h_vector);
    g_free(h3);

    // Free parent node
    g_free(parent->nname);
    g_free(parent);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/node/re_expansion_updates_children", test_node_re_expansion_updates_children);

    return g_test_run();
}
