/*
 * WCP Server Implementation for GTKWave
 * 
 * Uses GLib's GSocketService for async TCP handling, integrated
 * with GTK's main loop.
 */

#include "wcp_server.h"
#include <string.h>

#ifndef WCP_DEFAULT_PORT
#define WCP_DEFAULT_PORT 8765
#endif

static void handle_line(WcpServer *server, const char *line)
{
    GError *error = NULL;
    
    /* Skip empty lines */
    if (!line || strlen(line) == 0) {
        return;
    }
    
    /* Parse the command */
    WcpCommand *cmd = wcp_parse_command(line, &error);
    if (!cmd) {
        g_warning("WCP: Failed to parse command: %s", error->message);
        char *err_response = wcp_response_error("parse_error", error->message, NULL);
        wcp_server_send(server, err_response);
        g_error_free(error);
        return;
    }
    
    /* Handle greeting specially */
    if (cmd->type == WCP_CMD_UNKNOWN) {
        /* This was a client greeting, send our greeting back */
        char *greeting = wcp_response_greeting();
        wcp_server_send(server, greeting);
        wcp_command_free(cmd);
        return;
    }
    
    /* Dispatch to handler */
    if (server->handler) {
        char *response = server->handler(server, cmd, server->handler_data);
        if (response) {
            wcp_server_send(server, response);
        }
    } else {
        /* No handler - just ack */
        wcp_server_send(server, wcp_response_ack());
    }
    
    wcp_command_free(cmd);
}

static void read_line_async_callback(GObject *source, GAsyncResult *result, gpointer user_data)
{
    WcpServer *server = (WcpServer *)user_data;
    GError *error = NULL;
    
    char *line = g_data_input_stream_read_line_finish(server->data_input, result, NULL, &error);
    
    if (error) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("WCP: Read error: %s", error->message);
        }
        g_error_free(error);
        
        /* Client disconnected */
        server->client_connected = FALSE;
        g_message("WCP: Client disconnected");
        return;
    }
    
    if (!line) {
        /* EOF - client disconnected */
        server->client_connected = FALSE;
        g_message("WCP: Client disconnected (EOF)");
        return;
    }
    
    /* Process the line */
    handle_line(server, line);
    g_free(line);
    
    /* Continue reading if still connected */
    if (server->client_connected && server->running) {
        g_data_input_stream_read_line_async(server->data_input,
                                            G_PRIORITY_DEFAULT,
                                            NULL,
                                            read_line_async_callback,
                                            server);
    }
}

static void start_reading(WcpServer *server)
{
    /* Create data input stream for line-based reading */
    server->data_input = g_data_input_stream_new(server->input_stream);
    g_data_input_stream_set_newline_type(server->data_input, 
                                          G_DATA_STREAM_NEWLINE_TYPE_LF);
    
    /* Start async read */
    g_data_input_stream_read_line_async(server->data_input,
                                        G_PRIORITY_DEFAULT,
                                        NULL,
                                        read_line_async_callback,
                                        server);
}

static bool on_incoming_connection(GSocketService *service,
                                       GSocketConnection *connection,
                                       GObject *source_object,
                                       gpointer user_data)
{
    WcpServer *server = (WcpServer *)user_data;
    
    (void)service;
    (void)source_object;
    
    /* Only allow one client at a time */
    if (server->client_connected) {
        g_warning("WCP: Rejecting connection - already have a client");
        return FALSE;
    }
    
    g_message("WCP: Client connected");
    
    /* Keep reference to connection */
    server->client_connection = g_object_ref(connection);
    server->input_stream = g_io_stream_get_input_stream(G_IO_STREAM(connection));
    server->output_stream = g_io_stream_get_output_stream(G_IO_STREAM(connection));
    server->client_connected = TRUE;
    
    /* Send greeting */
    char *greeting = wcp_response_greeting();
    wcp_server_send(server, greeting);
    
    /* Start reading from client */
    start_reading(server);
    
    return TRUE;
}

WcpServer* wcp_server_new(uint16_t port, 
                          WcpCommandHandler handler,
                          gpointer user_data)
{
    WcpServer *server = g_new0(WcpServer, 1);
    
    server->port = port ? port : WCP_DEFAULT_PORT;
    server->handler = handler;
    server->handler_data = user_data;
    server->running = FALSE;
    server->client_connected = FALSE;
    return server;
}

bool wcp_server_start(WcpServer *server, GError **error)
{
    if (server->running) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED,
                    "WCP server already running");
        return FALSE;
    }
    
    server->service = g_socket_service_new();
    
    if (!g_socket_listener_add_inet_port(G_SOCKET_LISTENER(server->service),
                                         server->port,
                                         NULL,
                                         error)) {
        g_object_unref(server->service);
        server->service = NULL;
        return FALSE;
    }
    
    g_signal_connect(server->service, "incoming",
                     G_CALLBACK(on_incoming_connection), server);
    
    g_socket_service_start(server->service);
    server->running = TRUE;
    
    g_message("WCP: Server listening on port %d", server->port);
    
    return TRUE;
}

void wcp_server_stop(WcpServer *server)
{
    if (!server->running) return;
    
    server->running = FALSE;
    
    /* Close client connection if any */
    if (server->client_connection) {
        g_io_stream_close(G_IO_STREAM(server->client_connection), NULL, NULL);
        g_object_unref(server->client_connection);
        server->client_connection = NULL;
    }
    
    if (server->data_input) {
        g_object_unref(server->data_input);
        server->data_input = NULL;
    }
    
    server->client_connected = FALSE;
    server->input_stream = NULL;
    server->output_stream = NULL;
    
    /* Stop socket service */
    if (server->service) {
        g_socket_service_stop(server->service);
        g_object_unref(server->service);
        server->service = NULL;
    }
    
    g_message("WCP: Server stopped");
}

void wcp_server_free(WcpServer *server)
{
    if (!server) return;
    
    wcp_server_stop(server);
    g_free(server);
}

bool wcp_server_send(WcpServer *server, char *message)
{
    if (!server->client_connected || !server->output_stream) {
        g_free(message);
        return FALSE;
    }
    
    /* Append newline */
    char *msg_with_newline = g_strdup_printf("%s\n", message);
    g_free(message);
    
    GError *error = NULL;
    gsize bytes_written;
    
    bool success = g_output_stream_write_all(server->output_stream,
                                                  msg_with_newline,
                                                  strlen(msg_with_newline),
                                                  &bytes_written,
                                                  NULL,
                                                  &error);
    
    if (!success) {
        g_warning("WCP: Failed to send message: %s", error->message);
        g_error_free(error);
    }
    
    g_free(msg_with_newline);
    return success;
}

void wcp_server_emit_waveforms_loaded(WcpServer *server, const char *source)
{
    if (!server->client_connected) return;
    
    char *event = wcp_create_waveforms_loaded_event(source);
    wcp_server_send(server, event);
}

bool wcp_server_initiate(WcpServer *server,
                             const char *host,
                             uint16_t port,
                             GError **error)
{
    if (server->running) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_ALREADY_MOUNTED,
                    "WCP server already running");
        return FALSE;
    }
    
    GSocketClient *client = g_socket_client_new();
    
    GSocketConnection *connection = g_socket_client_connect_to_host(client,
                                                                     host,
                                                                     port,
                                                                     NULL,
                                                                     error);
    g_object_unref(client);
    
    if (!connection) {
        return FALSE;
    }
    
    g_message("WCP: Connected to %s:%d", host, port);
    
    server->client_connection = connection;
    server->input_stream = g_io_stream_get_input_stream(G_IO_STREAM(connection));
    server->output_stream = g_io_stream_get_output_stream(G_IO_STREAM(connection));
    server->client_connected = TRUE;
    server->running = TRUE;
    
    /* Send greeting */
    char *greeting = wcp_response_greeting();
    wcp_server_send(server, greeting);
    
    /* Start reading */
    start_reading(server);
    
    return TRUE;
}
