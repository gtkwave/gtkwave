/*
 * WCP Server for GTKWave
 * 
 * TCP server that listens for WCP client connections and dispatches
 * commands to GTKWave.
 */

#ifndef WCP_SERVER_H
#define WCP_SERVER_H

#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <stdint.h>
#include "wcp_protocol.h"

/* Forward declaration */
typedef struct _WcpServer WcpServer;

/* Callback for command execution - implemented by GTKWave integration */
typedef char* (*WcpCommandHandler)(WcpServer *server, WcpCommand *cmd, gpointer user_data);

/* Server state */
struct _WcpServer {
    GSocketService *service;
    GSocketConnection *client_connection;
    GOutputStream *output_stream;
    GInputStream *input_stream;
    GDataInputStream *data_input;
    
    uint16_t port;
    bool running;
    bool client_connected;
    
    WcpCommandHandler handler;
    gpointer handler_data;
    
};

/* ============================================================================
 * Server Lifecycle
 * ============================================================================ */

/**
 * Create a new WCP server
 * @param port Port to listen on (0 for auto-assign)
 * @param handler Callback function to handle commands
 * @param user_data User data passed to handler
 * @return New WCP server instance, or NULL on error
 */
WcpServer* wcp_server_new(uint16_t port, 
                          WcpCommandHandler handler,
                          gpointer user_data);

/**
 * Start the WCP server (begins listening)
 * @param server The server instance
 * @param error Location for error, or NULL
 * @return TRUE on success
 */
bool wcp_server_start(WcpServer *server, GError **error);

/**
 * Stop the WCP server
 * @param server The server instance
 */
void wcp_server_stop(WcpServer *server);

/**
 * Free the WCP server
 * @param server The server instance
 */
void wcp_server_free(WcpServer *server);

/* ============================================================================
 * Message Sending (Server -> Client)
 * ============================================================================ */

/**
 * Send a message to the connected client
 * @param server The server instance
 * @param message JSON message string (will be freed)
 * @return TRUE on success
 */
bool wcp_server_send(WcpServer *server, char *message);

/**
 * Send a waveforms_loaded event to the connected client
 * @param server The server instance
 * @param source Source filename
 */
void wcp_server_emit_waveforms_loaded(WcpServer *server, const char *source);

/* ============================================================================
 * Initiating Connection (for --wcp-initiate mode)
 * ============================================================================ */

/**
 * Connect to a WCP client (instead of listening)
 * @param server The server instance
 * @param host Host to connect to
 * @param port Port to connect to
 * @param error Location for error, or NULL
 * @return TRUE on success
 */
bool wcp_server_initiate(WcpServer *server,
                             const char *host,
                             uint16_t port,
                             GError **error);

#endif /* WCP_SERVER_H */
