#include "gw-util.h"

GParamSpec *gw_param_spec_time(const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               GParamFlags flags)
{
    return g_param_spec_int64(name, nick, blurb, G_MININT64, G_MAXINT64, 0, flags);
}

/*
 * compares two facilities a la strcmp but preserves
 * numbers for comparisons
 *
 * there are two flavors..the slow and accurate to any
 * arbitrary number of digits version (first) and the
 * fast one good to 2**31-1.  we default to the faster
 * version since there's probably no real need to
 * process ints larger than two billion anyway...
 */

#ifdef WAVE_USE_SIGCMP_INFINITE_PRECISION
#if __STDC_VERSION__ < 199901L
inline
#endif
    int
    sigcmp_2(const char *s1, const char *s2)
{
    char *n1, *n2;
    unsigned char c1, c2;
    int len1, len2;

    for (;;) {
        c1 = (unsigned char)*s1;
        c2 = (unsigned char)*s2;

        if ((c1 == 0) && (c2 == 0))
            return (0);
        if ((c1 >= '0') && (c1 <= '9') && (c2 >= '0') && (c2 <= '9')) {
            n1 = s1;
            n2 = s2;
            len1 = len2 = 0;

            do {
                len1++;
                c1 = (unsigned char)*(n1++);
            } while ((c1 >= '0') && (c1 <= '9'));
            if (!c1)
                n1--;

            do {
                len2++;
                c2 = (unsigned char)*(n2++);
            } while ((c2 >= '0') && (c2 <= '9'));
            if (!c2)
                n2--;

            do {
                if (len1 == len2) {
                    c1 = (unsigned char)*(s1++);
                    len1--;
                    c2 = (unsigned char)*(s2++);
                    len2--;
                } else if (len1 < len2) {
                    c1 = '0';
                    c2 = (unsigned char)*(s2++);
                    len2--;
                } else {
                    c1 = (unsigned char)*(s1++);
                    len1--;
                    c2 = '0';
                }

                if (c1 != c2)
                    return ((int)c1 - (int)c2);
            } while (len1);

            s1 = n1;
            s2 = n2;
            continue;
        } else {
            if (c1 != c2)
                return ((int)c1 - (int)c2);
        }

        s1++;
        s2++;
    }
}
#else
#if __STDC_VERSION__ < 199901L
inline
#endif
    int
    sigcmp_2(const char *s1, const char *s2)
{
    unsigned char c1, c2;
    int u1, u2;

    for (;;) {
        c1 = (unsigned char)*(s1++);
        c2 = (unsigned char)*(s2++);

        if (!(c1 | c2))
            return (0); /* removes extra branch through logical or */
        if ((c1 <= '9') && (c2 <= '9') && (c2 >= '0') && (c1 >= '0')) {
            u1 = (int)(c1 & 15);
            u2 = (int)(c2 & 15);

            while (((c2 = (unsigned char)*s2) >= '0') && (c2 <= '9')) {
                u2 *= 10;
                u2 += (unsigned int)(c2 & 15);
                s2++;
            }

            while (((c2 = (unsigned char)*s1) >= '0') && (c2 <= '9')) {
                u1 *= 10;
                u1 += (unsigned int)(c2 & 15);
                s1++;
            }

            if (u1 == u2)
                continue;
            else
                return ((int)u1 - (int)u2);
        } else {
            if (c1 != c2)
                return ((int)c1 - (int)c2);
        }
    }
}
#endif

gint gw_signal_name_compare(const gchar *name1, const gchar *name2)
{
    int rc = sigcmp_2(name1, name2);
    if (rc == 0) {
        /* to handle leading zero "0" vs "00" cases ... we provide a definite order so bsearch
         * doesn't fail */
        rc = strcmp(name1, name2);
    }

    return (rc);
}