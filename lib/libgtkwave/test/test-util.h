#pragma once

#include "gw-tree.h"

void assert_tree(GwTreeNode *node, const gchar *expected);
GwTreeNode *get_tree_node(GwTree *tree, const gchar *path);