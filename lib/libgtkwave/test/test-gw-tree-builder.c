#include <gtkwave.h>
#include "test-util.h"

static void test_build(void) {
    GwTreeBuilder *builder;
    GwTreeNode *node;

    // Empty tree

    builder = gw_tree_builder_new('.');
    node = gw_tree_builder_build(builder);
    g_assert_null(node);
    g_object_unref(builder);

    // One scope at root level

    builder = gw_tree_builder_new('.');
    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "one");
    node = gw_tree_builder_build(builder);
    assert_tree(node, "one(0)");
    g_object_unref(builder);

    // Two nested scopes

    builder = gw_tree_builder_new('.');
    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "one");
    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "two");
    node = gw_tree_builder_build(builder);
    assert_tree(node, "one(0){two(0)}");
    g_object_unref(builder);

    // Two siblings at root level

    builder = gw_tree_builder_new('.');
    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "one");
    gw_tree_builder_pop_scope(builder);
    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "two");
    node = gw_tree_builder_build(builder);
    assert_tree(node, "one(0), two(0)");
    g_object_unref(builder);
}

static void test_name_prefix(void) {
    GwTreeBuilder *builder = gw_tree_builder_new('.');
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, NULL);

    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "mod");
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, "mod");

    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "submod1");
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, "mod.submod1");

    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "submod2a");
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, "mod.submod1.submod2a");

    gw_tree_builder_pop_scope(builder);
    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "submod2b");
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, "mod.submod1.submod2b");

    gw_tree_builder_pop_scope(builder);
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, "mod.submod1");

    gw_tree_builder_pop_scope(builder);
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, "mod");

    gw_tree_builder_pop_scope(builder);
    g_assert_cmpstr(gw_tree_builder_get_name_prefix(builder), ==, NULL);

    g_object_unref(builder);
}

static void test_duplicate_scope(void) {
    GwTreeBuilder *builder = gw_tree_builder_new('.');

    GwTreeNode *mod1 = gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "mod");
    gw_tree_builder_pop_scope(builder);
    GwTreeNode *mod2 = gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "mod");
    gw_tree_builder_pop_scope(builder);

    GwTreeNode *node = gw_tree_builder_build(builder);
    assert_tree(node, "mod(0)");
    g_assert_true(mod1 == mod2);

    g_object_unref(builder);
}

static void test_get_symbol_name(void) {
    GwTreeBuilder *builder = gw_tree_builder_new('.');
    gchar *name;

    name = gw_tree_builder_get_symbol_name(builder, "sym1");
    g_assert_cmpstr(name, ==, "sym1");
    g_free(name);

    gw_tree_builder_push_scope(builder, GW_TREE_KIND_VCD_ST_MODULE, "mod");
    name = gw_tree_builder_get_symbol_name(builder, "sym2");
    g_assert_cmpstr(name, ==, "mod.sym2");
    g_free(name);

    g_object_unref(builder);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/tree_builder/build", test_build);
    g_test_add_func("/tree_builder/name_prefix", test_name_prefix);
    g_test_add_func("/tree_builder/duplicate_scope", test_duplicate_scope);
    g_test_add_func("/tree_builder/get_symbol_name", test_get_symbol_name);

    return g_test_run();
}
