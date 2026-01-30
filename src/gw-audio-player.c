#define G_LOG_DOMAIN "gw-audio-player"

#include "gw-audio-player.h"
#include "gw-audio-src.h"
#include "gw-audio-track.h"
#include "analyzer.h"
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/app/app.h>

// clang-format off
G_DEFINE_QUARK(gw-audio-error-quark, gw_audio_error)
// clang-format on

struct _GwAudioPlayer
{
    GObject parent_instance;

    GstElement *pipeline;

    GwAudioPlayerState state;
    GwTime position;
    gboolean position_valid;

    GRegex *parameter_list_regex;
    GRegex *parameter_regex;
};

G_DEFINE_TYPE(GwAudioPlayer, gw_audio_player, G_TYPE_OBJECT)

enum
{
    PROP_POSITION = 1,
    PROP_STATE,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_audio_player_finalize(GObject *object)
{
    GwAudioPlayer *self = GW_AUDIO_PLAYER(object);

    g_regex_unref(self->parameter_list_regex);
    g_regex_unref(self->parameter_regex);

    G_OBJECT_CLASS(gw_audio_player_parent_class)->finalize(object);
}

static void gw_audio_player_get_property(GObject *object,
                                         guint property_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
    GwAudioPlayer *self = GW_AUDIO_PLAYER(object);

    switch (property_id) {
        case PROP_POSITION:
            g_value_set_int64(value, gw_audio_player_get_position(self));
            break;

        case PROP_STATE:
            g_value_set_uint(value, gw_audio_player_get_state(self)); // TODO: enum
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_audio_player_class_init(GwAudioPlayerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_audio_player_finalize;
    object_class->get_property = gw_audio_player_get_property;

    properties[PROP_POSITION] =
        g_param_spec_int64("position",
                           NULL,
                           NULL,
                           G_MININT64,
                           G_MAXINT64,
                           0,
                           G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    // TODO: The state property should use an enum type.
    properties[PROP_STATE] =
        g_param_spec_uint("state",
                          NULL,
                          NULL,
                          0,
                          1,
                          0,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_audio_player_init(GwAudioPlayer *self)
{
    self->parameter_list_regex =
        g_regex_new("\\((\\s*\\w+\\s*:\\s*\\w+\\s*(?:,\\s*\\w+\\s*:\\s*\\w+\\s*)*)\\)", 0, 0, NULL);
    g_assert_nonnull(self->parameter_list_regex);

    self->parameter_regex = g_regex_new("(\\w+)\\s*:\\s*(\\w+)", 0, 0, NULL);
    g_assert_nonnull(self->parameter_regex);
}

GwAudioPlayer *gw_audio_player_new(void)
{
    return g_object_new(GW_TYPE_AUDIO_PLAYER, NULL);
}

static void gw_audio_player_set_state(GwAudioPlayer *self, GwAudioPlayerState state)
{
    if (self->state != state) {
        self->state = state;
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATE]);
    }
}

GwAudioPlayerState gw_audio_player_get_state(GwAudioPlayer *self)
{
    g_return_val_if_fail(GW_IS_AUDIO_PLAYER(self), GW_AUDIO_PLAYER_STATE_IDLE);

    return self->state;
}

static int bus_watch(GstBus *bus, GstMessage *message, void *user_data)
{
    GwAudioPlayer *self = user_data;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_STATE_CHANGED:
            if (message->src == GST_OBJECT(self->pipeline)) {
                GstState old_state;
                GstState new_state;
                gst_message_parse_state_changed(message, &old_state, &new_state, NULL);

                char *old_state_str = g_enum_to_string(GST_TYPE_STATE, old_state);
                char *new_state_str = g_enum_to_string(GST_TYPE_STATE, new_state);

                g_debug("pipeline state change: %s -> %s", old_state_str, new_state_str);

                g_free(old_state_str);
                g_free(new_state_str);

                gw_audio_player_set_state(self,
                                          new_state == GST_STATE_PLAYING
                                              ? GW_AUDIO_PLAYER_STATE_PLAYING
                                              : GW_AUDIO_PLAYER_STATE_IDLE);
            }
            break;

        case GST_MESSAGE_WARNING: {
            char *debug = NULL;
            GError *error = NULL;
            gst_message_parse_warning(message, &error, &debug);

            g_printerr("GStreamer warning:\n");
            g_printerr("  %s\n", error->message);
            if (debug != NULL) {
                g_printerr("  debug info: %s\n", debug);
            }
            g_printerr("  element: %s\n", GST_OBJECT_NAME(message->src));
            break;
        }

        case GST_MESSAGE_ERROR: {
            char *debug = NULL;
            GError *error = NULL;
            gst_message_parse_error(message, &error, &debug);

            g_printerr("GStreamer error:\n");
            g_printerr("  %s\n", error->message);
            if (debug != NULL) {
                g_printerr("  debug info: %s\n", debug);
            }
            g_printerr("  element: %s\n", GST_OBJECT_NAME(message->src));
            break;
        }

        case GST_MESSAGE_EOS:
            gst_element_set_state(self->pipeline, GST_STATE_READY);
            break;

        case GST_MESSAGE_STREAM_STATUS: {
            GstStreamStatusType type;
            GstElement *owner;
            gst_message_parse_stream_status(message, &type, &owner);
        } break;

        default: {
            // ignore
            break;
        }
    }

    return TRUE;
}

gboolean update(void *user_data)
{
    GwAudioPlayer *self = user_data;

    if (!GST_IS_ELEMENT(self->pipeline)) {
        return G_SOURCE_REMOVE;
    }

    int64_t gst_position = 0;
    gst_element_query_position(self->pipeline, GST_FORMAT_TIME, &gst_position);

    GwTime position = 0;
    if (gst_position >= 0) {
        // TODO: this assumes ps scale
        // TODO: handle overflow?
        position = gst_position * 1000;
    }

    if (self->position != position) {
        self->position = position;
        g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_POSITION]);
    }

    return G_SOURCE_CONTINUE;
}

static gboolean gw_audio_player_build_pipeline(GwAudioPlayer *self, GwTrace **traces)
{
    self->pipeline = gst_pipeline_new("gw-audio-player");
    g_return_val_if_fail(GST_IS_ELEMENT(self->pipeline), FALSE);

    GstElement *mixer = gst_element_factory_make("audiomixer", "mixer");
    g_return_val_if_fail(GST_IS_ELEMENT(mixer), FALSE);

    GstElement *audio_convert = gst_element_factory_make("audioconvert", "audio-convert");
    g_return_val_if_fail(GST_IS_ELEMENT(audio_convert), FALSE);

    GstElement *auto_audio_sink = gst_element_factory_make("autoaudiosink", "auto-audio-sink");
    g_return_val_if_fail(GST_IS_ELEMENT(auto_audio_sink), FALSE);

    gst_bin_add_many(GST_BIN(self->pipeline), mixer, audio_convert, auto_audio_sink, NULL);
    gst_element_link_many(mixer, audio_convert, auto_audio_sink, NULL);

    for (GwTrace **iter = traces; *iter != NULL; iter++) {
        GwTrace *trace = *iter;
        GstElement *src = gw_audio_src_new(trace->audio_track);

        if (!gst_bin_add(GST_BIN(self->pipeline), src)) {
            g_debug("failed to add source to pipeline");
        }

        GwAudioTrackSamples samples = gw_audio_track_get_samples(trace->audio_track);
        GstElement *volume = NULL;
        if (samples.volume != 1.0) {
            volume = gst_element_factory_make("volume", NULL);
            if (!gst_bin_add(GST_BIN(self->pipeline), volume)) {
                g_debug("failed to add source to pipeline");
            }
            g_object_set(volume, "volume", CLAMP(samples.volume, 0.0, 10.0), NULL);
        }

        GstElement *convert = gst_element_factory_make("audioconvert", NULL);
        g_return_val_if_fail(GST_IS_ELEMENT(convert), FALSE);
        if (!gst_bin_add(GST_BIN(self->pipeline), convert)) {
            g_debug("failed to add interleave to pipeline");
        }

        if (volume != NULL) {
            gst_element_link_many(src, volume, convert, NULL);
        } else {
            gst_element_link(src, convert);
        }

        // Always output 2 channel stereo from converter.
        GstCaps *caps = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, NULL);
        gst_element_link_filtered(convert, mixer, caps);
        gst_caps_unref(caps);
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(self->pipeline),
                              GST_DEBUG_GRAPH_SHOW_ALL,
                              "gw-audio-player-pipeline");

    return TRUE;
}

static GHashTable *gw_audio_player_parse_trace_parameters(GwAudioPlayer *self, GwTrace *trace)
{
    if (trace->name_full == NULL) {
        return NULL;
    }

    GHashTable *parameters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    GMatchInfo *match_info = NULL;
    if (g_regex_match(self->parameter_list_regex, trace->name_full, 0, &match_info)) {
        char *match = g_match_info_fetch(match_info, 1);

        GMatchInfo *match_info_inner = NULL;
        g_regex_match(self->parameter_regex, match, 0, &match_info_inner);
        while (g_match_info_matches(match_info_inner)) {
            char *key = g_match_info_fetch(match_info_inner, 1);
            char *value = g_match_info_fetch(match_info_inner, 2);

            g_hash_table_insert(parameters, key, value);

            g_match_info_next(match_info_inner, NULL);
        }
        g_match_info_free(match_info_inner);
    }
    g_match_info_free(match_info);

    return parameters;
}

gboolean gw_audio_player_play(GwAudioPlayer *self,
                              GwTrace **traces,
                              GwDumpFile *dump_file,
                              GwTimeRange *range,
                              GError **error)
{
    g_return_val_if_fail(GW_IS_AUDIO_PLAYER(self), FALSE);
    g_return_val_if_fail(traces != NULL, FALSE);
    g_return_val_if_fail(GW_IS_TIME_RANGE(range), FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    for (GwTrace **iter = traces; *iter != NULL; iter++) {
        GwTrace *trace = *iter;
        g_clear_object(&trace->audio_track);

        GHashTable *parameters = gw_audio_player_parse_trace_parameters(self, trace);

        trace->audio_track = gw_audio_track_new(trace, dump_file, parameters, error);
        if (trace->audio_track == NULL) {
            return FALSE;
        }

        if (parameters != NULL) {
            g_hash_table_unref(parameters);
        }
    }

    if (!gw_audio_player_build_pipeline(self, traces)) {
        return FALSE;
    }

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(self->pipeline));
    gst_bus_add_watch(bus, bus_watch, self);

    GwTime start = gw_time_range_get_start(range);
    GwTime end = gw_time_range_get_end(range);
    GwTime dimension = gw_dump_file_get_time_dimension(dump_file);
    start = gw_time_rescale(start, dimension, GW_TIME_DIMENSION_NANO);
    end = gw_time_rescale(end, dimension, GW_TIME_DIMENSION_NANO);

    g_debug("playing range: %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT,
            GST_TIME_ARGS(start),
            GST_TIME_ARGS(end));

    gst_element_set_state(self->pipeline, GST_STATE_PAUSED);

    if (!gst_element_seek(self->pipeline,
                          1.0,
                          GST_FORMAT_TIME,
                          GST_SEEK_FLAG_FLUSH,
                          GST_SEEK_TYPE_SET,
                          start,
                          GST_SEEK_TYPE_SET,
                          end)) {
        g_warning("failed to seek");
    }
    gst_element_set_state(self->pipeline, GST_STATE_PLAYING);

    g_timeout_add(16, update, self);

    return TRUE;
}

void gw_audio_player_stop(GwAudioPlayer *self)
{
    g_return_if_fail(GW_IS_AUDIO_PLAYER(self));

    if (self->pipeline != NULL) {
        gst_element_set_state(self->pipeline, GST_STATE_NULL);
        g_clear_object(&self->pipeline);
    }

    gw_audio_player_set_state(self, GW_AUDIO_PLAYER_STATE_IDLE);
}

GwTime gw_audio_player_get_position(GwAudioPlayer *self)
{
    g_return_val_if_fail(GW_IS_AUDIO_PLAYER(self), 0);

    return self->position;
}
