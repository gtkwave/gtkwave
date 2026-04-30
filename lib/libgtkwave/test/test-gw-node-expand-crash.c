/*
 * Copyright (c) 2024 GTKWave Contributors
 *
 * Test case to reproduce node expansion crash when expanding a vector node
 * that has scalar history entries (h_val) instead of vector entries (h_vector).
 *
 * This simulates the bug found when streaming VCD data where a multi-bit vector
 * has its history stored as scalars but the expansion code tries to access
 * them as vectors.
 */

#include <gtkwave.h>

static void test_expand_with_scalar_history_subprocess(void)
{
    // Create a node that looks like an 8-bit vector but has scalar history
    GwNode *node = g_new0(GwNode, 1);
    node->nname = g_strdup("mysim.sine_wave");
    node->extvals = 1;
    node->msi = 7;
    node->lsi = 0;
    node->vartype = 2;

    // Set up history array
    node->head.time = 0;
    node->head.v.h_val = 0;
    node->head.next = NULL;
    node->curr = &node->head;

    GwHistEnt *h1 = g_new0(GwHistEnt, 1);
    h1->time = 1;
    h1->v.h_val = 1;
    node->curr->next = h1;
    node->curr = h1;

    node->numhist = 2;

    node->harray = g_new(GwHistEnt *, 2);
    node->harray[0] = &node->head;
    node->harray[1] = h1;

    // This should now call g_error() and terminate the subprocess
    gw_node_expand(node);

    g_test_fail_printf("gw_node_expand did not cause a fatal error as expected");
}

static void test_expand_vector_with_scalar_history(void)
{
    g_test_trap_subprocess("/node/expand_vector_with_scalar_history/subprocess", 0, 0);
    g_test_trap_assert_failed();
    g_test_trap_assert_stderr("*ERROR*Cannot expand vector 'mysim.sine_wave'*");
}

static void test_expand_vector_with_proper_vector_history(void)
{
    // Test that proper vector nodes with h_vector history still expand correctly
    GwNode *node = g_new0(GwNode, 1);
    node->nname = g_strdup("test.vector");
    node->extvals = 1;
    node->msi = 3;
    node->lsi = 0;

    // Set up proper vector history
    node->head.time = -2;
    node->head.v.h_val = GW_BIT_X;
    node->head.next = NULL;
    
    GwHistEnt *h_init = g_new0(GwHistEnt, 1);
    h_init->time = -1;
    h_init->v.h_val = GW_BIT_Z;
    node->head.next = h_init;

    GwHistEnt *h1 = g_new0(GwHistEnt, 1);
    h1->time = 0;
    h1->v.h_vector = g_malloc(4);
    memset(h1->v.h_vector, GW_BIT_0, 4);
    h_init->next = h1;
    
    node->curr = h1;
    node->numhist = 3;
    
    node->harray = g_new(GwHistEnt *, 3);
    node->harray[0] = &node->head;
    node->harray[1] = h_init;
    node->harray[2] = h1;
    
    // This should succeed
    GwExpandInfo *expand_info = gw_node_expand(node);
    g_assert_nonnull(expand_info);
    
    // Clean up
    for (int i = 0; i < expand_info->width; i++) {
        if (expand_info->narray[i]) {
            g_free(expand_info->narray[i]->harray);
            g_free(expand_info->narray[i]->nname);
            g_free(expand_info->narray[i]->expansion);
            g_free(expand_info->narray[i]);
        }
    }
    gw_expand_info_free(expand_info);
    
    g_free(node->harray);
    g_free(node->nname);
    g_free(h1->v.h_vector);
    g_free(h_init);
    g_free(h1);
    g_free(node);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/node/expand_vector_with_scalar_history", test_expand_vector_with_scalar_history);
    g_test_add_func("/node/expand_vector_with_scalar_history/subprocess", test_expand_with_scalar_history_subprocess);
    g_test_add_func("/node/expand_vector_with_proper_vector_history", test_expand_vector_with_proper_vector_history);

    return g_test_run();
}