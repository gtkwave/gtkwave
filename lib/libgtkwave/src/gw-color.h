#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GW_TYPE_COLOR (gw_color_get_type())
GType gw_color_get_type(void);

typedef struct
{
    gdouble r;
    gdouble g;
    gdouble b;
    gdouble a;
} GwColor;

gboolean gw_color_equal(gconstpointer a, gconstpointer b);

gboolean gw_color_init_from_x11_name(GwColor *self, const gchar *color_name);
gboolean gw_color_init_from_hex(GwColor *self, const gchar *hex);
gchar *gw_color_to_hex(const GwColor *self);

extern const GwColor GW_COLOR_WHITE;
extern const GwColor GW_COLOR_BLACK;

typedef enum
{
    GW_RAINBOW_COLOR_RED,
    GW_RAINBOW_COLOR_ORANGE,
    GW_RAINBOW_COLOR_YELLOW,
    GW_RAINBOW_COLOR_LIME,
    GW_RAINBOW_COLOR_BLUE,
    GW_RAINBOW_COLOR_INDIGO,
    GW_RAINBOW_COLOR_VIOLET,
    GW_RAINBOW_COLOR_WHITE,
    GW_RAINBOW_COLOR_SILVER,
    GW_RAINBOW_COLOR_GRAY,
    GW_RAINBOW_COLOR_LGRAY,
    GW_RAINBOW_COLOR_LCYAN,
    GW_RAINBOW_COLOR_AQUA,
    GW_RAINBOW_COLOR_TEAL,
    GW_RAINBOW_COLOR_DBLUE,
    GW_RAINBOW_COLOR_SGREEN,
    GW_RAINBOW_COLOR_GREEN,
    GW_RAINBOW_COLOR_OLIVE,
    GW_RAINBOW_COLOR_KHAKI,
    GW_RAINBOW_COLOR_GOLD,
    GW_RAINBOW_COLOR_CHOCOLATE,
    GW_RAINBOW_COLOR_BROWN,
    GW_RAINBOW_COLOR_PINK,
    GW_RAINBOW_COLOR_MAGENTA,
    GW_RAINBOW_COLOR_LPURPLE,
    GW_RAINBOW_COLOR_PURPLE,
    GW_RAINBOW_COLOR_MAROON,
    GW_NUM_RAINBOW_COLORS,
} GwRainbowColor;

const GwColor *gw_rainbow_color_to_color(GwRainbowColor self);

G_END_DECLS
