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
 * added real_parameter vartype 04aug06ajb
 * added in/out port vartype    31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 * Backtracking fix             16oct18ajb
 */

/* AIX may need this for alloca to work */
#if defined _AIX
  #pragma alloca
#endif

#include <config.h>
#include "globals.h"
#include "vcd.h"
#include "hierpack.h"

#undef VCD_BSEARCH_IS_PERFECT		/* bsearch is imperfect under linux, but OK under AIX */

static void add_histent(TimeType time, struct Node *n, char ch, int regadd, char *vector);
static void add_tail_histents(void);
static void vcd_build_symbols(void);
static void vcd_cleanup(void);
static void evcd_strcpy(char *dst, char *src);

/******************************************************************/

static void malform_eof_fix(void)
{
if(feof(GLOBALS->vcd_handle_vcd_c_1))
        {
        memset(GLOBALS->vcdbuf_vcd_c_1, ' ', VCD_BSIZ);
        GLOBALS->vst_vcd_c_1=GLOBALS->vend_vcd_c_1;
        }
}

/**/

void strcpy_vcdalt(char *too, char *from, char delim)
{
char ch;

do
	{
	ch=*(from++);
	if(ch==delim)
		{
		ch=GLOBALS->hier_delimeter;
		}
	} while((*(too++)=ch));
}


int strcpy_delimfix(char *too, char *from)
{
char ch;
int found = 0;

do
	{
	ch=*(from++);
	if(ch==GLOBALS->hier_delimeter)
		{
		ch=VCDNAM_ESCAPE;
		found = 1;
		}
	} while((*(too++)=ch));

if(found) GLOBALS->escaped_names_found_vcd_c_1 = found;

return(found);
}

/******************************************************************/


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



/******************************************************************/

/*
 * histent structs are NEVER freed so this is OK..
 * (we are allocating as many entries that fit in 64k minus the size of the two
 * bookkeeping void* pointers found in the malloc_2/free_2 routines in
 * debug.c--unless Judy, then can dispense with pointer subtraction)
 */
#ifdef _WAVE_HAVE_JUDY
#define VCD_HISTENT_GRANULARITY ( (64*1024) / sizeof(HistEnt) )
#else
#define VCD_HISTENT_GRANULARITY ( ( (64*1024)-(2*sizeof(void *)) ) / sizeof(HistEnt) )
#endif

struct HistEnt *histent_calloc(void)
{
if(GLOBALS->he_curr_vcd_c_1==GLOBALS->he_fini_vcd_c_1)
	{
	GLOBALS->he_curr_vcd_c_1=(struct HistEnt *)calloc_2(VCD_HISTENT_GRANULARITY, sizeof(struct HistEnt));
	GLOBALS->he_fini_vcd_c_1=GLOBALS->he_curr_vcd_c_1+VCD_HISTENT_GRANULARITY;
	}

return(GLOBALS->he_curr_vcd_c_1++);
}

/******************************************************************/


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

if(GLOBALS->indexed_vcd_c_1)
        {
        unsigned int hsh = vcdid_hash(key, len);
        if((hsh>=GLOBALS->vcd_minid_vcd_c_1)&&(hsh<=GLOBALS->vcd_maxid_vcd_c_1))
                {
                return(GLOBALS->indexed_vcd_c_1[hsh-GLOBALS->vcd_minid_vcd_c_1]);
                }

	return(NULL);
        }

if(GLOBALS->sorted_vcd_c_1)
	{
	v=(struct vcdsymbol **)bsearch(key, GLOBALS->sorted_vcd_c_1, GLOBALS->numsyms_vcd_c_1,
		sizeof(struct vcdsymbol *), vcdsymbsearchcompare);

	if(v)
		{
		#ifndef VCD_BSEARCH_IS_PERFECT
			for(;;)
				{
				t=*v;

				if((v==GLOBALS->sorted_vcd_c_1)||(strcmp((*(--v))->id, key)))
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
	if(!GLOBALS->err_vcd_c_1)
		{
		fprintf(stderr, "Near byte %d, VCD search table NULL..is this a VCD file?\n", (int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)));
		GLOBALS->err_vcd_c_1=1;
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

if(GLOBALS->sorted_vcd_c_1)
	{
	free_2(GLOBALS->sorted_vcd_c_1);	/* this means we saw a 2nd enddefinition chunk! */
	GLOBALS->sorted_vcd_c_1=NULL;
	}

if(GLOBALS->indexed_vcd_c_1)
	{
	free_2(GLOBALS->indexed_vcd_c_1);
	GLOBALS->indexed_vcd_c_1=NULL;
	}

if(GLOBALS->numsyms_vcd_c_1)
	{
        vcd_distance = GLOBALS->vcd_maxid_vcd_c_1 - GLOBALS->vcd_minid_vcd_c_1 + 1;

        if((vcd_distance <= VCD_INDEXSIZ)||(!GLOBALS->vcd_hash_kill))
                {
                GLOBALS->indexed_vcd_c_1 = (struct vcdsymbol **)calloc_2(vcd_distance, sizeof(struct vcdsymbol *));

		/* printf("%d symbols span ID range of %d, using indexing...\n", GLOBALS->numsyms_vcd_c_1, vcd_distance); */

                v=GLOBALS->vcdsymroot_vcd_c_1;
                while(v)
                        {
                        if(!GLOBALS->indexed_vcd_c_1[v->nid - GLOBALS->vcd_minid_vcd_c_1]) GLOBALS->indexed_vcd_c_1[v->nid - GLOBALS->vcd_minid_vcd_c_1] = v;
                        v=v->next;
                        }
                }
                else
		{
		pnt=GLOBALS->sorted_vcd_c_1=(struct vcdsymbol **)calloc_2(GLOBALS->numsyms_vcd_c_1, sizeof(struct vcdsymbol *));
		v=GLOBALS->vcdsymroot_vcd_c_1;
		while(v)
			{
			*(pnt++)=v;
			v=v->next;
			}

		qsort(GLOBALS->sorted_vcd_c_1, GLOBALS->numsyms_vcd_c_1, sizeof(struct vcdsymbol *), vcdsymcompare);
		}
	}
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(void)
{
GLOBALS->vend_vcd_c_1=GLOBALS->vst_vcd_c_1=GLOBALS->vcdbuf_vcd_c_1=(char *)calloc_2(1,VCD_BSIZ);
}

static void getch_free(void)
{
free_2(GLOBALS->vcdbuf_vcd_c_1);
GLOBALS->vcdbuf_vcd_c_1=GLOBALS->vst_vcd_c_1=GLOBALS->vend_vcd_c_1=NULL;
}



static int getch_fetch(void)
{
size_t rd;

errno = 0;
if(feof(GLOBALS->vcd_handle_vcd_c_1)) return(-1);

GLOBALS->vcdbyteno_vcd_c_1+=(GLOBALS->vend_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1);
rd=fread(GLOBALS->vcdbuf_vcd_c_1, sizeof(char), VCD_BSIZ, GLOBALS->vcd_handle_vcd_c_1);
GLOBALS->vend_vcd_c_1=(GLOBALS->vst_vcd_c_1=GLOBALS->vcdbuf_vcd_c_1)+rd;

if((!rd)||(errno)) return(-1);

if(GLOBALS->vcd_fsiz_vcd_c_1)
	{
	splash_sync(GLOBALS->vcdbyteno_vcd_c_1, GLOBALS->vcd_fsiz_vcd_c_1); /* gnome 2.18 seems to set errno so splash moved here... */
	}

return((int)(*GLOBALS->vst_vcd_c_1));
}


static inline signed char getch(void) {
  signed char ch = ((GLOBALS->vst_vcd_c_1!=GLOBALS->vend_vcd_c_1)?((int)(*GLOBALS->vst_vcd_c_1)):(getch_fetch()));
  GLOBALS->vst_vcd_c_1++;
  return(ch);
}

static inline signed char getch_peek(void) {
  signed char ch = ((GLOBALS->vst_vcd_c_1!=GLOBALS->vend_vcd_c_1)?((int)(*GLOBALS->vst_vcd_c_1)):(getch_fetch()));
  /* no increment */
  return(ch);
}



static int getch_patched(void)
{
char ch;

ch=*GLOBALS->vsplitcurr_vcd_c_1;
if(!ch)
	{
	return(-1);
	}
	else
	{
	GLOBALS->vsplitcurr_vcd_c_1++;
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
	GLOBALS->yytext_vcd_c_1[len++]=ch;
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

for(GLOBALS->yytext_vcd_c_1[len++]=ch;;GLOBALS->yytext_vcd_c_1[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_c_1)
		{
		GLOBALS->yytext_vcd_c_1=(char *)realloc_2(GLOBALS->yytext_vcd_c_1, (GLOBALS->T_MAX_STR_vcd_c_1=GLOBALS->T_MAX_STR_vcd_c_1*2)+1);
		}
	ch=getch();
	if(ch<=' ') break;
	}
GLOBALS->yytext_vcd_c_1[len]=0;	/* terminator */

if(is_string)
	{
	GLOBALS->yylen_vcd_c_1=len;
	return(T_STRING);
	}

yyshadow=GLOBALS->yytext_vcd_c_1;
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

if(!GLOBALS->var_prevch_vcd_c_1)
	{
	for(;;)
		{
		ch=getch_patched();
		if(ch<0) { free_2(GLOBALS->varsplit_vcd_c_1); GLOBALS->varsplit_vcd_c_1=NULL; return(V_END); }
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=GLOBALS->var_prevch_vcd_c_1;
	GLOBALS->var_prevch_vcd_c_1=0;
	}

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(GLOBALS->yytext_vcd_c_1[len++]=ch;;GLOBALS->yytext_vcd_c_1[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_c_1)
		{
		GLOBALS->yytext_vcd_c_1=(char *)realloc_2(GLOBALS->yytext_vcd_c_1, (GLOBALS->T_MAX_STR_vcd_c_1=GLOBALS->T_MAX_STR_vcd_c_1*2)+1);
		}
	ch=getch_patched();
	if(ch<0) { free_2(GLOBALS->varsplit_vcd_c_1); GLOBALS->varsplit_vcd_c_1=NULL; break; }
	if((ch==':')||(ch==']'))
		{
		GLOBALS->var_prevch_vcd_c_1=ch;
		break;
		}
	}
GLOBALS->yytext_vcd_c_1[len]=0;	/* terminator */

if(match_kw)
	{
	int vt = vcd_keyword_code(GLOBALS->yytext_vcd_c_1, len);
	if(vt != V_STRING)
		{
		if(ch<0) { free_2(GLOBALS->varsplit_vcd_c_1); GLOBALS->varsplit_vcd_c_1=NULL; }
		return(vt);
		}
	}

GLOBALS->yylen_vcd_c_1=len;
if(ch<0) { free_2(GLOBALS->varsplit_vcd_c_1); GLOBALS->varsplit_vcd_c_1=NULL; }
return(V_STRING);
}

static int get_vartoken(int match_kw)
{
int ch;
int len=0;

if(GLOBALS->varsplit_vcd_c_1)
	{
	int rc=get_vartoken_patched(match_kw);
	if(rc!=V_END) return(rc);
	GLOBALS->var_prevch_vcd_c_1=0;
	}

if(!GLOBALS->var_prevch_vcd_c_1)
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
	ch=GLOBALS->var_prevch_vcd_c_1;
	GLOBALS->var_prevch_vcd_c_1=0;
	}

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

if(ch=='#')     /* for MTI System Verilog '$var reg 64 >w #implicit-var###VarElem:ram_di[0.0] [63:0] $end' style declarations */
        {       /* debussy simply escapes until the space */
        GLOBALS->yytext_vcd_c_1[len++]= '\\';
        }

for(GLOBALS->yytext_vcd_c_1[len++]=ch;;GLOBALS->yytext_vcd_c_1[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_c_1)
		{
		GLOBALS->yytext_vcd_c_1=(char *)realloc_2(GLOBALS->yytext_vcd_c_1, (GLOBALS->T_MAX_STR_vcd_c_1=GLOBALS->T_MAX_STR_vcd_c_1*2)+1);
		}

        ch=getch();
        if(ch==' ')
                {
                if(match_kw) break;
                if(getch_peek() == '[')
                        {
                        ch = getch();
                        GLOBALS->varsplit_vcd_c_1=GLOBALS->yytext_vcd_c_1+len;  /* keep looping so we get the *last* one */
                        continue;
                        }
                }

	if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
	if((ch=='[')&&(GLOBALS->yytext_vcd_c_1[0]!='\\'))
		{
		GLOBALS->varsplit_vcd_c_1=GLOBALS->yytext_vcd_c_1+len;		/* keep looping so we get the *last* one */
		}
	else
	if(((ch==':')||(ch==']'))&&(!GLOBALS->varsplit_vcd_c_1)&&(GLOBALS->yytext_vcd_c_1[0]!='\\'))
		{
		GLOBALS->var_prevch_vcd_c_1=ch;
		break;
		}
	}
GLOBALS->yytext_vcd_c_1[len]=0;	/* absolute terminator */
if((GLOBALS->varsplit_vcd_c_1)&&(GLOBALS->yytext_vcd_c_1[len-1]==']'))
	{
	char *vst;
	vst=malloc_2(strlen(GLOBALS->varsplit_vcd_c_1)+1);
	strcpy(vst, GLOBALS->varsplit_vcd_c_1);

	*GLOBALS->varsplit_vcd_c_1=0x00;		/* zero out var name at the left bracket */
	len=GLOBALS->varsplit_vcd_c_1-GLOBALS->yytext_vcd_c_1;

	GLOBALS->varsplit_vcd_c_1=GLOBALS->vsplitcurr_vcd_c_1=vst;
	GLOBALS->var_prevch_vcd_c_1=0;
	}
	else
	{
	GLOBALS->varsplit_vcd_c_1=NULL;
	}

if(match_kw)
	{
	int vt = vcd_keyword_code(GLOBALS->yytext_vcd_c_1, len);
	if(vt != V_STRING)
		{
		return(vt);
		}
	}

GLOBALS->yylen_vcd_c_1=len;
return(V_STRING);
}

static int get_strtoken(void)
{
int ch;
int len=0;

if(!GLOBALS->var_prevch_vcd_c_1)
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
      ch=GLOBALS->var_prevch_vcd_c_1;
      GLOBALS->var_prevch_vcd_c_1=0;
      }

for(GLOBALS->yytext_vcd_c_1[len++]=ch;;GLOBALS->yytext_vcd_c_1[len++]=ch)
      {
	if(len==GLOBALS->T_MAX_STR_vcd_c_1)
		{
		GLOBALS->yytext_vcd_c_1=(char *)realloc_2(GLOBALS->yytext_vcd_c_1, (GLOBALS->T_MAX_STR_vcd_c_1=GLOBALS->T_MAX_STR_vcd_c_1*2)+1);
		}
      ch=getch();
      if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
      }
GLOBALS->yytext_vcd_c_1[len]=0;        /* terminator */

GLOBALS->yylen_vcd_c_1=len;
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
	if(hdr) { DEBUG(fprintf(stderr," %s",GLOBALS->yytext_vcd_c_1)); }
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
	if(hdr) { DEBUG(fprintf(stderr," %s",GLOBALS->yytext_vcd_c_1)); }
        if(strstr(GLOBALS->yytext_vcd_c_1, "Icarus"))   /* turn off autocoalesce for Icarus */
                {
                GLOBALS->autocoalesce = 0;
		rc = 1;
                }
	}
if(hdr) { DEBUG(fprintf(stderr,"\n")); }
return(rc);
}

char *build_slisthier(void)
{
struct slist *s;
int len=0;

if(GLOBALS->slisthier)
	{
        free_2(GLOBALS->slisthier);
        }

if(!GLOBALS->slistroot)
	{
	GLOBALS->slisthier_len=0;
	GLOBALS->slisthier=(char *)malloc_2(1);
	*GLOBALS->slisthier=0;
	return(GLOBALS->slisthier);
	}

s=GLOBALS->slistroot; len=0;
while(s)
	{
	len+=s->len+(s->next?1:0);
	s=s->next;
	}

GLOBALS->slisthier=(char *)malloc_2((GLOBALS->slisthier_len=len)+1);
s=GLOBALS->slistroot; len=0;
while(s)
	{
	strcpy(GLOBALS->slisthier+len,s->str);
	len+=s->len;
	if(s->next)
		{
		strcpy(GLOBALS->slisthier+len,GLOBALS->vcd_hier_delimeter);
		len++;
		}
	s=s->next;
	}

return(GLOBALS->slisthier);
}


void append_vcd_slisthier(char *str)
{
struct slist *s;
s=(struct slist *)calloc_2(1,sizeof(struct slist));
s->len=strlen(str);
s->str=(char *)malloc_2(s->len+1);
strcpy(s->str,str);

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


static void parse_valuechange(void)
{
struct vcdsymbol *v;
char *vector;
int vlen;

switch(GLOBALS->yytext_vcd_c_1[0])
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
		if(GLOBALS->yylen_vcd_c_1>1)
			{
			v=bsearch_vcd(GLOBALS->yytext_vcd_c_1+1, GLOBALS->yylen_vcd_c_1-1);
			if(!v)
				{
				fprintf(stderr,"Near byte %d, Unknown VCD identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)),GLOBALS->yytext_vcd_c_1+1);
				malform_eof_fix();
				}
				else
				{
				v->value[0]=GLOBALS->yytext_vcd_c_1[0];
				DEBUG(fprintf(stderr,"%s = '%c'\n",v->name,v->value[0]));
				add_histent(GLOBALS->current_time_vcd_c_1,v->narray[0],v->value[0],1, NULL);
				}
			}
			else
			{
			fprintf(stderr,"Near byte %d, Malformed VCD identifier\n", (int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)));
			malform_eof_fix();
			}
		break;

	case 'b':
	case 'B':
		{
		/* extract binary number then.. */
		vector=malloc_2(GLOBALS->yylen_cache_vcd_c_1=GLOBALS->yylen_vcd_c_1);
		strcpy(vector,GLOBALS->yytext_vcd_c_1+1);
		vlen=GLOBALS->yylen_vcd_c_1-1;

		get_strtoken();
		v=bsearch_vcd(GLOBALS->yytext_vcd_c_1, GLOBALS->yylen_vcd_c_1);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), GLOBALS->yytext_vcd_c_1);
			free_2(vector);
			malform_eof_fix();
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
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), GLOBALS->yytext_vcd_c_1);
					free_2(d);
					malform_eof_fix();
					}
					else
					{
					add_histent(GLOBALS->current_time_vcd_c_1, v->narray[0],'g',1,(char *)d);
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
					add_histent(GLOBALS->current_time_vcd_c_1, v->narray[i],v->value[i],1, NULL);
					}
				free_2(vector);
				}
				else
				{
				if(GLOBALS->yylen_cache_vcd_c_1!=(v->size+1))
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				add_histent(GLOBALS->current_time_vcd_c_1, v->narray[0],0,1,vector);
				}

			}
		break;
		}

	case 'p':
		/* extract port dump value.. */
		vector=malloc_2(GLOBALS->yylen_cache_vcd_c_1=GLOBALS->yylen_vcd_c_1);
		strcpy(vector,GLOBALS->yytext_vcd_c_1+1);
		vlen=GLOBALS->yylen_vcd_c_1-1;

		get_strtoken();	/* throw away 0_strength_component */
		get_strtoken(); /* throw away 0_strength_component */
		get_strtoken(); /* this is the id                  */
		v=bsearch_vcd(GLOBALS->yytext_vcd_c_1, GLOBALS->yylen_vcd_c_1);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), GLOBALS->yytext_vcd_c_1);
			free_2(vector);
			malform_eof_fix();
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
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), GLOBALS->yytext_vcd_c_1);
					free_2(d);
					malform_eof_fix();
					}
					else
					{
					add_histent(GLOBALS->current_time_vcd_c_1, v->narray[0],'g',1,(char *)d);
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
					add_histent(GLOBALS->current_time_vcd_c_1, v->narray[i],v->value[i],1, NULL);
					}
				free_2(vector);
				}
				else
				{
				if(GLOBALS->yylen_cache_vcd_c_1<v->size)
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				add_histent(GLOBALS->current_time_vcd_c_1, v->narray[0],0,1,vector);
				}
			}
		break;


	case 'r':
	case 'R':
		{
		double *d;

		d=malloc_2(sizeof(double));
		sscanf(GLOBALS->yytext_vcd_c_1+1,"%lg",d);

		get_strtoken();
		v=bsearch_vcd(GLOBALS->yytext_vcd_c_1, GLOBALS->yylen_vcd_c_1);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), GLOBALS->yytext_vcd_c_1);
			free_2(d);
			malform_eof_fix();
			}
			else
			{
			add_histent(GLOBALS->current_time_vcd_c_1, v->narray[0],'g',1,(char *)d);
			}

		break;
		}

#ifndef STRICT_VCD_ONLY
	case 's':
	case 'S':
		{
		char *d;

		d=(char *)malloc_2(GLOBALS->yylen_vcd_c_1);
		vlen = fstUtilityEscToBin((unsigned char *)d, (unsigned char *)(GLOBALS->yytext_vcd_c_1+1), GLOBALS->yylen_vcd_c_1); /* includes 0 term */
		if(vlen != GLOBALS->yylen_vcd_c_1)
			{
			d = realloc_2(d, vlen);
			}

		get_strtoken();
		v=bsearch_vcd(GLOBALS->yytext_vcd_c_1, GLOBALS->yylen_vcd_c_1);
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), GLOBALS->yytext_vcd_c_1);
			free_2(d);
			malform_eof_fix();
			}
			else
			{
			add_histent(GLOBALS->current_time_vcd_c_1, v->narray[0],'s',1,(char *)d);
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
                        GLOBALS->global_time_offset=atoi_64(GLOBALS->yytext_vcd_c_1);

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
			fractional_timescale_fix(GLOBALS->yytext_vcd_c_1);
			GLOBALS->time_scale=atoi_64(GLOBALS->yytext_vcd_c_1);
			if(!GLOBALS->time_scale) GLOBALS->time_scale=1;
			for(i=0;i<GLOBALS->yylen_vcd_c_1;i++)
				{
				if((GLOBALS->yytext_vcd_c_1[i]<'0')||(GLOBALS->yytext_vcd_c_1[i]>'9'))
					{
					prefix=GLOBALS->yytext_vcd_c_1[i];
					break;
					}
				}
			if(prefix==' ')
				{
				vtok=get_token();
				if((vtok==T_END)||(vtok==T_EOF)) break;
				prefix=GLOBALS->yytext_vcd_c_1[0];
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
                                switch(GLOBALS->yytext_vcd_c_1[0])
                                        {
                                        case 'm':       ttype = TREE_VCD_ST_MODULE; break;
                                        case 't':       ttype = TREE_VCD_ST_TASK; break;
                                        case 'f':       ttype = (GLOBALS->yytext_vcd_c_1[1] == 'u') ? TREE_VCD_ST_FUNCTION : TREE_VCD_ST_FORK; break;
                                        case 'b':       ttype = TREE_VCD_ST_BEGIN; break;
					case 'g':	ttype = TREE_VCD_ST_GENERATE; break;
					case 's':	ttype = TREE_VCD_ST_STRUCT; break;
					case 'u':	ttype = TREE_VCD_ST_UNION; break;
					case 'c':	ttype = TREE_VCD_ST_CLASS; break;
					case 'i':	ttype = TREE_VCD_ST_INTERFACE; break;
					case 'p':	ttype = (GLOBALS->yytext_vcd_c_1[1] == 'r') ? TREE_VCD_ST_PROGRAM : TREE_VCD_ST_PACKAGE; break;

                                        case 'v':       {
                                                        char *vht = GLOBALS->yytext_vcd_c_1;
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
				s->len=GLOBALS->yylen_vcd_c_1;
				s->str=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+1);
				strcpy(s->str, GLOBALS->yytext_vcd_c_1);
				s->mod_tree_parent = GLOBALS->mod_tree_parent;

				allocate_and_decorate_module_tree_node(ttype, GLOBALS->yytext_vcd_c_1, NULL, GLOBALS->yylen_vcd_c_1, 0, 0, 0);

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
			if((GLOBALS->header_over_vcd_c_1)&&(0))
			{
			fprintf(stderr,"$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, exiting.\n",
				(int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)));
			vcd_exit(255);
			}
			else
			{
			int vtok;
			struct vcdsymbol *v=NULL;

			GLOBALS->var_prevch_vcd_c_1=0;

			if(GLOBALS->varsplit_vcd_c_1)
				{
				free_2(GLOBALS->varsplit_vcd_c_1);
				GLOBALS->varsplit_vcd_c_1=NULL;
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
					v->size=atoi_64(GLOBALS->yytext_vcd_c_1);
					if(!v->size) v->size=1;
					}
					else
					if(vtok==V_LB)
					{
					vtok=get_vartoken(1);
					if(vtok==V_END) goto err;
					if(vtok!=V_STRING) goto err;
					v->msi=atoi_64(GLOBALS->yytext_vcd_c_1);
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
						v->lsi=atoi_64(GLOBALS->yytext_vcd_c_1);
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
				v->id=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+1);
				strcpy(v->id, GLOBALS->yytext_vcd_c_1);
                                v->nid=vcdid_hash(GLOBALS->yytext_vcd_c_1,GLOBALS->yylen_vcd_c_1);

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

                                if(v->nid < GLOBALS->vcd_minid_vcd_c_1) GLOBALS->vcd_minid_vcd_c_1 = v->nid;
                                if(v->nid > GLOBALS->vcd_maxid_vcd_c_1) GLOBALS->vcd_maxid_vcd_c_1 = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(GLOBALS->slisthier_len)
					{
					v->name=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_c_1+1);
					strcpy(v->name,GLOBALS->slisthier);
					strcpy(v->name+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_c_1,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_c_1)) && (GLOBALS->yytext_vcd_c_1[0] != '\\'))
							{
							char *sd=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_c_1+2);
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
					v->name=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+1);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,GLOBALS->yytext_vcd_c_1,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name,GLOBALS->yytext_vcd_c_1)) && (GLOBALS->yytext_vcd_c_1[0] != '\\'))
							{
							char *sd=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+2);
							sd[0] = '\\';
							strcpy(sd+1,v->name);
							free_2(v->name);
							v->name = sd;
							}
						}
					}

                                if(GLOBALS->pv_vcd_c_1)
                                        {
                                        if(!strcmp(GLOBALS->pv_vcd_c_1->name,v->name) && !disable_autocoalesce && (!strchr(v->name, '\\')))
                                                {
                                                GLOBALS->pv_vcd_c_1->chain=v;
                                                v->root=GLOBALS->rootv_vcd_c_1;
                                                if(GLOBALS->pv_vcd_c_1==GLOBALS->rootv_vcd_c_1) GLOBALS->pv_vcd_c_1->root=GLOBALS->rootv_vcd_c_1;
                                                }
                                                else
                                                {
                                                GLOBALS->rootv_vcd_c_1=v;
                                                }
                                        }
					else
					{
					GLOBALS->rootv_vcd_c_1=v;
					}
                                GLOBALS->pv_vcd_c_1=v;
				}
				else	/* regular vcd var, not an evcd port var */
				{
				vtok=get_vartoken(1);
				if(vtok==V_END) goto err;
				v->size=atoi_64(GLOBALS->yytext_vcd_c_1);
				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+1);
				strcpy(v->id, GLOBALS->yytext_vcd_c_1);
                                v->nid=vcdid_hash(GLOBALS->yytext_vcd_c_1,GLOBALS->yylen_vcd_c_1);

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

                                if(v->nid < GLOBALS->vcd_minid_vcd_c_1) GLOBALS->vcd_minid_vcd_c_1 = v->nid;
                                if(v->nid > GLOBALS->vcd_maxid_vcd_c_1) GLOBALS->vcd_maxid_vcd_c_1 = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(GLOBALS->slisthier_len)
					{
					v->name=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_c_1+1);
					strcpy(v->name,GLOBALS->slisthier);
					strcpy(v->name+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_c_1,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_c_1)) && (GLOBALS->yytext_vcd_c_1[0] != '\\'))
							{
                                                        char *sd=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_c_1+2);
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
					v->name=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+1);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,GLOBALS->yytext_vcd_c_1,GLOBALS->alt_hier_delimeter);
						}
						else
						{
                                                if((strcpy_delimfix(v->name,GLOBALS->yytext_vcd_c_1)) && (GLOBALS->yytext_vcd_c_1[0] != '\\'))
                                                        {
                                                        char *sd=(char *)malloc_2(GLOBALS->yylen_vcd_c_1+2);
                                                        sd[0] = '\\';
                                                        strcpy(sd+1,v->name);
                                                        free_2(v->name);
                                                        v->name = sd;
                                                        }
						}
					}

                                if(GLOBALS->pv_vcd_c_1)
                                        {
                                        if(!strcmp(GLOBALS->pv_vcd_c_1->name,v->name))
                                                {
                                                GLOBALS->pv_vcd_c_1->chain=v;
                                                v->root=GLOBALS->rootv_vcd_c_1;
                                                if(GLOBALS->pv_vcd_c_1==GLOBALS->rootv_vcd_c_1) GLOBALS->pv_vcd_c_1->root=GLOBALS->rootv_vcd_c_1;
                                                }
                                                else
                                                {
                                                GLOBALS->rootv_vcd_c_1=v;
                                                }
                                        }
					else
					{
					GLOBALS->rootv_vcd_c_1=v;
					}
                                GLOBALS->pv_vcd_c_1=v;

				vtok=get_vartoken(1);
				if(vtok==V_END) goto dumpv;
				if(vtok!=V_LB) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->msi=atoi_64(GLOBALS->yytext_vcd_c_1);
				vtok=get_vartoken(0);
				if(vtok==V_RB)
					{
					v->lsi=v->msi;
					goto dumpv;
					}
				if(vtok!=V_COLON) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->lsi=atoi_64(GLOBALS->yytext_vcd_c_1);
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
				if(v->vartype!=V_STRINGTYPE)
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
					set_vcd_vartype(v, v->narray[0]);
					}
					else
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';

						v->narray[i]=(struct Node *)calloc_2(1,sizeof(struct Node));
						v->narray[i]->head.time=-1;
						v->narray[i]->head.v.h_val=AN_X;
						if(i == 0)
							{
							set_vcd_vartype(v, v->narray[0]);
							}
							else
							{
							v->narray[i]->vartype = v->narray[0]->vartype;
							}
						}
					}
				}

			if(!GLOBALS->vcdsymroot_vcd_c_1)
				{
				GLOBALS->vcdsymroot_vcd_c_1=GLOBALS->vcdsymcurr_vcd_c_1=v;
				}
				else
				{
				GLOBALS->vcdsymcurr_vcd_c_1->next=v;
				GLOBALS->vcdsymcurr_vcd_c_1=v;
				}
			GLOBALS->numsyms_vcd_c_1++;

			if(GLOBALS->vcd_save_handle)
				{
				if(v->msi==v->lsi)
					{
					if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE))
						{
						fprintf(GLOBALS->vcd_save_handle,"%s\n",v->name);
						}
						else
						{
						if(v->msi>=0)
							{
							if(!GLOBALS->vcd_explicit_zero_subscripts)
								fprintf(GLOBALS->vcd_save_handle,"%s%c%d\n",v->name,GLOBALS->hier_delimeter,v->msi);
								else
								fprintf(GLOBALS->vcd_save_handle,"%s[%d]\n",v->name,v->msi);
							}
							else
							{
							fprintf(GLOBALS->vcd_save_handle,"%s\n",v->name);
							}
						}
					}
					else
					{
					int i;

					if(!GLOBALS->atomic_vectors)
						{
						fprintf(GLOBALS->vcd_save_handle,"#%s[%d:%d]",v->name,v->msi,v->lsi);
						if(v->msi>v->lsi)
							{
							for(i=v->msi;i>=v->lsi;i--)
								{
								if(!GLOBALS->vcd_explicit_zero_subscripts)
									fprintf(GLOBALS->vcd_save_handle," %s%c%d",v->name,GLOBALS->hier_delimeter,i);
									else
									fprintf(GLOBALS->vcd_save_handle," %s[%d]",v->name,i);
								}
							}
							else
							{
							for(i=v->msi;i<=v->lsi;i++)
								{
								if(!GLOBALS->vcd_explicit_zero_subscripts)
									fprintf(GLOBALS->vcd_save_handle," %s%c%d",v->name,GLOBALS->hier_delimeter,i);
									else
									fprintf(GLOBALS->vcd_save_handle," %s[%d]",v->name,i);
								}
							}
						fprintf(GLOBALS->vcd_save_handle,"\n");
						}
						else
						{
						fprintf(GLOBALS->vcd_save_handle,"%s[%d:%d]\n",v->name,v->msi,v->lsi);
						}
					}
				}

			goto bail;
			err:
			if(v)
				{
				GLOBALS->error_count_vcd_c_1++;
				if(v->name)
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered with '%s'\n", (int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)), v->name);
					free_2(v->name);
					}
					else
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered\n", (int)(GLOBALS->vcdbyteno_vcd_c_1+(GLOBALS->vst_vcd_c_1-GLOBALS->vcdbuf_vcd_c_1)));
					}
				if(v->id) free_2(v->id);
				if(v->value) free_2(v->value);
				free_2(v);
				GLOBALS->pv_vcd_c_1 = NULL;
				}

			bail:
			if(vtok!=V_END) sync_end(NULL);
			break;
			}
		case T_ENDDEFINITIONS:
			GLOBALS->header_over_vcd_c_1=1;	/* do symbol table management here */
			create_sorted_table();
			if((!GLOBALS->sorted_vcd_c_1)&&(!GLOBALS->indexed_vcd_c_1))
				{
				fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
				vcd_exit(255);
				}
			if(GLOBALS->error_count_vcd_c_1)
				{
				fprintf(stderr, "\n%d VCD parse errors encountered, exiting.\n", GLOBALS->error_count_vcd_c_1);
				vcd_exit(255);
				}
			break;
		case T_STRING:
			if(!GLOBALS->header_over_vcd_c_1)
				{
				GLOBALS->header_over_vcd_c_1=1;	/* do symbol table management here */
				create_sorted_table();
				if((!GLOBALS->sorted_vcd_c_1)&&(!GLOBALS->indexed_vcd_c_1)) break;
				}
				{
				/* catchall for events when header over */
				if(GLOBALS->yytext_vcd_c_1[0]=='#')
					{
					TimeType tim;
					tim=atoi_64(GLOBALS->yytext_vcd_c_1+1);

					if(GLOBALS->start_time_vcd_c_1<0)
						{
						GLOBALS->start_time_vcd_c_1=tim;
						}
/* backtracking fix */
						else
						{
						if(tim < GLOBALS->current_time_vcd_c_1)	/* avoid backtracking time counts which can happen on malformed files */
							{
                                                        if(!GLOBALS->vcd_already_backtracked)
                                                                {
                                                                GLOBALS->vcd_already_backtracked = 1;
                                                                fprintf(stderr, "VCDLOAD | Time backtracking detected in VCD file!\n");
                                                                }
#if 0
							tim = GLOBALS->current_time_vcd_c_1;
#endif
							}
						}

					GLOBALS->current_time_vcd_c_1=tim;
					if(GLOBALS->end_time_vcd_c_1<tim) GLOBALS->end_time_vcd_c_1=tim;	/* in case of malformed vcd files */
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
			GLOBALS->dumping_off_vcd_c_1=1;
			/* if((!GLOBALS->blackout_regions)||((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart<=GLOBALS->blackout_regions->bend))) : remove redundant condition */
			if((!GLOBALS->blackout_regions)||(GLOBALS->blackout_regions->bstart<=GLOBALS->blackout_regions->bend))
				{
				struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));

				bt->bstart = GLOBALS->current_time_vcd_c_1;
				bt->next = GLOBALS->blackout_regions;
				GLOBALS->blackout_regions = bt;
				}
			break;
		case T_DUMPON:
		case T_DUMPPORTSON:
			GLOBALS->dumping_off_vcd_c_1=0;
			if((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart>GLOBALS->blackout_regions->bend))
				{
				GLOBALS->blackout_regions->bend = GLOBALS->current_time_vcd_c_1;
				}
			break;
		case T_DUMPVARS:
		case T_DUMPPORTS:
			if(GLOBALS->current_time_vcd_c_1<0)
				{ GLOBALS->start_time_vcd_c_1=GLOBALS->current_time_vcd_c_1=GLOBALS->end_time_vcd_c_1=0; }
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
				GLOBALS->blackout_regions->bend = GLOBALS->current_time_vcd_c_1;
				}
			return;
		default:
			DEBUG(fprintf(stderr,"UNKNOWN TOKEN\n"));
		}
	}
}


/*******************************************************************************/

void add_histent(TimeType tim, struct Node *n, char ch, int regadd, char *vector)
{
struct HistEnt *he;
char heval;

if(!vector)
{
if(!n->curr)
	{
	he=histent_calloc();
        he->time=-1;
        he->v.h_val=AN_X;

	n->curr=he;
	n->head.next=he;

	add_histent(tim,n,ch,regadd, vector);
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

	if((n->curr->v.h_val!=heval)||(tim==GLOBALS->start_time_vcd_c_1)||(n->vartype==ND_VCD_EVENT)||(GLOBALS->vcd_preserve_glitches)) /* same region == go skip */
        	{
		if(n->curr->time>=tim) /* backtracking fix */
			{
			DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
				tim, n, AN_STR[n->curr->v.h_val], ch));
			n->curr->v.h_val=heval;		/* we have a glitch! */

			GLOBALS->num_glitches_vcd_c_2++;
			if(!(n->curr->flags&HIST_GLITCH))
				{
				n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
				GLOBALS->num_glitch_regions_vcd_c_2++;
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
	if(!n->curr)
		{
		he=histent_calloc();
		he->flags=(HIST_STRING|HIST_REAL);
	        he->time=-1;
	        he->v.h_vector=NULL;

		n->curr=he;
		n->head.next=he;

		add_histent(tim,n,ch,regadd, vector);
		}
		else
		{
		if(regadd) { tim*=(GLOBALS->time_scale); }

			if(n->curr->time>=tim) /* backtracking fix */
				{
				DEBUG(printf("Warning: String Glitch at time ["TTFormat"] Signal [%p].\n",
					tim, n));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */

				GLOBALS->num_glitches_vcd_c_2++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_c_2++;
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
	if(!n->curr)
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

		add_histent(tim,n,ch,regadd, vector);
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
			||(tim==GLOBALS->start_time_vcd_c_1)
#ifndef WAVE_HAS_H_DOUBLE
			||(!n->curr->v.h_vector)
#endif
			||(GLOBALS->vcd_preserve_glitches)||(GLOBALS->vcd_preserve_glitches_real)
			) /* same region == go skip */
	        	{
			if(n->curr->time>=tim) /* backtracking fix */
				{
				DEBUG(printf("Warning: Real number Glitch at time ["TTFormat"] Signal [%p].\n",
					tim, n));
#ifdef WAVE_HAS_H_DOUBLE
				n->curr->v.h_double = *((double *)vector);
#else
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */
#endif
				GLOBALS->num_glitches_vcd_c_2++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_c_2++;
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
	if(!n->curr)
		{
		he=histent_calloc();
	        he->time=-1;
	        he->v.h_vector=NULL;

		n->curr=he;
		n->head.next=he;

		add_histent(tim,n,ch,regadd, vector);
		}
		else
		{
		if(regadd) { tim*=(GLOBALS->time_scale); }

		if(
		  (n->curr->v.h_vector&&vector&&(strcmp(n->curr->v.h_vector,vector)))
			||(tim==GLOBALS->start_time_vcd_c_1)
			||(!n->curr->v.h_vector)
			||(GLOBALS->vcd_preserve_glitches)
			) /* same region == go skip */
	        	{
			if(n->curr->time>=tim) /* backtracking fix */
				{
				DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
					tim, n, AN_STR[n->curr->v.h_val], ch));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */

				GLOBALS->num_glitches_vcd_c_2++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_c_2++;
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

}


void set_vcd_vartype(struct vcdsymbol *v, nptr n)
{
unsigned char nvt;

switch(v->vartype)
	{
        case V_EVENT:           nvt = ND_VCD_EVENT; break;
        case V_PARAMETER:       nvt = ND_VCD_PARAMETER; break;
        case V_INTEGER:         nvt = ND_VCD_INTEGER; break;
        case V_REAL:            nvt = ND_VCD_REAL; break;
        case V_REG:             nvt = ND_VCD_REG; break;
        case V_SUPPLY0:         nvt = ND_VCD_SUPPLY0; break;
        case V_SUPPLY1:         nvt = ND_VCD_SUPPLY1; break;
        case V_TIME:            nvt = ND_VCD_TIME; break;
        case V_TRI:             nvt = ND_VCD_TRI; break;
        case V_TRIAND:          nvt = ND_VCD_TRIAND; break;
        case V_TRIOR:           nvt = ND_VCD_TRIOR; break;
        case V_TRIREG:          nvt = ND_VCD_TRIREG; break;
        case V_TRI0:            nvt = ND_VCD_TRI0; break;
        case V_TRI1:            nvt = ND_VCD_TRI1; break;
        case V_WAND:            nvt = ND_VCD_WAND; break;
        case V_WIRE:            nvt = ND_VCD_WIRE; break;
        case V_WOR:             nvt = ND_VCD_WOR; break;
        case V_PORT:            nvt = ND_VCD_PORT; break;
	case V_STRINGTYPE:	nvt = ND_GEN_STRING; break;
	case V_BIT:		nvt = ND_SV_BIT; break;
	case V_LOGIC:		nvt = ND_SV_LOGIC; break;
	case V_INT:		nvt = ND_SV_INT; break;
	case V_SHORTINT:	nvt = ND_SV_SHORTINT; break;
	case V_LONGINT:		nvt = ND_SV_LONGINT; break;
	case V_BYTE:		nvt = ND_SV_BYTE; break;
	case V_ENUM:		nvt = ND_SV_ENUM; break;
	/* V_SHORTREAL as a type does not exist for VCD: is cast to V_REAL */
        default:                nvt = ND_UNSPECIFIED_DEFAULT; break;
	}
n->vartype = nvt;
}


static void add_tail_histents(void)
{
int j;
struct vcdsymbol *v;

/* dump out any pending events 1st (removed) */
/* then do 'x' trailers */

v=GLOBALS->vcdsymroot_vcd_c_1;
while(v)
	{
	if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE))
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=1.0;
		add_histent(MAX_HISTENT_TIME-1, v->narray[0], 'g', 0, (char *)d);
		}
	else
	if((v->size==1)||(!GLOBALS->atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		add_histent(MAX_HISTENT_TIME-1, v->narray[j], 'x', 0, NULL);
		}
	else
		{
		add_histent(MAX_HISTENT_TIME-1, v->narray[0], 'x', 0, (char *)calloc_2(1,sizeof(char)));
		}

	v=v->next;
	}

v=GLOBALS->vcdsymroot_vcd_c_1;
while(v)
	{
	if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE))
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=0.0;
		add_histent(MAX_HISTENT_TIME, v->narray[0], 'g', 0, (char *)d);
		}
	else
	if((v->size==1)||(!GLOBALS->atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		add_histent(MAX_HISTENT_TIME, v->narray[j], 'z', 0, NULL);
		}
	else
		{
		add_histent(MAX_HISTENT_TIME, v->narray[0], 'z', 0, (char *)calloc_2(1,sizeof(char)));
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

v=GLOBALS->vcdsymroot_vcd_c_1;
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

void vcd_sortfacs(void)
{
int i;

GLOBALS->facs=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));
GLOBALS->curnode=GLOBALS->firstnode;
for(i=0;i<GLOBALS->numfacs;i++)
        {
        char *subst;
#ifdef WAVE_HIERFIX
        char ch;
#endif
        int len;
	struct symchain *sc;

        GLOBALS->facs[i]=GLOBALS->curnode->symbol;
	subst=GLOBALS->facs[i]->name;
        if((len=strlen(subst))>GLOBALS->longestname) GLOBALS->longestname=len;
	sc = GLOBALS->curnode;
        GLOBALS->curnode=GLOBALS->curnode->next;
	free_2(sc);
#ifdef WAVE_HIERFIX
        while((ch=(*subst)))
                {
                if(ch==GLOBALS->hier_delimeter) { *subst=VCDNAM_HIERSORT; } /* forces sort at hier boundaries */
                subst++;
                }
#endif
        }
GLOBALS->firstnode=GLOBALS->curnode=NULL;

/* quicksort(facs,0,numfacs-1); */	/* quicksort deprecated because it degenerates on sorted traces..badly.  very badly. */
wave_heapsort(GLOBALS->facs,GLOBALS->numfacs);

#ifdef WAVE_HIERFIX
for(i=0;i<GLOBALS->numfacs;i++)
        {
        char *subst, ch;

        subst=GLOBALS->facs[i]->name;
        while((ch=(*subst)))
                {
                if(ch==VCDNAM_HIERSORT) { *subst=GLOBALS->hier_delimeter; } /* restore back to normal */
                subst++;
                }

#ifdef DEBUG_FACILITIES
        printf("%-4d %s\n",i,facs[i]->name);
#endif
        }
#endif

GLOBALS->facs_are_sorted=1;

init_tree();
for(i=0;i<GLOBALS->numfacs;i++)
{
char *n = GLOBALS->facs[i]->name;
build_tree_from_name(n, i);

if(GLOBALS->escaped_names_found_vcd_c_1)
	{
        char *subst, ch;
        subst=GLOBALS->facs[i]->name;
        while((ch=(*subst)))
                {
                if(ch==VCDNAM_ESCAPE) { *subst=GLOBALS->hier_delimeter; } /* restore back to normal */
                subst++;
                }
	}
}
treegraft(&GLOBALS->treeroot);
treesort(GLOBALS->treeroot, NULL);

if(GLOBALS->escaped_names_found_vcd_c_1)
	{
	treenamefix(GLOBALS->treeroot);
	}
}

/*******************************************************************************/

static void vcd_cleanup(void)
{
struct slist *s, *s2;
struct vcdsymbol *v, *vt;

if(GLOBALS->indexed_vcd_c_1)
	{
	free_2(GLOBALS->indexed_vcd_c_1); GLOBALS->indexed_vcd_c_1=NULL;
	}

if(GLOBALS->sorted_vcd_c_1)
	{
	free_2(GLOBALS->sorted_vcd_c_1); GLOBALS->sorted_vcd_c_1=NULL;
	}

v=GLOBALS->vcdsymroot_vcd_c_1;
while(v)
	{
	if(v->name) free_2(v->name);
	if(v->id) free_2(v->id);
	if(v->value) free_2(v->value);
	if(v->narray) free_2(v->narray);
	vt=v;
	v=v->next;
	free_2(vt);
	}
GLOBALS->vcdsymroot_vcd_c_1=GLOBALS->vcdsymcurr_vcd_c_1=NULL;

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

if(GLOBALS->vcd_is_compressed_vcd_c_1)
	{
	pclose(GLOBALS->vcd_handle_vcd_c_1);
	GLOBALS->vcd_handle_vcd_c_1 = NULL;
	}
	else
	{
	fclose(GLOBALS->vcd_handle_vcd_c_1);
	GLOBALS->vcd_handle_vcd_c_1 = NULL;
	}

if(GLOBALS->yytext_vcd_c_1)
	{
	free_2(GLOBALS->yytext_vcd_c_1);
	GLOBALS->yytext_vcd_c_1=NULL;
	}
}

/*******************************************************************************/

TimeType vcd_main(char *fname)
{
GLOBALS->pv_vcd_c_1=GLOBALS->rootv_vcd_c_1=NULL;
GLOBALS->vcd_hier_delimeter[0]=GLOBALS->hier_delimeter;

errno=0;	/* reset in case it's set for some reason */

GLOBALS->yytext_vcd_c_1=(char *)malloc_2(GLOBALS->T_MAX_STR_vcd_c_1+1);

if(!GLOBALS->hier_was_explicitly_set) /* set default hierarchy split char */
	{
	GLOBALS->hier_delimeter='.';
	}

if(suffix_check(fname, ".gz") || suffix_check(fname, ".zip"))
	{
	char *str;
	int dlen;
	dlen=strlen(WAVE_DECOMPRESSOR);
	str=wave_alloca(strlen(fname)+dlen+1);
	strcpy(str,WAVE_DECOMPRESSOR);
	strcpy(str+dlen,fname);
	GLOBALS->vcd_handle_vcd_c_1=popen(str,"r");
	GLOBALS->vcd_is_compressed_vcd_c_1=~0;
	}
	else
	{
	if(strcmp("-vcd",fname))
		{
		GLOBALS->vcd_handle_vcd_c_1=fopen(fname,"rb");

		if(GLOBALS->vcd_handle_vcd_c_1)
			{
			fseeko(GLOBALS->vcd_handle_vcd_c_1, 0, SEEK_END);	/* do status bar for vcd load */
			GLOBALS->vcd_fsiz_vcd_c_1 = ftello(GLOBALS->vcd_handle_vcd_c_1);
			fseeko(GLOBALS->vcd_handle_vcd_c_1, 0, SEEK_SET);
			}

		if(GLOBALS->vcd_warning_filesize < 0) GLOBALS->vcd_warning_filesize = VCD_SIZE_WARN;

		if(GLOBALS->vcd_warning_filesize)
		if(GLOBALS->vcd_fsiz_vcd_c_1 > (GLOBALS->vcd_warning_filesize * (1024 * 1024)))
			{
			fprintf(stderr, "Warning! File size is %d MB.  This might fail to load.\n"
                                        "Consider converting it to the FST database format instead.  (See the\n"
                                        "vcd2fst(1) manpage for more information.)\n"
					"To disable this warning, set rc variable vcd_warning_filesize to zero.\n"
					"Alternatively, use the -o, --optimize command line option to convert to LXT2.\n\n",
						(int)(GLOBALS->vcd_fsiz_vcd_c_1/(1024*1024)));
			}
		}
		else
		{
		GLOBALS->vcd_handle_vcd_c_1=stdin;
		GLOBALS->splash_disable = 1;
		}
	GLOBALS->vcd_is_compressed_vcd_c_1=0;
	}

if(!GLOBALS->vcd_handle_vcd_c_1)
	{
	fprintf(stderr, "Error opening %s .vcd file '%s'.\n",
		GLOBALS->vcd_is_compressed_vcd_c_1?"compressed":"", fname);
	perror("Why");
	vcd_exit(255);
	}

/* SPLASH */				splash_create();

sym_hash_initialize(GLOBALS);
getch_alloc();		/* alloc membuff for vcd getch buffer */

build_slisthier();

vcd_parse();
if(GLOBALS->varsplit_vcd_c_1)
	{
	free_2(GLOBALS->varsplit_vcd_c_1);
	GLOBALS->varsplit_vcd_c_1=NULL;
	}

if((!GLOBALS->sorted_vcd_c_1)&&(!GLOBALS->indexed_vcd_c_1))
	{
	fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
	vcd_exit(255);
	}
add_tail_histents();

if(GLOBALS->vcd_save_handle) { fclose(GLOBALS->vcd_save_handle); GLOBALS->vcd_save_handle = NULL; }

fprintf(stderr, "["TTFormat"] start time.\n["TTFormat"] end time.\n", GLOBALS->start_time_vcd_c_1*GLOBALS->time_scale, GLOBALS->end_time_vcd_c_1*GLOBALS->time_scale);
if(GLOBALS->num_glitches_vcd_c_2) fprintf(stderr, "Warning: encountered %d glitch%s across %d glitch region%s.\n",
		GLOBALS->num_glitches_vcd_c_2, (GLOBALS->num_glitches_vcd_c_2!=1)?"es":"",
		GLOBALS->num_glitch_regions_vcd_c_2, (GLOBALS->num_glitch_regions_vcd_c_2!=1)?"s":"");

if(GLOBALS->vcd_fsiz_vcd_c_1)
        {
        splash_sync(GLOBALS->vcd_fsiz_vcd_c_1, GLOBALS->vcd_fsiz_vcd_c_1);
	GLOBALS->vcd_fsiz_vcd_c_1 = 0;
        }
else
if(GLOBALS->vcd_is_compressed_vcd_c_1)
	{
        splash_sync(1,1);
	GLOBALS->vcd_fsiz_vcd_c_1 = 0;
	}

GLOBALS->min_time=GLOBALS->start_time_vcd_c_1*GLOBALS->time_scale;
GLOBALS->max_time=GLOBALS->end_time_vcd_c_1*GLOBALS->time_scale;
GLOBALS->global_time_offset = GLOBALS->global_time_offset * GLOBALS->time_scale;

if((GLOBALS->min_time==GLOBALS->max_time)&&(GLOBALS->max_time==LLDescriptor(-1)))
        {
        fprintf(stderr, "VCD times range is equal to zero.  Exiting.\n");
        vcd_exit(255);
        }

vcd_build_symbols();
vcd_sortfacs();
vcd_cleanup();

getch_free();		/* free membuff for vcd getch buffer */

if(GLOBALS->blackout_regions)
        {
        struct blackout_region_t *bt = GLOBALS->blackout_regions;
        while(bt)
                {
                bt->bstart *= GLOBALS->time_scale;
                bt->bend *= GLOBALS->time_scale;
                bt = bt->next;
                }
        }

GLOBALS->is_vcd=~0;

/* SPLASH */                            splash_finalize();
return(GLOBALS->max_time);
}

/*******************************************************************************/

