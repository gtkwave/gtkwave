#pragma once

#include <glib.h>
#include "gw-var-enums.h"

G_BEGIN_DECLS

/* Helpers for interpreting Extended VCD ($dumpports) state characters and
 * drive-strength codes, per IEEE 1364-2005 Clause 18.4.3.
 *
 * GTKWave's waveform renderer is four-state (0/1/x/z), so the value actually
 * drawn still resolves through gw_evcd_char_to_vcd().  The remaining helpers
 * recover the EVCD-specific information (pin direction, the conflict/strength
 * semantics, and the textual meaning) that the four-state collapse discards,
 * so loaders and tooltips can surface it without each maintaining its own
 * private decode table. */

/* Map an EVCD state character to the four-state VCD character ('0','1','x' or
 * 'z') used by the renderer.  Unrecognized characters map to 'x'. */
gchar gw_evcd_char_to_vcd(gchar evcd_char);

/* Direction implied by a single EVCD state character (18.4.3.1):
 *   input class   D U N Z d u           -> GW_VAR_DIR_IN
 *   output class  L H X T l h           -> GW_VAR_DIR_OUT
 *   unknown dir   0 1 ? F A a B b C c f -> GW_VAR_DIR_INOUT
 * Any other character -> GW_VAR_DIR_IMPLICIT. */
GwVarDir gw_evcd_char_to_direction(gchar evcd_char);

/* Combine two observed directions while accumulating over a port's transitions.
 * GW_VAR_DIR_IMPLICIT acts as the identity; two differing concrete directions
 * resolve to GW_VAR_DIR_INOUT. */
GwVarDir gw_evcd_direction_merge(GwVarDir a, GwVarDir b);

/* Human-readable meaning of an EVCD state character (e.g. "high, two or more
 * drivers (output)"), suitable for a value tooltip.  Returns NULL for
 * characters that are not part of the EVCD alphabet. */
const gchar *gw_evcd_char_describe(gchar evcd_char);

/* Verilog drive-strength name for a 0-7 strength-component code (18.4.3):
 * highz, small, medium, weak, large, pull, strong, supply.  Returns NULL when
 * the code is outside 0-7. */
const gchar *gw_evcd_strength_name(gint code);

G_END_DECLS
