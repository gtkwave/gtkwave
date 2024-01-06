#include <gtkwave.h>
#include "test-util.h"

// TODO: remove
enum AnalyzerBits
{
    AN_0,
    AN_X,
    AN_Z,
    AN_1,
    AN_H,
    AN_U,
    AN_W,
    AN_L,
    AN_DASH,
    AN_RSV9,
    AN_RSVA,
    AN_RSVB,
    AN_RSVC,
    AN_RSVD,
    AN_RSVE,
    AN_RSVF,
    AN_COUNT
};

// TODO: remove
static gchar bit_to_char(unsigned char bit)
{
    switch (bit) {
        case AN_0:
            return '0';
        case AN_X:
            return 'x';
        case AN_Z:
            return 'z';
        case AN_1:
            return '1';
        case AN_H:
            return 'h';
        case AN_U:
            return 'u';
        case AN_W:
            return 'w';
        case AN_L:
            return 'l';
        case AN_DASH:
            return '-';
        default:
            return bit;
    }
}

static void assert_scalar_transitions(GwNode *node, const gchar *expected)
{
    GwHistEnt *h = &node->head;

    GString *str = g_string_new(NULL);

    while (h != NULL && h->time < GW_TIME_MAX - 2) {
        if (str->len > 0) {
            g_string_append(str, ", ");
        }
        g_string_append_printf(str, "%c@%" GW_TIME_FORMAT, bit_to_char(h->v.h_val), h->time);

        h = h->next;
    }

    g_assert_cmpstr(str->str, ==, expected);

    g_string_free(str, TRUE);
}

static void test_basic()
{
    GwLoader *loader = gw_ghw_loader_new();

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, "files/basic.ghw", &error);
    g_assert_no_error(error);
    g_assert_true(GW_IS_DUMP_FILE(file));
    g_object_unref(loader);

    g_assert_cmpint(gw_dump_file_get_time_scale(file), ==, 1);
    g_assert_cmpint(gw_dump_file_get_time_dimension(file), ==, GW_TIME_DIMENSION_FEMTO);
    g_assert_cmpint(gw_dump_file_get_global_time_offset(file), ==, 0);

    GwTree *tree = gw_dump_file_get_tree(file);
    GwTreeNode *root = gw_tree_get_root(tree);

    assert_tree(
        root,
        "((top standard std_logic_1164 numeric_std (ghw_test sig_bit (sig_bit_vector [1] [0]) "
        "sig_std_logic (sig_std_logic_vector [7] [6] [5] [4] [3] [2] [1] [0]) sig_integer)))");

    GwTreeNode *sig_bit = get_tree_node(tree, "top.ghw_test.sig_bit");
    GwTreeNode *sig_bit_vector = get_tree_node(tree, "top.ghw_test.sig_bit_vector");
    GwTreeNode *sig_std_logic = get_tree_node(tree, "top.ghw_test.sig_std_logic");
    GwTreeNode *sig_std_logic_vector = get_tree_node(tree, "top.ghw_test.sig_std_logic_vector");
    GwTreeNode *sig_integer = get_tree_node(tree, "top.ghw_test.sig_integer");

    GwFacs *facs = gw_dump_file_get_facs(file);
    g_assert_cmpint(gw_facs_get_length(facs), ==, 1 + 2 + 1 + 8 + 1);

    GwSymbol *sym_bit = gw_facs_get(facs, sig_bit->t_which);
    g_assert_cmpstr(sym_bit->name, ==, "top.ghw_test.sig_bit");
    assert_scalar_transitions(sym_bit->n,
                              "0@-2, 0@-1, 1@1000000, 0@2000000, 1@3000000, 0@4000000, 1@5000000, "
                              "0@6000000, 1@7000000, 0@8000000, 1@9000000, 0@10000000");

    GwSymbol *sym_std_logic = gw_facs_get(facs, sig_std_logic->t_which);
    g_assert_cmpstr(sym_std_logic->name, ==, "top.ghw_test.sig_std_logic");
    assert_scalar_transitions(
        sym_std_logic->n,
        "0@-2, 0@-1, u@0, x@1000000, 0@2000000, 1@3000000, z@4000000, w@5000000, "
        "l@6000000, h@7000000, -@8000000, u@9000000, x@10000000");

    GwSymbol *sym_integer = gw_facs_get(facs, sig_integer->t_which);
    g_assert_cmpstr(sym_integer->name, ==, "top.ghw_test.sig_integer");

    // TODO: check remaining signals

    g_assert_cmpint(sig_bit_vector->t_which, ==, -1);
    g_assert_cmpint(sig_std_logic_vector->t_which, ==, -1);

    g_object_unref(file);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/ghw_loader/basic", test_basic);

    return g_test_run();
}
