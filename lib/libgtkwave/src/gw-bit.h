#pragma once

#include <glib.h>

typedef enum
{
    GW_BIT_0,
    GW_BIT_X,
    GW_BIT_Z,
    GW_BIT_1,
    GW_BIT_H,
    GW_BIT_U,
    GW_BIT_W,
    GW_BIT_L,
    GW_BIT_DASH,
    GW_BIT_RSV9,
    GW_BIT_RSVA,
    GW_BIT_RSVB,
    GW_BIT_RSVC,
    GW_BIT_RSVD,
    GW_BIT_RSVE,
    GW_BIT_RSVF,
    GW_BIT_COUNT, // must be a power of two!
} GwBit;

#define GW_BIT_MASK (GW_BIT_COUNT - 1)

gchar gw_bit_to_char(GwBit self);
