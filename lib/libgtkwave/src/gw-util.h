#pragma once

#include <glib-object.h>

GParamSpec *gw_param_spec_time(const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               GParamFlags flags);