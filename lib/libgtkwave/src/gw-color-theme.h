#pragma once

#include <glib-object.h>
#include "gw-color.h"

G_BEGIN_DECLS

typedef struct
{
    GwColor background;
    GwColor grid;
    GwColor grid2; // TODO: name

    GwColor marker_primary;
    GwColor marker_baseline;
    GwColor marker_named;

    GwColor value_text;

    GwColor stroke_0;
    GwColor stroke_x;
    GwColor stroke_z;
    GwColor stroke_1;
    GwColor stroke_h;
    GwColor stroke_u;
    GwColor stroke_w;
    GwColor stroke_l;
    GwColor stroke_dash;
    GwColor stroke_transition;
    GwColor stroke_vector;

    GwColor fill_x;
    GwColor fill_1;
    GwColor fill_h;
    GwColor fill_u;
    GwColor fill_w;
    GwColor fill_dash;

    GwColor timebar_background;
    GwColor timebar_text;
} GwWaveformColors;

#define GW_TYPE_WAVEFORM_COLORS (gw_waveform_colors_get_type())
GType gw_waveform_colors_get_type(void);

// TODO: names
typedef struct
{
    GwColor white;
    GwColor black;
    GwColor ltgray;
    GwColor normal;
    GwColor mdgray;
    GwColor dkgray;
    GwColor dkblue;
    GwColor brkred;
    GwColor ltblue;
    GwColor gmstrd;
} GwSignalListColors;

#define GW_TYPE_SIGNAL_LIST_COLORS (gw_signal_list_colors_get_type())
GType gw_signal_list_colors_get_type(void);

#define GW_TYPE_COLOR_THEME (gw_color_theme_get_type())
G_DECLARE_FINAL_TYPE(GwColorTheme, gw_color_theme, GW, COLOR_THEME, GObject)

GwColorTheme *gw_color_theme_new(void);

GwWaveformColors *gw_color_theme_get_waveform_colors(GwColorTheme *self);
GwSignalListColors *gw_color_theme_get_signal_list_colors(GwColorTheme *self);

GwWaveformColors *gw_waveform_colors_get_rainbow_variant(GwWaveformColors *self,
                                                         GwRainbowColor rainbow_color,
                                                         gboolean keep_xz);
GwWaveformColors *gw_waveform_colors_new_black_and_white(void);
GwSignalListColors *gw_signal_list_colors_new_black_and_white(void);

G_END_DECLS
