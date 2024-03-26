#pragma once

#include <glib.h>

/**
 * GwTime:
 *
 * Signed time type.
 */
typedef gint64 GwTime;

#define GW_TIME_CONSTANT(x) G_GINT64_CONSTANT(x)
#define GW_TIME_FORMAT G_GINT64_FORMAT
#define GW_TIME_MAX G_MAXINT64
#define GW_TIME_MIN G_MININT64

/**
 * GwUTime:
 *
 * Unsigned time type.
 */
typedef guint64 GwUTime;

#define GW_UTIME_CONSTANT(x) G_GUINT64_CONSTANT(x)
#define GW_UTIME_FORMAT G_GINT64_FORMAT

/**
 * GwTimeDimension:
 *
 * Time dimension.
 */
typedef enum
{
    GW_TIME_DIMENSION_NONE,
    GW_TIME_DIMENSION_BASE = ' ',
    GW_TIME_DIMENSION_MILLI = 'm',
    GW_TIME_DIMENSION_MICRO = 'u',
    GW_TIME_DIMENSION_NANO = 'n',
    GW_TIME_DIMENSION_PICO = 'p',
    GW_TIME_DIMENSION_FEMTO = 'f',
    GW_TIME_DIMENSION_ATTO = 'a',
    GW_TIME_DIMENSION_ZEPTO = 'z',
} GwTimeDimension;

GwTimeDimension gw_time_dimension_from_exponent(GwTime exponent);

typedef struct
{
    GwTime scale;
    GwTimeDimension dimension;
} GwTimeScaleAndDimension;

GwTimeScaleAndDimension *gw_time_scale_and_dimension_from_exponent(gint exponent);
