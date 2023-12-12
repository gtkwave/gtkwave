#include "gw-dump-file.h"
#include "gw-enums.h"

typedef struct
{
    GwBlackoutRegions *blackout_regions;
    GwStems *stems;
    GwTree *tree;

    GwTime time_scale;
    GwTimeDimension time_dimension;
    GwTime global_time_offset;
} GwDumpFilePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GwDumpFile, gw_dump_file, G_TYPE_OBJECT)

enum
{
    PROP_BLACKOUT_REGIONS = 1,
    PROP_STEMS,
    PROP_TREE,
    PROP_TIME_SCALE,
    PROP_TIME_DIMENSION,
    PROP_GLOBAL_TIME_OFFSET,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_dump_file_dispose(GObject *object)
{
    GwDumpFile *self = GW_DUMP_FILE(object);
    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    g_clear_object(&priv->blackout_regions);
    g_clear_object(&priv->stems);

    G_OBJECT_CLASS(gw_dump_file_parent_class)->dispose(object);
}

static void gw_dump_file_set_property(GObject *object,
                                      guint property_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
    GwDumpFile *self = GW_DUMP_FILE(object);
    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    switch (property_id) {
        case PROP_BLACKOUT_REGIONS: {
            GwBlackoutRegions *blackout_regions = g_value_get_object(value);
            if (blackout_regions == NULL) {
                blackout_regions = gw_blackout_regions_new();
            }

            g_set_object(&priv->blackout_regions, blackout_regions);
            break;
        }

        case PROP_STEMS: {
            GwStems *stems = g_value_get_object(value);
            if (stems == NULL) {
                stems = gw_stems_new();
            }

            g_set_object(&priv->stems, stems);
            break;
        }

        case PROP_TREE: {
            GwTree *tree = g_value_get_object(value);
            if (tree == NULL) {
                tree = gw_tree_new(NULL);
            }

            g_set_object(&priv->tree, tree);
            break;
        }

        case PROP_TIME_SCALE:
            priv->time_scale = g_value_get_int64(value);
            break;

        case PROP_TIME_DIMENSION:
            priv->time_dimension = g_value_get_enum(value);
            break;

        case PROP_GLOBAL_TIME_OFFSET:
            priv->global_time_offset = g_value_get_int64(value);
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

        case PROP_TREE:
            g_value_set_object(value, gw_dump_file_get_tree(self));
            break;

        case PROP_TIME_SCALE:
            g_value_set_int64(value, gw_dump_file_get_time_scale(self));
            break;

        case PROP_TIME_DIMENSION:
            g_value_set_enum(value, gw_dump_file_get_time_dimension(self));
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

    properties[PROP_TREE] =
        g_param_spec_object("tree",
                            NULL,
                            NULL,
                            GW_TYPE_TREE,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_TIME_SCALE] =
        g_param_spec_int64("time-scale",
                           NULL,
                           NULL,
                           1,
                           100,
                           1,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_TIME_DIMENSION] =
        g_param_spec_enum("time-dimension",
                          NULL,
                          NULL,
                          GW_TYPE_TIME_DIMENSION,
                          GW_TIME_DIMENSION_NONE,
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

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->blackout_regions;
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

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->stems;
}
/**
 * gw_dump_file_get_tree:
 * @self: A #GwDumpFile.
 *
 * Returns the symbols tree.
 *
 * Returns: (transfer none): The tree.
 */
GwTree *gw_dump_file_get_tree(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->tree;
}

/**
 * gw_dump_file_get_time_scale:
 * @self: A #GwDumpFile.
 *
 * Returns the time scale.
 *
 * Returns: The time scale.
 */
GwTime gw_dump_file_get_time_scale(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), GW_TIME_DIMENSION_NONE);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->time_scale;
}

/**
 * gw_dump_file_get_time_dimension:
 * @self: A #GwDumpFile.
 *
 * Returns the time dimension.
 *
 * Returns: The time dimension.
 */
GwTimeDimension gw_dump_file_get_time_dimension(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), GW_TIME_DIMENSION_NONE);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->time_dimension;
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

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->global_time_offset;
}
