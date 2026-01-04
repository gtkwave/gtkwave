/*
 * GTKWave WCP Integration
 * 
 * This module bridges WCP commands to GTKWave's internal functions.
 * It translates WCP protocol messages into GTKWave API calls.
 * 
 *
 */

#ifndef WCP_GTKWAVE_H
#define WCP_GTKWAVE_H

#include "wcp_server.h"

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * Initialize WCP support for GTKWave
 * Call this from main() after GTKWave is initialized
 * 
 * @param port Port to listen on (0 for default 8765)
 * @return TRUE on success
 */
gboolean wcp_gtkwave_init(guint16 port);

/**
 * Shutdown WCP support
 * Call this before GTKWave exits
 */
void wcp_gtkwave_shutdown(void);

/* ============================================================================
 * Event Emission (call these from GTKWave code)
 * ============================================================================ */

/**
 * Notify WCP client that waveforms have been loaded
 * Call this after successful file load
 */
void wcp_gtkwave_notify_waveforms_loaded(const gchar *filename);

#endif /* WCP_GTKWAVE_H */
