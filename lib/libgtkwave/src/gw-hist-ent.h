#pragma once

#include "gw-types.h"
#include "gw-time.h"

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct _GwHistEnt
{
    GwHistEnt *next; /* next transition in history */

    union
    {
        unsigned char h_val; /* value: AN_STR[val] or AnalyzerBits which correspond */
        char *h_vector; /* pointer to a whole vector of h_val type bits */
        double h_double;
    } v;

    GwTime time; /* time of transition */
    guint8 flags; /* so far only set on glitch/real condition */
};

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif

typedef enum
{
    GW_HIST_ENT_FLAG_GLITCH = 1 << 0,
    GW_HIST_ENT_FLAG_REAL = 1 << 1,
    GW_HIST_ENT_FLAG_STRING = 1 << 2,
} GwHistEntFlag;
