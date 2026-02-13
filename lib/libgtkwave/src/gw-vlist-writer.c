#include "gw-vlist-writer.h"
#include "gw-vlist.h"
#include "gw-vlist-packer.h"
#include "gw-bit.h"
#include <zlib.h>

struct _GwVlistWriter
{
    GObject parent_instance;

    gint compression_level;
    gboolean prepack;

    GwVlist *vlist;
    GwVlistPacker *packer;

    gboolean is_live;
};

G_DEFINE_TYPE(GwVlistWriter, gw_vlist_writer, G_TYPE_OBJECT)

enum
{
    PROP_COMPRESSION_LEVEL = 1,
    PROP_PREPACK,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_vlist_writer_dispose(GObject *object)
{
    GwVlistWriter *self = GW_VLIST_WRITER(object);



    g_clear_pointer(&self->packer, gw_vlist_packer_finalize_and_free);
    
    // Only destroy vlist if it hasn't been transferred (e.g., via finish())
    if (self->vlist != NULL) {
        g_clear_pointer(&self->vlist, gw_vlist_destroy);
    }

    G_OBJECT_CLASS(gw_vlist_writer_parent_class)->dispose(object);
}

static void gw_vlist_writer_finalize(GObject *object)
{
    GwVlistWriter *self = GW_VLIST_WRITER(object);

    // vlist and packer are already freed in dispose()

    G_OBJECT_CLASS(gw_vlist_writer_parent_class)->finalize(object);
}

static void gw_vlist_writer_constructed(GObject *object)
{
    GwVlistWriter *self = GW_VLIST_WRITER(object);

    G_OBJECT_CLASS(gw_vlist_writer_parent_class)->constructed(object);

    if (self->prepack) {
        self->packer = gw_vlist_packer_new(self->compression_level);
    } else {
        self->vlist = gw_vlist_create(1);
    }
}

static void gw_vlist_writer_set_property(GObject *object,
                                         guint property_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
    GwVlistWriter *self = GW_VLIST_WRITER(object);

    switch (property_id) {
        case PROP_COMPRESSION_LEVEL:
            self->compression_level = g_value_get_int(value);
            break;

        case PROP_PREPACK:
            self->prepack = g_value_get_boolean(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_vlist_writer_class_init(GwVlistWriterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->dispose = gw_vlist_writer_dispose;
    object_class->finalize = gw_vlist_writer_finalize;
    object_class->constructed = gw_vlist_writer_constructed;
    object_class->set_property = gw_vlist_writer_set_property;

    properties[PROP_COMPRESSION_LEVEL] =
        g_param_spec_int("compression-level",
                         NULL,
                         NULL,
                         Z_DEFAULT_COMPRESSION /* -1 */,
                         Z_BEST_COMPRESSION /* 9 */,
                         Z_DEFAULT_COMPRESSION,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    properties[PROP_PREPACK] =
        g_param_spec_boolean("prepack",
                             NULL,
                             NULL,
                             FALSE,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_vlist_writer_init(GwVlistWriter *self)
{
    self->is_live = FALSE;
}

GwVlistWriter *gw_vlist_writer_new(gint compression_level, gboolean prepack)
{
    prepack = !!prepack;

    // clang-format off
    return g_object_new(GW_TYPE_VLIST_WRITER,
                        "compression-level", compression_level,
                        "prepack", prepack,
                        NULL);
    // clang-format on
}

void gw_vlist_writer_append_uv32(GwVlistWriter *self, guint32 value)
{
    g_return_if_fail(GW_IS_VLIST_WRITER(self));

    while (TRUE) {
        guint32 next = value >> 7;
        if (next == 0) {
            break;
        }

        if (self->packer != NULL) {
            gw_vlist_packer_alloc(self->packer, value & 0x7f);
        } else {
            char *pnt = gw_vlist_alloc(&self->vlist, TRUE, self->compression_level);
            *pnt = (value & 0x7f);
        }

        value = next;
    }

    if (self->packer != NULL) {
        gw_vlist_packer_alloc(self->packer, (value & 0x7f) | 0x80);
    } else {
        char *pnt = gw_vlist_alloc(&self->vlist, TRUE, self->compression_level);
        *pnt = (value & 0x7f) | 0x80;
    }
}

void gw_vlist_writer_append_string(GwVlistWriter *self, const gchar *str)
{
    g_return_if_fail(GW_IS_VLIST_WRITER(self));
    g_return_if_fail(str != NULL);

    for (const gchar *iter = str; *iter != '\0'; iter++) {
        if (self->packer != NULL) {
            gw_vlist_packer_alloc(self->packer, *iter);
        } else {
            char *pnt = gw_vlist_alloc(&self->vlist, TRUE, self->compression_level);
            *pnt = *iter;
        }
    }

    if (self->packer != NULL) {
        gw_vlist_packer_alloc(self->packer, 0);
    } else {
        char *pnt = gw_vlist_alloc(&self->vlist, TRUE, self->compression_level);
        *pnt = 0;
    }
}

void gw_vlist_writer_append_mvl9_string(GwVlistWriter *self, const char *str)
{
    g_return_if_fail(GW_IS_VLIST_WRITER(self));
    g_return_if_fail(str != NULL);

    unsigned char which = 0;
    unsigned char accum = 0;
    unsigned int recoded_bit;

    for (const char *iter = str; *iter != '\0'; iter++) {
        switch (*iter) {
            case '0':
                recoded_bit = GW_BIT_0;
                break;
            case '1':
                recoded_bit = GW_BIT_1;
                break;
            case 'x':
            case 'X':
                recoded_bit = GW_BIT_X;
                break;
            case 'z':
            case 'Z':
                recoded_bit = GW_BIT_Z;
                break;
            case 'h':
            case 'H':
                recoded_bit = GW_BIT_H;
                break;
            case 'u':
            case 'U':
                recoded_bit = GW_BIT_U;
                break;
            case 'w':
            case 'W':
                recoded_bit = GW_BIT_W;
                break;
            case 'l':
            case 'L':
                recoded_bit = GW_BIT_L;
                break;
            default:
                recoded_bit = GW_BIT_DASH;
                break;
        }

        if (which == 0) {
            accum = recoded_bit << 4;
            which = 1;
        } else {
            accum |= recoded_bit;

            if (self->packer != NULL) {
                gw_vlist_packer_alloc(self->packer, accum);
            } else {
                char *pnt = gw_vlist_alloc(&self->vlist, TRUE, self->compression_level);
                *pnt = accum;
            }

            which = 0;
            accum = 0;
        }
    }

    /* XXX : this is assumed it is going to remain a 4 bit max quantity! */
    recoded_bit = GW_BIT_MASK;

    if (which == 0) {
        accum = recoded_bit << 4;
    } else {
        accum |= recoded_bit;
    }

    if (self->packer != NULL) {
        gw_vlist_packer_alloc(self->packer, accum);
    } else {
        char *pnt = gw_vlist_alloc(&self->vlist, TRUE, self->compression_level);
        *pnt = accum;
    }
}

GwVlist *gw_vlist_writer_finish(GwVlistWriter *self)
{
    g_return_val_if_fail(GW_IS_VLIST_WRITER(self), NULL);

    GwVlist *vlist = NULL;

    if (self->packer != NULL) {
        vlist = gw_vlist_packer_finalize_and_free(g_steal_pointer(&self->packer));
        self->packer = NULL; // Explicitly clear after stealing
    } else {
        vlist = g_steal_pointer(&self->vlist);
        self->vlist = NULL; // Explicitly clear after stealing
    }

    g_assert_nonnull(vlist);
    gw_vlist_freeze(&vlist, self->compression_level);

    return vlist;
}

/**
 * gw_vlist_writer_set_live_mode:
 * @self: A #GwVlistWriter.
 * @is_live: Whether live mode should be enabled.
 *
 * Sets the writer to live mode. In live mode, prepacking is disallowed and
 * compression is deferred until finish() is called. This allows a reader
 * to read from the writer's buffer as it is being populated.
 */
void gw_vlist_writer_set_live_mode(GwVlistWriter *self, gboolean is_live)
{
    g_return_if_fail(GW_IS_VLIST_WRITER(self));

    // Enforce the rule: live mode cannot be used with prepacking.
    if (is_live) {
        g_return_if_fail(self->prepack == FALSE);
    }

    self->is_live = !!is_live; // Coerce to a pure boolean
}

GwVlist *gw_vlist_writer_get_vlist(GwVlistWriter *self)
{
    g_return_val_if_fail(GW_IS_VLIST_WRITER(self), NULL);
    return self->vlist;
}

gboolean gw_vlist_writer_get_is_live(GwVlistWriter *self)
{
    g_return_val_if_fail(GW_IS_VLIST_WRITER(self), FALSE);
    return self->is_live;
}

gboolean gw_vlist_writer_get_prepack(GwVlistWriter *self)
{
    g_return_val_if_fail(GW_IS_VLIST_WRITER(self), FALSE);
    return self->prepack;
}