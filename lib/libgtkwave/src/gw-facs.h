#pragma once

#include <glib-object.h>
#include "gw-symbol.h"

G_BEGIN_DECLS

#define GW_TYPE_FACS (gw_facs_get_type())
G_DECLARE_FINAL_TYPE(GwFacs, gw_facs, GW, FACS, GObject)

GwFacs *gw_facs_new(guint length);

GwSymbol *gw_facs_get(GwFacs *self, guint index);
guint gw_facs_get_length(GwFacs *self);

G_END_DECLS
