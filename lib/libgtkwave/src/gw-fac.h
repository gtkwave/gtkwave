#pragma once

#include "gw-types.h"

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct _GwFac
{
    GwNode *working_node;
    int node_alias;
    int len;
    unsigned int flags;
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif

typedef enum {
    GW_FAC_FLAG_BITS = 0,
    GW_FAC_FLAG_INTEGER = 1 << 0,
    GW_FAC_FLAG_DOUBLE = 1 << 1,
    GW_FAC_FLAG_STRING = 1 << 2,
    GW_FAC_FLAG_ALIAS = 1 << 3,
    GW_FAC_FLAG_SYNVEC = 1 << 17,
} GwFacFlags;