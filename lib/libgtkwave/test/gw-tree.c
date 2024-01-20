#include <gtkwave.h>
#include "test-util.h"

static GwTreeNode *alloc_node(const gchar *name)
{
    GwTreeNode *node = gw_tree_node_new(GW_TREE_KIND_UNKNOWN, name);
    node->t_which = -1;

    return node;
}

static void test_to_string(void)
{
    assert_tree(NULL, "");

    GwTreeNode *root = alloc_node("root");
    assert_tree(root, "root");

    root->next = alloc_node("root_sibling");
    assert_tree(root, "root, root_sibling");

    root->child = alloc_node("child");
    assert_tree(root, "root[child], root_sibling");

    root->child->child = alloc_node("child2");
    assert_tree(root, "root[child[child2]], root_sibling");

    root->child->child->next = alloc_node("child3");
    assert_tree(root, "root[child[child2, child3]], root_sibling");
}

static void test_sort(void)
{
    GwTreeNode *root;
    GwTree *tree;

    // Empty tree.

    tree = gw_tree_new(NULL);
    gw_tree_sort(tree);
    assert_tree(gw_tree_get_root(tree), "");
    g_object_unref(tree);

    // Single root node

    root = alloc_node("a");
    tree = gw_tree_new(root);
    gw_tree_sort(tree);
    assert_tree(gw_tree_get_root(tree), "a");
    g_object_unref(tree);

    // Three flat nodes

    root = alloc_node("a");
    root->next = alloc_node("c");
    root->next->next = alloc_node("b");

    tree = gw_tree_new(root);
    gw_tree_sort(tree);
    assert_tree(gw_tree_get_root(tree), "c, b, a");
    g_object_unref(tree);

    // Nested nodes

    root = alloc_node("a");
    root->next = alloc_node("c");
    root->next->child = alloc_node("n2");
    root->next->child->next = alloc_node("n10");
    root->next->next = alloc_node("b");

    tree = gw_tree_new(root);
    gw_tree_sort(tree);
    assert_tree(gw_tree_get_root(tree), "c[n10, n2], b, a");
    g_object_unref(tree);
}

static void test_graft(void)
{
    GwTree *tree;
    GwTreeNode *chain;

    // Graft  node to empty tree

    chain = alloc_node("a");

    tree = gw_tree_new(NULL);
    gw_tree_graft(tree, chain);
    assert_tree(gw_tree_get_root(tree), "a");
    g_object_unref(tree);

    // Graft two nodes to empty tree

    chain = alloc_node("a");
    chain->next = alloc_node("b");

    tree = gw_tree_new(NULL);
    gw_tree_graft(tree, chain);
    assert_tree(gw_tree_get_root(tree), "a, b");
    g_object_unref(tree);

    // Graft nodes to parents

    GwTreeNode *p1 = alloc_node("parent1");
    GwTreeNode *p2 = alloc_node("parent2");
    p1->next = p2;

    GwTreeNode *c1 = alloc_node("child1");
    GwTreeNode *c2 = alloc_node("child2");
    GwTreeNode *c3 = alloc_node("child3");
    c1->next = c2;
    c2->next = c3;

    c1->child = p1;
    c2->child = p1;
    c3->child = p2;

    tree = gw_tree_new(p1);
    gw_tree_graft(tree, c1);
    assert_tree(gw_tree_get_root(tree), "parent1[child2, child1], parent2[child3]");
    g_object_unref(tree);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/tree/to_string", test_to_string);
    g_test_add_func("/tree/sort", test_sort);
    g_test_add_func("/tree/graft", test_graft);

    return g_test_run();
}
