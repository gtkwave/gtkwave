#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct
{
    const char *path;
    guint32 line_number;
} GwStem;

#define GW_TYPE_STEMS (gw_stems_get_type())
G_DECLARE_FINAL_TYPE(GwStems, gw_stems, GW, STEMS, GObject)

GwStems *gw_stems_new(void);
void gw_stems_shrink_to_fit(GwStems *self);

guint32 gw_stems_add_path(GwStems *self, const gchar *path);
guint32 gw_stems_add_stem(GwStems *self, guint32 path_index, guint32 line_number);
guint32 gw_stems_add_istem(GwStems *self, guint32 path_index, guint32 line_number);

GwStem gw_stems_get_stem(GwStems *self, guint32 index);
GwStem gw_stems_get_istem(GwStems *self, guint32 index);

gboolean gw_stems_is_path_index_valid(GwStems *self, guint32 path_index);
guint32 gw_stems_get_next_path_index(GwStems *self);
gboolean gw_stems_is_empty(GwStems *self);
gboolean gw_stems_has_istems(GwStems *self);

G_END_DECLS
