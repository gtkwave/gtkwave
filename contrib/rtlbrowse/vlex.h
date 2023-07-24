#ifndef VLEX_DEFINES_H
#define VLEX_DEFINES_H

#include <stdlib.h>

#define V_IGNORE (1)
#define V_ID (2)
#define V_PORT (3)
#define V_MODULE (4)
#define V_ENDMODULE (5)
#define V_STRING (6)
#define V_WS (7)
#define V_MACRO (8)
#define V_CMT (9)
#define V_NUMBER (10)
#define V_FUNC (11)
#define V_PREPROC (12)
#define V_PREPROC_WS (13)
#define V_KW (14)
#define V_KW_2005 (15)

extern int my_yylineno;
extern char *v_preproc_name;
int yylex(void);
extern char *yytext;

typedef size_t gtkwave_yy_size_t;
extern gtkwave_yy_size_t yyleng;

const char *is_builtin_define (register const char *str, register unsigned int len);

#endif

