#include "gw-hash.h"
#include "gw-util.h"

/*
 * Generic hash function for symbol names...
 */
int gw_hash(const gchar *str)
{
    const char *p;
    char ch;
    unsigned int h = 0, h2 = 0, pos = 0, g;
    for (p = str; *p; p++) {
        ch = *p;
        h2 <<= 3;
        h2 -= ((unsigned int)ch + (pos++)); /* this handles stranded vectors quite well.. */

        h = (h << 4) + ch;
        if ((g = h & 0xf0000000)) {
            h = h ^ (g >> 24);
            h = h ^ g;
        }
    }

    h ^= h2; /* combine the two hashes */

    return h % GW_HASH_PRIME;
}

