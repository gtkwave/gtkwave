#ifndef BROWSE_WAVELINK_H
#define BROWSE_WAVELINK_H

/* kill the GLOBALS_H header inclusion as it's not needed here */
#define GLOBALS_H

#include "../../src/ae2.h"
#include "../../src/debug.h"
#include "../../src/helpers/vzt_read.h"
#include "../../src/helpers/lxt2_read.h"
#include <fstapi.h>

extern struct vzt_rd_trace  *vzt;
extern struct lxt2_rd_trace *lx2;
extern void *fst;
extern int64_t timezero;

#ifdef AET2_IS_PRESENT
extern AE2_HANDLE ae2;
#endif

enum treeview2_columns {
XXX2_NAME_COLUMN,
XXX2_TREE_COLUMN,
XXX2_NUM_COLUMNS
};

#define XXX_GTK_OBJECT(x) x
#define XXX_GTK_OBJECT_GET_CLASS GTK_OBJECT_GET_CLASS

/* gtk3->4 deprecated */

#if GTK_CHECK_VERSION(3,0,0)

#define YYY_GTK_TEXT_VIEW GTK_SCROLLABLE
#define YYY_gtk_text_view_get_vadjustment gtk_scrollable_get_vadjustment

#else

#define YYY_GTK_TEXT_VIEW GTK_TEXT_VIEW
#define YYY_gtk_text_view_get_vadjustment gtk_text_view_get_vadjustment

#endif

#endif

