#define G_LOG_DOMAIN "gw-audio-track"

#include "gw-audio-track.h"
#include "analyzer.h"
#include <stdint.h>

#define DEFAULT_SAMPLE_RATE 44800

struct _GwAudioTrack
{
    GObject parent_instance;

    GwAudioTrackSamples samples;
};

G_DEFINE_TYPE(GwAudioTrack, gw_audio_track, G_TYPE_OBJECT)

static void gw_audio_track_finalize(GObject *object)
{
    GwAudioTrack *self = GW_AUDIO_TRACK(object);

    g_free(self->samples.values);

    G_OBJECT_CLASS(gw_audio_track_parent_class)->finalize(object);
}

static void gw_audio_track_class_init(GwAudioTrackClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_audio_track_finalize;
}

static void gw_audio_track_init(GwAudioTrack *self)
{
    self->samples.volume = 1.0;
    self->samples.channel = GW_AUDIO_CHANNEL_BOTH;
}

static float hist_ent_to_float(GwHistEnt *h,
                               size_t bit_width,
                               gboolean is_signed,
                               float sample_scale)
{
    const char *bits = h->v.h_vector;

    uint64_t value_uint = 0;
    for (int bit = 0; bit < bit_width; bit++) {
        value_uint = value_uint << 1;
        if (bits[bit] == GW_BIT_1 || bits[bit] == GW_BIT_H) {
            value_uint |= 0x1;
        }
    }

    if (is_signed) {
        int64_t value_int = (int64_t)value_uint;

        int64_t sign_mask = 1 << (bit_width - 1);
        value_int = (value_int ^ sign_mask) - sign_mask;

        return value_int * sample_scale;
    } else {
        return (value_uint * sample_scale) - 1.0;
    }
}
static gboolean gw_audio_track_sample_trace(GwAudioTrack *self,
                                            GwTrace *trace,
                                            GwDumpFile *dump_file,
                                            unsigned int sample_rate,
                                            GError **error)
{
    g_return_val_if_fail(HasWave(trace), FALSE);

    if (trace->vector) {
        g_set_error(error,
                    GW_AUDIO_ERROR,
                    GW_AUDIO_ERROR_UNSUPPORTED_TRACE_TYPE,
                    "Cannot play vector trace");
        return FALSE;
    }

    GwNode *node = trace->n.nd;
    int bit_width = ABS(node->msi - node->lsi) + 1;
    if (bit_width == 0 || bit_width > 64) {
        g_set_error(error,
                    GW_AUDIO_ERROR,
                    GW_AUDIO_ERROR_UNSUPPORTED_TRACE_TYPE,
                    "Unsupported bit depth");
        return FALSE;
    }

    GwTimeRange *range = gw_dump_file_get_time_range(dump_file);
    GwTime start = gw_time_range_get_start(range);
    GwTime end = gw_time_range_get_end(range);

    GwTime time_per_second =
        gw_time_rescale(1, GW_TIME_DIMENSION_BASE, gw_dump_file_get_time_dimension(dump_file));
    GwTime time_per_sample = time_per_second / sample_rate;

    self->samples.count = (end - start) / time_per_sample;
    self->samples.values = g_new(float, self->samples.count);
    self->samples.sample_rate = sample_rate;
    self->samples.duration_ns = 1.0e9 / sample_rate * self->samples.count;

    gboolean is_signed = (trace->flags & TR_SIGNED) != 0;
    gboolean is_scalar = node->msi == node->lsi; // TODO: use node kind
    float sample_scale = powf(0.5, bit_width - 1);

    GwNode *n = trace->n.nd;

    GwHistEnt *iter = &n->head;
    size_t i = 0;
    for (GwTime time = start; i < self->samples.count && time < end; time += time_per_sample, i++) {
        while (iter->next->time <= time) {
            iter = iter->next;
        }

        if (iter->flags & GW_HIST_ENT_FLAG_REAL) {
            self->samples.values[i] = iter->v.h_double;
        } else if (is_scalar) {
            gboolean high = iter->v.h_val == GW_BIT_1 || iter->v.h_val == GW_BIT_H;
            self->samples.values[i] = high ? 1.0 : -1.0;
        } else {
            self->samples.values[i] = hist_ent_to_float(iter, bit_width, is_signed, sample_scale);
        }
    }

    g_assert_cmpint(i, ==, self->samples.count);

    return TRUE;
}

GwAudioTrack *gw_audio_track_new(GwTrace *trace,
                                 GwDumpFile *dump_file,
                                 GHashTable *parameters,
                                 GError **error)
{
    g_return_val_if_fail(trace != NULL, NULL);
    g_return_val_if_fail(GW_IS_DUMP_FILE(dump_file), NULL);

    GwAudioTrack *self = g_object_new(GW_TYPE_AUDIO_TRACK, NULL);

    uint64_t sample_rate = DEFAULT_SAMPLE_RATE;
    if (parameters != NULL) {
        const char *param_s = g_hash_table_lookup(parameters, "s");
        if (param_s != NULL) {
            g_ascii_string_to_unsigned(param_s, 10, 1, UINT64_MAX, &sample_rate, NULL);
        }

        const char *param_c = g_hash_table_lookup(parameters, "c");
        if (param_c != NULL) {
            if (strcmp(param_c, "l") == 0) {
                self->samples.channel = GW_AUDIO_CHANNEL_LEFT;
            } else if (strcmp(param_c, "r") == 0) {
                self->samples.channel = GW_AUDIO_CHANNEL_RIGHT;
            }
        }

        const char *param_v = g_hash_table_lookup(parameters, "v");
        if (param_v != NULL) {
            uint64_t volume_percent = 100;
            g_ascii_string_to_unsigned(param_v, 10, 0, UINT64_MAX, &volume_percent, NULL);
            self->samples.volume = volume_percent / 100.0;
        }
    }

    if (gw_audio_track_sample_trace(self, trace, dump_file, sample_rate, error)) {
        return self;
    } else {
        g_object_unref(self);
        return NULL;
    }
}

GwAudioTrackSamples gw_audio_track_get_samples(GwAudioTrack *self)
{
    static const GwAudioTrackSamples NULL_SAMPLES;
    g_return_val_if_fail(GW_IS_AUDIO_TRACK(self), NULL_SAMPLES);

    return self->samples;
}

size_t gw_audio_track_time_to_sample_index(GwAudioTrack *self, int64_t time_ns)
{
    g_return_val_if_fail(GW_IS_AUDIO_TRACK(self), 0);
    g_return_val_if_fail(time_ns >= 0, 0);

    double factor = (double)time_ns / self->samples.duration_ns;

    size_t index = factor * self->samples.count;
    return MIN(index, self->samples.count - 1);
}
