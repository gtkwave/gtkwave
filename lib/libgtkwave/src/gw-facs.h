#pragma once

#include <glib-object.h>
#include "gw-symbol.h"
#include "gw-tree.h"

G_BEGIN_DECLS

#define GW_TYPE_FACS (gw_facs_get_type())
G_DECLARE_FINAL_TYPE(GwFacs, gw_facs, GW, FACS, GObject)

GwFacs *gw_facs_new(guint length);

void gw_facs_set(GwFacs *self, guint index, GwSymbol *symbol);
GwSymbol *gw_facs_get(GwFacs *self, guint index);
guint gw_facs_get_length(GwFacs *self);
GwSymbol **gw_facs_get_array(GwFacs *self);

void gw_facs_order_from_tree(GwFacs *self, GwTree *tree);
void gw_facs_sort(GwFacs *self);

G_END_DECLS
