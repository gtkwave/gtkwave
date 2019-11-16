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
 * recoder using vlists         17aug06ajb
 * code cleanup                 04sep06ajb
 * added in/out port vartype    31jan07ajb
 * use gperf for port vartypes  19feb07ajb
 * MTI SV implicit-var fix      05apr07ajb
 * MTI SV len=0 is real var     05apr07ajb
 * VCD fastloading		05mar09ajb
 * Backtracking fix             16oct18ajb
 */
#include <config.h>
#include "globals.h"
#include "vcd.h"
#include "vlist.h"
#include "lx2.h"
#include "hierpack.h"

/**/

static void malform_eof_fix(void)
{
if(feof(GLOBALS->vcd_handle_vcd_recoder_c_2))
	{
        memset(GLOBALS->vcdbuf_vcd_recoder_c_3, ' ', VCD_BSIZ);
        GLOBALS->vst_vcd_recoder_c_3=GLOBALS->vend_vcd_recoder_c_3;
        }
}

/**/

static void vlist_packer_emit_uv64(struct vlist_packer_t **vl, guint64 v)
{
guint64 nxt;

while((nxt = v>>7))
        {
	vlist_packer_alloc(*vl, v&0x7f);
        v = nxt;
        }

vlist_packer_alloc(*vl, (v&0x7f) | 0x80);
}


static void vlist_packer_emit_utt(struct vlist_packer_t **vl, UTimeType v)
{
UTimeType nxt;

while((nxt = v>>7))
        {
	vlist_packer_alloc(*vl, v&0x7f);
        v = nxt;
        }

vlist_packer_alloc(*vl, (v&0x7f) | 0x80);
}


static void vlist_packer_emit_uv32(struct vlist_packer_t **vl, unsigned int v)
{
unsigned int nxt;

while((nxt = v>>7))
        {
	vlist_packer_alloc(*vl, v&0x7f);
        v = nxt;
        }

vlist_packer_alloc(*vl, (v&0x7f) | 0x80);
}


static void vlist_packer_emit_string(struct vlist_packer_t **vl, char *s)
{
while(*s)
	{
	vlist_packer_alloc(*vl, *s);
	s++;
	}
vlist_packer_alloc(*vl, 0);
}

static void vlist_packer_emit_mvl9_string(struct vlist_packer_t **vl, char *s)
{
unsigned int recoded_bit;
unsigned char which = 0;
unsigned char accum = 0;

while(*s)
	{
	switch(*s)
	        {
	        case '0':		recoded_bit = AN_0; break;
	        case '1':		recoded_bit = AN_1; break;
	        case 'x': case 'X':	recoded_bit = AN_X; break;
	        case 'z': case 'Z':	recoded_bit = AN_Z; break;
	        case 'h': case 'H':	recoded_bit = AN_H; break;
	        case 'u': case 'U':	recoded_bit = AN_U; break;
	        case 'w': case 'W':     recoded_bit = AN_W; break;
	        case 'l': case 'L':	recoded_bit = AN_L; break;
		default:		recoded_bit = AN_DASH; break;
		}

	if(!which)
		{
		accum = (recoded_bit << 4);
		which = 1;
		}
		else
		{
		accum |= recoded_bit;
		vlist_packer_alloc(*vl, accum);
		which = accum = 0;
		}
	s++;
	}

recoded_bit = AN_MSK; /* XXX : this is assumed it is going to remain a 4 bit max quantity! */
if(!which)
	{
        accum = (recoded_bit << 4);
        }
        else
        {
        accum |= recoded_bit;
        }

vlist_packer_alloc(*vl, accum);
}

/**/

static void vlist_emit_uv32(struct vlist_t **vl, unsigned int v)
{
unsigned int nxt;
char *pnt;

if(GLOBALS->vlist_prepack) { vlist_packer_emit_uv32((struct vlist_packer_t **)vl, v); return; }

while((nxt = v>>7))
        {
	pnt = vlist_alloc(vl, 1);
        *pnt = (v&0x7f);
        v = nxt;
        }

pnt = vlist_alloc(vl, 1);
*pnt = (v&0x7f) | 0x80;
}


static void vlist_emit_string(struct vlist_t **vl, char *s)
{
char *pnt;

if(GLOBALS->vlist_prepack) { vlist_packer_emit_string((struct vlist_packer_t **)vl, s); return; }

while(*s)
	{
	pnt = vlist_alloc(vl, 1);
	*pnt = *s;
	s++;
	}
pnt = vlist_alloc(vl, 1);
*pnt = 0;
}

static void vlist_emit_mvl9_string(struct vlist_t **vl, char *s)
{
char *pnt;
unsigned int recoded_bit;
unsigned char which;
unsigned char accum;

if(GLOBALS->vlist_prepack) { vlist_packer_emit_mvl9_string((struct vlist_packer_t **)vl, s); return; }

which = accum = 0;

while(*s)
	{
	switch(*s)
	        {
	        case '0':		recoded_bit = AN_0; break;
	        case '1':		recoded_bit = AN_1; break;
	        case 'x': case 'X':	recoded_bit = AN_X; break;
	        case 'z': case 'Z':	recoded_bit = AN_Z; break;
	        case 'h': case 'H':	recoded_bit = AN_H; break;
	        case 'u': case 'U':	recoded_bit = AN_U; break;
	        case 'w': case 'W':     recoded_bit = AN_W; break;
	        case 'l': case 'L':	recoded_bit = AN_L; break;
		default:		recoded_bit = AN_DASH; break;
		}

	if(!which)
		{
		accum = (recoded_bit << 4);
		which = 1;
		}
		else
		{
		accum |= recoded_bit;
		pnt = vlist_alloc(vl, 1);
		*pnt = accum;
		which = accum = 0;
		}
	s++;
	}

recoded_bit = AN_MSK; /* XXX : this is assumed it is going to remain a 4 bit max quantity! */
if(!which)
	{
        accum = (recoded_bit << 4);
        }
        else
        {
        accum |= recoded_bit;
        }

pnt = vlist_alloc(vl, 1);
*pnt = accum;
}

/**/

static void write_fastload_time_section(void)
{
struct vlist_t *vlist;
struct vlist_packer_t *vlist_p;

/* emit blackout regions */
if(GLOBALS->blackout_regions)
	{
	struct blackout_region_t *bt = GLOBALS->blackout_regions;
	unsigned int bcnt = 0;
	while(bt)
		{
		bcnt++;
		bt = bt->next;
		}

	vlist_packer_emit_uv32((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, bcnt);

	bt = GLOBALS->blackout_regions;
	while(bt)
		{
		vlist_packer_emit_utt((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, bt->bstart);
		vlist_packer_emit_utt((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, bt->bend);
		bt = bt->next;
		}
	}
	else
	{
	vlist_packer_emit_uv32((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, 0);
	}

vlist_p = (struct vlist_packer_t *)GLOBALS->time_vlist_vcd_recoder_write;
vlist_packer_finalize(vlist_p);
vlist = vlist_p->v;
free_2(vlist_p);
GLOBALS->time_vlist_vcd_recoder_write = vlist;
vlist_freeze(&GLOBALS->time_vlist_vcd_recoder_write);
}

#ifdef HAVE_SYS_STAT_H
static void write_fastload_header(struct stat *mystat, unsigned int finalize_cnt)
#else
static void write_fastload_header(unsigned int finalize_cnt)
#endif
{
/*
write out the trailer information for vcd fastload...

vcd file size
vcd last modification time
number of finalize values
number of variable-length integer values in the time table (GLOBALS->time_vlist_count_vcd_recoder_c_1)
GLOBALS->time_vlist_vcd_recoder_write value
deltas from GLOBALS->time_vlist_vcd_recoder_write and on though stepping through list generated by vlist_emit_finalize()
*/

struct vlist_packer_t *vlist_p = vlist_packer_create();
struct vlist_t *vlist_summary_index;

struct vcdsymbol *v = GLOBALS->vcdsymroot_vcd_recoder_c_3;
guint64 val = (guint64)(uintptr_t)GLOBALS->time_vlist_vcd_recoder_write;
guint64 nval;
char buf[33];
char *pnt;

memset(buf, 0, sizeof(buf)); /* scan-build */

#ifdef HAVE_SYS_STAT_H
vlist_packer_emit_uv64(&vlist_p, (guint64)mystat->st_size);
vlist_packer_emit_uv64(&vlist_p, (guint64)mystat->st_mtime);
#else
vlist_packer_emit_uv64(&vlist_p, (guint64)0);
vlist_packer_emit_uv64(&vlist_p, (guint64)0);
#endif

vlist_packer_emit_uv32(&vlist_p, finalize_cnt);
vlist_packer_emit_uv64(&vlist_p, GLOBALS->time_vlist_count_vcd_recoder_c_1);

vlist_packer_emit_uv64(&vlist_p, val);
val = 0;
while(v)
	{
	nptr n = v->narray[0];
	nval = (guint64)(uintptr_t)n->mv.mvlfac_vlist;

	vlist_packer_emit_uv64(&vlist_p, nval - val);
	val = nval;

	v = v->next;
	}

vlist_packer_finalize(vlist_p);
vlist_summary_index = vlist_p->v;
free_2(vlist_p);
GLOBALS->time_vlist_vcd_recoder_write = vlist_summary_index;
vlist_freeze(&vlist_summary_index);

pnt = buf;
val = (guint64)(uintptr_t)vlist_summary_index;
while((nval = val>>7))
	{
	*(pnt++) = (val&0x7f);
	val = nval;
	}
*(pnt++) = ((val&0x7f) | 0x80);
do
	{
	pnt--;
	fputc((unsigned char)*pnt, GLOBALS->vlist_handle);
	} while(pnt != buf);

fflush(GLOBALS->vlist_handle);
}


static int read_fastload_body(void)
{
char *depacked = GLOBALS->fastload_depacked;
char *pnt = GLOBALS->fastload_current;
int rc = 0;
guint64 v = 0, vprev = 0;
int shamt = 0;
/* unsigned int num_finalize; */ /* scan-build */
unsigned int num_in_time_table, num_blackout_regions;
guint64 time_vlist_vcd_recoder_write;
struct vcdsymbol *vs;
struct vlist_t *vl;
unsigned int list_size;
unsigned int i;
struct blackout_region_t *bt_head = NULL, *bt_curr = NULL;

v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
/* num_finalize = v; */ /* scan-build */

v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
GLOBALS->time_vlist_count_vcd_recoder_c_1 = num_in_time_table = v;

v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
time_vlist_vcd_recoder_write = v;

vs=GLOBALS->vcdsymroot_vcd_recoder_c_3;
while(vs)
        {
        nptr n = vs->narray[0];

	v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
	v += vprev;
	vprev = v;

        n->mv.mvlfac_vlist = (struct vlist_t *)(intptr_t)(v);
	vs = vs->next;
	}

vlist_packer_decompress_destroy((char *)depacked);
GLOBALS->fastload_depacked = NULL;
GLOBALS->fastload_current = NULL;

/* now create the time table */
vl = (struct vlist_t *)(intptr_t)time_vlist_vcd_recoder_write;
vlist_uncompress(&vl);
depacked = pnt = (char *)vlist_packer_decompress(vl, &list_size);
vlist_destroy(vl);

vprev = 0;
for(i=0;i<num_in_time_table;i++)
	{
	TimeType tim;
	TimeType *tt;

	v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));

	v += vprev;
	vprev = v;

	tt = vlist_alloc(&GLOBALS->time_vlist_vcd_recoder_c_1, 0);
        *tt = tim = (TimeType)v;

	if(!i)
		{
		GLOBALS->start_time_vcd_recoder_c_3=tim;
		}

	GLOBALS->current_time_vcd_recoder_c_3=GLOBALS->end_time_vcd_recoder_c_3=tim;
	}

/* now process blackout regions */
v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
num_blackout_regions = v;

if(num_blackout_regions)
	{
	for(i=0;i<num_blackout_regions;i++)
		{
		struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));
		TimeType tim;

		if(!bt_head)
			{
			bt_head = bt_curr = bt;
			}
			else
			{
			bt_curr->next = bt;
			bt_curr = bt;
			}

		v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
		tim = v;
		bt->bstart = tim;

		v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));
		tim = v;
		bt->bend = tim;
		}

	GLOBALS->blackout_regions = bt_head;
	}

vlist_packer_decompress_destroy((char *)depacked);

return(rc);
}

#ifdef HAVE_SYS_STAT_H
static int read_fastload_header(struct stat *st)
#else
static int read_fastload_header(void)
#endif
{
int rc = 0;
int fs_rc = fseeko(GLOBALS->vlist_handle, 0, SEEK_END);
off_t ftlen = ftello(GLOBALS->vlist_handle);
guint64 v = 0;
int ch;
int shamt = 0;
unsigned char *depacked = NULL;
struct vlist_t *vl;
unsigned int list_size;
unsigned char *pnt;
#ifdef HAVE_SYS_STAT_H
struct stat mystat;
int stat_rc = stat(GLOBALS->loaded_file_name, &mystat);
#endif

if((fs_rc<0)||(!ftlen))
	{
	goto bail;
	}

do	{
	ftlen--;
	fseeko(GLOBALS->vlist_handle, ftlen, SEEK_SET);
	ch = fgetc(GLOBALS->vlist_handle);
	if(ch == EOF) { errno = 0; goto bail; }
	v |= ((guint64)(ch & 0x7f)) << shamt;
	shamt += 7;
	} while(!(ch & 0x80));

vl = (struct vlist_t *)(intptr_t)v;
vlist_uncompress(&vl);
depacked = vlist_packer_decompress(vl, &list_size);
vlist_destroy(vl);

pnt = depacked;
v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));

#ifdef HAVE_SYS_STAT_H
if((stat_rc)||(v != (guint64)mystat.st_size))
	{
	goto bail;
	}
#endif

v = 0; shamt = 0; do { v |= ((guint64)(*pnt & 0x7f)) << shamt; shamt += 7; } while(!(*(pnt++) & 0x80));

#ifdef HAVE_SYS_STAT_H
if(v != (guint64)st->st_mtime)
	{
	goto bail;
	}
#endif

rc = 1;
GLOBALS->fastload_depacked = (char *)depacked;
GLOBALS->fastload_current = (char *)pnt;

bail:
if(!rc) vlist_packer_decompress_destroy((char *)depacked);
return(rc);
}

/**/

#undef VCD_BSEARCH_IS_PERFECT		/* bsearch is imperfect under linux, but OK under AIX */

static void add_histent(TimeType time, struct Node *n, char ch, int regadd, char *vector);
static void vcd_build_symbols(void);
static void vcd_cleanup(void);
static void evcd_strcpy(char *dst, char *src);

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

if(GLOBALS->indexed_vcd_recoder_c_3)
        {
        unsigned int hsh = vcdid_hash(key, len);
        if((hsh>=GLOBALS->vcd_minid_vcd_recoder_c_3)&&(hsh<=GLOBALS->vcd_maxid_vcd_recoder_c_3))
                {
                return(GLOBALS->indexed_vcd_recoder_c_3[hsh-GLOBALS->vcd_minid_vcd_recoder_c_3]);
                }

	return(NULL);
        }

if(GLOBALS->sorted_vcd_recoder_c_3)
	{
	v=(struct vcdsymbol **)bsearch(key, GLOBALS->sorted_vcd_recoder_c_3, GLOBALS->numsyms_vcd_recoder_c_3,
		sizeof(struct vcdsymbol *), vcdsymbsearchcompare);

	if(v)
		{
		#ifndef VCD_BSEARCH_IS_PERFECT
			for(;;)
				{
				t=*v;

				if((v==GLOBALS->sorted_vcd_recoder_c_3)||(strcmp((*(--v))->id, key)))
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
	if(!GLOBALS->err_vcd_recoder_c_3)
		{
		fprintf(stderr, "Near byte %d, VCD search table NULL..is this a VCD file?\n", (int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)));
		GLOBALS->err_vcd_recoder_c_3=1;
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

if(GLOBALS->sorted_vcd_recoder_c_3)
	{
	free_2(GLOBALS->sorted_vcd_recoder_c_3);	/* this means we saw a 2nd enddefinition chunk! */
	GLOBALS->sorted_vcd_recoder_c_3=NULL;
	}

if(GLOBALS->indexed_vcd_recoder_c_3)
	{
	free_2(GLOBALS->indexed_vcd_recoder_c_3);
	GLOBALS->indexed_vcd_recoder_c_3=NULL;
	}

if(GLOBALS->numsyms_vcd_recoder_c_3)
	{
        vcd_distance = GLOBALS->vcd_maxid_vcd_recoder_c_3 - GLOBALS->vcd_minid_vcd_recoder_c_3 + 1;

        if((vcd_distance <= VCD_INDEXSIZ)||(!GLOBALS->vcd_hash_kill))
                {
                GLOBALS->indexed_vcd_recoder_c_3 = (struct vcdsymbol **)calloc_2(vcd_distance, sizeof(struct vcdsymbol *));

		/* printf("%d symbols span ID range of %d, using indexing... hash_kill = %d\n", GLOBALS->numsyms_vcd_recoder_c_3, vcd_distance, GLOBALS->vcd_hash_kill);  */

                v=GLOBALS->vcdsymroot_vcd_recoder_c_3;
                while(v)
                        {
                        if(!GLOBALS->indexed_vcd_recoder_c_3[v->nid - GLOBALS->vcd_minid_vcd_recoder_c_3]) GLOBALS->indexed_vcd_recoder_c_3[v->nid - GLOBALS->vcd_minid_vcd_recoder_c_3] = v;
                        v=v->next;
                        }
                }
                else
		{
		pnt=GLOBALS->sorted_vcd_recoder_c_3=(struct vcdsymbol **)calloc_2(GLOBALS->numsyms_vcd_recoder_c_3, sizeof(struct vcdsymbol *));
		v=GLOBALS->vcdsymroot_vcd_recoder_c_3;
		while(v)
			{
			*(pnt++)=v;
			v=v->next;
			}

		qsort(GLOBALS->sorted_vcd_recoder_c_3, GLOBALS->numsyms_vcd_recoder_c_3, sizeof(struct vcdsymbol *), vcdsymcompare);
		}
	}
}

/******************************************************************/

static unsigned int vlist_emit_finalize(void)
{
struct vcdsymbol *v /* , *vprime */; /* scan-build */
struct vlist_t *vlist;
char vlist_prepack = GLOBALS->vlist_prepack;
int cnt = 0;

v=GLOBALS->vcdsymroot_vcd_recoder_c_3;
while(v)
	{
	nptr n = v->narray[0];

	set_vcd_vartype(v, n);

	if(n->mv.mvlfac_vlist)
		{
		if(vlist_prepack)
			{
	                vlist_packer_finalize(n->mv.mvlfac_packer_vlist);
	                vlist = n->mv.mvlfac_packer_vlist->v;
	                free_2(n->mv.mvlfac_packer_vlist);
	                n->mv.mvlfac_vlist = vlist;
	                vlist_freeze(&n->mv.mvlfac_vlist);
			}
			else
			{
			vlist_freeze(&n->mv.mvlfac_vlist);
			}
		}
		else
		{
		n->mv.mvlfac_vlist = vlist_prepack ? ((struct vlist_t *)vlist_packer_create()) : vlist_create(sizeof(char));

		if((/* vprime= */ bsearch_vcd(v->id, strlen(v->id)))==v) /* hash mish means dup net */ /* scan-build */
			{
			switch(v->vartype)
				{
				case V_REAL:
		                          	vlist_emit_uv32(&n->mv.mvlfac_vlist, 'R');
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
						vlist_emit_uv32(&n->mv.mvlfac_vlist, 0);
						vlist_emit_string(&n->mv.mvlfac_vlist, "NaN");
						break;

				case V_STRINGTYPE:
						vlist_emit_uv32(&n->mv.mvlfac_vlist, 'S');
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
						vlist_emit_uv32(&n->mv.mvlfac_vlist, 0);
						vlist_emit_string(&n->mv.mvlfac_vlist, "UNDEF");
						break;

				default:
					if(v->size==1)
						{
	                                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)'0');
	                                        vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
						vlist_emit_uv32(&n->mv.mvlfac_vlist, RCV_X);
						}
						else
						{
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, 'B');
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
		                                vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
						vlist_emit_uv32(&n->mv.mvlfac_vlist, 0);
						vlist_emit_mvl9_string(&n->mv.mvlfac_vlist, "x");
						}
					break;
				}
			}

		if(vlist_prepack)
			{
	                vlist_packer_finalize(n->mv.mvlfac_packer_vlist);
	                vlist = n->mv.mvlfac_packer_vlist->v;
	                free_2(n->mv.mvlfac_packer_vlist);
	                n->mv.mvlfac_vlist = vlist;
	                vlist_freeze(&n->mv.mvlfac_vlist);
			}
			else
			{
			vlist_freeze(&n->mv.mvlfac_vlist);
			}
		}
	v=v->next;
	cnt++;
	}

return(cnt);
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(void)
{
GLOBALS->vend_vcd_recoder_c_3=GLOBALS->vst_vcd_recoder_c_3=GLOBALS->vcdbuf_vcd_recoder_c_3=(char *)calloc_2(1,VCD_BSIZ);
}

static void getch_free(void)
{
free_2(GLOBALS->vcdbuf_vcd_recoder_c_3);
GLOBALS->vcdbuf_vcd_recoder_c_3=GLOBALS->vst_vcd_recoder_c_3=GLOBALS->vend_vcd_recoder_c_3=NULL;
}



static int getch_fetch(void)
{
size_t rd;

errno = 0;
if(feof(GLOBALS->vcd_handle_vcd_recoder_c_2)) return(-1);

GLOBALS->vcdbyteno_vcd_recoder_c_3+=(GLOBALS->vend_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3);
rd=fread(GLOBALS->vcdbuf_vcd_recoder_c_3, sizeof(char), VCD_BSIZ, GLOBALS->vcd_handle_vcd_recoder_c_2);
GLOBALS->vend_vcd_recoder_c_3=(GLOBALS->vst_vcd_recoder_c_3=GLOBALS->vcdbuf_vcd_recoder_c_3)+rd;

if((!rd)||(errno)) return(-1);

if(GLOBALS->vcd_fsiz_vcd_recoder_c_2)
	{
	splash_sync(GLOBALS->vcdbyteno_vcd_recoder_c_3, GLOBALS->vcd_fsiz_vcd_recoder_c_2); /* gnome 2.18 seems to set errno so splash moved here... */
	}

return((int)(*GLOBALS->vst_vcd_recoder_c_3));
}

static inline signed char getch(void) {
  signed char ch = (GLOBALS->vst_vcd_recoder_c_3!=GLOBALS->vend_vcd_recoder_c_3)?((int)(*GLOBALS->vst_vcd_recoder_c_3)):(getch_fetch());
  GLOBALS->vst_vcd_recoder_c_3++;
  return(ch);
}

static inline signed char getch_peek(void) {
  signed char ch = (GLOBALS->vst_vcd_recoder_c_3!=GLOBALS->vend_vcd_recoder_c_3)?((int)(*GLOBALS->vst_vcd_recoder_c_3)):(getch_fetch());
  /* no increment */
  return(ch);
}

static int getch_patched(void)
{
char ch;

ch=*GLOBALS->vsplitcurr_vcd_recoder_c_3;
if(!ch)
	{
	return(-1);
	}
	else
	{
	GLOBALS->vsplitcurr_vcd_recoder_c_3++;
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
	GLOBALS->yytext_vcd_recoder_c_3[len++]=ch;
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

for(GLOBALS->yytext_vcd_recoder_c_3[len++]=ch;;GLOBALS->yytext_vcd_recoder_c_3[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_recoder_c_3)
		{
		GLOBALS->yytext_vcd_recoder_c_3=(char *)realloc_2(GLOBALS->yytext_vcd_recoder_c_3, (GLOBALS->T_MAX_STR_vcd_recoder_c_3=GLOBALS->T_MAX_STR_vcd_recoder_c_3*2)+1);
		}
	ch=getch();
	if(ch<=' ') break;
	}
GLOBALS->yytext_vcd_recoder_c_3[len]=0;	/* terminator */

if(is_string)
	{
	GLOBALS->yylen_vcd_recoder_c_3=len;
	return(T_STRING);
	}

yyshadow=GLOBALS->yytext_vcd_recoder_c_3;
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

if(!GLOBALS->var_prevch_vcd_recoder_c_3)
	{
	for(;;)
		{
		ch=getch_patched();
		if(ch<0) { free_2(GLOBALS->varsplit_vcd_recoder_c_3); GLOBALS->varsplit_vcd_recoder_c_3=NULL; return(V_END); }
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=GLOBALS->var_prevch_vcd_recoder_c_3;
	GLOBALS->var_prevch_vcd_recoder_c_3=0;
	}

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(GLOBALS->yytext_vcd_recoder_c_3[len++]=ch;;GLOBALS->yytext_vcd_recoder_c_3[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_recoder_c_3)
		{
		GLOBALS->yytext_vcd_recoder_c_3=(char *)realloc_2(GLOBALS->yytext_vcd_recoder_c_3, (GLOBALS->T_MAX_STR_vcd_recoder_c_3=GLOBALS->T_MAX_STR_vcd_recoder_c_3*2)+1);
		}
	ch=getch_patched();
	if(ch<0) { free_2(GLOBALS->varsplit_vcd_recoder_c_3); GLOBALS->varsplit_vcd_recoder_c_3=NULL; break; }
	if((ch==':')||(ch==']'))
		{
		GLOBALS->var_prevch_vcd_recoder_c_3=ch;
		break;
		}
	}
GLOBALS->yytext_vcd_recoder_c_3[len]=0;	/* terminator */

if(match_kw)
	{
	int vr = vcd_keyword_code(GLOBALS->yytext_vcd_recoder_c_3, len);
	if(vr != V_STRING)
		{
		if(ch<0) { free_2(GLOBALS->varsplit_vcd_recoder_c_3); GLOBALS->varsplit_vcd_recoder_c_3=NULL; }
		return(vr);
		}
	}

GLOBALS->yylen_vcd_recoder_c_3=len;
if(ch<0) { free_2(GLOBALS->varsplit_vcd_recoder_c_3); GLOBALS->varsplit_vcd_recoder_c_3=NULL; }
return(V_STRING);
}

static int get_vartoken(int match_kw)
{
int ch;
int len=0;

if(GLOBALS->varsplit_vcd_recoder_c_3)
	{
	int rc=get_vartoken_patched(match_kw);
	if(rc!=V_END) return(rc);
	GLOBALS->var_prevch_vcd_recoder_c_3=0;
	}

if(!GLOBALS->var_prevch_vcd_recoder_c_3)
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
	ch=GLOBALS->var_prevch_vcd_recoder_c_3;
	GLOBALS->var_prevch_vcd_recoder_c_3=0;
	}

if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

if(ch=='#')	/* for MTI System Verilog '$var reg 64 >w #implicit-var###VarElem:ram_di[0.0] [63:0] $end' style declarations */
	{	/* debussy simply escapes until the space */
	GLOBALS->yytext_vcd_recoder_c_3[len++]= '\\';
	}

for(GLOBALS->yytext_vcd_recoder_c_3[len++]=ch;;GLOBALS->yytext_vcd_recoder_c_3[len++]=ch)
	{
	if(len==GLOBALS->T_MAX_STR_vcd_recoder_c_3)
		{
		GLOBALS->yytext_vcd_recoder_c_3=(char *)realloc_2(GLOBALS->yytext_vcd_recoder_c_3, (GLOBALS->T_MAX_STR_vcd_recoder_c_3=GLOBALS->T_MAX_STR_vcd_recoder_c_3*2)+1);
		}

	ch=getch();
	if(ch==' ')
		{
		if(match_kw) break;
		if(getch_peek() == '[')
			{
			ch = getch();
			GLOBALS->varsplit_vcd_recoder_c_3=GLOBALS->yytext_vcd_recoder_c_3+len;	/* keep looping so we get the *last* one */
			continue;
			}
		}

	if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
	if((ch=='[')&&(GLOBALS->yytext_vcd_recoder_c_3[0]!='\\'))
		{
		GLOBALS->varsplit_vcd_recoder_c_3=GLOBALS->yytext_vcd_recoder_c_3+len;		/* keep looping so we get the *last* one */
		}
	else
	if(((ch==':')||(ch==']'))&&(!GLOBALS->varsplit_vcd_recoder_c_3)&&(GLOBALS->yytext_vcd_recoder_c_3[0]!='\\'))
		{
		GLOBALS->var_prevch_vcd_recoder_c_3=ch;
		break;
		}
	}

GLOBALS->yytext_vcd_recoder_c_3[len]=0;	/* absolute terminator */
if((GLOBALS->varsplit_vcd_recoder_c_3)&&(GLOBALS->yytext_vcd_recoder_c_3[len-1]==']'))
	{
	char *vst;
	vst=malloc_2(strlen(GLOBALS->varsplit_vcd_recoder_c_3)+1);
	strcpy(vst, GLOBALS->varsplit_vcd_recoder_c_3);

	*GLOBALS->varsplit_vcd_recoder_c_3=0x00;		/* zero out var name at the left bracket */
	len=GLOBALS->varsplit_vcd_recoder_c_3-GLOBALS->yytext_vcd_recoder_c_3;

	GLOBALS->varsplit_vcd_recoder_c_3=GLOBALS->vsplitcurr_vcd_recoder_c_3=vst;
	GLOBALS->var_prevch_vcd_recoder_c_3=0;
	}
	else
	{
	GLOBALS->varsplit_vcd_recoder_c_3=NULL;
	}

if(match_kw)
	{
        int vr = vcd_keyword_code(GLOBALS->yytext_vcd_recoder_c_3, len);
        if(vr != V_STRING)
		{
		return(vr);
		}
	}

GLOBALS->yylen_vcd_recoder_c_3=len;
return(V_STRING);
}

static int get_strtoken(void)
{
int ch;
int len=0;

if(!GLOBALS->var_prevch_vcd_recoder_c_3)
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
      ch=GLOBALS->var_prevch_vcd_recoder_c_3;
      GLOBALS->var_prevch_vcd_recoder_c_3=0;
      }

for(GLOBALS->yytext_vcd_recoder_c_3[len++]=ch;;GLOBALS->yytext_vcd_recoder_c_3[len++]=ch)
      {
	if(len==GLOBALS->T_MAX_STR_vcd_recoder_c_3)
		{
		GLOBALS->yytext_vcd_recoder_c_3=(char *)realloc_2(GLOBALS->yytext_vcd_recoder_c_3, (GLOBALS->T_MAX_STR_vcd_recoder_c_3=GLOBALS->T_MAX_STR_vcd_recoder_c_3*2)+1);
		}
      ch=getch();
      if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
      }
GLOBALS->yytext_vcd_recoder_c_3[len]=0;        /* terminator */

GLOBALS->yylen_vcd_recoder_c_3=len;
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
	if(hdr) { DEBUG(fprintf(stderr," %s",GLOBALS->yytext_vcd_recoder_c_3)); }
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
	if(hdr) { DEBUG(fprintf(stderr," %s",GLOBALS->yytext_vcd_recoder_c_3)); }
	if(strstr(GLOBALS->yytext_vcd_recoder_c_3, "Icarus"))	/* turn off autocoalesce for Icarus */
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
unsigned char typ;

switch((typ = GLOBALS->yytext_vcd_recoder_c_3[0]))
	{
	/* encode bits as (time delta<<4) + (enum AnalyzerBits value) */
        case '0':
        case '1':
        case 'x': case 'X':
        case 'z': case 'Z':
        case 'h': case 'H':
        case 'u': case 'U':
        case 'w': case 'W':
        case 'l': case 'L':
        case '-':
                if(GLOBALS->yylen_vcd_recoder_c_3>1)
                        {
                        v=bsearch_vcd(GLOBALS->yytext_vcd_recoder_c_3+1, GLOBALS->yylen_vcd_recoder_c_3-1);
                        if(!v)
                                {
                                fprintf(stderr,"Near byte %d, Unknown VCD identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)),GLOBALS->yytext_vcd_recoder_c_3+1);
				malform_eof_fix();
                                }
                                else
                                {
				nptr n = v->narray[0];
				unsigned int time_delta;
				unsigned int rcv;

				if(!n->mv.mvlfac_vlist) /* overloaded for vlist, numhist = last position used */
					{
					n->mv.mvlfac_vlist = (GLOBALS->vlist_prepack) ? ((struct vlist_t *)vlist_packer_create()) : vlist_create(sizeof(char));
					vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)'0'); /* represents single bit routine for decompression */
					vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
					}

				time_delta = GLOBALS->time_vlist_count_vcd_recoder_c_1 - (unsigned int)n->numhist;
				n->numhist = GLOBALS->time_vlist_count_vcd_recoder_c_1;

				switch(GLOBALS->yytext_vcd_recoder_c_3[0])
				        {
				        case '0':
				        case '1':		rcv = ((GLOBALS->yytext_vcd_recoder_c_3[0]&1)<<1) | (time_delta<<2);
								break; /* pack more delta bits in for 0/1 vchs */

				        case 'x': case 'X':	rcv = RCV_X | (time_delta<<4); break;
				        case 'z': case 'Z':	rcv = RCV_Z | (time_delta<<4); break;
				        case 'h': case 'H':	rcv = RCV_H | (time_delta<<4); break;
				        case 'u': case 'U':	rcv = RCV_U | (time_delta<<4); break;
				        case 'w': case 'W':     rcv = RCV_W | (time_delta<<4); break;
				        case 'l': case 'L':	rcv = RCV_L | (time_delta<<4); break;
					default:		rcv = RCV_D | (time_delta<<4); break;
					}

				vlist_emit_uv32(&n->mv.mvlfac_vlist, rcv);
                                }
                        }
                        else
                        {
                        fprintf(stderr,"Near byte %d, Malformed VCD identifier\n", (int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)));
			malform_eof_fix();
                        }
                break;

	/* encode everything else literally as a time delta + a string */
#ifndef STRICT_VCD_ONLY
        case 's':
        case 'S':
                vector=wave_alloca(GLOBALS->yylen_cache_vcd_recoder_c_3=GLOBALS->yylen_vcd_recoder_c_3);
		vlen = fstUtilityEscToBin((unsigned char *)vector, (unsigned char *)(GLOBALS->yytext_vcd_recoder_c_3+1), GLOBALS->yylen_vcd_recoder_c_3-1);
		vector[vlen] = 0;

                get_strtoken();
		goto process_binary;
#endif
        case 'b':
        case 'B':
        case 'r':
        case 'R':
                vector=wave_alloca(GLOBALS->yylen_cache_vcd_recoder_c_3=GLOBALS->yylen_vcd_recoder_c_3);
                strcpy(vector,GLOBALS->yytext_vcd_recoder_c_3+1);
                vlen=GLOBALS->yylen_vcd_recoder_c_3-1;

                get_strtoken();
process_binary:
                v=bsearch_vcd(GLOBALS->yytext_vcd_recoder_c_3, GLOBALS->yylen_vcd_recoder_c_3);
                if(!v)
			{
                        fprintf(stderr,"Near byte %d, Unknown VCD identifier: '%s'\n",(int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)),GLOBALS->yytext_vcd_recoder_c_3+1);
			malform_eof_fix();
                        }
                        else
                        {
			nptr n = v->narray[0];
			unsigned int time_delta;

			if(!n->mv.mvlfac_vlist) /* overloaded for vlist, numhist = last position used */
				{
				unsigned char typ2 = toupper(typ);
				n->mv.mvlfac_vlist = (GLOBALS->vlist_prepack) ? ((struct vlist_t *)vlist_packer_create()) : vlist_create(sizeof(char));

				if((v->vartype!=V_REAL) && (v->vartype!=V_STRINGTYPE))
					{
                                        if((typ2=='R')||(typ2=='S'))
                                                {
                                                typ2 = 'B';     /* ok, typical case...fix as 'r' on bits variable causes recoder crash during trace extraction */
                                                }
					}
					else
					{
					if(typ2=='B')
						{
						typ2 = 'S';	/* should never be necessary...this is defensive */
						}
					}


				vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)toupper(typ2)); /* B/R/P/S for decompress */
				vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->vartype);
				vlist_emit_uv32(&n->mv.mvlfac_vlist, (unsigned int)v->size);
				}

			time_delta = GLOBALS->time_vlist_count_vcd_recoder_c_1 - (unsigned int)n->numhist;
			n->numhist = GLOBALS->time_vlist_count_vcd_recoder_c_1;

			vlist_emit_uv32(&n->mv.mvlfac_vlist, time_delta);

			if((typ=='b')||(typ=='B'))
				{
				if((v->vartype!=V_REAL)&&(v->vartype!=V_STRINGTYPE))
					{
					vlist_emit_mvl9_string(&n->mv.mvlfac_vlist, vector);
					}
					else
					{
					vlist_emit_string(&n->mv.mvlfac_vlist, vector);
					}
				}
				else
				{
				if((v->vartype == V_REAL)||(v->vartype == V_STRINGTYPE)||(typ=='s')||(typ=='S'))
					{
					vlist_emit_string(&n->mv.mvlfac_vlist, vector);
					}
					else
					{
					char *bits = wave_alloca(v->size + 1);
					int i, j, k=0;

					memset(bits, 0x0, v->size + 1);

					for(i=0;i<vlen;i++)
						{
						for(j=0;j<8;j++)
							{
							bits[k++] = ((vector[i] >> (7-j)) & 1) | '0';
							if(k >= v->size) goto bit_term;
							}
						}

					bit_term:
					vlist_emit_mvl9_string(&n->mv.mvlfac_vlist, bits);
					}
				}
                        }
		break;

	case 'p':
	case 'P':
		/* extract port dump value.. */
		vector=wave_alloca(GLOBALS->yylen_cache_vcd_recoder_c_3=GLOBALS->yylen_vcd_recoder_c_3);
		evcd_strcpy(vector,GLOBALS->yytext_vcd_recoder_c_3+1);	/* convert to regular vcd */
		vlen=GLOBALS->yylen_vcd_recoder_c_3-1;

		get_strtoken();	/* throw away 0_strength_component */
		get_strtoken(); /* throw away 0_strength_component */
		get_strtoken(); /* this is the id                  */

		typ = 'b';			/* convert to regular vcd */
		goto process_binary; /* store string literally */

	default:
		break;
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
			GLOBALS->global_time_offset=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);

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
			fractional_timescale_fix(GLOBALS->yytext_vcd_recoder_c_3);
			GLOBALS->time_scale=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
			if(!GLOBALS->time_scale) GLOBALS->time_scale=1;
			for(i=0;i<GLOBALS->yylen_vcd_recoder_c_3;i++)
				{
				if((GLOBALS->yytext_vcd_recoder_c_3[i]<'0')||(GLOBALS->yytext_vcd_recoder_c_3[i]>'9'))
					{
					prefix=GLOBALS->yytext_vcd_recoder_c_3[i];
					break;
					}
				}
			if(prefix==' ')
				{
				vtok=get_token();
				if((vtok==T_END)||(vtok==T_EOF)) break;
				prefix=GLOBALS->yytext_vcd_recoder_c_3[0];
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
				switch(GLOBALS->yytext_vcd_recoder_c_3[0])
					{
					case 'm':	ttype = TREE_VCD_ST_MODULE; break;
					case 't':	ttype = TREE_VCD_ST_TASK; break;
					case 'f':	ttype = (GLOBALS->yytext_vcd_recoder_c_3[1] == 'u') ? TREE_VCD_ST_FUNCTION : TREE_VCD_ST_FORK; break;
					case 'b':	ttype = TREE_VCD_ST_BEGIN; break;
					case 'g':       ttype = TREE_VCD_ST_GENERATE; break;
					case 's':       ttype = TREE_VCD_ST_STRUCT; break;
					case 'u':       ttype = TREE_VCD_ST_UNION; break;
					case 'c':       ttype = TREE_VCD_ST_CLASS; break;
					case 'i':       ttype = TREE_VCD_ST_INTERFACE; break;
					case 'p':       ttype = (GLOBALS->yytext_vcd_recoder_c_3[1] == 'r') ? TREE_VCD_ST_PROGRAM : TREE_VCD_ST_PACKAGE; break;

					case 'v':	{
							char *vht = GLOBALS->yytext_vcd_recoder_c_3;
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

					default:	ttype = TREE_UNKNOWN;
							break;
					}
				}
			T_GET;
			if(tok==T_STRING)
				{
				struct slist *s;
				s=(struct slist *)calloc_2(1,sizeof(struct slist));
				s->len=GLOBALS->yylen_vcd_recoder_c_3;
				s->str=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+1);
				strcpy(s->str, GLOBALS->yytext_vcd_recoder_c_3);
				s->mod_tree_parent = GLOBALS->mod_tree_parent;

				allocate_and_decorate_module_tree_node(ttype, GLOBALS->yytext_vcd_recoder_c_3, NULL, GLOBALS->yylen_vcd_recoder_c_3, 0, 0, 0);

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
			if((GLOBALS->header_over_vcd_recoder_c_3)&&(0))
			{
			fprintf(stderr,"$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, exiting.\n",
				(int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)));
			vcd_exit(255);
			}
			else
			{
			int vtok;
			struct vcdsymbol *v=NULL;

			GLOBALS->var_prevch_vcd_recoder_c_3=0;
                        if(GLOBALS->varsplit_vcd_recoder_c_3)
                                {
                                free_2(GLOBALS->varsplit_vcd_recoder_c_3);
                                GLOBALS->varsplit_vcd_recoder_c_3=NULL;
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
					v->size=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
					if(!v->size) v->size=1;
					}
					else
					if(vtok==V_LB)
					{
					vtok=get_vartoken(1);
					if(vtok==V_END) goto err;
					if(vtok!=V_STRING) goto err;
					v->msi=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
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
						v->lsi=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
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
				v->id=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+1);
				strcpy(v->id, GLOBALS->yytext_vcd_recoder_c_3);
                                v->nid=vcdid_hash(GLOBALS->yytext_vcd_recoder_c_3,GLOBALS->yylen_vcd_recoder_c_3);

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

                                if(v->nid < GLOBALS->vcd_minid_vcd_recoder_c_3) GLOBALS->vcd_minid_vcd_recoder_c_3 = v->nid;
                                if(v->nid > GLOBALS->vcd_maxid_vcd_recoder_c_3) GLOBALS->vcd_maxid_vcd_recoder_c_3 = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				if(GLOBALS->slisthier_len)
					{
					v->name=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_recoder_c_3+1);
					strcpy(v->name,GLOBALS->slisthier);
					strcpy(v->name+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_recoder_c_3,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_recoder_c_3)) && (GLOBALS->yytext_vcd_recoder_c_3[0] != '\\'))
							{
							char *sd=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_recoder_c_3+2);
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
					v->name=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+1);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,GLOBALS->yytext_vcd_recoder_c_3,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name,GLOBALS->yytext_vcd_recoder_c_3)) && (GLOBALS->yytext_vcd_recoder_c_3[0] != '\\'))
							{
							char *sd=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+2);
							sd[0] = '\\';
							strcpy(sd+1,v->name);
							free_2(v->name);
							v->name = sd;
							}
						}
					}

                                if(GLOBALS->pv_vcd_recoder_c_3)
                                        {
                                        if(!strcmp(GLOBALS->prev_hier_uncompressed_name,v->name) && !disable_autocoalesce && (!strchr(v->name, '\\')))
                                                {
                                                GLOBALS->pv_vcd_recoder_c_3->chain=v;
                                                v->root=GLOBALS->rootv_vcd_recoder_c_3;
                                                if(GLOBALS->pv_vcd_recoder_c_3==GLOBALS->rootv_vcd_recoder_c_3) GLOBALS->pv_vcd_recoder_c_3->root=GLOBALS->rootv_vcd_recoder_c_3;
                                                }
                                                else
                                                {
                                                GLOBALS->rootv_vcd_recoder_c_3=v;
                                                }

					free_2(GLOBALS->prev_hier_uncompressed_name);
                                        }
					else
					{
					GLOBALS->rootv_vcd_recoder_c_3=v;
					}

                                GLOBALS->pv_vcd_recoder_c_3=v;
				GLOBALS->prev_hier_uncompressed_name = strdup_2(v->name);
				}
				else	/* regular vcd var, not an evcd port var */
				{
				vtok=get_vartoken(1);
				if(vtok==V_END) goto err;
				v->size=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+1);
				strcpy(v->id, GLOBALS->yytext_vcd_recoder_c_3);
                                v->nid=vcdid_hash(GLOBALS->yytext_vcd_recoder_c_3,GLOBALS->yylen_vcd_recoder_c_3);

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

                                if(v->nid < GLOBALS->vcd_minid_vcd_recoder_c_3) GLOBALS->vcd_minid_vcd_recoder_c_3 = v->nid;
                                if(v->nid > GLOBALS->vcd_maxid_vcd_recoder_c_3) GLOBALS->vcd_maxid_vcd_recoder_c_3 = v->nid;

				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;

				if(GLOBALS->slisthier_len)
					{
					v->name=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_recoder_c_3+1);
					strcpy(v->name,GLOBALS->slisthier);
					strcpy(v->name+GLOBALS->slisthier_len,GLOBALS->vcd_hier_delimeter);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_recoder_c_3,GLOBALS->alt_hier_delimeter);
						}
						else
						{
						if((strcpy_delimfix(v->name+GLOBALS->slisthier_len+1,GLOBALS->yytext_vcd_recoder_c_3)) && (GLOBALS->yytext_vcd_recoder_c_3[0] != '\\'))
							{
                                                        char *sd=(char *)malloc_2(GLOBALS->slisthier_len+1+GLOBALS->yylen_vcd_recoder_c_3+2);
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
					v->name=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+1);
					if(GLOBALS->alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,GLOBALS->yytext_vcd_recoder_c_3,GLOBALS->alt_hier_delimeter);
						}
						else
						{
                                                if((strcpy_delimfix(v->name,GLOBALS->yytext_vcd_recoder_c_3)) && (GLOBALS->yytext_vcd_recoder_c_3[0] != '\\'))
                                                        {
                                                        char *sd=(char *)malloc_2(GLOBALS->yylen_vcd_recoder_c_3+2);
                                                        sd[0] = '\\';
                                                        strcpy(sd+1,v->name);
                                                        free_2(v->name);
                                                        v->name = sd;
                                                        }
						}
					}

                                if(GLOBALS->pv_vcd_recoder_c_3)
                                        {
                                        if(!strcmp(GLOBALS->prev_hier_uncompressed_name,v->name))
                                                {
                                                GLOBALS->pv_vcd_recoder_c_3->chain=v;
                                                v->root=GLOBALS->rootv_vcd_recoder_c_3;
                                                if(GLOBALS->pv_vcd_recoder_c_3==GLOBALS->rootv_vcd_recoder_c_3) GLOBALS->pv_vcd_recoder_c_3->root=GLOBALS->rootv_vcd_recoder_c_3;
                                                }
                                                else
                                                {
                                                GLOBALS->rootv_vcd_recoder_c_3=v;
                                                }

					free_2(GLOBALS->prev_hier_uncompressed_name);
                                        }
					else
					{
					GLOBALS->rootv_vcd_recoder_c_3=v;
					}
                                GLOBALS->pv_vcd_recoder_c_3=v;
				GLOBALS->prev_hier_uncompressed_name = strdup_2(v->name);

				vtok=get_vartoken(1);
				if(vtok==V_END) goto dumpv;
				if(vtok!=V_LB) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->msi=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
				vtok=get_vartoken(0);
				if(vtok==V_RB)
					{
					v->lsi=v->msi;
					goto dumpv;
					}
				if(vtok!=V_COLON) goto err;
				vtok=get_vartoken(0);
				if(vtok!=V_STRING) goto err;
				v->lsi=atoi_64(GLOBALS->yytext_vcd_recoder_c_3);
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


			if((v->vartype==V_REAL)||(v->vartype==V_STRINGTYPE))
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
			v->narray=(struct Node **)calloc_2(1,sizeof(struct Node *));
			v->narray[0]=(struct Node *)calloc_2(1,sizeof(struct Node));
			v->narray[0]->head.time=-1;
			v->narray[0]->head.v.h_val=AN_X;

			if(!GLOBALS->vcdsymroot_vcd_recoder_c_3)
				{
				GLOBALS->vcdsymroot_vcd_recoder_c_3=GLOBALS->vcdsymcurr_vcd_recoder_c_3=v;
				}
				else
				{
				GLOBALS->vcdsymcurr_vcd_recoder_c_3->next=v;
				GLOBALS->vcdsymcurr_vcd_recoder_c_3=v;
				}
			GLOBALS->numsyms_vcd_recoder_c_3++;

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
					fprintf(GLOBALS->vcd_save_handle,"%s[%d:%d]\n",v->name,v->msi,v->lsi);
					}
				}

			goto bail;
			err:
			if(v)
				{
				GLOBALS->error_count_vcd_recoder_c_3++;
				if(v->name)
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered with '%s'\n", (int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)), v->name);
					free_2(v->name);
					}
					else
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered\n", (int)(GLOBALS->vcdbyteno_vcd_recoder_c_3+(GLOBALS->vst_vcd_recoder_c_3-GLOBALS->vcdbuf_vcd_recoder_c_3)));
					}
				if(v->id) free_2(v->id);
				free_2(v); v=NULL;
				GLOBALS->pv_vcd_recoder_c_3 = NULL;
				}

			bail:
			if(vtok!=V_END) sync_end(NULL);
			break;
			}
		case T_ENDDEFINITIONS:
			GLOBALS->header_over_vcd_recoder_c_3=1;	/* do symbol table management here */
			create_sorted_table();
			if((!GLOBALS->sorted_vcd_recoder_c_3)&&(!GLOBALS->indexed_vcd_recoder_c_3))
				{
				fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
				vcd_exit(255);
				}
			if(GLOBALS->error_count_vcd_recoder_c_3)
				{
				fprintf(stderr, "\n%d VCD parse errors encountered, exiting.\n", GLOBALS->error_count_vcd_recoder_c_3);
				vcd_exit(255);
				}

			if(GLOBALS->use_fastload == VCD_FSL_READ)
				{
				read_fastload_body();
				fprintf(stderr, "VCDLOAD | Using fastload file.\n");
				return;
				}

			break;
		case T_STRING:
			if(!GLOBALS->header_over_vcd_recoder_c_3)
				{
				GLOBALS->header_over_vcd_recoder_c_3=1;	/* do symbol table management here */
				create_sorted_table();
				if((!GLOBALS->sorted_vcd_recoder_c_3)&&(!GLOBALS->indexed_vcd_recoder_c_3)) break;
				}
				{
				/* catchall for events when header over */
				if(GLOBALS->yytext_vcd_recoder_c_3[0]=='#')
					{
					TimeType tim;
					TimeType *tt;

					tim=atoi_64(GLOBALS->yytext_vcd_recoder_c_3+1);

					if(GLOBALS->start_time_vcd_recoder_c_3<0)
						{
						GLOBALS->start_time_vcd_recoder_c_3=tim;

						if(GLOBALS->time_vlist_vcd_recoder_write)
							{
							vlist_packer_emit_utt((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, tim);
							}
						}
						else
						{
/* backtracking fix */
						if(tim < GLOBALS->current_time_vcd_recoder_c_3)
							{
							if(!GLOBALS->vcd_already_backtracked)
								{
								GLOBALS->vcd_already_backtracked = 1;
								fprintf(stderr, "VCDLOAD | Time backtracking detected in VCD file!\n");
								}
							}
#if 0
						if(tim < GLOBALS->current_time_vcd_recoder_c_3) /* avoid backtracking time counts which can happen on malformed files */
							{
							tim = GLOBALS->current_time_vcd_recoder_c_3;
							}
#endif

						if(GLOBALS->time_vlist_vcd_recoder_write)
							{
							vlist_packer_emit_utt((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, tim - GLOBALS->current_time_vcd_recoder_c_3);
							}
						}

					GLOBALS->current_time_vcd_recoder_c_3=tim;
					if(GLOBALS->end_time_vcd_recoder_c_3<tim) GLOBALS->end_time_vcd_recoder_c_3=tim;	/* in case of malformed vcd files */
					DEBUG(fprintf(stderr,"#"TTFormat"\n",tim));

					tt = vlist_alloc(&GLOBALS->time_vlist_vcd_recoder_c_1, 0);
					*tt = tim;
					GLOBALS->time_vlist_count_vcd_recoder_c_1++;
					}
					else
					{
					if(GLOBALS->time_vlist_count_vcd_recoder_c_1)
						{
						/* OK, otherwise fix for System C which doesn't emit time zero... */
						}
						else
						{
						TimeType tim = LLDescriptor(0);
						TimeType *tt;

						GLOBALS->start_time_vcd_recoder_c_3=GLOBALS->current_time_vcd_recoder_c_3=GLOBALS->end_time_vcd_recoder_c_3=tim;

						if(GLOBALS->time_vlist_vcd_recoder_write)
							{
							vlist_packer_emit_utt((struct vlist_packer_t **)(void *)&GLOBALS->time_vlist_vcd_recoder_write, tim);
							}

						tt = vlist_alloc(&GLOBALS->time_vlist_vcd_recoder_c_1, 0);
						*tt = tim;
						GLOBALS->time_vlist_count_vcd_recoder_c_1=1;
						}
					parse_valuechange();
					}
				}
			break;
		case T_DUMPALL:	/* dump commands modify vals anyway so */
		case T_DUMPPORTSALL:
			break;	/* just loop through..                 */
		case T_DUMPOFF:
		case T_DUMPPORTSOFF:
			GLOBALS->dumping_off_vcd_recoder_c_3=1;
                        /* if((!GLOBALS->blackout_regions)||((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart<=GLOBALS->blackout_regions->bend))) : remove redundant condition */
                        if((!GLOBALS->blackout_regions)||(GLOBALS->blackout_regions->bstart<=GLOBALS->blackout_regions->bend))
				{
				struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));

				bt->bstart = GLOBALS->current_time_vcd_recoder_c_3;
				bt->next = GLOBALS->blackout_regions;
				GLOBALS->blackout_regions = bt;
				}
			break;
		case T_DUMPON:
		case T_DUMPPORTSON:
			GLOBALS->dumping_off_vcd_recoder_c_3=0;
			if((GLOBALS->blackout_regions)&&(GLOBALS->blackout_regions->bstart>GLOBALS->blackout_regions->bend))
				{
				GLOBALS->blackout_regions->bend = GLOBALS->current_time_vcd_recoder_c_3;
				}
			break;
		case T_DUMPVARS:
		case T_DUMPPORTS:
			if(GLOBALS->current_time_vcd_recoder_c_3<0)
				{ GLOBALS->start_time_vcd_recoder_c_3=GLOBALS->current_time_vcd_recoder_c_3=GLOBALS->end_time_vcd_recoder_c_3=0; }
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
				GLOBALS->blackout_regions->bend = GLOBALS->current_time_vcd_recoder_c_3;
				}

			GLOBALS->pv_vcd_recoder_c_3 = NULL;
			if(GLOBALS->prev_hier_uncompressed_name) { free_2(GLOBALS->prev_hier_uncompressed_name); GLOBALS->prev_hier_uncompressed_name = NULL; }

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

	if((n->curr->v.h_val!=heval)||(tim==GLOBALS->start_time_vcd_recoder_c_3)||(n->vartype==ND_VCD_EVENT)||(GLOBALS->vcd_preserve_glitches)) /* same region == go skip */
        	{
		if(n->curr->time==tim)
			{
			DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
				tim, n, AN_STR[n->curr->v.h_val], ch));
			n->curr->v.h_val=heval;		/* we have a glitch! */

			GLOBALS->num_glitches_vcd_recoder_c_4++;
			if(!(n->curr->flags&HIST_GLITCH))
				{
				n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
				GLOBALS->num_glitch_regions_vcd_recoder_c_4++;
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

			if(n->curr->time==tim)
				{
				DEBUG(printf("Warning: String Glitch at time ["TTFormat"] Signal [%p].\n",
					tim, n));
				if(n->curr->v.h_vector) free_2(n->curr->v.h_vector);
				n->curr->v.h_vector=vector;		/* we have a glitch! */

				GLOBALS->num_glitches_vcd_recoder_c_4++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_recoder_c_4++;
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
			||(tim==GLOBALS->start_time_vcd_recoder_c_3)
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
				GLOBALS->num_glitches_vcd_recoder_c_4++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_recoder_c_4++;
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
			||(tim==GLOBALS->start_time_vcd_recoder_c_3)
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

				GLOBALS->num_glitches_vcd_recoder_c_4++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					GLOBALS->num_glitch_regions_vcd_recoder_c_4++;
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

v=GLOBALS->vcdsymroot_vcd_recoder_c_3;
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

		if((v->size==1)&&(v->vartype!=V_REAL)&&(v->vartype!=V_STRINGTYPE))
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
						/* n->head=n2->head; */
						/* n->curr=n2->curr; */
						n->curr=(hptr)n2;
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
					/* n->head=n2->head; */
					/* n->curr=n2->curr; */
					n->curr=(hptr)n2;
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
struct vcdsymbol *v, *vt;

if(GLOBALS->indexed_vcd_recoder_c_3)
	{
	free_2(GLOBALS->indexed_vcd_recoder_c_3); GLOBALS->indexed_vcd_recoder_c_3=NULL;
	}

if(GLOBALS->sorted_vcd_recoder_c_3)
	{
	free_2(GLOBALS->sorted_vcd_recoder_c_3); GLOBALS->sorted_vcd_recoder_c_3=NULL;
	}

v=GLOBALS->vcdsymroot_vcd_recoder_c_3;
while(v)
	{
	if(v->name) free_2(v->name);
	if(v->id) free_2(v->id);
	if(v->narray) free_2(v->narray);
	vt=v;
	v=v->next;
	free_2(vt);
	}
GLOBALS->vcdsymroot_vcd_recoder_c_3=GLOBALS->vcdsymcurr_vcd_recoder_c_3=NULL;

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

if(GLOBALS->vcd_is_compressed_vcd_recoder_c_2)
	{
	pclose(GLOBALS->vcd_handle_vcd_recoder_c_2);
	GLOBALS->vcd_handle_vcd_recoder_c_2 = NULL;
	}
	else
	{
	fclose(GLOBALS->vcd_handle_vcd_recoder_c_2);
	GLOBALS->vcd_handle_vcd_recoder_c_2 = NULL;
	}

if(GLOBALS->yytext_vcd_recoder_c_3)
	{
	free_2(GLOBALS->yytext_vcd_recoder_c_3);
	GLOBALS->yytext_vcd_recoder_c_3=NULL;
	}
}

/*******************************************************************************/

TimeType vcd_recoder_main(char *fname)
{
unsigned int finalize_cnt = 0;
#ifdef HAVE_SYS_STAT_H
struct stat mystat;
int stat_rc = stat(fname, &mystat);
#endif

GLOBALS->pv_vcd_recoder_c_3=GLOBALS->rootv_vcd_recoder_c_3=NULL;
GLOBALS->vcd_hier_delimeter[0]=GLOBALS->hier_delimeter;

if(GLOBALS->use_fastload)
	{
        char *ffname = malloc_2(strlen(fname) + 4 + 1);
        sprintf(ffname, "%s.idx", fname);

        GLOBALS->vlist_handle = fopen(ffname, "rb");
	if(GLOBALS->vlist_handle)
		{
		GLOBALS->use_fastload = VCD_FSL_READ;

		/* need to do a sanity check looking for time of vcd file vs recoder file, etc. */
#ifdef HAVE_SYS_STAT_H
		if( (stat_rc) || (!read_fastload_header(&mystat)) )
#else
		if(!read_fastload_header())
#endif
			{
			GLOBALS->use_fastload = VCD_FSL_WRITE;
			fclose(GLOBALS->vlist_handle);
			GLOBALS->vlist_handle = NULL;
			}
		}
		else
		{
		GLOBALS->use_fastload = VCD_FSL_WRITE;
		}

        free_2(ffname);
        }

errno=0;	/* reset in case it's set for some reason */

GLOBALS->yytext_vcd_recoder_c_3=(char *)malloc_2(GLOBALS->T_MAX_STR_vcd_recoder_c_3+1);

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
	GLOBALS->vcd_handle_vcd_recoder_c_2=popen(str,"r");
	GLOBALS->vcd_is_compressed_vcd_recoder_c_2=~0;
	}
	else
	{
	if(strcmp("-vcd",fname))
		{
		GLOBALS->vcd_handle_vcd_recoder_c_2=fopen(fname,"rb");

		if(GLOBALS->vcd_handle_vcd_recoder_c_2)
			{
			fseeko(GLOBALS->vcd_handle_vcd_recoder_c_2, 0, SEEK_END);	/* do status bar for vcd load */
			GLOBALS->vcd_fsiz_vcd_recoder_c_2 = ftello(GLOBALS->vcd_handle_vcd_recoder_c_2);
			fseeko(GLOBALS->vcd_handle_vcd_recoder_c_2, 0, SEEK_SET);
			}

		if(GLOBALS->vcd_warning_filesize < 0) GLOBALS->vcd_warning_filesize = VCD_SIZE_WARN;

		if(GLOBALS->vcd_warning_filesize)
		if(GLOBALS->vcd_fsiz_vcd_recoder_c_2 > (GLOBALS->vcd_warning_filesize * (1024 * 1024)))
			{
			if(!GLOBALS->vlist_prepack)
				{
				fprintf(stderr, "Warning! File size is %d MB.  This might fail in recoding.\n"
					"Consider converting it to the FST database format instead.  (See the\n"
					"vcd2fst(1) manpage for more information.)\n"
					"To disable this warning, set rc variable vcd_warning_filesize to zero.\n"
					"Alternatively, use the -o, --optimize command line option to convert to FST\n"
					"or the -g, --giga command line option to use dynamically compressed memory.\n\n",
						(int)(GLOBALS->vcd_fsiz_vcd_recoder_c_2/(1024*1024)));
				}
				else
				{
				fprintf(stderr, "VCDLOAD | File size is %d MB, using vlist prepacking%s.\n\n",
						(int)(GLOBALS->vcd_fsiz_vcd_recoder_c_2/(1024*1024)),
						GLOBALS->vlist_spill_to_disk ? " and spill file" : "");
				}
			}
		}
		else
		{
		GLOBALS->splash_disable = 1;
		GLOBALS->vcd_handle_vcd_recoder_c_2=stdin;
		}
	GLOBALS->vcd_is_compressed_vcd_recoder_c_2=0;
	}

if(!GLOBALS->vcd_handle_vcd_recoder_c_2)
	{
	fprintf(stderr, "Error opening %s .vcd file '%s'.\n",
		GLOBALS->vcd_is_compressed_vcd_recoder_c_2?"compressed":"", fname);
	perror("Why");
	vcd_exit(255);
	}

/* SPLASH */				splash_create();

sym_hash_initialize(GLOBALS);
getch_alloc();		/* alloc membuff for vcd getch buffer */

build_slisthier();

GLOBALS->time_vlist_vcd_recoder_c_1 = vlist_create(sizeof(TimeType));

if(GLOBALS->use_fastload == VCD_FSL_WRITE)
	{
	GLOBALS->time_vlist_vcd_recoder_write = ((struct vlist_t *)vlist_packer_create());
	}

if((GLOBALS->vlist_spill_to_disk) && (GLOBALS->use_fastload != VCD_FSL_READ))
	{
	vlist_init_spillfile();
	}

vcd_parse();
if(GLOBALS->varsplit_vcd_recoder_c_3)
	{
        free_2(GLOBALS->varsplit_vcd_recoder_c_3);
        GLOBALS->varsplit_vcd_recoder_c_3=NULL;
        }

if(GLOBALS->vlist_handle)
	{
	FILE *vh = GLOBALS->vlist_handle;
	GLOBALS->vlist_handle = NULL;
	vlist_freeze(&GLOBALS->time_vlist_vcd_recoder_c_1);
	GLOBALS->vlist_handle = vh;
	}
	else
	{
	vlist_freeze(&GLOBALS->time_vlist_vcd_recoder_c_1);
	}

if(GLOBALS->time_vlist_vcd_recoder_write)
	{
	write_fastload_time_section();
	}

if(GLOBALS->use_fastload != VCD_FSL_READ)
	{
	finalize_cnt = vlist_emit_finalize();
	}

if(GLOBALS->time_vlist_vcd_recoder_write)
	{
#ifdef HAVE_SYS_STAT_H
	write_fastload_header(&mystat, finalize_cnt);
#else
	write_fastload_header(finalize_cnt);
#endif
	}


if((!GLOBALS->sorted_vcd_recoder_c_3)&&(!GLOBALS->indexed_vcd_recoder_c_3))
	{
	fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
	vcd_exit(255);
	}

if(GLOBALS->vcd_save_handle) { fclose(GLOBALS->vcd_save_handle); GLOBALS->vcd_save_handle = NULL; }

fprintf(stderr, "["TTFormat"] start time.\n["TTFormat"] end time.\n", GLOBALS->start_time_vcd_recoder_c_3*GLOBALS->time_scale, GLOBALS->end_time_vcd_recoder_c_3*GLOBALS->time_scale);

if(GLOBALS->vcd_fsiz_vcd_recoder_c_2)
        {
        splash_sync(GLOBALS->vcd_fsiz_vcd_recoder_c_2, GLOBALS->vcd_fsiz_vcd_recoder_c_2);
	GLOBALS->vcd_fsiz_vcd_recoder_c_2 = 0;
        }
else
if(GLOBALS->vcd_is_compressed_vcd_recoder_c_2)
	{
        splash_sync(1,1);
	GLOBALS->vcd_fsiz_vcd_recoder_c_2 = 0;
	}

GLOBALS->min_time=GLOBALS->start_time_vcd_recoder_c_3*GLOBALS->time_scale;
GLOBALS->max_time=GLOBALS->end_time_vcd_recoder_c_3*GLOBALS->time_scale;
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

/* is_vcd=~0; */
GLOBALS->is_lx2 = LXT2_IS_VLIST;

/* SPLASH */                            splash_finalize();
return(GLOBALS->max_time);
}

/*******************************************************************************/

void vcd_import_masked(void)
{
/* nothing */
}

void vcd_set_fac_process_mask(nptr np)
{
if(np && np->mv.mvlfac_vlist)
	{
	import_vcd_trace(np);
	}
}


#define vlist_locate_import(x,y) ((GLOBALS->vlist_prepack) ? ((depacked) + (y)) : vlist_locate((x),(y)))

void import_vcd_trace(nptr np)
{
struct vlist_t *v = np->mv.mvlfac_vlist;
int len = 1;
unsigned int list_size;
unsigned char vlist_type;
/* unsigned int vartype = 0; */ /* scan-build */
unsigned int vlist_pos = 0;
unsigned char *chp;
unsigned int time_idx = 0;
TimeType *curtime_pnt;
unsigned char arr[5];
int arr_pos;
unsigned int accum;
unsigned char ch;
double *d;
unsigned char *depacked = NULL;

if(!v) return;
vlist_uncompress(&v);

if(GLOBALS->vlist_prepack)
	{
	depacked = vlist_packer_decompress(v, &list_size);
	vlist_destroy(v);
	}
	else
	{
	list_size=vlist_size(v);
	}

if(!list_size)
	{
	len = 1;
	vlist_type = '!'; /* possible alias */
	}
	else
	{
	chp = vlist_locate_import(v, vlist_pos++);
	if(chp)
		{
		switch((vlist_type = (*chp & 0x7f)))
			{
			case '0':
				len = 1;
				chp = vlist_locate_import(v, vlist_pos++);
				if(!chp)
					{
					fprintf(stderr, "Internal error file '%s' line %d, exiting.\n", __FILE__, __LINE__);
					exit(255);
					}
				/* vartype = (unsigned int)(*chp & 0x7f); */ /*scan-build */
				break;

			case 'B':
			case 'R':
			case 'S':
				chp = vlist_locate_import(v, vlist_pos++);
				if(!chp)
					{
					fprintf(stderr, "Internal error file '%s' line %d, exiting.\n", __FILE__, __LINE__);
					exit(255);
					}
				/* vartype = (unsigned int)(*chp & 0x7f); */ /* scan-build */

				arr_pos = accum = 0;

		                do      {
		                	chp = vlist_locate_import(v, vlist_pos++);
		                        if(!chp) break;
		                        ch = *chp;
					arr[arr_pos++] = ch;
		                        } while (!(ch & 0x80));

				for(--arr_pos; arr_pos>=0; arr_pos--)
					{
					ch = arr[arr_pos];
					accum <<= 7;
					accum |= (unsigned int)(ch & 0x7f);
					}

				len = accum;

				break;

			default:
				fprintf(stderr, "Unsupported vlist type '%c', exiting.", vlist_type);
				vcd_exit(255);
				break;
			}
		}
		else
		{
		len = 1;
		vlist_type = '!'; /* possible alias */
		}
	}


if(vlist_type == '0') /* single bit */
	{
	while(vlist_pos < list_size)
		{
		unsigned int delta, bitval;
		char ascval;

		arr_pos = accum = 0;

                do      {
                	chp = vlist_locate_import(v, vlist_pos++);
                        if(!chp) break;
                        ch = *chp;
			arr[arr_pos++] = ch;
                        } while (!(ch & 0x80));

		for(--arr_pos; arr_pos>=0; arr_pos--)
			{
			ch = arr[arr_pos];
			accum <<= 7;
			accum |= (unsigned int)(ch & 0x7f);
			}

		if(!(accum&1))
			{
			delta = accum >> 2;
			bitval = (accum >> 1) & 1;
			ascval = '0' + bitval;
			}
			else
			{
			delta = accum >> 4;
			bitval = (accum >> 1) & 7;
			ascval = RCV_STR[bitval];
			}
		time_idx += delta;

		curtime_pnt = vlist_locate(GLOBALS->time_vlist_vcd_recoder_c_1, time_idx ? time_idx-1 : 0);
		if(!curtime_pnt)
			{
			fprintf(stderr, "GTKWAVE | malformed bitwise signal data for '%s' after time_idx = %d\n",
				np->nname, time_idx - delta);
			exit(255);
			}

		add_histent(*curtime_pnt,np,ascval,1, NULL);
		}

	add_histent(MAX_HISTENT_TIME-1, np, 'x', 0, NULL);
	add_histent(MAX_HISTENT_TIME,   np, 'z', 0, NULL);
	}
else if(vlist_type == 'B') /* bit vector, port type was converted to bit vector already */
	{
	char *sbuf = malloc_2(len+1);
	int dst_len;
	char *vector;

	while(vlist_pos < list_size)
		{
		unsigned int delta;

		arr_pos = accum = 0;

                do      {
                	chp = vlist_locate_import(v, vlist_pos++);
                        if(!chp) break;
                        ch = *chp;
			arr[arr_pos++] = ch;
                        } while (!(ch & 0x80));

		for(--arr_pos; arr_pos>=0; arr_pos--)
			{
			ch = arr[arr_pos];
			accum <<= 7;
			accum |= (unsigned int)(ch & 0x7f);
			}

		delta = accum;
		time_idx += delta;

		curtime_pnt = vlist_locate(GLOBALS->time_vlist_vcd_recoder_c_1,  time_idx ? time_idx-1 : 0);
		if(!curtime_pnt)
			{
			fprintf(stderr, "GTKWAVE | malformed 'b' signal data for '%s' after time_idx = %d\n",
				np->nname, time_idx - delta);
			exit(255);
			}

		dst_len = 0;
		for(;;)
			{
			chp = vlist_locate_import(v, vlist_pos++);
			if(!chp) break;
			ch = *chp;
			if((ch >> 4) == AN_MSK) break;
			if(dst_len == len) { if(len != 1) memmove(sbuf, sbuf+1, dst_len - 1); dst_len--; }
			sbuf[dst_len++] = AN_STR[ch >> 4];
			if((ch & AN_MSK) == AN_MSK) break;
			if(dst_len == len) { if(len != 1) memmove(sbuf, sbuf+1, dst_len - 1); dst_len--; }
			sbuf[dst_len++] = AN_STR[ch & AN_MSK];
			}

		if(len == 1)
			{
			add_histent(*curtime_pnt, np,sbuf[0],1, NULL);
			}
			else
			{
			vector = malloc_2(len+1);
			if(dst_len < len)
				{
				unsigned char extend=(sbuf[0]=='1')?'0':sbuf[0];
				memset(vector, extend, len - dst_len);
				memcpy(vector + (len - dst_len), sbuf, dst_len);
				}
				else
				{
				memcpy(vector, sbuf, len);
				}

			vector[len] = 0;
			add_histent(*curtime_pnt, np,0,1,vector);
			}
		}

	if(len==1)
		{
		add_histent(MAX_HISTENT_TIME-1, np, 'x', 0, NULL);
		add_histent(MAX_HISTENT_TIME,   np, 'z', 0, NULL);
		}
		else
		{
		add_histent(MAX_HISTENT_TIME-1, np, 'x', 0, (char *)calloc_2(1,sizeof(char)));
		add_histent(MAX_HISTENT_TIME,   np, 'z', 0, (char *)calloc_2(1,sizeof(char)));
		}

	free_2(sbuf);
	}
else if(vlist_type == 'R') /* real */
	{
	char *sbuf = malloc_2(64);
	int dst_len;
	char *vector;

	while(vlist_pos < list_size)
		{
		unsigned int delta;

		arr_pos = accum = 0;

                do      {
                	chp = vlist_locate_import(v, vlist_pos++);
                        if(!chp) break;
                        ch = *chp;
			arr[arr_pos++] = ch;
                        } while (!(ch & 0x80));

		for(--arr_pos; arr_pos>=0; arr_pos--)
			{
			ch = arr[arr_pos];
			accum <<= 7;
			accum |= (unsigned int)(ch & 0x7f);
			}

		delta = accum;
		time_idx += delta;

		curtime_pnt = vlist_locate(GLOBALS->time_vlist_vcd_recoder_c_1,  time_idx ? time_idx-1 : 0);
		if(!curtime_pnt)
			{
			fprintf(stderr, "GTKWAVE | malformed 'r' signal data for '%s' after time_idx = %d\n",
				np->nname, time_idx - delta);
			exit(255);
			}

		dst_len = 0;
		do
			{
			chp = vlist_locate_import(v, vlist_pos++);
			if(!chp) break;
			ch = *chp;
			sbuf[dst_len++] = ch;
			} while(ch);

		vector=malloc_2(sizeof(double));
		sscanf(sbuf,"%lg",(double *)vector);
		add_histent(*curtime_pnt, np,'g',1,(char *)vector);
		}

	d=malloc_2(sizeof(double));
	*d=1.0;
	add_histent(MAX_HISTENT_TIME-1, np, 'g', 0, (char *)d);

	d=malloc_2(sizeof(double));
	*d=0.0;
	add_histent(MAX_HISTENT_TIME, np, 'g', 0, (char *)d);

	free_2(sbuf);
	}
else if(vlist_type == 'S') /* string */
	{
	char *sbuf = malloc_2(list_size); /* being conservative */
	int dst_len;
	char *vector;

	while(vlist_pos < list_size)
		{
		unsigned int delta;

		arr_pos = accum = 0;

                do      {
                	chp = vlist_locate_import(v, vlist_pos++);
                        if(!chp) break;
                        ch = *chp;
			arr[arr_pos++] = ch;
                        } while (!(ch & 0x80));

		for(--arr_pos; arr_pos>=0; arr_pos--)
			{
			ch = arr[arr_pos];
			accum <<= 7;
			accum |= (unsigned int)(ch & 0x7f);
			}

		delta = accum;
		time_idx += delta;

		curtime_pnt = vlist_locate(GLOBALS->time_vlist_vcd_recoder_c_1,  time_idx ? time_idx-1 : 0);
		if(!curtime_pnt)
			{
			fprintf(stderr, "GTKWAVE | malformed 's' signal data for '%s' after time_idx = %d\n",
				np->nname, time_idx - delta);
			exit(255);
			}

		dst_len = 0;
		do
			{
			chp = vlist_locate_import(v, vlist_pos++);
			if(!chp) break;
			ch = *chp;
			sbuf[dst_len++] = ch;
			} while(ch);

		vector=malloc_2(dst_len + 1);
		strcpy(vector, sbuf);
		add_histent(*curtime_pnt, np,'s',1,(char *)vector);
		}

	d=malloc_2(sizeof(double));
	*d=1.0;
	add_histent(MAX_HISTENT_TIME-1, np, 'g', 0, (char *)d);

	d=malloc_2(sizeof(double));
	*d=0.0;
	add_histent(MAX_HISTENT_TIME, np, 'g', 0, (char *)d);

	free_2(sbuf);
	}
else if(vlist_type == '!') /* error in loading */
	{
	nptr n2 = (nptr)np->curr;

	if((n2)&&(n2 != np))	/* keep out any possible infinite recursion from corrupt pointer bugs */
		{
		import_vcd_trace(n2);

		if(GLOBALS->vlist_prepack)
			{
			vlist_packer_decompress_destroy((char *)depacked);
			}
			else
			{
			vlist_destroy(v);
			}
		np->mv.mvlfac_vlist = NULL;

		np->head = n2->head;
		np->curr = n2->curr;
		return;
		}

	fprintf(stderr, "Error in decompressing vlist for '%s', exiting.\n", np->nname);
	vcd_exit(255);
	}

if(GLOBALS->vlist_prepack)
	{
	vlist_packer_decompress_destroy((char *)depacked);
	}
	else
	{
	vlist_destroy(v);
	}
np->mv.mvlfac_vlist = NULL;
}

