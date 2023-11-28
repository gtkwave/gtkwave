#include "gw-blackout-regions.h"

typedef struct
{
    GwTime start;
    GwTime end;
} GwBlackoutRegion;

struct _GwBlackoutRegions
{
    GObject parent_instance;

    GSList *regions;

    gboolean dumpoff_valid;
    GwTime dumpoff_time;
};

G_DEFINE_TYPE(GwBlackoutRegions, gw_blackout_regions, G_TYPE_OBJECT)

static void gw_blackout_regions_finalize(GObject *object)
{
    GwBlackoutRegions *self = GW_BLACKOUT_REGIONS(object);

    g_slist_free_full(self->regions, g_free);

    G_OBJECT_CLASS(gw_blackout_regions_parent_class)->finalize(object);
}

static void gw_blackout_regions_class_init(GwBlackoutRegionsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_blackout_regions_finalize;
}

static void gw_blackout_regions_init(GwBlackoutRegions *self)
{
    (void)self;
}

GwBlackoutRegions *gw_blackout_regions_new(void)
{
    return g_object_new(GW_TYPE_BLACKOUT_REGIONS, NULL);
}

void gw_blackout_regions_add(GwBlackoutRegions *self, GwTime start, GwTime end)
{
    g_return_if_fail(GW_IS_BLACKOUT_REGIONS(self));

    GwBlackoutRegion *region = g_new0(GwBlackoutRegion, 1);
    region->start = start;
    region->end = end;

    self->regions = g_slist_prepend(self->regions, region);
}

void gw_blackout_regions_add_dumpoff(GwBlackoutRegions *self, GwTime t)
{
    g_return_if_fail(GW_IS_BLACKOUT_REGIONS(self));

    // Ignore repeated dumpoff commands.
    if (self->dumpoff_valid) {
        return;
    }

    self->dumpoff_valid = TRUE;
    self->dumpoff_time = t;
}

void gw_blackout_regions_add_dumpon(GwBlackoutRegions *self, GwTime t)
{
    g_return_if_fail(GW_IS_BLACKOUT_REGIONS(self));

    // Ignore dumpon commands without an associated dumpoff command.
    if (!self->dumpoff_valid) {
        return;
    }

    gw_blackout_regions_add(self, self->dumpoff_time, t);

    self->dumpoff_valid = FALSE;
}

gboolean gw_blackout_regions_contains(GwBlackoutRegions *self, GwTime t)
{
    g_return_val_if_fail(GW_IS_BLACKOUT_REGIONS(self), FALSE);

    for (GSList *iter = self->regions; iter != NULL; iter = iter->next) {
        GwBlackoutRegion *region = iter->data;

        if (t >= region->start && t < region->end) {
            return TRUE;
        }
    }

    return FALSE;
}

void gw_blackout_regions_scale(GwBlackoutRegions *self, GwTime scale)
{
    g_return_if_fail(GW_IS_BLACKOUT_REGIONS(self));

    for (GSList *iter = self->regions; iter != NULL; iter = iter->next) {
        GwBlackoutRegion *region = iter->data;

        region->start *= scale;
        region->end *= scale;
    }
}

void gw_blackout_regions_foreach(GwBlackoutRegions *self,
                                 GwBlackoutRegionsForEach function,
                                 gpointer user_data)
{
    g_return_if_fail(GW_IS_BLACKOUT_REGIONS(self));
    g_return_if_fail(function != NULL);

    for (GSList *iter = self->regions; iter != NULL; iter = iter->next) {
        GwBlackoutRegion *region = iter->data;

        function(region->start, region->end, user_data);
    }
}

guint gw_blackout_regions_length(GwBlackoutRegions *self)
{
    g_return_val_if_fail(GW_IS_BLACKOUT_REGIONS(self), 0);

    return g_slist_length(self->regions);
}
