#ifndef BROWSE_WAVELINK_H
#define BROWSE_WAVELINK_H

/* kill the GLOBALS_H header inclusion as it's not needed here */
#define GLOBALS_H

#include "../../src/tree.h"
#include "../../src/debug.h"
#include <vzt_read.h>
#include <lxt2_read.h>
#include <fstapi.h>
#include <gtkwave.h>

extern struct vzt_rd_trace  *vzt;
extern struct lxt2_rd_trace *lx2;
extern void *fst;
extern int64_t timezero;

enum treeview2_columns {
XXX2_NAME_COLUMN,
XXX2_TREE_COLUMN,
XXX2_NUM_COLUMNS
};

/* gtk3->4 deprecated */

#if GTK_CHECK_VERSION(3,0,0)

#define YYY_GTK_TEXT_VIEW GTK_SCROLLABLE
#define YYY_gtk_text_view_get_vadjustment gtk_scrollable_get_vadjustment

#else

#define YYY_GTK_TEXT_VIEW GTK_TEXT_VIEW
#define YYY_gtk_text_view_get_vadjustment gtk_text_view_get_vadjustment

#endif

#endif

