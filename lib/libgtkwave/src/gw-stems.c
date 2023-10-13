#include "gw-stems.h"

typedef struct
{
    guint32 path_index;
    guint32 line_number;
} GwStemInternal;

static const GwStem ERROR_STEM = {
    .path = "ERROR",
    .line_number = 0,
};

struct _GwStems
{
    GObject parent_instance;

    GPtrArray *paths;
    GArray *stems;
    GArray *istems;
};

G_DEFINE_TYPE(GwStems, gw_stems, G_TYPE_OBJECT)

static void gw_stems_finalize(GObject *object)
{
    GwStems *self = GW_STEMS(object);

    g_ptr_array_free(self->paths, TRUE);
    g_array_free(self->stems, TRUE);
    g_array_free(self->istems, TRUE);

    G_OBJECT_CLASS(gw_stems_parent_class)->finalize(object);
}

static void gw_stems_class_init(GwStemsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_stems_finalize;
}

static void gw_stems_init(GwStems *self)
{
    self->paths = g_ptr_array_new_with_free_func(g_free);
    self->stems = g_array_new(FALSE, FALSE, sizeof(GwStemInternal));
    self->istems = g_array_new(FALSE, FALSE, sizeof(GwStemInternal));
}

GwStems *gw_stems_new(void)
{
    return g_object_new(GW_TYPE_STEMS, NULL);
}

void gw_stems_shrink_to_fit(GwStems *self)
{
    g_return_if_fail(GW_IS_STEMS(self));

    g_ptr_array_set_size(self->paths, self->paths->len);
    g_array_set_size(self->stems, self->stems->len);
    g_array_set_size(self->istems, self->istems->len);
}

guint32 gw_stems_add_path(GwStems *self, const gchar *path)
{
    g_return_val_if_fail(GW_IS_STEMS(self), 0);

    g_ptr_array_add(self->paths, g_strdup(path));

    return self->paths->len;
}

static guint32 add_stem_common(GArray *array, guint32 path_index, guint32 line_number)
{
    GwStemInternal stem = (GwStemInternal){
        .path_index = path_index,
        .line_number = line_number,

    };
    g_array_append_val(array, stem);

    return array->len;
}

guint32 gw_stems_add_stem(GwStems *self, guint32 path_index, guint32 line_number)
{
    g_return_val_if_fail(GW_IS_STEMS(self), 0);

    return add_stem_common(self->stems, path_index, line_number);
}

guint32 gw_stems_add_istem(GwStems *self, guint32 path_index, guint32 line_number)
{
    g_return_val_if_fail(GW_IS_STEMS(self), 0);

    return add_stem_common(self->istems, path_index, line_number);
}

GwStem gw_stems_get_common(GwStems *self, GArray *stems, guint32 index)
{
    g_return_val_if_fail(index > 0 && index <= stems->len, ERROR_STEM);

    GwStemInternal *stem = &g_array_index(stems, GwStemInternal, index - 1);

    g_return_val_if_fail(gw_stems_is_path_index_valid(self, stem->path_index), ERROR_STEM);

    return (GwStem){
        .path = g_ptr_array_index(self->paths, stem->path_index - 1),
        .line_number = stem->line_number,
    };
}

GwStem gw_stems_get_stem(GwStems *self, guint32 index)
{
    g_return_val_if_fail(GW_IS_STEMS(self), ERROR_STEM);

    return gw_stems_get_common(self, self->stems, index);
}

GwStem gw_stems_get_istem(GwStems *self, guint32 index)
{
    g_return_val_if_fail(GW_IS_STEMS(self), ERROR_STEM);

    return gw_stems_get_common(self, self->istems, index);
}

gboolean gw_stems_is_path_index_valid(GwStems *self, guint32 path_index)
{
    g_return_val_if_fail(GW_IS_STEMS(self), FALSE);

    return path_index > 0 && path_index <= self->paths->len;
}

guint32 gw_stems_get_next_path_index(GwStems *self)
{
    g_return_val_if_fail(GW_IS_STEMS(self), FALSE);

    return self->paths->len + 1;
}

gboolean gw_stems_is_empty(GwStems *self)
{
    g_return_val_if_fail(GW_IS_STEMS(self), TRUE);

    return self->paths->len == 0 || (self->stems->len == 0 && self->istems->len == 0);
}

gboolean gw_stems_has_istems(GwStems *self)
{
    g_return_val_if_fail(GW_IS_STEMS(self), FALSE);

    return self->istems->len != 0;
}
