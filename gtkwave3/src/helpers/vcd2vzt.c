/*
 * Copyright (c) 1999-2015 Tony Bybell.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * vcd.c 			23jan99ajb
 * evcd parts 			29jun99ajb
 * profiler optimizations 	15jul99ajb
 * stripped out of gtkwave	21jul99ajb
 * fix for duplicate nets       19dec00ajb
 * lxt conversion added		20nov01ajb
 */

#if defined _AIX
#pragma alloca
#endif

#include <config.h>
#include <wavealloca.h>

#include "v2l_analyzer_lxt2.h"
#include "vzt_write.h"
#include "wave_locale.h"

#undef VCD_BSEARCH_IS_PERFECT           /* bsearch is imperfect under linux, but OK under AIX */

struct vzt_wr_trace *lt=NULL;

int numfacs=0;
int deadcnt=0;
int ziptype=0;

int opt_depth = 4;
uint64_t opt_break_size = 0;
int opt_maxgranule = 8;
int opt_twostate = 0;
int opt_rle = 0;

struct symbol **sym=NULL;
struct symbol **facs=NULL;
struct symbol *firstnode=NULL;
struct symbol *curnode=NULL;
TimeType min_time=-1, max_time=-1;
char hier_delimeter='.';
char deadchar='X';

int vcd_explicit_zero_subscripts=-1;  /* 0=yes, -1=no */
char atomic_vectors=1;

static FILE *vcd_handle=NULL;
static char vcd_is_compressed=0;

static void add_histent(TimeType time, struct Node *n, char ch, int regadd, char *vector);
static void add_tail_histents(void);
static void evcd_strcpy(char *dst, char *src);

static int vcdlineno=1;
static int header_over=0;
static int dumping_off=0;
static TimeType start_time=-1;
static TimeType end_time=-1;
static TimeType current_time=-1;
static TimeType time_scale=1;	/* multiplier is 1, 10, 100 */
static TimeType time_zero=0;

static char vcd_hier_delimeter[2]={0, 0};   /* fill in after rc reading code */

/******************************************************************/

static struct slist *slistroot=NULL, *slistcurr=NULL;
static char *slisthier=NULL;
static int slisthier_len=0;

/******************************************************************/

enum Tokens   { T_VAR, T_END, T_SCOPE, T_UPSCOPE,
		T_COMMENT, T_DATE, T_DUMPALL, T_DUMPOFF, T_DUMPON,
		T_DUMPVARS, T_ENDDEFINITIONS,
		T_DUMPPORTS, T_DUMPPORTSOFF, T_DUMPPORTSON, T_DUMPPORTSALL,
		T_TIMESCALE, T_VERSION, T_VCDCLOSE, T_TIMEZERO,
		T_EOF, T_STRING, T_UNKNOWN_KEY };

char *tokens[]={ "var", "end", "scope", "upscope",
		 "comment", "date", "dumpall", "dumpoff", "dumpon",
		 "dumpvars", "enddefinitions",
		 "dumpports", "dumpportsoff", "dumpportson", "dumpportsall",
		 "timescale", "version", "vcdclose", "timezero",
		 "", "", "" };

#define NUM_TOKENS 19

static int T_MAX_STR=1024;	/* was originally a const..now it reallocs */
static char *yytext=NULL;
static int yylen=0, yylen_cache=0;

#define T_GET tok=get_token();if((tok==T_END)||(tok==T_EOF))break;

/******************************************************************/

static struct vcdsymbol *vcdsymroot=NULL, *vcdsymcurr=NULL;
static struct vcdsymbol **sorted=NULL;
static struct vcdsymbol **indexed=NULL;

enum VarTypes { V_EVENT, V_PARAMETER,
                V_INTEGER, V_REAL, V_REAL_PARAMETER=V_REAL, V_REALTIME=V_REAL, V_STRINGTYPE=V_REAL, V_REG, V_SUPPLY0,
                V_SUPPLY1, V_TIME, V_TRI, V_TRIAND, V_TRIOR,
                V_TRIREG, V_TRI0, V_TRI1, V_WAND, V_WIRE, V_WOR, V_PORT, V_IN=V_PORT, V_OUT=V_PORT, V_INOUT=V_PORT,
                V_END, V_LB, V_COLON, V_RB, V_STRING };

static char *vartypes[]={ "event", "parameter",
                "integer", "real", "real_parameter", "realtime", "string", "reg", "supply0",
                "supply1", "time", "tri", "triand", "trior",
                "trireg", "tri0", "tri1", "wand", "wire", "wor", "port", "in", "out", "inout",
                "$end", "", "", "", ""};

static const unsigned char varenums[] = {  V_EVENT, V_PARAMETER,
                V_INTEGER, V_REAL, V_REAL_PARAMETER, V_REALTIME, V_STRINGTYPE, V_REG, V_SUPPLY0,
                V_SUPPLY1, V_TIME, V_TRI, V_TRIAND, V_TRIOR,
                V_TRIREG, V_TRI0, V_TRI1, V_WAND, V_WIRE, V_WOR, V_PORT, V_IN, V_OUT, V_INOUT,
                V_END, V_LB, V_COLON, V_RB, V_STRING };

#define NUM_VTOKENS 25

static int numsyms=0;

/******************************************************************/

static struct queuedevent *queuedevents=NULL;

/******************************************************************/

static unsigned int vcd_minid = ~0;
static unsigned int vcd_maxid = 0;

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

if(indexed)
	{
	unsigned int hsh = vcdid_hash(key, len);
	if((hsh>=vcd_minid)&&(hsh<=vcd_maxid))
		{
		return(indexed[hsh-vcd_minid]);
		}
	}

v=(struct vcdsymbol **)bsearch(key, sorted, numsyms,
        sizeof(struct vcdsymbol *), vcdsymbsearchcompare);

if(v)
        {
#ifndef VCD_BSEARCH_IS_PERFECT
                for(;;)
                        {
                        t=*v;

                        if((v==sorted)||(strcmp((*(--v))->id, key)))
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
 * alias vs normal symbol adding
 */
static void alias_vs_normal_symadd(struct vcdsymbol *v, struct vcdsymbol *root_v)
{
if(!v) return; /* scan-build : should never happen */

if(!root_v)
	{
	if((v->vartype==V_INTEGER)||(v->vartype==V_REAL))
		{
		v->ltsym = vzt_wr_symbol_add(lt, v->name, 0, v->msi, v->lsi, (v->vartype==V_INTEGER)?VZT_WR_SYM_F_INTEGER:((v->vartype==V_REAL)?VZT_WR_SYM_F_DOUBLE:VZT_WR_SYM_F_BITS));
		}
		else
		{
		char buf[65537];
		if(v->msi==v->lsi)
			{
			sprintf(buf, "%s[%d]", v->name, v->msi);
			}
			else
			{
			sprintf(buf, "%s[%d:%d]", v->name, v->msi, v->lsi);
			}
		v->ltsym = vzt_wr_symbol_add(lt, buf, 0, v->msi, v->lsi, (v->vartype==V_INTEGER)?VZT_WR_SYM_F_INTEGER:((v->vartype==V_REAL)?VZT_WR_SYM_F_DOUBLE:VZT_WR_SYM_F_BITS));
		}
	}
	else
	{
	if((v->vartype==V_INTEGER)||(v->vartype==V_REAL))
		{
		vzt_wr_symbol_alias(lt, root_v->name, v->name, v->msi, v->lsi);
		}
		else
		{
		char bufold[65537], buf[65537];
		if(v->msi==v->lsi)
			{
			sprintf(bufold, "%s[%d]", root_v->name, root_v->msi);
			sprintf(buf, "%s[%d]", v->name, v->msi);
			}
			else
			{
			sprintf(bufold, "%s[%d:%d]", root_v->name, root_v->msi, root_v->lsi);
			sprintf(buf, "%s[%d:%d]", v->name, v->msi, v->lsi);
			}
		vzt_wr_symbol_alias(lt, bufold, buf, v->msi, v->lsi);
		}
	}
}

/*
 * create sorted (by id) table
 */
static void create_sorted_table(void)
{
struct vcdsymbol *v;
struct vcdsymbol **pnt;
unsigned int vcd_distance;
struct vcdsymbol *root_v;
int i;

if(numsyms)
	{
	vcd_distance = vcd_maxid - vcd_minid + 1;

	if(vcd_distance <= 8 * 1024 * 1024)
		{
		indexed = (struct vcdsymbol **)calloc_2(vcd_distance, sizeof(struct vcdsymbol *));

		printf("%d symbols span ID range of %d, using indexing...\n", numsyms, vcd_distance);

		v=vcdsymroot;
		while(v)
			{
			if(!(root_v=indexed[v->nid - vcd_minid]))
				{
				indexed[v->nid - vcd_minid] = v;
				}
			alias_vs_normal_symadd(v, root_v);

			v=v->next;
			}
		}
		else
		{
		pnt=sorted=(struct vcdsymbol **)calloc_2(numsyms, sizeof(struct vcdsymbol *));
		v=vcdsymroot;
		while(v)
			{
			*(pnt++)=v;
			v=v->next;
			}

		qsort(sorted, numsyms, sizeof(struct vcdsymbol *), vcdsymcompare);

		root_v = NULL;
		for(i=0;i<numsyms;i++)
			{
			if(sorted[i] != root_v) root_v = NULL;
			alias_vs_normal_symadd(sorted[i], root_v);
			}
		}

        v=vcdsymroot;
        while(v)
                {
                free(v->name); v->name = NULL;
                v=v->next;
                }
	}
}

/******************************************************************/

/*
 * single char get
 */
static int getch(void)
{
int ch;

ch=fgetc(vcd_handle);
if(ch=='\n') vcdlineno++;
return(((ch==EOF)||(errno))?(-1):(ch));
}

static int getch_peek(void)
{
int ch;

ch=fgetc(vcd_handle);
ungetc(ch, vcd_handle);
return(((ch==EOF)||(errno))?(-1):(ch));
}


static char *varsplit=NULL, *vsplitcurr=NULL;
static int getch_patched(void)
{
char ch;

ch=*vsplitcurr;
if(!ch)
        {
        return(-1);
        }
        else
        {
        vsplitcurr++;
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
	yytext[len++]=ch;
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

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch();
	if(ch<=' ') break;
	}
yytext[len]=0;	/* terminator */

if(is_string)
	{
	yylen=len;
	return(T_STRING);
	}

yyshadow=yytext;
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

static int var_prevch=0;
static int get_vartoken_patched(int match_kw)
{
int ch;
int i, len=0;

if(!var_prevch)
        {
        for(;;)
                {
                ch=getch_patched();
                if(ch<0) { free_2(varsplit); varsplit=NULL; return(V_END); }
                if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
                break;
                }
        }
        else
        {
        ch=var_prevch;
        var_prevch=0;
        }

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(yytext[len++]=ch;;yytext[len++]=ch)
        {
        if(len==T_MAX_STR)
                {
                yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
                }
        ch=getch_patched();
        if(ch<0) { free_2(varsplit); varsplit=NULL; break; }
        if((ch==':')||(ch==']'))
                {
                var_prevch=ch;
                break;
                }
        }
yytext[len]=0;  /* terminator */

if(match_kw)
for(i=0;i<NUM_VTOKENS;i++)
        {
        if(!strcmp(yytext,vartypes[i]))
                {
                if(ch<0) { free_2(varsplit); varsplit=NULL; }
                return(varenums[i]);
                }
        }

yylen=len;
if(ch<0) { free_2(varsplit); varsplit=NULL; }
return(V_STRING);
}


static int get_vartoken(int match_kw)
{
int ch;
int i, len=0;

if(varsplit)
        {
        int rc=get_vartoken_patched(match_kw);
        if(rc!=V_END) return(rc);
        var_prevch=0;
        }

if(!var_prevch)
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
        ch=var_prevch;
        var_prevch=0;
        }

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

if(ch=='#')     /* for MTI System Verilog '$var reg 64 >w #implicit-var###VarElem:ram_di[0.0] [63:0] $end' style declarations */
        {       /* debussy simply escapes until the space */
        yytext[len++]= '\\';
        }

for(yytext[len++]=ch;;yytext[len++]=ch)
        {
        if(len==T_MAX_STR)
                {
                yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
                }

        ch=getch();
        if(ch==' ')
                {
                if(match_kw) break;
                if(getch_peek() == '[')
                        {
                        ch = getch();
                        varsplit=yytext+len; /* keep looping so we get the *last* one */
                        continue;
                        }
                }

        if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
        if((ch=='[')&&(yytext[0]!='\\'))
                {
                varsplit=yytext+len;            /* keep looping so we get the *last* one */
                }
        else
        if(((ch==':')||(ch==']'))&&(!varsplit)&&(yytext[0]!='\\'))
                {
                var_prevch=ch;
                break;
                }
        }
yytext[len]=0;  /* absolute terminator */
if((varsplit)&&(yytext[len-1]==']'))
        {
        char *vst;
        vst=malloc_2(strlen(varsplit)+1);
        strcpy(vst, varsplit);

        *varsplit=0x00;         /* zero out var name at the left bracket */
        len=varsplit-yytext;

        varsplit=vsplitcurr=vst;
        var_prevch=0;
        }
        else
        {
        varsplit=NULL;
        }

if(match_kw)
for(i=0;i<NUM_VTOKENS;i++)
        {
        if(!strcmp(yytext,vartypes[i]))
                {
                return(varenums[i]);
                }
        }

yylen=len;
return(V_STRING);
}

static int get_strtoken(void)
{
int ch;
int len=0;

if(!var_prevch)
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
      ch=var_prevch;
      var_prevch=0;
      }

for(yytext[len++]=ch;;yytext[len++]=ch)
      {
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
      ch=getch();
      if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
      }
yytext[len]=0;        /* terminator */

yylen=len;
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
	if(hdr) { DEBUG(fprintf(stderr," %s",yytext)); }
	}
if(hdr) { DEBUG(fprintf(stderr,"\n")); }
}

static char *build_slisthier(void)
{
struct slist *s;
int len=0;

if(!slistroot)
	{
	if(slisthier)
		{
		free_2(slisthier);
		}

	slisthier_len=0;
	slisthier=(char *)malloc_2(1);
	*slisthier=0;
	return(slisthier);
	}

s=slistroot; len=0;
while(s)
	{
	len+=s->len+(s->next?1:0);
	s=s->next;
	}

if(slisthier)
	{
	free_2(slisthier);
	}

slisthier=(char *)malloc_2((slisthier_len=len)+1);
s=slistroot; len=0;
while(s)
	{
	strcpy(slisthier+len,s->str);
	len+=s->len;
	if(s->next)
		{
		strcpy(slisthier+len,vcd_hier_delimeter);
		len++;
		}
	s=s->next;
	}
return(slisthier);
}


void append_vcd_slisthier(char *str)
{
struct slist *s;
s=(struct slist *)calloc_2(1,sizeof(struct slist));
s->len=strlen(str);
s->str=(char *)malloc_2(s->len+1);
strcpy(s->str,str);

if(slistcurr)
	{
	slistcurr->next=s;
	slistcurr=s;
	}
	else
	{
	slistcurr=slistroot=s;
	}

build_slisthier();
DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
}


static void parse_valuechange(void)
{
struct vcdsymbol *v;
char *vector;
int vlen;

switch(yytext[0])
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
		if(yylen>1)
			{
			v=bsearch_vcd(yytext+1, yylen-1);
			if(!v)
				{
				fprintf(stderr,"Near line %d, Unknown VCD identifier: '%s'\n",vcdlineno,yytext+1);
				}
				else
				{
				if(v->vartype!=V_EVENT)
					{
					char vl[2];
					vl[0]=yytext[0]; vl[1]=0;
					vzt_wr_emit_value_bit_string(lt, v->ltsym, 0, vl);

					v->value[0]=yytext[0];
					DEBUG(fprintf(stderr,"%s = '%c'\n",v->name,v->value[0]));
					add_histent(current_time,v->narray[0],v->value[0],1, NULL);
					}
					else
					{
					char vl[2];

					v->value[0]=(dumping_off)?'x':'1'; /* only '1' is relevant */
					if(current_time!=(v->ev->last_event_time+1))
						{
						/* dump degating event */
						DEBUG(fprintf(stderr,"#"TTFormat" %s = '%c' (event)\n",v->ev->last_event_time+1,v->name,'0'));
						add_histent(v->ev->last_event_time+1,v->narray[0],'0',1, NULL);
						}
					DEBUG(fprintf(stderr,"%s = '%c' (event)\n",v->name,v->value[0]));
					add_histent(current_time,v->narray[0],v->value[0],1, NULL);

					vl[0]='1'; vl[1]=0;
					vzt_wr_emit_value_bit_string(lt, v->ltsym, 0, vl);
					vl[0]='0'; vl[1]=0;
					vzt_wr_emit_value_bit_string(lt, v->ltsym, 0, vl);

					v->ev->last_event_time=current_time;
					}
				}
			}
			else
			{
			fprintf(stderr,"Near line %d, Malformed VCD identifier\n", vcdlineno);
			}
		break;

	case 'b':
	case 'B':
		/* extract binary number then.. */
		vector=malloc_2(yylen_cache=yylen);
		strcpy(vector,yytext+1);
		vlen=yylen-1;

		get_strtoken();
		v=bsearch_vcd(yytext, yylen);
		if(!v)
			{
			fprintf(stderr,"Near line %d, Unknown identifier: '%s'\n",vcdlineno, yytext);
			free_2(vector);
			}
			else
			{
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
			vzt_wr_emit_value_bit_string(lt, v->ltsym, 0, v->value);

			if((v->size==1)||(!atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					add_histent(current_time, v->narray[i],v->value[i],1, NULL);
					}
				free_2(vector);
				}
				else
				{
				if(yylen_cache!=(v->size+1))
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				add_histent(current_time, v->narray[0],0,1,vector);
				free_2(vector);
				}

			}

		break;


	case 'p':
		/* extract port dump value.. */
		vector=malloc_2(yylen_cache=yylen);
		strcpy(vector,yytext+1);
		vlen=yylen-1;

		get_strtoken();	/* throw away 0_strength_component */
		get_strtoken(); /* throw away 0_strength_component */
		get_strtoken(); /* this is the id                  */
		v=bsearch_vcd(yytext, yylen);
		if(!v)
			{
			fprintf(stderr,"Near line %d, Unknown identifier: '%s'\n",vcdlineno, yytext);
			free_2(vector);
			}
			else
			{
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
			vzt_wr_emit_value_bit_string(lt, v->ltsym, 0, v->value);

			if((v->size==1)||(!atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					add_histent(current_time, v->narray[i],v->value[i],1, NULL);
					}
				free_2(vector);
				}
				else
				{
				if(yylen_cache<v->size)
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				add_histent(current_time, v->narray[0],0,1,vector);
				free_2(vector);
				}
			}
		break;


	case 'r':
	case 'R':
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d = 0;
		sscanf(yytext+1,"%lg",d);
		errno = 0;

		get_strtoken();
		v=bsearch_vcd(yytext, yylen);
		if(!v)
			{
			fprintf(stderr,"Near line %d, Unknown identifier: '%s'\n",vcdlineno, yytext);
			free_2(d);
			}
			else
			{
			if(v->vartype == V_REAL)
				{
				vzt_wr_emit_value_double(lt, v->ltsym, 0, *d);
				add_histent(current_time, v->narray[0],'g',1,(char *)d);
				}
				else
				{
				fprintf(stderr,"Near line %d, real value change for non-real '%s'\n",vcdlineno, yytext);
				}
			free_2(d);
			}

		break;
		}

	case 's':
	case 'S':
		{
		get_strtoken();	/* simply skip for now */
		break;
		}
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
			sync_end("VERSION:");
			break;
                case T_TIMEZERO:
                        {
                        int vtok=get_token();
                        if((vtok==T_END)||(vtok==T_EOF)) break;
                        time_zero=atoi_64(yytext);
                        vzt_wr_set_timezero(lt, time_zero);
                        sync_end(NULL);
                        }
                        break;
		case T_TIMESCALE:
			{
			int vtok;
			int i;
			char prefix=' ';
			int timelogadjust = 0;

			vtok=get_token();
			if((vtok==T_END)||(vtok==T_EOF)) break;
			time_scale=atoi_64(yytext);
			if(!time_scale) time_scale=1;
			else if (time_scale == 10 ) timelogadjust = +1;
			else if (time_scale == 100) timelogadjust = +2;

			for(i=0;i<yylen;i++)
				{
				if((yytext[i]<'0')||(yytext[i]>'9'))
					{
					prefix=yytext[i];
					break;
					}
				}
			if(prefix==' ')
				{
				vtok=get_token();
				if((vtok==T_END)||(vtok==T_EOF)) break;
				prefix=yytext[0];
				}
			switch(prefix)
				{
				case 's':
				case ' ': vzt_wr_set_timescale(lt,  0+timelogadjust); break;
				case 'm': vzt_wr_set_timescale(lt, -3+timelogadjust); break;
				case 'u': vzt_wr_set_timescale(lt, -6+timelogadjust); break;
				case 'n': vzt_wr_set_timescale(lt, -9+timelogadjust); break;
				case 'p': vzt_wr_set_timescale(lt, -12+timelogadjust); break;
				case 'f': vzt_wr_set_timescale(lt, -15+timelogadjust); break;
				default:	/* unknown */
					  vzt_wr_set_timescale(lt, -9+timelogadjust); break;
				}

			sync_end(NULL);
			}
			break;
		case T_SCOPE:
			T_GET;
			T_GET;
			if(tok==T_STRING)
				{
				struct slist *s;
				s=(struct slist *)calloc_2(1,sizeof(struct slist));
				s->len=yylen;
				s->str=(char *)malloc_2(yylen+1);
				strcpy(s->str,yytext);

				if(slistcurr)
					{
					slistcurr->next=s;
					slistcurr=s;
					}
					else
					{
					slistcurr=slistroot=s;
					}

				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
				}
			sync_end(NULL);
			break;
		case T_UPSCOPE:
			if(slistroot)
				{
				struct slist *s;

				s=slistroot;
				if(!s->next)
					{
					free_2(s->str);
					free_2(s);
					slistroot=slistcurr=NULL;
					}
				else
				for(;;)
					{
					if(!s->next->next)
						{
						free_2(s->next->str);
						free_2(s->next);
						s->next=NULL;
						slistcurr=s;
						break;
						}
					s=s->next;
					}
				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
				}
			sync_end(NULL);
			break;
		case T_VAR:
			{
			int vtok;
			struct vcdsymbol *v=NULL;

			var_prevch=0;
			if(varsplit)
				{
				free_2(varsplit);
				varsplit=NULL;
				}
			vtok=get_vartoken(1);
			if(vtok>V_PORT) goto bail;

			v=(struct vcdsymbol *)calloc_2(1,sizeof(struct vcdsymbol));
			v->vartype=vtok;
			v->msi=v->lsi=vcd_explicit_zero_subscripts; /* indicate [un]subscripted status */

			if(vtok==V_PORT)
				{
				vtok=get_vartoken(1);
				if(vtok==V_STRING)
					{
					v->size=atoi_64(yytext);
					if(!v->size) v->size=1;
					}
					else
					if(vtok==V_LB)
					{
					vtok=get_vartoken(1);
					if(vtok==V_END) goto err;
					if(vtok!=V_STRING) goto err;
					v->msi=atoi_64(yytext);
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
						v->lsi=atoi_64(yytext);
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
				v->id=(char *)malloc_2(yylen+1);
				strcpy(v->id, yytext);
				v->nid=vcdid_hash(yytext,yylen);

				if(v->nid < vcd_minid) vcd_minid = v->nid;
				if(v->nid > vcd_maxid) vcd_maxid = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(slisthier_len)
					{
					v->name=(char *)malloc_2(slisthier_len+1+yylen+1);
					strcpy(v->name,slisthier);
					strcpy(v->name+slisthier_len,vcd_hier_delimeter);
					strcpy(v->name+slisthier_len+1,yytext);
					}
					else
					{
					v->name=(char *)malloc_2(yylen+1);
					strcpy(v->name,yytext);
					}
				}
				else	/* regular vcd var, not an evcd port var */
				{
				vtok=get_vartoken(1);
				if(vtok==V_END) goto err;
				v->size=atoi_64(yytext);
				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(yylen+1);
				strcpy(v->id, yytext);
				v->nid=vcdid_hash(yytext,yylen);

				if(v->nid < vcd_minid) vcd_minid = v->nid;
				if(v->nid > vcd_maxid) vcd_maxid = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(slisthier_len)
					{
					v->name=(char *)malloc_2(slisthier_len+1+yylen+1);
					strcpy(v->name,slisthier);
					strcpy(v->name+slisthier_len,vcd_hier_delimeter);
					strcpy(v->name+slisthier_len+1,yytext);
					}
					else
					{
					v->name=(char *)malloc_2(yylen+1);
					strcpy(v->name,yytext);
					}

				vtok=get_vartoken(1);
				if(vtok==V_END) goto dumpv;
				if(vtok!=V_LB) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->msi=atoi_64(yytext);
				vtok=get_vartoken(0);
				if(vtok==V_RB)
					{
					v->lsi=v->msi;
					goto dumpv;
					}
				if(vtok!=V_COLON) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->lsi=atoi_64(yytext);
				vtok=get_vartoken(0);
				if(vtok!=V_RB) goto err;
				}

			dumpv:
                        if(v->size == 0) { v->vartype = V_REAL; } /* MTI fix */

			if(v->vartype==V_REAL)
				{
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
				if(v->vartype!=V_EVENT) goto err;
				v->size=v->msi-v->lsi+1;
				}
			else
			if((v->lsi>=v->msi)&&((v->lsi-v->msi+1)!=v->size))
				{
				if(v->vartype!=V_EVENT) goto err;
				v->size=v->msi-v->lsi+1;
				}

			/* initial conditions */
			v->value=(char *)malloc_2(v->size+1);
			v->value[v->size]=0;
			v->narray=(struct Node **)calloc_2(v->size,sizeof(struct Node *));
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					v->value[i]='x';

					v->narray[i]=(struct Node *)calloc_2(1,sizeof(struct Node));
					v->narray[i]->head.time=-1;
					v->narray[i]->head.v.val=1;
					}
				}

			if(v->vartype==V_EVENT)
				{
				struct queuedevent *q;
				v->ev=q=(struct queuedevent *)calloc_2(1,sizeof(struct queuedevent));
				q->sym=v;
				q->last_event_time=-1;
				q->next=queuedevents;
				queuedevents=q;
				}

			if(!vcdsymroot)
				{
				vcdsymroot=vcdsymcurr=v;
				}
				else
				{
				vcdsymcurr->next=v;
				vcdsymcurr=v;
				}
			numsyms++;

#if 0
			if((v->vartype==V_INTEGER)||(v->vartype==V_REAL))
				{
				v->ltsym = vzt_wr_symbol_add(lt, v->name, 0, v->msi, v->lsi, (v->vartype==V_INTEGER)?VZT_WR_SYM_F_INTEGER:((v->vartype==V_REAL)?VZT_WR_SYM_F_DOUBLE:VZT_WR_SYM_F_BITS));
				}
				else
				{
				char buf[65537];
				if(v->msi==v->lsi)
					{
					sprintf(buf, "%s[%d]", v->name, v->msi);
					}
					else
					{
					sprintf(buf, "%s[%d:%d]", v->name, v->msi, v->lsi);
					}
				v->ltsym = vzt_wr_symbol_add(lt, buf, 0, v->msi, v->lsi, (v->vartype==V_INTEGER)?VZT_WR_SYM_F_INTEGER:((v->vartype==V_REAL)?VZT_WR_SYM_F_DOUBLE:VZT_WR_SYM_F_BITS));
				}
#endif

			DEBUG(fprintf(stderr,"VAR %s %d %s %s[%d:%d]\n",
				vartypes[v->vartype], v->size, v->id, v->name,
					v->msi, v->lsi));
			goto bail;
			err:
			if(v)
				{
				if(v->name) free_2(v->name);
				if(v->id) free_2(v->id);
				if(v->value) free_2(v->value);
				free_2(v);
				}

			bail:
			if(vtok!=V_END) sync_end(NULL);
			break;
			}
		case T_ENDDEFINITIONS:
			if(!header_over)
				{
				header_over=1;	/* do symbol table management here */
				create_sorted_table();
				if((!sorted)&&(!indexed))
					{
					fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
					exit(1);
					}
				}
			break;
		case T_STRING:
			if(header_over)
				{
				/* catchall for events when header over */
				if(yytext[0]=='#')
					{
					TimeType t_time;
					t_time=atoi_64(yytext+1);

					if(start_time<0)
						{
						start_time=t_time;
						}

                                        if(t_time < current_time) /* avoid backtracking time counts which can happen on malformed files */
                                                {
                                                t_time = current_time;
                                                }

                                        current_time=t_time;
                                        if(end_time<t_time) end_time=t_time;        /* in case of malformed vcd files */

					vzt_wr_set_time64(lt, current_time);
					DEBUG(fprintf(stderr,"#"TTFormat"\n",t_time));
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
			dumping_off=1;
			vzt_wr_set_dumpoff(lt);
			break;
		case T_DUMPON:
		case T_DUMPPORTSON:
			dumping_off=0;
			vzt_wr_set_dumpon(lt);
			break;
		case T_DUMPVARS:
		case T_DUMPPORTS:
                        if(current_time<0)
                                { start_time=current_time=end_time=0; /* vzt_wr_set_time(lt, current_time); */ }
                        break;
                case T_VCDCLOSE:
                        sync_end("VCDCLOSE:");
                        break;  /* next token will be '#' time related followed by $end */
		case T_END:	/* either closure for dump commands or */
			break;	/* it's spurious                       */
		case T_UNKNOWN_KEY:
			sync_end(NULL);	/* skip over unknown keywords */
			break;
		case T_EOF:
			return;
		default:
			DEBUG(fprintf(stderr,"UNKNOWN TOKEN\n"));
		}
	}
}


/*******************************************************************************/

/*
 * this function doesn't do anything useful anymore except for to mark
 * statistics on the various nets...
 */
void add_histent(TimeType t_time, struct Node *n, char ch, int regadd, char *vector)
{
struct HistEnt *he;

if(!vector)
{
if(!n->curr)
	{
	he=(struct HistEnt *)calloc_2(1,sizeof(struct HistEnt));
        he->time=-1;
        he->v.val=1;

	n->curr=he;
	n->head.next=he;

	add_histent(t_time,n,ch,regadd, vector);
	}
	else
	{
	/* if(regadd) { t_time*=(time_scale); } */ /* scan-build : never read */

	if(toupper((int)(unsigned char)ch)!=deadchar) n->notdead=1;
	n->numtrans++;
       }
}
else
{
if(ch=='g')	/* real number */
	{
	if(!n->curr)
		{
		he=(struct HistEnt *)calloc_2(1,sizeof(struct HistEnt));
	        he->time=-1;
	        he->v.vector=NULL;

		n->curr=he;
		n->head.next=he;

		add_histent(t_time,n,ch,regadd, vector);
		}
		else
		{
		n->notdead=1;
		n->numtrans++;
	        }
	}
	else
	{
	if(!n->curr)
		{
		he=(struct HistEnt *)calloc_2(1,sizeof(struct HistEnt));
	        he->time=-1;
	        he->v.vector=NULL;

		n->curr=he;
		n->head.next=he;

		add_histent(t_time,n,ch,regadd, vector);
		}
		else
		{
		int i, nlen;

		nlen = strlen(vector);
		if(nlen)
			{
			n->numtrans++;
			for(i=0;i<nlen;i++)
				{
				if(toupper((int)(unsigned char)vector[i])!=deadchar)
					{
					n->notdead=1;
					return;
					}
				}
			}
	       }
	}
}

}


static void add_tail_histents(void)
{
/* dump out any pending events 1st */
struct queuedevent *q;
q=queuedevents;
while(q)
	{
	struct vcdsymbol *v;

	v=q->sym;
	if(current_time!=(v->ev->last_event_time+1))
		{
		/* dump degating event */
		DEBUG(fprintf(stderr,"#"TTFormat" %s = '%c' (event)\n",v->ev->last_event_time+1,v->name,'0'));
		add_histent(v->ev->last_event_time+1,v->narray[0],'0',1, NULL);
		}
	q=q->next;
	}
}

/*******************************************************************************/

TimeType vcd_main(char *fname, char *lxname)
{
#ifdef ONLY_NEEDED_FOR_VALGRIND_CLEAN_TEST
struct vcdsymbol *v, *v2;
#endif

vcd_hier_delimeter[0]=hier_delimeter;

errno=0;	/* reset in case it's set for some reason */

yytext=(char *)malloc_2(T_MAX_STR+1);

if((strlen(fname)>2)&&(!strcmp(fname+strlen(fname)-3,".gz")))
	{
	char *str;
	int dlen;
	dlen=strlen(WAVE_DECOMPRESSOR);
	str=(char *)wave_alloca(strlen(fname)+dlen+1);
	strcpy(str,WAVE_DECOMPRESSOR);
	strcpy(str+dlen,fname);
	vcd_handle=popen(str,"r");
	vcd_is_compressed=~0;
	}
	else
	{
	if(strcmp("-",fname))
		{
		vcd_handle=fopen(fname,"rb");
		}
		else
		{
		vcd_handle=stdin;
		}
	vcd_is_compressed=0;
	}

if(!vcd_handle)
	{
	fprintf(stderr, "Error opening %s .vcd file '%s'.\n",
		vcd_is_compressed?"compressed":"", fname);
	exit(1);
	}

lt=vzt_wr_init(lxname);
if(!lt)
	{
	fprintf(stderr, "Problem opening output file '%s'\n", lxname);
	perror("Why");
	exit(255);
	}

vzt_wr_set_compression_type(lt, ziptype);

if(opt_twostate)
	{
	vzt_wr_force_twostate(lt);
	}

vzt_wr_set_rle(lt, opt_rle);
vzt_wr_set_compression_depth(lt, opt_depth);
vzt_wr_set_break_size(lt, (off_t)opt_break_size);
vzt_wr_set_maxgranule(lt, opt_maxgranule);
vzt_wr_symbol_bracket_stripping(lt, 1);	/* this is intentional */

sym=(struct symbol **)calloc_2(SYMPRIME,sizeof(struct symbol *));
printf("\nConverting VCD File '%s' to VZT file '%s'...\n\n",(vcd_handle!=stdin)?fname:"from stdin", lxname);
build_slisthier();
vcd_parse();
if(varsplit)
	{
	free_2(varsplit);
	varsplit=NULL;
	}

add_tail_histents();

printf("["TTFormat"] start time.\n["TTFormat"] end time.\n\n", start_time, end_time);

vzt_wr_close(lt);
lt=NULL;

min_time=start_time*time_scale;
max_time=end_time*time_scale;

if((min_time==max_time)||(max_time==0))
        {
        fprintf(stderr, "VCD times range is equal to zero.  Exiting.\n");
        exit(1);
        }

if(vcd_handle!=stdin)
	{
	fclose(vcd_handle);
	vcd_handle=NULL;
	}

free(yytext); yytext=NULL;
if(indexed) { free(indexed); indexed=NULL; }
if(sorted) { free(sorted); sorted=NULL; }

#ifdef ONLY_NEEDED_FOR_VALGRIND_CLEAN_TEST
v=vcdsymroot;
while(v)
	{
	if(v->name) { free(v->name); v->name=NULL; }
	if(v->id) { free(v->id); v->id=NULL; }
	if(v->value) { free(v->value); v->value=NULL; }

	if(v->narray)
		{
		int i;
		for(i=0;i<v->size;i++)
			{
			struct HistEnt *h1, *h2;

			if((h1 = v->narray[i]->head.next))
				{
				h1 = v->narray[i]->head.next;
				while(h1)
					{
					h2 = h1->next;
					free(h1);
					h1 = h2;
					}
				}

			free(v->narray[i]); v->narray[i]=NULL;
			}

		free(v->narray); v->narray=NULL;
		}

	v2=v->next;
	free(v);
	v=v2;
	}
vcdsymroot=vcdsymcurr=NULL;
#endif

free(sym); sym=NULL;

if(slisthier) { free(slisthier); slisthier=NULL; }

return(max_time);
}

/*******************************************************************************/

/*
 * Generic hash function for symbol names...
 */
int hash(char *s)
{
char *p;
unsigned int h=0, g;
for(p=s;*p;p++)
        {
        h=(h<<4)+(*p);
        if((g=h&0xf0000000))
                {
                h=h^(g>>24);
                h=h^g;
                }
        }
return(h%SYMPRIME);
}

/*
 * add symbol to table.  no duplicate checking
 * is necessary as aet's are "correct."
 */
struct symbol *symadd(char *name, int hv)
{
struct symbol *s;

s=(struct symbol *)calloc_2(1,sizeof(struct symbol));
strcpy(s->name=(char *)malloc_2(strlen(name)+1),name);
s->next=sym[hv];
sym[hv]=s;
return(s);
}

/*
 * find a slot already in the table...
 */
struct symbol *symfind(char *s)
{
int hv;
struct symbol *temp;

        hv=hash(s);
        if(!(temp=sym[hv])) return(NULL); /* no hash entry, add here wanted to add */

        while(temp)
                {
                if(!strcmp(temp->name,s))
                        {
                        return(temp); /* in table already */
                        }
                if(!temp->next) break;
                temp=temp->next;
                }

        return(NULL); /* not found, add here if you want to add*/
}

int sigcmp(char *s1, char *s2)
{
unsigned char c1, c2;
int u1, u2;

for(;;)
        {
        c1=(unsigned char)*(s1++);
        c2=(unsigned char)*(s2++);

        if((!c1)&&(!c2)) return(0);
        if((c1<='9')&&(c2<='9')&&(c2>='0')&&(c1>='0'))
                {
                u1=(int)(c1&15);
                u2=(int)(c2&15);

                while(((c2=(unsigned char)*s2)>='0')&&(c2<='9'))
                        {
                        u2*=10;
                        u2+=(unsigned int)(c2&15);
                        s2++;
                        }

                while(((c2=(unsigned char)*s1)>='0')&&(c2<='9'))
                        {
                        u1*=10;
                        u1+=(unsigned int)(c2&15);
                        s1++;
                        }

                if(u1==u2) continue;
                        else return((int)u1-(int)u2);
                }
                else
                {
                if(c1!=c2) return((int)c1-(int)c2);
                }
        }
}


int partition(struct symbol **a, int p, int r)
{
struct symbol *x, *t;
int i,j;

x=a[p];
i=p-1;
j=r+1;

while(1)
        {
        do
                {
                j--;
                } while(sigcmp(a[j]->name,x->name)>0);

        do      {
                i++;
                } while(sigcmp(a[i]->name,x->name)<0);

        if(i<j)
                {
                t=a[i];
                a[i]=a[j];
                a[j]=t;
                }
                else
                {
                return(j);
                }
        }
}

void quicksort(struct symbol **a, int p, int r)
{
int q;

if(p<r)
        {
        q=partition(a,p,r);
        quicksort(a,p,q);
        quicksort(a,q+1,r);
        }
}


/*******************************************************************************/

void print_help(char *nam)
{
#ifdef __linux__
printf(
"Usage: %s [OPTION]... [VCDFILE] [VZTFILE]\n\n"
"  -v, --vcdname=FILE         specify VCD input filename\n"
"  -l, --vztname=FILE         specify VZT output filename\n"
"  -d, --depth=value          specify 0..9 compression depth (default = 4)\n"
"  -m, --maxgranule=value     specify number of granules per section (def = 8)\n"
"  -b, --break=value          specify break size (default = 0 = off)\n"
"  -z, --ziptype=value        specify zip type (default: 0 gzip, 1 bzip2, 2 lzma)\n"
"  -t, --twostate             force MVL2 twostate mode (default is MVL4)\n"
"  -r, --rle                  use bitwise RLE compression on value table\n"
"  -h, --help                 display this help then exit\n\n"

"VCD files may be compressed with zip or gzip.  Note that VCDFILE and VZTFILE\n"
"are optional provided the --vcdname and --vztname options are specified.\n"
"Use \"-\" as a VCD filename to accept uncompressed input from stdin.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#else
printf(
"Usage: %s [OPTION]... [VCDFILE] [VZTFILE]\n\n"
"  -v FILE                    specify VCD input filename\n"
"  -l FILE                    specify VZT output filename\n"
"  -d value                   specify 0..9 compression depth (default = 4)\n"
"  -m value                   specify number of granules per section (def = 8)\n"
"  -b value                   specify break size (default = 0 = off)\n"
"  -z value                   specify zip type (default: 0 gzip, 1 bzip2, 2 lzma)\n"
"  -t                         force MVL2 twostate mode (default is MVL4)\n"
"  -r                         use bitwise RLE compression on value table\n"
"  -h                         display this help then exit\n\n"

"VCD files may be compressed with zip or gzip.  Note that VCDFILE and VZTFILE\n"
"are optional provided the --vcdname and --vztname options are specified.\n"
"Use \"-\" as a VCD filename to accept uncompressed input from stdin.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#endif

exit(0);
}


int main(int argc, char **argv)
{
char opt_errors_encountered=0;
char *vname=NULL, *lxname=NULL;
int c;

WAVE_LOCALE_FIX

while (1)
        {
#ifdef __linux__
        int option_index = 0;

        static struct option long_options[] =
                {
		{"vcdname", 1, 0, 'v'},
		{"vztname", 1, 0, 'l'},
		{"depth", 1, 0, 'd'},
		{"maxgranule", 1, 0, 'm'},
		{"break", 1, 0, 'b'},
                {"help", 0, 0, 'h'},
                {"twostate", 0, 0, 't'},
                {"rle", 0, 0, 'r'},
		{"ziptype", 1, 0, 'z'},
                {0, 0, 0, 0}
                };

        c = getopt_long (argc, argv, "v:l:d:m:b:z:htr", long_options, &option_index);
#else
        c = getopt      (argc, argv, "v:l:d:m:b:z:htr");
#endif

        if (c == -1) break;     /* no more args */

        switch (c)
                {
		case 'v':
			if(vname) free(vname);
                        vname = malloc_2(strlen(optarg)+1);
                        strcpy(vname, optarg);
			break;

		case 'l':
			if(lxname) free(lxname);
                        lxname = malloc_2(strlen(optarg)+1);
                        strcpy(lxname, optarg);
			break;

		case 'd':
			opt_depth = atoi(optarg);
			if(opt_depth<0) opt_depth = 0;
			if(opt_depth>9) opt_depth = 9;
			break;

		case 'm':
			opt_maxgranule = atoi(optarg);
			if(opt_maxgranule<1) opt_maxgranule=1;
			break;

		case 'b':
			sscanf(optarg, "%"SCNu64, &opt_break_size);
			errno = 0;
			break;

                case 'h':
			print_help(argv[0]);
                        break;

                case 't':
			opt_twostate = 1;
                        break;

                case 'r':
			opt_rle = 1;
                        break;

		case 'z':
			ziptype = atoi(optarg);
			if((ziptype < VZT_WR_IS_GZ) || (ziptype > VZT_WR_IS_LZMA)) ziptype = VZT_WR_IS_GZ;
			break;

                case '?':
                        opt_errors_encountered=1;
                        break;

                default:
                        /* unreachable */
                        break;
                }
        }

if(opt_errors_encountered)
        {
        print_help(argv[0]);
        }

if (optind < argc)
        {
        while (optind < argc)
                {
                if(!vname)
                        {
                        vname = malloc_2(strlen(argv[optind])+1);
                        strcpy(vname, argv[optind++]);
                        }
                else if(!lxname)
                        {
                        lxname = malloc_2(strlen(argv[optind])+1);
                        strcpy(lxname, argv[optind++]);
                        }
		else
			{
			break;
			}
                }
        }

if((!vname)||(!lxname))
        {
        print_help(argv[0]);
        }

vcd_main(vname, lxname);

free(vname); free(lxname);

return(0);
}

