#include <config.h>
#include "gw-mouseover.h"
#include "analyzer.h"

struct _GwMouseover
{
    GtkWindow parent_instance;

    GtkLabel *name_label;
    GtkLabel *flags_label;
    GtkLabel *value_label;
};

G_DEFINE_TYPE(GwMouseover, gw_mouseover, GTK_TYPE_WINDOW)

static gchar *trace_flags_to_string(GwTrace *t)
{
    TraceFlagsType flags = t->flags;

    GString *str = g_string_new(NULL);

    if ((flags & TR_SIGNED) != 0) {
        g_string_append_c(str, '+');
    }

    if ((flags & TR_HEX) != 0) {
        g_string_append_c(str, 'X');
    } else if ((flags & TR_ASCII) != 0) {
        g_string_append_c(str, 'A');
    } else if ((flags & TR_DEC) != 0) {
        g_string_append_c(str, 'D');
    } else if ((flags & TR_BIN) != 0) {
        g_string_append_c(str, 'B');
    } else if ((flags & TR_OCT) != 0) {
        g_string_append_c(str, 'O');
    }

    if ((flags & TR_RJUSTIFY) != 0) {
        g_string_append_c(str, 'J');
    }

    if ((flags & TR_INVERT) != 0) {
        g_string_append_c(str, '~');
    }

    if ((flags & TR_REVERSE) != 0) {
        g_string_append_c(str, 'V');
    }

    if ((flags & (TR_ANALOG_STEP | TR_ANALOG_INTERPOLATED)) ==
        (TR_ANALOG_STEP | TR_ANALOG_INTERPOLATED)) {
        g_string_append_c(str, '*');
    } else if ((flags & TR_ANALOG_STEP) != 0) {
        g_string_append_c(str, 'S');
    } else if ((flags & TR_ANALOG_INTERPOLATED) != 0) {
        g_string_append_c(str, 'I');
    }

    if ((flags & TR_REAL) != 0) {
        g_string_append_c(str, 'R');
    }

    if ((flags & TR_REAL2BITS) != 0) {
        g_string_append_c(str, 'r');
    }

    if ((flags & TR_ZEROFILL) != 0) {
        g_string_append_c(str, '0');
    } else if ((flags & TR_ONEFILL) != 0) {
        g_string_append_c(str, '1');
    }

    if ((flags & TR_BINGRAY) != 0) {
        g_string_append_c(str, 'G');
    }

    if ((flags & TR_GRAYBIN) != 0) {
        g_string_append_c(str, 'g');
    }

    if ((flags & TR_FTRANSLATED) != 0) {
        g_string_append_c(str, 'F');
    }

    if ((flags & TR_PTRANSLATED) != 0) {
        g_string_append_c(str, 'P');
    }

    if ((flags & TR_TTRANSLATED) != 0) {
        g_string_append_c(str, 'T');
    }

    if ((flags & TR_FFO) != 0) {
        g_string_append_c(str, 'f');
    }

    if ((flags & TR_POPCNT) != 0) {
        g_string_append_c(str, 'p');
    }

    if ((flags & TR_FPDECSHIFT) != 0) {
        g_string_append_printf(str, "s(%d)", t->t_fpdecshift);
    }

    return g_string_free(str, FALSE);
}

static GwNode *trace_first_node(GwTrace *t)
{
    if (t->vector) {
        if (t->n.vec != NULL && t->n.vec->bits != NULL && t->n.vec->bits->nnbits > 0) {
            return t->n.vec->bits->nodes[0];
        }
        return NULL;
    }

    return t->n.nd;
}

/* For an Extended VCD ($dumpports) port, summarize the EVCD-specific
 * information that the four-state waveform cannot show: the inferred pin
 * direction and the drive strength (IEEE 1364 18.4.3).  Returns NULL for
 * signals that are not EVCD ports. */
static gchar *evcd_info_to_string(GwTrace *t)
{
    GwNode *nd = trace_first_node(t);
    if (nd == NULL || nd->vartype != GW_VAR_TYPE_VCD_PORT) {
        return NULL;
    }

    GString *str = g_string_new(NULL);

    switch (nd->vardir) {
        case GW_VAR_DIR_IN:
            g_string_append(str, "in");
            break;
        case GW_VAR_DIR_OUT:
            g_string_append(str, "out");
            break;
        case GW_VAR_DIR_INOUT:
            g_string_append(str, "inout");
            break;
        default:
            g_string_append(str, "port");
            break;
    }

    if (nd->evcd_strength_valid) {
        const gchar *s0 = gw_evcd_strength_name(nd->evcd_strength0);
        const gchar *s1 = gw_evcd_strength_name(nd->evcd_strength1);
        if (s0 != NULL && s1 != NULL) {
            if (g_strcmp0(s0, s1) == 0) {
                g_string_append_printf(str, ", %s", s0);
            } else {
                g_string_append_printf(str, ", strength0=%s strength1=%s", s0, s1);
            }
        }
    }

    return g_string_free(str, FALSE);
}

static gchar *flags_with_evcd(GwTrace *trace, const gchar *base)
{
    gchar *evcd = evcd_info_to_string(trace);
    gchar *result;

    if (evcd != NULL) {
        result = g_strdup_printf("%s | EVCD: %s", base, evcd);
    } else {
        result = g_strdup(base);
    }

    g_free(evcd);
    return result;
}

static gchar *local_trace_asciival(GwTrace *t, GwTime tim)
{
    if (t->name == NULL) {
        return NULL;
    }

    if ((tim != -1) && (!(t->flags & TR_EXCLUDE))) {
        GLOBALS->shift_timebase = t->shift;

        if (t->vector) {
            GwVectorEnt *v = bsearch_vector(t->n.vec, tim - t->shift);
            return convert_ascii(t, v);
        } else {
            char *str;
            GwHistEnt *h_ptr;
            if ((h_ptr = bsearch_node(t->n.nd, tim - t->shift))) {
                if (!t->n.nd->extvals) {
                    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
                    GwTime primary_pos = gw_marker_get_position(primary_marker);

                    unsigned char h_val = h_ptr->v.h_val;
                    if (t->n.nd->vartype == GW_VAR_TYPE_VCD_EVENT) {
                        h_val = (h_ptr->time >= GLOBALS->tims.first) &&
                                        ((primary_pos - GLOBALS->shift_timebase) == h_ptr->time)
                                    ? GW_BIT_1
                                    : GW_BIT_0; /* generate impulse */
                    }

                    if (t->flags & TR_INVERT) {
                        h_val = gw_bit_invert(h_val);
                    }

                    str = (char *)calloc_2(1, 2 * sizeof(char));
                    str[0] = gw_bit_to_char(h_val);

                    return str;
                } else {
                    if (h_ptr->flags & GW_HIST_ENT_FLAG_REAL) {
                        if (!(h_ptr->flags & GW_HIST_ENT_FLAG_STRING)) {
                            str = convert_ascii_real(t, &h_ptr->v.h_double);
                        } else {
                            str = convert_ascii_string((char *)h_ptr->v.h_vector);
                        }
                    } else {
                        str = convert_ascii_vec(t, h_ptr->v.h_vector);
                    }

                    return str;
                }
            }
        }
    }

    return NULL;
}

static char *get_fullname(GwTrace *t)
{
    char *s = NULL;
    if (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH))) {
        if (t->vector == TRUE) {
            s = strdup_2(t->n.vec->bvname);
        } else {
            if (!HasAlias(t)) {
                s = t->n.nd->nname;
            }
        }
    }

    return (s);
}

static void gw_mouseover_class_init(GwMouseoverClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class,
                                                "/io/github/gtkwave/GTKWave/ui/gw-mouseover.ui");
    gtk_widget_class_bind_template_child(widget_class, GwMouseover, name_label);
    gtk_widget_class_bind_template_child(widget_class, GwMouseover, flags_label);
    gtk_widget_class_bind_template_child(widget_class, GwMouseover, value_label);

    gtk_widget_class_set_css_name(widget_class, "GwMouseover");
}

static void gw_mouseover_init(GwMouseover *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));
}

GtkWidget *gw_mouseover_new(void)
{
    return g_object_new(GW_TYPE_MOUSEOVER, "type", GTK_WINDOW_POPUP, NULL);
}

static void gw_mouseover_update_clipboard(GwMouseover *self, GwTrace *trace)
{
    GdkDisplay *g = gdk_display_get_default();

    if (trace->name != NULL) {
        // middle mouse button
        GtkClipboard *clip = gtk_clipboard_get_for_display(g, GDK_SELECTION_PRIMARY);
        gtk_clipboard_set_text(clip, trace->name, strlen(trace->name));
    }

    const gchar *value = gtk_label_get_text(self->value_label);
    if (value != NULL) {
        // ctrl-c/ctrl-v
        GtkClipboard *clip = gtk_clipboard_get_for_display(g, GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clip, value, strlen(value));
    }
}

static void gw_mouseover_update_common(GwMouseover *self, GwTrace *trace, GwTime t)
{
    // Set width-chars to prevent that short names are ellipsized.
    guint name_len = trace->name != NULL ? strlen(trace->name) : 0;
    name_len = MIN(name_len, gtk_label_get_max_width_chars(self->name_label));
    gtk_label_set_width_chars(self->name_label, name_len);

    char *asciival = local_trace_asciival(trace, t);
    gtk_label_set_text(GTK_LABEL(self->value_label), asciival);
    free_2(asciival);

    if (GLOBALS->clipboard_mouseover) {
        gw_mouseover_update_clipboard(self, trace);
    }
}

void gw_mouseover_update(GwMouseover *self, GwTrace *trace, GwTime t)
{
    g_return_if_fail(GW_IS_MOUSEOVER(self));
    g_return_if_fail(trace != NULL);

    gw_mouseover_update_common(self, trace, t);
    gtk_label_set_text(self->name_label, trace->name);

    gchar *flags = trace_flags_to_string(trace);
    gchar *flags_with_dir = flags_with_evcd(trace, flags);
    gchar *flags_text = g_strdup_printf(" | %s", flags_with_dir);
    gtk_label_set_text(GTK_LABEL(self->flags_label), flags_text);
    g_free(flags_text);
    g_free(flags_with_dir);
    g_free(flags);
}

void gw_mouseover_update_signal_list(GwMouseover *self, GwTrace *trace, GwTime t)
{
    g_return_if_fail(GW_IS_MOUSEOVER(self));
    g_return_if_fail(trace != NULL);

    gw_mouseover_update_common(self, trace, t);
    gtk_label_set_text(self->name_label, get_fullname(trace));

    gchar *flags = trace_flags_to_string(trace);
    gchar *flags_text;
    if (trace->vector) {
        flags_text = g_strdup_printf(" | %s", flags);
    } else {
        gint vartype = trace->n.nd->vartype;
        if (vartype < 0 || vartype > GW_VAR_TYPE_MAX) {
            vartype = 0;
        }
        const gchar *vartype_str = gw_var_type_to_string(vartype);
        flags_text = g_strdup_printf(" | %s:%s", flags, vartype_str);
    }

    gchar *evcd = evcd_info_to_string(trace);
    if (evcd != NULL) {
        gchar *with_evcd = g_strdup_printf("%s | EVCD: %s", flags_text, evcd);
        g_free(flags_text);
        flags_text = with_evcd;
        g_free(evcd);
    }

    gtk_label_set_text(GTK_LABEL(self->flags_label), flags_text);
    g_free(flags_text);
    g_free(flags);
}
