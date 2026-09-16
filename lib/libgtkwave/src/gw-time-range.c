#include "gw-time-range.h"
#include "gw-util.h"

struct _GwTimeRange
{
    GObject parent_instance;

    GwTime start;
    GwTime end;
};

G_DEFINE_TYPE(GwTimeRange, gw_time_range, G_TYPE_OBJECT)

enum
{
    PROP_START = 1,
    PROP_END,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_time_range_constructed(GObject *object)
{
    GwTimeRange *self = GW_TIME_RANGE(object);

    G_OBJECT_CLASS(gw_time_range_parent_class)->constructed(object);

    // Ensure that start is always less or equal to end
    if (self->start > self->end) {
        GwTime t = self->start;
        self->start = self->end;
        self->end = t;
    }
}

static void gw_time_range_set_property(GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
    GwTimeRange *self = GW_TIME_RANGE(object);

    switch (property_id) {
        case PROP_START:
            self->start = g_value_get_int64(value);
            break;

        case PROP_END:
            self->end = g_value_get_int64(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_time_range_get_property(GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
    GwTimeRange *self = GW_TIME_RANGE(object);

    switch (property_id) {
        case PROP_START:
            g_value_set_int64(value, gw_time_range_get_start(self));
            break;

        case PROP_END:
            g_value_set_int64(value, gw_time_range_get_end(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_time_range_class_init(GwTimeRangeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->constructed = gw_time_range_constructed;
    object_class->set_property = gw_time_range_set_property;
    object_class->get_property = gw_time_range_get_property;

    properties[PROP_START] =
        gw_param_spec_time("start",
                           NULL,
                           NULL,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_END] =
        gw_param_spec_time("end",
                           NULL,
                           NULL,
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_time_range_init(GwTimeRange *self)
{
    (void)self;
}

GwTimeRange *gw_time_range_new(GwTime start, GwTime end)
{
    return g_object_new(GW_TYPE_TIME_RANGE, "start", start, "end", end, NULL);
}

GwTime gw_time_range_get_start(GwTimeRange *self)
{
    g_return_val_if_fail(GW_IS_TIME_RANGE(self), 0);

    return self->start;
}

GwTime gw_time_range_get_end(GwTimeRange *self)
{
    g_return_val_if_fail(GW_IS_TIME_RANGE(self), 0);

    return self->end;
}

gboolean gw_time_range_contains(GwTimeRange *self, GwTime time)
{
    g_return_val_if_fail(GW_IS_TIME_RANGE(self), FALSE);

    return time >= self->start && time <= self->end;
}
