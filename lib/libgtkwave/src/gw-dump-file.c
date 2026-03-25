#include "gw-dump-file.h"
#include "gw-enums.h"
#include "gw-string-table.h"

// clang-format off
G_DEFINE_QUARK(gw-dump-file-error-quark, gw_dump_file_error)
// clang-format on

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
    gboolean has_escaped_names;
    gboolean uses_vhdl_component_format;
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
    PROP_HAS_ESCAPED_NAMES,
    PROP_USES_VHDL_COMPONENT_FORMAT,
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

        case PROP_HAS_ESCAPED_NAMES:
            priv->has_escaped_names = g_value_get_boolean(value);
            break;

        case PROP_USES_VHDL_COMPONENT_FORMAT:
            priv->uses_vhdl_component_format = g_value_get_boolean(value);
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

        case PROP_HAS_ESCAPED_NAMES:
            g_value_set_boolean(value, gw_dump_file_has_escaped_names(self));
            break;

        case PROP_USES_VHDL_COMPONENT_FORMAT:
            g_value_set_boolean(value, gw_dump_file_get_uses_vhdl_component_format(self));
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

    properties[PROP_HAS_ESCAPED_NAMES] =
        g_param_spec_boolean("has-escaped-names",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_USES_VHDL_COMPONENT_FORMAT] =
        g_param_spec_boolean("uses-vhdl-component-format",
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
 * gw_dump_file_import_traces:
 * @self: A #GwDumpFile.
 * @nodes: (array zero-terminated=1): The nodes to import.
 * @error: A location for a #GError, or %NULL.
 *
 * Returns: %TRUE on success
 */
gboolean gw_dump_file_import_traces(GwDumpFile *self, GwNode **nodes, GError **error)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), FALSE);
    g_return_val_if_fail(nodes != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (GW_DUMP_FILE_GET_CLASS(self)->import_traces == NULL) {
        return TRUE;
    }

    return GW_DUMP_FILE_GET_CLASS(self)->import_traces(self, nodes, error);
}

/**
 * gw_dump_file_import_all:
 * @self: A #GwDumpFile.
 * @error: A location for a #GError, or %NULL.
 *
 * Returns: %TRUE on success
 */
gboolean gw_dump_file_import_all(GwDumpFile *self, GError **error)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (GW_DUMP_FILE_GET_CLASS(self)->import_traces == NULL) {
        return TRUE;
    }

    GwFacs *facs = gw_dump_file_get_facs(self);
    if (gw_facs_get_length(facs) == 0) {
        return TRUE;
    }

    GPtrArray *nodes = g_ptr_array_new();
    for (guint i = 0; i < gw_facs_get_length(facs); i++) {
        GwSymbol *s = gw_facs_get(facs, i);
        g_ptr_array_add(nodes, s->n);
    }
    g_ptr_array_add(nodes, NULL);

    gboolean ret =
        GW_DUMP_FILE_GET_CLASS(self)->import_traces(self, (GwNode **)nodes->pdata, error);

    g_ptr_array_free(nodes, TRUE);

    return ret;
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
 * gw_dump_file_get_enum_filters_for_node:
 * @self: A #GwDumpFile.
 * @node: The #GwNode.
 *
 * Returns the enum filter that is associated with the given node.
 *
 * Returns: The index of the enum filter or %0 if no enum filter is set.
 */
// TODO: Return a pointer to the enum filter instead of an index.
guint gw_dump_file_get_enum_filter_for_node(GwDumpFile *self, GwNode *node)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);
    g_return_val_if_fail(node != NULL, 0);

    if (GW_DUMP_FILE_GET_CLASS(self)->get_enum_filter_for_node == NULL) {
        return 0;
    }

    return GW_DUMP_FILE_GET_CLASS(self)->get_enum_filter_for_node(self, node);
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
 * gw_dump_file_set_time_range:
 * @self: A #GwDumpFile.
 * @time_range: The new time range.
 *
 * Sets the time range for the dump file.
 */
void gw_dump_file_set_time_range(GwDumpFile *self, GwTimeRange *time_range)
{
    g_return_if_fail(GW_IS_DUMP_FILE(self));

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    if (priv->time_range != time_range) {
        g_clear_object(&priv->time_range);
        priv->time_range = time_range ? g_object_ref(time_range) : NULL;
        
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TIME_RANGE]);
    }
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

/**
 * gw_dump_file_has_escaped_names:
 * @self: A #GwDumpFile.
 *
 * Returns whether the dump file has escaped names.
 *
 * Returns: %TRUE, if the dump file has escaped names.
 */
gboolean gw_dump_file_has_escaped_names(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->has_escaped_names;
}

/**
 * gw_dump_file_get_uses_vhdl_component_format:
 * @self: A #GwDumpFile.
 *
 * Returns whether the dump file uses the VDHL component format.
 *
 * Returns: %TRUE, if the dump file uses the VHDL component format.
 */
gboolean gw_dump_file_get_uses_vhdl_component_format(GwDumpFile *self)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), 0);

    GwDumpFilePrivate *priv = gw_dump_file_get_instance_private(self);

    return priv->uses_vhdl_component_format;
}

/*
 * find a slot already in the table...
 */
static GwSymbol *symfind_2(GwDumpFile *self, const char *s)
{
    GwFacs *facs = gw_dump_file_get_facs(self);

    GwSymbol *sr = gw_facs_lookup(facs, s);
    if (sr != NULL) {
        return sr;
    }

    // this is because . is > in ascii than chars like $ but . was converted to 0x1 on sort

    // char *s2;
    // int i;
    // int mat;

    if (!gw_dump_file_has_escaped_names(self)) {
        return NULL;
    }

    // TODO: fix support for escaped names
    g_warning("TODO: implement support for escaped names in gw_dump_file_lookup_symbol");

    return NULL;

    // guint numfacs = gw_facs_get_length(facs);

    // if (GLOBALS->facs_have_symbols_state_machine == 0) {
    //     if (has_escaped_names) {
    //         mat = 1;
    //     } else {
    //         mat = 0;

    //         for (i = 0; i < numfacs; i++) {
    //             char *hfacname = NULL;

    //             GwSymbol *fac = gw_facs_get(facs, i);

    //             hfacname = fac->name;
    //             s2 = hfacname;
    //             while (*s2) {
    //                 if (*s2 < GLOBALS->hier_delimeter) {
    //                     mat = 1;
    //                     break;
    //                 }
    //                 s2++;
    //             }

    //             /* if(was_packed) { free_2(hfacname); } ...not needed with
    //              * HIER_DEPACK_STATIC */
    //             if (mat) {
    //                 break;
    //             }
    //         }
    //     }

    //     if (mat) {
    //         GLOBALS->facs_have_symbols_state_machine = 1;
    //     } else {
    //         GLOBALS->facs_have_symbols_state_machine = 2;
    //     } /* prevent code below from executing */
    // }

    // if (GLOBALS->facs_have_symbols_state_machine == 1) {
    //     mat = 0;
    //     for (i = 0; i < numfacs; i++) {
    //         char *hfacname = NULL;

    //         GwSymbol *fac = gw_facs_get(facs, i);

    //         hfacname = fac->name;
    //         if (!strcmp(hfacname, s)) {
    //             mat = 1;
    //         }

    //         /* if(was_packed) { free_2(hfacname); } ...not needed with HIER_DEPACK_STATIC */
    //         if (mat) {
    //             sr = fac;
    //             break;
    //         }
    //     }
    // }

    // return (sr);
}

/**
 * gw_dump_file_lookup_symbol:
 * @self: A #GwDumpFile.
 * @name: The symbol name to look up.
 *
 * Looks up the symbol with the given name.
 *
 * Returns: (nullable) (transfer none): The symbol, or %NULL if no symbol with that name exists.
 */
GwSymbol *gw_dump_file_lookup_symbol(GwDumpFile *self, const gchar *name)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);
    g_return_val_if_fail(name != NULL && name[0] != '\0', NULL);

    GwSymbol *s_pnt = symfind_2(self, name);
    if (s_pnt != NULL) {
        return s_pnt;
    }

    int len = strlen(name);
    char last_char = name[len - 1];
    if (last_char == ']') {
        return NULL;
    }

    char *name2 = g_alloca(len + 4);
    memcpy(name2, name, len);
    strcpy(name2 + len, "[0]"); /* bluespec vs modelsim */

    return symfind_2(self, name2);
}

/**
 * gw_dump_file_find_symbols:
 * @self: A #GwDumpFile.
 * @pattern: The regular expression to search.
 * @error: Return location for a #GErro or %NULL.
 *
 * Searches for all symbols that match the given regular expression.
 *
 * Returns: (transfer container) (element-type GwSymbol): A #GPtrArray of #GwSymbol or %NULL in case
 *          of an error.
 */
GPtrArray *gw_dump_file_find_symbols(GwDumpFile *self, const gchar *pattern, GError **error)
{
    g_return_val_if_fail(GW_IS_DUMP_FILE(self), NULL);
    g_return_val_if_fail(pattern != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    GRegex *regex = g_regex_new(pattern, G_REGEX_CASELESS, 0, error);
    if (regex == NULL) {
        return NULL;
    }

    GPtrArray *symbols = g_ptr_array_new();

    GwFacs *facs = gw_dump_file_get_facs(self);
    guint facs_count = gw_facs_get_length(facs);

    for (guint i = 0; i < facs_count; i++) {
        GwSymbol *fac = gw_facs_get(facs, i);

        if (g_regex_match(regex, fac->name, 0, NULL)) {
            g_ptr_array_add(symbols, fac);
        }
    }

    g_regex_unref(regex);

    return symbols;
}
