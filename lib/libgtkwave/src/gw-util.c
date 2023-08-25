#include "gw-util.h"

GParamSpec *gw_param_spec_time(const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               GParamFlags flags)
{
    return g_param_spec_int64(name, nick, blurb, G_MININT64, G_MAXINT64, 0, flags);
}