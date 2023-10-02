#pragma once

#include "gw-types.h"
#include "gw-time.h"

struct _GwTrace
{
    GwTrace *t_next; /* doubly linked list of traces */
    GwTrace *t_prev;
    GwTrace *t_grp; /* pointer to group I'm in */
    GwTrace *t_match; /* If group begin pointer to group end and visa versa */

    char *name; /* current name */
    char *name_full; /* full name */
    char *asciivalue; /* value that marker points to */
    char *transaction_args; /* for TR_TTRANSLATED traces */
    GwTime asciitime; /* time this value corresponds with */
    GwTime shift; /* offset added to all entries in the trace */
    GwTime shift_drag; /* cached initial offset for CTRL+LMB drag on highlighted */

    double d_minval, d_maxval; /* cached value for when auto scaling is turned off */
    int d_num_ext; /* need to regen if differs from current in analog! */

    union
    {
        GwNode *nd; /* what makes up this trace */
        GwBitVector *vec;
    } n;

    guint64 flags; /* see def below in TraceEntFlagBits */
    guint64 cached_flags; /* used for tcl for saving flags during cut and paste */

    int f_filter; /* file filter */
    int p_filter; /* process filter */
    int t_filter; /* transaction process filter */
    int e_filter; /* enum filter (from FST) */

    unsigned int t_color; /* trace color index */
    unsigned char t_fpdecshift; /* for fixed point decimal */

    unsigned is_cursor : 1; /* set to mark a cursor trace */
    unsigned is_alias : 1; /* set when it's an alias (safe to free t->name then) */
    unsigned is_depacked : 1; /* set when it's been depacked from a compressed entry (safe to free
                                 t->name then) */
    unsigned vector : 1; /* 1 if bit vector, 0 if node */
    unsigned shift_drag_valid : 1; /* qualifies shift_drag above */
    unsigned interactive_vector_needs_regeneration : 1; /* for interactive VCDs */
    unsigned minmax_valid : 1; /* for d_minval, d_maxval */
    unsigned is_sort_group : 1; /* only used for sorting purposes */
    unsigned t_filter_converted : 1; /* used to mark that data conversion already occurred if
                                        t_filter != 0*/
};
