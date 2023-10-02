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
