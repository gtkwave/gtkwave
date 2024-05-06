#include <gtkwave.h>
#include <glib-object.h>

static void print_header(const gchar *title)
{
    g_print("%s\n", title);
    for (gsize i = 0; i < strlen(title); i++) {
        g_print("-");
    }
    g_print("\n");
}

static void print_indent(gint level)
{
    for (; level > 0; level--) {
        g_print("    ");
    }
}

static void print_enum(const gchar *label, GType type, gint value)
{
    gchar *str = g_enum_to_string(type, value);
    g_print("%s: %s\n", label, str);
    g_free(str);
}

static void dump_time(GwDumpFile *file)
{
    print_header("Time");

    g_print("scale: %" GW_TIME_FORMAT "\n", gw_dump_file_get_time_scale(file));
    gchar *dimension =
        g_enum_to_string(GW_TYPE_TIME_DIMENSION, gw_dump_file_get_time_dimension(file));
    g_print("dimension: %s\n", dimension);
    g_free(dimension);

    GwTimeRange *range = gw_dump_file_get_time_range(file);
    g_print("range: %" GW_TIME_FORMAT " - %" GW_TIME_FORMAT "\n",
            gw_time_range_get_start(range),
            gw_time_range_get_end(range));

    g_print("global time offset: %" GW_TIME_FORMAT "\n", gw_dump_file_get_global_time_offset(file));
    g_print("\n");
}

static void dump_tree_rec(GwDumpFile *file, GwTreeNode *tree_node, gint level)
{
    for (GwTreeNode *iter = tree_node; iter != NULL; iter = iter->next) {
        print_indent(level);
        gchar *kind = g_enum_to_string(GW_TYPE_TREE_KIND, iter->kind);
        g_print("%s (kind=%s, t_which=%d", iter->name, kind, iter->t_which);
        g_free(kind);
        if (iter->t_stem != 0) {
            g_print(", t_stem=%d", iter->t_stem);
        }
        if (iter->t_istem != 0) {
            g_print(", t_istem=%d", iter->t_stem);
        }
        g_print(")\n");

        if (iter->child != NULL) {
            dump_tree_rec(file, iter->child, level + 1);
        }
    }
}

static void dump_tree(GwDumpFile *file)
{
    print_header("Tree");

    GwTree *tree = gw_dump_file_get_tree(file);
    dump_tree_rec(file, gw_tree_get_root(tree), 0);

    g_print("\n");
}

static void dump_node(GwDumpFile *file, GwNode *node)
{
    (void)file;

    g_print("    node: %s\n", node->nname);
    if (node->expansion != NULL) {
        g_print("        has_expansion\n");
    }

    print_enum("        vartype", GW_TYPE_VAR_TYPE, node->vartype);
    print_enum("        vardt", GW_TYPE_VAR_DATA_TYPE, node->vardt);
    print_enum("        vardir", GW_TYPE_VAR_DIR, node->vardir);
    g_print("        varxt: %d\n", node->varxt);

    g_print("        extvals: %d\n", node->extvals);
    g_print("        msi, lsi: %d, %d\n", node->msi, node->lsi);
    g_print("        numhist: %d\n", node->numhist);

    g_print("        transitions:\n");

    // TODO: use gw_hist_ent_to_string
    for (GwHistEnt *iter = &node->head; iter != NULL; iter = iter->next) {
        g_print("            ");
        if (iter->flags & (GW_HIST_ENT_FLAG_REAL | GW_HIST_ENT_FLAG_STRING)) {
            if (iter->flags & GW_HIST_ENT_FLAG_STRING) {
                if (iter->time < 0) {
                    g_print("?");
                } else {
                    g_print("\"%s\"", iter->v.h_vector);
                }
            } else {
                g_print("%f", iter->v.h_double);
            }
        } else if (node->msi == node->lsi) {
            g_print("%c", gw_bit_to_char(iter->v.h_val));
        } else {
            // TODO: VCD files with aliased vectors cause a segfault without this check
            if (iter->time < 0) {
                g_print("?");
            } else {
                gint bits = ABS(node->msi - node->lsi + 1);
                for (gint i = 0; i < bits; i++) {
                    g_print("%c", gw_bit_to_char(iter->v.h_vector[i]));
                }
            }
        }
        g_print(" @ %" GW_TIME_FORMAT "\n", iter->time);
    }
}

static void dump_facs(GwDumpFile *file)
{
    print_header("Facs");

    GwFacs *facs = gw_dump_file_get_facs(file);
    for (guint i = 0; i < gw_facs_get_length(facs); i++) {
        GwSymbol *symbol = gw_facs_get(facs, i);
        g_print("%s\n", symbol->name);
        if (symbol->vec_root != NULL) {
            g_print("    vec_root: %s\n", symbol->vec_root->name);
        }
        if (symbol->vec_chain != NULL) {
            g_print("    vec_chain: %s\n", symbol->vec_chain->name);
        }
        dump_node(file, symbol->n);
    }

    g_print("\n");
}

static int pointer_strcmp(const gchar **a, const gchar **b)
{
    return g_strcmp0(*a, *b);
}

static void dump_aliases(GwDumpFile *file)
{
    // Build a hash table to collect nodes with the same second GwHistEnt after head.

    GHashTable *next_pointers = g_hash_table_new(g_direct_hash, g_direct_equal);

    GwFacs *facs = gw_dump_file_get_facs(file);
    for (guint i = 0; i < gw_facs_get_length(facs); i++) {
        GwSymbol *symbol = gw_facs_get(facs, i);
        GwNode *node = symbol->n;

        GwHistEnt *h = node->head.next;

        GSList *old = g_hash_table_lookup(next_pointers, h);
        GSList *new = g_slist_prepend(old, node->nname);
        g_hash_table_replace(next_pointers, h, new);
    }

    // Convert the hash table into a list of strings

    GPtrArray *strings = g_ptr_array_new_with_free_func(g_free);
    GList *groups = g_hash_table_get_values(next_pointers);
    for (GList *iter = groups; iter != NULL; iter = iter->next) {
        GSList *group = iter->data;

        if (g_slist_length(group) >= 2) {
            group = g_slist_sort(group, (GCompareFunc)g_strcmp0);

            GString *s = g_string_new(NULL);
            for (GSList *iter = group; iter != NULL; iter = iter->next) {
                gchar *node_name = iter->data;
                if (s->len > 0) {
                    g_string_append(s, ", ");
                }
                g_string_append(s, node_name);
            }
            g_ptr_array_add(strings, g_string_free(s, FALSE));
        }

        g_slist_free(group);
    }
    g_list_free(groups);
    g_hash_table_destroy(next_pointers);

    // Print aliases if at least one exists

    if (strings->len > 0) {
        g_ptr_array_sort(strings, (GCompareFunc)pointer_strcmp);

        print_header("Aliases");
        for (guint i = 0; i < strings->len; i++) {
            g_print("%s\n", (gchar *)g_ptr_array_index(strings, i));
        }
        g_print("\n");
    }

    g_ptr_array_free(strings, TRUE);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        g_printerr("USAGE: %s DUMP_FILE\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];

    GwLoader *loader = NULL;
    if (g_str_has_suffix(filename, ".fst")) {
        loader = gw_fst_loader_new();
    } else if (g_str_has_suffix(filename, ".vcd")) {
        loader = gw_vcd_loader_new();
    } else if (g_str_has_suffix(filename, ".ghw")) {
        loader = gw_ghw_loader_new();
    } else {
        g_error("Unknown filetype");
    }

    GError *error = NULL;
    GwDumpFile *file = gw_loader_load(loader, filename, &error);
    g_object_unref(loader);

    if (file == NULL) {
        g_error("Couldn't load dumpfile: %s", error->message);
    }

    if (!gw_dump_file_import_all(file, &error)) {
        g_error("Couldn't import traces: %s", error->message);
    }
    // TODO: at the moment it is necessary to import twice to correctly handle aliases
    if (!gw_dump_file_import_all(file, &error)) {
        g_error("Couldn't import traces: %s", error->message);
    }

    dump_time(file);
    dump_tree(file);
    dump_facs(file);
    dump_aliases(file);

    g_object_unref(file);

    return EXIT_SUCCESS;
}