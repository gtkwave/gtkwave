#include <gtkwave.h>

// TODO: check generated HistEnts
// TODO: add 2D test
static void test_expand(void)
{
    GwLoader *loader = gw_fst_loader_new();
    GwDumpFile *file = gw_loader_load(loader, "files/basic.fst", NULL);
    g_assert_nonnull(file);
    g_object_unref(loader);

    GwSymbol *vector = gw_dump_file_lookup_symbol(file, "variables.vector[7:0]");
    g_assert_nonnull(vector);


    GwExpandInfo *expand_info = gw_node_expand(vector->n);
    g_assert_nonnull(expand_info);
    g_assert_cmpint(expand_info->lsb, ==, 0);
    g_assert_cmpint(expand_info->msb, ==, 7);
    g_assert_cmpint(expand_info->width, ==, 8);

    for (gint i = 0; i < 8; i++) {
        GwNode *node = expand_info->narray[i];
        g_assert_nonnull(node);

        g_assert_cmpint(node->msi, ==, 0);
        g_assert_cmpint(node->lsi, ==, 0);

        gchar *expected_name = g_strdup_printf("variables.vector[%d]", 7 - i);
        g_assert_cmpstr(node->nname, ==, expected_name);
        g_free(expected_name);
    }

    g_object_unref(file);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/node/expand", test_expand);

    return g_test_run();
}