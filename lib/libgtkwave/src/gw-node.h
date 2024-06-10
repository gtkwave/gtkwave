#pragma once

#include "gw-types.h"
#include "gw-hist-ent.h"
#include "gw-vlist-writer.h"

/* struct Node bitfield widths */
#define WAVE_VARXT_WIDTH (16)
#define WAVE_VARXT_MAX_ID ((1 << WAVE_VARXT_WIDTH) - 1)
#define WAVE_VARDT_WIDTH (6)
#define WAVE_VARDIR_WIDTH (3)
#define WAVE_VARTYPE_WIDTH (6)

struct _GwExpandReferences
{
    GwNode *parent; /* which atomic vec we expanded from */
    int parentbit; /* which bit from that atomic vec */
    int actual; /* bit number to be used in [] */
    int refcnt;
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct _GwNode
{
    GwExpandReferences *expansion; /* indicates which nptr this node was expanded from (if it was
                        expanded at all) and (when implemented) refcnts */
    char *nname; /* ascii name of node */
    GwHistEnt head; /* first entry in transition history */
    GwHistEnt *curr; /* ptr. to current history entry */

    GwHistEnt **harray; /* fill this in when we make a trace.. contains  */
    /*  a ptr to an array of histents for bsearching */
    union
    {
        GwFac *mvlfac; /* for use with mvlsim aets */
        GwVlist *mvlfac_vlist;
        GwVlistWriter *mvlfac_vlist_writer;
        char *value; /* for use when unrolling ae2 values */
    } mv; /* anon union is a gcc extension so use mv instead.  using this union avoids crazy casting
             warnings */

    int msi, lsi; /* for 64-bit, more efficient than having as an external struct ExtNode*/

    int numhist; /* number of elements in the harray */

    unsigned varxt : WAVE_VARXT_WIDTH; /* reference inside subvar_pnt[] */
    unsigned vardt : WAVE_VARDT_WIDTH; /* see nodeVarDataType, this is an internal value */
    unsigned vardir : WAVE_VARDIR_WIDTH; /* see nodeVarDir, this is an internal value (currently
                                            used only by extload and FST) */
    unsigned vartype : WAVE_VARTYPE_WIDTH; /* see nodeVarType, this is an internal value */

    unsigned extvals : 1; /* was formerly a pointer to ExtNode "ext", now simply a flag */
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif