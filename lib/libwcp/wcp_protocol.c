/*
 * WCP Protocol Implementation for GTKWave
 * 
 * Handles JSON parsing of incoming commands and generation of responses.
 */

#include "wcp_protocol.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

/* Supported commands for greeting */
static const char *supported_commands[] = {
    "get_item_list",
    "get_item_info",
    "set_item_color",
    "add_variables",
    "add_scope",
    "add_items",
    "add_markers",
    "remove_items",
    "focus_item",
    "clear",
    "set_viewport_to",
    "set_viewport_range",
    "zoom_to_fit",
    "load",
    "reload",
    "shutdown",
    NULL
};

static char* wcp_json_builder_to_string(JsonBuilder *builder)
{
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    char *json_str = json_generator_to_data(gen, NULL);

    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);

    return json_str;
}

static WcpCommandType parse_command_type(const char *cmd_str)
{
    g_return_val_if_fail(cmd_str != NULL, WCP_CMD_UNKNOWN);
    
    /* clang-format off */
    if (g_str_equal(cmd_str, "get_item_list"))      return WCP_CMD_GET_ITEM_LIST;
    if (g_str_equal(cmd_str, "get_item_info"))      return WCP_CMD_GET_ITEM_INFO;
    if (g_str_equal(cmd_str, "set_item_color"))     return WCP_CMD_SET_ITEM_COLOR;
    if (g_str_equal(cmd_str, "add_variables"))      return WCP_CMD_ADD_VARIABLES;
    if (g_str_equal(cmd_str, "add_scope"))          return WCP_CMD_ADD_SCOPE;
    if (g_str_equal(cmd_str, "add_items"))          return WCP_CMD_ADD_ITEMS;
    if (g_str_equal(cmd_str, "add_markers"))        return WCP_CMD_ADD_MARKERS;
    if (g_str_equal(cmd_str, "remove_items"))       return WCP_CMD_REMOVE_ITEMS;
    if (g_str_equal(cmd_str, "focus_item"))         return WCP_CMD_FOCUS_ITEM;
    if (g_str_equal(cmd_str, "clear"))              return WCP_CMD_CLEAR;
    if (g_str_equal(cmd_str, "set_viewport_to"))    return WCP_CMD_SET_VIEWPORT_TO;
    if (g_str_equal(cmd_str, "set_viewport_range")) return WCP_CMD_SET_VIEWPORT_RANGE;
    if (g_str_equal(cmd_str, "zoom_to_fit"))        return WCP_CMD_ZOOM_TO_FIT;
    if (g_str_equal(cmd_str, "load"))               return WCP_CMD_LOAD;
    if (g_str_equal(cmd_str, "reload"))             return WCP_CMD_RELOAD;
    if (g_str_equal(cmd_str, "shutdown"))           return WCP_CMD_SHUTDOWN;
    /* clang-format on */
    
    return WCP_CMD_UNKNOWN;
}

static bool json_object_require_array(JsonObject *obj,
                                          const char *name,
                                          JsonArray **out,
                                          GError **error)
{
    if (!json_object_has_member(obj, name)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Missing required field: %s", name);
        return FALSE;
    }

    JsonNode *node = json_object_get_member(obj, name);
    if (!JSON_NODE_HOLDS_ARRAY(node)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Field '%s' must be an array", name);
        return FALSE;
    }

    *out = json_node_get_array(node);
    return TRUE;
}

static bool json_object_require_string(JsonObject *obj,
                                           const char *name,
                                           char **out,
                                           GError **error)
{
    if (!json_object_has_member(obj, name)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Missing required field: %s", name);
        return FALSE;
    }

    JsonNode *node = json_object_get_member(obj, name);
    if (!JSON_NODE_HOLDS_VALUE(node) ||
        json_node_get_value_type(node) != G_TYPE_STRING) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Field '%s' must be a string", name);
        return FALSE;
    }

    *out = g_strdup(json_node_get_string(node));
    return TRUE;
}

static bool json_object_require_boolean(JsonObject *obj,
                                            const char *name,
                                            bool *out,
                                            GError **error)
{
    if (!json_object_has_member(obj, name)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Missing required field: %s", name);
        return FALSE;
    }

    JsonNode *node = json_object_get_member(obj, name);
    if (!JSON_NODE_HOLDS_VALUE(node) ||
        json_node_get_value_type(node) != G_TYPE_BOOLEAN) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Field '%s' must be a boolean", name);
        return FALSE;
    }

    *out = json_node_get_boolean(node);
    return TRUE;
}

static bool json_object_require_int64(JsonObject *obj,
                                          const char *name,
                                          int64_t *out,
                                          GError **error)
{
    if (!json_object_has_member(obj, name)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Missing required field: %s", name);
        return FALSE;
    }

    JsonNode *node = json_object_get_member(obj, name);
    if (!JSON_NODE_HOLDS_VALUE(node)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Field '%s' must be a number", name);
        return FALSE;
    }

    GType type = json_node_get_value_type(node);
    if (type == G_TYPE_INT64) {
        *out = json_node_get_int(node);
        return TRUE;
    }
    if (type == G_TYPE_DOUBLE) {
        *out = (int64_t)json_node_get_double(node);
        return TRUE;
    }

    g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                "Field '%s' must be a number", name);
    return FALSE;
}

static bool json_object_require_uint(JsonObject *obj,
                                         const char *name,
                                         uint32_t *out,
                                         GError **error)
{
    int64_t value = 0;
    if (!json_object_require_int64(obj, name, &value, error)) {
        return FALSE;
    }
    if (value < 0 || value > UINT_MAX) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Field '%s' must be a non-negative integer", name);
        return FALSE;
    }
    *out = (uint32_t)value;
    return TRUE;
}

static GArray* parse_id_array(JsonArray *arr, GError **error)
{
    GArray *ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    size_t len = (size_t)json_array_get_length(arr);
    
    for (size_t i = 0; i < len; i++) {
        JsonNode *node = json_array_get_element(arr, i);
        if (!JSON_NODE_HOLDS_VALUE(node)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                        "ids[%u] must be a number", i);
            g_array_free(ids, TRUE);
            return NULL;
        }

        GType type = json_node_get_value_type(node);
        int64_t value = 0;
        if (type == G_TYPE_INT64) {
            value = json_node_get_int(node);
        } else if (type == G_TYPE_DOUBLE) {
            value = (int64_t)json_node_get_double(node);
        } else {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                        "ids[%u] must be a number", i);
            g_array_free(ids, TRUE);
            return NULL;
        }

        if (value < 0) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                        "ids[%u] must be non-negative", i);
            g_array_free(ids, TRUE);
            return NULL;
        }

        WcpDisplayedItemRef ref;
        ref.id = (uint64_t)value;
        g_array_append_val(ids, ref);
    }
    
    return ids;
}

static GPtrArray* parse_string_array(JsonArray *arr, GError **error, const char *label)
{
    GPtrArray *strings = g_ptr_array_new_with_free_func(g_free);
    size_t len = (size_t)json_array_get_length(arr);
    
    for (size_t i = 0; i < len; i++) {
        JsonNode *node = json_array_get_element(arr, i);
        if (!JSON_NODE_HOLDS_VALUE(node) ||
            json_node_get_value_type(node) != G_TYPE_STRING) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                        "%s[%u] must be a string", label ? label : "items", i);
            g_ptr_array_free(strings, TRUE);
            return NULL;
        }
        g_ptr_array_add(strings, g_strdup(json_node_get_string(node)));
    }
    
    return strings;
}

static GArray* parse_marker_array(JsonArray *arr, GError **error)
{
    GArray *markers = g_array_new(FALSE, TRUE, sizeof(WcpMarkerInfo));
    size_t len = (size_t)json_array_get_length(arr);
    
    for (size_t i = 0; i < len; i++) {
        JsonNode *node = json_array_get_element(arr, i);
        if (!JSON_NODE_HOLDS_OBJECT(node)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                        "markers[%u] must be an object", i);
            g_array_free(markers, TRUE);
            return NULL;
        }
        JsonObject *marker_obj = json_node_get_object(node);
        WcpMarkerInfo marker = {0};
        
        if (!json_object_require_int64(marker_obj, "time", &marker.time, error) ||
            !json_object_require_boolean(marker_obj, "move_focus", &marker.move_focus, error)) {
            g_array_free(markers, TRUE);
            return NULL;
        }
        
        if (json_object_has_member(marker_obj, "name")) {
            JsonNode *name_node = json_object_get_member(marker_obj, "name");
            if (!JSON_NODE_HOLDS_VALUE(name_node) ||
                json_node_get_value_type(name_node) != G_TYPE_STRING) {
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                            "Field 'name' must be a string");
                g_array_free(markers, TRUE);
                return NULL;
            }
            marker.name = g_strdup(json_node_get_string(name_node));
        }
        
        g_array_append_val(markers, marker);
    }
    
    return markers;
}

WcpCommand* wcp_parse_command(const char *json_str, GError **error)
{
    JsonParser *parser = json_parser_new();
    
    if (!json_parser_load_from_data(parser, json_str, -1, error)) {
        g_object_unref(parser);
        return NULL;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "WCP message must be a JSON object");
        g_object_unref(parser);
        return NULL;
    }
    
    JsonObject *obj = json_node_get_object(root);
    
    /* Check message type */
    const char *msg_type = json_object_get_string_member(obj, "type");
    if (!msg_type || !g_str_equal(msg_type, "command")) {
        /* Could be a greeting - handle separately */
        if (msg_type && g_str_equal(msg_type, "greeting")) {
            /* Client greeting - we just acknowledge it */
            WcpCommand *cmd = g_new0(WcpCommand, 1);
            cmd->type = WCP_CMD_UNKNOWN;  /* Special case for greeting */
            g_object_unref(parser);
            return cmd;
        }
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Unknown message type: %s", msg_type ? msg_type : "(null)");
        g_object_unref(parser);
        return NULL;
    }
    
    /* Get command name */
    const char *cmd_name = json_object_get_string_member(obj, "command");
    WcpCommandType cmd_type = parse_command_type(cmd_name);
    
    if (cmd_type == WCP_CMD_UNKNOWN) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Unknown command: %s", cmd_name ? cmd_name : "(null)");
        g_object_unref(parser);
        return NULL;
    }
    
    WcpCommand *cmd = g_new0(WcpCommand, 1);
    cmd->type = cmd_type;
    
    /* Parse command-specific fields */
    switch (cmd_type) {
        case WCP_CMD_GET_ITEM_INFO:
        case WCP_CMD_REMOVE_ITEMS:
        {
            JsonArray *arr = NULL;
            if (!json_object_require_array(obj, "ids", &arr, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            cmd->data.item_refs.ids = parse_id_array(arr, error);
            if (!cmd->data.item_refs.ids) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;
        }
        case WCP_CMD_SET_ITEM_COLOR:
        {
            int64_t id_value = 0;
            if (!json_object_require_int64(obj, "id", &id_value, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            if (id_value < 0) {
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                            "Field 'id' must be non-negative");
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            cmd->data.set_color.id.id = (uint64_t)id_value;
            if (!json_object_require_string(obj, "color", &cmd->data.set_color.color, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;
        }
        case WCP_CMD_ADD_VARIABLES:
        {
            JsonArray *arr = NULL;
            if (!json_object_require_array(obj, "variables", &arr, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            cmd->data.add_vars.variables = parse_string_array(arr, error, "variables");
            if (!cmd->data.add_vars.variables) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;
        }
        case WCP_CMD_ADD_SCOPE:
        {
            if (!json_object_require_string(obj, "scope", &cmd->data.add_scope.scope, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            if (json_object_has_member(obj, "recursive")) {
                JsonNode *node = json_object_get_member(obj, "recursive");
                if (!JSON_NODE_HOLDS_VALUE(node) ||
                    json_node_get_value_type(node) != G_TYPE_BOOLEAN) {
                    g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                                "Field 'recursive' must be a boolean");
                    g_object_unref(parser);
                    wcp_command_free(cmd);
                    return NULL;
                }
                cmd->data.add_scope.recursive = json_node_get_boolean(node);
            }
            break;
        }
        case WCP_CMD_ADD_ITEMS:
        {
            JsonArray *arr = NULL;
            if (!json_object_require_array(obj, "items", &arr, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            cmd->data.add_items.items = parse_string_array(arr, error, "items");
            if (!cmd->data.add_items.items) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            if (json_object_has_member(obj, "recursive")) {
                JsonNode *node = json_object_get_member(obj, "recursive");
                if (!JSON_NODE_HOLDS_VALUE(node) ||
                    json_node_get_value_type(node) != G_TYPE_BOOLEAN) {
                    g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                                "Field 'recursive' must be a boolean");
                    g_object_unref(parser);
                    wcp_command_free(cmd);
                    return NULL;
                }
                cmd->data.add_items.recursive = json_node_get_boolean(node);
            }
            break;
        }
        case WCP_CMD_ADD_MARKERS:
        {
            JsonArray *arr = NULL;
            if (!json_object_require_array(obj, "markers", &arr, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            cmd->data.add_markers.markers = parse_marker_array(arr, error);
            if (!cmd->data.add_markers.markers) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;
        }
        case WCP_CMD_SET_VIEWPORT_TO:
            if (!json_object_require_int64(obj, "timestamp",
                                           &cmd->data.viewport_to.timestamp, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;

        case WCP_CMD_SET_VIEWPORT_RANGE:
            if (!json_object_require_int64(obj, "start", &cmd->data.viewport_range.start, error) ||
                !json_object_require_int64(obj, "end", &cmd->data.viewport_range.end, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;

        case WCP_CMD_FOCUS_ITEM:
        {
            int64_t id_value = 0;
            if (!json_object_require_int64(obj, "id", &id_value, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            if (id_value < 0) {
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                            "Field 'id' must be non-negative");
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            cmd->data.focus.id.id = (uint64_t)id_value;
            break;
        }
        case WCP_CMD_LOAD:
            if (!json_object_require_string(obj, "source", &cmd->data.load.source, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;

        case WCP_CMD_ZOOM_TO_FIT:
            if (!json_object_has_member(obj, "viewport_idx")) {
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                            "Missing required field: viewport_idx");
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            if (!json_object_require_uint(obj, "viewport_idx",
                                          &cmd->data.zoom.viewport_idx, error)) {
                g_object_unref(parser);
                wcp_command_free(cmd);
                return NULL;
            }
            break;
            
        default:
            /* Commands with no additional data */
            break;
    }
    
    g_object_unref(parser);
    return cmd;
}

void wcp_command_free(WcpCommand *cmd)
{
    if (!cmd) return;
    
    switch (cmd->type) {
        case WCP_CMD_GET_ITEM_INFO:
        case WCP_CMD_REMOVE_ITEMS:
            if (cmd->data.item_refs.ids) {
                g_array_free(cmd->data.item_refs.ids, TRUE);
            }
            break;
            
        case WCP_CMD_SET_ITEM_COLOR:
            g_free(cmd->data.set_color.color);
            break;
            
        case WCP_CMD_ADD_VARIABLES:
            if (cmd->data.add_vars.variables) {
                g_ptr_array_free(cmd->data.add_vars.variables, TRUE);
            }
            break;
            
        case WCP_CMD_ADD_SCOPE:
            g_free(cmd->data.add_scope.scope);
            break;
            
        case WCP_CMD_ADD_ITEMS:
            if (cmd->data.add_items.items) {
                g_ptr_array_free(cmd->data.add_items.items, TRUE);
            }
            break;
            
        case WCP_CMD_ADD_MARKERS:
            if (cmd->data.add_markers.markers) {
                for (size_t i = 0; i < (size_t)cmd->data.add_markers.markers->len; i++) {
                    WcpMarkerInfo *m = &g_array_index(cmd->data.add_markers.markers, 
                                                       WcpMarkerInfo, i);
                    g_free(m->name);
                }
                g_array_free(cmd->data.add_markers.markers, TRUE);
            }
            break;
            
        case WCP_CMD_LOAD:
            g_free(cmd->data.load.source);
            break;
            
        default:
            break;
    }
    
    g_free(cmd);
}

char* wcp_create_greeting(void)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "greeting");
    
    json_builder_set_member_name(builder, "version");
    json_builder_add_string_value(builder, WCP_VERSION);
    
    json_builder_set_member_name(builder, "commands");
    json_builder_begin_array(builder);
    for (const char **cmd = supported_commands; *cmd != NULL; cmd++) {
        json_builder_add_string_value(builder, *cmd);
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    return wcp_json_builder_to_string(builder);
}

char* wcp_create_ack(void)
{
    return g_strdup("{\"type\":\"response\",\"command\":\"ack\"}");
}

char* wcp_create_error(const char *error_type, 
                        const char *message,
                        GPtrArray *arguments)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "error");
    
    json_builder_set_member_name(builder, "error");
    json_builder_add_string_value(builder, error_type);
    
    json_builder_set_member_name(builder, "message");
    json_builder_add_string_value(builder, message);
    
    json_builder_set_member_name(builder, "arguments");
    json_builder_begin_array(builder);
    if (arguments) {
        for (size_t i = 0; i < (size_t)arguments->len; i++) {
            json_builder_add_string_value(builder, g_ptr_array_index(arguments, i));
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    return wcp_json_builder_to_string(builder);
}

char* wcp_create_item_list_response(GArray *ids)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "response");
    
    json_builder_set_member_name(builder, "command");
    json_builder_add_string_value(builder, "get_item_list");
    
    json_builder_set_member_name(builder, "ids");
    json_builder_begin_array(builder);
    if (ids) {
        for (size_t i = 0; i < (size_t)ids->len; i++) {
            WcpDisplayedItemRef *ref = &g_array_index(ids, WcpDisplayedItemRef, i);
            json_builder_add_int_value(builder, (int64_t)ref->id);
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    return wcp_json_builder_to_string(builder);
}

char* wcp_create_item_info_response(GPtrArray *items)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "response");
    
    json_builder_set_member_name(builder, "command");
    json_builder_add_string_value(builder, "get_item_info");
    
    json_builder_set_member_name(builder, "results");
    json_builder_begin_array(builder);
    if (items) {
        for (size_t i = 0; i < (size_t)items->len; i++) {
            WcpItemInfo *info = g_ptr_array_index(items, i);
            json_builder_begin_object(builder);
            
            json_builder_set_member_name(builder, "name");
            json_builder_add_string_value(builder, info->name);
            
            json_builder_set_member_name(builder, "type");
            json_builder_add_string_value(builder, info->type);
            
            json_builder_set_member_name(builder, "id");
            json_builder_add_int_value(builder, (int64_t)info->id.id);
            
            json_builder_end_object(builder);
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    return wcp_json_builder_to_string(builder);
}

char* wcp_create_add_items_response_for(const char *command, GArray *ids)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "response");
    
    json_builder_set_member_name(builder, "command");
    json_builder_add_string_value(builder, command ? command : "add_items");
    
    json_builder_set_member_name(builder, "ids");
    json_builder_begin_array(builder);
    if (ids) {
        for (size_t i = 0; i < (size_t)ids->len; i++) {
            WcpDisplayedItemRef *ref = &g_array_index(ids, WcpDisplayedItemRef, i);
            json_builder_add_int_value(builder, (int64_t)ref->id);
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    return wcp_json_builder_to_string(builder);
}

char* wcp_create_waveforms_loaded_event(const char *source)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "event");
    
    json_builder_set_member_name(builder, "event");
    json_builder_add_string_value(builder, "waveforms_loaded");
    
    json_builder_set_member_name(builder, "source");
    json_builder_add_string_value(builder, source);
    
    json_builder_end_object(builder);
    
    return wcp_json_builder_to_string(builder);
}
