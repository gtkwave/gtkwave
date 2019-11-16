/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


/*
 * vcd.c 			23jan99ajb
 * evcd parts 			29jun99ajb
 * profiler optimizations 	15jul99ajb
 * more profiler optimizations	25jan00ajb
 * finsim parameter fix		26jan00ajb
 * vector rechaining code	03apr00ajb
 * multiple var section code	06apr00ajb
 * fix for duplicate nets	19dec00ajb
 * support for alt hier seps	23dec00ajb
 * fix for rcs identifiers	16jan01ajb
 * coredump fix for bad VCD	04apr02ajb
 * min/maxid speedup            27feb03ajb
 * bugfix on min/maxid speedup  06jul03ajb
 * escaped hier modification    20feb06ajb
 * added partial loader support 04aug06ajb
 * added real_parameter vartype 04aug06ajb
 * added in/out port vartype	31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 */
#include <config.h>
#include "globals.h"
#include "vcd.h"
#include "hierpack.h"

#if !defined _MSC_VER && !defined __MINGW32__
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef __MINGW32__
#include <windows.h>
#endif

#undef VCD_BSEARCH_IS_PERFECT		/* bsearch is imperfect under linux, but OK under AIX */

static hptr add_histent_p(TimeType time, struct Node *n, char ch, int regadd, char *vector);
static void add_tail_histents(void);
static void vcd_build_symbols(void);
static void vcd_cleanup(void);
static void evcd_strcpy(char *dst, char *src);





/*******************************************************************************/

#define WAVE_PARTIAL_VCD_RING_BUFFER_SIZE (1024*1024)


unsigned int get_8(char *p)
{
if(p >= (GLOBALS->buf_vcd_partial_c_2 + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
        {
        p-= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
        }

return((unsigned int)((unsigned char)*p));
}

unsigned int get_32(char *p)
{
unsigned int rc;
rc =    (get_8(p++) << 24) ;
rc|=    (get_8(p++) << 16) ;
rc|=    (get_8(p++) <<  8) ;
rc|=    (get_8(p)   <<  0) ;

return(rc);
}


int consume(void)       /* for testing only */
{
int len;

GLOBALS->consume_countdown_vcd_partial_c_1--;
if(!GLOBALS->consume_countdown_vcd_partial_c_1)
	{
	GLOBALS->consume_countdown_vcd_partial_c_1 = 100000;
	return(0);
	}

if((len = *GLOBALS->consume_ptr_vcd_partial_c_1))
        {
        int i;

        len = get_32(GLOBALS->consume_ptr_vcd_partial_c_1+1);
        for(i=0;i<len;i++)
                {
                GLOBALS->vcdbuf_vcd_partial_c_2[i] = get_8(GLOBALS->consume_ptr_vcd_partial_c_1+i+5);
                }
        GLOBALS->vcdbuf_vcd_partial_c_2[i] = 0;

        *GLOBALS->consume_ptr_vcd_partial_c_1 = 0;
        GLOBALS->consume_ptr_vcd_partial_c_1 = GLOBALS->consume_ptr_vcd_partial_c_1+i+5;
        if(GLOBALS->consume_ptr_vcd_partial_c_1 >= (GLOBALS->buf_vcd_partial_c_2 + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
                {
                GLOBALS->consume_ptr_vcd_partial_c_1 -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
                }
        }

return(len);
}


/******************************************************************/

enum Tokens   { T_VAR, T_END, T_SCOPE, T_UPSCOPE,
		T_COMMENT, T_DATE, T_DUMPALL, T_DUMPOFF, T_DUMPON,
		T_DUMPVARS, T_ENDDEFINITIONS,
		T_DUMPPORTS, T_DUMPPORTSOFF, T_DUMPPORTSON, T_DUMPPORTSALL,
		T_TIMESCALE, T_VERSION, T_VCDCLOSE, T_TIMEZERO,
		T_EOF, T_STRING, T_UNKNOWN_KEY };

static char *tokens[]={ "var", "end", "scope", "upscope",
                 "comment", "date", "dumpall", "dumpoff", "dumpon",
                 "dumpvars", "enddefinitions",
                 "dumpports", "dumpportsoff", "dumpportson", "dumpportsall",
                 "timescale", "version", "vcdclose", "timezero",
                 "", "", "" };


#define NUM_TOKENS 19


#define T_GET tok=get_token();if((tok==T_END)||(tok==T_EOF))break;

/******************************************************************/

#define NUM_VTOKENS 23

/******************************************************************/


static unsigned int vcdid_hash(char *s, int len)
{
unsigned int val=0;
int i;

s+=(len-1);

for(i=0;i<len;i++)
        {
        val *= 94;
        val += (((unsigned char)*s) - 32);
        s--;
        }

return(val);
}

/******************************************************************/

/*
 * bsearch compare
 */
static int vcdsymbsearchcompare(const void *s1, const void *s2)
{
char *v1;
struct vcdsymbol *v2;

v1=(char *)s1;
v2=*((struct vcdsymbol **)s2);

return(strcmp(v1, v2->id));
}


/*
 * actual bsearch
 */
static struct vcdsymbol *bsearch_vcd(char *key, int len)
{
struct vcdsymbol **v;
struct vcdsymbol *t;

if(GLOBALS->indexed_vcd_partial_c_2)
        {
        unsigned int hsh = vcdid_hash(key, len);
        if((hsh>=GLOBALS->vcd_minid_vcd_partial_c_2)&&(hsh<=GLOBALS->vcd_maxid_vcd_partial_c_2))
                {
                return(GLOBALS->indexed_vcd_partial_c_2[hsh-GLOBALS->vcd_minid_vcd_partial_c_2]);
                }

	return(NULL);
        }

if(GLOBALS->sorted_vcd_partial_c_2)
	{
	v=(struct vcdsymbol **)bsearch(key, GLOBALS->sorted_vcd_partial_c_2, GLOBALS->numsyms_vcd_partial_c_2,
		sizeof(struct vcdsymbol *), vcdsymbsearchcompare);

	if(v)
		{
		#ifndef VCD_BSEARCH_IS_PERFECT
			for(;;)
				{
				t=*v;

				if((v==GLOBALS->sorted_vcd_partial_c_2)||(strcmp((*(--v))->id, key)))
					{
					return(t);
					}
				}
		#else
			return(*v);
		#endif
		}
		else
		{
		return(NULL);
		}
	}
	else
	{
	if(!GLOBALS->err_vcd_partial_c_2)
		{
		fprintf(stderr, "Near byte %d, VCD search table NULL..is this a VCD file?\n", (int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)));
		GLOBALS->err_vcd_partial_c_2=1;
		}
	return(NULL);
	}
}


/*
 * sort on vcdsymbol pointers
 */
static int vcdsymcompare(const void *s1, const void *s2)
{
struct vcdsymbol *v1, *v2;

v1=*((struct vcdsymbol **)s1);
v2=*((struct vcdsymbol **)s2);

return(strcmp(v1->id, v2->id));
}


/*
 * create sorted (by id) table
 */
static void create_sorted_table(void)
{
struct vcdsymbol *v;
struct vcdsymbol **pnt;
unsigned int vcd_distance;

if(GLOBALS->sorted_vcd_partial_c_2)
	{
	free_2(GLOBALS->sorted_vcd_partial_c_2);	/* this means we saw a 2nd enddefinition chunk! */
	GLOBALS->sorted_vcd_partial_c_2=NULL;
	}

if(GLOBALS->indexed_vcd_partial_c_2)
	{
	free_2(GLOBALS->indexed_vcd_partial_c_2);
	GLOBALS->indexed_vcd_partial_c_2=NULL;
	}

if(GLOBALS->numsyms_vcd_partial_c_2)
	{
        vcd_distance = GLOBALS->vcd_maxid_vcd_partial_c_2 - GLOBALS->vcd_minid_vcd_partial_c_2 + 1;

        if((vcd_distance <= VCD_INDEXSIZ)||(!GLOBALS->vcd_hash_kill))
                {
                GLOBALS->indexed_vcd_partial_c_2 = (struct vcdsymbol **)calloc_2(vcd_distance, sizeof(struct vcdsymbol *));

		/* printf("%d symbols span ID range of %d, using indexing...\n", numsyms, vcd_distance); */

                v=GLOBALS->vcdsymroot_vcd_partial_c_2;
                while(v)
                        {
                        if(!GLOBALS->indexed_vcd_partial_c_2[v->nid - GLOBALS->vcd_minid_vcd_partial_c_2]) GLOBALS->indexed_vcd_partial_c_2[v->nid - GLOBALS->vcd_minid_vcd_partial_c_2] = v;
                        v=v->next;
                        }
                }
                else
		{
		pnt=GLOBALS->sorted_vcd_partial_c_2=(struct vcdsymbol **)calloc_2(GLOBALS->numsyms_vcd_partial_c_2, sizeof(struct vcdsymbol *));
		v=GLOBALS->vcdsymroot_vcd_partial_c_2;
		while(v)
			{
			*(pnt++)=v;
			v=v->next;
			}

		qsort(GLOBALS->sorted_vcd_partial_c_2, GLOBALS->numsyms_vcd_partial_c_2, sizeof(struct vcdsymbol *), vcdsymcompare);
		}
	}
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(void)
{
GLOBALS->vend_vcd_partial_c_2=GLOBALS->vst_vcd_partial_c_2=GLOBALS->vcdbuf_vcd_partial_c_2=(char *)calloc_2(1,VCD_BSIZ);
}


static int getch_fetch(void)
{
size_t rd;

errno = 0;

GLOBALS->vcdbyteno_vcd_partial_c_2+=(GLOBALS->vend_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2);
rd=consume();
GLOBALS->vend_vcd_partial_c_2=(GLOBALS->vst_vcd_partial_c_2=GLOBALS->vcdbuf_vcd_partial_c_2)+rd;

if(!rd) return(-1);

return((int)(*GLOBALS->vst_vcd_partial_c_2));
}

static inline signed char getch(void) {
  signed char ch = ((GLOBALS->vst_vcd_partial_c_2!=GLOBALS->vend_vcd_partial_c_2)?((int)(*GLOBALS->vst_vcd_partial_c_2)):(getch_fetch()));
  if(ch>=0) { GLOBALS->vst_vcd_partial_c_2++; };
  return(ch);
}

static inline signed char getch_peek(void) {
  signed char ch = ((GLOBALS->vst_vcd_partial_c_2!=GLOBALS->vend_vcd_partial_c_2)?((int)(*GLOBALS->vst_vcd_partial_c_2)):(getch_fetch()));
  /* no increment */
  return(ch);
}


static int getch_patched(void)
{
char ch;

ch=*GLOBALS->vsplitcurr_vcd_partial_c_2;
if(!ch)
	{
	return(-1);
	}
	else
	{
	GLOBALS->vsplitcurr_vcd_partial_c_2++;
	return((int)ch);
	}
}

/*
 * simple tokenizer
 */
static int get_token(void)
{
int ch;
int i, len=0;
int is_string=0;
char *yyshadow;

for(;;)
	{
	ch=getch();
	if(ch<0) return(T_EOF);
	if(ch<=' ') continue;	/* val<=' ' is a quick whitespace check      */
	break;			/* (take advantage of fact that vcd is text) */
	}
if(ch=='$')
	{
	GLOBALS->yytext_vcd_partial_c_2[len++]=ch;
	for(;;)
		{
		ch=getch();
		if(ch<0) return(T_EOF);
		if(ch<=' ') continue;
		break;
		}
	}
	else
	{
	is_string=1;
	}

for(GLOBALS->yytext_vcd_partial_c_2[len++]=ch;;GLOBALS->yytext_vcd_partial_c_2[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_partial_c_2)
		{
		GLOBALS->yytext_vcd_partial_c_2=(char *)realloc_2(GLOBALS->yytext_vcd_partial_c_2, (GLOBALS->T_MAX_STR_vcd_partial_c_2=GLOBALS->T_MAX_STR_vcd_partial_c_2*2)+1);
		}
	ch=getch();
	if(ch<=' ') break;
	}
GLOBALS->yytext_vcd_partial_c_2[len]=0;	/* terminator */

if(is_string)
	{
	GLOBALS->yylen_vcd_partial_c_2=len;
	return(T_STRING);
	}

yyshadow=GLOBALS->yytext_vcd_partial_c_2;
do
{
yyshadow++;
for(i=0;i<NUM_TOKENS;i++)
	{
	if(!strcmp(yyshadow,tokens[i]))
		{
		return(i);
		}
	}

} while(*yyshadow=='$'); /* fix for RCS ids in version strings */

return(T_UNKNOWN_KEY);
}


static int get_vartoken_patched(int match_kw)
{
int ch;
int len=0;

if(!GLOBALS->var_prevch_vcd_partial_c_2)
	{
	for(;;)
		{
		ch=getch_patched();
		if(ch<0) { free_2(GLOBALS->varsplit_vcd_partial_c_2); GLOBALS->varsplit_vcd_partial_c_2=NULL; return(V_END); }
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=GLOBALS->var_prevch_vcd_partial_c_2;
	GLOBALS->var_prevch_vcd_partial_c_2=0;
	}

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(GLOBALS->yytext_vcd_partial_c_2[len++]=ch;;GLOBALS->yytext_vcd_partial_c_2[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_partial_c_2)
		{
		GLOBALS->yytext_vcd_partial_c_2=(char *)realloc_2(GLOBALS->yytext_vcd_partial_c_2, (GLOBALS->T_MAX_STR_vcd_partial_c_2=GLOBALS->T_MAX_STR_vcd_partial_c_2*2)+1);
		}
	ch=getch_patched();
	if(ch<0) { free_2(GLOBALS->varsplit_vcd_partial_c_2); GLOBALS->varsplit_vcd_partial_c_2=NULL; break; }
	if((ch==':')||(ch==']'))
		{
		GLOBALS->var_prevch_vcd_partial_c_2=ch;
		break;
		}
	}
GLOBALS->yytext_vcd_partial_c_2[len]=0;	/* terminator */

if(match_kw)
	{
	int vt = vcd_keyword_code(GLOBALS->yytext_vcd_partial_c_2, len);
	if(vt != V_STRING)
		{
		if(ch<0) { free_2(GLOBALS->varsplit_vcd_partial_c_2); GLOBALS->varsplit_vcd_partial_c_2=NULL; }
		return(vt);
		}
	}

GLOBALS->yylen_vcd_partial_c_2=len;
if(ch<0) { free_2(GLOBALS->varsplit_vcd_partial_c_2); GLOBALS->varsplit_vcd_partial_c_2=NULL; }
return(V_STRING);
}

static int get_vartoken(int match_kw)
{
int ch;
int len=0;

if(GLOBALS->varsplit_vcd_partial_c_2)
	{
	int rc=get_vartoken_patched(match_kw);
	if(rc!=V_END) return(rc);
	GLOBALS->var_prevch_vcd_partial_c_2=0;
	}

if(!GLOBALS->var_prevch_vcd_partial_c_2)
	{
	for(;;)
		{
		ch=getch();
		if(ch<0) return(V_END);
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=GLOBALS->var_prevch_vcd_partial_c_2;
	GLOBALS->var_prevch_vcd_partial_c_2=0;
	}

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

if(ch=='#')     /* for MTI System Verilog '$var reg 64 >w #implicit-var###VarElem:ram_di[0.0] [63:0] $end' style declarations */
        {       /* debussy simply escapes until the space */
        GLOBALS->yytext_vcd_partial_c_2[len++]= '\\';
        }

for(GLOBALS->yytext_vcd_partial_c_2[len++]=ch;;GLOBALS->yytext_vcd_partial_c_2[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_partial_c_2)
		{
		GLOBALS->yytext_vcd_partial_c_2=(char *)realloc_2(GLOBALS->yytext_vcd_partial_c_2, (GLOBALS->T_MAX_STR_vcd_partial_c_2=GLOBALS->T_MAX_STR_vcd_partial_c_2*2)+1);
		}

	ch=getch();
        if(ch==' ')
                {
                if(match_kw) break;
                if(getch_peek() == '[')
                        {
                        ch = getch();
                        GLOBALS->varsplit_vcd_partial_c_2=GLOBALS->yytext_vcd_partial_c_2+len; /* keep looping so we get the *last* one */
                        continue;
                        }
                }

	if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
	if((ch=='[')&&(GLOBALS->yytext_vcd_partial_c_2[0]!='\\'))
		{
		GLOBALS->varsplit_vcd_partial_c_2=GLOBALS->yytext_vcd_partial_c_2+len;		/* keep looping so we get the *last* one */
		}
	else
	if(((ch==':')||(ch==']'))&&(!GLOBALS->varsplit_vcd_partial_c_2)&&(GLOBALS->yytext_vcd_partial_c_2[0]!='\\'))
		{
		GLOBALS->var_prevch_vcd_partial_c_2=ch;
		break;
		}
	}
GLOBALS->yytext_vcd_partial_c_2[len]=0;	/* absolute terminator */
if((GLOBALS->varsplit_vcd_partial_c_2)&&(GLOBALS->yytext_vcd_partial_c_2[len-1]==']'))
	{
	char *vst;
	vst=malloc_2(strlen(GLOBALS->varsplit_vcd_partial_c_2)+1);
	strcpy(vst, GLOBALS->varsplit_vcd_partial_c_2);

	*GLOBALS->varsplit_vcd_partial_c_2=0x00;		/* zero out var name at the left bracket */
	len=GLOBALS->varsplit_vcd_partial_c_2-GLOBALS->yytext_vcd_partial_c_2;

	GLOBALS->varsplit_vcd_partial_c_2=GLOBALS->vsplitcurr_vcd_partial_c_2=vst;
	GLOBALS->var_prevch_vcd_partial_c_2=0;
	}
	else
	{
	GLOBALS->varsplit_vcd_partial_c_2=NULL;
	}

if(match_kw)
	{
	int vt = vcd_keyword_code(GLOBALS->yytext_vcd_partial_c_2, len);
	if(vt != V_STRING)
		{
		return(vt);
		}
	}

GLOBALS->yylen_vcd_partial_c_2=len;
return(V_STRING);
}

static int get_strtoken(void)
{
int ch;
int len=0;

if(!GLOBALS->var_prevch_vcd_partial_c_2)
      {
      for(;;)
              {
              ch=getch();
              if(ch<0) return(V_END);
              if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
              break;
              }
      }
      else
      {
      ch=GLOBALS->var_prevch_vcd_partial_c_2;
      GLOBALS->var_prevch_vcd_partial_c_2=0;
      }

for(GLOBALS->yytext_vcd_partial_c_2[len++]=ch;;GLOBALS->yytext_vcd_partial_c_2[len++]=ch)
      {
	if(len==GLOBALS->T_MAX_STR_vcd_partial_c_2)
		{
		GLOBALS->yytext_vcd_partial_c_2=(char *)realloc_2(GLOBALS->yytext_vcd_partial_c_2, (GLOBALS->T_MAX_STR_vcd_partial_c_2=GLOBALS->T_MAX_STR_vcd_partial_c_2*2)+1);
		}
      ch=getch();
      if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
      }
GLOBALS->yytext_vcd_partial_c_2[len]=0;        /* terminator */

GLOBALS->yylen_vcd_partial_c_2=len;
return(V_STRING);
}

static void sync_end(char *hdr)
{
int tok;

if(hdr) { DEBUG(fprintf(stderr,"%s",hdr)); }
for(;;)
	{
	tok=get_token();
	if((tok==T_END)||(tok==T_EOF)) break;
	if(hdr) { DEBUG(fprintf(stderr," %s",GLOBALS->yytext_vcd_partial_c_2)); }
	}
if(hdr) { DEBUG(fprintf(stderr,"\n")); }
}

static int version_sync_end(char *hdr)
{
int tok;
int rc = 0;

if(hdr) { DEBUG(fprintf(stderr,"%s",hdr)); }
for(;;)
	{
	tok=get_token();
	if((tok==T_END)||(tok==T_EOF)) break;
	if(hdr) { DEBUG(fprintf(stderr," %s",GLOBALS->yytext_vcd_partial_c_2)); }
        if(strstr(GLOBALS->yytext_vcd_partial_c_2, "Icarus"))   /* turn off autocoalesce for Icarus */
                {
		GLOBALS->autocoalesce = 0;
                rc = 1;
                }
	}
if(hdr) { DEBUG(fprintf(stderr,"\n")); }
return(rc);
}


static void parse_valuechange(void)
{
struct vcdsymbol *v;
char *vector;
int vlen;
/* hptr hsuf; */ /* scan-build */

switch(GLOBALS->yytext_vcd_partial_c_2[0])
	{
	case '0':
	case '1':
	case 'x': case 'X':
	case 'z': case 'Z':
	case 'h': case 'H':
	case 'u': case 'U':
	case 'w': case 'W':
	case 'l': case 'L':
	case '-':
		if(GLOBALS->yylen_vcd_partial_c_2>1)
			{
			v=bsearch_vcd(GLOBALS->yytext_vcd_partial_c_2+1, GLOBALS->yylen_vcd_partial_c_2-1);
			if(!v)
				{
				fprintf(stderr,"Near byte %d, Unknown VCD identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)),GLOBALS->yytext_vcd_partial_c_2+1);
				}
				else
				{
				v->value[0]=GLOBALS->yytext_vcd_partial_c_2[0];
				DEBUG(fprintf(stderr,"%s = '%c'\n",v->name,v->value[0]));

				v->narray[0]->curr = v->app_array[0];
				/* hsuf = */ add_histent_p(GLOBALS->current_time_vcd_partial_c_2,v->narray[0],v->value[0],1, NULL); /* scan-build */
				v->app_array[0] = v->narray[0]->curr;
				v->narray[0]->curr->next = v->tr_array[0];
				if(v->narray[0]->harray) { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
				}
			}
			else
			{
			fprintf(stderr,"Near byte %d, Malformed VCD identifier\n", (int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)));
			}
		break;

	case 'b':
	case 'B':
		{
		/* extract binary number then.. */
		vector=malloc_2(GLOBALS->yylen_cache_vcd_partial_c_2=GLOBALS->yylen_vcd_partial_c_2);
		strcpy(vector,GLOBALS->yytext_vcd_partial_c_2+1);
		vlen=GLOBALS->yylen_vcd_partial_c_2-1;

		get_strtoken();
		v=bsearch_vcd(GLOBALS->yytext_vcd_partial_c_2, GLOBALS->yylen_vcd_partial_c_2);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), GLOBALS->yytext_vcd_partial_c_2);
			free_2(vector);
			}
			else
			{
			if ((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE)||((GLOBALS->convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				double *d;
				char *pnt;
				char ch;
				TimeType k=0;

				pnt=vector;
				while((ch=*(pnt++))) { k=(k<<1)|((ch=='1')?1:0); }
				free_2(vector);

				d=malloc_2(sizeof(double));
				*d=(double)k;

				if(!v)
					{
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), GLOBALS->yytext_vcd_partial_c_2);
					free_2(d);
					}
					else
					{
					v->narray[0]->curr = v->app_array[0];
					add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[0],'g',1,(char *)d);
					v->app_array[0] = v->narray[0]->curr;
					v->narray[0]->curr->next = v->tr_array[0];
					if(v->narray[0]->harray)
					        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
					}
				break;
				}

			if(vlen<v->size) 	/* fill in left part */
				{
				char extend;
				int i, fill;

				extend=(vector[0]=='1')?'0':vector[0];

				fill=v->size-vlen;
				for(i=0;i<fill;i++)
					{
					v->value[i]=extend;
					}
				strcpy(v->value+fill,vector);
				}
			else if(vlen==v->size) 	/* straight copy */
				{
				strcpy(v->value,vector);
				}
			else			/* too big, so copy only right half */
				{
				int skip;

				skip=vlen-v->size;
				strcpy(v->value,vector+skip);
				}
			DEBUG(fprintf(stderr,"%s = '%s'\n",v->name, v->value));

			if((v->size==1)||(!GLOBALS->atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					v->narray[i]->curr = v->app_array[i];
					add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[i],v->value[i],1, NULL);
					v->app_array[i] = v->narray[i]->curr;
					v->narray[i]->curr->next = v->tr_array[i];
					if(v->narray[i]->harray)
					        { free_2(v->narray[i]->harray); v->narray[i]->harray = NULL; }
					}
				free_2(vector);
				}
				else
				{
				if(GLOBALS->yylen_cache_vcd_partial_c_2!=(v->size+1))
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				v->narray[0]->curr = v->app_array[0];
				add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[0],0,1,vector);
				v->app_array[0] = v->narray[0]->curr;
				v->narray[0]->curr->next = v->tr_array[0];
				if(v->narray[0]->harray)
				        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
				}

			}
		break;
		}

	case 'p':
		/* extract port dump value.. */
		vector=malloc_2(GLOBALS->yylen_cache_vcd_partial_c_2=GLOBALS->yylen_vcd_partial_c_2);
		strcpy(vector,GLOBALS->yytext_vcd_partial_c_2+1);
		vlen=GLOBALS->yylen_vcd_partial_c_2-1;

		get_strtoken();	/* throw away 0_strength_component */
		get_strtoken(); /* throw away 0_strength_component */
		get_strtoken(); /* this is the id                  */
		v=bsearch_vcd(GLOBALS->yytext_vcd_partial_c_2, GLOBALS->yylen_vcd_partial_c_2);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), GLOBALS->yytext_vcd_partial_c_2);
			free_2(vector);
			}
			else
			{
			if ((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE)||((GLOBALS->convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				double *d;
				char *pnt;
				char ch;
				TimeType k=0;

				pnt=vector;
				while((ch=*(pnt++))) { k=(k<<1)|((ch=='1')?1:0); }
				free_2(vector);

				d=malloc_2(sizeof(double));
				*d=(double)k;

				if(!v)
					{
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), GLOBALS->yytext_vcd_partial_c_2);
					free_2(d);
					}
					else
					{
					v->narray[0]->curr = v->app_array[0];
					add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[0],'g',1,(char *)d);
					v->app_array[0] = v->narray[0]->curr;
					v->narray[0]->curr->next = v->tr_array[0];
					if(v->narray[0]->harray)
					        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
					}
				break;
				}

			if(vlen<v->size) 	/* fill in left part */
				{
				char extend;
				int i, fill;

				extend='0';

				fill=v->size-vlen;
				for(i=0;i<fill;i++)
					{
					v->value[i]=extend;
					}
				evcd_strcpy(v->value+fill,vector);
				}
			else if(vlen==v->size) 	/* straight copy */
				{
				evcd_strcpy(v->value,vector);
				}
			else			/* too big, so copy only right half */
				{
				int skip;

				skip=vlen-v->size;
				evcd_strcpy(v->value,vector+skip);
				}
			DEBUG(fprintf(stderr,"%s = '%s'\n",v->name, v->value));

			if((v->size==1)||(!GLOBALS->atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					v->narray[i]->curr = v->app_array[i];
					add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[i],v->value[i],1, NULL);
					v->app_array[i] = v->narray[i]->curr;
					v->narray[i]->curr->next = v->tr_array[i];
					if(v->narray[i]->harray)
					        { free_2(v->narray[i]->harray); v->narray[i]->harray = NULL; }
					}
				free_2(vector);
				}
				else
				{
				if(GLOBALS->yylen_cache_vcd_partial_c_2<v->size)
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				v->narray[0]->curr = v->app_array[0];
				add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[0],0,1,vector);
				v->app_array[0] = v->narray[0]->curr;
				v->narray[0]->curr->next = v->tr_array[0];
				if(v->narray[0]->harray)
				        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
				}
			}
		break;


	case 'r':
	case 'R':
		{
		double *d;

		d=malloc_2(sizeof(double));
		sscanf(GLOBALS->yytext_vcd_partial_c_2+1,"%lg",d);

		get_strtoken();
		v=bsearch_vcd(GLOBALS->yytext_vcd_partial_c_2, GLOBALS->yylen_vcd_partial_c_2);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), GLOBALS->yytext_vcd_partial_c_2);
			free_2(d);
			}
			else
			{
			v->narray[0]->curr = v->app_array[0];
			add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[0],'g',1,(char *)d);
			v->app_array[0] = v->narray[0]->curr;
			v->narray[0]->curr->next = v->tr_array[0];
			if(v->narray[0]->harray)
			        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
			}

		break;
		}

#ifndef STRICT_VCD_ONLY
	case 's':
	case 'S':
		{
		char *d;

                d=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2);
                vlen = fstUtilityEscToBin((unsigned char *)d, (unsigned char *)(GLOBALS->yytext_vcd_partial_c_2+1), GLOBALS->yylen_vcd_partial_c_2); /* includes 0 term */
                if(vlen != GLOBALS->yylen_vcd_partial_c_2)
                        {
                        d = realloc_2(d, vlen);
                        }

		get_strtoken();
		v=bsearch_vcd(GLOBALS->yytext_vcd_partial_c_2, GLOBALS->yylen_vcd_partial_c_2);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), GLOBALS->yytext_vcd_partial_c_2);
			free_2(d);
			}
			else
			{
			v->narray[0]->curr = v->app_array[0];
			add_histent_p(GLOBALS->current_time_vcd_partial_c_2, v->narray[0],'s',1,(char *)d);
			v->app_array[0] = v->narray[0]->curr;
			v->narray[0]->curr->next = v->tr_array[0];
			if(v->narray[0]->harray)
			        { free_2(v->narray[0]->harray); v->narray[0]->harray = NULL; }
			}

		break;
		}
#endif
	}

}


static void evcd_strcpy(char *dst, char *src)
{
static const char *evcd="DUNZduLHXTlh01?FAaBbCcf";
static const char  *vcd="01xz0101xz0101xzxxxxxxz";

char ch;
int i;

while((ch=*src))
	{
	for(i=0;i<23;i++)
		{
		if(evcd[i]==ch)
			{
			*dst=vcd[i];
			break;
			}
		}
	if(i==23) *dst='x';

	src++;
	dst++;
	}

*dst=0;	/* null terminate destination */
}


static void vcd_parse(void)
{
int tok;
unsigned char ttype;
int disable_autocoalesce = 0;

for(;;)
	{
	switch(get_token())
		{
		case T_COMMENT:
			sync_end("COMMENT:");
			break;
		case T_DATE:
			sync_end("DATE:");
			break;
		case T_VERSION:
			disable_autocoalesce = version_sync_end("VERSION:");
			break;
                case T_TIMEZERO:
                        {
                        int vtok=get_token();
                        if((vtok==T_END)||(vtok==T_EOF)) break;
                        GLOBALS->global_time_offset=atoi_64(GLOBALS->yytext_vcd_partial_c_2);

                        DEBUG(fprintf(stderr,"TIMEZERO: "TTFormat"\n",GLOBALS->global_time_offset));
                        sync_end(NULL);
                        }
                        break;
		case T_TIMESCALE:
			{
			int vtok;
			int i;
			char prefix=' ';

			vtok=get_token();
			if((vtok==T_END)||(vtok==T_EOF)) break;
			fractional_timescale_fix(GLOBALS->yytext_vcd_partial_c_2);
			GLOBALS->time_scale=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
			if(!GLOBALS->time_scale) GLOBALS->time_scale=1;
			for(i=0;i<GLOBALS->yylen_vcd_partial_c_2;i++)
				{
				if((GLOBALS->yytext_vcd_partial_c_2[i]<'0')||(GLOBALS->yytext_vcd_partial_c_2[i]>'9'))
					{
					prefix=GLOBALS->yytext_vcd_partial_c_2[i];
					break;
					}
				}
			if(prefix==' ')
				{
				vtok=get_token();
				if((vtok==T_END)||(vtok==T_EOF)) break;
				prefix=GLOBALS->yytext_vcd_partial_c_2[0];
				}
			switch(prefix)
				{
				case ' ':
				case 'm':
				case 'u':
				case 'n':
				case 'p':
				case 'f':
				case 'a':
				case 'z':
					GLOBALS->time_dimension=prefix;
					break;
				case 's':
					GLOBALS->time_dimension=' ';
					break;
				default:	/* unknown */
					GLOBALS->time_dimension='n';
					break;
				}

			DEBUG(fprintf(stderr,"TIMESCALE: "TTFormat" %cs\n",GLOBALS->time_scale, GLOBALS->time_dimension));
			sync_end(NULL);
			}
			break;
		case T_SCOPE:
			T_GET;
                                {
                                switch(GLOBALS->yytext_vcd_partial_c_2[0])
                                        {
                                        case 'm':       ttype = TREE_VCD_ST_MODULE; break;
                                        case 't':       ttype = TREE_VCD_ST_TASK; break;
                                        case 'f':       ttype = (GLOBALS->yytext_vcd_partial_c_2[1] == 'u') ? TREE_VCD_ST_FUNCTION : TREE_VCD_ST_FORK; break;
                                        case 'b':       ttype = TREE_VCD_ST_BEGIN; break;
					case 'g':       ttype = TREE_VCD_ST_GENERATE; break;
					case 's':       ttype = TREE_VCD_ST_STRUCT; break;
					case 'u':       ttype = TREE_VCD_ST_UNION; break;
					case 'c':       ttype = TREE_VCD_ST_CLASS; break;
					case 'i':       ttype = TREE_VCD_ST_INTERFACE; break;
					case 'p':       ttype = (GLOBALS->yytext_vcd_partial_c_2[1] == 'r') ? TREE_VCD_ST_PROGRAM : TREE_VCD_ST_PACKAGE; break;

                                        case 'v':       {
                                                        char *vht = GLOBALS->yytext_vcd_partial_c_2;
                                                        if(!strncmp(vht, "vhdl_", 5))
                                                                {
                                                                switch(vht[5])
                                                                        {
                                                                        case 'a':       ttype = TREE_VHDL_ST_ARCHITECTURE; break;
                                                                        case 'r':       ttype = TREE_VHDL_ST_RECORD; break;
                                                                        case 'b':       ttype = TREE_VHDL_ST_BLOCK; break;
                                                                        case 'g':       ttype = TREE_VHDL_ST_GENERATE; break;
                                                                        case 'i':       ttype = TREE_VHDL_ST_GENIF; break;
                                                                        case 'f':       ttype = (vht[6] == 'u') ? TREE_VHDL_ST_FUNCTION : TREE_VHDL_ST_GENFOR; break;
		                                                       	case 'p':       ttype = (!strncmp(vht+6, "roces", 5)) ? TREE_VHDL_ST_PROCESS: TREE_VHDL_ST_PROCEDURE; break;
                                                                        default:        ttype = TREE_UNKNOWN; break;
                                                                        }
                                                                }
                                                                else
                                                                {
                                                                ttype = TREE_UNKNOWN;
                                                                }
                                                        }
                                                        break;

                                        default:        ttype = TREE_UNKNOWN;
                                                        break;
                                        }
                                }
			T_GET;
			if(tok==T_STRING)
				{
				struct slist *s;
				s=(struct slist *)calloc_2(1,sizeof(struct slist));
				s->len=GLOBALS->yylen_vcd_partial_c_2;
				s->str=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+1);
				strcpy(s->str, GLOBALS->yytext_vcd_partial_c_2);
				s->mod_tree_parent = GLOBALS->mod_tree_parent;

				allocate_and_decorate_module_tree_node(ttype, GLOBALS->yytext_vcd_partial_c_2, NULL, GLOBALS->yylen_vcd_partial_c_2, 0, 0, 0);

				if(GLOBALS->slistcurr)
					{
					GLOBALS->slistcurr->next=s;
					GLOBALS->slistcurr=s;
					}
					else
					{
					GLOBALS->slistcurr=GLOBALS->slistroot=s;
					}

				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",GLOBALS->slisthier));
				}
			sync_end(NULL);
			break;
		case T_UPSCOPE:
			if(GLOBALS->slistroot)
				{
				struct slist *s;

				GLOBALS->mod_tree_parent = GLOBALS->slistcurr->mod_tree_parent;
				s=GLOBALS->slistroot;
				if(!s->next)
					{
					free_2(s->str);
					free_2(s);
					GLOBALS->slistroot=GLOBALS->slistcurr=NULL;
					}
				else
				for(;;)
					{
					if(!s->next->next)
						{
						free_2(s->next->str);
						free_2(s->next);
						s->next=NULL;
						GLOBALS->slistcurr=s;
						break;
						}
					s=s->next;
					}
				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",GLOBALS->slisthier));
				}
				else
				{
				GLOBALS->mod_tree_parent = NULL;
				}
			sync_end(NULL);
			break;
		case T_VAR:
			if((GLOBALS->header_over_vcd_partial_c_2)&&(0))
			{
			fprintf(stderr,"$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, exiting.\n",
				(int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)));
			exit(0);
			}
			else
			{
			int vtok;
			struct vcdsymbol *v=NULL;

			GLOBALS->var_prevch_vcd_partial_c_2=0;
			if(GLOBALS->varsplit_vcd_partial_c_2)
				{
				free_2(GLOBALS->varsplit_vcd_partial_c_2);
				GLOBALS->varsplit_vcd_partial_c_2=NULL;
				}
			vtok=get_vartoken(1);
			if(vtok>V_STRINGTYPE) goto bail;

			v=(struct vcdsymbol *)calloc_2(1,sizeof(struct vcdsymbol));
			v->vartype=vtok;
			v->msi=v->lsi=GLOBALS->vcd_explicit_zero_subscripts; /* indicate [un]subscripted status */

			if(vtok==V_PORT)
				{
				vtok=get_vartoken(1);
				if(vtok==V_STRING)
					{
					v->size=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
					if(!v->size) v->size=1;
					}
					else
					if(vtok==V_LB)
					{
					vtok=get_vartoken(1);
					if(vtok==V_END) goto err;
					if(vtok!=V_STRING) goto err;
					v->msi=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
					vtok=get_vartoken(0);
					if(vtok==V_RB)
						{
						v->lsi=v->msi;
						v->size=1;
						}
						else
						{
						if(vtok!=V_COLON) goto err;
						vtok=get_vartoken(0);
						if(vtok!=V_STRING) goto err;
						v->lsi=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
						vtok=get_vartoken(0);
						if(vtok!=V_RB) goto err;

						if(v->msi>v->lsi)
							{
							v->size=v->msi-v->lsi+1;
							}
							else
							{
							v->size=v->lsi-v->msi+1;
							}
						}
					}
					else goto err;

				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+1);
				strcpy(v->id, GLOBALS->yytext_vcd_partial_c_2);
                                v->nid=vcdid_hash(GLOBALS->yytext_vcd_partial_c_2,GLOBALS->yylen_vcd_partial_c_2);

                                if(v->nid == (GLOBALS->vcd_hash_max+1))
                                        {
                                        GLOBALS->vcd_hash_max = v->nid;
                                        }
                                else
                                if((v->nid>0)&&(v->nid<=GLOBALS->vcd_hash_max))
                                        {
                                        /* general case with aliases */
                                        }
                                else
                                        {
                                        GLOBALS->vcd_hash_kill = 1;
                                        }

                                if(v->nid < GLOBALS->vcd_minid_vcd_partial_c_2) GLOBALS->vcd_minid_vcd_partial_c_2 = v->nid;
                                if(v->nid > GLOBALS->vcd_maxid_vcd_partial_c_2) GLOBALS->vcd_maxid_vcd_partial_c_2 = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(GLOBALS->slisthier_len)
					{
					v->name=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_partial_c_2+1);
					strcpy(v->name,GLOBALS->slisthier);
					strcpy(v->name+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_partial_c_2,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_partial_c_2)) && (GLOBALS->yytext_vcd_partial_c_2[0] != '\\'))
							{
							char *sd=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_partial_c_2+2);
							strcpy(sd,GLOBALS->slisthier);
							strcpy(sd+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
							sd[GLOBALS->slisthier_len+1] = '\\';
							strcpy(sd+GLOBALS->slisthier_len+2,v->name+GLOBALS->slisthier_len+1);
							free_2(v->name);
							v->name = sd;
							}
						}
					}
					else
					{
					v->name=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+1);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,GLOBALS->yytext_vcd_partial_c_2,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name,GLOBALS->yytext_vcd_partial_c_2)) && (GLOBALS->yytext_vcd_partial_c_2[0] != '\\'))
							{
							char *sd=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+2);
							sd[0] = '\\';
							strcpy(sd+1,v->name);
							free_2(v->name);
							v->name = sd;
							}
						}
					}

                                if(GLOBALS->pv_vcd_partial_c_2)
                                        {
                                        if(!strcmp(GLOBALS->pv_vcd_partial_c_2->name,v->name) && !disable_autocoalesce && (!strchr(v->name, '\\')))
                                                {
                                                GLOBALS->pv_vcd_partial_c_2->chain=v;
                                                v->root=GLOBALS->rootv_vcd_partial_c_2;
                                                if(GLOBALS->pv_vcd_partial_c_2==GLOBALS->rootv_vcd_partial_c_2) GLOBALS->pv_vcd_partial_c_2->root=GLOBALS->rootv_vcd_partial_c_2;
                                                }
                                                else
                                                {
                                                GLOBALS->rootv_vcd_partial_c_2=v;
                                                }
                                        }
					else
					{
					GLOBALS->rootv_vcd_partial_c_2=v;
					}
                                GLOBALS->pv_vcd_partial_c_2=v;
				}
				else	/* regular vcd var, not an evcd port var */
				{
				vtok=get_vartoken(1);
				if(vtok==V_END) goto err;
				v->size=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+1);
				strcpy(v->id, GLOBALS->yytext_vcd_partial_c_2);
                                v->nid=vcdid_hash(GLOBALS->yytext_vcd_partial_c_2,GLOBALS->yylen_vcd_partial_c_2);

                                if(v->nid == (GLOBALS->vcd_hash_max+1))
                                        {
                                        GLOBALS->vcd_hash_max = v->nid;
                                        }
                                else
                                if((v->nid>0)&&(v->nid<=GLOBALS->vcd_hash_max))
                                        {
                                        /* general case with aliases */
                                        }
                                else
                                        {
                                        GLOBALS->vcd_hash_kill = 1;
                                        }

                                if(v->nid < GLOBALS->vcd_minid_vcd_partial_c_2) GLOBALS->vcd_minid_vcd_partial_c_2 = v->nid;
                                if(v->nid > GLOBALS->vcd_maxid_vcd_partial_c_2) GLOBALS->vcd_maxid_vcd_partial_c_2 = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(GLOBALS->slisthier_len)
					{
					v->name=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_partial_c_2+1);
					strcpy(v->name,GLOBALS->slisthier);
					strcpy(v->name+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_partial_c_2,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_partial_c_2)) && (GLOBALS->yytext_vcd_partial_c_2[0] != '\\'))
							{
                                                        char *sd=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_partial_c_2+2);
                                                        strcpy(sd,GLOBALS->slisthier);
                                                        strcpy(sd+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
                                                        sd[GLOBALS->slisthier_len+1] = '\\';
                                                        strcpy(sd+GLOBALS->slisthier_len+2,v->name+GLOBALS->slisthier_len+1);
                                                        free_2(v->name);
                                                        v->name = sd;
							}
						}
					}
					else
					{
					v->name=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+1);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,GLOBALS->yytext_vcd_partial_c_2,GLOBALS->alt_hier_delimeter);
						}
						else
						{
                                                if((strcpy_delimfix(v->name,GLOBALS->yytext_vcd_partial_c_2)) && (GLOBALS->yytext_vcd_partial_c_2[0] != '\\'))
                                                        {
                                                        char *sd=(char *)malloc_2(GLOBALS->yylen_vcd_partial_c_2+2);
                                                        sd[0] = '\\';
                                                        strcpy(sd+1,v->name);
                                                        free_2(v->name);
                                                        v->name = sd;
                                                        }
						}
					}

                                if(GLOBALS->pv_vcd_partial_c_2)
                                        {
                                        if(!strcmp(GLOBALS->pv_vcd_partial_c_2->name,v->name))
                                                {
                                                GLOBALS->pv_vcd_partial_c_2->chain=v;
                                                v->root=GLOBALS->rootv_vcd_partial_c_2;
                                                if(GLOBALS->pv_vcd_partial_c_2==GLOBALS->rootv_vcd_partial_c_2) GLOBALS->pv_vcd_partial_c_2->root=GLOBALS->rootv_vcd_partial_c_2;
                                                }
                                                else
                                                {
                                                GLOBALS->rootv_vcd_partial_c_2=v;
                                                }
                                        }
					else
					{
					GLOBALS->rootv_vcd_partial_c_2=v;
					}
                                GLOBALS->pv_vcd_partial_c_2=v;

				vtok=get_vartoken(1);
				if(vtok==V_END) goto dumpv;
				if(vtok!=V_LB) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->msi=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
				vtok=get_vartoken(0);
				if(vtok==V_RB)
					{
					v->lsi=v->msi;
					goto dumpv;
					}
				if(vtok!=V_COLON) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->lsi=atoi_64(GLOBALS->yytext_vcd_partial_c_2);
				vtok=get_vartoken(0);
				if(vtok!=V_RB) goto err;
				}

			dumpv:
                        if(v->size == 0)
                                {
                                if(v->vartype != V_EVENT)
                                        {
					if(v->vartype != V_STRINGTYPE)
						{
	                                        v->vartype = V_REAL;
						}
                                        }
                                        else
                                        {
                                        v->size = 1;
                                        }

                                } /* MTI fix */

			if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE)||((GLOBALS->convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				if(v->vartype != V_STRINGTYPE)
					{
					v->vartype=V_REAL;
					}
				v->size=1;		/* override any data we parsed in */
				v->msi=v->lsi=0;
				}
			else
			if((v->size>1)&&(v->msi<=0)&&(v->lsi<=0))
				{
				if(v->vartype==V_EVENT)
					{
					v->size=1;
					}
					else
					{
					/* any criteria for the direction here? */
					v->msi=v->size-1;
					v->lsi=0;
					}
				}
			else
			if((v->msi>v->lsi)&&((v->msi-v->lsi+1)!=v->size))
				{
                                if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER))
                                        {
					if((v->msi-v->lsi+1) > v->size) /* if() is 2d add */
						{
	                                        v->msi = v->size-1; v->lsi = 0;
						}
                                        /* all this formerly was goto err; */
                                        }
                                        else
                                        {
                                        v->size=v->msi-v->lsi+1;
                                        }
				}
			else
			if((v->lsi>=v->msi)&&((v->lsi-v->msi+1)!=v->size))
				{
                                if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER))
                                        {
					if((v->lsi-v->msi+1) > v->size) /* if() is 2d add */
						{
	                                        v->lsi = v->size-1; v->msi = 0;
						}
                                        /* all this formerly was goto err; */
                                        }
                                        else
                                        {
                                        v->size=v->lsi-v->msi+1;
                                        }
				}

			/* initial conditions */
			v->value=(char *)malloc_2(v->size+1);
			v->value[v->size]=0;
			v->narray=(struct Node **)calloc_2(v->size,sizeof(struct Node *));
			v->tr_array=(hptr *)calloc_2(v->size,sizeof(hptr));
			v->app_array=(hptr *)calloc_2(v->size,sizeof(hptr));
				{
				int i;
				if(GLOBALS->atomic_vectors)
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';
						}
					v->narray[0]=(struct Node *)calloc_2(1,sizeof(struct Node));
					v->narray[0]->head.time=-1;
					v->narray[0]->head.v.h_val=AN_X;
					}
					else
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';

						v->narray[i]=(struct Node *)calloc_2(1,sizeof(struct Node));
						v->narray[i]->head.time=-1;
						v->narray[i]->head.v.h_val=AN_X;
						}
					}
				}

			if(!GLOBALS->vcdsymroot_vcd_partial_c_2)
				{
				GLOBALS->vcdsymroot_vcd_partial_c_2=GLOBALS->vcdsymcurr_vcd_partial_c_2=v;
				}
				else
				{
				GLOBALS->vcdsymcurr_vcd_partial_c_2->next=v;
				GLOBALS->vcdsymcurr_vcd_partial_c_2=v;
				}
			GLOBALS->numsyms_vcd_partial_c_2++;

			goto bail;
			err:
			if(v)
				{
				GLOBALS->error_count_vcd_partial_c_2++;
				if(v->name)
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered with '%s'\n", (int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)), v->name);
					free_2(v->name);
					}
					else
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered\n", (int)(GLOBALS->vcdbyteno_vcd_partial_c_2+(GLOBALS->vst_vcd_partial_c_2-GLOBALS->vcdbuf_vcd_partial_c_2)));
					}
				if(v->id) free_2(v->id);
				if(v->value) free_2(v->value);
				free_2(v);
				GLOBALS->pv_vcd_partial_c_2 = NULL;
				}

			bail:
			if(vtok!=V_END) sync_end(NULL);
			break;
			}
		case T_ENDDEFINITIONS:
			GLOBALS->header_over_vcd_partial_c_2=1;	/* do symbol table management here */
			create_sorted_table();
			if((!GLOBALS->sorted_vcd_partial_c_2)&&(!GLOBALS->indexed_vcd_partial_c_2))
				{
				fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
				exit(1);
				}
			if(GLOBALS->error_count_vcd_partial_c_2)
				{
				fprintf(stderr, "\n%d VCD parse errors encountered, exiting.\n", GLOBALS->error_count_vcd_partial_c_2);
				exit(1);
				}
			return;
			break;
		case T_STRING:
			if(!GLOBALS->header_over_vcd_partial_c_2)
				{
				GLOBALS->header_over_vcd_partial_c_2=1;	/* do symbol table management here */
				create_sorted_table();
				if((!GLOBALS->sorted_vcd_partial_c_2)&&(!GLOBALS->indexed_vcd_partial_c_2)) break;
				}
				{
				/* catchall for events when header over */
				if(GLOBALS->yytext_vcd_partial_c_2[0]=='#')
					{
					TimeType tim;
					tim=atoi_64(GLOBALS->yytext_vcd_partial_c_2+1);

					if(GLOBALS->start_time_vcd_partial_c_2<0)
						{
						GLOBALS->start_time_vcd_partial_c_2=tim;
						}

					GLOBALS->current_time_vcd_partial_c_2=tim;
					if(GLOBALS->end_time_vcd_partial_c_2<tim) GLOBALS->end_time_vcd_partial_c_2=tim;	/* in case of malformed vcd files */
					DEBUG(fprintf(stderr,"#"TTFormat"\n",tim));
					}
					else
					{
					parse_valuechange();
					}
				}
			break;
		case T_DUMPALL:	/* dump commands modify vals anyway so */
		case T_DUMPPORTSALL:
			break;	/* just loop through..                 */
		case T_DUMPOFF:
		case T_DUMPPORTSOFF:
			GLOBALS->dumping_off_vcd_partial_c_2=1;
                        /* if((!GLOBALS->blackout_regions)||((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart<=GLOBALS->blackout_regions->bend))) : remove redundant condition */
                        if((!GLOBALS->blackout_regions)||(GLOBALS->blackout_regions->bstart<=GLOBALS->blackout_regions->bend))
				{
				struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));

				bt->bstart = GLOBALS->current_time_vcd_partial_c_2 * GLOBALS->time_scale;
				bt->next = GLOBALS->blackout_regions;
				GLOBALS->blackout_regions = bt;
				}
			break;
		case T_DUMPON:
		case T_DUMPPORTSON:
			GLOBALS->dumping_off_vcd_partial_c_2=0;
			if((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart>GLOBALS->blackout_regions->bend))
				{
				GLOBALS->blackout_regions->bend = GLOBALS->current_time_vcd_partial_c_2 * GLOBALS->time_scale;
				}
			break;
		case T_DUMPVARS:
		case T_DUMPPORTS:
			if(GLOBALS->current_time_vcd_partial_c_2<0)
				{ GLOBALS->start_time_vcd_partial_c_2=GLOBALS->current_time_vcd_partial_c_2=GLOBALS->end_time_vcd_partial_c_2=0; }
			break;
		case T_VCDCLOSE:
			sync_end("VCDCLOSE:");
			break;	/* next token will be '#' time related followed by $end */
		case T_END:	/* either closure for dump commands or */
			break;	/* it's spurious                       */
		case T_UNKNOWN_KEY:
			sync_end(NULL);	/* skip over unknown keywords */
			break;
		case T_EOF:
			if((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart>GLOBALS->blackout_regions->bend))
				{
				GLOBALS->blackout_regions->bend = GLOBALS->current_time_vcd_partial_c_2 * GLOBALS->time_scale;
				}
			return;
		default:
			DEBUG(fprintf(stderr,"UNKNOWN TOKEN\n"));
		}
	}
}


/*******************************************************************************/

hptr add_histent_p(TimeType tim, struct Node *n, char ch, int regadd, char *vector)
{
struct HistEnt *he, *rc;
char heval;

if(!vector)
{
if(!(rc=n->curr))
	{
	he=histent_calloc();
        he->time=-1;
        he->v.h_val=AN_X;

	n->curr=he;
	n->head.next=he;

	add_histent_p(tim,n,ch,regadd, vector);
	rc = he;
	}
	else
	{
	if(regadd) { tim*=(GLOBALS->time_scale); }

	if(ch=='0')              heval=AN_0; else
	if(ch=='1')              heval=AN_1; else
        if((ch=='x')||(ch=='X')) heval=AN_X; else
        if((ch=='z')||(ch=='Z')) heval=AN_Z; else
        if((ch=='h')||(ch=='H')) heval=AN_H; else
        if((ch=='u')||(ch=='U')) heval=AN_U; else
        if((ch=='w')||(ch=='W')) heval=AN_W; else
        if((ch=='l')||(ch=='L')) heval=AN_L; else
        /* if(ch=='-') */        heval=AN_DASH;		/* default */

	if((n->curr->v.h_val!=heval)||(tim==GLOBALS->start_time_vcd_partial_c_2)||(n->vartype==ND_VCD_EVENT)||(GLOBALS->vcd_preserve_glitches)) /* same region == go skip */
        	{
		if(n->curr->time==tim)
			{
			DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
				tim, n, AN_STR[n->curr->v.h_val], ch));
			n->curr->v.h_val=heval;		/* we have a glitch! */

			GLOBALS->num_glitches_vcd_partial_c_3++;
			if(!(n->curr->flags&HIST_GLITCH))
				{
				n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
				GLOBALS->num_glitch_regions_vcd_partial_c_3++;
				}
			}
			else
			{
                	he=histent_calloc();
                	he->time=tim;
                	he->v.h_val=heval;

                	n->curr->next=he;
                        if(n->curr->v.h_val==heval)
                                {
                                n->curr->flags|=HIST_GLITCH;    /* set the glitch flag */
                                GLOBALS->num_glitch_regions_vcd_recoder_c_4++;
                                }
			n->curr=he;
                	GLOBALS->regions+=regadd;
			}
                }
       }
}
else
{
switch(ch)
	{
	case 's': /* string */
	{
	if(!(rc=n->curr))
		{
		he=histent_calloc();
		he->flags=(HIST_STRING|HIST_REAL);
	        he->time=-1;
	        he->v.h_vector=NULL;

		n->curr=he;
		n->head.next=he;

		add_histent_p(tim,n,ch,regadd, vector);
		rc = he;
		}
		else
		{
		if(regadd) { tim*=(GLOBALS->time_scale); }

			if(n->curr->time==tim)
				{
				DEBUG(printf("Warning: String Glitch at time ["TTFormat"] Signal [%p].\n",
					tim, n));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */

				GLOBALS->num_glitches_vcd_partial_c_3++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_partial_c_3++;
					}
				}
				else
				{
	                	he=histent_calloc();
				he->flags=(HIST_STRING|HIST_REAL);
	                	he->time=tim;
	                	he->v.h_vector=vector;

	                	n->curr->next=he;
				n->curr=he;
	                	GLOBALS->regions+=regadd;
				}
	       }
	break;
	}
	case 'g': /* real number */
	{
	if(!(rc=n->curr))
		{
		he=histent_calloc();
		he->flags=HIST_REAL;
	        he->time=-1;
#ifdef WAVE_HAS_H_DOUBLE
                he->v.h_double = strtod("NaN", NULL);
#else
	        he->v.h_vector=NULL;
#endif
		n->curr=he;
		n->head.next=he;

		add_histent_p(tim,n,ch,regadd, vector);
		rc = he;
		}
		else
		{
		if(regadd) { tim*=(GLOBALS->time_scale); }

		if(
#ifdef WAVE_HAS_H_DOUBLE
                  (vector&&(n->curr->v.h_double!=*(double *)vector))
#else
		  (n->curr->v.h_vector&&vector&&(*(double *)n->curr->v.h_vector!=*(double *)vector))
#endif
			||(tim==GLOBALS->start_time_vcd_partial_c_2)
#ifndef WAVE_HAS_H_DOUBLE
			||(!n->curr->v.h_vector)
#endif
			||(GLOBALS->vcd_preserve_glitches)||(GLOBALS->vcd_preserve_glitches_real)
			) /* same region == go skip */
	        	{
			if(n->curr->time==tim)
				{
				DEBUG(printf("Warning: Real number Glitch at time ["TTFormat"] Signal [%p].\n",
					tim, n));
#ifdef WAVE_HAS_H_DOUBLE
                                n->curr->v.h_double = *((double *)vector);
#else
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */
#endif
				GLOBALS->num_glitches_vcd_partial_c_3++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_partial_c_3++;
					}
				}
				else
				{
	                	he=histent_calloc();
				he->flags=HIST_REAL;
	                	he->time=tim;
#ifdef WAVE_HAS_H_DOUBLE
                                he->v.h_double = *((double *)vector);
#else
	                	he->v.h_vector=vector;
#endif
	                	n->curr->next=he;
				n->curr=he;
	                	GLOBALS->regions+=regadd;
				}
	                }
			else
			{
#ifndef WAVE_HAS_H_DOUBLE
			free_2(vector);
#endif
			}
#ifdef WAVE_HAS_H_DOUBLE
	        free_2(vector);
#endif
	       }
	break;
	}
	default:
	{
	if(!(rc=n->curr))
		{
		he=histent_calloc();
	        he->time=-1;
	        he->v.h_vector=NULL;

		n->curr=he;
		n->head.next=he;

		add_histent_p(tim,n,ch,regadd, vector);
		rc = he;
		}
		else
		{
		if(regadd) { tim*=(GLOBALS->time_scale); }

		if(
		  (n->curr->v.h_vector&&vector&&(strcmp(n->curr->v.h_vector,vector)))
			||(tim==GLOBALS->start_time_vcd_partial_c_2)
			||(!n->curr->v.h_vector)
			||(GLOBALS->vcd_preserve_glitches)
			) /* same region == go skip */
	        	{
			if(n->curr->time==tim)
				{
				DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
					tim, n, AN_STR[n->curr->v.h_val], ch));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */

				GLOBALS->num_glitches_vcd_partial_c_3++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_partial_c_3++;
					}
				}
				else
				{
	                	he=histent_calloc();
	                	he->time=tim;
	                	he->v.h_vector=vector;

	                	n->curr->next=he;
				n->curr=he;
	                	GLOBALS->regions+=regadd;
				}
	                }
			else
			{
			free_2(vector);
			}
	       }
	break;
	}
	}
}

return(rc);
}


static void add_tail_histents(void)
{
int j;
struct vcdsymbol *v;
hptr rc;


/* do 'x' trailers */

v=GLOBALS->vcdsymroot_vcd_partial_c_2;
while(v)
	{
	if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE))
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=1.0;
		rc = add_histent_p(MAX_HISTENT_TIME-1, v->narray[0], 'g', 0, (char *)d);
		set_vcd_vartype(v, v->narray[0]);
		v->app_array[0] = rc;
		v->tr_array[0] = v->narray[0]->curr;
		}
	else
	if((v->size==1)||(!GLOBALS->atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		rc = add_histent_p(MAX_HISTENT_TIME-1, v->narray[j], 'x', 0, NULL);
		set_vcd_vartype(v, v->narray[j]);
		v->app_array[j] = rc;
		v->tr_array[j] = v->narray[j]->curr;
		}
	else
		{
		rc = add_histent_p(MAX_HISTENT_TIME-1, v->narray[0], 'x', 0, (char *)calloc_2(1,sizeof(char)));
		set_vcd_vartype(v, v->narray[0]);
		v->app_array[0] = rc;
		v->tr_array[0] = v->narray[0]->curr;
		}

	v=v->next;
	}

v=GLOBALS->vcdsymroot_vcd_partial_c_2;
while(v)
	{
	if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE))
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=0.0;
		add_histent_p(MAX_HISTENT_TIME, v->narray[0], 'g', 0, (char *)d);
		}
	else
	if((v->size==1)||(!GLOBALS->atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		add_histent_p(MAX_HISTENT_TIME, v->narray[j], 'z', 0, NULL);
		}
	else
		{
		add_histent_p(MAX_HISTENT_TIME, v->narray[0], 'z', 0, (char *)calloc_2(1,sizeof(char)));
		}

	v=v->next;
	}
}

/*******************************************************************************/

static void vcd_build_symbols(void)
{
int j;
int max_slen=-1;
struct sym_chain *sym_chain=NULL, *sym_curr=NULL;
int duphier=0;
char hashdirty;
struct vcdsymbol *v, *vprime;
char *str = wave_alloca(1); /* quiet scan-build null pointer warning below */
#ifdef _WAVE_HAVE_JUDY
int ss_len, longest = 0;
#endif

v=GLOBALS->vcdsymroot_vcd_partial_c_2;
while(v)
	{
	int msi;
	int delta;

		{
		int slen;
		int substnode;

		msi=v->msi;
		delta=((v->lsi-v->msi)<0)?-1:1;
		substnode=0;

		slen=strlen(v->name);
		str=(slen>max_slen)?(wave_alloca((max_slen=slen)+32)):(str); /* more than enough */
		strcpy(str,v->name);

		if(v->msi>=0)
			{
			strcpy(str+slen,GLOBALS->vcd_hier_delimeter);
			slen++;
			}

		if((vprime=bsearch_vcd(v->id, strlen(v->id)))!=v) /* hash mish means dup net */
			{
			if(v->size!=vprime->size)
				{
				fprintf(stderr,"ERROR: Duplicate IDs with differing width: %s %s\n", v->name, vprime->name);
				}
				else
				{
				substnode=1;
				}
			}

		if(((v->size==1)||(!GLOBALS->atomic_vectors))&&(v->vartype!=V_REAL)&&(v->vartype!=V_STRINGTYPE))
			{
			struct symbol *s = NULL;

			for(j=0;j<v->size;j++)
				{
				if(v->msi>=0)
					{
					if(!GLOBALS->vcd_explicit_zero_subscripts)
						sprintf(str+slen,"%d",msi);
						else
						sprintf(str+slen-1,"[%d]",msi);
					}

				hashdirty=0;
				if(symfind(str, NULL))
					{
					char *dupfix=(char *)malloc_2(max_slen+32);
#ifndef _WAVE_HAVE_JUDY
					hashdirty=1;
#endif
					DEBUG(fprintf(stderr,"Warning: %s is a duplicate net name.\n",str));

					do sprintf(dupfix, "$DUP%d%s%s", duphier++, GLOBALS->vcd_hier_delimeter, str);
						while(symfind(dupfix, NULL));

					strcpy(str, dupfix);
					free_2(dupfix);
					duphier=0; /* reset for next duplicate resolution */
					}
					/* fallthrough */
					{
					s=symadd(str,hashdirty?hash(str):GLOBALS->hashcache);
#ifdef _WAVE_HAVE_JUDY
                                        ss_len = strlen(str); if(ss_len >= longest) { longest = ss_len + 1; }
#endif
					s->n=v->narray[j];
					if(substnode)
						{
						struct Node *n, *n2;

						n=s->n;
						n2=vprime->narray[j];
						/* nname stays same */
						n->head=n2->head;
						n->curr=n2->curr;
						/* harray calculated later */
						n->numhist=n2->numhist;
						}

#ifndef _WAVE_HAVE_JUDY
					s->n->nname=s->name;
#endif
					if(!GLOBALS->firstnode)
					        {
					        GLOBALS->firstnode=
					        GLOBALS->curnode=calloc_2(1, sizeof(struct symchain));
					        }
					        else
					        {
					        GLOBALS->curnode->next=calloc_2(1, sizeof(struct symchain));
					        GLOBALS->curnode=GLOBALS->curnode->next;
					        }
					GLOBALS->curnode->symbol=s;

					GLOBALS->numfacs++;
					DEBUG(fprintf(stderr,"Added: %s\n",str));
					}
				msi+=delta;
				}

			if((j==1)&&(v->root))
				{
				s->vec_root=(struct symbol *)v->root;		/* these will get patched over */
				s->vec_chain=(struct symbol *)v->chain;		/* these will get patched over */
				v->sym_chain=s;

				if(!sym_chain)
					{
					sym_curr=(struct sym_chain *)calloc_2(1,sizeof(struct sym_chain));
					sym_chain=sym_curr;
					}
					else
					{
					sym_curr->next=(struct sym_chain *)calloc_2(1,sizeof(struct sym_chain));
					sym_curr=sym_curr->next;
					}
				sym_curr->val=s;
				}
			}
			else	/* atomic vector */
			{
			if((v->vartype!=V_REAL)&&(v->vartype!=V_STRINGTYPE)&&(v->vartype!=V_INTEGER)&&(v->vartype!=V_PARAMETER))
			/* if((v->vartype!=V_REAL)&&(v->vartype!=V_STRINGTYPE)) */
				{
				sprintf(str+slen-1,"[%d:%d]",v->msi,v->lsi);
				/* 2d add */
                                if((v->msi>v->lsi)&&((v->msi-v->lsi+1)!=v->size))   
                                        {
                                        if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER))
                                                {
                                                v->msi = v->size-1; v->lsi = 0;
                                                }
                                        }
                                else
                                if((v->lsi>=v->msi)&&((v->lsi-v->msi+1)!=v->size))
                                        {
                                        if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER))
                                                {
                                                v->lsi = v->size-1; v->msi = 0;
                                                }
                                        }
				}
				else
				{
				*(str+slen-1)=0;
				}


			hashdirty=0;
			if(symfind(str, NULL))
				{
				char *dupfix=(char *)malloc_2(max_slen+32);
#ifndef _WAVE_HAVE_JUDY
				hashdirty=1;
#endif
				DEBUG(fprintf(stderr,"Warning: %s is a duplicate net name.\n",str));

				do sprintf(dupfix, "$DUP%d%s%s", duphier++, GLOBALS->vcd_hier_delimeter, str);
					while(symfind(dupfix, NULL));

				strcpy(str, dupfix);
				free_2(dupfix);
				duphier=0; /* reset for next duplicate resolution */
				}
				/* fallthrough */
				{
				struct symbol *s;

				s=symadd(str,hashdirty?hash(str):GLOBALS->hashcache);	/* cut down on double lookups.. */
#ifdef _WAVE_HAVE_JUDY
                                ss_len = strlen(str); if(ss_len >= longest) { longest = ss_len + 1; }
#endif
				s->n=v->narray[0];
				if(substnode)
					{
					struct Node *n, *n2;

					n=s->n;
					n2=vprime->narray[0];
					/* nname stays same */
					n->head=n2->head;
					n->curr=n2->curr;
					/* harray calculated later */
					n->numhist=n2->numhist;
					n->extvals=n2->extvals;
					n->msi=n2->msi;
					n->lsi=n2->lsi;
					}
					else
					{
					s->n->msi=v->msi;
					s->n->lsi=v->lsi;

					s->n->extvals=1;
					}

#ifndef _WAVE_HAVE_JUDY
				s->n->nname=s->name;
#endif
				if(!GLOBALS->firstnode)
				        {
				        GLOBALS->firstnode=
				        GLOBALS->curnode=calloc_2(1, sizeof(struct symchain));
				        }
				        else
				        {
				        GLOBALS->curnode->next=calloc_2(1, sizeof(struct symchain));
				        GLOBALS->curnode=GLOBALS->curnode->next;
				        }
				GLOBALS->curnode->symbol=s;

				GLOBALS->numfacs++;
				DEBUG(fprintf(stderr,"Added: %s\n",str));
				}
			}
		}

	v=v->next;
	}

#ifdef _WAVE_HAVE_JUDY
{
Pvoid_t  PJArray = GLOBALS->sym_judy;
PPvoid_t PPValue;
char *Index = calloc_2(1, longest);

for (PPValue  = JudySLFirst (PJArray, (uint8_t *)Index, PJE0);
         PPValue != (PPvoid_t) NULL;
         PPValue  = JudySLNext  (PJArray, (uint8_t *)Index, PJE0))
    {
        struct symbol *s = *(struct symbol **)PPValue;
        s->name = strdup_2(Index);
        s->n->nname = s->name;
    }

free_2(Index);
}
#endif

if(sym_chain)
	{
	sym_curr=sym_chain;
	while(sym_curr)
		{
		sym_curr->val->vec_root= ((struct vcdsymbol *)sym_curr->val->vec_root)->sym_chain;

		if ((struct vcdsymbol *)sym_curr->val->vec_chain)
			sym_curr->val->vec_chain=((struct vcdsymbol *)sym_curr->val->vec_chain)->sym_chain;

		DEBUG(printf("Link: ('%s') '%s' -> '%s'\n",sym_curr->val->vec_root->name, sym_curr->val->name, sym_curr->val->vec_chain?sym_curr->val->vec_chain->name:"(END)"));

		sym_chain=sym_curr;
		sym_curr=sym_curr->next;
		free_2(sym_chain);
		}
	}
}

/*******************************************************************************/

static void vcd_cleanup(void)
{
struct slist *s, *s2;

if(GLOBALS->slisthier) { free_2(GLOBALS->slisthier); GLOBALS->slisthier=NULL; }
s=GLOBALS->slistroot;
while(s)
	{
	s2=s->next;
	if(s->str)free_2(s->str);
	free_2(s);
	s=s2;
	}

GLOBALS->slistroot=GLOBALS->slistcurr=NULL; GLOBALS->slisthier_len=0;
}

/*******************************************************************************/

TimeType vcd_partial_main(char *fname)
{
unsigned int shmidu = ~(0L);
int shmid;

GLOBALS->pv_vcd_partial_c_2=GLOBALS->rootv_vcd_partial_c_2=NULL;
GLOBALS->vcd_hier_delimeter[0]=GLOBALS->hier_delimeter;

errno=0;	/* reset in case it's set for some reason */

GLOBALS->yytext_vcd_partial_c_2=(char *)malloc_2(GLOBALS->T_MAX_STR_vcd_partial_c_2+1);

if(!GLOBALS->hier_was_explicitly_set) /* set default hierarchy split char */
	{
	GLOBALS->hier_delimeter='.';
	}

#ifdef __MINGW32__
if(GetEnvironmentVariable("SHMID", NULL, 0))
 	{
	char sEnv[128];
	GetEnvironmentVariable("SHMID", sEnv, 128);
  	sscanf(sEnv, "%x", &shmidu);
 	}
else
#endif
	{
	if(!strcmp(fname, "-vcd"))
		{
		if(!fscanf(stdin, "%x", &shmidu)) shmidu = ~(0L); /* allow use of -v flag to pass straight from stdin */
		}
		else
		{
		sscanf(fname, "%x", &shmidu);	/* passed as a filename */
		}
	}

shmid = (int)shmidu;

#if !defined _MSC_VER
#ifdef __MINGW32__
{
HANDLE hMapFile;
char mapName[257];

sprintf(mapName, "shmidcat%d", shmid);
hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapName);
if(hMapFile == NULL)
	{
	fprintf(stderr, "Could not attach shared memory map name '%s', exiting.\n", mapName);
	exit(255);
	}
GLOBALS->consume_ptr_vcd_partial_c_1 = GLOBALS->buf_vcd_partial_c_2 = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE);
if(GLOBALS->consume_ptr_vcd_partial_c_1 == NULL)
	{
	fprintf(stderr, "Could not map view of file '%s', exiting.\n", mapName);
	exit(255);
	}
}
#else
errno = 0;
GLOBALS->consume_ptr_vcd_partial_c_1 = GLOBALS->buf_vcd_partial_c_2 = shmat(shmid, NULL, 0);
if(errno)
	{
	fprintf(stderr, "Could not attach shared memory ID %08x\n", shmid);
	perror("Why");
	exit(255);
	}
#endif
#else
fprintf(stderr, "Interactive VCD mode does not work with Windows, exiting.\n");
exit(255);
#endif

sym_hash_initialize(GLOBALS);
getch_alloc();		/* alloc membuff for vcd getch buffer */
build_slisthier();

GLOBALS->vcd_preserve_glitches = 1; /* splicing dictates that we override */
while(!GLOBALS->header_over_vcd_partial_c_2) { vcd_parse(); }

if(GLOBALS->varsplit_vcd_partial_c_2)
	{
	free_2(GLOBALS->varsplit_vcd_partial_c_2);
	GLOBALS->varsplit_vcd_partial_c_2=NULL;
	}

if((!GLOBALS->sorted_vcd_partial_c_2)&&(!GLOBALS->indexed_vcd_partial_c_2))
	{
	fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
	exit(1);
	}

add_tail_histents();
vcd_build_symbols();
vcd_sortfacs();
vcd_cleanup();

GLOBALS->min_time=GLOBALS->start_time_vcd_partial_c_2*GLOBALS->time_scale;
GLOBALS->max_time=GLOBALS->end_time_vcd_partial_c_2*GLOBALS->time_scale;
GLOBALS->global_time_offset = GLOBALS->global_time_offset * GLOBALS->time_scale;

if((GLOBALS->min_time==GLOBALS->max_time)||(GLOBALS->max_time==0))
        {
	GLOBALS->min_time = GLOBALS->max_time = 0;
        }

GLOBALS->is_vcd=~0;
GLOBALS->partial_vcd = ~0;

#ifdef __linux__
	{
	struct shmid_ds ds;
	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
	}
#endif

return(GLOBALS->max_time);
}

/*******************************************************************************/

static void regen_harray(Trptr t, nptr nd)
{
int i, histcount;
hptr histpnt;
hptr *harray;

if(!nd->harray)         /* make quick array lookup for aet display */
        {
        histpnt=&(nd->head);
        histcount=0;

        while(histpnt)
                {
                histcount++;
                histpnt=histpnt->next;
                }

        nd->numhist=histcount;

        if(!(nd->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
                {
                fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
                        nd->nname );
                free_2(t);
		return; /* scan-build : really can't do anything here */
                }

        histpnt=&(nd->head);
        for(i=0;i<histcount;i++)
                {
                *harray=histpnt;

                /* printf("%s, time: %d, val: %d\n", nd->nname,
                        (*harray)->time, (*harray)->val); */

                harray++;
                histpnt=histpnt->next;
                }
        }
}

/* mark vectors that need to be regenerated */
static void regen_trace_mark(Trptr t, int mandclear)
{
if(t->vector)
	{
	bvptr b = t->n.vec;
	bptr bts = b->bits;
	int i;

	if(1)
		{
		for(i=0;i<bts->nnbits;i++)
			{
			if(!bts->nodes[i]->expansion)
				{
				if(bts->nodes[i]->harray)
					{
					free_2(bts->nodes[i]->harray);
					bts->nodes[i]->harray = NULL;
					}
				}
				else
				{
				t->interactive_vector_needs_regeneration = 1;
				}
			}
		}

	for(i=0;i<bts->nnbits;i++)
		{
		if(!bts->nodes[i]->harray)
			{
			t->interactive_vector_needs_regeneration = 1;
			return;
			}
		}
	}
	else
	{
	if(t->n.nd) /* comment and blank traces don't have a valid node */
	if((t->n.nd->harray) && (mandclear))
		{
		free_2(t->n.nd->harray);
		t->n.nd->harray = NULL;
		}
	}
}

/* sweep through and regen nodes/dirty vectors */
static void regen_trace_sweep(Trptr t)
{
if(!t->vector)
	{
	if(t->n.nd) /* comment and blank traces don't have a valid node */
	if(!t->n.nd->harray)
		{
		regen_harray(t, t->n.nd);
		}
	}
else if(t->interactive_vector_needs_regeneration)
	{
	bvptr b = t->n.vec;
	bptr bts = b->bits;
	int i;
	bvptr b2;

	for(i=0;i<bts->nnbits;i++)
		{
		if(bts->nodes[i]->expansion)
			{
			nptr parent = bts->nodes[i]->expansion->parent;
			int parentbit = bts->nodes[i]->expansion->parentbit;

			DeleteNode(bts->nodes[i]);

			bts->nodes[i] = ExtractNodeSingleBit(parent, parentbit);
			}

		if(!bts->nodes[i]->harray)
			{
			regen_harray(t, bts->nodes[i]);
			}
		}

	if(!bts->name)
		{
		bts->name = "";
		b2 = bits2vector(bts);
		bts->name = NULL;
		}
		else
		{
		b2 = bits2vector(bts);
		}

	t->n.vec = b2;
	b2->bits=bts;

	free_2(b2->bvname);
	b2->bvname = b->bvname;

	for(i=0;i<b->numregions;i++)
		{
		free_2(b->vectors[i]);
		}

	free_2(b);
	}
}

/*******************************************************************************/

void kick_partial_vcd(void)
{
#if !defined _MSC_VER

if(GLOBALS->partial_vcd)
	{
#ifdef __MINGW32__
	Sleep(10);
#else
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 1000000 / 100;
	select(0, NULL, NULL, NULL, &tv);
#endif

	while(*GLOBALS->consume_ptr_vcd_partial_c_1)
		{
		int old_maxtime_marker_conflict = (GLOBALS->tims.marker > GLOBALS->max_time);

		vcd_parse();

		GLOBALS->min_time=GLOBALS->start_time_vcd_partial_c_2*GLOBALS->time_scale;
		GLOBALS->max_time=GLOBALS->end_time_vcd_partial_c_2*GLOBALS->time_scale;

		GLOBALS->tims.last=GLOBALS->max_time;
		GLOBALS->tims.end=GLOBALS->tims.last;             /* until the configure_event of wavearea */

		if(!GLOBALS->timeset_vcd_partial_c_1)
			{
			GLOBALS->tims.first=GLOBALS->tims.start=GLOBALS->tims.laststart=GLOBALS->min_time;
			GLOBALS->timeset_vcd_partial_c_1 = 1;
			}

		update_endcap_times_for_partial_vcd();
		update_maxmarker_labels();

		if(old_maxtime_marker_conflict)
			{
			old_maxtime_marker_conflict = (GLOBALS->tims.marker<=GLOBALS->max_time); /* data is now past what was invisible marker */
			}

		vcd_partial_mark_and_sweep(1);

		if ((GLOBALS->zoom_dyn) && (!GLOBALS->helpbox_is_active))
			{
			GLOBALS->tims.marker = GLOBALS->tims.last;
			service_zoom_full(NULL, NULL);

		        GLOBALS->signalwindow_width_dirty=1;
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
			}
		else if ((GLOBALS->zoom_dyne) && (!GLOBALS->helpbox_is_active))
			{
			GLOBALS->tims.marker = GLOBALS->tims.last;
			service_zoom_right(NULL, NULL);

		        GLOBALS->signalwindow_width_dirty=1;
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
			}
		else if ((old_maxtime_marker_conflict) && (!GLOBALS->helpbox_is_active))
			{
			GLOBALS->signalwindow_width_dirty=1;
			MaxSignalLength();
			signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
			}
			else
			{
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
			}

		update_maxmarker_labels();

		gtkwave_main_iteration();
		}
	}

#endif

gtkwave_main_iteration();
}


static void vcd_partial_regen_node_expansion(Trptr t)
{
if(!t->vector)
	{
	if(t->n.nd && t->n.nd->expansion)
		{
		nptr np_ex = ExtractNodeSingleBit(t->n.nd->expansion->parent, t->n.nd->expansion->parentbit);

		DeleteNode(t->n.nd);
		t->n.nd = np_ex;
		t->name_full = np_ex->nname;
		t->name = (GLOBALS->hier_max_level) ? hier_extract(t->name_full, GLOBALS->hier_max_level) : t->name_full;
		}
	}

}

void vcd_partial_mark_and_sweep(int mandclear)
{
Trptr t;



/* node */
t = GLOBALS->traces.first; while(t) { if(!t->vector) regen_trace_mark(t, mandclear); t = t->t_next; }
t = GLOBALS->traces.buffer; while(t) { if(!t->vector) regen_trace_mark(t, mandclear); t = t->t_next; }

t = GLOBALS->traces.first; while(t) { if(!t->vector) regen_trace_sweep(t); t = t->t_next; }
t = GLOBALS->traces.buffer; while(t) { if(!t->vector) regen_trace_sweep(t); t = t->t_next; }

/* node that is single bit extracted */
t = GLOBALS->traces.first; while(t) { vcd_partial_regen_node_expansion(t); t = t->t_next; }
t = GLOBALS->traces.buffer; while(t) { vcd_partial_regen_node_expansion(t); t = t->t_next; }

/* vector */
t = GLOBALS->traces.first; while(t) { if(t->vector) regen_trace_mark(t, mandclear); t = t->t_next; }
t = GLOBALS->traces.buffer; while(t) { if(t->vector) regen_trace_mark(t, mandclear); t = t->t_next; }

t = GLOBALS->traces.first; while(t) { if(t->vector) regen_trace_sweep(t); t = t->t_next; }
t = GLOBALS->traces.buffer; while(t) { if(t->vector) regen_trace_sweep(t); t = t->t_next; }

/* floating point */
t = GLOBALS->traces.first; while(t) { if(t->minmax_valid) t->minmax_valid = 0; t = t->t_next; }
t = GLOBALS->traces.buffer; while(t) { if(t->minmax_valid) t->minmax_valid = 0; t = t->t_next; }
}
