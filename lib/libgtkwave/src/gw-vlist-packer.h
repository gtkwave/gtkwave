#pragma once

#include <glib.h>
#include "gw-vlist.h"

typedef struct _GwVlistPacker GwVlistPacker;

GwVlistPacker *gw_vlist_packer_new(gint compression_level);
void gw_vlist_packer_alloc(GwVlistPacker *self, unsigned char ch);
GwVlist *gw_vlist_packer_finalize_and_free(GwVlistPacker *self);

unsigned char *gw_vlist_packer_decompress(GwVlist *vl, unsigned int *declen);
void gw_vlist_packer_decompress_destroy(guchar *mem);