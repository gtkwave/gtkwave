#include "gw-vlist-reader.h"
#include "gw-vlist.h"
#include "gw-vlist-packer.h"
#include "gw-vlist-writer.h"
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

    GString *string_buffer;

    GwVlistWriter *live_writer_source;
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

    // Only destroy the vlist if it's not owned by a live writer
    if (self->live_writer_source == NULL) {
        g_clear_pointer(&self->vlist, gw_vlist_destroy);
    } else {
        // For live readers, the vlist is owned by the writer
        // Don't destroy it here, just clear the pointer
        self->vlist = NULL;
    }
    g_clear_pointer(&self->depacked, gw_vlist_packer_decompress_destroy);
    g_string_free(self->string_buffer, TRUE);
    g_clear_object(&self->live_writer_source);

    G_OBJECT_CLASS(gw_vlist_reader_parent_class)->finalize(object);
}

static void gw_vlist_reader_constructed(GObject *object)
{
    GwVlistReader *self = GW_VLIST_READER(object);

    G_OBJECT_CLASS(gw_vlist_reader_parent_class)->constructed(object);

    if (self->prepacked) {
        self->depacked = gw_vlist_packer_decompress(self->vlist, &self->size);
        g_clear_pointer(&self->vlist, gw_vlist_destroy);
    } else if (self->live_writer_source != NULL) {
        // For live readers, size will be updated dynamically
        self->size = 0;
    } else if (self->vlist != NULL) {
        self->size = gw_vlist_size(self->vlist);
    } else {
        self->size = 0;
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
    self->string_buffer = g_string_new(NULL);
    self->live_writer_source = NULL;
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

/**
 * gw_vlist_reader_new_from_writer:
 * @writer: A #GwVlistWriter in live mode.
 *
 * Creates a new reader that can read directly from the buffer of an
 * un-finalized writer. The writer must have been set to live mode.
 *
 * Returns: (transfer full): A new #GwVlistReader.
 */
GwVlistReader *gw_vlist_reader_new_from_writer(GwVlistWriter *writer)
{
    g_return_val_if_fail(GW_IS_VLIST_WRITER(writer), NULL);
    // Ensure the writer is properly configured for this operation
    g_return_val_if_fail(gw_vlist_writer_get_is_live(writer) == TRUE, NULL);
    g_return_val_if_fail(gw_vlist_writer_get_prepack(writer) == FALSE, NULL);

    GwVlistReader *self = g_object_new(GW_TYPE_VLIST_READER, NULL);

    self->live_writer_source = g_object_ref(writer); // Keep the writer alive
    self->vlist = gw_vlist_writer_get_vlist(writer); // Borrow the pointer
    self->prepacked = FALSE;
    self->position = 0;
    self->size = self->vlist ? gw_vlist_size(self->vlist) : 0; // Set initial size
    self->depacked = NULL; // Live readers read directly from vlist

    return self;
}

static void _gw_vlist_reader_update_from_live_source(GwVlistReader *self)
{
    if (self->live_writer_source) {
        // Sync the size with how much data has been written
        GwVlist *vlist = gw_vlist_writer_get_vlist(self->live_writer_source);
        if (vlist != NULL) {
            self->size = gw_vlist_size(vlist);
        } else {
            self->size = 0;
        }
    }
}

gint gw_vlist_reader_next(GwVlistReader *self)
{
    _gw_vlist_reader_update_from_live_source(self);
    g_return_val_if_fail(GW_IS_VLIST_READER(self), -1);

    if (self->position >= self->size) {
        return -1;
    }

    guint8 value = 0;
    if (self->depacked != NULL) {
        value = self->depacked[self->position];
    } else if (self->live_writer_source != NULL) {
        GwVlist *vlist = gw_vlist_writer_get_vlist(self->live_writer_source);
        if (vlist != NULL) {
            void *location = gw_vlist_locate(vlist, self->position);
            if (location != NULL) {
                value = *(guint8 *)location;
            } else {
                return -1; // Invalid position or corrupted vlist
            }
        } else {
            return -1; // Writer has no vlist
        }
    } else {
        void *location = gw_vlist_locate(self->vlist, self->position);
        if (location != NULL) {
            value = *(guint8 *)location;
        } else {
            return -1; // Invalid position or corrupted vlist
        }
    }

    self->position++;

    return value;
}

guint32 gw_vlist_reader_read_uv32(GwVlistReader *self)
{
    _gw_vlist_reader_update_from_live_source(self);
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

const gchar *gw_vlist_reader_read_string(GwVlistReader *self)
{
    _gw_vlist_reader_update_from_live_source(self);
    g_return_val_if_fail(GW_IS_VLIST_READER(self), NULL);

    g_string_truncate(self->string_buffer, 0);

    while (TRUE) {
        gint c = gw_vlist_reader_next(self);
        if (c <= 0) {
            break;
        }
        g_string_append_c(self->string_buffer, c);
    };

    return self->string_buffer->str;
}

void gw_vlist_reader_set_position(GwVlistReader *self, guint position)
{
    _gw_vlist_reader_update_from_live_source(self);
    g_return_if_fail(GW_IS_VLIST_READER(self));

    // Clamp the position to be within valid bounds
    if (position > self->size) {
        self->position = self->size;
    } else {
        self->position = position;
    }
}

gboolean gw_vlist_reader_is_done(GwVlistReader *self)
{
    _gw_vlist_reader_update_from_live_source(self);
    g_return_val_if_fail(GW_IS_VLIST_READER(self), TRUE);

    return self->position >= self->size;
}