#include "gw-evcd.h"

/* The EVCD state-character alphabet and its collapse to four-state VCD.
 * Kept in the same order as the historic gw-vcd-loader.c table so behaviour of
 * the four-state waveform is unchanged. */
static const char EVCD_CHARS[] = "DUNZduLHXTlh01?FAaBbCcf";
static const char EVCD_TO_VCD[] = "01xz0101xz0101xzxxxxxxz";

gchar gw_evcd_char_to_vcd(gchar evcd_char)
{
    for (const char *p = EVCD_CHARS; *p != '\0'; p++) {
        if (*p == evcd_char) {
            return EVCD_TO_VCD[p - EVCD_CHARS];
        }
    }

    return 'x';
}

GwVarDir gw_evcd_char_to_direction(gchar evcd_char)
{
    switch (evcd_char) {
        /* Input (test fixture) class. */
        case 'D':
        case 'U':
        case 'N':
        case 'Z':
        case 'd':
        case 'u':
            return GW_VAR_DIR_IN;

        /* Output (DUT) class. */
        case 'L':
        case 'H':
        case 'X':
        case 'T':
        case 'l':
        case 'h':
            return GW_VAR_DIR_OUT;

        /* Unknown-direction class: both ends active or unresolved. */
        case '0':
        case '1':
        case '?':
        case 'F':
        case 'A':
        case 'a':
        case 'B':
        case 'b':
        case 'C':
        case 'c':
        case 'f':
            return GW_VAR_DIR_INOUT;

        default:
            return GW_VAR_DIR_IMPLICIT;
    }
}

GwVarDir gw_evcd_direction_merge(GwVarDir a, GwVarDir b)
{
    if (b == GW_VAR_DIR_IMPLICIT) {
        return a;
    }
    if (a == GW_VAR_DIR_IMPLICIT) {
        return b;
    }
    if (a == b) {
        return a;
    }

    return GW_VAR_DIR_INOUT;
}

const gchar *gw_evcd_char_describe(gchar evcd_char)
{
    switch (evcd_char) {
        /* Input (test fixture). */
        case 'D':
            return "low (input)";
        case 'U':
            return "high (input)";
        case 'N':
            return "unknown (input)";
        case 'Z':
            return "three-state (input)";
        case 'd':
            return "low, two or more drivers (input)";
        case 'u':
            return "high, two or more drivers (input)";

        /* Output (DUT). */
        case 'L':
            return "low (output)";
        case 'H':
            return "high (output)";
        case 'X':
            return "unknown / do-not-care (output)";
        case 'T':
            return "three-state (output)";
        case 'l':
            return "low, two or more drivers (output)";
        case 'h':
            return "high, two or more drivers (output)";

        /* Unknown direction. */
        case '0':
            return "low (input and output active)";
        case '1':
            return "high (input and output active)";
        case '?':
            return "unknown";
        case 'F':
            return "three-state (input and output unconnected)";
        case 'A':
            return "unknown (input 0, output 1)";
        case 'a':
            return "unknown (input 0, output X)";
        case 'B':
            return "unknown (input 1, output 0)";
        case 'b':
            return "unknown (input 1, output X)";
        case 'C':
            return "unknown (input X, output 0)";
        case 'c':
            return "unknown (input X, output 1)";
        case 'f':
            return "unknown (input and output three-stated)";

        default:
            return NULL;
    }
}

const gchar *gw_evcd_strength_name(gint code)
{
    static const gchar *const STRENGTHS[] = {
        "highz", /* 0 */
        "small", /* 1 */
        "medium", /* 2 */
        "weak", /* 3 */
        "large", /* 4 */
        "pull", /* 5 */
        "strong", /* 6 */
        "supply", /* 7 */
    };

    if (code < 0 || code > 7) {
        return NULL;
    }

    return STRENGTHS[code];
}
