#pragma once

#include <glib-object.h>
#include "gw-tree.h"

#define GW_HASH_PRIME 500009

GParamSpec *gw_param_spec_time(const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               GParamFlags flags);

gint gw_signal_name_compare(const gchar *name1, const gchar *name2);

void allocate_and_decorate_module_tree_node(GwTreeNode **tree_root,
                                            unsigned char ttype,
                                            const char *scopename,
                                            gint component_index,
                                            guint32 t_stem,
                                            guint32 t_istem,
                                            GwTreeNode **mod_tree_parent);