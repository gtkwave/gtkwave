#include "test-util.h"

static void tree_to_string_recursive(GwTreeNode *node, GString *str)
{
    if (node->child == NULL) {
        g_string_append(str, node->name);
    } else {
        g_string_append_c(str, '(');
        g_string_append(str, node->name);

        for (GwTreeNode *iter = node->child; iter != NULL; iter = iter->next) {
            g_string_append_c(str, ' ');
            tree_to_string_recursive(iter, str);
        }

        g_string_append_c(str, ')');
    }
}

static gchar *tree_to_string(GwTreeNode *node)
{
    GString *str = g_string_new(NULL);

    if (node != NULL) {
        g_string_append_c(str, '(');
        for (GwTreeNode *iter = node; iter != NULL; iter = iter->next) {
            if (str->len > 1) {
                g_string_append_c(str, ' ');
            }
            tree_to_string_recursive(iter, str);
        }
        g_string_append_c(str, ')');
    }

    return g_string_free(str, FALSE);
}

void assert_tree(GwTreeNode *node, const gchar *expected)
{
    gchar *str = tree_to_string(node);
    g_assert_cmpstr(str, ==, expected);
    g_free(str);
}

static GwTreeNode *get_tree_node_flat(GwTreeNode *node, const gchar *name)
{
    while (node != NULL) {
        if (g_strcmp0(node->name, name) == 0) {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

GwTreeNode *get_tree_node(GwTree *tree, const gchar *path)
{
    gchar **parts = g_strsplit(path, ".", 0);
    guint part_count = g_strv_length(parts);
    g_assert_cmpint(part_count, >, 0);

    GwTreeNode *node = gw_tree_get_root(tree);
    for (guint i = 0; i < part_count; i++) {
        node = get_tree_node_flat(node, parts[i]);
        g_assert_nonnull(node);
        if (i != part_count - 1) {
            node = node->child;
            g_assert_nonnull(node);
        }
    }

    g_strfreev(parts);

    return node;
}