#pragma once

#include "gw-time.h"

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct _GwVectorEnt GwVectorEnt;

struct _GwVectorEnt
{
    GwTime time;
    GwVectorEnt *next;
    unsigned char flags; /* so far only set on strings */
    unsigned char v[]; /* C99 */
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif
