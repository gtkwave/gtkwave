#pragma once

#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_FST_FILE (gw_fst_file_get_type())
G_DECLARE_FINAL_TYPE(GwFstFile, gw_fst_file, GW, FST_FILE, GwDumpFile)

gchar *gw_fst_file_get_subvar(GwFstFile *self, gint index);
void gw_fst_file_limit_time_range(GwFstFile *self, GwTimeRange *range);

G_END_DECLS
