/*
 * Copyright (c) Tony Bybell 1999-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef WAVE_PIXMAPS_H
#define WAVE_PIXMAPS_H

#include <gtk/gtk.h>

#define WAVE_SPLASH_X (512)
#define WAVE_SPLASH_Y (384)

void make_splash_pixmaps(GtkWidget *window);

typedef struct
{
    GdkPixbuf *module;
    GdkPixbuf *task;
    GdkPixbuf *function;
    GdkPixbuf *begin;
    GdkPixbuf *fork;
    GdkPixbuf *interface;
    GdkPixbuf *svpackage;
    GdkPixbuf *program;
    GdkPixbuf *class;
    GdkPixbuf *record;
    GdkPixbuf *generate;
    GdkPixbuf *design;
    GdkPixbuf *block;
    GdkPixbuf *generateif;
    GdkPixbuf *generatefor;
    GdkPixbuf *instance;
    GdkPixbuf *package;
    GdkPixbuf *signal;
    GdkPixbuf *portin;
    GdkPixbuf *portout;
    GdkPixbuf *portinout;
    GdkPixbuf *buffer;
    GdkPixbuf *linkage;
} GwHierarchyIcons;

GwHierarchyIcons *gw_hierarchy_icons_new(void);
GdkPixbuf *gw_hierarchy_icons_get(GwHierarchyIcons *self, guint tree_kind);

#endif
