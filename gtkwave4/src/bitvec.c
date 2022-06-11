/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */
#if defined _AIX
  #pragma alloca
#endif

#include "globals.h"
#include <config.h>
#include "analyzer.h"
#include "symbol.h"
#include "lxt.h"
#include "lx2.h"
#include "lxt2_read.h"
#include "vcd.h"
#include "extload.h"
#include "debug.h"
#include "bsearch.h"
#include "strace.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "main.h"
#include "menu.h"
#include "busy.h"
#include "hierpack.h"
#include <stdlib.h>

/*
 * attempt to match top vs bottom rather than emit <Vector>
 */
char *attempt_vecmatch_2(char *s1, char *s2)
{
char *s;
char *p1, *p2;
char *n1=NULL, *n2=NULL;
int n1len = 0, n2len = 0;
char *pfx = NULL;
int pfxlen = 0;
int pfxstate = 0;
char *sfx = NULL;
int sfxlen = 0;
int totlen;
int idx;

if(!strcmp(s1, s2)) { s = malloc_2(strlen(s1)+1); strcpy(s, s1); return(s); }

p1 = s1; p2 = s2;
while(*p1 && *p2)
	{
	if(pfxstate==0)
		{
		if(*p1 == *p2)
			{
			pfx = p1;
			pfxlen = p1 - s1 + 1;
			p1++;
			p2++;
			continue;
			}

		if(!pfx) return(NULL);

		if(isdigit((int)(unsigned char)*p1)&&isdigit((int)(unsigned char)*p2))
			{
			n1 = p1; n2 = p2;
			while(*p1)
				{
				if(isdigit((int)(unsigned char)*p1))
					{
					n1len++;
					}
					else
					{
					break;
					}
				p1++;
				}

			while(*p2)
				{
				if(isdigit((int)(unsigned char)*p2))
					{
					n2len++;
					}
					else
					{
					break;
					}
				p2++;
				}

			if(*p1 && *p2)
				{
				pfxstate = 1;
				sfx = p1;
				continue;
				}
				else
				{
				break;
				}
			}
		}

	if(pfxstate==1)
		{
		if(*p1 == *p2)
			{
			sfxlen++;
			p1++;
			p2++;
			continue;
			}
			else
			{
			return(NULL);
			}
		}

	return(NULL);
	}

if((*p1)||(*p2)) return(NULL);
if((!n1)||(!n2)) return(NULL); /* scan-build : null pointer on strcpy below */

while(pfxlen>1)	/* backup if matching a sequence like 20..24 where the 2 matches outside of the left bracket */
	{
	if(isdigit((int)(unsigned char)s1[pfxlen-1]))
		{
		pfxlen--;
		n1--; n1len++;
		n2--; n2len++;
		}
		else
		{
		break;
		}
	}

totlen = pfxlen + 1 + n1len + 1 + n2len + 1 + sfxlen + 1;
s = malloc_2(totlen);
memcpy(s, s1, pfxlen);
idx = pfxlen;
if(!(pfxlen && (s1[pfxlen-1]=='[')))
	{
	s[idx] = '[';
	idx++;
	}
memcpy(s+idx, n1, n1len);
idx += n1len;
s[idx] = ':';
idx++;
memcpy(s+idx, n2, n2len);
idx += n2len;
if((!sfx) || (*sfx != ']'))
	{
	s[idx] = ']';
	idx++;
	}

if(sfxlen) { memcpy(s+idx, sfx, sfxlen); idx+=sfxlen; }
s[idx] = 0;

return(s);
}


char *attempt_vecmatch(char *s1, char *s2)
{
char *pnt = NULL;

if(!s1 || !s2)
	{
	return(pnt);
	}
	else
	{
	int ns1_was_decompressed = HIER_DEPACK_ALLOC;
	char *ns1 = hier_decompress_flagged(s1, &ns1_was_decompressed);
	int ns2_was_decompressed = HIER_DEPACK_ALLOC;
	char *ns2 = hier_decompress_flagged(s2, &ns2_was_decompressed);

	if(*ns1 && *ns2)
		{
		pnt = attempt_vecmatch_2(ns1, ns2);
		}

	if(ns1_was_decompressed) free_2(ns1);
	if(ns2_was_decompressed) free_2(ns2);

	return(pnt);
	}
}


/*
 * mvlfac resolution
 */
void import_trace(nptr np)
{
set_window_busy(NULL);

if(GLOBALS->is_lxt)
	{
	import_lxt_trace(np);
	}
else
if(GLOBALS->extload) /*needs to be ahead of is_lx2 as now can be is_lx2 with FsdbReader */
	{
	import_extload_trace(np);
	}
else
if(GLOBALS->is_lx2)
	{
	import_lx2_trace(np);
	}
else
	{
	fprintf(stderr, "Internal error with mvlfac trace handling, exiting.\n");
	exit(255);
	}

set_window_idle(NULL);
}


/*
 * turn a Bits structure into a vector with deltas for faster displaying
 */
bvptr bits2vector(struct Bits *b)
{
int i;
int regions=0;
struct Node *n;
hptr *h;
vptr vhead=NULL, vcurr=NULL, vadd;
int numextrabytes;
TimeType mintime, lasttime=-1;
bvptr bitvec=NULL;
TimeType tshift, tmod;
int is_string;
int string_len;

if(!b) return(NULL);

h=(hptr *)calloc_2(b->nnbits, sizeof(hptr));

numextrabytes=b->nnbits;

for(i=0;i<b->nnbits;i++)
	{
	n=b->nodes[i];
	h[i]=&(n->head);
	}

while(h[0])	/* should never exit through this point the way we set up histents with trailers now */
	{
	mintime=MAX_HISTENT_TIME;

	vadd=(vptr)calloc_2(1,sizeof(struct VectorEnt)+numextrabytes);

	for(i=0;i<b->nnbits;i++) /* was 1...big mistake */
		{
		tshift = (b->attribs) ? b->attribs[i].shift : 0;

		if(h[i]->next)
			{
			if((h[i]->next->time >= 0) && (h[i]->next->time < MAX_HISTENT_TIME-2))
				{
				tmod = h[i]->next->time+tshift;
				if(tmod < 0) tmod = 0;
				if(tmod > MAX_HISTENT_TIME-2) tmod = MAX_HISTENT_TIME-2;
				}
				else
				{
				tmod = h[i]->next->time;	/* don't timeshift endcaps */
				}

			if(tmod < mintime)
				{
				mintime = tmod;
				}
			}
		}

	vadd->time=lasttime;
	lasttime=mintime;

	regions++;

	is_string = 1;
	string_len = 0;
	for(i=0;i<b->nnbits;i++)
                {
		if((h[i]->flags & HIST_STRING))
			{
			if(h[i]->time >= 0)
				{ 
				if(h[i]->v.h_vector)
					{
					if((GLOBALS->loaded_file_type == GHW_FILE) && (h[i]->v.h_vector[0] == '\'') && (h[i]->v.h_vector[1]) && (h[i]->v.h_vector[2] == '\''))
						{
						string_len++;
						}
						else
						{
						string_len += strlen(h[i]->v.h_vector);
						}
					}
				}
			}
			else
			{
			is_string = 0;
			break;
			}
		}

	if(is_string)
		{
		vadd->flags |= HIST_STRING;
		vadd=(vptr)realloc_2(vadd,sizeof(struct VectorEnt)+string_len+1);
		vadd->v[0] = 0;
		}

	for(i=0;i<b->nnbits;i++)
		{
		unsigned char enc;

		tshift = (b->attribs) ? b->attribs[i].shift : 0;

		if(!is_string)
			{
			if((b->attribs)&&(b->attribs[i].flags & TR_INVERT))
				{
				enc  = ((unsigned char)(h[i]->v.h_val));
				switch(enc)	/* don't remember if it's preconverted in all cases; being conservative is OK */
					{
					case AN_0: case '0':
						enc = AN_1; break;
	
					case AN_1: case '1':
						enc = AN_0; break;
	
					case AN_H: case 'h': case 'H':
						enc = AN_L; break;
	
					case AN_L: case 'l': case 'L':
						enc = AN_H; break;
	
					case 'x': case 'X':
						enc = AN_X; break;
	
					case 'z': case 'Z':
						enc = AN_Z; break;
	
					case 'u': case 'U':
						enc = AN_U; break;
	
					case 'w': case 'W':
						enc = AN_W; break;
	
					default:
						enc = enc & AN_MSK; break;
					}
				}
				else
				{
				enc  = ((unsigned char)(h[i]->v.h_val)) & AN_MSK;
				}

			vadd->v[i] = enc;
			}
			else
			{
			if(h[i]->time >= 0)
				{
				if(h[i]->v.h_vector)
					{
					if((GLOBALS->loaded_file_type == GHW_FILE) && (h[i]->v.h_vector[0] == '\'') && (h[i]->v.h_vector[1]) && (h[i]->v.h_vector[2] == '\''))
						{
						char ghw_str[2];
						ghw_str[0] = h[i]->v.h_vector[1];
						ghw_str[1] = 0;
						strcat((char *)vadd->v, ghw_str);
						}
						else
						{
						strcat((char *)vadd->v, h[i]->v.h_vector);
						}
					}			
				}
			}

		if(h[i]->next)
			{
			if((h[i]->next->time >= 0) && (h[i]->next->time < MAX_HISTENT_TIME-2))
				{
				tmod = h[i]->next->time+tshift;
				if(tmod < 0) tmod = 0;
				if(tmod > MAX_HISTENT_TIME-2) tmod = MAX_HISTENT_TIME-2;
				}
				else
				{
				tmod = h[i]->next->time;	/* don't timeshift endcaps */
				}

			if(tmod < mintime)
				{
				mintime = tmod;
				}

			if(tmod == mintime)
				{
				h[i]=h[i]->next;
				}
			}
		}

	if(vhead)
		{
		vcurr->next=vadd;
		vcurr=vadd;
		}
		else
		{
		vhead=vcurr=vadd;
		}

	if(mintime==MAX_HISTENT_TIME) break;	/* normal bail part */
	}

vadd=(vptr)calloc_2(1,sizeof(struct VectorEnt)+numextrabytes);
vadd->time=MAX_HISTENT_TIME;
for(i=0;i<numextrabytes;i++) vadd->v[i]=AN_U; /* formerly 0x55 */
if(vcurr) { vcurr->next=vadd; } /* scan-build */
regions++;

bitvec=(bvptr)calloc_2(1,sizeof(struct BitVector)+((regions)*sizeof(vptr))); /* ajb : found "regions" by manual inspection, changed to "regions-1" as array is already [1] */ /* C99, back to regions with [] */

strcpy(bitvec->bvname=(char *)malloc_2(strlen(b->name)+1),b->name);
bitvec->nbits=b->nnbits;
bitvec->numregions=regions;

vcurr=vhead;
for(i=0;i<regions;i++)
	{
	bitvec->vectors[i]=vcurr;
	if(vcurr) { vcurr=vcurr->next; } /* scan-build */
	}

return(bitvec);
}


/*
 * Make solitary traces from wildcarded signals...
 */
int maketraces(char *str, char *alias, int quick_return)
{
char *pnt, *wild;
char ch, wild_active=0;
int len;
int i;
int made=0;
unsigned int rows = 0;

pnt=str;
while((ch=*pnt))
	{
	if(ch=='*')
		{
		wild_active=1;
		break;
		}
	pnt++;
	}

if(!wild_active)	/* short circuit wildcard evaluation with bsearch */
	{
	struct symbol *s;
	nptr nexp;

	if(str[0]=='(')
		{
		for(i=1;;i++)
			{
			if(str[i]==0) return(0);
			if((str[i]==')')&&(str[i+1])) {i++; break; }
			}

		s=symfind(str+i, &rows);
		if(s)
			{
			nexp = ExtractNodeSingleBit(&s->n[rows], atoi(str+1));
			if(nexp)
				{
				AddNode(nexp, alias);
				return(~0);
				}
			}

		return(0);
		}
		else
		{
		if((s=symfind(str, &rows)))
			{
			AddNode(&s->n[rows],alias);
			return(~0);
			}
			else
			{
			/* in case its a 1-bit bit-blasted signal */
			char *str2;
                        int l = strlen(str);
			str2  = calloc_2(1,l+4);
			strcpy(str2, str);
			str2[l] = '[';
			str2[l+1] = '0';
			str2[l+2] = ']';
			str2[l+3] = 0;
			if((s=symfind(str2, &rows)))
			{
			AddNode(&s->n[rows],alias);
			return(~0);
			}
			else
			return(0);
			}
		}
	}

while(1)
{
pnt=str;
len=0;

while(1)
	{
	ch=*pnt++;
	if(isspace((int)(unsigned char)ch)||(!ch)) break;
	len++;
	}

if(len)
	{
	wild=(char *)calloc_2(1,len+1);
	memcpy(wild,str,len);
	wave_regex_compile(wild, WAVE_REGEX_WILD);

	for(i=0;i<GLOBALS->numfacs;i++)
		{
		if(wave_regex_match(GLOBALS->facs[i]->name, WAVE_REGEX_WILD))
			{
			AddNode(GLOBALS->facs[i]->n,NULL);
			made=~0;
			if(quick_return) break;
			}
		}

	free_2(wild);
	}

if(!ch) break;
str=pnt;
}
return(made);
}


/*
 * Create a vector from wildcarded signals...
 */
struct Bits *makevec(char *vec, char *str)
{
char *pnt, *pnt2, *wild=NULL;
char ch, ch2, wild_active;
int len, nodepnt=0;
int i;
struct Node *n[BITATTRIBUTES_MAX];
struct Bits *b=NULL;
unsigned int rows = 0;

while(1)
{
pnt=str;
len=0;

while(1)
	{
	ch=*pnt++;
	if(isspace((int)(unsigned char)ch)||(!ch)) break;
	len++;
	}

if(len)
	{
	wild=(char *)calloc_2(1,len+1);
	memcpy(wild,str,len);

	DEBUG(printf("WILD: %s\n",wild));

	wild_active=0;
	pnt2=wild;
	while((ch2=*pnt2))
		{
		if(ch2=='*')
			{
			wild_active=1;
			break;
			}
		pnt2++;
		}

	if(!wild_active)	/* short circuit wildcard evaluation with bsearch */
		{
		struct symbol *s;
		if(wild[0]=='(')
			{
			nptr nexp;

			for(i=1;;i++)
				{
				if(wild[i]==0) break;
				if((wild[i]==')')&&(wild[i+1]))
					{
					i++;
					s=symfind(wild+i, &rows);
					if(s)
						{
						nexp = ExtractNodeSingleBit(&s->n[rows], atoi(wild+1));
						if(nexp)
							{
							n[nodepnt++]=nexp;
							if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }
							}
						}
					else
                                              	{
                                                char *lp = strrchr(wild+i, '[');
                                                if(lp)
                                                        {
                                                        char *ns = malloc_2(strlen(wild+i) + 32);
                                                        char *colon = strchr(lp+1, ':');
                                                        int msi, lsi, bval, actual;
                                                        *lp = 0;

                                                        bval = atoi(wild+1);
                                                        if(colon)
                                                                {
                                                                msi = atoi(lp+1);
                                                                lsi = atoi(colon+1);

                                                                if(lsi > msi)
                                                                        {
                                                                        actual = msi + bval;
                                                                        }
                                                                        else
                                                                        {
                                                                        actual = msi - bval;
                                                                        }
                                                                }
                                                                else
                                                                {
                                                                actual = bval; /* punt */
                                                                }

                                                        sprintf(ns, "%s[%d]", wild+i, actual);
                                                        *lp = '[';

                                                        s=symfind(ns, &rows);
                                                        if(s)
                                                                {
								nexp =&s->n[rows];
								if(nexp)
									{
									n[nodepnt++]=nexp;
									if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }
									}
                                                                }

							free_2(ns);
                                                        }
						}
					break;
					}
				}
			}
			else
			{
			if((s=symfind(wild, &rows)))
				{
				n[nodepnt++]=&s->n[rows];
				if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }
				}
			}
		}
		else
		{
		wave_regex_compile(wild, WAVE_REGEX_WILD);
		for(i=GLOBALS->numfacs-1;i>=0;i--)	/* to keep vectors in little endian hi..lo order */
			{
			if(wave_regex_match(GLOBALS->facs[i]->name, WAVE_REGEX_WILD))
				{
				n[nodepnt++]=GLOBALS->facs[i]->n;
				if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }
				}
			}
		}
	free_2(wild);
	}

if(!ch) break;
str=pnt;
}

ifnode:
if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt)*sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mv.mvlfac) import_trace(n[i]);
		}

	b->nnbits=nodepnt;
	strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
	}

return(b);
}

/*
 * Create an annotated (b->attribs) vector from stranded signals...
 */
struct Bits *makevec_annotated(char *vec, char *str)
{
char *pnt, *wild=NULL;
char ch;
int len, nodepnt=0;
int i;
struct Node *n[BITATTRIBUTES_MAX];
struct BitAttributes ba[BITATTRIBUTES_MAX];
struct Bits *b=NULL;
int state = 0;
unsigned int rows = 0;

memset(ba, 0, sizeof(ba)); /* scan-build */

while(1)
{
pnt=str;
len=0;

while(1)
	{
	ch=*pnt++;
	if(isspace((int)(unsigned char)ch)||(!ch)) break;
	len++;
	}

if(len)
	{
	wild=(char *)calloc_2(1,len+1);
	memcpy(wild,str,len);

	DEBUG(printf("WILD: %s\n",wild));

	if(state==1)
		{
		ba[nodepnt-1].shift = atoi_64(wild);
		state++;
		goto fw;
		}
	else
	if(state==2)
		{
		sscanf(wild, "%"TRACEFLAGSSCNFMT, &ba[nodepnt-1].flags);
		state = 0;
		goto fw;
		}

	state++;

	/* no wildcards for annotated! */
		{
		struct symbol *s;

		if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }

		if(wild[0]=='(')
			{
			nptr nexp;

			for(i=1;;i++)
				{
				if(wild[i]==0) break;
				if((wild[i]==')')&&(wild[i+1]))
					{
					i++;
					s=symfind(wild+i, &rows);
					if(s)
						{
						nexp = ExtractNodeSingleBit(&s->n[rows], atoi(wild+1));
						if(nexp)
							{
							n[nodepnt++]=nexp;
							if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }
							}
						}
					else
                                              	{
                                                char *lp = strrchr(wild+i, '[');
                                                if(lp)
                                                        {
                                                        char *ns = malloc_2(strlen(wild+i) + 32);
                                                        char *colon = strchr(lp+1, ':');
                                                        int msi, lsi, bval, actual;
                                                        *lp = 0;

                                                        bval = atoi(wild+1);
                                                        if(colon)
                                                                {
                                                                msi = atoi(lp+1);
                                                                lsi = atoi(colon+1);

                                                                if(lsi > msi)
                                                                        {
                                                                        actual = msi + bval;
                                                                        }
                                                                        else
                                                                        {
                                                                        actual = msi - bval;
                                                                        }
                                                                }
                                                                else
                                                                {
                                                                actual = bval; /* punt */
                                                                }

                                                        sprintf(ns, "%s[%d]", wild+i, actual);
                                                        *lp = '[';

                                                        s=symfind(ns, &rows);
                                                        if(s)
                                                                {
								nexp =&s->n[rows];
								if(nexp)
									{
									n[nodepnt++]=nexp;
									if(nodepnt==BITATTRIBUTES_MAX) { free_2(wild); goto ifnode; }
									}
                                                                }

							free_2(ns);
                                                        }
						}

					break;
					}
				}
			}
			else
			{
			if((s=symfind(wild, &rows)))
				{
				n[nodepnt++]=&s->n[rows];
				}
			}
		}

fw:	free_2(wild);
	}

if(!ch) break;
str=pnt;
}

ifnode:
if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt)*sizeof(struct Node *));

	b->attribs = calloc_2(nodepnt, sizeof(struct BitAttributes));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mv.mvlfac) import_trace(n[i]);

		b->attribs[i].shift = ba[i].shift;
		b->attribs[i].flags = ba[i].flags;
		}

	b->nnbits=nodepnt;
	strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
	}

return(b);
}


/*
 * Create a vector from selected_status signals...
 */
struct Bits *makevec_selected(char *vec, int numrows, char direction)
{
int nodepnt=0;
int i;
struct Node *n[BITATTRIBUTES_MAX];
struct Bits *b=NULL;

if(!direction)
for(i=GLOBALS->numfacs-1;i>=0;i--)	/* to keep vectors in hi..lo order */
	{
	if(get_s_selected(GLOBALS->facs[i]))
		{
		n[nodepnt++]=GLOBALS->facs[i]->n;
		if((nodepnt==BITATTRIBUTES_MAX)||(numrows==nodepnt)) break;
		}
	}
else
for(i=0;i<GLOBALS->numfacs;i++)		/* to keep vectors in lo..hi order */
	{
	if(get_s_selected(GLOBALS->facs[i]))
		{
		n[nodepnt++]=GLOBALS->facs[i]->n;
		if((nodepnt==BITATTRIBUTES_MAX)||(numrows==nodepnt)) break;
		}
	}

if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt)*sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mv.mvlfac) import_trace(n[i]);
		}

	b->nnbits=nodepnt;
	strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
	}

return(b);
}

/*
 * add vector made in previous function
 */
int add_vector_selected(char *alias, int numrows, char direction)
{
bvptr v=NULL;
bptr b=NULL;

if((b=makevec_selected(alias, numrows, direction)))
	{
        if((v=bits2vector(b)))
                {
                v->bits=b;      /* only needed for savefile function */
                AddVector(v, NULL);
                free_2(b->name);
                b->name=NULL;
                return(v!=NULL);
                }
                else
                {
                free_2(b->name);
		if(b->attribs) free_2(b->attribs);
                free_2(b);
                }
        }
return(v!=NULL);
}

/***********************************************************************************/

/*
 * Create a vector from a range of signals...currently the single
 * bit facility_name[x] case never gets hit, but may be used in the
 * future...
 */
struct Bits *makevec_chain(char *vec, struct symbol *sym, int len)
{
int nodepnt=0, nodepnt_rev;
int i;
struct Node **n;
struct Bits *b=NULL;
struct symbol *symhi = NULL, *symlo = NULL;
char hier_delimeter2;

if(!GLOBALS->vcd_explicit_zero_subscripts)	/* 0==yes, -1==no */
	{
	hier_delimeter2=GLOBALS->hier_delimeter;
	}
	else
	{
	hier_delimeter2='[';
	}

n=(struct Node **)wave_alloca(len*sizeof(struct Node *));
memset(n, 0, len*sizeof(struct Node *)); /* scan-build */

if(!GLOBALS->autocoalesce_reversal)		/* normal case for MTI */
	{
	symhi=sym;
	while(sym)
		{
		symlo=sym;
		n[nodepnt++]=sym->n;
		sym=sym->vec_chain;
		}
	}
	else				/* for verilog XL */
	{
	nodepnt_rev=len;
	symlo=sym;
	while(sym)
		{
		nodepnt++;
		symhi=sym;
		n[--nodepnt_rev]=sym->n;
		sym=sym->vec_chain;
		}
	}

if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt)*sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mv.mvlfac) import_trace(n[i]);
		}

	b->nnbits=nodepnt;

	if(vec)
		{
		strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
		}
		else
		{
		char *s1, *s2;
		int s1_was_packed = HIER_DEPACK_ALLOC, s2_was_packed = HIER_DEPACK_ALLOC;
		int root1len=0, root2len=0;
		int l1, l2;

		s1=symhi->n->nname;
		s2=symlo->n->nname;

		if(GLOBALS->do_hier_compress)
			{
			s1 = hier_decompress_flagged(s1, &s1_was_packed);
			s2 = hier_decompress_flagged(s2, &s2_was_packed);
			}

		l1=strlen(s1);

		for(i=l1-1;i>=0;i--)
			{
			if(s1[i]==hier_delimeter2) { root1len=i+1; break; }
			}

		l2=strlen(s2);
		for(i=l2-1;i>=0;i--)
			{
			if(s2[i]==hier_delimeter2) { root2len=i+1; break; }
			}

		if((root1len!=root2len)||(!root1len)||(!root2len)||
			(strncasecmp(s1,s2,root1len)))
			{
			if(symlo!=symhi)
				{
				if(!b->attribs)
					{
					char *aname = attempt_vecmatch(s1, s2);
					if(aname) b->name = aname; else { strcpy(b->name=(char *)malloc_2(8+1),"<Vector>"); }
					}
					else
					{
					char *aname = attempt_vecmatch(s1, s2);
					if(aname) b->name = aname; else { strcpy(b->name=(char *)malloc_2(15+1),"<ComplexVector>"); }
					}
				}
				else
				{
				strcpy(b->name=(char *)malloc_2(l1+1),s1);
				}
			}
			else
			{
			int add1, add2, totallen;

			add1=l1-root1len; add2=l2-root2len;
			if(GLOBALS->vcd_explicit_zero_subscripts==-1)
				{
				add1--;
				add2--;
				}

			if(symlo!=symhi)
				{
				unsigned char fixup1 = 0, fixup2 = 0;

				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add :	      */
					+add2		/* right value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				if(GLOBALS->vcd_explicit_zero_subscripts==-1)
					{
					fixup1=*(s1+l1-1); *(s1+l1-1)=0;
					fixup2=*(s2+l2-1); *(s2+l2-1)=0;
					}

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s:%s]",s1+root1len, s2+root2len);

				if(GLOBALS->vcd_explicit_zero_subscripts==-1)
					{
					*(s1+l1-1)=fixup1;
					*(s2+l2-1)=fixup2;
					}
				}
				else
				{
				unsigned char fixup1 = 0;

				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				if(GLOBALS->vcd_explicit_zero_subscripts==-1)
					{
					fixup1=*(s1+l1-1); *(s1+l1-1)=0;
					}

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s]",s1+root1len);

				if(GLOBALS->vcd_explicit_zero_subscripts==-1)
					{
					*(s1+l1-1)=fixup1;
					}
				}
			}

		if(GLOBALS->do_hier_compress)
			{
			if(s2_was_packed) free_2(s2);
			if(s1_was_packed) free_2(s1);
			}
		}
	}

return(b);
}

/*
 * add vector made in previous function
 */
int add_vector_chain(struct symbol *s, int len)
{
bvptr v=NULL;
bptr b=NULL;

if(len>1)
	{
	if((b=makevec_chain(NULL, s, len)))
		{
	        if((v=bits2vector(b)))
	                {
	                v->bits=b;      /* only needed for savefile function */
	                AddVector(v, NULL);
	                free_2(b->name);
	                b->name=NULL;
	                return(v!=NULL);
	                }
	                else
	                {
	                free_2(b->name);
			if(b->attribs) free_2(b->attribs);
	                free_2(b);
	                }
	        }
	return(v!=NULL);
	}
	else
	{
	return(AddNode(s->n,NULL));
	}
}

/***********************************************************************************/

/*
 * Create a vector from a range of signals...currently the single
 * bit facility_name[x] case never gets hit, but may be used in the
 * future...
 */
struct Bits *makevec_range(char *vec, int lo, int hi, char direction)
{
int nodepnt=0;
int i;
struct Node *n[BITATTRIBUTES_MAX];
struct Bits *b=NULL;

if(!direction)
for(i=hi;i>=lo;i--)	/* to keep vectors in hi..lo order */
	{
	n[nodepnt++]=GLOBALS->facs[i]->n;
	if(nodepnt==BITATTRIBUTES_MAX) break;
	}
else
for(i=lo;i<=hi;i++)	/* to keep vectors in lo..hi order */
	{
	n[nodepnt++]=GLOBALS->facs[i]->n;
	if(nodepnt==BITATTRIBUTES_MAX) break;
	}

if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt)*sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mv.mvlfac) import_trace(n[i]);
		}

	b->nnbits=nodepnt;

	if(vec)
		{
		strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
		}
		else
		{
		char *s1, *s2;
		int s1_was_packed = HIER_DEPACK_ALLOC, s2_was_packed = HIER_DEPACK_ALLOC;
		int root1len=0, root2len=0;
		int l1, l2;

		if(!direction)
			{
			s1=GLOBALS->facs[hi]->n->nname;
			s2=GLOBALS->facs[lo]->n->nname;
			}
			else
			{
			s1=GLOBALS->facs[lo]->n->nname;
			s2=GLOBALS->facs[hi]->n->nname;
			}

		if(GLOBALS->do_hier_compress)
			{
			s1 = hier_decompress_flagged(s1, &s1_was_packed);
			s2 = hier_decompress_flagged(s2, &s2_was_packed);
			}

		l1=strlen(s1);

		for(i=l1-1;i>=0;i--)
			{
			if(s1[i]==GLOBALS->hier_delimeter) { root1len=i+1; break; }
			}

		l2=strlen(s2);
		for(i=l2-1;i>=0;i--)
			{
			if(s2[i]==GLOBALS->hier_delimeter) { root2len=i+1; break; }
			}

		if((root1len!=root2len)||(!root1len)||(!root2len)||
			(strncasecmp(s1,s2,root1len)))
			{
			if(lo!=hi)
				{
				if(!b->attribs)
					{
					char *aname = attempt_vecmatch(s1, s2);
					if(aname) b->name = aname; else { strcpy(b->name=(char *)malloc_2(8+1),"<Vector>"); }
					}
					else
					{
					char *aname = attempt_vecmatch(s1, s2);
					if(aname) b->name = aname; else { strcpy(b->name=(char *)malloc_2(15+1),"<ComplexVector>"); }
					}
				}
				else
				{
				strcpy(b->name=(char *)malloc_2(l1+1),s1);
				}
			}
			else
			{
			int add1, add2, totallen;

			add1=l1-root1len; add2=l2-root2len;

			if(lo!=hi)
				{
				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add :	      */
					+add2		/* right value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s:%s]",s1+root1len, s2+root2len);
				}
				else
				{
				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s]",s1+root1len);
				}
			}
		if(GLOBALS->do_hier_compress)
			{
			if(s2_was_packed) free_2(s2);
			if(s1_was_packed) free_2(s1);
			}
		}
	}

return(b);
}

/*
 * add vector made in previous function
 */
int add_vector_range(char *alias, int lo, int hi, char direction)
{
bvptr v=NULL;
bptr b=NULL;

if(lo!=hi)
	{
	if((b=makevec_range(alias, lo, hi, direction)))
		{
	        if((v=bits2vector(b)))
	                {
	                v->bits=b;      /* only needed for savefile function */
	                AddVector(v, NULL);
	                free_2(b->name);
	                b->name=NULL;
	                return(v!=NULL);
	                }
	                else
	                {
	                free_2(b->name);
			if(b->attribs) free_2(b->attribs);
	                free_2(b);
	                }
	        }
	return(v!=NULL);
	}
	else
	{
	return(AddNode(GLOBALS->facs[lo]->n,NULL));
	}
}


/*
 * splits facility name into signal and bitnumber
 */
void facsplit(char *str, int *len, int *number)
{
char *ptr;
char *numptr=NULL;
char ch;

ptr=str;

while((ch=*ptr))
        {
        if((ch>='0')&&(ch<='9'))
                {
                if(!numptr) numptr=ptr;
                }
                else numptr=NULL;
        ptr++;
        }

if(numptr)
        {
        *number=atoi(numptr);
        *len=numptr-str;
        }
        else
        {
        *number=0;
        *len=ptr-str;
        }
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
int sigcmp_2(char *s1, char *s2)
{
char *n1, *n2;
unsigned char c1, c2;
int len1, len2;

for(;;)
	{
	c1=(unsigned char)*s1;
	c2=(unsigned char)*s2;

	if((c1==0)&&(c2==0)) return(0);
	if((c1>='0')&&(c1<='9')&&(c2>='0')&&(c2<='9'))
		{
		n1=s1; n2=s2;
		len1=len2=0;

		do	{
			len1++;
			c1=(unsigned char)*(n1++);
			} while((c1>='0')&&(c1<='9'));
		if(!c1) n1--;

		do	{
			len2++;
			c2=(unsigned char)*(n2++);
			} while((c2>='0')&&(c2<='9'));
		if(!c2) n2--;

		do	{
			if(len1==len2)
				{
				c1=(unsigned char)*(s1++);
				len1--;
				c2=(unsigned char)*(s2++);
				len2--;
				}
			else
			if(len1<len2)
				{
				c1='0';
				c2=(unsigned char)*(s2++);
				len2--;
				}
			else
				{
				c1=(unsigned char)*(s1++);
				len1--;
				c2='0';
				}

			if(c1!=c2) return((int)c1-(int)c2);
			} while(len1);

		s1=n1; s2=n2;
		continue;
		}
		else
		{
		if(c1!=c2) return((int)c1-(int)c2);
		}

	s1++; s2++;
	}
}
#else
#if __STDC_VERSION__ < 199901L
inline
#endif
int sigcmp_2(char *s1, char *s2)
{
unsigned char c1, c2;
int u1, u2;

for(;;)
	{
	c1=(unsigned char)*(s1++);
	c2=(unsigned char)*(s2++);

	if(!(c1|c2)) return(0);	/* removes extra branch through logical or */
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
#endif

int sigcmp(char *s1, char *s2)
{
int rc = sigcmp_2(s1, s2);
if(!rc)
	{
	rc = strcmp(s1, s2); /* to handle leading zero "0" vs "00" cases ... we provide a definite order so bsearch doesn't fail */
	}

return(rc);
}


#ifndef __linux__
/*
 * heapsort algorithm.  this typically outperforms quicksort.  note
 * that glibc will use a modified mergesort if memory is available, so
 * under linux use the stock qsort instead.
 */
static struct symbol **hp;
static void heapify(int i, int heap_size)
{
int l, r;
int largest;
struct symbol *t;
int maxele=heap_size/2-1;	/* points to where heapswaps don't matter anymore */

for(;;)
        {
        l=2*i+1;
        r=l+1;

        if((l<heap_size)&&(sigcmp(hp[l]->name,hp[i]->name)>0))
                {
                largest=l;
                }
                else
                {
                largest=i;
                }
        if((r<heap_size)&&(sigcmp(hp[r]->name,hp[largest]->name)>0))
                {
                largest=r;
                }

        if(i!=largest)
                {
                t=hp[i];
                hp[i]=hp[largest];
                hp[largest]=t;

                if(largest<=maxele)
                        {
                        i=largest;
                        }
                        else
                        {
                        break;
                        }
                }
                else
                {
                break;
                }
        }
}

void wave_heapsort(struct symbol **a, int num)
{
int i;
int indx=num-1;
struct symbol *t;

hp=a;

for(i=(num/2-1);i>0;i--)	/* build facs into having heap property */
        {
        heapify(i,num);
        }

for(;;)
	{
        if(indx) heapify(0,indx+1);
	DEBUG(printf("%s\n", a[0]->name));

	if(indx!=0)
		{
		t=a[0];			/* sort in place by doing a REVERSE sort and */
		a[0]=a[indx];		/* swapping the front and back of the tree.. */
		a[indx--]=t;
		}
		else
		{
		break;
		}
	}
}

#else

static int qssigcomp(const void *v1, const void *v2)
{
struct symbol *a1 = *((struct symbol **)v1);
struct symbol *a2 = *((struct symbol **)v2);
return(sigcmp(a1->name, a2->name));
}

void wave_heapsort(struct symbol **a, int num)
{
qsort(a, num, sizeof(struct symbol *), qssigcomp);
}

#endif

/*
 * Malloc/Create a name from a range of signals starting from vec_root...currently the single
 * bit facility_name[x] case never gets hit, but may be used in the
 * future...
 */
char *makename_chain(struct symbol *sym)
{
int i;
struct symbol *symhi = NULL, *symlo = NULL;
char hier_delimeter2;
char *name=NULL;
char *s1, *s2;
int s1_was_packed = HIER_DEPACK_ALLOC, s2_was_packed = HIER_DEPACK_ALLOC;
int root1len=0, root2len=0;
int l1, l2;

if(!sym)
	{
	fprintf(stderr, "Internal error '%s' line %d, exiting.\n", __FILE__, __LINE__);
	exit(255);
	}

if(!GLOBALS->vcd_explicit_zero_subscripts)	/* 0==yes, -1==no */
	{
	hier_delimeter2=GLOBALS->hier_delimeter;
	}
	else
	{
	hier_delimeter2='[';
	}

if(!GLOBALS->autocoalesce_reversal)		/* normal case for MTI */
	{
	symhi=sym;
	symlo=sym; /* scan-build */
	while(sym)
		{
		symlo=sym;
		sym=sym->vec_chain;
		}
	}
	else				/* for verilog XL */
	{
	symlo=sym;
	symhi=sym; /* scan-build */
	while(sym)
		{
		symhi=sym;
		sym=sym->vec_chain;
		}
	}


s1=hier_decompress_flagged(symhi->n->nname, &s1_was_packed);
s2=hier_decompress_flagged(symlo->n->nname, &s2_was_packed);

l1=strlen(s1);

for(i=l1-1;i>=0;i--)
	{
	if(s1[i]==hier_delimeter2) { root1len=i+1; break; }
	}

l2=strlen(s2);
for(i=l2-1;i>=0;i--)
	{
	if(s2[i]==hier_delimeter2) { root2len=i+1; break; }
	}

if((root1len!=root2len)||(!root1len)||(!root2len)||
	(strncasecmp(s1,s2,root1len)))
	{
	if(symlo!=symhi)
		{
		char *aname = attempt_vecmatch(s1, s2);
		if(aname) name = aname; else { strcpy(name=(char *)malloc_2(8+1),"<Vector>"); }
		}
		else
		{
		strcpy(name=(char *)malloc_2(l1+1),s1);
		}
	}
	else
	{
	int add1, add2, totallen;

	add1=l1-root1len; add2=l2-root2len;
	if(GLOBALS->vcd_explicit_zero_subscripts==-1)
		{
		add1--;
		add2--;
		}

	if(symlo!=symhi)
		{
		unsigned char fixup1 = 0, fixup2 = 0;

		totallen=
			root1len
			-1		/* zap HIER_DELIMETER */
			+1		/* add [              */
			+add1		/* left value	      */
			+1		/* add :	      */
			+add2		/* right value	      */
			+1		/* add ]	      */
			+1		/* add 0x00	      */
			;

		if(GLOBALS->vcd_explicit_zero_subscripts==-1)
			{
			fixup1=*(s1+l1-1); *(s1+l1-1)=0;
			fixup2=*(s2+l2-1); *(s2+l2-1)=0;
			}

		name=(char *)malloc_2(totallen);
		strncpy(name,s1,root1len-1);
		sprintf(name+root1len-1,"[%s:%s]",s1+root1len, s2+root2len);

		if(GLOBALS->vcd_explicit_zero_subscripts==-1)
			{
			*(s1+l1-1)=fixup1;
			*(s2+l2-1)=fixup2;
			}
		}
		else
		{
		unsigned char fixup1 = 0;

		totallen=
			root1len
			-1		/* zap HIER_DELIMETER */
			+1		/* add [              */
			+add1		/* left value	      */
			+1		/* add ]	      */
			+1		/* add 0x00	      */
			;

		if(GLOBALS->vcd_explicit_zero_subscripts==-1)
			{
			fixup1=*(s1+l1-1); *(s1+l1-1)=0;
			}

		name=(char *)malloc_2(totallen);
		strncpy(name,s1,root1len-1);
		sprintf(name+root1len-1,"[%s]",s1+root1len);

		if(GLOBALS->vcd_explicit_zero_subscripts==-1)
			{
			*(s1+l1-1)=fixup1;
			}
		}
	}

if(s1_was_packed) { free_2(s1); }
if(s2_was_packed) { free_2(s2); }

return(name);
}

/******************************************************************/

eptr ExpandNode(nptr n)
{
int width;
int msb, lsb, delta;
int actual;
hptr h, htemp;
int i, j;
nptr *narray;
char *nam;
int offset, len;
eptr rc=NULL;
exptr exp1;

int row_hi = 0, row_lo = 0, new_msi = 0, new_lsi = 0;
int row_delta = 0, bit_delta = 0;
int curr_row = 0, curr_bit = 0;
int is_2d = 0;

if(n->mv.mvlfac) import_trace(n);

if(!n->extvals)
	{
	DEBUG(fprintf(stderr, "Nothing to expand\n"));
	}
	else
	{
	char *namex;
	int was_packed = HIER_DEPACK_ALLOC;

	msb = n->msi;
	lsb = n->lsi;
	if(msb>lsb)
		{
		width = msb - lsb + 1;
		delta = -1;
		}
		else
		{
		width = lsb - msb + 1;
		delta = 1;
		}
	actual = msb;

	narray=(nptr *)malloc_2(width*sizeof(nptr));
	rc = malloc_2(sizeof(ExpandInfo));
	rc->narray = narray;
	rc->msb = msb;
	rc->lsb = lsb;
	rc->width = width;

	if(GLOBALS->do_hier_compress)
		{
		namex = hier_decompress_flagged(n->nname, &was_packed);
		}
		else
		{
		namex = n->nname;
		}

	offset = strlen(namex);
	for(i=offset-1;i>=0;i--)
		{
		if(namex[i]=='[') break;
		}
	if(i>-1) offset=i;

	if(i>3)
		{
		if(namex[i-1]==']')
			{
			int colon_seen = 0;
			j = i-2;
		        for(;j>=0;j--)
		                {
                		if(namex[j]=='[') break;
				if(namex[j]==':') colon_seen = 1;
                		}  

			if((j>-1)&&(colon_seen))
				{
				int items = sscanf(namex+j, "[%d:%d][%d:%d]", &row_hi, &row_lo, &new_msi, &new_lsi);
				if(items == 4)
					{
					/* printf(">> %d %d %d %d (items = %d)\n", row_hi, row_lo, new_msi, new_lsi, items); */

					row_delta = (row_hi > row_lo)   ? -1 : 1;
					bit_delta = (new_msi > new_lsi) ? -1 : 1;

					curr_row = row_hi;
					curr_bit = new_msi;

					is_2d = (((row_lo - row_hi) * row_delta) + 1) * (((new_lsi - new_msi) * bit_delta) + 1) == width;
					if(is_2d) offset = j;
					}
				}
			}
		}

	nam=(char *)wave_alloca(offset+20+30);
	memcpy(nam, namex, offset);

	if(was_packed)
		{
		free_2(namex);
		}

	if(!n->harray)         /* make quick array lookup for aet display--normally this is done in addnode */
	        {
		hptr histpnt;
		int histcount;
		hptr *harray;

	        histpnt=&(n->head);
	        histcount=0;

	        while(histpnt)
	                {
	                histcount++;
	                histpnt=histpnt->next;
	                }

	        n->numhist=histcount;

	        if(!(n->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
	                {
	                fprintf( stderr, "Out of memory, can't add to analyzer\n");
	                return(NULL);
	                }

	        histpnt=&(n->head);
	        for(i=0;i<histcount;i++)
	                {
	                *harray=histpnt;
	                harray++;
	                histpnt=histpnt->next;
	                }
	        }

	h=&(n->head);
	while(h)
		{
		if(h->flags & (HIST_REAL|HIST_STRING)) return(NULL);
		h=h->next;
		}

	DEBUG(fprintf(stderr, "Expanding: (%d to %d) for %d bits over %d entries.\n", msb, lsb, width, n->numhist));

	for(i=0;i<width;i++)
		{
		narray[i] = (nptr)calloc_2(1, sizeof(struct Node));
		if(!is_2d)
			{
			sprintf(nam+offset, "[%d]", actual);
			}
			else
			{
			sprintf(nam+offset, "[%d][%d]", curr_row, curr_bit);
			if(curr_bit == new_lsi)
				{
				curr_bit = new_msi;
				curr_row += row_delta;
				}
				else
				{
				curr_bit += bit_delta;
				}
			}
#ifdef WAVE_ARRAY_SUPPORT
		if(n->array_height)
			{
			len = offset + strlen(nam+offset);
			sprintf(nam+len, "{%d}", n->this_row);
			}
#endif
		len = offset + strlen(nam+offset);
		narray[i]->nname = (char *)malloc_2(len+1);
		strcpy(narray[i]->nname, nam);

		exp1 = (exptr) calloc_2(1, sizeof(struct ExpandReferences));
		exp1->parent=n;							/* point to parent */
		exp1->parentbit=i;
		exp1->actual = actual;
		actual += delta;
		narray[i]->expansion = exp1;					/* can be safely deleted if expansion set like here */
		}

	for(i=0;i<n->numhist;i++)
		{
		h=n->harray[i];
		if((h->time<GLOBALS->min_time)||(h->time>GLOBALS->max_time))
			{
			for(j=0;j<width;j++)
				{
				if(narray[j]->curr)
					{
					htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));
					htemp->v.h_val = AN_X;			/* 'x' */
					htemp->time = h->time;
					narray[j]->curr->next = htemp;
					narray[j]->curr = htemp;
					}
					else
					{
					narray[j]->head.v.h_val = AN_X;		/* 'x' */
					narray[j]->head.time  = h->time;
					narray[j]->curr = &(narray[j]->head);
					}

				narray[j]->numhist++;
				}
			}
			else
			{
			for(j=0;j<width;j++)
				{
				unsigned char val = h->v.h_vector[j];
				switch(val)
					{
					case '0':		val = AN_0; break;
					case '1':		val = AN_1; break;
					case 'x': case 'X':	val = AN_X; break;
					case 'z': case 'Z':	val = AN_Z; break;
					case 'h': case 'H':	val = AN_H; break;
					case 'l': case 'L':	val = AN_L; break;
					case 'u': case 'U':	val = AN_U; break;
					case 'w': case 'W':	val = AN_W; break;
					case '-':		val = AN_DASH; break;
					default:	break;			/* leave val alone as it's been converted already.. */
					}

				if(narray[j]->curr->v.h_val != val) 		/* curr will have been established already by 'x' at time: -1 */
					{
					htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));
					htemp->v.h_val = val;
					htemp->time = h->time;
					narray[j]->curr->next = htemp;
					narray[j]->curr = htemp;
					narray[j]->numhist++;
					}
				}
			}
		}

	for(i=0;i<width;i++)
		{
		narray[i]->harray = (hptr *)calloc_2(narray[i]->numhist, sizeof(hptr));
		htemp = &(narray[i]->head);
		for(j=0;j<narray[i]->numhist;j++)
			{
			narray[i]->harray[j] = htemp;
			htemp = htemp->next;
			}
		}

	}

return(rc);
}

/******************************************************************/

nptr ExtractNodeSingleBit(nptr n, int bit)
{
int lft, rgh;
hptr h, htemp;
int i, j;
int actual;
nptr np;
char *nam;
int offset, len, width;
exptr exp1;

int row_hi = 0, row_lo = 0, new_msi = 0, new_lsi = 0;
int row_delta = 0, bit_delta = 0;
int curr_row = 0, curr_bit = 0;
int is_2d = 0;

if(n->mv.mvlfac) import_trace(n);

if(!n->extvals)
	{
	DEBUG(fprintf(stderr, "Nothing to expand\n"));
	return(NULL);
	}
	else
	{
	char *namex;
	int was_packed = HIER_DEPACK_ALLOC;

	if(n->lsi > n->msi)
		{
		width = n->lsi - n->msi + 1;
		rgh = n->lsi; lft = n->msi;
		actual = n->msi + bit;
		}
		else
		{
		width = n->msi - n->lsi + 1;
		rgh = n->msi; lft = n->lsi;
		actual = n->msi - bit;
		}

	if((actual>rgh)||(actual<lft))
		{
		DEBUG(fprintf(stderr, "Out of range\n"));
		return(NULL);
		}

	if(GLOBALS->do_hier_compress)
		{
		namex = hier_decompress_flagged(n->nname, &was_packed);
		}
		else
		{
		namex = n->nname;
		}

	offset = strlen(namex);
	for(i=offset-1;i>=0;i--)
		{
		if(namex[i]=='[') break;
		}
	if(i>-1) offset=i;

	if(i>3)
		{
		if(namex[i-1]==']')
			{
			int colon_seen = 0;
			j = i-2;
		        for(;j>=0;j--)
		                {
                		if(namex[j]=='[') break;
				if(namex[j]==':') colon_seen = 1;
                		}  

			if((j>-1)&&(colon_seen))
				{
				int items = sscanf(namex+j, "[%d:%d][%d:%d]", &row_hi, &row_lo, &new_msi, &new_lsi);
				if(items == 4)
					{
					/* printf("bit >> %d %d %d %d (items = %d)\n", row_hi, row_lo, new_msi, new_lsi, items); */

					row_delta = (row_hi > row_lo)   ? -1 : 1;
					bit_delta = (new_msi > new_lsi) ? -1 : 1;

					curr_row = row_hi;
					curr_bit = new_msi;

					is_2d = (((row_lo - row_hi) * row_delta) + 1) * (((new_lsi - new_msi) * bit_delta) + 1) == width;
					if(is_2d) offset = j;
					}
				}
			}
		}

	nam=(char *)wave_alloca(offset+20);
	memcpy(nam, namex, offset);

	if(was_packed)
		{
		free_2(namex);
		}

	if(!n->harray)         /* make quick array lookup for aet display--normally this is done in addnode */
	        {
		hptr histpnt;
		int histcount;
		hptr *harray;

	        histpnt=&(n->head);
	        histcount=0;

	        while(histpnt)
	                {
	                histcount++;
	                histpnt=histpnt->next;
	                }

	        n->numhist=histcount;

	        if(!(n->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
	                {
	                DEBUG(fprintf( stderr, "Out of memory, can't add to analyzer\n"));
	                return(NULL);
	                }

	        histpnt=&(n->head);
	        for(i=0;i<histcount;i++)
	                {
	                *harray=histpnt;
	                harray++;
	                histpnt=histpnt->next;
	                }
	        }

	h=&(n->head);
	while(h)
		{
		if(h->flags & (HIST_REAL|HIST_STRING)) return(NULL);
		h=h->next;
		}

	DEBUG(fprintf(stderr, "Extracting: (%d to %d) for offset #%d over %d entries.\n", n->msi, n->lsi, bit, n->numhist));

	np = (nptr)calloc_2(1, sizeof(struct Node));


	if(!is_2d)
		{
		sprintf(nam+offset, "[%d]", actual);
		}
		else
		{
		for(i=0;i<bit;i++)
			{
			if(curr_bit == new_lsi)
				{
				curr_bit = new_msi;
				curr_row += row_delta;
				}
				else
				{
				curr_bit += bit_delta;
				}
			}
		sprintf(nam+offset, "[%d][%d]", curr_row, curr_bit);
		}

#ifdef WAVE_ARRAY_SUPPORT
        if(n->array_height)
        	{
                len = offset + strlen(nam+offset);
                sprintf(nam+len, "{%d}", n->this_row);
                }
#endif
	len = offset + strlen(nam+offset);


	np->nname = (char *)malloc_2(len+1);
	strcpy(np->nname, nam);

	exp1 = (exptr) calloc_2(1, sizeof(struct ExpandReferences));
	exp1->parent=n;							/* point to parent */
	exp1->parentbit=bit;
	exp1->actual=actual;						/* actual bitnum in [] */
	np->expansion = exp1;						/* can be safely deleted if expansion set like here */

	for(i=0;i<n->numhist;i++)
		{
		h=n->harray[i];
		if((h->time<GLOBALS->min_time)||(h->time>GLOBALS->max_time))
			{
			if(np->curr)
				{
				htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));
				htemp->v.h_val = AN_X;			/* 'x' */
				htemp->time = h->time;
				np->curr->next = htemp;
				np->curr = htemp;
				}
				else
				{
				np->head.v.h_val = AN_X;		/* 'x' */
				np->head.time  = h->time;
				np->curr = &(np->head);
				}

			np->numhist++;
			}
			else
			{
			unsigned char val = h->v.h_vector[bit];
			switch(val)
				{
				case '0':		val = AN_0; break;
				case '1':		val = AN_1; break;
				case 'x': case 'X':	val = AN_X; break;
				case 'z': case 'Z':	val = AN_Z; break;
				case 'h': case 'H':	val = AN_H; break;
				case 'l': case 'L':	val = AN_L; break;
				case 'u': case 'U':	val = AN_U; break;
				case 'w': case 'W':	val = AN_W; break;
				case '-':		val = AN_DASH; break;
				default:	break;			/* leave val alone as it's been converted already.. */
				}

			if(np->curr->v.h_val != val) 		/* curr will have been established already by 'x' at time: -1 */
				{
				htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));
				htemp->v.h_val = val;
				htemp->time = h->time;
				np->curr->next = htemp;
				np->curr = htemp;
				np->numhist++;
				}
			}
		}

	np->harray = (hptr *)calloc_2(np->numhist, sizeof(hptr));
	htemp = &(np->head);
	for(j=0;j<np->numhist;j++)
		{
		np->harray[j] = htemp;
		htemp = htemp->next;
		}

	return(np);
	}
}

/******************************************************************/

/*
 * this only frees nodes created via expansion in ExpandNode() functions above!
 */
void DeleteNode(nptr n)
{
int i;

if(n->expansion)
	{
	if(n->expansion->refcnt==0)
		{
		for(i=1;i<n->numhist;i++)	/* 1st is actually part of the Node! */
			{
			free_2(n->harray[i]);
			}
		free_2(n->harray);
		free_2(n->expansion);
		free_2(n->nname);
		free_2(n);
		}
		else
		{
		n->expansion->refcnt--;
		}
	}
}
