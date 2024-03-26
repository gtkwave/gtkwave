#include "gw-tree.h"
#include "gw-util.h"

struct _GwTree
{
    GObject parent_instance;

    GwTreeNode *root;
};

G_DEFINE_TYPE(GwTree, gw_tree, G_TYPE_OBJECT)

enum
{
    PROP_ROOT = 1,
    N_PROPERTIES,
};

static GParamSpec *properties[N_PROPERTIES];

static void gw_tree_set_property(GObject *object,
                                 guint property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
    GwTree *self = GW_TREE(object);

    switch (property_id) {
        case PROP_ROOT:
            self->root = g_value_get_pointer(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_tree_get_property(GObject *object,
                                 guint property_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
    GwTree *self = GW_TREE(object);

    switch (property_id) {
        case PROP_ROOT:
            g_value_set_pointer(value, gw_tree_get_root(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gw_tree_class_init(GwTreeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = gw_tree_set_property;
    object_class->get_property = gw_tree_get_property;

    properties[PROP_ROOT] =
        g_param_spec_pointer("root",
                             NULL,
                             NULL,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void gw_tree_init(GwTree *self)
{
    (void)self;
}

GwTree *gw_tree_new(GwTreeNode *root)
{
    return g_object_new(GW_TYPE_TREE, "root", root, NULL);
}

GwTreeNode *gw_tree_get_root(GwTree *self)
{
    g_return_val_if_fail(GW_IS_TREE(self), NULL);

    return self->root;
}

static int tree_qsort_cmp(const void *v1, const void *v2)
{
    GwTreeNode *t1 = *(GwTreeNode **)v1;
    GwTreeNode *t2 = *(GwTreeNode **)v2;

    return gw_signal_name_compare(t2->name, t1->name); /* because list must be in rvs */
}

static void gw_tree_sort_recursive(GwTree *self,
                                   GwTreeNode *t,
                                   GwTreeNode *p,
                                   GwTreeNode ***tm,
                                   int *tm_siz)
{
    GwTreeNode *it;
    GwTreeNode **srt;
    int cnt;
    int i;

    if (t->next) {
        it = t;
        cnt = 0;
        do {
            cnt++;
            it = it->next;
        } while (it);

        if (cnt > *tm_siz) {
            *tm_siz = cnt;
            if (*tm) {
                g_free(*tm);
            }
            *tm = g_malloc_n(cnt + 1, sizeof(GwTreeNode *));
        }
        srt = *tm;

        for (i = 0; i < cnt; i++) {
            srt[i] = t;
            t = t->next;
        }
        srt[i] = NULL;

        qsort((void *)srt, cnt, sizeof(GwTreeNode *), tree_qsort_cmp);

        if (p) {
            p->child = srt[0];
        } else {
            self->root = srt[0];
        }

        for (i = 0; i < cnt; i++) {
            srt[i]->next = srt[i + 1];
        }

        it = srt[0];
        for (i = 0; i < cnt; i++) {
            if (it->child) {
                gw_tree_sort_recursive(self, it->child, it, tm, tm_siz);
            }
            it = it->next;
        }
    } else if (t->child) {
        gw_tree_sort_recursive(self, t->child, t, tm, tm_siz);
    }
}

void gw_tree_sort(GwTree *self)
{
    g_return_if_fail(GW_IS_TREE(self));

    if (self->root == NULL) {
        return;
    }

    GwTreeNode **tm = NULL;
    int tm_siz = 0;

    gw_tree_sort_recursive(self, self->root, NULL, &tm, &tm_siz);

    g_free(tm);
}

void gw_tree_graft(GwTree *self, GwTreeNode *graft_chain)
{
    g_return_if_fail(GW_IS_TREE(self));
    g_return_if_fail(graft_chain != NULL);

    GwTreeNode *iter = graft_chain;

    while (iter != NULL) {
        GwTreeNode *iter_next = iter->next;
        GwTreeNode *parent = g_steal_pointer(&iter->child);

        if (parent != NULL) {
            if (parent->child != NULL) {
                iter->next = parent->child;
                parent->child = iter;
            } else {
                parent->child = iter;
                iter->next = NULL;
            }
        } else {
            if (self->root != NULL) {
                iter->next = self->root->next;
                self->root->next = iter;
            } else {
                self->root = iter;
                iter->next = NULL;
            }
        }

        iter = iter_next;
    }
}

GwTreeNode *gw_tree_node_new(GwTreeKind kind, const gchar *name)
{
    GwTreeNode *node = g_malloc0(sizeof(GwTreeNode) + strlen(name) + 1);
    node->kind = kind;
    strcpy(node->name, name);

    return node;
}
