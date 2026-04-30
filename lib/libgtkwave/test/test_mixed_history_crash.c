/*
 * Copyright (c) 2024 GTKWave Contributors
 *
 * Test case for the node expansion crash when a vector node has
 * mixed history entries (valid vector followed by invalid scalar).
 * This test verifies that the program terminates with a fatal error.
 */

#include <gtkwave.h>
#include "test-util.h"

static void test_expand_with_mixed_history_subprocess(void)
{
    // This function runs in a separate process and is expected to crash.

    // Create a node representing an 8-bit vector
    GwNode *node = g_new0(GwNode, 1);
    node->nname = g_strdup("mysim.mixed_wave");
    node->extvals = 1;
    node->msi = 7;
    node->lsi = 0;
    node->vartype = 2; // Integer type

    // --- History Setup ---
    // Entry 0: time = -2 (special)
    node->head.time = -2;
    node->head.v.h_val = GW_BIT_X;
    node->head.next = NULL;
    node->curr = &node->head;

    // Entry 1: time = -1 (special)
    GwHistEnt *h_init = g_new0(GwHistEnt, 1);
    h_init->time = -1;
    h_init->v.h_val = GW_BIT_Z;
    node->curr->next = h_init;
    node->curr = h_init;

    // Entry 2: time = 0 (VALID vector entry)
    GwHistEnt *h1 = g_new0(GwHistEnt, 1);
    h1->time = 0;
    h1->v.h_vector = g_malloc(8); // Properly allocated vector
    memset(h1->v.h_vector, GW_BIT_0, 8);
    node->curr->next = h1;
    node->curr = h1;

    // Entry 3: time = 1 (INVALID scalar entry)
    GwHistEnt *h2 = g_new0(GwHistEnt, 1);
    h2->time = 1;
    h2->v.h_val = 127; // h_vector is NULL, which will be caught by the check.
    node->curr->next = h2;
    node->curr = h2;

    node->numhist = 4;

    // Build harray to simulate the state before expansion
    node->harray = g_new(GwHistEnt *, node->numhist);
    node->harray[0] = &node->head;
    node->harray[1] = h_init;
    node->harray[2] = h1;
    node->harray[3] = h2;

    // This call is now expected to call g_error(), which will
    // terminate this subprocess.
    gw_node_expand(node);

    // This part should not be reached.
    g_test_fail_printf("gw_node_expand did not cause a fatal error as expected");
}

static void test_expand_vector_with_mixed_history_crash(void)
{
    // Set up the trap to run the test in a subprocess.
    g_test_trap_subprocess("/node/expand_vector_with_mixed_history_crash/subprocess", 0, 0);
    g_test_trap_assert_failed();
    g_test_trap_assert_stderr("*ERROR*Cannot expand vector 'mysim.mixed_wave'*");
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    // The main test case that sets up the trap
    g_test_add_func("/node/expand_vector_with_mixed_history_crash", test_expand_vector_with_mixed_history_crash);

    // The function that runs in the subprocess
    g_test_add_func("/node/expand_vector_with_mixed_history_crash/subprocess", test_expand_with_mixed_history_subprocess);

    return g_test_run();
}