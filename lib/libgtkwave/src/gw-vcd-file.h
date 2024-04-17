#pragma once

#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_VCD_FILE (gw_vcd_file_get_type())
G_DECLARE_FINAL_TYPE(GwVcdFile, gw_vcd_file, GW, VCD_FILE, GwDumpFile)

G_END_DECLS
