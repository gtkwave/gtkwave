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
