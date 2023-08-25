#pragma once

#include <glib.h>

typedef gint64 GwTime;

#define GW_TIME_CONSTANT(x) G_GINT64_CONSTANT(x)
#define GW_TIME_FORMAT G_GINT64_FORMAT
#define GW_TIME_MAX G_MAXINT64
#define GW_TIME_MIN G_MININT64

typedef guint64 GwUTime;

#define GW_UTIME_CONSTANT(x) G_GUINT64_CONSTANT(x)
#define GW_UTIME_FORMAT G_GINT64_FORMAT
