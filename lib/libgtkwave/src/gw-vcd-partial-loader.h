#pragma once

#include <glib-object.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_TYPE_VCD_PARTIAL_LOADER (gw_vcd_partial_loader_get_type())
G_DECLARE_FINAL_TYPE(GwVcdPartialLoader, gw_vcd_partial_loader, GW, VCD_PARTIAL_LOADER, GwLoader)

/**
 * gw_vcd_partial_loader_new:
 *
 * Creates a new, stateful VCD partial loader.
 *
 * Returns: (transfer full): A new #GwVcdPartialLoader instance.
 */
GwVcdPartialLoader *gw_vcd_partial_loader_new(void);

/**
 * gw_vcd_partial_loader_feed:
 * @self: A #GwVcdPartialLoader.
 * @data: (array length=len): A chunk of VCD data.
 * @len: The length of the data chunk.
 * @error: The return location for a #GError or %NULL.
 *
 * Feeds a chunk of VCD data to the loader. The loader's internal
 * state and its associated dump file view are updated immediately.
 *
 * Returns: %TRUE on success, %FALSE on error.
 */
gboolean gw_vcd_partial_loader_feed(GwVcdPartialLoader *self, const gchar *data, gsize len, GError **error);

/**
 * gw_vcd_partial_loader_get_dump_file:
 * @self: A #GwVcdPartialLoader.
 *
 * Gets the dump file representing the current state of the parsed stream.
 * The returned dump file is owned by the loader and MUST NOT be unreferenced.
 * Its contents will update after subsequent calls to `gw_vcd_partial_loader_feed()`.
 *
 * Returns: (transfer none): The live #GwDumpFile, or %NULL if no data has been processed.
 */
GwDumpFile *gw_vcd_partial_loader_get_dump_file(GwVcdPartialLoader *self);



G_END_DECLS