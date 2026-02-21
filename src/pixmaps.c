/*
 * Copyright (c) Tony Bybell 1999-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "pixmaps.h"

GwHierarchyIcons *gw_hierarchy_icons_new(void)
{
    GwHierarchyIcons *self = g_new0(GwHierarchyIcons, 1);

    GtkIconTheme *theme = gtk_icon_theme_get_default();
    gtk_icon_theme_add_resource_path (theme, "/io/github/gtkwave/GTKWave/icons");

    /* Verilog */
    self->module = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-module", 16, 0, NULL);
    self->task = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-task", 16, 0, NULL);
    self->function = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-function", 16, 0, NULL);
    self->begin = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-begin", 16, 0, NULL);
    self->fork = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-fork", 16, 0, NULL);

    /* SV */
    self->interface = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-interface", 16, 0, NULL);
    self->svpackage = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-svpackage", 16, 0, NULL);
    self->program = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-program", 16, 0, NULL);
    self->class = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-class", 16, 0, NULL);
    
    /* VHDL */
    self->design = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-design", 16, 0, NULL);
    self->block = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-block", 16, 0, NULL);
    self->generateif = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-generateif", 16, 0, NULL);
    self->generatefor = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-generatefor", 16, 0, NULL);
    self->instance = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-instance", 16, 0, NULL);
    self->package = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-package", 16, 0, NULL);

    self->signal = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-signal", 16, 0, NULL);
    self->portin = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-portin", 16, 0, NULL);
    self->portout = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-portout", 16, 0, NULL);
    self->portinout = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-portinout", 16, 0, NULL);
    self->buffer = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-buffer", 16, 0, NULL);
    self->linkage = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-linkage", 16, 0, NULL);

    /* FSDB VHDL (on top of GHW's existing) */
    self->record = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-record", 16, 0, NULL);
    self->generate = gtk_icon_theme_load_icon(theme, "gtkwave-hierarchy-generate", 16, 0, NULL);

    return self;
}

GdkPixbuf *gw_hierarchy_icons_get(GwHierarchyIcons *self, guint tree_kind)
{
    g_return_val_if_fail(self != NULL, NULL);

    // clang-format off
    switch (tree_kind) {
        case GW_TREE_KIND_VCD_ST_MODULE:    return self->module;
        case GW_TREE_KIND_VCD_ST_TASK:      return self->task;
        case GW_TREE_KIND_VCD_ST_FUNCTION:  return self->function;
        case GW_TREE_KIND_VCD_ST_BEGIN:     return self->begin;
        case GW_TREE_KIND_VCD_ST_FORK:      return self->fork;
        case GW_TREE_KIND_VCD_ST_GENERATE:  return self->generatefor; // same as GW_TREE_KIND_VHDL_ST_GENFOR
        case GW_TREE_KIND_VCD_ST_STRUCT:    return self->block;       // same as GW_TREE_KIND_VHDL_ST_BLOCK
        case GW_TREE_KIND_VCD_ST_UNION:     return self->instance;    // same as GW_TREE_KIND_VHDL_ST_INSTANCE
        case GW_TREE_KIND_VCD_ST_CLASS:     return self->class;
        case GW_TREE_KIND_VCD_ST_INTERFACE: return self->interface;
        case GW_TREE_KIND_VCD_ST_PACKAGE:   return self->svpackage;
        case GW_TREE_KIND_VCD_ST_PROGRAM:   return self->program;

        case GW_TREE_KIND_VHDL_ST_DESIGN:   return self->design;
        case GW_TREE_KIND_VHDL_ST_BLOCK:    return self->block;
        case GW_TREE_KIND_VHDL_ST_GENIF:    return self->generateif;
        case GW_TREE_KIND_VHDL_ST_GENFOR:   return self->generatefor;
        case GW_TREE_KIND_VHDL_ST_INSTANCE: return self->instance;
        case GW_TREE_KIND_VHDL_ST_PACKAGE:  return self->package;

        case GW_TREE_KIND_VHDL_ST_SIGNAL:    return self->signal;
        case GW_TREE_KIND_VHDL_ST_PORTIN:    return self->portin;
        case GW_TREE_KIND_VHDL_ST_PORTOUT:   return self->portout;
        case GW_TREE_KIND_VHDL_ST_PORTINOUT: return self->portinout;
        case GW_TREE_KIND_VHDL_ST_BUFFER:    return self->buffer;
        case GW_TREE_KIND_VHDL_ST_LINKAGE:   return self->linkage;

        case GW_TREE_KIND_VHDL_ST_ARCHITECTURE: return self->module; // same as GW_TREE_KIND_VCD_ST_MODULE
        case GW_TREE_KIND_VHDL_ST_FUNCTION:     return self->function; // same as GW_TREE_KIND_VCD_ST_FUNCTION
        case GW_TREE_KIND_VHDL_ST_PROCESS:      return self->task; // same as GW_TREE_KIND_VCD_ST_TASK
        case GW_TREE_KIND_VHDL_ST_PROCEDURE:    return self->class; // same as GW_TREE_KIND_VCD_ST_CLASS
        case GW_TREE_KIND_VHDL_ST_RECORD:       return self->record;
        case GW_TREE_KIND_VHDL_ST_GENERATE:     return self->generate;

        default:
            return NULL;
    }
    // clang-format on
}
