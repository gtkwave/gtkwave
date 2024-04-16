#pragma once

#include "gw-tree.h"
#include "gw-dump-file.h"

void assert_tree(GwTreeNode *node, const gchar *expected);
GwTreeNode *get_tree_node(GwTree *tree, const gchar *path);
void common_basic_vcd_and_fst_test(GwDumpFile *file);