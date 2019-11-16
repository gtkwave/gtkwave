/*
 * Copyright (c) Tony Bybell 2003-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <stdio.h>
#include "vzt.h"
#include "lx2.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "vcd.h"
#include "lxt.h"
#include "lxt2_read.h"
#include "vzt_read.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"

/*
 * mainline
 */
TimeType vzt_main(char *fname, char *skip_start, char *skip_end)
{
int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;
signed char scale;
unsigned int numalias = 0;
struct symbol *sym_block = NULL;
struct Node *node_block = NULL;
char **f_name = NULL;

GLOBALS->vzt_vzt_c_1 = vzt_rd_init_smp(fname, GLOBALS->num_cpus);
if(!GLOBALS->vzt_vzt_c_1)
        {
	return(LLDescriptor(0));	/* look at GLOBALS->vzt_vzt_c_1 in caller for success status... */
        }
/* SPLASH */                            splash_create();

vzt_rd_process_blocks_linearly(GLOBALS->vzt_vzt_c_1, 1);
/* vzt_rd_set_max_block_mem_usage(vzt, 0); */

scale=(signed char)vzt_rd_get_timescale(GLOBALS->vzt_vzt_c_1);
exponent_to_time_scale(scale);
GLOBALS->global_time_offset = vzt_rd_get_timezero(GLOBALS->vzt_vzt_c_1);


GLOBALS->numfacs=vzt_rd_get_num_facs(GLOBALS->vzt_vzt_c_1);
GLOBALS->mvlfacs_vzt_c_3=(struct fac *)calloc_2(GLOBALS->numfacs,sizeof(struct fac));
f_name = calloc_2(F_NAME_MODULUS+1,sizeof(char *));
GLOBALS->vzt_table_vzt_c_1=(struct lx2_entry *)calloc_2(GLOBALS->numfacs, sizeof(struct lx2_entry));
sym_block = (struct symbol *)calloc_2(GLOBALS->numfacs, sizeof(struct symbol));
node_block=(struct Node *)calloc_2(GLOBALS->numfacs,sizeof(struct Node));

for(i=0;i<GLOBALS->numfacs;i++)
	{
	GLOBALS->mvlfacs_vzt_c_3[i].node_alias=vzt_rd_get_fac_rows(GLOBALS->vzt_vzt_c_1, i);
	node_block[i].msi=vzt_rd_get_fac_msb(GLOBALS->vzt_vzt_c_1, i);
	node_block[i].lsi=vzt_rd_get_fac_lsb(GLOBALS->vzt_vzt_c_1, i);
	GLOBALS->mvlfacs_vzt_c_3[i].flags=vzt_rd_get_fac_flags(GLOBALS->vzt_vzt_c_1, i);
	GLOBALS->mvlfacs_vzt_c_3[i].len=vzt_rd_get_fac_len(GLOBALS->vzt_vzt_c_1, i);
	}

fprintf(stderr, VZT_RDLOAD"Finished building %d facs.\n", GLOBALS->numfacs);
/* SPLASH */                            splash_sync(1, 5);

GLOBALS->first_cycle_vzt_c_3 = (TimeType) vzt_rd_get_start_time(GLOBALS->vzt_vzt_c_1) * GLOBALS->time_scale;
GLOBALS->last_cycle_vzt_c_3 = (TimeType) vzt_rd_get_end_time(GLOBALS->vzt_vzt_c_1) * GLOBALS->time_scale;
GLOBALS->total_cycles_vzt_c_3 = GLOBALS->last_cycle_vzt_c_3 - GLOBALS->first_cycle_vzt_c_3 + 1;

/* do your stuff here..all useful info has been initialized by now */

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

if(GLOBALS->numfacs)
	{
	char *fnam = vzt_rd_get_facname(GLOBALS->vzt_vzt_c_1, 0);
	int flen = strlen(fnam);
	f_name[0]=malloc_2(flen+1);
	strcpy(f_name[0], fnam);
	}

for(i=0;i<GLOBALS->numfacs;i++)
        {
	char buf[65537];
	char *str;
	struct fac *f;

	if(i!=(GLOBALS->numfacs-1))
		{
		char *fnam = vzt_rd_get_facname(GLOBALS->vzt_vzt_c_1, i+1);
		int flen = strlen(fnam);
		f_name[(i+1)&F_NAME_MODULUS]=malloc_2(flen+1);
		strcpy(f_name[(i+1)&F_NAME_MODULUS], fnam);
		}

	if(i>1)
		{
		free_2(f_name[(i-2)&F_NAME_MODULUS]);
		f_name[(i-2)&F_NAME_MODULUS] = NULL;
		}

	if(GLOBALS->mvlfacs_vzt_c_3[i].flags&VZT_RD_SYM_F_ALIAS)
		{
		int alias = GLOBALS->mvlfacs_vzt_c_3[i].node_alias;
		f=GLOBALS->mvlfacs_vzt_c_3+alias;

		while(f->flags&VZT_RD_SYM_F_ALIAS)
			{
			f=GLOBALS->mvlfacs_vzt_c_3+f->node_alias;
			}

		numalias++;
		}
		else
		{
		f=GLOBALS->mvlfacs_vzt_c_3+i;
		}

	if((f->len>1)&& (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) )
		{
		int len=sprintf(buf, "%s[%d:%d]", f_name[(i)&F_NAME_MODULUS],node_block[i].msi, node_block[i].lsi);
		str=malloc_2(len+1);

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
		s=&sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		prevsymroot = prevsym = NULL;
		}
		else
		{
                int gatecmp = (f->len==1) && (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) && (node_block[i].msi!=-1) && (node_block[i].lsi!=-1);
                int revcmp = gatecmp && (i) && (!strcmp(f_name[(i)&F_NAME_MODULUS], f_name[(i-1)&F_NAME_MODULUS]));

		if(gatecmp)
			{
			int len = sprintf(buf, "%s[%d]", f_name[(i)&F_NAME_MODULUS],node_block[i].msi);
			str=malloc_2(len+1);
			if(!GLOBALS->alt_hier_delimeter)
				{
				strcpy(str, buf);
				}
				else
				{
				strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
				}
			s=&sym_block[i];
		        symadd_name_exists_sym_exists(s,str,0);
			if((prevsym)&&(revcmp)&&(!strchr(f_name[(i)&F_NAME_MODULUS], '\\')))	/* allow chaining for search functions.. */
				{
				prevsym->vec_root = prevsymroot;
				prevsym->vec_chain = s;
				s->vec_root = prevsymroot;
				prevsym = s;
				}
				else
				{
				prevsymroot = prevsym = s;
				}
			}
			else
			{
			str=malloc_2(strlen(f_name[(i)&F_NAME_MODULUS])+1);
			if(!GLOBALS->alt_hier_delimeter)
				{
				strcpy(str, f_name[(i)&F_NAME_MODULUS]);
				}
				else
				{
				strcpy_vcdalt(str, f_name[(i)&F_NAME_MODULUS], GLOBALS->alt_hier_delimeter);
				}
			s=&sym_block[i];
		        symadd_name_exists_sym_exists(s,str,0);
			prevsymroot = prevsym = NULL;

			if(f->flags&VZT_RD_SYM_F_INTEGER)
				{
				node_block[i].msi=31;
				node_block[i].lsi=0;
				GLOBALS->mvlfacs_vzt_c_3[i].len=32;
				}
			}
		}

        n=&node_block[i];
        n->nname=s->name;
        n->mv.mvlfac = GLOBALS->mvlfacs_vzt_c_3+i;
	GLOBALS->mvlfacs_vzt_c_3[i].working_node = n;

	if((f->len>1)||(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
		{
		n->extvals = 1;
		}

        n->head.time=-1;        /* mark 1st node as negative time */
        n->head.v.h_val=AN_X;
        s->n=n;
        }

for(i=0;i<=F_NAME_MODULUS;i++)
	{
	if(f_name[(i)&F_NAME_MODULUS])
		{
		free_2(f_name[(i)&F_NAME_MODULUS]);
		f_name[(i)&F_NAME_MODULUS] = NULL;
		}
	}
free_2(f_name); f_name = NULL;

/* SPLASH */                            splash_sync(2, 5);
GLOBALS->facs=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));

if(GLOBALS->fast_tree_sort)
        {
        for(i=0;i<GLOBALS->numfacs;i++)
                {
                int len;
                GLOBALS->facs[i]=&sym_block[i];
                if((len=strlen(GLOBALS->facs[i]->name))>GLOBALS->longestname) GLOBALS->longestname=len;
                }

        if(numalias)
                {
                int idx_lft = 0;
                int idx_lftmax = GLOBALS->numfacs - numalias;
                int idx_rgh = GLOBALS->numfacs - numalias;
                struct symbol **facs_merge=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));

		fprintf(stderr, VZT_RDLOAD"Merging in %d aliases.\n", numalias);

                for(i=0;i<GLOBALS->numfacs;i++)  /* fix possible tail appended aliases by remerging in partial one pass merge sort */
                        {
                        if(strcmp(GLOBALS->facs[idx_lft]->name, GLOBALS->facs[idx_rgh]->name) <= 0)
                                {
                                facs_merge[i] = GLOBALS->facs[idx_lft++];

                                if(idx_lft == idx_lftmax)
                                        {
                                        for(i++;i<GLOBALS->numfacs;i++)
                                                {
                                                facs_merge[i] = GLOBALS->facs[idx_rgh++];
                                                }
                                        }
                                }
                                else
                                {
                                facs_merge[i] = GLOBALS->facs[idx_rgh++];

                                if(idx_rgh == GLOBALS->numfacs)
                                        {
                                        for(i++;i<GLOBALS->numfacs;i++)
                                                {
                                                facs_merge[i] = GLOBALS->facs[idx_lft++];
                                                }
                                        }
                                }
                        }

                free_2(GLOBALS->facs); GLOBALS->facs = facs_merge;
                }

/* SPLASH */                            splash_sync(3, 5);
        fprintf(stderr, VZT_RDLOAD"Building facility hierarchy tree.\n");

        init_tree();
        for(i=0;i<GLOBALS->numfacs;i++)
                {
                int esc = 0;
                char *subst = GLOBALS->facs[i]->name;
                char ch;

                while((ch=(*subst)))
                        {
                        if(ch==GLOBALS->hier_delimeter) { if(esc) *subst = VCDNAM_ESCAPE; }
                        else if(ch=='\\') { esc = 1; GLOBALS->escaped_names_found_vcd_c_1 = 1; }
                        subst++;
                        }

		build_tree_from_name(GLOBALS->facs[i]->name, i);
                }
/* SPLASH */                            splash_sync(4, 5);
        if(GLOBALS->escaped_names_found_vcd_c_1)
                {
                for(i=0;i<GLOBALS->numfacs;i++)
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

        fprintf(stderr, VZT_RDLOAD"Sorting facility hierarchy tree.\n");
        treesort(GLOBALS->treeroot, NULL);
/* SPLASH */                            splash_sync(5, 5);
        order_facs_from_treesort(GLOBALS->treeroot, &GLOBALS->facs);
        if(GLOBALS->escaped_names_found_vcd_c_1)
                {
                treenamefix(GLOBALS->treeroot);
                }

        GLOBALS->facs_are_sorted=1;
        }
        else
	{
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst, ch;
		int len;
		int esc = 0;

		GLOBALS->facs[i]=&sym_block[i];
	        if((len=strlen(subst=GLOBALS->facs[i]->name))>GLOBALS->longestname) GLOBALS->longestname=len;
                while((ch=(*subst)))
                        {
#ifdef WAVE_HIERFIX
                        if(ch==GLOBALS->hier_delimeter) { *subst=(!esc) ? VCDNAM_HIERSORT : VCDNAM_ESCAPE; }    /* forces sort at hier boundaries */
#else
			if((ch==GLOBALS->hier_delimeter)&&(esc)) { *subst = VCDNAM_ESCAPE; }    /* forces sort at hier boundaries */
#endif
                        else if(ch=='\\') { esc = 1; GLOBALS->escaped_names_found_vcd_c_1 = 1; }
                        subst++;
                        }
		}

/* SPLASH */                            splash_sync(3, 5);
	fprintf(stderr, VZT_RDLOAD"Sorting facilities at hierarchy boundaries.\n");
	wave_heapsort(GLOBALS->facs,GLOBALS->numfacs);

#ifdef WAVE_HIERFIX
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst, ch;

		subst=GLOBALS->facs[i]->name;
		while((ch=(*subst)))
			{
			if(ch==VCDNAM_HIERSORT) { *subst=GLOBALS->hier_delimeter; }	/* restore back to normal */
			subst++;
			}
		}
#endif

	GLOBALS->facs_are_sorted=1;

/* SPLASH */                            splash_sync(4, 5);
	fprintf(stderr, VZT_RDLOAD"Building facility hierarchy tree.\n");

	init_tree();
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *nf = GLOBALS->facs[i]->name;
	        build_tree_from_name(nf, i);
		}
/* SPLASH */                            splash_sync(5, 5);
        if(GLOBALS->escaped_names_found_vcd_c_1)
                {
                for(i=0;i<GLOBALS->numfacs;i++)
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

GLOBALS->min_time = GLOBALS->first_cycle_vzt_c_3; GLOBALS->max_time=GLOBALS->last_cycle_vzt_c_3;
GLOBALS->is_lx2 = LXT2_IS_VZT;

if(skip_start || skip_end)
	{
	TimeType b_start, b_end;

	if(!skip_start) b_start = GLOBALS->min_time; else b_start = unformat_time(skip_start, GLOBALS->time_dimension);
	if(!skip_end) b_end = GLOBALS->max_time; else b_end = unformat_time(skip_end, GLOBALS->time_dimension);

	if(b_start<GLOBALS->min_time) b_start = GLOBALS->min_time;
	else if(b_start>GLOBALS->max_time) b_start = GLOBALS->max_time;

	if(b_end<GLOBALS->min_time) b_end = GLOBALS->min_time;
	else if(b_end>GLOBALS->max_time) b_end = GLOBALS->max_time;

        if(b_start > b_end)
                {
		TimeType tmp_time = b_start;
                b_start = b_end;
                b_end = tmp_time;
                }

	if(!vzt_rd_limit_time_range(GLOBALS->vzt_vzt_c_1, b_start, b_end))
		{
		fprintf(stderr, VZT_RDLOAD"--begin/--end options yield zero blocks, ignoring.\n");
		vzt_rd_unlimit_time_range(GLOBALS->vzt_vzt_c_1);
		}
		else
		{
		GLOBALS->min_time = b_start;
		GLOBALS->max_time = b_end;
		}
	}

/* SPLASH */				splash_finalize();
return(GLOBALS->max_time);
}


/*
 * vzt callback (only does bits for now)
 */
static void vzt_callback(struct vzt_rd_trace **lt, lxtint64_t *tim, lxtint32_t *facidx, char **value)
{
(void)lt;

struct HistEnt *htemp = histent_calloc();
struct lx2_entry *l2e = GLOBALS->vzt_table_vzt_c_1+(*facidx);
struct fac *f = GLOBALS->mvlfacs_vzt_c_3+(*facidx);


GLOBALS->busycnt_vzt_c_2++;
if(GLOBALS->busycnt_vzt_c_2==WAVE_BUSY_ITER)
	{
	busy_window_refresh();
	GLOBALS->busycnt_vzt_c_2 = 0;
	}

/* fprintf(stderr, "%lld %d %s\n", *tim, *facidx, *value); */

if(!(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
	{
	if(f->len>1)
	        {
	        htemp->v.h_vector = (char *)malloc_2(f->len);
		memcpy(htemp->v.h_vector, *value, f->len);
	        }
	        else
	        {
		switch(**value)
			{
			case '0':	htemp->v.h_val = AN_0; break;
			case '1':	htemp->v.h_val = AN_1; break;
			case 'Z':
			case 'z':	htemp->v.h_val = AN_Z; break;
			default:	htemp->v.h_val = AN_X; break;
			}
	        }
	}
else if(f->flags&VZT_RD_SYM_F_DOUBLE)
	{
#ifdef WAVE_HAS_H_DOUBLE
	sscanf(*value, "%lg", &htemp->v.h_double);
#else
	double *d = malloc_2(sizeof(double));
	sscanf(*value, "%lg", d);
	htemp->v.h_vector = (char *)d;
#endif
	htemp->flags = HIST_REAL;
	}
else	/* string */
	{
	char *s = malloc_2(strlen(*value)+1);
	strcpy(s, *value);
	htemp->v.h_vector = s;
	htemp->flags = HIST_REAL|HIST_STRING;
	}


htemp->time = (*tim) * (GLOBALS->time_scale);

if(l2e->histent_head)
	{
	l2e->histent_curr->next = htemp;
	l2e->histent_curr = htemp;
	}
	else
	{
	l2e->histent_head = l2e->histent_curr = htemp;
	}

l2e->numtrans++;
}


/*
 * this is the black magic that handles aliased signals...
 */
static void vzt_resolver(nptr np, nptr resolve)
{
np->extvals = resolve->extvals;
np->msi = resolve->msi;
np->lsi = resolve->lsi;
memcpy(&np->head, &resolve->head, sizeof(struct HistEnt));
np->curr = resolve->curr;
np->harray = resolve->harray;
np->numhist = resolve->numhist;
np->mv.mvlfac=NULL;
}



/*
 * actually import a vzt trace but don't do it if it's already been imported
 */
void import_vzt_trace(nptr np)
{
struct HistEnt *htemp, *histent_tail;
int len, i;
struct fac *f;
int txidx;
nptr nold = np;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f - GLOBALS->mvlfacs_vzt_c_3;
if(np->mv.mvlfac->flags&VZT_RD_SYM_F_ALIAS)
	{
	txidx = vzt_rd_get_alias_root(GLOBALS->vzt_vzt_c_1, txidx);
	np = GLOBALS->mvlfacs_vzt_c_3[txidx].working_node;

	if(!(f=np->mv.mvlfac))
		{
		vzt_resolver(nold, np);
		return;	/* already imported */
		}
	}

fprintf(stderr, "Import: %s\n", np->nname);

/* new stuff */
len = np->mv.mvlfac->len;

if(f->node_alias <= 1) /* sorry, arrays not supported, but vzt doesn't support them yet either */
	{
	vzt_rd_set_fac_process_mask(GLOBALS->vzt_vzt_c_1, txidx);
	vzt_rd_iter_blocks(GLOBALS->vzt_vzt_c_1, vzt_callback, NULL);
	vzt_rd_clr_fac_process_mask(GLOBALS->vzt_vzt_c_1, txidx);
	}

histent_tail = htemp = histent_calloc();
if(len>1)
	{
	htemp->v.h_vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_Z;
	}
	else
	{
	htemp->v.h_val = AN_Z;		/* z */
	}
htemp->time = MAX_HISTENT_TIME;

htemp = histent_calloc();
if(len>1)
	{
	htemp->v.h_vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_X;
	}
	else
	{
	htemp->v.h_val = AN_X;		/* x */
	}
htemp->time = MAX_HISTENT_TIME-1;
htemp->next = histent_tail;

if(GLOBALS->vzt_table_vzt_c_1[txidx].histent_curr)
	{
	GLOBALS->vzt_table_vzt_c_1[txidx].histent_curr->next = htemp;
	htemp = GLOBALS->vzt_table_vzt_c_1[txidx].histent_head;
	}

if(!(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
        {
	if(len>1)
		{
		np->head.v.h_vector = (char *)malloc_2(len);
		for(i=0;i<len;i++) np->head.v.h_vector[i] = AN_X;
		}
		else
		{
		np->head.v.h_val = AN_X;	/* x */
		}
	}
        else
        {
        np->head.flags = HIST_REAL;
        if(f->flags&VZT_RD_SYM_F_STRING) np->head.flags |= HIST_STRING;
        }

	{
        struct HistEnt *htemp2 = histent_calloc();
        htemp2->time = -1;
        if(len>1)
        	{
                htemp2->v.h_vector = htemp->v.h_vector;
                }
                else
                {
                htemp2->v.h_val = htemp->v.h_val;
                }
	htemp2->next = htemp;
        htemp = htemp2;
        GLOBALS->vzt_table_vzt_c_1[txidx].numtrans++;
        }

np->head.time  = -2;
np->head.next = htemp;
np->numhist=GLOBALS->vzt_table_vzt_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

memset(GLOBALS->vzt_table_vzt_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

np->curr = histent_tail;
np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

if(nold!=np)
	{
	vzt_resolver(nold, np);
	}
}


/*
 * pre-import many traces at once so function above doesn't have to iterate...
 */
void vzt_set_fac_process_mask(nptr np)
{
struct fac *f;
int txidx;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f-GLOBALS->mvlfacs_vzt_c_3;

if(np->mv.mvlfac->flags&VZT_RD_SYM_F_ALIAS)
	{
	txidx = vzt_rd_get_alias_root(GLOBALS->vzt_vzt_c_1, txidx);
	np = GLOBALS->mvlfacs_vzt_c_3[txidx].working_node;

	if(!(np->mv.mvlfac)) return;	/* already imported */
	}

if(np->mv.mvlfac->node_alias <= 1) /* sorry, arrays not supported, but vzt doesn't support them yet either */
	{
	vzt_rd_set_fac_process_mask(GLOBALS->vzt_vzt_c_1, txidx);
	GLOBALS->vzt_table_vzt_c_1[txidx].np = np;
	}
}


void vzt_import_masked(void)
{
int txidx, i, cnt;

cnt = 0;
for(txidx=0;txidx<GLOBALS->numfacs;txidx++)
	{
	if(vzt_rd_get_fac_process_mask(GLOBALS->vzt_vzt_c_1, txidx))
		{
		cnt++;
		}
	}

if(!cnt) return;

if(cnt>100)
	{
	fprintf(stderr, VZT_RDLOAD"Extracting %d traces\n", cnt);
	}

set_window_busy(NULL);
vzt_rd_iter_blocks(GLOBALS->vzt_vzt_c_1, vzt_callback, NULL);
set_window_idle(NULL);

for(txidx=0;txidx<GLOBALS->numfacs;txidx++)
	{
	if(vzt_rd_get_fac_process_mask(GLOBALS->vzt_vzt_c_1, txidx))
		{
		struct HistEnt *htemp, *histent_tail;
		struct fac *f = GLOBALS->mvlfacs_vzt_c_3+txidx;
		int len = f->len;
		nptr np = GLOBALS->vzt_table_vzt_c_1[txidx].np;

		histent_tail = htemp = histent_calloc();
                if(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))
                        {
                        htemp->v.h_vector = strdup_2((f->flags&VZT_RD_SYM_F_DOUBLE) ? "NaN" : "UNDEF");
                        htemp->flags = HIST_REAL;
                        if(f->flags&VZT_RD_SYM_F_STRING) htemp->flags |= HIST_STRING;
                        }
                        else
                        {
			if(len>1)
				{
				htemp->v.h_vector = (char *)malloc_2(len);
				for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_Z;
				}
				else
				{
				htemp->v.h_val = AN_Z;		/* z */
				}
			}
		htemp->time = MAX_HISTENT_TIME;

		htemp = histent_calloc();
                if(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))
                        {
                        htemp->v.h_vector = strdup_2((f->flags&VZT_RD_SYM_F_DOUBLE) ? "NaN" : "UNDEF");
                        htemp->flags = HIST_REAL;
                        if(f->flags&VZT_RD_SYM_F_STRING) htemp->flags |= HIST_STRING;
                        }
                        else
                        {
			if(len>1)
				{
				htemp->v.h_vector = (char *)malloc_2(len);
				for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_X;
				}
				else
				{
				htemp->v.h_val = AN_X;		/* x */
				}
			}
		htemp->time = MAX_HISTENT_TIME-1;
		htemp->next = histent_tail;

		if(GLOBALS->vzt_table_vzt_c_1[txidx].histent_curr)
			{
			GLOBALS->vzt_table_vzt_c_1[txidx].histent_curr->next = htemp;
			htemp = GLOBALS->vzt_table_vzt_c_1[txidx].histent_head;
			}

		if(!(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
		        {
			if(len>1)
				{
				np->head.v.h_vector = (char *)malloc_2(len);
				for(i=0;i<len;i++) np->head.v.h_vector[i] = AN_X;
				}
				else
				{
				np->head.v.h_val = AN_X;	/* x */
				}
			}
		        else
		        {
		        np->head.flags = HIST_REAL;
		        if(f->flags&VZT_RD_SYM_F_STRING) np->head.flags |= HIST_STRING;

			np->head.v.h_vector = strdup_2((f->flags&VZT_RD_SYM_F_DOUBLE) ? "NaN" : "UNDEF");
		        }

                        {
                        struct HistEnt *htemp2 = histent_calloc();
                        htemp2->time = -1;

	                if(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))
	                        {
	                        htemp2->v.h_vector = strdup_2((f->flags&VZT_RD_SYM_F_DOUBLE) ? "NaN" : "UNDEF");
	                        htemp2->flags = HIST_REAL;
	                        if(f->flags&VZT_RD_SYM_F_STRING) htemp2->flags |= HIST_STRING;
	                        }
	                        else
	                        {
	                        if(len>1)
	                                {
	                                htemp2->v.h_vector = htemp->v.h_vector;
	                                }
	                                else
	                                {
	                                htemp2->v.h_val = htemp->v.h_val;
	                                }
				}
                        htemp2->next = htemp;
                        htemp = htemp2;
                        GLOBALS->vzt_table_vzt_c_1[txidx].numtrans++;
                        }

		np->head.time  = -2;
		np->head.next = htemp;
		np->numhist=GLOBALS->vzt_table_vzt_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

		memset(GLOBALS->vzt_table_vzt_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

		np->curr = histent_tail;
		np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */
		vzt_rd_clr_fac_process_mask(GLOBALS->vzt_vzt_c_1, txidx);
		}
	}
}

