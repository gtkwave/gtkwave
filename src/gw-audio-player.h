#pragma once

#include <glib-object.h>
#include <stdint.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_AUDIO_ERROR (gw_audio_error_quark())
GQuark gw_audio_error_quark(void);

typedef enum
{
    GW_AUDIO_ERROR_UNSUPPORTED_TRACE_TYPE,
    GW_AUDIO_ERROR_INIT,
    GW_AUDIO_ERROR_PLAY,
} GwAudioError;

typedef enum
{
    GW_AUDIO_PLAYER_STATE_IDLE,
    GW_AUDIO_PLAYER_STATE_PLAYING,
} GwAudioPlayerState;

#define GW_TYPE_AUDIO_PLAYER (gw_audio_player_get_type())
G_DECLARE_FINAL_TYPE(GwAudioPlayer, gw_audio_player, GW, AUDIO_PLAYER, GObject)

GwAudioPlayer *gw_audio_player_new(void);

gboolean gw_audio_player_play(GwAudioPlayer *self,
                              GwTrace **traces,
                              GwDumpFile *dump_file,
                              GwTimeRange *range,
                              GError **error);
void gw_audio_player_stop(GwAudioPlayer *self);

GwTime gw_audio_player_get_position(GwAudioPlayer *self);
GwAudioPlayerState gw_audio_player_get_state(GwAudioPlayer *self);

G_END_DECLS
