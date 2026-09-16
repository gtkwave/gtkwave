#ifndef BROWSE_WAVELINK_H
#define BROWSE_WAVELINK_H

/* kill the GLOBALS_H header inclusion as it's not needed here */
#define GLOBALS_H

/* Workaround: disable the gtk2->3 compats from gtkwave */
/* Remove this after the gtk2->3 compats got removed from gtkwave */
#define WAVE_GTK23COMPAT_H

#include "../../src/tree.h"
#include "../../src/debug.h"
#include <vzt_read.h>
#include <lxt2_read.h>
#include <fstapi.h>
#include <gtkwave.h>

extern struct vzt_rd_trace *vzt;
extern struct lxt2_rd_trace *lx2;
extern void *fst;
extern int64_t timezero;


#endif
