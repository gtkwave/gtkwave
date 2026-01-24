#pragma once

#include <glib-object.h>
#include <stdint.h>
#include <gtkwave.h>

G_BEGIN_DECLS

#define GW_AUDIO_ERROR (gw_audio_error_quark())

typedef enum
{
    GW_AUDIO_ERROR_INIT,
    GW_AUDIO_ERROR_PLAY,
} GwAudioError;

#define GW_TYPE_AUDIO_PLAYER (gw_audio_player_get_type())
G_DECLARE_FINAL_TYPE(GwAudioPlayer, gw_audio_player, GW, AUDIO_PLAYER, GObject)

GwAudioPlayer *gw_audio_player_new(GError **error);
gboolean gw_audio_player_play(GwAudioPlayer *self, GwTrace **traces, GwTimeRange *range, GError **error);

G_END_DECLS
