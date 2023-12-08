#include "gw-time.h"

GwTimeDimension gw_time_dimension_from_exponent(GwTime exponent)
{
    switch (exponent) {
        case 0:
            return GW_TIME_DIMENSION_BASE;
        case -3:
            return GW_TIME_DIMENSION_MILLI;
        case -6:
            return GW_TIME_DIMENSION_MICRO;
        case -9:
            return GW_TIME_DIMENSION_NANO;
        case -12:
            return GW_TIME_DIMENSION_PICO;
        case -15:
            return GW_TIME_DIMENSION_FEMTO;
        case -18:
            return GW_TIME_DIMENSION_ATTO;
        case -21:
            return GW_TIME_DIMENSION_ZEPTO;
        default:
            g_return_val_if_reached(GW_TIME_DIMENSION_BASE);
    }
}

GwTimeScaleAndDimension *gw_time_scale_and_dimension_from_exponent(gint exponent)
{
    g_return_val_if_fail(exponent >= -21 && exponent <= 2, NULL);

    GwTimeScaleAndDimension *self = g_new0(GwTimeScaleAndDimension, 1);

    switch (exponent % 3) {
        case 2:
        case -1:
            self->scale = 100;
            self->dimension = gw_time_dimension_from_exponent(exponent - 2);
            break;

        case 1:
        case -2:
            self->scale = 10;
            self->dimension = gw_time_dimension_from_exponent(exponent - 1);
            break;

        default:
            self->scale = 1;
            self->dimension = gw_time_dimension_from_exponent(exponent);
            break;
    }

    return self;
}
