#include "gw-dump-file.h"

struct _GwDumpFile
{
    GObject parent_instance;

    GwBlackoutRegions *blackout_regions;
};

G_DEFINE_TYPE(GwDumpFile, gw_dump_file, G_TYPE_OBJECT)

enum
{
    PROP_BLACKOUT_REGIONS = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_dump_file_dispose(GObject *object) {
    GwDumpFile *self = GW_DUMP_FILE(object);

    g_clear_object(&self->blackout_regions);

    G_OBJECT_CLASS(gw_dump_file_parent_class)->dispose(object);
}


static void gw_dump_file_set_property(GObject *object,
                                      guint property_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
    GwDumpFile *self = GW_DUMP_FILE(object);

    switch (property_id) {
        case PROP_BLACKOUT_REGIONS: {
            GwBlackoutRegions *blackout_regions = g_value_get_object(value);
            if (blackout_regions == NULL) {
                blackout_regions = gw_blackout_regions_new();
            }

            g_set_object(&self->blackout_regions, blackout_regions);
            break;
        }

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_dump_file_get_property(GObject *object,
                                      guint property_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
    GwDumpFile *self = GW_DUMP_FILE(object);

    switch (property_id) {
        case PROP_BLACKOUT_REGIONS:
            g_value_set_object(value, gw_dump_file_get_blackout_regions(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_dump_file_class_init(GwDumpFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->dispose = gw_dump_file_dispose;
    object_class->set_property = gw_dump_file_set_property;
    object_class->get_property = gw_dump_file_get_property;

    properties[PROP_BLACKOUT_REGIONS] =
        g_param_spec_object("blackout-regions",
                            NULL,
                            NULL,
                            GW_TYPE_BLACKOUT_REGIONS,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_dump_file_init(GwDumpFile *self)
{
}

/**
 * gw_dump_file_get_blackout_regions:
 * @self: A #GwDumpFile.
 *
 * Returns the blackout regions.
 *
 * Returns: (transfer none): The blackout regions.
 */
GwBlackoutRegions *gw_dump_file_get_blackout_regions(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    return self->blackout_regions;
}