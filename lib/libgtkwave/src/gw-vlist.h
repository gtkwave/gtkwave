#pragma once

#include <glib.h>

typedef struct _GwVlist GwVlist;

struct _GwVlist
{
    GwVlist *next;
    guint size;
    guint offset;
    guint element_size;
};

GwVlist *gw_vlist_create(guint elem_siz);
void gw_vlist_destroy(GwVlist *v);
void *gw_vlist_alloc(GwVlist **v, gboolean compressable, gint compression_level);
guint gw_vlist_size(GwVlist *v);
void *gw_vlist_locate(GwVlist *v, guint idx);
void gw_vlist_freeze(GwVlist **v, gint compression_level);
void gw_vlist_uncompress(GwVlist **v);
