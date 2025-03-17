/* C code produced by gperf version 3.0.4 */
/* Command-line: /usr/bin/gperf -o -i 1 -C -k '1,$' -L C -H keyword_hash -N check_identifier -tT --initializer-suffix=,0 ./vcd_keywords.gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "./vcd_keywords.gperf"


/* AIX may need this for alloca to work */
#if defined _AIX
  #pragma alloca
#endif

#include <config.h>
#include <string.h>
#include "vcd.h"

struct vcd_keyword { const char *name; int token; };


#define TOTAL_KEYWORDS 33
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 14
#define MIN_HASH_VALUE 5
#define MAX_HASH_VALUE 69
/* maximum key range = 65, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
keyword_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 36, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 56, 51,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 26,
      26,  1, 36,  1, 70, 36,  6, 70, 31,  1,
       1, 70, 16,  1,  6,  1,  6,  1, 70, 70,
      21, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
      70, 70, 70, 70, 70, 70, 70
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]+1];
}

#ifdef __GNUC__
__inline
#if defined __GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct vcd_keyword *
check_identifier (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct vcd_keyword wordlist[] =
    {
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
#line 23 "./vcd_keywords.gperf"
      {"reg", V_REG},
#line 26 "./vcd_keywords.gperf"
      {"time", V_TIME},
      {"",0},
#line 30 "./vcd_keywords.gperf"
      {"trireg", V_TRIREG},
#line 37 "./vcd_keywords.gperf"
      {"in", V_IN},
#line 22 "./vcd_keywords.gperf"
      {"realtime", V_REALTIME},
#line 36 "./vcd_keywords.gperf"
      {"port", V_PORT},
#line 29 "./vcd_keywords.gperf"
      {"trior", V_TRIOR},
#line 40 "./vcd_keywords.gperf"
      {"string", V_STRINGTYPE},
#line 45 "./vcd_keywords.gperf"
      {"longint", V_LONGINT},
#line 43 "./vcd_keywords.gperf"
      {"int", V_INT},
#line 18 "./vcd_keywords.gperf"
      {"parameter", V_PARAMETER},
#line 39 "./vcd_keywords.gperf"
      {"inout", V_INOUT},
      {"",0},
#line 19 "./vcd_keywords.gperf"
      {"integer", V_INTEGER},
#line 44 "./vcd_keywords.gperf"
      {"shortint", V_SHORTINT},
#line 21 "./vcd_keywords.gperf"
      {"real_parameter", V_REAL_PARAMETER},
      {"",0}, {"",0}, {"",0},
#line 38 "./vcd_keywords.gperf"
      {"out", V_OUT},
#line 34 "./vcd_keywords.gperf"
      {"wire", V_WIRE},
      {"",0}, {"",0}, {"",0},
#line 35 "./vcd_keywords.gperf"
      {"wor", V_WOR},
#line 46 "./vcd_keywords.gperf"
      {"byte", V_BYTE},
#line 42 "./vcd_keywords.gperf"
      {"logic", V_LOGIC},
#line 28 "./vcd_keywords.gperf"
      {"triand", V_TRIAND},
      {"",0},
#line 41 "./vcd_keywords.gperf"
      {"bit", V_BIT},
#line 20 "./vcd_keywords.gperf"
      {"real", V_REAL},
      {"",0}, {"",0}, {"",0},
#line 27 "./vcd_keywords.gperf"
      {"tri", V_TRI},
#line 47 "./vcd_keywords.gperf"
      {"enum", V_ENUM},
      {"",0}, {"",0}, {"",0}, {"",0},
#line 48 "./vcd_keywords.gperf"
      {"shortreal", V_SHORTREAL},
#line 17 "./vcd_keywords.gperf"
      {"event", V_EVENT},
      {"",0}, {"",0}, {"",0},
#line 33 "./vcd_keywords.gperf"
      {"wand", V_WAND},
      {"",0}, {"",0}, {"",0}, {"",0},
#line 32 "./vcd_keywords.gperf"
      {"tri1", V_TRI1},
      {"",0}, {"",0}, {"",0}, {"",0},
#line 31 "./vcd_keywords.gperf"
      {"tri0", V_TRI0},
      {"",0}, {"",0},
#line 25 "./vcd_keywords.gperf"
      {"supply1", V_SUPPLY1},
      {"",0},
#line 49 "./vcd_keywords.gperf"
      {"$end", V_END},
      {"",0}, {"",0},
#line 24 "./vcd_keywords.gperf"
      {"supply0", V_SUPPLY0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = keyword_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 50 "./vcd_keywords.gperf"


int vcd_keyword_code(const char *s, unsigned int len)
{
const struct vcd_keyword *rc = check_identifier(s, len);
return(rc ? rc->token : V_STRING);
}

