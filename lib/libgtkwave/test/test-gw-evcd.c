#include <gtkwave.h>

static void test_char_to_vcd(void)
{
    /* Input class. */
    g_assert_cmpint(gw_evcd_char_to_vcd('D'), ==, '0');
    g_assert_cmpint(gw_evcd_char_to_vcd('U'), ==, '1');
    g_assert_cmpint(gw_evcd_char_to_vcd('N'), ==, 'x');
    g_assert_cmpint(gw_evcd_char_to_vcd('Z'), ==, 'z');
    g_assert_cmpint(gw_evcd_char_to_vcd('d'), ==, '0');
    g_assert_cmpint(gw_evcd_char_to_vcd('u'), ==, '1');

    /* Output class. */
    g_assert_cmpint(gw_evcd_char_to_vcd('L'), ==, '0');
    g_assert_cmpint(gw_evcd_char_to_vcd('H'), ==, '1');
    g_assert_cmpint(gw_evcd_char_to_vcd('X'), ==, 'x');
    g_assert_cmpint(gw_evcd_char_to_vcd('T'), ==, 'z');
    g_assert_cmpint(gw_evcd_char_to_vcd('l'), ==, '0');
    g_assert_cmpint(gw_evcd_char_to_vcd('h'), ==, '1');

    /* Unknown direction. */
    g_assert_cmpint(gw_evcd_char_to_vcd('0'), ==, '0');
    g_assert_cmpint(gw_evcd_char_to_vcd('1'), ==, '1');
    g_assert_cmpint(gw_evcd_char_to_vcd('?'), ==, 'x');
    g_assert_cmpint(gw_evcd_char_to_vcd('F'), ==, 'z');
    g_assert_cmpint(gw_evcd_char_to_vcd('f'), ==, 'z');

    /* A, a, B, b, C, c all collapse to x. */
    g_assert_cmpint(gw_evcd_char_to_vcd('A'), ==, 'x');
    g_assert_cmpint(gw_evcd_char_to_vcd('c'), ==, 'x');

    /* Anything outside the alphabet collapses to x. */
    g_assert_cmpint(gw_evcd_char_to_vcd('q'), ==, 'x');
    g_assert_cmpint(gw_evcd_char_to_vcd('\0'), ==, 'x');
}

static void test_char_to_direction(void)
{
    /* Input (test fixture) class. */
    g_assert_cmpint(gw_evcd_char_to_direction('D'), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_char_to_direction('U'), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_char_to_direction('N'), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_char_to_direction('Z'), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_char_to_direction('d'), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_char_to_direction('u'), ==, GW_VAR_DIR_IN);

    /* Output (DUT) class. */
    g_assert_cmpint(gw_evcd_char_to_direction('L'), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(gw_evcd_char_to_direction('H'), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(gw_evcd_char_to_direction('X'), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(gw_evcd_char_to_direction('T'), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(gw_evcd_char_to_direction('l'), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(gw_evcd_char_to_direction('h'), ==, GW_VAR_DIR_OUT);

    /* Unknown-direction class. */
    g_assert_cmpint(gw_evcd_char_to_direction('0'), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_char_to_direction('1'), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_char_to_direction('?'), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_char_to_direction('F'), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_char_to_direction('A'), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_char_to_direction('f'), ==, GW_VAR_DIR_INOUT);

    /* Outside the alphabet. */
    g_assert_cmpint(gw_evcd_char_to_direction('q'), ==, GW_VAR_DIR_IMPLICIT);
}

static void test_direction_merge(void)
{
    /* Identity. */
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_IMPLICIT, GW_VAR_DIR_IN), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_OUT, GW_VAR_DIR_IMPLICIT), ==, GW_VAR_DIR_OUT);
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_IMPLICIT, GW_VAR_DIR_IMPLICIT),
                    ==,
                    GW_VAR_DIR_IMPLICIT);

    /* Same class stays. */
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_IN, GW_VAR_DIR_IN), ==, GW_VAR_DIR_IN);
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_OUT, GW_VAR_DIR_OUT), ==, GW_VAR_DIR_OUT);

    /* Conflicting concrete directions resolve to inout. */
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_IN, GW_VAR_DIR_OUT), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_OUT, GW_VAR_DIR_IN), ==, GW_VAR_DIR_INOUT);
    g_assert_cmpint(gw_evcd_direction_merge(GW_VAR_DIR_IN, GW_VAR_DIR_INOUT), ==, GW_VAR_DIR_INOUT);
}

static void test_char_describe(void)
{
    g_assert_cmpstr(gw_evcd_char_describe('D'), ==, "low (input)");
    g_assert_cmpstr(gw_evcd_char_describe('u'), ==, "high, two or more drivers (input)");
    g_assert_cmpstr(gw_evcd_char_describe('H'), ==, "high (output)");
    g_assert_cmpstr(gw_evcd_char_describe('T'), ==, "three-state (output)");
    g_assert_cmpstr(gw_evcd_char_describe('F'), ==, "three-state (input and output unconnected)");
    g_assert_cmpstr(gw_evcd_char_describe('A'), ==, "unknown (input 0, output 1)");

    /* Outside the alphabet. */
    g_assert_null(gw_evcd_char_describe('q'));
}

static void test_strength_name(void)
{
    g_assert_cmpstr(gw_evcd_strength_name(0), ==, "highz");
    g_assert_cmpstr(gw_evcd_strength_name(1), ==, "small");
    g_assert_cmpstr(gw_evcd_strength_name(2), ==, "medium");
    g_assert_cmpstr(gw_evcd_strength_name(3), ==, "weak");
    g_assert_cmpstr(gw_evcd_strength_name(4), ==, "large");
    g_assert_cmpstr(gw_evcd_strength_name(5), ==, "pull");
    g_assert_cmpstr(gw_evcd_strength_name(6), ==, "strong");
    g_assert_cmpstr(gw_evcd_strength_name(7), ==, "supply");

    /* Out of range. */
    g_assert_null(gw_evcd_strength_name(-1));
    g_assert_null(gw_evcd_strength_name(8));
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/evcd/char_to_vcd", test_char_to_vcd);
    g_test_add_func("/evcd/char_to_direction", test_char_to_direction);
    g_test_add_func("/evcd/direction_merge", test_direction_merge);
    g_test_add_func("/evcd/char_describe", test_char_describe);
    g_test_add_func("/evcd/strength_name", test_strength_name);

    return g_test_run();
}
