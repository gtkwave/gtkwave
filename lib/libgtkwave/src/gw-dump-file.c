#include "gw-dump-file.h"
#include "gw-enums.h"
#include "gw-string-table.h"

typedef struct
{
    GwTree *tree;
    GwFacs *facs;
    GwBlackoutRegions *blackout_regions;
    GwStems *stems;
    GwStringTable *component_names;
    GwEnumFilterList *enum_filters;

    GwTime time_scale;
    GwTimeDimension time_dimension;
    GwTimeRange *time_range;
    GwTime global_time_offset;

    gboolean has_nonimplicit_directions;
    gboolean has_supplemental_datatypes;
    gboolean has_supplemental_vartypes;
} GwDumpFilePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GwDumpFile, gw_dump_file, G_TYPE_OBJECT)

enum
{
    PROP_TREE = 1,
    PROP_FACS,
    PROP_BLACKOUT_REGIONS,
    PROP_STEMS,
    PROP_COMPONENT_NAMES,
    PROP_ENUM_FILTERS,
    PROP_TIME_SCALE,
    PROP_TIME_DIMENSION,
    PROP_TIME_RANGE,
    PROP_GLOBAL_TIME_OFFSET,
    PROP_HAS_NONIMPLICIT_DIRECTIONS,
    PROP_HAS_SUPPLEMENTAL_DATATYPES,
    PROP_HAS_SUPPLEMENTAL_VARTYPES,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_dump_file_dispose(GObject *object)
{
    GwDumpFile *self = GW_DUMP_FILE(object);
    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    g_clear_object(&priv->blackout_regions);
    g_clear_object(&priv->stems);
    g_clear_object(&priv->component_names);
    g_clear_object(&priv->enum_filters);
    g_clear_object(&priv->time_range);

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
        case PROP_TREE: {
            GwTree *tree = g_value_get_object(value);
            if (tree == NULL) {
                tree = gw_tree_new(NULL);
            }

            g_set_object(&priv->tree, tree);
            break;
        }

        case PROP_FACS: {
            GwFacs *facs = g_value_get_object(value);
            if (facs == NULL) {
                facs = gw_facs_new(0);
            }

            g_set_object(&priv->facs, facs);
            break;
        }

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

        case PROP_COMPONENT_NAMES: {
            GwStringTable *component_names = g_value_get_object(value);
            if (component_names == NULL) {
                component_names = gw_string_table_new();
            }

            g_set_object(&priv->component_names, component_names);
            break;
        }

        case PROP_ENUM_FILTERS: {
            GwEnumFilterList *enum_filters = g_value_get_object(value);
            if (enum_filters == NULL) {
                enum_filters = gw_enum_filter_list_new();
            }

            g_set_object(&priv->enum_filters, enum_filters);
            break;
        }

        case PROP_TIME_SCALE:
            priv->time_scale = g_value_get_int64(value);
            break;

        case PROP_TIME_DIMENSION:
            priv->time_dimension = g_value_get_enum(value);
            break;

        case PROP_TIME_RANGE: {
            GwTimeRange *time_range = g_value_get_object(value);
            if (time_range == NULL) {
                time_range = gw_time_range_new(0, 0);
            }

            g_set_object(&priv->time_range, time_range);
            break;
        }

        case PROP_GLOBAL_TIME_OFFSET:
            priv->global_time_offset = g_value_get_int64(value);
            break;

        case PROP_HAS_NONIMPLICIT_DIRECTIONS:
            priv->has_nonimplicit_directions = g_value_get_boolean(value);
            break;

        case PROP_HAS_SUPPLEMENTAL_DATATYPES:
            priv->has_supplemental_datatypes = g_value_get_boolean(value);
            break;

        case PROP_HAS_SUPPLEMENTAL_VARTYPES:
            priv->has_supplemental_vartypes = g_value_get_boolean(value);
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
        case PROP_TREE:
            g_value_set_object(value, gw_dump_file_get_tree(self));
            break;

        case PROP_FACS:
            g_value_set_object(value, gw_dump_file_get_facs(self));
            break;

        case PROP_BLACKOUT_REGIONS:
            g_value_set_object(value, gw_dump_file_get_blackout_regions(self));
            break;

        case PROP_STEMS:
            g_value_set_object(value, gw_dump_file_get_stems(self));
            break;

        case PROP_COMPONENT_NAMES:
            g_value_set_object(value, gw_dump_file_get_component_names(self));
            break;

        case PROP_ENUM_FILTERS:
            g_value_set_object(value, gw_dump_file_get_enum_filters(self));
            break;

        case PROP_TIME_SCALE:
            g_value_set_int64(value, gw_dump_file_get_time_scale(self));
            break;

        case PROP_TIME_DIMENSION:
            g_value_set_enum(value, gw_dump_file_get_time_dimension(self));
            break;

        case PROP_TIME_RANGE:
            g_value_set_object(value, gw_dump_file_get_time_range(self));
            break;

        case PROP_GLOBAL_TIME_OFFSET:
            g_value_set_int64(value, gw_dump_file_get_global_time_offset(self));
            break;

        case PROP_HAS_NONIMPLICIT_DIRECTIONS:
            g_value_set_boolean(value, gw_dump_file_has_nonimplicit_directions(self));
            break;

        case PROP_HAS_SUPPLEMENTAL_DATATYPES:
            g_value_set_boolean(value, gw_dump_file_has_supplemental_datatypes(self));
            break;

        case PROP_HAS_SUPPLEMENTAL_VARTYPES:
            g_value_set_boolean(value, gw_dump_file_has_supplemental_vartypes(self));
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

    properties[PROP_TREE] =
        g_param_spec_object("tree",
                            NULL,
                            NULL,
                            GW_TYPE_TREE,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_FACS] =
        g_param_spec_object("facs",
                            NULL,
                            NULL,
                            GW_TYPE_FACS,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

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

    properties[PROP_COMPONENT_NAMES] =
        g_param_spec_object("component-names",
                            NULL,
                            NULL,
                            GW_TYPE_STRING_TABLE,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_ENUM_FILTERS] =
        g_param_spec_object("enum-filters",
                            NULL,
                            NULL,
                            GW_TYPE_ENUM_FILTER_LIST,
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

    properties[PROP_TIME_RANGE] =
        g_param_spec_object("time-range",
                            NULL,
                            NULL,
                            GW_TYPE_TIME_RANGE,
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_GLOBAL_TIME_OFFSET] =
        g_param_spec_int64("global-time-offset",
                           NULL,
                           NULL,
                           G_MININT64,
                           G_MAXINT64,
                           0,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_HAS_NONIMPLICIT_DIRECTIONS] =
        g_param_spec_boolean("has-nonimplicit-directions",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_HAS_SUPPLEMENTAL_DATATYPES] =
        g_param_spec_boolean("has-supplemental-datatypes",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_HAS_SUPPLEMENTAL_VARTYPES] =
        g_param_spec_boolean("has-supplemental-vartypes",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_dump_file_init(GwDumpFile *self)
{
    (void)self;
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
 * gw_dump_file_get_facs:
 * @self: A #GwDumpFile.
 *
 * Returns the facs.
 *
 * Returns: (transfer none): The facs.
 */
GwFacs *gw_dump_file_get_facs(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->facs;
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
 * gw_dump_file_get_component_names:
 * @self: A #GwDumpFile.
 *
 * Returns the component names table.
 *
 * Returns: (transfer none): The component names table.
 */
GwStringTable *gw_dump_file_get_component_names(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->component_names;
}

/**
 * gw_dump_file_get_enum_filters:
 * @self: A #GwDumpFile.
 *
 * Returns the enum filters.
 *
 * Returns: (transfer none): The enum filters.
 */
GwEnumFilterList *gw_dump_file_get_enum_filters(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->enum_filters;
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
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 1);

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
 * gw_dump_file_get_time_range:
 * @self: A #GwDumpFile.
 *
 * Returns the time range.
 *
 * Returns: (transfer none): The time range.
 */
GwTimeRange *gw_dump_file_get_time_range(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->time_range;
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

/**
 * gw_dump_file_has_nonimplicit_directions:
 * @self: A #GwDumpFile.
 *
 * Returns whether the dump file has non implicit directions.
 *
 * Returns: %TRUE, if the dump file has non implicit directions.
 */
gboolean gw_dump_file_has_nonimplicit_directions(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->has_nonimplicit_directions;
}

/**
 * gw_dump_file_has_supplemental_datatypes:
 * @self: A #GwDumpFile.
 *
 * Returns whether the dump file has supplemental datatypes.
 *
 * Returns: %TRUE, if the dump file has supplemental datatypes.
 */
gboolean gw_dump_file_has_supplemental_datatypes(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->has_supplemental_datatypes;
}

/**
 * gw_dump_file_has_supplemental_vartatypes:
 * @self: A #GwDumpFile.
 *
 * Returns whether the dump file has supplemental vartatypes.
 *
 * Returns: %TRUE, if the dump file has supplemental vartatypes.
 */
gboolean gw_dump_file_has_supplemental_vartypes(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->has_supplemental_vartypes;
}
