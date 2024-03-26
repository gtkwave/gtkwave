#pragma once

#include <glib-object.h>
#include "gw-hist-ent.h"

G_BEGIN_DECLS

#define GW_TYPE_HIST_ENT_FACTORY (gw_hist_ent_factory_get_type())
G_DECLARE_FINAL_TYPE(GwHistEntFactory, gw_hist_ent_factory, GW, HIST_ENT_FACTORY, GObject)

GwHistEntFactory *gw_hist_ent_factory_new(void);

GwHistEnt *gw_hist_ent_factory_alloc(GwHistEntFactory *self);

G_END_DECLS
