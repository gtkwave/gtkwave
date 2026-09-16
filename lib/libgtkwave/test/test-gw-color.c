#include <gtkwave.h>

static const GwColor BLACK = {.a = 1.0};
static const GwColor RED = {.r = 1.0, .a = 1.0};
static const GwColor GREEN = {.g = 1.0, .a = 1.0};
static const GwColor BLUE = {.b = 1.0, .a = 1.0};

static void test_init_from_x11_name(void)
{
    GwColor color;

    g_assert_true(gw_color_init_from_x11_name(&color, "red"));
    g_assert_true(gw_color_equal(&color, &RED));

    // First color in the color list.
    g_assert_true(gw_color_init_from_x11_name(&color, "alice blue"));
    g_assert_true(color.r == 240 / 255.0 && color.g == 248 / 255.0 && color.b == 255 / 255.0 &&
                  color.a == 1.0);

    // Last color in the color list.
    g_assert_true(gw_color_init_from_x11_name(&color, "YellowGreen"));
    g_assert_true(color.r == 154 / 255.0 && color.g == 205 / 255.0 && color.b == 50 / 255.0 &&
                  color.a == 1.0);

    // Colors should be matched case insensitive.
    g_assert_true(gw_color_init_from_x11_name(&color, "AlIcE BlUe"));
    g_assert_true(color.r == 240 / 255.0 && color.g == 248 / 255.0 && color.b == 255 / 255.0 &&
                  color.a == 1.0);

    // Invalid color name.
    g_assert_false(gw_color_init_from_x11_name(&color, "DoesntExist"));
}

static void test_init_from_hex(void)
{
    GwColor color;

    // Valid colors.

    g_assert_true(gw_color_init_from_hex(&color, "000000"));
    g_assert_true(gw_color_equal(&color, &BLACK));

    g_assert_true(gw_color_init_from_hex(&color, "FF0000"));
    g_assert_true(gw_color_equal(&color, &RED));

    g_assert_true(gw_color_init_from_hex(&color, "00FF00"));
    g_assert_true(gw_color_equal(&color, &GREEN));

    g_assert_true(gw_color_init_from_hex(&color, "0000FF"));
    g_assert_true(gw_color_equal(&color, &BLUE));

    // Invalid colors.

    g_assert_false(gw_color_init_from_hex(&color, "0000"));
    g_assert_false(gw_color_init_from_hex(&color, "0000000"));
    g_assert_false(gw_color_init_from_hex(&color, "00?000"));
}

static void test_to_hex(void)
{
    GwColor color = {};
    gchar *hex;

    hex = gw_color_to_hex(&color);
    g_assert_cmpstr(hex, ==, "#00000000");
    g_free(hex);

    color.r = 1.0;
    hex = gw_color_to_hex(&color);
    g_assert_cmpstr(hex, ==, "#FF000000");
    g_free(hex);

    color.a = 0.5;
    hex = gw_color_to_hex(&color);
    g_assert_cmpstr(hex, ==, "#FF00007F");
    g_free(hex);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/color/init_from_x11_name", test_init_from_x11_name);
    g_test_add_func("/color/init_from_hex", test_init_from_hex);
    g_test_add_func("/color/to_hex", test_to_hex);

    return g_test_run();
}