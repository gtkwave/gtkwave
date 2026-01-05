/*
 * WCP (Waveform Control Protocol) for GTKWave
 * 
 * This implements the WCP protocol as defined by the Surfer project.
 * See: https://gitlab.com/surfer-project/surfer
 * 
 * WCP is a JSON-based protocol for controlling waveform viewers,
 * inspired by LSP (Language Server Protocol).
 */

#ifndef WCP_PROTOCOL_H
#define WCP_PROTOCOL_H

#include <glib.h>
#include <json-glib/json-glib.h>
#include <stdbool.h>
#include <stdint.h>

/* WCP Protocol Version */
#define WCP_VERSION "1"

/* ============================================================================
 * WCP Message Types (Client -> Server)
 * ============================================================================ */

typedef enum {
    WCP_CMD_UNKNOWN = 0,
    
    /* Query commands */
    WCP_CMD_GET_ITEM_LIST,      /* Get list of displayed items */
    WCP_CMD_GET_ITEM_INFO,      /* Get info about specific items */
    
    /* Modification commands */
    WCP_CMD_SET_ITEM_COLOR,     /* Change item color */
    WCP_CMD_ADD_VARIABLES,      /* Add signals to view */
    WCP_CMD_ADD_SCOPE,          /* Add all signals in scope */
    WCP_CMD_ADD_ITEMS,          /* Add variables or scopes */
    WCP_CMD_ADD_MARKERS,        /* Add time markers */
    WCP_CMD_REMOVE_ITEMS,       /* Remove items from view */
    WCP_CMD_FOCUS_ITEM,         /* Focus on specific item */
    WCP_CMD_CLEAR,              /* Remove all displayed items */
    
    /* Navigation commands */
    WCP_CMD_SET_VIEWPORT_TO,    /* Center view on timestamp */
    WCP_CMD_SET_VIEWPORT_RANGE, /* Set view to time range */
    WCP_CMD_ZOOM_TO_FIT,        /* Zoom to show entire waveform */
    
    /* File commands */
    WCP_CMD_LOAD,               /* Load a waveform file */
    WCP_CMD_RELOAD,             /* Reload waveform from disk */
    
    /* Control commands */
    WCP_CMD_SHUTDOWN,           /* Stop WCP server */
} WcpCommandType;

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/* Reference to a displayed item (signal, marker, etc.) */
typedef struct {
    uint64_t id;
} WcpDisplayedItemRef;

/* Information about an item */
typedef struct {
    char *name;
    char *type;  /* "signal", "marker" */
    WcpDisplayedItemRef id;
} WcpItemInfo;

/* Marker information */
typedef struct {
    int64_t time;
    char *name;       /* Optional */
    bool move_focus;
} WcpMarkerInfo;

/* Parsed WCP command */
typedef struct {
    WcpCommandType type;
    
    /* Command-specific data */
    union {
        /* get_item_info, remove_items */
        struct {
            GArray *ids;  /* Array of WcpDisplayedItemRef */
        } item_refs;
        
        /* set_item_color */
        struct {
            WcpDisplayedItemRef id;
            char *color;
        } set_color;
        
        /* add_variables */
        struct {
            GPtrArray *variables;  /* Array of char* */
        } add_vars;
        
        /* add_scope */
        struct {
            char *scope;
            bool recursive;
        } add_scope;
        
        /* add_items */
        struct {
            GPtrArray *items;  /* Array of char* */
            bool recursive;
        } add_items;
        
        /* add_markers */
        struct {
            GArray *markers;  /* Array of WcpMarkerInfo */
        } add_markers;
        
        /* set_viewport_to */
        struct {
            int64_t timestamp;
        } viewport_to;
        
        /* set_viewport_range */
        struct {
            int64_t start;
            int64_t end;
        } viewport_range;
        
        /* focus_item */
        struct {
            WcpDisplayedItemRef id;
        } focus;
        
        /* load */
        struct {
            char *source;
        } load;
        
        /* zoom_to_fit */
        struct {
            uint32_t viewport_idx;
        } zoom;
    } data;
} WcpCommand;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* Parse a JSON message into a WcpCommand structure */
WcpCommand* wcp_parse_command(const char *json_str, GError **error);

/* Free a WcpCommand structure */
void wcp_command_free(WcpCommand *cmd);

/* Create JSON response messages */
char* wcp_response_greeting(void);
char* wcp_response_ack(void);
char* wcp_response_error(const char *error_type,
                         const char *message,
                         GPtrArray *arguments);
char* wcp_response_item_info(GPtrArray *items);
char* wcp_response_id_list(const char *command, GArray *ids);

/* Create JSON event messages */
char* wcp_create_waveforms_loaded_event(const char *source);

#endif /* WCP_PROTOCOL_H */
