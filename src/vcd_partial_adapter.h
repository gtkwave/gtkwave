#pragma once

#include <glib.h>
#include <gtkwave.h>

G_BEGIN_DECLS

/**
 * vcd_partial_adapter_main:
 * @shm_id: The shared memory identifier as a string
 * 
 * Initializes the interactive VCD session by creating a partial loader
 * and starting to read from the shared memory segment.
 * 
 * Returns: (transfer none): The live GwDumpFile representing the parsed VCD data,
 *          or %NULL on error.
 */
GwDumpFile *vcd_partial_adapter_main(const gchar *shm_id);

/**
 * vcd_partial_adapter_cleanup:
 * 
 * Cleans up resources associated with the interactive VCD session.
 * This should be called when the session ends or the application exits.
 */
void vcd_partial_adapter_cleanup(void);

G_END_DECLS