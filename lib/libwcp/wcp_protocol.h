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
    guint64 id;
} WcpDisplayedItemRef;

/* Information about an item */
typedef struct {
    gchar *name;
    gchar *type;  /* "signal", "marker" */
    WcpDisplayedItemRef id;
} WcpItemInfo;

/* Marker information */
typedef struct {
    gint64 time;
    gchar *name;       /* Optional */
    gboolean move_focus;
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
            gchar *color;
        } set_color;
        
        /* add_variables */
        struct {
            GPtrArray *variables;  /* Array of gchar* */
        } add_vars;
        
        /* add_scope */
        struct {
            gchar *scope;
            gboolean recursive;
        } add_scope;
        
        /* add_items */
        struct {
            GPtrArray *items;  /* Array of gchar* */
            gboolean recursive;
        } add_items;
        
        /* add_markers */
        struct {
            GArray *markers;  /* Array of WcpMarkerInfo */
        } add_markers;
        
        /* set_viewport_to */
        struct {
            gint64 timestamp;
        } viewport_to;
        
        /* set_viewport_range */
        struct {
            gint64 start;
            gint64 end;
        } viewport_range;
        
        /* focus_item */
        struct {
            WcpDisplayedItemRef id;
        } focus;
        
        /* load */
        struct {
            gchar *source;
        } load;
        
        /* zoom_to_fit */
        struct {
            guint viewport_idx;
        } zoom;
    } data;
} WcpCommand;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* Parse a JSON message into a WcpCommand structure */
WcpCommand* wcp_parse_command(const gchar *json_str, GError **error);

/* Free a WcpCommand structure */
void wcp_command_free(WcpCommand *cmd);

/* Create JSON response messages */
gchar* wcp_create_greeting(void);
gchar* wcp_create_ack(void);
gchar* wcp_create_error(const gchar *error_type, 
                        const gchar *message,
                        GPtrArray *arguments);
gchar* wcp_create_item_list_response(GArray *ids);
gchar* wcp_create_item_info_response(GPtrArray *items);
gchar* wcp_create_add_items_response_for(const gchar *command, GArray *ids);

/* Create JSON event messages */
gchar* wcp_create_waveforms_loaded_event(const gchar *source);

#endif /* WCP_PROTOCOL_H */
