#pragma once

#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_VCD_FILE (gw_vcd_file_get_type())
G_DECLARE_FINAL_TYPE(GwVcdFile, gw_vcd_file, GW, VCD_FILE, GwDumpFile)

void gw_vcd_file_import_masked(GwVcdFile *self);
void gw_vcd_file_set_fac_process_mask(GwVcdFile *self, GwNode *np);
void gw_vcd_file_import_trace(GwVcdFile *self, GwNode *np);

G_END_DECLS
