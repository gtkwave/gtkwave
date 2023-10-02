#pragma once

#include "gw-types.h"

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct _GwBitVector
{
    GwBitVector *transaction_cache; /* for TR_TTRANSLATED traces */
    GwBitVector *transaction_chain; /* for TR_TTRANSLATED traces */
    GwNode *transaction_nd; /* for TR_TTRANSLATED traces */

    char *bvname; /* name of this vector of bits           */
    int nbits; /* number of bits in this vector         */
    int numregions; /* number of regions that follow         */
    GwBits *bits; /* pointer to Bits structs for save file */
    GwVectorEnt *vectors[]; /* C99 pointers to the vectors           */
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif

