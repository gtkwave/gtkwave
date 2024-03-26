#include "gw-hist-ent-factory.h"

#define BLOCK_SIZE (64 * 1024)
#define HIST_ENTS_PER_BLOCK (BLOCK_SIZE / sizeof(GwHistEnt))

struct _GwHistEntFactory
{
    GObject parent_instance;

    GPtrArray *blocks;
    GwHistEnt *current_block;
    gint next_index;
};

G_DEFINE_TYPE(GwHistEntFactory, gw_hist_ent_factory, G_TYPE_OBJECT)

static void gw_hist_ent_factory_finalize(GObject *object)
{
    GwHistEntFactory *self = GW_HIST_ENT_FACTORY(object);

    g_ptr_array_free(self->blocks, TRUE);

    G_OBJECT_CLASS(gw_hist_ent_factory_parent_class)->finalize(object);
}

static void gw_hist_ent_factory_class_init(GwHistEntFactoryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_hist_ent_factory_finalize;
}

static void gw_hist_ent_factory_init(GwHistEntFactory *self)
{
    self->blocks = g_ptr_array_new_with_free_func(g_free);
}

GwHistEntFactory *gw_hist_ent_factory_new(void)
{
    return g_object_new(GW_TYPE_HIST_ENT_FACTORY, NULL);
}

GwHistEnt *gw_hist_ent_factory_alloc(GwHistEntFactory *self)
{
    g_return_val_if_fail(GW_IS_HIST_ENT_FACTORY(self), NULL);

    if (self->next_index == HIST_ENTS_PER_BLOCK || self->blocks->len == 0) {
        self->current_block = g_malloc0(BLOCK_SIZE);

        g_ptr_array_add(self->blocks, self->current_block);
        self->next_index = 0;
    }

    GwHistEnt *h = &self->current_block[self->next_index];

    self->next_index++;

    return h;
}