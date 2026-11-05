/*
 * Utilities and non-trivial high level functions
 * 
 * Utilities that are provided for convenience but can be reused in
 * multiple parts of GTKWave.
 */

#ifndef GW_UTILS_H
#define GW_UTILS_H

#include "gw-time.h"
#include "gw-types.h"


GwTime get_current_time(void);
char *get_symbol_value_at_time(const GwSymbol *sym, GwTime time);
char *get_trace_value_at_time(GwTrace *trace, GwTime time);

#endif
