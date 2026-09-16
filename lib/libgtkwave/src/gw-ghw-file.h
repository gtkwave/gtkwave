#pragma once

#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_GHW_FILE (gw_ghw_file_get_type())
G_DECLARE_FINAL_TYPE(GwGhwFile, gw_ghw_file, GW, GHW_FILE, GwDumpFile)

G_END_DECLS

