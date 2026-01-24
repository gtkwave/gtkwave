#include "gw-audio-player.h"
#include "analyzer.h"
#include <SDL2/SDL.h>
#include <SDL_mixer.h>

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

void channel_finished(int channel)
{
    Mix_Chunk *chunk = Mix_GetChunk(channel);
    g_free(chunk->abuf);
    Mix_FreeChunk(chunk);
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

    Mix_Init(0);

    if (Mix_OpenAudioDevice(self->sample_rate, AUDIO_S16, 1, 2048, NULL, 0) < 0) {
        g_set_error(error,
                    GW_AUDIO_ERROR,
                    GW_AUDIO_ERROR_INIT,
                    "Failed to open audio device: %s",
                    Mix_GetError());
    }

    Mix_ChannelFinished(channel_finished);

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

Mix_Chunk *gw_audio_player_samples_from_trace(GwAudioPlayer *self,
                                              GwTrace *trace,
                                              GwTimeRange *range)
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

    return Mix_QuickLoad_RAW((Uint8 *)samples, sample_count * 2);
}

gboolean gw_audio_player_play(GwAudioPlayer *self,
                              GwTrace **traces,
                              GwTimeRange *range,
                              GError **error)
{
    g_return_val_if_fail(GW_IS_AUDIO_PLAYER(self), FALSE);
    g_return_val_if_fail(traces != NULL, FALSE);
    g_return_val_if_fail(GW_IS_TIME_RANGE(range), FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    Mix_HaltChannel(-1);

    gboolean failed = FALSE;
    GPtrArray *chunks = g_ptr_array_new();
    for (GwTrace **iter = traces; *iter != NULL; iter++) {
        GwTrace *trace = *iter;

        if (trace->vector) {
            g_set_error(error, GW_AUDIO_ERROR, GW_AUDIO_ERROR_PLAY, "Cannot play vector trace");
            failed = TRUE;
            break;
        }

        GwNode *n = trace->n.nd;
        if (ABS(n->msi - n->lsi) + 1 != 16) {
            g_set_error(error, GW_AUDIO_ERROR, GW_AUDIO_ERROR_PLAY, "Trace must be 16 bit wide");
            failed = TRUE;
            break;
        }

        Mix_Chunk *chunk = gw_audio_player_samples_from_trace(self, trace, range);
        g_ptr_array_add(chunks, chunk);
    }

    if (failed) {
        for (size_t i = 0; i < chunks->len; i++) {
            Mix_Chunk *chunk = g_ptr_array_index(chunks, i);
            if (chunk != NULL) {
                g_free(chunk->abuf);
                Mix_FreeChunk(chunk);
            }
        }
        g_ptr_array_free(chunks, TRUE);
        return FALSE;
    }

    for (size_t i = 0; i < chunks->len; i++) {
        Mix_Chunk *chunk = g_ptr_array_index(chunks, i);

        if (Mix_PlayChannel(-1, chunk, 0) < 0) {
            g_set_error(error,
                        GW_AUDIO_ERROR,
                        GW_AUDIO_ERROR_PLAY,
                        "Failed to play audio: %s",
                        SDL_GetError());
            return FALSE;
        }
    }

    g_ptr_array_free(chunks, TRUE);
    return TRUE;
}
