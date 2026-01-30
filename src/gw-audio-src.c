#define G_LOG_DOMAIN "gw-audio-src"

#include "gw-audio-src.h"
#include "analyzer.h"
#include <gst/audio/audio.h>
#include <stdint.h>

struct _GwAudioSrc
{
    GstAppSrc parent_instance;

    GwAudioTrack *track;
    size_t position;
};

G_DEFINE_TYPE(GwAudioSrc, gw_audio_src, GST_TYPE_APP_SRC)

static void gw_audio_src_need_data(GstAppSrc *app_src, unsigned int length)
{
    GwAudioSrc *self = GW_AUDIO_SRC(app_src);

    GwAudioTrackSamples samples = gw_audio_track_get_samples(self->track);

    if (self->position >= samples.count) {
        if (gst_app_src_end_of_stream(app_src) != GST_FLOW_OK) {
            g_warning("end_of_stream failed");
        }

        // g_debug("need-data: %d -> eos", length);
        return;
    }

    size_t max_samples = length / sizeof(float);
    size_t sample_count = MIN(samples.count - self->position, max_samples);
    size_t sample_bytes = sample_count * sizeof(float);

    GstBuffer *buffer = gst_buffer_new_and_alloc(sample_bytes);
    GST_BUFFER_TIMESTAMP(buffer) =
        gst_util_uint64_scale(self->position, GST_SECOND, samples.sample_rate);
    GST_BUFFER_DURATION(buffer) =
        gst_util_uint64_scale(sample_count, GST_SECOND, samples.sample_rate);

    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, &samples.values[self->position], sample_bytes);
    gst_buffer_unmap(buffer, &info);

    self->position += sample_count;

    if (gst_app_src_push_buffer(app_src, buffer) != GST_FLOW_OK) {
        g_warning("push_buffer failed");
    }

    // g_debug("need-data: %d -> pushed %zu", length, sample_bytes);
}

static gboolean gw_audio_src_seek_data(GstAppSrc *app_src, uint64_t offset)
{
    GwAudioSrc *self = GW_AUDIO_SRC(app_src);

    size_t index = gw_audio_track_time_to_sample_index(self->track, offset);
    self->position = index;

    g_debug("seek_data: offset=%lu, index=%zu", offset, index);

    return TRUE;
}

static void gw_audio_src_dispose(GObject *object)
{
    GwAudioSrc *self = GW_AUDIO_SRC(object);

    g_clear_object(&self->track);

    G_OBJECT_CLASS(gw_audio_src_parent_class)->dispose(object);
}

static void gw_audio_src_class_init(GwAudioSrcClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GstAppSrcClass *app_src_class = GST_APP_SRC_CLASS(klass);

    object_class->dispose = gw_audio_src_dispose;

    app_src_class->need_data = gw_audio_src_need_data;
    app_src_class->seek_data = gw_audio_src_seek_data;
}

static void gw_audio_src_init(GwAudioSrc *self)
{
    (void)self;
}

static GstAudioChannelPosition gw_audio_channel_to_gst(GwAudioChannel self)
{
    switch (self) {
        case GW_AUDIO_CHANNEL_LEFT:
            return GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;

        case GW_AUDIO_CHANNEL_RIGHT:
            return GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;

        case GW_AUDIO_CHANNEL_BOTH:
            return GST_AUDIO_CHANNEL_POSITION_MONO;

        default:
            g_return_val_if_reached(GST_AUDIO_CHANNEL_POSITION_MONO);
    }
}

GstElement *gw_audio_src_new(GwAudioTrack *track)
{
    g_return_val_if_fail(GW_IS_AUDIO_TRACK(track), NULL);

    GwAudioTrackSamples samples = gw_audio_track_get_samples(track);
    GstAudioChannelPosition channel_position = gw_audio_channel_to_gst(samples.channel);

    GstAudioInfo audio_info;
    gst_audio_info_set_format(&audio_info,
                              GST_AUDIO_FORMAT_F32,
                              samples.sample_rate,
                              1,
                              &channel_position);

    GstCaps *audio_caps = gst_audio_info_to_caps(&audio_info);

    GwAudioSrc *self = g_object_new(GW_TYPE_AUDIO_SRC,
                                    "caps",
                                    audio_caps,
                                    "format",
                                    GST_FORMAT_TIME,
                                    "stream-type",
                                    GST_APP_STREAM_TYPE_SEEKABLE,
                                    NULL);
    self->track = g_object_ref(track);

    gst_app_src_set_duration(GST_APP_SRC(self), samples.duration_ns);

    return GST_ELEMENT(self);
}
