#include "gw-bit.h"

gchar gw_bit_to_char(GwBit self)
{
    g_return_val_if_fail(self < GW_BIT_COUNT, '?');

    static const gchar *BIT_TO_CHAR = "0xz1huwl-???????";

    return BIT_TO_CHAR[self];
}
