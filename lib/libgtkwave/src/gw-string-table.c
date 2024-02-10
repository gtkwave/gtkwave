#include "gw-string-table.h"

// TODO: Use blocks to allocate strings.

struct _GwStringTable
{
    GObject parent_instance;

    GHashTable *hash_table;
    GPtrArray *array;
};

G_DEFINE_TYPE(GwStringTable, gw_string_table, G_TYPE_OBJECT)

static void gw_string_table_finalize(GObject *object)
{
    GwStringTable *self = GW_STRING_TABLE(object);

    g_clear_pointer(&self->hash_table, g_hash_table_destroy);
    g_ptr_array_free(self->array, TRUE);

    G_OBJECT_CLASS(gw_string_table_parent_class)->finalize(object);
}

static void gw_string_table_class_init(GwStringTableClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_string_table_finalize;
}

static void gw_string_table_init(GwStringTable *self)
{
    self->hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    self->array = g_ptr_array_new_with_free_func(g_free);
}

GwStringTable *gw_string_table_new(void)
{
    return g_object_new(GW_TYPE_STRING_TABLE, NULL);
}

static gboolean gw_string_table_is_frozen(GwStringTable *self)
{
    return self->hash_table == NULL;
}

guint gw_string_table_add(GwStringTable *self, const gchar *str)
{
    g_return_val_if_fail(GW_IS_STRING_TABLE(self), 0);
    g_return_val_if_fail(!gw_string_table_is_frozen(self), 0);
    g_return_val_if_fail(str != NULL && str[0] != '\0', 0);

    // Check if string is already in the string table.

    gpointer value = NULL;
    if (g_hash_table_lookup_extended(self->hash_table, str, NULL, &value)) {
        return GPOINTER_TO_UINT(value);
    }

    // Add new string to hash table and array.

    gchar *str_dup = g_strdup(str);

    guint index = self->array->len;
    g_ptr_array_add(self->array, str_dup);
    g_hash_table_insert(self->hash_table, str_dup, GUINT_TO_POINTER(index));

    return index;
}

const gchar *gw_string_table_get(GwStringTable *self, guint index)
{
    g_return_val_if_fail(GW_IS_STRING_TABLE(self), NULL);
    g_return_val_if_fail(index < self->array->len, NULL);

    return g_ptr_array_index(self->array, index);
}

void gw_string_table_freeze(GwStringTable *self)
{
    g_return_if_fail(GW_IS_STRING_TABLE(self));

    g_clear_pointer(&self->hash_table, g_hash_table_destroy);
}