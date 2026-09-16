#include <gtkwave.h>

static const GwColor BLACK = {.a = 1.0};
static const GwColor WHITE = {1.0, 1.0, 1.0, 1.0};

static void test_properties(void)
{
    GwColorTheme *theme = gw_color_theme_new();

    GwColor grid = {.r = 1.0};
    GwColor background = {.g = 1.0};

    g_object_set(theme, "waveform-grid", &grid, "waveform-background", &background, NULL);

    const GwWaveformColors *waveform_colors = gw_color_theme_get_waveform_colors(theme);
    g_assert_true(gw_color_equal(&grid, &waveform_colors->grid));
    g_assert_true(gw_color_equal(&background, &waveform_colors->background));

    GwColor *a = NULL;
    GwColor *b = NULL;
    g_object_get(theme, "waveform-grid", &a, "waveform-background", &b, NULL);

    g_assert_true(gw_color_equal(&grid, a));
    g_assert_true(gw_color_equal(&background, b));

    g_free(a);
    g_free(b);

    g_object_unref(theme);
}

static void test_defaults(void)
{
    GwColorTheme *theme = gw_color_theme_new();

    const GwSignalListColors *colors = gw_color_theme_get_signal_list_colors(theme);
    g_assert_true(gw_color_equal(&colors->black, &BLACK));
    g_assert_true(gw_color_equal(&colors->white, &WHITE));

    g_object_unref(theme);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/color-theme/properties", test_properties);
    g_test_add_func("/color-theme/defaults", test_defaults);

    return g_test_run();
}