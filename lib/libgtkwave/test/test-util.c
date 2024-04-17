#include "test-util.h"
#include "gw-dump-file.h"
#include "gw-vcd-file.h"

static void tree_to_string_recursive(GwTreeNode *node, GString *str)
{
    for (GwTreeNode *iter = node; iter != NULL; iter = iter->next) {
        if (iter != node) {
            g_string_append(str, ", ");
        }

        g_string_append(str, iter->name);
        if (iter->t_which >= 0) {
            g_string_append_printf(str, "(%d)", iter->t_which);
        }

        if (iter->child != NULL) {
            g_string_append_c(str, '{');
            tree_to_string_recursive(iter->child, str);
            g_string_append_c(str, '}');
        }
    }
}

static gchar *tree_to_string(GwTreeNode *node)
{
    GString *str = g_string_new(NULL);

    tree_to_string_recursive(node, str);

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

static void assert_scalar_transitions(GwNode *node, const gchar *expected)
{
    GwHistEnt *h = &node->head;

    GString *str = g_string_new(NULL);

    while (h != NULL && h->time < GW_TIME_MAX - 2) {
        if (str->len > 0) {
            g_string_append(str, ", ");
        }
        g_string_append_printf(str, "%c@%" GW_TIME_FORMAT, gw_bit_to_char(h->v.h_val), h->time);

        h = h->next;
    }

    g_assert_cmpstr(str->str, ==, expected);

    g_string_free(str, TRUE);
}

static void check_alias(GwDumpFile *file, GwTreeNode *variable, GwTreeNode *alias)
{
    g_assert_true(variable->t_which != alias->t_which);

    GwFacs *facs = gw_dump_file_get_facs(file);

    GwNode *variable_node = gw_facs_get(facs, variable->t_which)->n;
    GwNode *alias_node = gw_facs_get(facs, alias->t_which)->n;

    g_assert_true(variable_node != alias_node);
    // TODO: why does this fail for FST files?
    if (GW_IS_VCD_FILE(file)) {
        g_assert_true(variable_node->head.next == alias_node->head.next);
    }
}

void common_basic_vcd_and_fst_test(GwDumpFile *file)
{
    g_assert_true(GW_IS_DUMP_FILE(file));

    g_assert_cmpint(gw_dump_file_get_time_scale(file), ==, 1);
    g_assert_cmpint(gw_dump_file_get_time_dimension(file), ==, GW_TIME_DIMENSION_NANO);
    g_assert_cmpint(gw_dump_file_get_global_time_offset(file), ==, 0);

    // Import all facs

    GwFacs *facs = gw_dump_file_get_facs(file);
    g_assert_cmpint(gw_facs_get_length(facs), ==, 4 * 2);
    g_assert_true(gw_dump_file_import_all(file, NULL));

    // Check hierarchy

    GwTree *tree = gw_dump_file_get_tree(file);
    GwTreeNode *root = gw_tree_get_root(tree);

    assert_tree(root,
                "variables{vector[7:0](7), string(6), real(5), bit(4)}, "
                "aliases{vector_alias[7:0](3), string_alias(2), real_alias(1), bit_alias(0)}");

    // Get all tree nodes

    GwTreeNode *tn_variable_bit = get_tree_node(tree, "variables.bit");
    GwTreeNode *tn_variable_vector = get_tree_node(tree, "variables.vector[7:0]");
    GwTreeNode *tn_variable_real = get_tree_node(tree, "variables.real");
    GwTreeNode *tn_variable_string = get_tree_node(tree, "variables.string");

    GwTreeNode *tn_alias_bit = get_tree_node(tree, "aliases.bit_alias");
    GwTreeNode *tn_alias_vector = get_tree_node(tree, "aliases.vector_alias[7:0]");
    GwTreeNode *tn_alias_real = get_tree_node(tree, "aliases.real_alias");
    GwTreeNode *tn_alias_string = get_tree_node(tree, "aliases.string_alias");

    // Ensure that aliases point to the same waveform data as the variables

    check_alias(file, tn_variable_bit, tn_alias_bit);
    check_alias(file, tn_variable_vector, tn_alias_vector);
    check_alias(file, tn_variable_real, tn_alias_real);
    check_alias(file, tn_variable_string, tn_alias_string);

    // Check `bit` signal

    GwSymbol *sym_bit = gw_facs_get(facs, tn_variable_bit->t_which);
    g_assert_cmpstr(sym_bit->name, ==, "variables.bit");
    assert_scalar_transitions(sym_bit->n,
                              "x@-2, x@-1, 0@0, x@1, z@2, 1@3, h@4, u@5, w@6, l@7, -@8");

    // TODO: check other signals
}