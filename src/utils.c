/*
 * Utilities and non-trivial high level functions
 * 
 * Utilities that are provided for convenience but can be reused in
 * multiple parts of GTKWave.
 */

#include "utils.h"
#include "globals.h"
#include "gw-types.h"
#include <string.h>


GwTime get_current_time(void)
{
    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    if (!gw_marker_is_enabled(primary_marker))
        return 0 ;
    else 
        return gw_marker_get_position(primary_marker);
}

char* get_symbol_value_at_time(const GwSymbol *sym, GwTime time)
{
    g_return_val_if_fail(sym != NULL, NULL);

    GwNode *nd = sym->n;

    /* Import on-demand (may mutate dump-file state but not GUI) */
    if (nd->mv.mvlfac)
    {
        import_trace(nd);
        // Bug workaround
        import_trace(nd);
    }

    /* Ensure harray exists (copy of AddNodeTraceReturn behavior) */
    if (!nd->harray) {
        GwHistEnt *histpnt = &(nd->head);
        int histcount = 0;
        while (histpnt) {
            histcount++;
            histpnt = histpnt->next;
        }
        nd->numhist = histcount;
        if (histcount == 0)
            return NULL;
        nd->harray = malloc_2(histcount * sizeof(GwHistEnt *));
        if (!nd->harray)
            return NULL;
        histpnt = &(nd->head);
        for (int i = 0; i < histcount; i++) {
            nd->harray[i] = histpnt;
            histpnt = histpnt->next;
        }
    }

    /* Build a minimal temporary GwTrace (on stack) used by converters */
    GwTrace t = {0};

    t.shift = 0; /* adjust if you need per-trace shift */
    if (!GLOBALS->hier_max_level)
        t.name = nd->nname;
    else
        t.name = hier_extract(nd->nname, GLOBALS->hier_max_level);

    /* choose flags similar to AddNodeTraceReturn */
    if (nd->extvals) {
        // Force hex format for later simplicity.
        // Would be relevant to return trace, but free will be complicated.
        t.flags = TR_HEX | TR_RJUSTIFY;
        
        // int n = nd->msi - nd->lsi;
        // if (n < 0)
        //     n = -n;
        // n++;
        
        // switch (nd->vartype) {
        // case GW_VAR_TYPE_VCD_INTEGER:
        // case GW_VAR_TYPE_VCD_PARAMETER:
        // case GW_VAR_TYPE_SV_INT:
        // case GW_VAR_TYPE_SV_SHORTINT:
        // case GW_VAR_TYPE_SV_LONGINT:
        //     t.flags = TR_SIGNED | TR_RJUSTIFY;
        //     break;
        // default:
        //     t.flags = ((n > 3) || (n < -3)) ? TR_HEX | TR_RJUSTIFY : TR_BIN | TR_RJUSTIFY;
        //     break;
        // }
    } else {
        // t.flags = TR_BIN;
        t.flags = TR_HEX;
    }
    t.vector = FALSE;
    t.n.nd = nd;

    return get_trace_value_at_time(&t, time);
    
}

char *get_trace_value_at_time(GwTrace *trace, GwTime time)
{
    GwNode* nd = trace->n.nd;
    /* Find history entry at desired time (bsearch_node expects key = time - t.shift) */
    GwHistEnt *hptr = bsearch_node(nd, time - trace->shift);
    if (!hptr)
        return NULL;

    /* Scalar simple variable */
    if (!nd->extvals) {
        unsigned char h_val = hptr->v.h_val;
        // TODO: support inverted traces
        // if (t.flags & TR_INVERT)
        //     h_val = gw_bit_invert(h_val);

        char *rc = calloc_2(1, 2 * sizeof(char));
        rc[0] = gw_bit_to_char(h_val);
        return rc;
    }

    /* Extended / vector values: may be real/string/vector */
    if (hptr->flags & GW_HIST_ENT_FLAG_REAL) {
        if (!(hptr->flags & GW_HIST_ENT_FLAG_STRING)) {
            return convert_ascii_real(trace, &hptr->v.h_double);
        } else {
            return convert_ascii_string((char *)hptr->v.h_vector);
        }
    } else {
        /* hptr->v.h_vector is a char* representation */
        return convert_ascii_vec(trace, hptr->v.h_vector);
    }
}
