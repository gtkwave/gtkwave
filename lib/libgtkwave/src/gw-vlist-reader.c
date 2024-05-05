#include "gw-vlist-reader.h"
#include "gw-vlist.h"
#include "gw-vlist-packer.h"
#include "gw-bit.h"
#include <zlib.h>

struct _GwVlistReader
{
    GObject parent_instance;

    GwVlist *vlist;
    guint8 *depacked;
    gboolean prepacked;

    guint position;
    guint size;
};

G_DEFINE_TYPE(GwVlistReader, gw_vlist_reader, G_TYPE_OBJECT)

enum
{
    PROP_VLIST = 1,
    PROP_PREPACKED,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_vlist_reader_finalize(GObject *object)
{
    GwVlistReader *self = GW_VLIST_READER(object);

    g_clear_pointer(&self->vlist, gw_vlist_destroy);
    g_clear_pointer(&self->depacked, gw_vlist_packer_decompress_destroy);

    G_OBJECT_CLASS(gw_vlist_reader_parent_class)->finalize(object);
}

static void gw_vlist_reader_constructed(GObject *object)
{
    GwVlistReader *self = GW_VLIST_READER(object);

    G_OBJECT_CLASS(gw_vlist_reader_parent_class)->constructed(object);

    if (self->prepacked) {
        self->depacked = gw_vlist_packer_decompress(self->vlist, &self->size);
        g_clear_pointer(&self->vlist, gw_vlist_destroy);
    } else {
        self->size = gw_vlist_size(self->vlist);
    }
}

static void gw_vlist_reader_set_property(GObject *object,
                                         guint property_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
    GwVlistReader *self = GW_VLIST_READER(object);

    switch (property_id) {
        case PROP_VLIST:
            self->vlist = g_value_get_pointer(value);
            break;

        case PROP_PREPACKED:
            self->prepacked = g_value_get_boolean(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_vlist_reader_class_init(GwVlistReaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_vlist_reader_finalize;
    object_class->constructed = gw_vlist_reader_constructed;
    object_class->set_property = gw_vlist_reader_set_property;

    properties[PROP_VLIST] =
        g_param_spec_pointer("vlist",
                             NULL,
                             NULL,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    properties[PROP_PREPACKED] =
        g_param_spec_boolean("prepacked",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_vlist_reader_init(GwVlistReader *self)
{
    (void)self;
}

GwVlistReader *gw_vlist_reader_new(GwVlist *vlist, gboolean prepacked)
{
    // clang-format off
    return g_object_new(GW_TYPE_VLIST_READER,
                        "vlist", vlist,
                        "prepacked", prepacked,
                        NULL);
    // clang-format on
}

gint gw_vlist_reader_next(GwVlistReader *self)
{
    g_return_val_if_fail(GW_IS_VLIST_READER(self), -1);

    if (self->position >= self->size) {
        return -1;
    }

    guint8 value = 0;
    if (self->depacked != NULL) {
        value = self->depacked[self->position];
    } else {
        value = *(guint *)gw_vlist_locate(self->vlist, self->position);
    }

    self->position++;

    return value;
}

guint32 gw_vlist_reader_read_uv32(GwVlistReader *self)
{
    g_return_val_if_fail(GW_IS_VLIST_READER(self), 0);

    guint8 arr[5];
    gint arr_pos = 0;

    gint c = 0;
    do {
        c = gw_vlist_reader_next(self);
        if (c < 0) {
            break;
        }
        g_assert_cmpint(arr_pos, <, 5);
        arr[arr_pos++] = c & 0x7F;
    } while ((c & 0x80) == 0);

    g_assert_cmpint(arr_pos, >, 0);

    guint32 accum = 0;
    for (--arr_pos; arr_pos >= 0; arr_pos--) {
        guint8 c = arr[arr_pos];
        accum <<= 7;
        accum |= c;
    }

    return accum;
}

gboolean gw_vlist_reader_is_done(GwVlistReader *self)
{
    g_return_val_if_fail(GW_IS_VLIST_READER(self), TRUE);

    return self->position >= self->size;
}