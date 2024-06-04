#include "gw-tree-builder.h"

struct _GwTreeBuilder
{
    GObject parent_instance;

    GwTreeNode *root;
    GPtrArray *scopes;

    GString *name_prefix;
    gchar hierarchy_delimiter;
    gchar hierarchy_delimiter_str[2];
};

G_DEFINE_TYPE(GwTreeBuilder, gw_tree_builder, G_TYPE_OBJECT)

enum
{
    PROP_HIERARCHY_DELIMITER = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_tree_builder_finalize(GObject *object)
{
    GwTreeBuilder *self = GW_TREE_BUILDER(object);

    g_ptr_array_free(self->scopes, TRUE);
    g_string_free(self->name_prefix, TRUE);
    gw_tree_node_free(self->root);

    G_OBJECT_CLASS(gw_tree_builder_parent_class)->finalize(object);
}

static void gw_tree_builder_set_property(GObject *object,
                                         guint property_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
    GwTreeBuilder *self = GW_TREE_BUILDER(object);

    switch (property_id) {
        case PROP_HIERARCHY_DELIMITER:
            self->hierarchy_delimiter = g_value_get_uchar(value);
            self->hierarchy_delimiter_str[0] = self->hierarchy_delimiter;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_tree_builder_class_init(GwTreeBuilderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gw_tree_builder_finalize;
    object_class->set_property = gw_tree_builder_set_property;

    // TODO: use unichar property
    properties[PROP_HIERARCHY_DELIMITER] =
        g_param_spec_uchar("hierarchy-delimiter",
                           NULL,
                           NULL,
                           1,
                           127,
                           '.',
                           G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_tree_builder_init(GwTreeBuilder *self)
{
    self->root = gw_tree_node_new(GW_TREE_KIND_UNKNOWN, "root");

    self->scopes = g_ptr_array_new();
    g_ptr_array_add(self->scopes, self->root);

    self->name_prefix = g_string_new(NULL);
}

GwTreeBuilder *gw_tree_builder_new(gchar hierarchy_delimiter)
{
    return g_object_new(GW_TYPE_TREE_BUILDER, "hierarchy-delimiter", hierarchy_delimiter, NULL);
}

static void gw_tree_builder_update_name_prefix(GwTreeBuilder *self)
{
    g_string_truncate(self->name_prefix, 0);

    // start at 1 to skip root scope
    for (guint i = 1; i < self->scopes->len; i++) {
        GwTreeNode *scope = g_ptr_array_index(self->scopes, i);

        if (self->name_prefix->len > 0) {
            g_string_append_c(self->name_prefix, self->hierarchy_delimiter);
        }

        g_string_append(self->name_prefix, scope->name);
    }
}

GwTreeNode *gw_tree_builder_push_scope(GwTreeBuilder *self, GwTreeKind kind, const gchar *name)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);
    g_return_val_if_fail(name != NULL && name[0] != '\0', NULL);

    GwTreeNode *parent = g_ptr_array_index(self->scopes, self->scopes->len - 1);
    GwTreeNode *scope = NULL;

    if (parent->child == NULL) {
        scope = gw_tree_node_new(kind, name);
        parent->child = scope;
    } else {
        // Search for duplicate scope names
        // TODO: using a linear search to find duplicates is inefficient
        // TODO: add warning if name is equal but other fields are different
        GwTreeNode *iter = parent->child;
        while (TRUE) {
            if (strcmp(iter->name, name) == 0) {
                scope = iter;
                break;
            }

            if (iter->next == NULL) {
                scope = gw_tree_node_new(kind, name);
                iter->next = scope;
                break;
            }
            iter = iter->next;
        }
    }

    g_ptr_array_add(self->scopes, scope);
    gw_tree_builder_update_name_prefix(self);

    return scope;
}

void gw_tree_builder_pop_scope(GwTreeBuilder *self)
{
    g_return_if_fail(GW_IS_TREE_BUILDER(self));

    // Don't pop the root scope
    if (self->scopes->len > 1) {
        g_ptr_array_set_size(self->scopes, self->scopes->len - 1);
    }

    gw_tree_builder_update_name_prefix(self);
}

const gchar *gw_tree_builder_get_name_prefix(GwTreeBuilder *self)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);

    if (self->name_prefix->len > 0) {
        return self->name_prefix->str;
    } else {
        return NULL;
    }
}

static gchar *gw_tree_builder_get_symbol_name_common(GwTreeBuilder *self,
                                                     const gchar *identifier,
                                                     gint num_inidices,
                                                     gint index1,
                                                     gint index2)
{
    // This function temporarily adds the symbol name to the name prefix to
    // prevent additional allocations. After the string is copied into a new
    // buffer the original prefix string is restored by truncating it.
    guint prefix_len = self->name_prefix->len;

    if (self->name_prefix->len > 0) {
        g_string_append_c(self->name_prefix, self->hierarchy_delimiter);
    }
    g_string_append(self->name_prefix, identifier);

    if (num_inidices == 0) {
        // Nothing
    } else if (num_inidices == 1) {
        g_string_append_printf(self->name_prefix, "[%d]", index1);
    } else if (num_inidices == 2) {
        g_string_append_printf(self->name_prefix, "[%d:%d]", index1, index2);
    } else {
        g_return_val_if_reached(NULL);
    }

    gchar *ret = g_malloc(self->name_prefix->len + 1);
    memcpy(ret, self->name_prefix->str, self->name_prefix->len + 1);

    g_string_truncate(self->name_prefix, prefix_len);

    return ret;
}

// TODO: check how much better the performance is with the original implementation below
#if 0
/*
 * fast itoa for decimal numbers
 */
static char *itoa_2(int value, char *result)
{
    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= 10;
        *ptr++ = "9876543210123456789"[9 + (tmp_value - value * 10)];
    } while (value);

    if (tmp_value < 0)
        *ptr++ = '-';
    result = ptr;
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return (result);
}

/*
 * preformatted sprintf statements which remove parsing latency
 */
static int sprintf_2_sd(char *s, char *c, int d)
{
    char *s2 = s;

    while (*c) {
        *(s2++) = *(c++);
    }
    *(s2++) = '[';
    s2 = itoa_2(d, s2);
    *(s2++) = ']';
    *s2 = 0;

    return (s2 - s);
}

static int sprintf_2_sdd(char *s, char *c, int d, int d2)
{
    char *s2 = s;

    while (*c) {
        *(s2++) = *(c++);
    }
    *(s2++) = '[';
    s2 = itoa_2(d, s2);
    *(s2++) = ':';
    s2 = itoa_2(d2, s2);
    *(s2++) = ']';
    *s2 = 0;

    return (s2 - s);
}
#endif

gchar *gw_tree_builder_get_symbol_name(GwTreeBuilder *self, const gchar *identifier)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);
    g_return_val_if_fail(identifier != NULL && identifier[0] != '\0', NULL);

    return gw_tree_builder_get_symbol_name_common(self, identifier, 0, 0, 0);
}

gchar *gw_tree_builder_get_symbol_name_with_one_index(GwTreeBuilder *self,
                                                      const gchar *identifier,
                                                      gint index)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);
    g_return_val_if_fail(identifier != NULL && identifier[0] != '\0', NULL);

    return gw_tree_builder_get_symbol_name_common(self, identifier, 1, index, 0);
}

gchar *gw_tree_builder_get_symbol_name_with_two_indices(GwTreeBuilder *self,
                                                        const gchar *identifier,
                                                        gint index1,
                                                        gint index2)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);
    g_return_val_if_fail(identifier != NULL && identifier[0] != '\0', NULL);

    return gw_tree_builder_get_symbol_name_common(self, identifier, 2, index1, index2);
}

GwTreeNode *gw_tree_builder_build(GwTreeBuilder *self)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);
    g_return_val_if_fail(self->scopes->len > 0, NULL);

    GwTreeNode *root = g_ptr_array_index(self->scopes, 0);
    return g_steal_pointer(&root->child);
}

GwTreeNode *gw_tree_builder_get_current_scope(GwTreeBuilder *self)
{
    g_return_val_if_fail(GW_IS_TREE_BUILDER(self), NULL);

    // Return NULL instead of the root scope
    if (self->scopes->len <= 1) {
        return NULL;
    }

    return g_ptr_array_index(self->scopes, self->scopes->len - 1);
}