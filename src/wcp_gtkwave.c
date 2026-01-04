/*
 * GTKWave WCP Integration Implementation
 * 
 * Maps WCP protocol commands to GTKWave internal functions.
 */

#include "wcp_gtkwave.h"

#include <gtkwave.h>
#include <stdlib.h>
#include <string.h>
#include "analyzer.h"
#include "color.h"
#include "currenttime.h"
#include "globals.h"
#include "lx2.h"
#include "menu.h"
#include "signal_list.h"
#include "symbol.h"
#include "timeentry.h"
#include "wavewindow.h"
#include "zoombuttons.h"

/* Global WCP server instance */
WcpServer *g_wcp_server = NULL;

#define WCP_ITEM_MARKER_FLAG (1ull << 62)

static GHashTable *wcp_trace_to_id = NULL;
static GHashTable *wcp_id_to_trace = NULL;
static guint64 wcp_next_trace_id = 1;

static guint64 *wcp_id_new(guint64 id)
{
    guint64 *value = g_new(guint64, 1);
    *value = id;
    return value;
}

static void wcp_trace_map_init(void)
{
    if (wcp_trace_to_id) {
        return;
    }
    wcp_trace_to_id = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    wcp_id_to_trace = g_hash_table_new_full(g_int64_hash, g_int64_equal, g_free, NULL);
}

static void wcp_trace_map_free(void)
{
    if (wcp_trace_to_id) {
        g_hash_table_destroy(wcp_trace_to_id);
        wcp_trace_to_id = NULL;
    }
    if (wcp_id_to_trace) {
        g_hash_table_destroy(wcp_id_to_trace);
        wcp_id_to_trace = NULL;
    }
}

static gboolean wcp_trace_is_live(GwTrace *t)
{
    for (GwTrace *cur = GLOBALS->traces.first; cur; cur = cur->t_next) {
        if (cur == t) {
            return TRUE;
        }
    }
    return FALSE;
}

static void wcp_trace_map_prune(void)
{
    if (!wcp_trace_to_id) {
        return;
    }

    GHashTable *live = g_hash_table_new(g_direct_hash, g_direct_equal);
    for (GwTrace *t = GLOBALS->traces.first; t; t = t->t_next) {
        g_hash_table_add(live, t);
    }

    GHashTableIter iter;
    gpointer key;
    gpointer value;
    g_hash_table_iter_init(&iter, wcp_trace_to_id);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        if (!g_hash_table_contains(live, key)) {
            guint64 id = *(guint64 *)value;
            g_hash_table_remove(wcp_id_to_trace, &id);
            g_hash_table_iter_remove(&iter);
        }
    }

    g_hash_table_destroy(live);
}

static guint64 wcp_get_trace_id(GwTrace *t)
{
    wcp_trace_map_init();

    guint64 *existing = g_hash_table_lookup(wcp_trace_to_id, t);
    if (existing) {
        return *existing;
    }

    while ((wcp_next_trace_id & WCP_ITEM_MARKER_FLAG) != 0) {
        wcp_next_trace_id++;
    }

    guint64 id = wcp_next_trace_id++;
    g_hash_table_insert(wcp_trace_to_id, t, wcp_id_new(id));
    g_hash_table_insert(wcp_id_to_trace, wcp_id_new(id), t);
    return id;
}

static void wcp_remove_trace_id(GwTrace *t)
{
    if (!wcp_trace_to_id) {
        return;
    }
    guint64 *id_ptr = g_hash_table_lookup(wcp_trace_to_id, t);
    if (id_ptr) {
        guint64 id = *id_ptr;
        g_hash_table_remove(wcp_trace_to_id, t);
        g_hash_table_remove(wcp_id_to_trace, &id);
    }
}

static GwTrace *wcp_lookup_trace(guint64 id)
{
    if (!wcp_id_to_trace) {
        return NULL;
    }
    gint64 key = (gint64)id;
    GwTrace *t = g_hash_table_lookup(wcp_id_to_trace, &key);
    if (!t) {
        return NULL;
    }
    if (!wcp_trace_is_live(t)) {
        wcp_remove_trace_id(t);
        return NULL;
    }
    return t;
}

static gboolean wcp_is_marker_id(guint64 id)
{
    return (id & WCP_ITEM_MARKER_FLAG) != 0;
}

static guint64 wcp_marker_index_from_id(guint64 id)
{
    return id & ~WCP_ITEM_MARKER_FLAG;
}

static guint64 wcp_marker_id_from_index(guint64 idx)
{
    return WCP_ITEM_MARKER_FLAG | idx;
}

static gboolean wcp_has_dump_file(void)
{
    return GLOBALS->dump_file != NULL && GLOBALS->loaded_file_type != MISSING_FILE;
}

static void wcp_item_info_free(gpointer data)
{
    WcpItemInfo *info = data;
    if (!info) {
        return;
    }
    g_free(info->name);
    g_free(info->type);
    g_free(info);
}

static gint wcp_find_marker_index(GwNamedMarkers *markers, GwMarker *marker)
{
    guint count = gw_named_markers_get_number_of_markers(markers);
    for (guint i = 0; i < count; i++) {
        if (gw_named_markers_get(markers, i) == marker) {
            return (gint)i;
        }
    }
    return -1;
}

static gint wcp_parse_color(const gchar *color)
{
    if (!color || !*color) {
        return -1;
    }

    if (g_ascii_isdigit(color[0])) {
        char *end = NULL;
        long val = strtol(color, &end, 10);
        if (end && *end == '\0' && val >= 0 && val <= WAVE_NUM_RAINBOW) {
            return (gint)val;
        }
    }

    static const struct {
        const gchar *name;
        gint value;
    } color_map[] = {
        {"normal", WAVE_COLOR_NORMAL},
        {"default", WAVE_COLOR_NORMAL},
        {"red", WAVE_COLOR_RED},
        {"orange", WAVE_COLOR_ORANGE},
        {"yellow", WAVE_COLOR_YELLOW},
        {"green", WAVE_COLOR_GREEN},
        {"blue", WAVE_COLOR_BLUE},
        {"indigo", WAVE_COLOR_INDIGO},
        {"violet", WAVE_COLOR_VIOLET},
    };

    for (guint i = 0; i < G_N_ELEMENTS(color_map); i++) {
        if (!g_ascii_strcasecmp(color, color_map[i].name)) {
            return color_map[i].value;
        }
    }

    return -1;
}

static gboolean wcp_add_symbol(GwSymbol *sym, GHashTable *added_vec_roots, GArray *added_ids)
{
    GwTrace *added_trace = NULL;

    if (sym->vec_root && GLOBALS->autocoalesce) {
        GwSymbol *root = sym->vec_root;
        if (g_hash_table_contains(added_vec_roots, root)) {
            return FALSE;
        }
        g_hash_table_add(added_vec_roots, root);

        int len = 0;
        for (GwSymbol *t = root; t; t = t->vec_chain) {
            len++;
        }
        if (len > 0) {
            if (GLOBALS->is_lx2) {
                for (GwSymbol *t = root; t; t = t->vec_chain) {
                    if (t->n && t->n->mv.mvlfac) {
                        lx2_set_fac_process_mask(t->n);
                    }
                }
                lx2_import_masked();
            }
            add_vector_chain(root, len);
            added_trace = GLOBALS->traces.last;
        }
    } else {
        if (GLOBALS->is_lx2 && sym->n && sym->n->mv.mvlfac) {
            lx2_set_fac_process_mask(sym->n);
            lx2_import_masked();
        }
        AddNodeTraceReturn(sym->n, NULL, &added_trace);
    }

    if (added_trace && added_ids) {
        WcpDisplayedItemRef ref;
        ref.id = wcp_get_trace_id(added_trace);
        g_array_append_val(added_ids, ref);
        return TRUE;
    }

    return FALSE;
}

static GwTreeNode *wcp_find_scope_node(const gchar *scope)
{
    if (!scope || !*scope) {
        return NULL;
    }

    GwTree *tree = gw_dump_file_get_tree(GLOBALS->dump_file);
    if (!tree) {
        return NULL;
    }

    GwTreeNode *root = gw_tree_get_root(tree);
    if (!root) {
        return NULL;
    }

    char delim[2] = {GLOBALS->hier_delimeter, 0};
    gchar **parts = g_strsplit(scope, delim, -1);
    gint idx = 0;

    GwTreeNode *current = root;
    if (current->name[0] != '\0' && parts[0] && !strcmp(current->name, parts[0])) {
        idx = 1;
    }

    for (; parts[idx]; idx++) {
        if (!parts[idx][0]) {
            continue;
        }
        GwTreeNode *child = current->child;
        while (child && strcmp(child->name, parts[idx])) {
            child = child->next;
        }
        if (!child) {
            g_strfreev(parts);
            return NULL;
        }
        current = child;
    }

    g_strfreev(parts);
    return current;
}

static void wcp_collect_scope_symbols(GwTreeNode *node,
                                      gboolean recursive,
                                      GPtrArray *symbols)
{
    if (!node) {
        return;
    }

    for (GwTreeNode *cur = node; cur; cur = cur->next) {
        if (cur->t_which >= 0) {
            GwFacs *facs = gw_dump_file_get_facs(GLOBALS->dump_file);
            GwSymbol *sym = gw_facs_get(facs, cur->t_which);
            if (sym) {
                g_ptr_array_add(symbols, sym);
            }
        }

        if (recursive && cur->child) {
            wcp_collect_scope_symbols(cur->child, recursive, symbols);
        }
    }
}

/* ============================================================================
 * Command Handlers
 * 
 * Each handler maps a WCP command to GTKWave functionality.
 * Returns a JSON response string (caller frees).
 * ============================================================================ */

static gchar* handle_get_item_list(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    (void)cmd;
    
    GArray *ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    
    wcp_trace_map_prune();
    for (GwTrace *t = GLOBALS->traces.first; t; t = t->t_next) {
        WcpDisplayedItemRef ref;
        ref.id = wcp_get_trace_id(t);
        g_array_append_val(ids, ref);
    }

    if (GLOBALS->project) {
        GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
        guint count = gw_named_markers_get_number_of_markers(markers);
        for (guint i = 0; i < count; i++) {
            GwMarker *marker = gw_named_markers_get(markers, i);
            if (marker && gw_marker_is_enabled(marker)) {
                WcpDisplayedItemRef ref;
                ref.id = wcp_marker_id_from_index(i);
                g_array_append_val(ids, ref);
            }
        }
    }
    
    gchar *response = wcp_create_item_list_response(ids);
    g_array_free(ids, TRUE);
    return response;
}

static gchar* handle_get_item_info(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    
    GPtrArray *results = g_ptr_array_new_with_free_func(wcp_item_info_free);
    
    if (cmd->data.item_refs.ids) {
        for (guint i = 0; i < cmd->data.item_refs.ids->len; i++) {
            WcpDisplayedItemRef *ref = &g_array_index(cmd->data.item_refs.ids,
                                                       WcpDisplayedItemRef, i);

            if (wcp_is_marker_id(ref->id)) {
                GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
                guint64 idx64 = wcp_marker_index_from_id(ref->id);
                if (idx64 > G_MAXUINT) {
                    g_ptr_array_free(results, TRUE);
                    return wcp_create_error("invalid_item", "Unknown marker id", NULL);
                }
                guint idx = (guint)idx64;
                GwMarker *marker = gw_named_markers_get(markers, idx);
                if (!marker || !gw_marker_is_enabled(marker)) {
                    g_ptr_array_free(results, TRUE);
                    return wcp_create_error("invalid_item", "Unknown marker id", NULL);
                }
                WcpItemInfo *info = g_new0(WcpItemInfo, 1);
                info->id = *ref;
                info->name = g_strdup(gw_marker_get_display_name(marker));
                info->type = g_strdup("marker");
                g_ptr_array_add(results, info);
                continue;
            }

            GwTrace *t = wcp_lookup_trace(ref->id);
            if (!t) {
                g_ptr_array_free(results, TRUE);
                return wcp_create_error("invalid_item", "Unknown item id", NULL);
            }
            WcpItemInfo *info = g_new0(WcpItemInfo, 1);
            info->id = *ref;
            info->name = g_strdup(t->name ? t->name : "");
            info->type = g_strdup("signal");
            g_ptr_array_add(results, info);
        }
    }
    
    gchar *response = wcp_create_item_info_response(results);
    g_ptr_array_free(results, TRUE);
    return response;
}

static gchar* handle_set_item_color(WcpServer *server, WcpCommand *cmd)
{
    (void)server;

    if (wcp_is_marker_id(cmd->data.set_color.id.id)) {
        return wcp_create_error("invalid_item",
                                "set_item_color only applies to signals",
                                NULL);
    }

    GwTrace *t = wcp_lookup_trace(cmd->data.set_color.id.id);
    if (!t) {
        return wcp_create_error("invalid_item",
                                "Unknown item id",
                                NULL);
    }

    gint color = wcp_parse_color(cmd->data.set_color.color);
    if (color < 0) {
        return wcp_create_error("invalid_color",
                                "Unsupported color value",
                                NULL);
    }

    t->t_color = (unsigned int)color;
    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();

    return wcp_create_ack();
}

static gchar* handle_add_variables(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    
    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    GArray *added_ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    
    if (cmd->data.add_vars.variables) {
        GHashTable *added_vec_roots = g_hash_table_new(g_direct_hash, g_direct_equal);
        for (guint i = 0; i < cmd->data.add_vars.variables->len; i++) {
            const gchar *varname = g_ptr_array_index(cmd->data.add_vars.variables, i);

            GwSymbol *sym = gw_dump_file_lookup_symbol(GLOBALS->dump_file, varname);
            if (sym) {
                wcp_add_symbol(sym, added_vec_roots, added_ids);
            }
        }
        g_hash_table_destroy(added_vec_roots);
    }
    
    GLOBALS->signalwindow_width_dirty = 1;
    MaxSignalLength();
    redraw_signals_and_waves();
    
    gchar *response = wcp_create_add_items_response_for("add_variables", added_ids);
    g_array_free(added_ids, TRUE);
    return response;
}

static gchar* handle_add_scope(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    
    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    GArray *added_ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    
    GwTreeNode *scope = wcp_find_scope_node(cmd->data.add_scope.scope);
    if (!scope) {
        g_array_free(added_ids, TRUE);
        return wcp_create_error("invalid_scope", "Scope not found", NULL);
    }

    GPtrArray *symbols = g_ptr_array_new();
    wcp_collect_scope_symbols(scope->child, cmd->data.add_scope.recursive, symbols);

    GHashTable *added_vec_roots = g_hash_table_new(g_direct_hash, g_direct_equal);
    for (guint i = 0; i < symbols->len; i++) {
        GwSymbol *sym = g_ptr_array_index(symbols, i);
        wcp_add_symbol(sym, added_vec_roots, added_ids);
    }
    g_hash_table_destroy(added_vec_roots);
    g_ptr_array_free(symbols, TRUE);
    
    GLOBALS->signalwindow_width_dirty = 1;
    MaxSignalLength();
    redraw_signals_and_waves();
    
    gchar *response = wcp_create_add_items_response_for("add_scope", added_ids);
    g_array_free(added_ids, TRUE);
    return response;
}

static gchar* handle_add_markers(WcpServer *server, WcpCommand *cmd)
{
    (void)server;

    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    GArray *added_ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    
    if (cmd->data.add_markers.markers) {
        GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
        for (guint i = 0; i < cmd->data.add_markers.markers->len; i++) {
            WcpMarkerInfo *m = &g_array_index(cmd->data.add_markers.markers,
                                               WcpMarkerInfo, i);

            GwMarker *marker = gw_named_markers_find_first_disabled(markers);
            if (!marker) {
                g_array_free(added_ids, TRUE);
                return wcp_create_error("no_available_marker",
                                        "No available named marker slots",
                                        NULL);
            }

            gw_marker_set_position(marker, (GwTime)m->time);
            gw_marker_set_enabled(marker, TRUE);
            if (m->name) {
                gw_marker_set_alias(marker, m->name);
            }

            gint idx = wcp_find_marker_index(markers, marker);
            if (idx >= 0) {
                WcpDisplayedItemRef ref;
                ref.id = wcp_marker_id_from_index((guint64)idx);
                g_array_append_val(added_ids, ref);
            }

            if (m->move_focus) {
                GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
                gw_marker_set_position(primary_marker, (GwTime)m->time);
                gw_marker_set_enabled(primary_marker, TRUE);
                update_time_box();
            }
        }
    }
    
    redraw_signals_and_waves();

    gchar *response = wcp_create_add_items_response_for("add_markers", added_ids);
    g_array_free(added_ids, TRUE);
    return response;
}

static gchar* handle_add_items(WcpServer *server, WcpCommand *cmd)
{
    (void)server;

    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    GArray *added_ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    if (cmd->data.add_items.items) {
        GHashTable *added_vec_roots = g_hash_table_new(g_direct_hash, g_direct_equal);

        for (guint i = 0; i < cmd->data.add_items.items->len; i++) {
            const gchar *item = g_ptr_array_index(cmd->data.add_items.items, i);
            GwSymbol *sym = gw_dump_file_lookup_symbol(GLOBALS->dump_file, item);
            if (sym) {
                wcp_add_symbol(sym, added_vec_roots, added_ids);
                continue;
            }

            GwTreeNode *scope = wcp_find_scope_node(item);
            if (scope) {
                GPtrArray *symbols = g_ptr_array_new();
                wcp_collect_scope_symbols(scope->child, cmd->data.add_items.recursive, symbols);
                for (guint j = 0; j < symbols->len; j++) {
                    wcp_add_symbol(g_ptr_array_index(symbols, j), added_vec_roots, added_ids);
                }
                g_ptr_array_free(symbols, TRUE);
            }
        }

        g_hash_table_destroy(added_vec_roots);
    }

    GLOBALS->signalwindow_width_dirty = 1;
    MaxSignalLength();
    redraw_signals_and_waves();

    gchar *response = wcp_create_add_items_response_for("add_items", added_ids);
    g_array_free(added_ids, TRUE);
    return response;
}

static gchar* handle_set_viewport_to(WcpServer *server, WcpCommand *cmd)
{
    (void)server;

    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    GwTime target = (GwTime)cmd->data.viewport_to.timestamp;
    GwTime width = (GwTime)(GLOBALS->wavewidth * GLOBALS->nspx);
    GwTime max_width = GLOBALS->tims.last - GLOBALS->tims.first;

    if (width <= 0 || width > max_width) {
        width = max_width;
    }

    GwTime start = target - (width / 2);
    if (start < GLOBALS->tims.first) {
        start = GLOBALS->tims.first;
    }
    if (start > GLOBALS->tims.last - width + 1) {
        start = GLOBALS->tims.last - width + 1;
    }
    if (start < GLOBALS->tims.first) {
        start = GLOBALS->tims.first;
    }

    GtkAdjustment *hadj = GTK_ADJUSTMENT(GLOBALS->wave_hslider);
    gtk_adjustment_set_value(hadj, start);
    GLOBALS->tims.timecache = start;
    time_update();
    
    return wcp_create_ack();
}

static gchar* handle_set_viewport_range(WcpServer *server, WcpCommand *cmd)
{
    (void)server;

    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    GwTime start = (GwTime)cmd->data.viewport_range.start;
    GwTime end = (GwTime)cmd->data.viewport_range.end;
    if (start > end) {
        GwTime tmp = start;
        start = end;
        end = tmp;
    }

    service_dragzoom(start, end);
    
    return wcp_create_ack();
}

static gchar* handle_remove_items(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    
    if (cmd->data.item_refs.ids) {
        for (guint i = 0; i < cmd->data.item_refs.ids->len; i++) {
            WcpDisplayedItemRef *ref = &g_array_index(cmd->data.item_refs.ids,
                                                       WcpDisplayedItemRef, i);
            if (wcp_is_marker_id(ref->id)) {
                GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
                guint64 idx64 = wcp_marker_index_from_id(ref->id);
                if (idx64 > G_MAXUINT) {
                    continue;
                }
                GwMarker *marker =
                    gw_named_markers_get(markers, (guint)idx64);
                if (marker) {
                    gw_marker_set_enabled(marker, FALSE);
                    gw_marker_set_alias(marker, NULL);
                }
                continue;
            }

            GwTrace *t = wcp_lookup_trace(ref->id);
            if (t) {
                wcp_remove_trace_id(t);
                RemoveTrace(t, 1);
            }
        }
    }
    
    GLOBALS->signalwindow_width_dirty = 1;
    MaxSignalLength();
    redraw_signals_and_waves();
    
    return wcp_create_ack();
}

static gchar* handle_focus_item(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    
    if (wcp_is_marker_id(cmd->data.focus.id.id)) {
        GwNamedMarkers *markers = gw_project_get_named_markers(GLOBALS->project);
        guint64 idx64 = wcp_marker_index_from_id(cmd->data.focus.id.id);
        if (idx64 > G_MAXUINT) {
            return wcp_create_error("invalid_item", "Unknown marker id", NULL);
        }
        GwMarker *marker =
            gw_named_markers_get(markers, (guint)idx64);
        if (marker) {
            GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
            gw_marker_set_position(primary_marker, gw_marker_get_position(marker));
            gw_marker_set_enabled(primary_marker, TRUE);
            update_time_box();
            redraw_signals_and_waves();
            return wcp_create_ack();
        }
        return wcp_create_error("invalid_item", "Unknown marker id", NULL);
    }

    GwTrace *t = wcp_lookup_trace(cmd->data.focus.id.id);
    if (!t) {
        return wcp_create_error("invalid_item", "Unknown item id", NULL);
    }

    ClearTraces();
    t->flags |= TR_HIGHLIGHT;
    gw_signal_list_scroll_to_trace(GW_SIGNAL_LIST(GLOBALS->signalarea), t);
    redraw_signals_and_waves();
    
    return wcp_create_ack();
}

static gchar* handle_clear(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    (void)cmd;
    
    while (GLOBALS->traces.first) {
        GwTrace *t = GLOBALS->traces.first;
        wcp_remove_trace_id(t);
        RemoveTrace(t, 1);
    }

    collect_all_named_markers(NULL, 0, NULL);
    GLOBALS->signalwindow_width_dirty = 1;
    MaxSignalLength();
    redraw_signals_and_waves();
    
    return wcp_create_ack();
}

static gchar* handle_load(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    
    const gchar *filename = cmd->data.load.source;
    
    if (!filename || !*filename) {
        return wcp_create_error("invalid_argument", "Missing source", NULL);
    }

    if (!deal_with_rpc_open(filename, NULL)) {
        return wcp_create_error("load_failed", "Failed to queue file load", NULL);
    }
    
    return wcp_create_ack();
}

static gchar* handle_reload(WcpServer *server, WcpCommand *cmd)
{
    (void)server;
    (void)cmd;
    
    if (!wcp_has_dump_file() || !GLOBALS->loaded_file_name) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    reload_into_new_context();
    wcp_gtkwave_notify_waveforms_loaded(GLOBALS->loaded_file_name);
    
    return wcp_create_ack();
}

static gchar* handle_zoom_to_fit(WcpServer *server, WcpCommand *cmd)
{
    (void)server;

    if (!wcp_has_dump_file()) {
        return wcp_create_error("no_waveform", "No waveform loaded", NULL);
    }

    if (cmd->data.zoom.viewport_idx != 0) {
        return wcp_create_error("invalid_argument", "Unsupported viewport index", NULL);
    }

    service_zoom_fit(NULL, NULL);
    
    return wcp_create_ack();
}

static gchar* handle_shutdown(WcpServer *server, WcpCommand *cmd)
{
    (void)cmd;

    /* Stop the WCP server (but don't quit GTKWave) */
    wcp_server_stop(server);
    
    return wcp_create_ack();
}

/* ============================================================================
 * Main Command Dispatcher
 * ============================================================================ */

static gchar* wcp_command_handler(WcpServer *server, WcpCommand *cmd, gpointer user_data)
{
    (void)user_data;
    
    switch (cmd->type) {
        case WCP_CMD_GET_ITEM_LIST:
            return handle_get_item_list(server, cmd);
            
        case WCP_CMD_GET_ITEM_INFO:
            return handle_get_item_info(server, cmd);
            
        case WCP_CMD_SET_ITEM_COLOR:
            return handle_set_item_color(server, cmd);
            
        case WCP_CMD_ADD_VARIABLES:
            return handle_add_variables(server, cmd);
            
        case WCP_CMD_ADD_SCOPE:
            return handle_add_scope(server, cmd);
            
        case WCP_CMD_ADD_ITEMS:
            return handle_add_items(server, cmd);
            
        case WCP_CMD_ADD_MARKERS:
            return handle_add_markers(server, cmd);
            
        case WCP_CMD_REMOVE_ITEMS:
            return handle_remove_items(server, cmd);
            
        case WCP_CMD_FOCUS_ITEM:
            return handle_focus_item(server, cmd);
            
        case WCP_CMD_CLEAR:
            return handle_clear(server, cmd);
            
        case WCP_CMD_SET_VIEWPORT_TO:
            return handle_set_viewport_to(server, cmd);
            
        case WCP_CMD_SET_VIEWPORT_RANGE:
            return handle_set_viewport_range(server, cmd);
            
        case WCP_CMD_ZOOM_TO_FIT:
            return handle_zoom_to_fit(server, cmd);
            
        case WCP_CMD_LOAD:
            return handle_load(server, cmd);
            
        case WCP_CMD_RELOAD:
            return handle_reload(server, cmd);
            
        case WCP_CMD_SHUTDOWN:
            return handle_shutdown(server, cmd);
            
        default:
            return wcp_create_error("unknown_command", 
                                    "Unknown command type",
                                    NULL);
    }
}

/* ============================================================================
 * Public API
 * ============================================================================ */

gboolean wcp_gtkwave_init(guint16 port)
{
    if (g_wcp_server) {
        g_warning("WCP: Already initialized");
        return FALSE;
    }
    
    g_wcp_server = wcp_server_new(port, wcp_command_handler, NULL);
    
    GError *error = NULL;
    if (!wcp_server_start(g_wcp_server, &error)) {
        g_warning("WCP: Failed to start server: %s", error->message);
        g_error_free(error);
        wcp_server_free(g_wcp_server);
        g_wcp_server = NULL;
        return FALSE;
    }
    
    return TRUE;
}

gboolean wcp_gtkwave_initiate(const gchar *host, guint16 port)
{
    if (g_wcp_server) {
        g_warning("WCP: Already initialized");
        return FALSE;
    }
    
    g_wcp_server = wcp_server_new(0, wcp_command_handler, NULL);
    
    GError *error = NULL;
    if (!wcp_server_initiate(g_wcp_server, host, port, &error)) {
        g_warning("WCP: Failed to connect: %s", error->message);
        g_error_free(error);
        wcp_server_free(g_wcp_server);
        g_wcp_server = NULL;
        return FALSE;
    }
    
    return TRUE;
}

void wcp_gtkwave_shutdown(void)
{
    if (g_wcp_server) {
        wcp_server_free(g_wcp_server);
        g_wcp_server = NULL;
    }
    wcp_trace_map_free();
}

void wcp_gtkwave_notify_waveforms_loaded(const gchar *filename)
{
    if (g_wcp_server) {
        wcp_server_emit_waveforms_loaded(g_wcp_server, filename);
    }
}
