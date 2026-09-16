#include "gw-bit.h"

gchar gw_bit_to_char(GwBit self)
{
    g_return_val_if_fail(self < GW_BIT_COUNT, '?');

    static const gchar *BIT_TO_CHAR = "0xz1huwl-???????";

    return BIT_TO_CHAR[self];
}

GwBit gw_bit_invert(GwBit self)
{
    static const GwBit INVERSE[] = {
        GW_BIT_1,
        GW_BIT_X,
        GW_BIT_Z,
        GW_BIT_0,
        GW_BIT_L,
        GW_BIT_U,
        GW_BIT_W,
        GW_BIT_H,
        GW_BIT_DASH,
        GW_BIT_DASH,
        GW_BIT_DASH,
        GW_BIT_DASH,
        GW_BIT_DASH,
        GW_BIT_DASH,
        GW_BIT_DASH,
        GW_BIT_DASH,
    };

    return INVERSE[self];
}

GwBit gw_bit_from_char(gchar c)
{
    switch (c) {
        case '0':
            return GW_BIT_0;

        case '1':
            return GW_BIT_1;

        case 'X':
        case 'x':
            return GW_BIT_X;

        case 'Z':
        case 'z':
            return GW_BIT_Z;

        case 'H':
        case 'h':
            return GW_BIT_H;

        case 'U':
        case 'u':
            return GW_BIT_U;

        case 'W':
        case 'w':
            return GW_BIT_W;

        case 'L':
        case 'l':
            return GW_BIT_L;

        case '-':
            return GW_BIT_DASH;

        default:
            g_return_val_if_reached(GW_BIT_X);
    }
}