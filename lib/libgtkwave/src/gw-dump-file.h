#pragma once

#include <glib-object.h>
#include "gw-blackout-regions.h"
#include "gw-stems.h"
#include "gw-tree.h"
#include "gw-time.h"
#include "gw-time-range.h"
#include "gw-facs.h"
#include "gw-enum-filter-list.h"
#include "gw-string-table.h"

G_BEGIN_DECLS

GQuark gw_dump_file_error_quark(void);
#define GW_DUMP_FILE_ERROR (gw_dump_file_error_quark())

typedef enum
{
    GW_DUMP_FILE_ERROR_UNKNOWN,
    GW_DUMP_FILE_ERROR_NO_SYMBOLS,
    GW_DUMP_FILE_ERROR_NO_TRANSITIONS,
} GwDumpFileErrorEnum;

#define GW_TYPE_DUMP_FILE (gw_dump_file_get_type())
G_DECLARE_DERIVABLE_TYPE(GwDumpFile, gw_dump_file, GW, DUMP_FILE, GObject)

struct _GwDumpFileClass
{
    GObjectClass parent_class;

    gboolean (*import_traces)(GwDumpFile *self, GwNode **nodes, GError **error);
    guint (*get_enum_filter_for_node)(GwDumpFile *self, GwNode *node);
};

gboolean gw_dump_file_import_traces(GwDumpFile *self, GwNode **nodes, GError **error);
gboolean gw_dump_file_import_all(GwDumpFile *self, GError **error);

GwTree *gw_dump_file_get_tree(GwDumpFile *self);
GwFacs *gw_dump_file_get_facs(GwDumpFile *self);
GwBlackoutRegions *gw_dump_file_get_blackout_regions(GwDumpFile *self);
GwStems *gw_dump_file_get_stems(GwDumpFile *self);
GwStringTable *gw_dump_file_get_component_names(GwDumpFile *self);
GwEnumFilterList *gw_dump_file_get_enum_filters(GwDumpFile *self);
guint gw_dump_file_get_enum_filter_for_node(GwDumpFile *self, GwNode *node);

GwTimeDimension gw_dump_file_get_time_dimension(GwDumpFile *self);
GwTime gw_dump_file_get_time_scale(GwDumpFile *self);
GwTimeRange *gw_dump_file_get_time_range(GwDumpFile *self);
void gw_dump_file_set_time_range(GwDumpFile *self, GwTimeRange *time_range);
GwTime gw_dump_file_get_global_time_offset(GwDumpFile *self);

gboolean gw_dump_file_has_nonimplicit_directions(GwDumpFile *self);
gboolean gw_dump_file_has_supplemental_datatypes(GwDumpFile *self);
gboolean gw_dump_file_has_supplemental_vartypes(GwDumpFile *self);
gboolean gw_dump_file_has_escaped_names(GwDumpFile *self);
gboolean gw_dump_file_get_uses_vhdl_component_format(GwDumpFile *self);

GwSymbol *gw_dump_file_lookup_symbol(GwDumpFile *self, const gchar *name);
GPtrArray *gw_dump_file_find_symbols(GwDumpFile *self, const gchar *pattern, GError **error);

G_END_DECLS