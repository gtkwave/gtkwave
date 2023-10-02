#include "signal_list.h"
#include "globals.h"
#include "menu.h"

#define SCROLL_AREA_HEIGHT (GLOBALS->fontheight * 2)

#ifdef MAC_INTEGRATION
#define GW_CONTROL_MASK GDK_META_MASK
#else
#define GW_CONTROL_MASK GDK_CONTROL_MASK
#endif

/* workaround for old versions of glib such as in centos7 */
#if !defined(G_SOURCE_FUNC)
#define G_SOURCE_FUNC(f) ((GSourceFunc)(void (*)(void))(f))
#endif

// Variables related to dragging from the signal list
typedef struct
{
    // TRUE if the primary button is pressed but the drag distance threshold hasn't been reached
    gboolean pending;

    // TRUE if the cursor was located over a selected trace when the button was pressed
    gboolean selected;

    // Start position of the drag gesture
    gdouble start_x, start_y;
} Drag;

// Variables related to dropping traces on the signal list
typedef struct
{
    // Index of the drop position highlight or -1 if no drop position is shown
    int highlight_position;

    // Selected action.
    GdkDragAction action;

    // TRUE if the cursor is inside the autoscroll area in the top and bottom of the list
    gboolean in_scroll_area_top;
    gboolean in_scroll_area_bottom;

    // Timer for automatic scrolling
    guint scroll_timeout_source;
} Drop;

// Reset a Drop struct
//
// Returns TRUE if the widget needs to be redrawn.
static gboolean drop_reset(Drop *drop)
{
    drop->in_scroll_area_top = FALSE;
    drop->in_scroll_area_bottom = FALSE;

    if (drop->highlight_position != -1) {
        drop->highlight_position = -1;
        return TRUE;
    } else {
        return FALSE;
    }
}

struct _GwSignalList
{
    GtkDrawingArea parent_instance;

    cairo_surface_t *surface;

    // TRUE if the internal surface needs to be redrawn
    gboolean dirty;

    // Cursor trace or NULL if the cursor isn't active
    GwTrace *cursor;

    Drag drag;
    Drop drop;

    // Scroll adjustments
    GtkAdjustment *hadjustment;
    GtkAdjustment *vadjustment;
};

G_DEFINE_TYPE(GwSignalList, gw_signal_list, GTK_TYPE_DRAWING_AREA)

// Get the text and background colors for a trace
static void get_trace_colors(GwTrace *t,
                             GwSignalListColors *colors,
                             GwColor *bg_color,
                             GwColor *text_color)
{
    GwColor clr_comment = colors->brkred;
    GwColor clr_group = colors->gmstrd;
    GwColor clr_shadowed = colors->ltblue;
    GwColor clr_signal = colors->dkblue;

    if (IsSelected(t)) {
        if (HasWave(t)) {
            *bg_color = IsShadowed(t) ? clr_shadowed : clr_signal;
        } else if (IsGroupBegin(t) || IsGroupEnd(t)) {
            *bg_color = clr_group;
        } else {
            *bg_color = clr_comment;
        }
    } else {
        *bg_color = colors->ltgray;
    }

    if (IsSelected(t)) {
        *text_color = colors->white;
    } else if (HasWave(t)) {
        *text_color = colors->black;
    } else if (IsGroupBegin(t) || IsGroupEnd(t)) {
        *text_color = clr_group;
    } else {
        *text_color = clr_comment;
    }
}

// Return the maximum number of traces that can be displayed
int gw_signal_list_get_num_traces_displayable(GwSignalList *signal_list)
{
    g_return_val_if_fail(GW_IS_SIGNAL_LIST(signal_list), 0);

    int height = gtk_widget_get_allocated_height(GTK_WIDGET(signal_list));

    /* minus one for the time trace that is always there */
    return height / GLOBALS->fontheight - 1;
}

// Render one signal row
//
// The signal is always rendered at the origin. The cairo context needs to be
// translated to the correct position before calling this function.
static void render_signal(cairo_t *cr,
                          GwSignalListColors *colors,
                          GwTrace *t,
                          int width,
                          int text_dx)
{
    char buf[2048];
    char *subname = NULL;
    GwBitVector *bv = NULL;

    /* seek to real xact trace if present... */
    if (t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)) {
        GwTrace *tscan = t;
        int bcnt = 0;
        while (tscan && (tscan = GivePrevTrace(tscan))) {
            if (!(tscan->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
                if (tscan->flags & TR_TTRANSLATED) {
                    break; /* found it */
                } else {
                    tscan = NULL;
                }
            } else {
                bcnt++; /* bcnt is number of blank traces */
            }
        }

        if (tscan != NULL && tscan->vector != 0) {
            bv = tscan->n.vec;
            do {
                bv = bv->transaction_chain; /* correlate to blank trace */
            } while (bv && (bcnt--));

            if (bv) {
                subname = bv->bvname;
                if (GLOBALS->hier_max_level) {
                    subname = hier_extract(subname, GLOBALS->hier_max_level);
                }
            }
        }
    }

    buf[0] = 0;
    populateBuffer(t, subname, buf);

    UpdateSigValue(t); /* in case it's stale on nonprop */

    // Center text vertically
    gdouble text_y = GLOBALS->fontheight / 2.0 + GLOBALS->signalfont->ascent / 2.0 -
                     GLOBALS->signalfont->descent / 2.0;

    GwColor bg_color;
    GwColor text_color;
    get_trace_colors(t, colors, &bg_color, &text_color);

    cairo_set_source_rgba(cr, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    cairo_rectangle(cr, 0, 0, width, GLOBALS->fontheight - 1);
    cairo_fill(cr);

    if (t->name || subname) {
        int text_x;
        if (((IsGroupBegin(t) || IsGroupEnd(t)) && !HasWave(t)) || GLOBALS->left_justify_sigs) {
            text_x = 3 + text_dx;
        } else {
            text_x = 3 + GLOBALS->max_signal_name_pixel_width -
                     font_engine_string_measure(GLOBALS->signalfont, buf) + text_dx;
        }

        XXX_font_engine_draw_string(cr, GLOBALS->signalfont, &text_color, text_x, text_y, buf);
    }

    if (HasWave(t) || bv) {
        if (t->asciivalue && !(t->flags & TR_EXCLUDE)) {
            int text_x = GLOBALS->max_signal_name_pixel_width + 6 + text_dx;
            XXX_font_engine_draw_string(cr,
                                        GLOBALS->signalfont,
                                        &text_color,
                                        text_x,
                                        text_y,
                                        t->asciivalue);
        }
    }
}

// Render all signals
static void render_signals(GwSignalList *signal_list, GwSignalListColors *colors)
{
    cairo_t *cr = cairo_create(signal_list->surface);

    // Clear background
    cairo_set_source_rgba(cr, colors->white.r, colors->white.g, colors->white.b, colors->white.a);
    cairo_paint(cr);

    int num_traces_displayable = gw_signal_list_get_num_traces_displayable(signal_list);
    int width = gtk_widget_get_allocated_width(GTK_WIDGET(signal_list));
    int text_dx = -gtk_adjustment_get_value(signal_list->hadjustment);

    GwTrace *t = gw_signal_list_get_trace(signal_list, 0);
    if (t) {
        int i;
        for (i = 0; i < num_traces_displayable && t; i++) {
            render_signal(cr, colors, t, width, text_dx);

            cairo_translate(cr, 0, GLOBALS->fontheight);

            t = GiveNextTrace(t);
        }
    }

    cairo_destroy(cr);
}

static gint configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
    (void)event;

    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    if (signal_list->surface) {
        cairo_surface_destroy(signal_list->surface);
    }

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    signal_list->surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget),
                                                             CAIRO_CONTENT_COLOR,
                                                             allocation.width,
                                                             allocation.height);
    signal_list->dirty = TRUE;

    return GDK_EVENT_STOP;
}

static void update_hadjustment(GwSignalList *signal_list)
{
    gdouble width = (gdouble)gtk_widget_get_allocated_width(GTK_WIDGET(signal_list));

    gdouble lower = 0.0;
    gdouble upper = MAX(GLOBALS->signal_pixmap_width, 1.0);
    gdouble increment = 1.0;
    gdouble page_size = width;
    gdouble page_increment = width;
    gdouble value = gtk_adjustment_get_value(signal_list->hadjustment);

    gtk_adjustment_configure(signal_list->hadjustment,
                             value,
                             lower,
                             upper,
                             increment,
                             page_increment,
                             page_size);
}

static gboolean draw(GtkWidget *widget, cairo_t *cr)
{
    GwSignalListColors *colors = gw_color_theme_get_signal_list_colors(GLOBALS->color_theme);
    if (GLOBALS->black_and_white) {
        colors = gw_signal_list_colors_new_black_and_white();
    }

    gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
    struct Global *g_old = GLOBALS;
    set_GLOBALS((*GLOBALS->contexts)[page_num]);

    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    // Update surface if dirty flag is set
    if (signal_list->dirty) {
        // Update scroll bounds.
        gw_signal_list_scroll(signal_list, gtk_adjustment_get_value(signal_list->vadjustment));

        update_hadjustment(signal_list);
        render_signals(signal_list, colors);
        signal_list->dirty = FALSE;
    }

    // Draw surface underneath the header
    cairo_set_source_surface(cr, signal_list->surface, 0.0, GLOBALS->fontheight);
    cairo_paint(cr);

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    // Draw the header
    if (!GLOBALS->use_dark) {
        cairo_set_source_rgba(cr,
                              colors->mdgray.r,
                              colors->mdgray.g,
                              colors->mdgray.b,
                              colors->mdgray.a);
        cairo_rectangle(cr, 0, 0, allocation.width, GLOBALS->fontheight);
        cairo_fill(cr);

        XXX_font_engine_draw_string(cr,
                                    GLOBALS->signalfont,
                                    &colors->black,
                                    4,
                                    GLOBALS->fontheight - 4,
                                    "Time");
    }

    // Draw focus rectangle
    if (gtk_widget_has_focus(widget)) {
        cairo_set_source_rgba(cr,
                              colors->black.r,
                              colors->black.g,
                              colors->black.b,
                              colors->black.a);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, 0.5, 0.5, allocation.width - 1, allocation.height - 1);
        cairo_stroke(cr);
    }

    // Draw drop position highlight
    if (signal_list->drop.highlight_position >= 0) {
        int ylin = ((signal_list->drop.highlight_position + 1) * GLOBALS->fontheight) - 1;

        cairo_set_source_rgba(cr,
                              colors->black.r,
                              colors->black.g,
                              colors->black.b,
                              colors->black.a);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, 0.0, ylin + 0.5);
        cairo_rel_line_to(cr, allocation.width, 0);
        cairo_stroke(cr);
    }

    set_GLOBALS(g_old);

    if (GLOBALS->black_and_white) {
        g_free(colors);
    }

    return GDK_EVENT_STOP;
}

// Return the nth trace from the top.
GwTrace *gw_signal_list_get_trace(GwSignalList *signal_list, guint index)
{
    g_return_val_if_fail(GW_IS_SIGNAL_LIST(signal_list), NULL);

    guint which = (guint)gtk_adjustment_get_value(signal_list->vadjustment) + index;
    GwTrace *t = GLOBALS->traces.first;
    while (which > 0 && t != NULL) {
        GwTrace *t_next = GiveNextTrace(t);
        if (t_next == NULL) {
            return NULL;
        }

        t = t_next;
        which--;
    }

    return t;
}

// Return the trace at the given y position
//
// Returns NULL if there is no trace at that y position.
static GwTrace *get_trace_for_y(GwSignalList *signal_list, int y)
{
    g_return_val_if_fail(GW_IS_SIGNAL_LIST(signal_list), NULL);

    int num_traces_displayable = gw_signal_list_get_num_traces_displayable(signal_list);

    int which = y;
    which = which / GLOBALS->fontheight - 1; // Subtract one for the header trace

    if (which >= 0 && which < num_traces_displayable) {
        // Get topmost trace
        GwTrace *t = gw_signal_list_get_trace(signal_list, 0);

        while (t != NULL && which > 0) {
            t = GiveNextTrace(t);
            which--;
        }

        if (t != NULL) {
            return t;
        }
    }

    return NULL;
}

// Selects all traces between a and b (inclusive).
static void select_range(GwTrace *a, GwTrace *b)
{
    // Determine the order of trace a and trace b.
    GwTrace *from = NULL;
    GwTrace *to = NULL;
    GwTrace *t = GLOBALS->traces.first;
    while (t) {
        if (t == a) {
            from = a;
            to = b;
            break;
        } else if (t == b) {
            from = b;
            to = a;
            break;
        }

        t = t->t_next;
    }

    if (from == NULL || to == NULL) {
        g_error("couldn't determine order of traces");
        return;
    }

    // Highlight traces
    t = from;
    while (t != NULL) {
        t->flags |= TR_HIGHLIGHT;

        if (t == to) {
            break;
        }

        t = t->t_next;
    };

    if (t != to) {
        g_error("invalid selection range");
    }
}

static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event)
{
    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);
    gboolean full_redraw =
        GLOBALS->highlight_wavewindow; // force redraw for this because signal updates cause
                                       // wavewindow updates when active

    gtk_widget_grab_focus(widget);

    if (gdk_event_triggers_context_menu((GdkEvent *)event)) {
        GwTrace *t = get_trace_for_y(signal_list, (int)event->y);
        if (t != NULL) {
            if (IsSelected(t)) {
                // Keep current selection if click was on an already selected trace
            } else {
                // Otherwise only select the clicked trace
                ClearTraces();
                t->flags ^= TR_HIGHLIGHT;
                signal_list->cursor = t;
                signal_list->dirty = TRUE;
            }
        }

        do_popup_menu(widget, event);
    } else if (event->button == GDK_BUTTON_PRIMARY) {
        if (event->type == GDK_BUTTON_PRESS) {
            // Single primary press -> select trace or start DnD

            GwTrace *t = get_trace_for_y(signal_list, (int)event->y);

            gboolean selected = t != NULL && IsSelected(t) ? TRUE : FALSE;

            // Clear previous selection if control key isn't pressend.
            // Also don't clear selection if an already selected trace was under
            // the cursor. In this case the decision if the selection should be
            // changed is postponed until the button is released to allow dragging
            // of selected traces.
            if (!((event->state & GW_CONTROL_MASK) || selected)) {
                ClearTraces();
                signal_list->dirty = TRUE;
            }

            if (t != NULL) {
                // Start possible drag gesture.
                signal_list->drag.pending = TRUE;
                signal_list->drag.selected = selected;
                signal_list->drag.start_x = event->x;
                signal_list->drag.start_y = event->y;

                if ((event->state & GDK_SHIFT_MASK) && signal_list->cursor != NULL) {
                    select_range(signal_list->cursor, t);
                    signal_list->dirty = TRUE;
                } else if (!signal_list->drag.selected) {
                    // Highlight trace immediately if trace wasn't previously selected.
                    // If the trace was already selected this might be the start
                    // of a drag gesture and the decision is postopned until the
                    // button is released or the drag threshold distance has
                    // been reached.
                    t->flags ^= TR_HIGHLIGHT;
                    signal_list->cursor = t;
                    signal_list->dirty = TRUE;
                }
            }
        } else if (event->type == GDK_2BUTTON_PRESS) {
            // Double primary press -> open/close groups and expand/collapse waves

            // After a double click was detected the user input can no longer be
            // the start of a drag gesture. Reset the  pending flag to make sure
            // the release handler doesn't cause unwanted selection changes.
            signal_list->drag.pending = FALSE;

            GwTrace *t = get_trace_for_y(signal_list, (int)event->y);
            if (t != NULL) {
                if (IsGroupBegin(t) || IsGroupEnd(t)) {
                    if (IsClosed(t)) {
                        OpenTrace(t);
                        full_redraw = TRUE;
                    } else {
                        CloseTrace(t);
                        full_redraw = TRUE;
                    }
                } else if (HasWave(t)) {
                    ClearTraces();
                    t->flags |= TR_HIGHLIGHT;

                    // Setting full_redraw to TRUE isn't necessary because
                    // menu_expand already triggers a redraw.
                    menu_expand(NULL, 0, NULL);
                }
            }
        }
    }

    // A full redraw isn't necessary if only the selection has changed
    if (full_redraw) {
        redraw_signals_and_waves();
    } else if (signal_list->dirty) {
        gtk_widget_queue_draw(widget);
    }

    return GDK_EVENT_STOP;
}

static gboolean button_release_event(GtkWidget *widget, GdkEventButton *event)
{
    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    gtk_widget_grab_focus(widget);

    if (signal_list->drag.pending && signal_list->drag.selected) {
        // The primary button was pressed on a selected trace, but the
        // distance threshold for a drag gesture wasn't reached.
        // This should be handled by selecting the trace under the cursor.

        GwTrace *t = get_trace_for_y(signal_list, (int)event->y);
        if (t != NULL) {
            if (event->state & GW_CONTROL_MASK) {
                t->flags ^= TR_HIGHLIGHT;
            } else {
                ClearTraces();
                t->flags |= TR_HIGHLIGHT;
            }

            signal_list->cursor = t;

            signal_list->dirty = TRUE;
            gtk_widget_queue_draw(widget);
        }
    }

    signal_list->drag.pending = FALSE;

    return GDK_EVENT_STOP;
}

// Scroll the list to show the trace with the given index as the topmost trace.
void gw_signal_list_scroll(GwSignalList *signal_list, int index)
{
    g_return_if_fail(GW_IS_SIGNAL_LIST(signal_list));

    int num_traces_displayable = gw_signal_list_get_num_traces_displayable(signal_list);

    UpdateTracesVisible();

    gdouble lower = 0.0;
    gdouble upper = MAX((gdouble)GLOBALS->traces.visible, 1.0);
    gdouble increment = 1.0;
    gdouble page_size = (gdouble)num_traces_displayable;
    gdouble page_increment = page_size;

    gtk_adjustment_configure(signal_list->vadjustment,
                             index,
                             lower,
                             upper,
                             increment,
                             page_increment,
                             page_size);
}

// Scroll the list vertically to show the given trace.
void gw_signal_list_scroll_to_trace(GwSignalList *signal_list, GwTrace *trace)
{
    g_return_if_fail(GW_IS_SIGNAL_LIST(signal_list));

    if (trace == NULL) {
        return;
    }

    int which = 0;
    GwTrace *t = GLOBALS->traces.first;
    while (t != NULL && t != trace) {
        t = GiveNextTrace(t);
        which++;
    }

    int value = gtk_adjustment_get_value(signal_list->vadjustment);
    int num_traces = gw_signal_list_get_num_traces_displayable(signal_list);

    if (which < value) {
        // The trace is above the current top trace
        // -> move scrollbar up to show trace as the new top trace
        value = which;
    } else if (which >= value + num_traces) {
        // The trace is below the current bottom trace
        // -> move scrollbar down to show trace as the new bottom trace
        value = which - num_traces + 1;
    }

    // Use gw_signal_list_scroll instead of gtk_adjustment_set_value to make
    // sure the upper and lower bounds are updated if they have been changed
    // and no redraw has occured since then.
    gw_signal_list_scroll(signal_list, value);
}

// Scroll list upward
//
// page == FALSE -> scroll by one trace
// page == TRUE  -> scroll by one page
static void scroll_up(GwSignalList *signal_list, gboolean page)
{
    gdouble delta = 1.0;
    if (page) {
        delta = gtk_adjustment_get_page_increment(signal_list->vadjustment);
    }

    gdouble new_value = gtk_adjustment_get_value(signal_list->vadjustment) - delta;
    gtk_adjustment_set_value(signal_list->vadjustment, new_value);
}

// Scroll list downward
//
// page == FALSE -> scroll by one trace
// page == TRUE  -> scroll by one page
static void scroll_down(GwSignalList *signal_list, gboolean page)
{
    gdouble delta = 1.0;
    if (page) {
        delta = gtk_adjustment_get_page_increment(signal_list->vadjustment);
    }

    gdouble new_value = gtk_adjustment_get_value(signal_list->vadjustment) + delta;
    gtk_adjustment_set_value(signal_list->vadjustment, new_value);
}

// Handle scroll wheel events
static gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    if (event->direction == GDK_SCROLL_UP) {
        scroll_up(signal_list, FALSE);
        return GDK_EVENT_STOP;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        scroll_down(signal_list, FALSE);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

// Timer callback to handle automatic scrolling
//
// This timer gets enabled if a drag operation is active and the cursor is over
// the top or bottom autoscroll area.
static gboolean scroll_timeout(GwSignalList *signal_list)
{
    g_return_val_if_fail(GW_IS_SIGNAL_LIST(signal_list), G_SOURCE_REMOVE);

    if (signal_list->drop.in_scroll_area_top) {
        scroll_up(signal_list, FALSE);

        return G_SOURCE_CONTINUE;
    } else if (signal_list->drop.in_scroll_area_bottom) {
        scroll_down(signal_list, FALSE);

        return G_SOURCE_CONTINUE;
    } else {
        // The cursor has left the autoscoll areas -> disable timer
        signal_list->drop.scroll_timeout_source = 0;
        return G_SOURCE_REMOVE;
    }
}

// Sets the cursor trace and make sure the trace is visible.
static void set_cursor(GwSignalList *signal_list, GwTrace *cursor)
{
    if (signal_list->cursor != NULL) {
        signal_list->cursor->flags &= ~TR_HIGHLIGHT;
    }

    signal_list->cursor = cursor;
    if (signal_list->cursor != NULL) {
        signal_list->cursor->flags |= TR_HIGHLIGHT;

        gw_signal_list_scroll_to_trace(signal_list, signal_list->cursor);

        signal_list->dirty = TRUE;
        gtk_widget_queue_draw(GTK_WIDGET(signal_list));
    }
}

static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event)
{
    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    GdkModifierType state = event->state & (GDK_SHIFT_MASK | GW_CONTROL_MASK);

    if (state == GDK_SHIFT_MASK) {
        if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) {
            // Shift + Up: Move cursor up one trace
            if (signal_list->cursor != NULL) {
                GwTrace *cursor = GivePrevTrace(signal_list->cursor);
                if (cursor != NULL) {
                    set_cursor(signal_list, cursor);
                }
            } else {
                set_cursor(signal_list, GLOBALS->traces.first);
            }
            return GDK_EVENT_STOP;
        } else if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) {
            // Shift + Down: Move cursor down one trace
            if (signal_list->cursor != NULL) {
                GwTrace *cursor = GiveNextTrace(signal_list->cursor);
                if (cursor != NULL) {
                    set_cursor(signal_list, cursor);
                }
            } else {
                set_cursor(signal_list, GLOBALS->traces.first);
            }
            return GDK_EVENT_STOP;
        }
    } else if (state == GW_CONTROL_MASK) {
        if (event->keyval == GDK_KEY_A) {
            // CTRL + a: Highlight all traces
            menu_dataformat_highlight_all(NULL, 0, NULL);

            signal_list->dirty = TRUE;
            gtk_widget_queue_draw(widget);
            return GDK_EVENT_STOP;
        }
    } else if (state == (GDK_SHIFT_MASK | GW_CONTROL_MASK)) {
        if (event->keyval == GDK_KEY_A) {
            // CTRL + SHIFT + A: UnHighlight all traces
            menu_dataformat_unhighlight_all(NULL, 0, NULL);

            signal_list->dirty = TRUE;
            gtk_widget_queue_draw(widget);
            return GDK_EVENT_STOP;
        }
    } else if (!state) {
        if (event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_KP_Left) {
            // Left: Find next edge to the left
            service_left_edge(NULL, NULL);
            return GDK_EVENT_STOP;
        } else if (event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_KP_Right) {
            // Right: Find next edge to the right
            service_right_edge(NULL, NULL);
            return GDK_EVENT_STOP;
        } else if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) {
            // Up: Scroll up by one trace
            scroll_up(signal_list, FALSE);
            return GDK_EVENT_STOP;
        } else if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) {
            // Down: Scroll down by one trace
            scroll_down(signal_list, FALSE);
            return GDK_EVENT_STOP;
        } else if (event->keyval == GDK_KEY_Page_Up || event->keyval == GDK_KEY_KP_Page_Up) {
            // Page Up: Scroll up by one page
            scroll_up(signal_list, TRUE);
            return GDK_EVENT_STOP;
        } else if (event->keyval == GDK_KEY_Page_Down || event->keyval == GDK_KEY_KP_Page_Down) {
            // Page Down: Scroll down by one page
            scroll_down(signal_list, TRUE);
            return GDK_EVENT_STOP;
        }
    }

    return GDK_EVENT_PROPAGATE;
}

static int y_to_drop_position(int y)
{
    // Shift the position of the drop targets up by one half row height to make
    // it easier to drop items above or below another row.
    int position = (y - GLOBALS->fontheight / 2) / GLOBALS->fontheight;

    // Limit position to the total number of traces.
    // Dropping an item on the empty space after the last appends the item at the end of the list.
    position = CLAMP(position, 0, GLOBALS->traces.visible);

    return position;
}

static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time)
{
    (void)x;

    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    GdkAtom target = gtk_drag_dest_find_target(widget, context, NULL);
    if (target == GDK_NONE) {
        gdk_drag_status(context, 0, time);

        signal_list->drop.highlight_position = -1;
        gtk_widget_queue_draw(widget);

        return GDK_EVENT_STOP;
    }

    // Default to move if a trace is dragged inside the signal list and control isn't pressed
    GdkDragAction action = GDK_ACTION_COPY;
    GtkWidget *source_widget = gtk_drag_get_source_widget(context);
    if (widget == source_widget && (gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE)) {
        action = GDK_ACTION_MOVE;
    }

    gdk_drag_status(context, action, time);

    int drop_position = y_to_drop_position(y);
    if (signal_list->drop.highlight_position != drop_position) {
        signal_list->drop.highlight_position = drop_position;
        gtk_widget_queue_draw(widget);
    }

    int height = gtk_widget_get_allocated_height(widget);
    signal_list->drop.in_scroll_area_top = FALSE;
    signal_list->drop.in_scroll_area_bottom = FALSE;

    if (height > 2 * SCROLL_AREA_HEIGHT) {
        if (y < SCROLL_AREA_HEIGHT + GLOBALS->fontheight) { // include header height
            signal_list->drop.in_scroll_area_top = TRUE;
        } else if (y >= height - SCROLL_AREA_HEIGHT) {
            signal_list->drop.in_scroll_area_bottom = TRUE;
        }
    }

    if (signal_list->drop.scroll_timeout_source == 0 &&
        (signal_list->drop.in_scroll_area_top || signal_list->drop.in_scroll_area_bottom)) {
        signal_list->drop.scroll_timeout_source =
            g_timeout_add(100, G_SOURCE_FUNC(scroll_timeout), signal_list);
    }

    return GDK_EVENT_STOP;
}

static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time)
{
    (void)context;
    (void)time;

    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    if (drop_reset(&signal_list->drop)) {
        gtk_widget_queue_draw(widget);
    }
}

static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time)
{
    (void)x;
    (void)y;

    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    GdkAtom target = gtk_drag_dest_find_target(widget, context, NULL);
    if (target != GDK_NONE) {
        signal_list->drop.action = gdk_drag_context_get_selected_action(context);

        gtk_drag_get_data(widget, context, target, time);
    } else {
        fprintf(stderr, "GTKWAVE | No valid DnD target found\n");
    }

    return TRUE;
}

static void drag_data_received(GtkWidget *widget,
                               GdkDragContext *context,
                               gint x,
                               gint y,
                               GtkSelectionData *data,
                               guint info,
                               guint time)
{
    (void)x;

    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    // recalculate the drop position (the highlight position has been cleared by drag_leave)
    int drop_position = y_to_drop_position(y);

    switch (info) {
        case WAVE_DRAG_INFO_SIGNAL_LIST:
            if (drop_position == 0) {
                if (signal_list->drop.action == GDK_ACTION_COPY) {
                    char *tcl =
                        emit_gtkwave_savefile_formatted_entries_in_tcl_list(GLOBALS->traces.first,
                                                                            TRUE);
                    ClearTraces();
                    process_tcl_list(tcl, FALSE);
                    free_2(tcl);
                }

                CutBuffer();
                PrependBuffer();
            } else {
                GwTrace *t = gw_signal_list_get_trace(signal_list, drop_position - 1);

                // Search upward for the first non selected trace.
                // CutBuffer would otherwise remove the trace that determines the
                // paste position if traces were dropped on any selected trace.
                while (t != NULL && IsSelected(t)) {
                    t = GivePrevTrace(t);
                }

                if (t != NULL) {
                    if (signal_list->drop.action == GDK_ACTION_MOVE) {
                        CutBuffer();

                        t->flags |= TR_HIGHLIGHT;
                        PasteBuffer();
                    } else {
                        char *tcl = emit_gtkwave_savefile_formatted_entries_in_tcl_list(
                            GLOBALS->traces.first,
                            TRUE);

                        ClearTraces();
                        t->flags |= TR_HIGHLIGHT;

                        process_tcl_list(tcl, FALSE);
                        free_2(tcl);
                    }
                }
            }
            break;

        case WAVE_DRAG_INFO_TCL:
            if (gtk_selection_data_get_length(data) > 0) {
                const char *tcl = (const char *)gtk_selection_data_get_data(data);

                ClearTraces();

                GwTrace *t;
                gboolean prepend;
                if (drop_position > 0) {
                    t = gw_signal_list_get_trace(signal_list, drop_position - 1);
                    prepend = FALSE;
                } else {
                    t = GLOBALS->traces.first;
                    prepend = TRUE;
                }

                if (t != NULL) {
                    t->flags |= TR_HIGHLIGHT;
                }

                process_tcl_list(tcl, prepend);
            }

            gtk_drag_finish(context, TRUE, FALSE, time);
            break;

        default:
            fprintf(stderr, "GTKWAVE | Invalid DnD info\n");
            break;
    }

    drop_reset(&signal_list->drop);
    redraw_signals_and_waves();
}

static void drag_data_get(GtkWidget *widget,
                          GdkDragContext *context,
                          GtkSelectionData *data,
                          guint info,
                          guint time)
{
    (void)widget;
    (void)context;
    (void)time;

    switch (info) {
        case WAVE_DRAG_INFO_SIGNAL_LIST:
            // Data isn't required for widget internal dragging.
            break;

        case WAVE_DRAG_INFO_TCL: {
            char *tcl =
                emit_gtkwave_savefile_formatted_entries_in_tcl_list(GLOBALS->traces.first, TRUE);
            if (tcl != NULL) {
                GdkAtom type = gdk_atom_intern_static_string(WAVE_DRAG_TARGET_TCL);
                gtk_selection_data_set(data, type, 8, (const guchar *)tcl, strlen(tcl));
                free_2(tcl);
            }
        } break;

        default:
            fprintf(stderr, "GTKWAVE | Invalid DnD info\n");
            break;
    }
}

static gboolean motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    if (signal_list->drag.pending) {
        if (gtk_drag_check_threshold(widget,
                                     signal_list->drag.start_x,
                                     signal_list->drag.start_y,
                                     event->x,
                                     event->y)) {
            gchar *target_signal_list = g_strdup(WAVE_DRAG_TARGET_SIGNAL_LIST);
            gchar *target_tcl = g_strdup(WAVE_DRAG_TARGET_TCL);

            GtkTargetEntry targets[] = {
                {
                    .target = target_signal_list,
                    .flags = GTK_TARGET_SAME_APP,
                    .info = WAVE_DRAG_INFO_SIGNAL_LIST,
                },
                {
                    .target = target_tcl,
                    .flags = 0,
                    .info = WAVE_DRAG_INFO_TCL,
                },
            };

            GtkTargetList *target_list = gtk_target_list_new(targets, G_N_ELEMENTS(targets));

            gtk_drag_begin_with_coordinates(widget,
                                            target_list,
                                            GDK_ACTION_MOVE | GDK_ACTION_COPY,
                                            GDK_BUTTON_PRIMARY,
                                            (GdkEvent *)event,
                                            signal_list->drag.start_x,
                                            signal_list->drag.start_y);

            gtk_target_list_unref(target_list);
            g_free(target_signal_list);
            g_free(target_tcl);

            signal_list->drag.pending = FALSE;
        }
    }

    GLOBALS->cached_mouseover_x = event->x;
    GLOBALS->cached_mouseover_y = event->y;

    return GDK_EVENT_STOP;
}

static void destroy(GtkWidget *widget)
{
    GwSignalList *signal_list = GW_SIGNAL_LIST(widget);

    if (signal_list->hadjustment != NULL) {
        g_object_unref(signal_list->hadjustment);
        signal_list->hadjustment = NULL;
    }

    if (signal_list->vadjustment != NULL) {
        g_object_unref(signal_list->vadjustment);
        signal_list->vadjustment = NULL;
    }
}

static void gw_signal_list_class_init(GwSignalListClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    widget_class->configure_event = configure_event;
    widget_class->draw = draw;
    widget_class->destroy = destroy;
    widget_class->button_press_event = button_press_event;
    widget_class->button_release_event = button_release_event;
    widget_class->motion_notify_event = motion_notify_event;
    widget_class->key_press_event = key_press_event;
    widget_class->scroll_event = scroll_event;
    widget_class->drag_motion = drag_motion;
    widget_class->drag_leave = drag_leave;
    widget_class->drag_drop = drag_drop;
    widget_class->drag_data_received = drag_data_received;
    widget_class->drag_data_get = drag_data_get;
}

static void gw_signal_list_init(GwSignalList *signal_list)
{
    GtkWidget *widget = GTK_WIDGET(signal_list);

    signal_list->dirty = TRUE;
    drop_reset(&signal_list->drop);

    gtk_widget_set_events(widget,
                          GDK_SCROLL_MASK | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK | GDK_FOCUS_CHANGE_MASK);

    gtk_widget_set_can_focus(widget, TRUE);

    gchar *target_signal_list = g_strdup(WAVE_DRAG_TARGET_SIGNAL_LIST);
    gchar *target_tcl = g_strdup(WAVE_DRAG_TARGET_TCL);

    GtkTargetEntry targets[] = {
        {
            .target = target_signal_list,
            .flags = GTK_TARGET_SAME_WIDGET,
            .info = WAVE_DRAG_INFO_SIGNAL_LIST,
        },
        {
            .target = target_tcl,
            .flags = 0,
            .info = WAVE_DRAG_INFO_TCL,
        },
    };

    gtk_drag_dest_set(widget, 0, targets, G_N_ELEMENTS(targets), GDK_ACTION_MOVE);

    g_free(target_signal_list);
    g_free(target_tcl);

    signal_list->hadjustment = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    signal_list->vadjustment = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
    g_object_ref(signal_list->hadjustment);
    g_object_ref(signal_list->vadjustment);
    g_signal_connect_swapped(signal_list->hadjustment,
                             "value-changed",
                             G_CALLBACK(gw_signal_list_force_redraw),
                             signal_list);
    g_signal_connect_swapped(signal_list->vadjustment,
                             "value-changed",
                             G_CALLBACK(gw_signal_list_force_redraw),
                             signal_list);
}

GtkAdjustment *gw_signal_list_get_hadjustment(GwSignalList *signal_list)
{
    g_return_val_if_fail(GW_IS_SIGNAL_LIST(signal_list), NULL);

    return signal_list->hadjustment;
}

GtkAdjustment *gw_signal_list_get_vadjustment(GwSignalList *signal_list)
{
    g_return_val_if_fail(GW_IS_SIGNAL_LIST(signal_list), NULL);

    return signal_list->vadjustment;
}

void gw_signal_list_force_redraw(GwSignalList *signal_list)
{
    g_return_if_fail(GW_IS_SIGNAL_LIST(signal_list));

    MaxSignalLength();

    signal_list->dirty = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(signal_list));
}

GtkWidget *gw_signal_list_new(void)
{
    return GTK_WIDGET(g_object_new(GW_TYPE_SIGNAL_LIST, NULL));
}
