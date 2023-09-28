#include "gw-color-theme.h"

static GwWaveformColors *gw_waveform_colors_copy(GwWaveformColors *self);
static GwSignalListColors *gw_signal_list_colors_copy(GwSignalListColors *self);

G_DEFINE_BOXED_TYPE(GwWaveformColors, gw_waveform_colors, gw_waveform_colors_copy, g_free)
G_DEFINE_BOXED_TYPE(GwSignalListColors, gw_signal_list_colors, gw_signal_list_colors_copy, g_free)

struct _GwColorTheme
{
    GObject parent_instance;

    GwWaveformColors waveform_colors;
    GwSignalListColors signal_list_colors;
};

G_DEFINE_TYPE(GwColorTheme, gw_color_theme, G_TYPE_OBJECT)

enum
{
    PROP_WAVEFORM_BACKGROUND = 1,
    PROP_WAVEFORM_GRID,
    PROP_WAVEFORM_GRID2,
    PROP_WAVEFORM_MARKER_PRIMARY,
    PROP_WAVEFORM_MARKER_BASELINE,
    PROP_WAVEFORM_MARKER_NAMED,
    PROP_WAVEFORM_VALUE_TEXT,

    PROP_STROKE_0,
    PROP_STROKE_X,
    PROP_STROKE_Z,
    PROP_STROKE_1,
    PROP_STROKE_H,
    PROP_STROKE_U,
    PROP_STROKE_W,
    PROP_STROKE_L,
    PROP_STROKE_DASH,
    PROP_STROKE_TRANSITION,
    PROP_STROKE_VECTOR,

    PROP_FILL_X,
    PROP_FILL_1,
    PROP_FILL_H,
    PROP_FILL_U,
    PROP_FILL_W,
    PROP_FILL_DASH,

    PROP_TIMEBAR_BACKGROUND,
    PROP_TIMEBAR_TEXT,

    PROP_SIGNAL_LIST_WHITE,
    PROP_SIGNAL_LIST_BLACK,
    PROP_SIGNAL_LIST_LTGRAY,
    PROP_SIGNAL_LIST_NORMAL,
    PROP_SIGNAL_LIST_MDGRAY,
    PROP_SIGNAL_LIST_DKGRAY,
    PROP_SIGNAL_LIST_DKBLUE,
    PROP_SIGNAL_LIST_BRKRED,
    PROP_SIGNAL_LIST_LTBLUE,
    PROP_SIGNAL_LIST_GMSTRD,

    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static GwColor *gw_color_theme_property_to_color(GwColorTheme *self, guint property_id)
{
    // clang-format off
    switch (property_id) {
        case PROP_WAVEFORM_BACKGROUND:        return &self->waveform_colors.background;
        case PROP_WAVEFORM_GRID:              return &self->waveform_colors.grid;
        case PROP_WAVEFORM_GRID2:             return &self->waveform_colors.grid2;
        case PROP_WAVEFORM_MARKER_PRIMARY:    return &self->waveform_colors.marker_primary;
        case PROP_WAVEFORM_MARKER_BASELINE:   return &self->waveform_colors.marker_baseline;
        case PROP_WAVEFORM_MARKER_NAMED:      return &self->waveform_colors.marker_named;
        case PROP_WAVEFORM_VALUE_TEXT:        return &self->waveform_colors.value_text;

        case PROP_STROKE_0:          return &self->waveform_colors.stroke_0;
        case PROP_STROKE_X:          return &self->waveform_colors.stroke_x;
        case PROP_STROKE_Z:          return &self->waveform_colors.stroke_z;
        case PROP_STROKE_1:          return &self->waveform_colors.stroke_1;
        case PROP_STROKE_H:          return &self->waveform_colors.stroke_h;
        case PROP_STROKE_U:          return &self->waveform_colors.stroke_u;
        case PROP_STROKE_W:          return &self->waveform_colors.stroke_w;
        case PROP_STROKE_L:          return &self->waveform_colors.stroke_l;
        case PROP_STROKE_DASH:       return &self->waveform_colors.stroke_dash;
        case PROP_STROKE_TRANSITION: return &self->waveform_colors.stroke_transition;
        case PROP_STROKE_VECTOR:     return &self->waveform_colors.stroke_vector;

        case PROP_FILL_X:    return &self->waveform_colors.fill_x;
        case PROP_FILL_1:    return &self->waveform_colors.fill_1;
        case PROP_FILL_H:    return &self->waveform_colors.fill_h;
        case PROP_FILL_U:    return &self->waveform_colors.fill_u;
        case PROP_FILL_W:    return &self->waveform_colors.fill_w;
        case PROP_FILL_DASH: return &self->waveform_colors.fill_dash;

        case PROP_TIMEBAR_BACKGROUND: return &self->waveform_colors.timebar_background;
        case PROP_TIMEBAR_TEXT:       return &self->waveform_colors.timebar_text;

        case PROP_SIGNAL_LIST_WHITE:  return &self->signal_list_colors.white;
        case PROP_SIGNAL_LIST_BLACK:  return &self->signal_list_colors.black;
        case PROP_SIGNAL_LIST_LTGRAY: return &self->signal_list_colors.ltgray;
        case PROP_SIGNAL_LIST_NORMAL: return &self->signal_list_colors.normal;
        case PROP_SIGNAL_LIST_MDGRAY: return &self->signal_list_colors.mdgray;
        case PROP_SIGNAL_LIST_DKGRAY: return &self->signal_list_colors.dkgray;
        case PROP_SIGNAL_LIST_DKBLUE: return &self->signal_list_colors.dkblue;
        case PROP_SIGNAL_LIST_BRKRED: return &self->signal_list_colors.brkred;
        case PROP_SIGNAL_LIST_LTBLUE: return &self->signal_list_colors.ltblue;
        case PROP_SIGNAL_LIST_GMSTRD: return &self->signal_list_colors.gmstrd;

        default:
            return NULL;
    }
    // clang-format on
}

static void gw_color_theme_set_property(GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
    GwColorTheme *self = GW_COLOR_THEME(object);

    GwColor *color = gw_color_theme_property_to_color(self, property_id);
    if (color == NULL) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        return;
    }

    *color = *(GwColor *)g_value_get_boxed(value);
}

static void gw_color_theme_get_property(GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
    GwColorTheme *self = GW_COLOR_THEME(object);

    GwColor *color = gw_color_theme_property_to_color(self, property_id);
    if (color == NULL) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        return;
    }

    g_value_set_boxed(value, color);
}

static void init_prop(guint property_id, const gchar *name)
{
    // TODO: add better nicks and blurbs
    properties[property_id] = g_param_spec_boxed(name,
                                                 name,
                                                 name,
                                                 GW_TYPE_COLOR,
                                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
}

static void gw_color_theme_class_init(GwColorThemeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = gw_color_theme_set_property;
    object_class->get_property = gw_color_theme_get_property;

    init_prop(PROP_WAVEFORM_BACKGROUND, "waveform-background");
    init_prop(PROP_WAVEFORM_GRID, "waveform-grid");
    init_prop(PROP_WAVEFORM_GRID2, "waveform-grid2");
    init_prop(PROP_WAVEFORM_MARKER_PRIMARY, "waveform-marker-primary");
    init_prop(PROP_WAVEFORM_MARKER_BASELINE, "waveform-marker-baseline");
    init_prop(PROP_WAVEFORM_MARKER_NAMED, "waveform-marker-named");
    init_prop(PROP_WAVEFORM_VALUE_TEXT, "waveform-value-text");

    init_prop(PROP_STROKE_0, "stroke-0");
    init_prop(PROP_STROKE_X, "stroke-x");
    init_prop(PROP_STROKE_Z, "stroke-z");
    init_prop(PROP_STROKE_1, "stroke-1");
    init_prop(PROP_STROKE_H, "stroke-h");
    init_prop(PROP_STROKE_U, "stroke-u");
    init_prop(PROP_STROKE_W, "stroke-w");
    init_prop(PROP_STROKE_L, "stroke-l");
    init_prop(PROP_STROKE_DASH, "stroke-dash");
    init_prop(PROP_STROKE_TRANSITION, "stroke-transition");
    init_prop(PROP_STROKE_VECTOR, "stroke-vector");

    init_prop(PROP_FILL_X, "fill-x");
    init_prop(PROP_FILL_1, "fill-1");
    init_prop(PROP_FILL_H, "fill-h");
    init_prop(PROP_FILL_U, "fill-u");
    init_prop(PROP_FILL_W, "fill-w");
    init_prop(PROP_FILL_DASH, "fill-dash");

    init_prop(PROP_TIMEBAR_BACKGROUND, "timebar-background");
    init_prop(PROP_TIMEBAR_TEXT, "timebar-text");

    init_prop(PROP_SIGNAL_LIST_WHITE, "signal-list-white");
    init_prop(PROP_SIGNAL_LIST_BLACK, "signal-list-black");
    init_prop(PROP_SIGNAL_LIST_LTGRAY, "signal-list-ltgray");
    init_prop(PROP_SIGNAL_LIST_NORMAL, "signal-list-normal");
    init_prop(PROP_SIGNAL_LIST_MDGRAY, "signal-list-mdgray");
    init_prop(PROP_SIGNAL_LIST_DKGRAY, "signal-list-dkgray");
    init_prop(PROP_SIGNAL_LIST_DKBLUE, "signal-list-dkblue");
    init_prop(PROP_SIGNAL_LIST_BRKRED, "signal-list-brkred");
    init_prop(PROP_SIGNAL_LIST_LTBLUE, "signal-list-ltblue");
    init_prop(PROP_SIGNAL_LIST_GMSTRD, "signal-list-gmstrd");

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_waveform_colors_init(GwWaveformColors *self)
{
    gw_color_init_from_hex(&self->background, "000000"); /* black */
    gw_color_init_from_hex(&self->grid, "202070"); /* dark dark blue */
    gw_color_init_from_hex(&self->grid2, "6a5acd"); /* slate blue */
    gw_color_init_from_hex(&self->value_text, "ffffff"); /* white */

    gw_color_init_from_hex(&self->marker_primary, "ff8080"); /* pink */
    gw_color_init_from_hex(&self->marker_baseline, "ffffff"); /* white */
    gw_color_init_from_hex(&self->marker_named, "ffff80"); /* light yellow */

    gw_color_init_from_hex(&self->stroke_h, "79f6f2"); /* light light blue */
    gw_color_init_from_hex(&self->stroke_l, "5dbebb"); /* light blue */
    gw_color_init_from_hex(&self->stroke_1, "00ff00"); /* green */
    gw_color_init_from_hex(&self->stroke_0, "008000"); /* dark green */
    gw_color_init_from_hex(&self->stroke_z, "c0c000"); /* mustard */
    gw_color_init_from_hex(&self->stroke_x, "ff0000"); /* red */
    gw_color_init_from_hex(&self->stroke_u, "cc0000"); /* brick */
    gw_color_init_from_hex(&self->stroke_w, "79f6f2"); /* light light blue */
    gw_color_init_from_hex(&self->stroke_dash, "edf508"); /* yellow */
    gw_color_init_from_hex(&self->stroke_transition, "00c000"); /* medium green */
    gw_color_init_from_hex(&self->stroke_vector, "00ff00"); /* green */

    gw_color_init_from_hex(&self->fill_h, "4ca09d"); /* dark dark blue */
    gw_color_init_from_hex(&self->fill_1, "004d00"); /* dark dark green */
    gw_color_init_from_hex(&self->fill_x, "400000"); /* dark maroon */
    gw_color_init_from_hex(&self->fill_u, "200000"); /* dark maroon */
    gw_color_init_from_hex(&self->fill_w, "3f817f"); /* dark blue-green */
    gw_color_init_from_hex(&self->fill_dash, "7d8104"); /* green mustard */

    gw_color_init_from_hex(&self->timebar_text, "ffffff"); /* white */
    gw_color_init_from_hex(&self->timebar_background, "000000"); /* black */
}

static void gw_signal_list_colors_init(GwSignalListColors *self)
{
    gw_color_init_from_hex(&self->white, "ffffff"); /* white */
    gw_color_init_from_hex(&self->black, "000000"); /* black */
    gw_color_init_from_hex(&self->ltgray, "f5f5f5");
    gw_color_init_from_hex(&self->normal, "e6e6e6");
    gw_color_init_from_hex(&self->mdgray, "cccccc");
    gw_color_init_from_hex(&self->dkgray, "aaaaaa");
    gw_color_init_from_hex(&self->dkblue, "4464ac");
    gw_color_init_from_hex(&self->brkred, "cc0000"); /* brick */
    gw_color_init_from_hex(&self->ltblue, "5dbebb"); /* light blue    */
    gw_color_init_from_hex(&self->gmstrd, "7d8104"); /* green mustard */
}

static void gw_color_theme_init(GwColorTheme *self)
{
    gw_waveform_colors_init(&self->waveform_colors);
    gw_signal_list_colors_init(&self->signal_list_colors);
}

GwColorTheme *gw_color_theme_new(void)
{
    return g_object_new(GW_TYPE_COLOR_THEME, NULL);
}

GwWaveformColors *gw_color_theme_get_waveform_colors(GwColorTheme *self)
{
    g_return_val_if_fail(GW_IS_COLOR_THEME(self), NULL);

    return &self->waveform_colors;
}

GwSignalListColors *gw_color_theme_get_signal_list_colors(GwColorTheme *self)
{
    g_return_val_if_fail(GW_IS_COLOR_THEME(self), NULL);

    return &self->signal_list_colors;
}

static GwWaveformColors *gw_waveform_colors_copy(GwWaveformColors *self)
{
    GwWaveformColors *copy = g_new(GwWaveformColors, 1);
    *copy = *self;

    return copy;
}

static GwSignalListColors *gw_signal_list_colors_copy(GwSignalListColors *self)
{
    GwSignalListColors *copy = g_new(GwSignalListColors, 1);
    *copy = *self;

    return copy;
}

GwWaveformColors *gw_waveform_colors_get_rainbow_variant(GwWaveformColors *self,
                                                         GwRainbowColor rainbow_color,
                                                         gboolean keep_xz)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(rainbow_color < GW_NUM_RAINBOW_COLORS, NULL);

    GwColor stroke = *gw_rainbow_color_to_color(rainbow_color);
    GwColor fill = {stroke.r / 2.0, stroke.g / 2.0, stroke.b / 2.0, 1.0};

    GwWaveformColors *colors = g_new(GwWaveformColors, 1);
    *colors = *self;

    colors->stroke_0 = stroke;
    colors->stroke_1 = stroke;
    colors->stroke_h = stroke;
    colors->stroke_l = stroke;
    colors->stroke_transition = stroke;
    colors->stroke_vector = stroke;

    if (!keep_xz) {
        colors->stroke_dash = stroke;
        colors->stroke_u = stroke;
        colors->stroke_w = stroke;
        colors->stroke_x = stroke;
        colors->stroke_z = stroke;

        colors->fill_1 = fill;
        colors->fill_dash = fill;
        colors->fill_h = fill;
        colors->fill_u = fill;
        colors->fill_w = fill;
        colors->fill_x = fill;
    }

    return colors;
}

GwWaveformColors *gw_waveform_colors_new_black_and_white(void)
{
    GwWaveformColors *self = g_new0(GwWaveformColors, 1);

    self->background = GW_COLOR_WHITE;
    self->grid = GW_COLOR_BLACK;
    self->grid2 = GW_COLOR_BLACK;

    self->marker_primary = GW_COLOR_BLACK;
    self->marker_baseline = GW_COLOR_BLACK;
    self->marker_named = GW_COLOR_WHITE;

    self->value_text = GW_COLOR_BLACK;

    self->stroke_0 = GW_COLOR_BLACK;
    self->stroke_x = GW_COLOR_BLACK;
    self->stroke_z = GW_COLOR_BLACK;
    self->stroke_1 = GW_COLOR_BLACK;
    self->stroke_h = GW_COLOR_BLACK;
    self->stroke_u = GW_COLOR_BLACK;
    self->stroke_w = GW_COLOR_BLACK;
    self->stroke_l = GW_COLOR_BLACK;
    self->stroke_dash = GW_COLOR_BLACK;
    self->stroke_transition = GW_COLOR_BLACK;
    self->stroke_vector = GW_COLOR_BLACK;

    self->fill_x = GW_COLOR_BLACK;
    self->fill_1 = GW_COLOR_BLACK;
    self->fill_h = GW_COLOR_BLACK;
    self->fill_u = GW_COLOR_BLACK;
    self->fill_w = GW_COLOR_BLACK;
    self->fill_dash = GW_COLOR_BLACK;

    self->timebar_background = GW_COLOR_WHITE;
    self->timebar_text = GW_COLOR_BLACK;

    return self;
}

GwSignalListColors *gw_signal_list_colors_new_black_and_white(void)
{
    GwSignalListColors *self = g_new0(GwSignalListColors, 1);

    self->white = GW_COLOR_WHITE;
    self->black = GW_COLOR_BLACK;
    self->ltgray = GW_COLOR_WHITE;
    self->normal = GW_COLOR_WHITE;
    self->mdgray = GW_COLOR_WHITE;
    self->dkgray = GW_COLOR_WHITE;
    self->dkblue = GW_COLOR_BLACK;
    self->brkred = GW_COLOR_BLACK;
    self->ltblue = GW_COLOR_BLACK;
    self->gmstrd = GW_COLOR_BLACK;

    return self;
}
