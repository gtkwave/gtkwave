#pragma once

#include "gw-audio-track.h"
#include <gtkwave.h>
#include <gst/app/app.h>

G_BEGIN_DECLS

#define GW_TYPE_AUDIO_SRC (gw_audio_src_get_type())
G_DECLARE_FINAL_TYPE(GwAudioSrc, gw_audio_src, GW, AUDIO_SRC, GstAppSrc)

GstElement *gw_audio_src_new(GwAudioTrack *track);

G_END_DECLS
