#include <gtkwave.h>

static void test_time_scale_and_dimension_from_exponent(void)
{
    static const GwTimeScaleAndDimension EXPECTED[24] = {
        {100, GW_TIME_DIMENSION_BASE},  {10, GW_TIME_DIMENSION_BASE},  {1, GW_TIME_DIMENSION_BASE},
        {100, GW_TIME_DIMENSION_MILLI}, {10, GW_TIME_DIMENSION_MILLI}, {1, GW_TIME_DIMENSION_MILLI},
        {100, GW_TIME_DIMENSION_MICRO}, {10, GW_TIME_DIMENSION_MICRO}, {1, GW_TIME_DIMENSION_MICRO},
        {100, GW_TIME_DIMENSION_NANO},  {10, GW_TIME_DIMENSION_NANO},  {1, GW_TIME_DIMENSION_NANO},
        {100, GW_TIME_DIMENSION_PICO},  {10, GW_TIME_DIMENSION_PICO},  {1, GW_TIME_DIMENSION_PICO},
        {100, GW_TIME_DIMENSION_FEMTO}, {10, GW_TIME_DIMENSION_FEMTO}, {1, GW_TIME_DIMENSION_FEMTO},
        {100, GW_TIME_DIMENSION_ATTO},  {10, GW_TIME_DIMENSION_ATTO},  {1, GW_TIME_DIMENSION_ATTO},
        {100, GW_TIME_DIMENSION_ZEPTO}, {10, GW_TIME_DIMENSION_ZEPTO}, {1, GW_TIME_DIMENSION_ZEPTO},
    };

    int exponent = 2;
    for (size_t i = 0; i < G_N_ELEMENTS(EXPECTED); i++, exponent--) {
        GwTimeScaleAndDimension *value = gw_time_scale_and_dimension_from_exponent(exponent);

        g_assert_cmpint(value->scale, ==, EXPECTED[i].scale);
        g_assert_cmpint(value->dimension, ==, EXPECTED[i].dimension);

        g_free(value);
    }
    g_assert_cmpint(exponent, ==, -22);
}

typedef struct
{
    GwTime value;
    GwTimeDimension source_dimension;

    GwTime expected_value;
    GwTimeDimension target_dimension;
} RescaleTest;

static void test_rescale(void)
{
    static const RescaleTest TESTS[] = {
        {1, GW_TIME_DIMENSION_BASE, 1000, GW_TIME_DIMENSION_MILLI},
        {1, GW_TIME_DIMENSION_BASE, 1000000, GW_TIME_DIMENSION_MICRO},
        {12345, GW_TIME_DIMENSION_ATTO, 12, GW_TIME_DIMENSION_FEMTO},
    };

    for (size_t i = 0; i < G_N_ELEMENTS(TESTS); i++) {
        const RescaleTest *test = &TESTS[i];

        GwTime value = gw_time_rescale(test->value, test->source_dimension, test->target_dimension);
        g_assert_cmpint(value, ==, test->expected_value);
    }
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/time/time_scale_and_dimension_from_exponent",
                    test_time_scale_and_dimension_from_exponent);
    g_test_add_func("/time/rescale", test_rescale);

    return g_test_run();
}
