#include "gw-tree.h"

static int sigcmp(char *s1, char *s2);

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

    return sigcmp(t2->name, t1->name); /* because list must be in rvs */
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

/*
 * compares two facilities a la strcmp but preserves
 * numbers for comparisons
 *
 * there are two flavors..the slow and accurate to any
 * arbitrary number of digits version (first) and the
 * fast one good to 2**31-1.  we default to the faster
 * version since there's probably no real need to
 * process ints larger than two billion anyway...
 */

#ifdef WAVE_USE_SIGCMP_INFINITE_PRECISION
#if __STDC_VERSION__ < 199901L
inline
#endif
    int
    sigcmp_2(char *s1, char *s2)
{
    char *n1, *n2;
    unsigned char c1, c2;
    int len1, len2;

    for (;;) {
        c1 = (unsigned char)*s1;
        c2 = (unsigned char)*s2;

        if ((c1 == 0) && (c2 == 0))
            return (0);
        if ((c1 >= '0') && (c1 <= '9') && (c2 >= '0') && (c2 <= '9')) {
            n1 = s1;
            n2 = s2;
            len1 = len2 = 0;

            do {
                len1++;
                c1 = (unsigned char)*(n1++);
            } while ((c1 >= '0') && (c1 <= '9'));
            if (!c1)
                n1--;

            do {
                len2++;
                c2 = (unsigned char)*(n2++);
            } while ((c2 >= '0') && (c2 <= '9'));
            if (!c2)
                n2--;

            do {
                if (len1 == len2) {
                    c1 = (unsigned char)*(s1++);
                    len1--;
                    c2 = (unsigned char)*(s2++);
                    len2--;
                } else if (len1 < len2) {
                    c1 = '0';
                    c2 = (unsigned char)*(s2++);
                    len2--;
                } else {
                    c1 = (unsigned char)*(s1++);
                    len1--;
                    c2 = '0';
                }

                if (c1 != c2)
                    return ((int)c1 - (int)c2);
            } while (len1);

            s1 = n1;
            s2 = n2;
            continue;
        } else {
            if (c1 != c2)
                return ((int)c1 - (int)c2);
        }

        s1++;
        s2++;
    }
}
#else
#if __STDC_VERSION__ < 199901L
inline
#endif
    int
    sigcmp_2(char *s1, char *s2)
{
    unsigned char c1, c2;
    int u1, u2;

    for (;;) {
        c1 = (unsigned char)*(s1++);
        c2 = (unsigned char)*(s2++);

        if (!(c1 | c2))
            return (0); /* removes extra branch through logical or */
        if ((c1 <= '9') && (c2 <= '9') && (c2 >= '0') && (c1 >= '0')) {
            u1 = (int)(c1 & 15);
            u2 = (int)(c2 & 15);

            while (((c2 = (unsigned char)*s2) >= '0') && (c2 <= '9')) {
                u2 *= 10;
                u2 += (unsigned int)(c2 & 15);
                s2++;
            }

            while (((c2 = (unsigned char)*s1) >= '0') && (c2 <= '9')) {
                u1 *= 10;
                u1 += (unsigned int)(c2 & 15);
                s1++;
            }

            if (u1 == u2)
                continue;
            else
                return ((int)u1 - (int)u2);
        } else {
            if (c1 != c2)
                return ((int)c1 - (int)c2);
        }
    }
}
#endif

// TODO: move
static int sigcmp(char *s1, char *s2)
{
    int rc = sigcmp_2(s1, s2);
    if (!rc) {
        rc = strcmp(s1, s2); /* to handle leading zero "0" vs "00" cases ... we provide a definite
                                order so bsearch doesn't fail */
    }

    return (rc);
}