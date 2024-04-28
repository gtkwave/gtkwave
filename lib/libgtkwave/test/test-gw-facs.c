#include <gtkwave.h>

// TODO: replace with gw_tree_node_new or similar
static GwTreeNode *alloc_node(const gchar *name, gint which)
{
    GwTreeNode *node = g_malloc0(sizeof(GwTreeNode) + strlen(name) + 1);
    strcpy(node->name, name);
    node->t_which = which;

    return node;
}

static void test_order_from_tree()
{
    GwTreeNode *root = alloc_node("a", 4);
    root->next = alloc_node("b", 0);
    root->next->next = alloc_node("c", 3);
    root->child = alloc_node("a.a", 1);
    root->child->next = alloc_node("a.b", 2);

    GwTree *tree = gw_tree_new(root);

    GwSymbol *symbols = g_new0(GwSymbol, 5);
    symbols[0].name = "b";
    symbols[1].name = "a.a";
    symbols[2].name = "a.b";
    symbols[3].name = "c";
    symbols[4].name = "a";

    GwFacs *facs = gw_facs_new(5);
    for (gint i = 0; i < 5; i++) {
        gw_facs_set(facs, i, &symbols[i]);
    }

    gw_facs_order_from_tree(facs, tree);

    g_assert_cmpstr(gw_facs_get(facs, 0)->name, ==, "c");
    g_assert_cmpstr(gw_facs_get(facs, 1)->name, ==, "b");
    g_assert_cmpstr(gw_facs_get(facs, 2)->name, ==, "a");
    g_assert_cmpstr(gw_facs_get(facs, 3)->name, ==, "a.b");
    g_assert_cmpstr(gw_facs_get(facs, 4)->name, ==, "a.a");
}

static void test_sort()
{
    GwSymbol *symbols = g_new0(GwSymbol, 5);
    symbols[0].name = "b";
    symbols[1].name = "a.a";
    symbols[2].name = "a.b";
    symbols[3].name = "c";
    symbols[4].name = "a";

    GwFacs *facs = gw_facs_new(5);
    for (gint i = 0; i < 5; i++) {
        gw_facs_set(facs, i, &symbols[i]);
    }

    gw_facs_sort(facs);

    g_assert_cmpstr(gw_facs_get(facs, 0)->name, ==, "a");
    g_assert_cmpstr(gw_facs_get(facs, 1)->name, ==, "a.a");
    g_assert_cmpstr(gw_facs_get(facs, 2)->name, ==, "a.b");
    g_assert_cmpstr(gw_facs_get(facs, 3)->name, ==, "b");
    g_assert_cmpstr(gw_facs_get(facs, 4)->name, ==, "c");
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/facs/order_from_tree", test_order_from_tree);
    g_test_add_func("/facs/sort", test_sort);

    return g_test_run();
}