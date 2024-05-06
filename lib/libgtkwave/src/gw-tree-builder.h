#pragma once

#include <glib-object.h>
#include "gw-tree.h"

G_BEGIN_DECLS

#define GW_TYPE_TREE_BUILDER (gw_tree_builder_get_type())
G_DECLARE_FINAL_TYPE(GwTreeBuilder, gw_tree_builder, GW, TREE_BUILDER, GObject)

GwTreeBuilder *gw_tree_builder_new(gchar hierarchy_delimiter);
GwTreeNode *gw_tree_builder_push_scope(GwTreeBuilder *self, GwTreeKind kind, const gchar *name);
void gw_tree_builder_pop_scope(GwTreeBuilder *self);
const gchar *gw_tree_builder_get_name_prefix(GwTreeBuilder *self);
GwTreeNode *gw_tree_builder_build(GwTreeBuilder *self);
GwTreeNode *gw_tree_builder_get_current_scope(GwTreeBuilder *self);

G_END_DECLS
