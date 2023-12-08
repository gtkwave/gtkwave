#include "gw-dump-file.h"

struct _GwDumpFile
{
    GObject parent_instance;

    GwBlackoutRegions *blackout_regions;
    GwStems *stems;
    GwHistEntFactory *hist_ent_factory;

    GwTime global_time_offset;
};

G_DEFINE_TYPE(GwDumpFile, gw_dump_file, G_TYPE_OBJECT)

enum
{
    PROP_BLACKOUT_REGIONS = 1,
    PROP_STEMS,
    PROP_HIST_ENT_FACTORY,
    PROP_GLOBAL_TIME_OFFSET,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_dump_file_dispose(GObject *object)
{
    GwDumpFile *self = GW_DUMP_FILE(object);

    g_clear_object(&self->blackout_regions);
    g_clear_object(&self->stems);
    g_clear_object(&self->hist_ent_factory);

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

        case PROP_STEMS: {
            GwStems *stems = g_value_get_object(value);
            if (stems == NULL) {
                stems = gw_stems_new();
            }

            g_set_object(&self->stems, stems);
            break;
        }

        case PROP_HIST_ENT_FACTORY: {
            GwHistEntFactory *hist_ent_factory = g_value_get_object(value);
            if (hist_ent_factory == NULL) {
                hist_ent_factory = gw_hist_ent_factory_new();
            }

            g_set_object(&self->hist_ent_factory, hist_ent_factory);
            break;
        }

        case PROP_GLOBAL_TIME_OFFSET:
            self->global_time_offset = g_value_get_int64(value);
            break;

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

        case PROP_STEMS:
            g_value_set_object(value, gw_dump_file_get_stems(self));
            break;

        case PROP_HIST_ENT_FACTORY:
            g_value_set_object(value, gw_dump_file_get_hist_ent_factory(self));
            break;

        case PROP_GLOBAL_TIME_OFFSET:
            g_value_set_int64(value, gw_dump_file_get_global_time_offset(self));
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

    properties[PROP_STEMS] =
        g_param_spec_object("stems",
                            NULL,
                            NULL,
                            GW_TYPE_STEMS,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_HIST_ENT_FACTORY] =
        g_param_spec_object("hist-ent-factory",
                            NULL,
                            NULL,
                            GW_TYPE_HIST_ENT_FACTORY,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_GLOBAL_TIME_OFFSET] =
        g_param_spec_int64("global-time-offset",
                           NULL,
                           NULL,
                           G_MININT64,
                           G_MAXINT64,
                           0,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_dump_file_init(GwDumpFile *self)
{
    (void)self;
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

/**
 * gw_dump_file_get_stems:
 * @self: A #GwDumpFile.
 *
 * Returns the stems.
 *
 * Returns: (transfer none): The stems.
 */
GwStems *gw_dump_file_get_stems(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    return self->stems;
}

/**
 * gw_dump_file_get_hist_ent_factory:
 * @self: A #GwDumpFile.
 *
 * Returns the hist ent factory.
 *
 * Returns: (transfer none): The hist ent factory.
 */
GwHistEntFactory *gw_dump_file_get_hist_ent_factory(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    return self->hist_ent_factory;
}

/**
 * gw_dump_file_get_global_time_offset:
 * @self: A #GwDumpFile.
 *
 * Returns the global time offset.
 *
 * Returns: The global time stems.
 */
GwTime gw_dump_file_get_global_time_offset(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);

    return self->global_time_offset;
}
