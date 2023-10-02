#pragma once

#include "gw-types.h"
#include "gw-time.h"

struct _GwBitAttributes
{
    GwTime shift;
    //TraceFlagsType flags;
    guint64 flags;
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct _GwBits
{
    char *name; /* name of this vector of bits   */
    int nnbits; /* number of bits in this vector */
    GwBitAttributes
        *attribs; /* for keeping track of combined timeshifts and inversions (and for savefile) */

    GwNode *nodes[]; /* C99 pointers to the bits (nodes)  */
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif
