#include "gw-audio-player.h"
#include <SDL2/SDL.h>

// clang-format off
G_DEFINE_QUARK(gw-audio-error-quark, gw_audio_error)
// clang-format on

struct _GwAudioPlayer
{
    GObject parent_instance;

    uint64_t sample_rate;
    GwTime time_per_sample;

    SDL_AudioDeviceID audio_device;
};

G_DEFINE_TYPE(GwAudioPlayer, gw_audio_player, G_TYPE_OBJECT)

static void gw_audio_player_class_init(GwAudioPlayerClass *klass)
{
    (void)klass;
}

static void gw_audio_player_init(GwAudioPlayer *self)
{
    self->sample_rate = 44800;
    self->time_per_sample = 1000000000000 / self->sample_rate;
}

GwAudioPlayer *gw_audio_player_new(GError **error)
{
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    GwAudioPlayer *self = g_object_new(GW_TYPE_AUDIO_PLAYER, NULL);

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        g_set_error(error,
                    GW_AUDIO_ERROR,
                    GW_AUDIO_ERROR_INIT,
                    "Failed to initialize SDL: %s",
                    SDL_GetError());
        return NULL;
    }

    SDL_AudioSpec spec = {
        .freq = self->sample_rate,
        .format = AUDIO_S16,
        .channels = 1,
    };

    self->audio_device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (self->audio_device == 0) {
        g_set_error(error,
                    GW_AUDIO_ERROR,
                    GW_AUDIO_ERROR_INIT,
                    "Failed to open audio device: %s",
                    SDL_GetError());

        g_object_unref(self);
        return NULL;
    }

    return self;
}

int16_t hist_ent_to_i16(GwHistEnt *h)
{
    const char *bits = h->v.h_vector;

    uint16_t value = 0;
    for (int bit = 0; bit < 16; bit++) {
        value = value << 1;
        if (bits[bit] == GW_BIT_1 || bits[bit] == GW_BIT_H) {
            value |= 0x1;
        }
    }

    return (int16_t)value;
}

int16_t *gw_audio_player_samples_from_trace(GwAudioPlayer *self,
                                            GwTrace *trace,
                                            GwTimeRange *range,
                                            size_t *sample_count_out)
{
    GwTime start = gw_time_range_get_start(range);
    GwTime end = gw_time_range_get_end(range);

    size_t sample_count = (end - start) / self->time_per_sample;
    int16_t *samples = g_new0(int16_t, sample_count);

    GwNode *n = trace->n.nd;

    GwHistEnt *iter = &n->head;
    size_t i = 0;
    for (GwTime time = start; i < sample_count && time < end; time += self->time_per_sample, i++) {
        while (iter->next->time <= time) {
            iter = iter->next;
        }

        g_assert_cmpint(iter->time, <=, time);
        g_assert_cmpint(iter->next->time, >, time);

        samples[i] = hist_ent_to_i16(iter);
    }

    *sample_count_out = sample_count;

    return samples;
}

gboolean gw_audio_player_play(GwAudioPlayer *self,
                              GwTrace *trace,
                              GwTimeRange *range,
                              GError **error)
{
    g_return_val_if_fail(GW_IS_AUDIO_PLAYER(self), FALSE);
    g_return_val_if_fail(trace != NULL, FALSE);
    g_return_val_if_fail(GW_IS_TIME_RANGE(range), FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (trace->vector) {
        g_set_error(error, GW_AUDIO_ERROR, GW_AUDIO_ERROR_PLAY, "Cannot play vector trace");
        return FALSE;
    }

    GwNode *n = trace->n.nd;
    if (ABS(n->msi - n->lsi) + 1 != 16) {
        g_set_error(error, GW_AUDIO_ERROR, GW_AUDIO_ERROR_PLAY, "Trace must be 16 bit wide");
        return FALSE;
    }

    size_t sample_count = 0;
    int16_t *samples = gw_audio_player_samples_from_trace(self, trace, range, &sample_count);

    size_t bytes = sample_count * 2;
    if (SDL_QueueAudio(self->audio_device, samples, bytes) < 0) {
        g_set_error(error,
                    GW_AUDIO_ERROR,
                    GW_AUDIO_ERROR_PLAY,
                    "Failed to queue audio samples: %s",
                    SDL_GetError());
        return FALSE;
    }

    g_free(samples);

    SDL_PauseAudioDevice(self->audio_device, 0);

    return TRUE;
}
