#pragma once

#include <gtkwave.h>
#include <glib-object.h>
#include <stdint.h>

G_BEGIN_DECLS

typedef enum
{
    GW_AUDIO_CHANNEL_LEFT,
    GW_AUDIO_CHANNEL_RIGHT,
    GW_AUDIO_CHANNEL_BOTH,
} GwAudioChannel;

typedef struct
{
    float *values;
    size_t count;
    unsigned int sample_rate;
    GwAudioChannel channel;
    double volume;
    uint64_t duration_ns;
} GwAudioTrackSamples;

#define GW_TYPE_AUDIO_TRACK (gw_audio_track_get_type())
G_DECLARE_FINAL_TYPE(GwAudioTrack, gw_audio_track, GW, AUDIO_TRACK, GObject)

GwAudioTrack *gw_audio_track_new(GwTrace *trace,
                                 GwDumpFile *dump_file,
                                 GHashTable *parameters,
                                 GError **error);
GwAudioTrackSamples gw_audio_track_get_samples(GwAudioTrack *self);

size_t gw_audio_track_time_to_sample_index(GwAudioTrack *self, int64_t time_ns);

G_END_DECLS
