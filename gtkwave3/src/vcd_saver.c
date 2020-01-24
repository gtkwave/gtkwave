/*
 * Copyright (c) Tony Bybell 2005-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "vcd_saver.h"
#include "helpers/lxt_write.h"
#include "ghw.h"
#include "hierpack.h"
#include <time.h>

WAVE_NODEVARTYPE_STR

static void w32redirect_fprintf(int is_trans, FILE *sfd, const char *format, ...)
{
#if defined _MSC_VER || defined __MINGW32__
if(is_trans)
	{
	char buf[16385];
	BOOL bSuccess;
	DWORD dwWritten;

	va_list ap;
	va_start(ap, format);
	buf[0] = 0;
	vsprintf(buf, format, ap);
	bSuccess = WriteFile((HANDLE)sfd, buf, strlen(buf), &dwWritten, NULL);
	va_end(ap);
	}
	else
#else
(void) is_trans;
#endif
	{
	va_list ap;
	va_start(ap, format);
	vfprintf(sfd, format, ap);
	va_end(ap);
	}
}


/*
 * unconvert trace data back to VCD representation...use strict mode for LXT
 */
static unsigned char analyzer_demang(int strict, unsigned char ch)
{
if(!strict)
	{
	if(ch < AN_COUNT)
		{
		return(AN_STR[ch]);
		}
		else
		{
		return(ch);
		}
	}
else
	{
	if(ch < AN_COUNT)
		{
		return(AN_STR4ST[ch]);
		}
		else
		{
		return(ch);
		}
	}
}


/*
 * generate a vcd identifier for a given facindx
 */
static char *vcdid(unsigned int value, int export_typ)
{
char *pnt = GLOBALS->buf_vcd_saver_c_3;
unsigned int vmod;

if(export_typ != WAVE_EXPORT_TRANS)
	{
	value++;
	for(;;)
	        {
	        if((vmod = (value % 94)))
	                {
	                *(pnt++) = (char)(vmod + 32);
	                }
	                else
	                {
	                *(pnt++) = '~'; value -= 94;
	                }
	        value = value / 94;
	        if(!value) { break; }
	        }

	*pnt = 0;
	}
	else
	{
	sprintf(pnt, "%d", value);
	}

return(GLOBALS->buf_vcd_saver_c_3);
}


static char *vcd_truncate_bitvec(char *s)
{
char l, r;

r=*s;
if(r=='1')
        {
        return s;
        }
        else
        {
        s++;
        }

for(;;s++)
        {
        l=r; r=*s;
        if(!r) return (s-1);

        if(l!=r)
                {
                return(((l=='0')&&(r=='1'))?s:s-1);
                }
        }
}


/************************ splay ************************/

/*
 * integer splay
 */
typedef struct vcdsav_tree_node vcdsav_Tree;
struct vcdsav_tree_node {
    vcdsav_Tree * left, * right;
    nptr item;
    int val;
    hptr hist;
    int len;

    union
	{
	void *p;
	long l;
	int i;
	} handle;	/* future expansion for adding other writers that need pnt/handle info */

    unsigned char flags;
};


static long vcdsav_cmp_l(void *i, void *j)
{
intptr_t il = (intptr_t)i, jl = (intptr_t)j;
return(il - jl);
}

static vcdsav_Tree * vcdsav_splay (void *i, vcdsav_Tree * t) {
/* Simple top down splay, not requiring i to be in the tree t.  */
/* What it does is described above.                             */
    vcdsav_Tree N, *l, *r, *y;
    int dir;

    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
	dir = vcdsav_cmp_l(i, t->item);
	if (dir < 0) {
	    if (t->left == NULL) break;
	    if (vcdsav_cmp_l(i, t->left->item)<0) {
		y = t->left;                           /* rotate right */
		t->left = y->right;
		y->right = t;
		t = y;
		if (t->left == NULL) break;
	    }
	    r->left = t;                               /* link right */
	    r = t;
	    t = t->left;
	} else if (dir > 0) {
	    if (t->right == NULL) break;
	    if (vcdsav_cmp_l(i, t->right->item)>0) {
		y = t->right;                          /* rotate left */
		t->right = y->left;
		y->left = t;
		t = y;
		if (t->right == NULL) break;
	    }
	    l->right = t;                              /* link left */
	    l = t;
	    t = t->right;
	} else {
	    break;
	}
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}


static vcdsav_Tree * vcdsav_insert(void *i, vcdsav_Tree * t, int val, unsigned char flags, hptr h) {
/* Insert i into the tree t, unless it's already there.    */
/* Return a pointer to the resulting tree.                 */
    vcdsav_Tree * n;
    int dir;

    n = (vcdsav_Tree *) calloc_2(1, sizeof (vcdsav_Tree));
    if (n == NULL) {
	fprintf(stderr, "vcdsav_insert: ran out of memory, exiting.\n");
	exit(255);
    }
    n->item = i;
    n->val = val;
    n->flags = flags;
    n->hist = h;
    if (t == NULL) {
	n->left = n->right = NULL;
	return n;
    }
    t = vcdsav_splay(i,t);
    dir = vcdsav_cmp_l(i,t->item);
    if (dir<0) {
	n->left = t->left;
	n->right = t;
	t->left = NULL;
	return n;
    } else if (dir>0) {
	n->right = t->right;
	n->left = t;
	t->right = NULL;
	return n;
    } else { /* We get here if it's already in the tree */
             /* Don't add it again                      */
	free_2(n);
	return t;
    }
}

/************************ heap ************************/

static int hpcmp(vcdsav_Tree *hp1, vcdsav_Tree *hp2)
{
hptr n1 = hp1->hist;
hptr n2 = hp2->hist;
TimeType t1, t2;

if(n1)  t1 = n1->time; else t1 = MAX_HISTENT_TIME;
if(n2)  t2 = n2->time; else t2 = MAX_HISTENT_TIME;

if(t1 == t2)
	{
	return(0);
	}
else
if(t1 > t2)
	{
	return(-1);
	}
else
	{
	return(1);
	}
}


static void recurse_build(vcdsav_Tree *vt, vcdsav_Tree ***hp)
{
if(vt->left) recurse_build(vt->left, hp);

**hp = vt;
*hp = (*hp) + 1;

if(vt->right) recurse_build(vt->right, hp);
}


/*
 * heapify algorithm...used to grab the next value change
 */
static void heapify(int i, int heap_size)
{
int l, r;
int largest;
vcdsav_Tree *t;
int maxele=heap_size/2-1;	/* points to where heapswaps don't matter anymore */

for(;;)
        {
        l=2*i+1;
        r=l+1;

        if((l<heap_size)&&(hpcmp(GLOBALS->hp_vcd_saver_c_1[l],GLOBALS->hp_vcd_saver_c_1[i])>0))
                {
                largest=l;
                }
                else
                {
                largest=i;
                }
        if((r<heap_size)&&(hpcmp(GLOBALS->hp_vcd_saver_c_1[r],GLOBALS->hp_vcd_saver_c_1[largest])>0))
                {
                largest=r;
                }

        if(i!=largest)
                {
                t=GLOBALS->hp_vcd_saver_c_1[i];
                GLOBALS->hp_vcd_saver_c_1[i]=GLOBALS->hp_vcd_saver_c_1[largest];
                GLOBALS->hp_vcd_saver_c_1[largest]=t;

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


/*
 * mainline
 */
int save_nodes_to_export_generic(FILE *trans_file, Trptr trans_head, const char *fname, int export_typ)
{
Trptr t = trans_head ? trans_head : GLOBALS->traces.first;
int nodecnt = 0;
vcdsav_Tree *vt = NULL;
vcdsav_Tree **hp_clone = GLOBALS->hp_vcd_saver_c_1;
nptr n;
/* ExtNode *e; */
/* int msi, lsi; */
int i;
TimeType prevtime = LLDescriptor(-1);
time_t walltime;
struct strace *st = NULL;
int strace_append = 0;
int max_len = 1;
char *row_data = NULL;
struct lt_trace *lt = NULL;
int lxt = (export_typ == WAVE_EXPORT_LXT);
int is_trans = (export_typ == WAVE_EXPORT_TRANS);
int dumpvars_state = 0;

if(export_typ == WAVE_EXPORT_TIM)
	{
	return(do_timfile_save(fname));
	}

errno = 0;
if(lxt)
	{
	lt = lt_init(fname);
	if(!lt)
		{
		return(VCDSAV_FILE_ERROR);
		}
	}
	else
	{
	if(export_typ != WAVE_EXPORT_TRANS)
		{
		GLOBALS->f_vcd_saver_c_1 = fopen(fname, "wb");
		}
		else
		{
		if(!trans_head) /* scan-build : is programming error to get here */
			{
			return(VCDSAV_FILE_ERROR);
			}
		GLOBALS->f_vcd_saver_c_1 = trans_file;
		}

	if(!GLOBALS->f_vcd_saver_c_1)
		{
		return(VCDSAV_FILE_ERROR);
		}
	}

while(t)
	{
	if(!t->vector)
		{
		if(t->n.nd)
			{
			n = t->n.nd;
			if(n->expansion) n = n->expansion->parent;
			vt = vcdsav_splay(n, vt);
			if(!vt || vt->item != n)
				{
				unsigned char flags = 0;

				if(n->head.next)
				if(n->head.next->next)
					{
					flags = n->head.next->next->flags;
					}

				vt = vcdsav_insert(n, vt, ++nodecnt, flags, &n->head);
				}
			}
		}
		else
		{
		bvptr b = t->n.vec;
		if(b)
			{
			bptr bt = b->bits;
			if(bt)
				{
				for(i=0;i<bt->nnbits;i++)
					{
					if(bt->nodes[i])
						{
						n = bt->nodes[i];

						if(n->expansion) n = n->expansion->parent;
						vt = vcdsav_splay(n, vt);
						if(!vt || vt->item != n)
							{
							unsigned char flags = 0;

							if(n->head.next)
							if(n->head.next->next)
								{
								flags = n->head.next->next->flags;
								}

							vt = vcdsav_insert(n, vt, ++nodecnt, flags, &n->head);
							}
						}
					}
				}
			}
		}


	if(export_typ == WAVE_EXPORT_TRANS)
		{
		break;
		}

	if(!strace_append)
		{
		t=t->t_next;
		if(t) continue;
		}
		else
		{
		st = st->next;
		t = st ? st->trace : NULL;
		if(t)
			{
			continue;
			}
			else
			{
			swap_strace_contexts();
			}
		}

strace_concat:
	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = strace_append];
	strace_append++;
	if(strace_append == WAVE_NUM_STRACE_WINDOWS) break;

	if(!GLOBALS->strace_ctx->shadow_straces)
		{
		goto strace_concat;
		}

	swap_strace_contexts();
	st = GLOBALS->strace_ctx->straces;
	t = st ? st->trace : NULL;
	if(!t) {swap_strace_contexts(); goto strace_concat; }
	}

if(!nodecnt) return(VCDSAV_EMPTY);


/* header */
if(lxt)
	{
	int dim;

	lt_set_chg_compress(lt);
	lt_set_clock_compress(lt);
	lt_set_initial_value(lt, 'x');
	lt_set_time64(lt, 0);
	lt_symbol_bracket_stripping(lt, 1);

	switch(GLOBALS->time_dimension)
		{
		case 'm':	dim = -3; break;
		case 'u':	dim = -6; break;
		case 'n':	dim = -9; break;
		case 'p':	dim = -12; break;
		case 'f':	dim = -15; break;
		default: 	dim = 0; break;
		}

	lt_set_timescale(lt, dim);
	}
	else
	{
	if(export_typ != WAVE_EXPORT_TRANS)
		{
		time(&walltime);
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$date\n");
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "\t%s",asctime(localtime(&walltime)));
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$end\n");
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$version\n\t"WAVE_VERSION_INFO"\n$end\n");
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$timescale\n\t%d%c%s\n$end\n", (int)GLOBALS->time_scale, GLOBALS->time_dimension, (GLOBALS->time_dimension=='s') ? "" : "s");
		if(GLOBALS->global_time_offset) { w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$timezero\n\t"TTFormat"\n$end\n",GLOBALS->global_time_offset); }
		}
		else
		{
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment data_start %p $end\n", (void *)trans_head); /* arbitrary hex identifier */
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment name %s $end\n", trans_head->name ? trans_head->name : "UNKNOWN");
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$timescale %d%c%s $end\n", (int)GLOBALS->time_scale, GLOBALS->time_dimension, (GLOBALS->time_dimension=='s') ? "" : "s");
		if(GLOBALS->global_time_offset) { w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$timezero "TTFormat" $end\n",GLOBALS->global_time_offset); }
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment min_time "TTFormat" $end\n", GLOBALS->min_time / GLOBALS->time_scale);
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment max_time "TTFormat" $end\n", GLOBALS->max_time / GLOBALS->time_scale);
		}
	}

if(export_typ == WAVE_EXPORT_TRANS)
	{
        w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment max_seqn %d $end\n", nodecnt);
	if(t && t->transaction_args)
		{
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment args \"%s\" $end\n", t->transaction_args);
		}
        }

/* write out netnames here ... */
hp_clone = GLOBALS->hp_vcd_saver_c_1 = calloc_2(nodecnt, sizeof(vcdsav_Tree *));
recurse_build(vt, &hp_clone);

for(i=0;i<nodecnt;i++)
	{
	int was_packed = HIER_DEPACK_STATIC;
	char *hname = hier_decompress_flagged(GLOBALS->hp_vcd_saver_c_1[i]->item->nname, &was_packed);
	char *netname = lxt ? hname : output_hier(is_trans, hname);

	if(export_typ == WAVE_EXPORT_TRANS)
		{
	        w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment seqn %d %s $end\n", GLOBALS->hp_vcd_saver_c_1[i]->val, hname);
	        }

	if(GLOBALS->hp_vcd_saver_c_1[i]->flags & (HIST_REAL|HIST_STRING))
		{
		if(lxt)
			{
			GLOBALS->hp_vcd_saver_c_1[i]->handle.p = lt_symbol_add(lt, netname, 0, 0, 0, GLOBALS->hp_vcd_saver_c_1[i]->flags & HIST_STRING ? LT_SYM_F_STRING : LT_SYM_F_DOUBLE);
			}
			else
			{
			const char *typ = (GLOBALS->hp_vcd_saver_c_1[i]->flags & HIST_STRING) ? "string" : "real";
			int tlen = (GLOBALS->hp_vcd_saver_c_1[i]->flags & HIST_STRING) ? 0 : 1;
			w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$var %s %d %s %s $end\n", typ, tlen, vcdid(GLOBALS->hp_vcd_saver_c_1[i]->val, export_typ), netname);
			}
		}
		else
		{
		int msi = -1, lsi = -1;

		if(GLOBALS->hp_vcd_saver_c_1[i]->item->extvals)
			{
			msi = GLOBALS->hp_vcd_saver_c_1[i]->item->msi;
			lsi = GLOBALS->hp_vcd_saver_c_1[i]->item->lsi;
			}

		if(msi==lsi)
			{
			if(lxt)
				{
				int strand_idx = strand_pnt(netname);
				if(strand_idx >= 0)
					{
					msi = lsi = atoi(netname + strand_idx + 1);
					}
				GLOBALS->hp_vcd_saver_c_1[i]->handle.p = lt_symbol_add(lt, netname, 0, msi, lsi, LT_SYM_F_BITS);
				}
				else
				{
				w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$var %s 1 %s %s $end\n", vartype_strings[GLOBALS->hp_vcd_saver_c_1[i]->item->vartype], vcdid(GLOBALS->hp_vcd_saver_c_1[i]->val, export_typ), netname);
				}
			}
			else
			{
			int len = (msi < lsi) ? (lsi - msi + 1) : (msi - lsi + 1);
			if(lxt)
				{
				GLOBALS->hp_vcd_saver_c_1[i]->handle.p = lt_symbol_add(lt, netname, 0, msi, lsi, LT_SYM_F_BITS);
				}
				else
				{
				w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$var %s %d %s %s $end\n", vartype_strings[GLOBALS->hp_vcd_saver_c_1[i]->item->vartype], len, vcdid(GLOBALS->hp_vcd_saver_c_1[i]->val, export_typ), netname);
				}
			GLOBALS->hp_vcd_saver_c_1[i]->len = len;
			if(len > max_len) max_len = len;
			}
		}

	/* if(was_packed) { free_2(hname); } ...not needed for HIER_DEPACK_STATIC */
	}

row_data = malloc_2(max_len + 1);

if(!lxt)
	{
	output_hier(is_trans, "");
	free_hier();

	w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$enddefinitions $end\n");
	}

/* value changes */

for(i=(nodecnt/2-1);i>0;i--)        /* build nodes into having heap property */
        {
        heapify(i,nodecnt);
        }

for(;;)
	{
	heapify(0, nodecnt);

	if(!GLOBALS->hp_vcd_saver_c_1[0]->hist) break;
	if(GLOBALS->hp_vcd_saver_c_1[0]->hist->time > GLOBALS->max_time) break;

	if((GLOBALS->hp_vcd_saver_c_1[0]->hist->time != prevtime) && (GLOBALS->hp_vcd_saver_c_1[0]->hist->time >= LLDescriptor(0)))
		{
		TimeType tnorm = GLOBALS->hp_vcd_saver_c_1[0]->hist->time;
		if(GLOBALS->time_scale != 1)
			{
			tnorm /= GLOBALS->time_scale;
			}

		if(lxt)
			{
			lt_set_time64(lt, tnorm);
			}
			else
			{
			if(dumpvars_state == 1) { w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$end\n"); dumpvars_state = 2; }
			w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "#"TTFormat"\n", tnorm);
			if(!dumpvars_state) { w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$dumpvars\n"); dumpvars_state = 1; }
			}
		prevtime = GLOBALS->hp_vcd_saver_c_1[0]->hist->time;
		}

	if(GLOBALS->hp_vcd_saver_c_1[0]->hist->time >= LLDescriptor(0))
		{
		if(GLOBALS->hp_vcd_saver_c_1[0]->flags & (HIST_REAL|HIST_STRING))
			{
			if(GLOBALS->hp_vcd_saver_c_1[0]->flags & HIST_STRING)
				{
				char *vec = GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_vector ? GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_vector : "UNDEF";

				if(lxt)
					{
					lt_emit_value_string(lt, GLOBALS->hp_vcd_saver_c_1[0]->handle.p, 0, vec);
					}
					else
					{
					int vec_slen = strlen(vec);
					char *vec_escaped = malloc_2(vec_slen*4 + 1); /* worst case */
					int vlen = fstUtilityBinToEsc((unsigned char *)vec_escaped, (unsigned char *)vec, vec_slen);

					vec_escaped[vlen] = 0;
					if(vlen)
						{
						w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "s%s %s\n", vec_escaped, vcdid(GLOBALS->hp_vcd_saver_c_1[0]->val, export_typ));
						}
						else
						{
						w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "s\\000 %s\n", vcdid(GLOBALS->hp_vcd_saver_c_1[0]->val, export_typ));
						}
					free_2(vec_escaped);
					}
				}
				else
				{
#ifdef WAVE_HAS_H_DOUBLE
				double *d = &GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_double;
#else
				double *d = (double *)GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_vector;
#endif
                                double value;

				if(!d)
					{
	                                sscanf("NaN", "%lg", &value);
					}
					else
					{
					value = *d;
					}

				if(lxt)
					{
					lt_emit_value_double(lt, GLOBALS->hp_vcd_saver_c_1[0]->handle.p, 0, value);
					}
					else
					{
					w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "r%.16g %s\n", value, vcdid(GLOBALS->hp_vcd_saver_c_1[0]->val, export_typ));
					}
				}
			}
		else
		if(GLOBALS->hp_vcd_saver_c_1[0]->len)
			{
			if(GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_vector)
				{
				for(i=0;i<GLOBALS->hp_vcd_saver_c_1[0]->len;i++)
					{
					row_data[i] = analyzer_demang(lxt, GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_vector[i]);
					}
				}
				else
				{
				for(i=0;i<GLOBALS->hp_vcd_saver_c_1[0]->len;i++)
					{
					row_data[i] = 'x';
					}
				}
			row_data[i] = 0;

			if(lxt)
				{
				lt_emit_value_bit_string(lt, GLOBALS->hp_vcd_saver_c_1[0]->handle.p, 0, row_data);
				}
				else
				{
				w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "b%s %s\n", vcd_truncate_bitvec(row_data), vcdid(GLOBALS->hp_vcd_saver_c_1[0]->val, export_typ));
				}
			}
		else
			{
			if(lxt)
				{
				row_data[0] = analyzer_demang(lxt, GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_val);
				row_data[1] = 0;

				lt_emit_value_bit_string(lt, GLOBALS->hp_vcd_saver_c_1[0]->handle.p, 0, row_data);
				}
				else
				{
				w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "%c%s\n", analyzer_demang(lxt, GLOBALS->hp_vcd_saver_c_1[0]->hist->v.h_val), vcdid(GLOBALS->hp_vcd_saver_c_1[0]->val, export_typ));
				}
			}
		}

	GLOBALS->hp_vcd_saver_c_1[0]->hist = GLOBALS->hp_vcd_saver_c_1[0]->hist->next;
	}

if(prevtime < GLOBALS->max_time)
	{
	if(lxt)
		{
		lt_set_time64(lt, GLOBALS->max_time / GLOBALS->time_scale);
		}
		else
		{
		if(dumpvars_state == 1) { w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$end\n"); dumpvars_state = 2; }
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "#"TTFormat"\n", GLOBALS->max_time / GLOBALS->time_scale);
		}
	}


for(i=0;i<nodecnt;i++)
	{
	free_2(GLOBALS->hp_vcd_saver_c_1[i]);
	}

free_2(GLOBALS->hp_vcd_saver_c_1); GLOBALS->hp_vcd_saver_c_1 = NULL;
free_2(row_data); row_data = NULL;

if(lxt)
	{
	lt_close(lt); lt = NULL;
	}
	else
	{
	if(export_typ != WAVE_EXPORT_TRANS)
		{
		fclose(GLOBALS->f_vcd_saver_c_1);
		}
		else
		{
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$comment data_end %p $end\n", (void *)trans_head); /* arbitrary hex identifier */
#if !defined _MSC_VER && !defined __MINGW32__
		fflush(GLOBALS->f_vcd_saver_c_1);
#endif
		}

	GLOBALS->f_vcd_saver_c_1 = NULL;
	}

return(VCDSAV_OK);
}



int save_nodes_to_export(const char *fname, int export_typ)
{
return(save_nodes_to_export_generic(NULL, NULL, fname, export_typ));
}

int save_nodes_to_trans(FILE *trans, Trptr t)
{
return(save_nodes_to_export_generic(trans, t, NULL, WAVE_EXPORT_TRANS));
}


/************************ scopenav ************************/

struct namehier
{
struct namehier *next;
char *name;
char not_final;
};



void free_hier(void)
{
struct namehier *nhtemp;

while(GLOBALS->nhold_vcd_saver_c_1)
	{
	nhtemp=GLOBALS->nhold_vcd_saver_c_1->next;
	free_2(GLOBALS->nhold_vcd_saver_c_1->name);
	free_2(GLOBALS->nhold_vcd_saver_c_1);
	GLOBALS->nhold_vcd_saver_c_1=nhtemp;
	}
}

/*
 * navigate up and down the scope hierarchy and
 * emit the appropriate vcd scope primitives
 */
static void diff_hier(int is_trans, struct namehier *nh1, struct namehier *nh2)
{
/* struct namehier *nhtemp; */ /* scan-build */

if(!nh2)
	{
	while((nh1)&&(nh1->not_final))
		{
		w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$scope module %s $end\n", nh1->name);
		nh1=nh1->next;
		}
	return;
	}

for(;;)
	{
	if((nh1->not_final==0)&&(nh2->not_final==0)) /* both are equal */
		{
		break;
		}

	if(nh2->not_final==0)	/* old hier is shorter */
		{
		/* nhtemp=nh1; */ /* scan-build */
		while((nh1)&&(nh1->not_final))
			{
			w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$scope module %s $end\n", nh1->name);
			nh1=nh1->next;
			}
		break;
		}

	if(nh1->not_final==0)	/* new hier is shorter */
		{
		/* nhtemp=nh2; */ /* scan-build */
		while((nh2)&&(nh2->not_final))
			{
			w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$upscope $end\n");
			nh2=nh2->next;
			}
		break;
		}

	if(strcmp(nh1->name, nh2->name))
		{
		/* nhtemp=nh2; */ /* prune old hier */ /* scan-build */
		while((nh2)&&(nh2->not_final))
			{
			w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$upscope $end\n");
			nh2=nh2->next;
			}

		/* nhtemp=nh1; */ /* add new hier */ /* scan-build */
		while((nh1)&&(nh1->not_final))
			{
			w32redirect_fprintf(is_trans, GLOBALS->f_vcd_saver_c_1, "$scope module %s $end\n", nh1->name);
			nh1=nh1->next;
			}
		break;
		}

	nh1=nh1->next;
	nh2=nh2->next;
	}
}


/*
 * output scopedata for a given name if needed, return pointer to name string
 */
char *output_hier(int is_trans, char *name)
{
char *pnt, *pnt2;
char *s;
int len;
struct namehier *nh_head=NULL, *nh_curr=NULL, *nhtemp;

pnt=pnt2=name;

for(;;)
{
if(*pnt2 == '\\')
	{
	while(*pnt2) pnt2++;
	}
	else
	{
	/* while((*pnt2!='.')&&(*pnt2)) pnt2++; ... does not handle dot at end of name */

	while(*pnt2)
		{
		if(*pnt2!='.')
			{
			pnt2++;
			continue;
			}
			else
			{
			if(!*(pnt2+1))	/* if dot is at end of name */
				{
				pnt2++;
				continue;
				}
				else
				{
				break;
				}
			}
		}
	}

s=(char *)calloc_2(1,(len=pnt2-pnt)+1);
memcpy(s, pnt, len);
nhtemp=(struct namehier *)calloc_2(1,sizeof(struct namehier));
nhtemp->name=s;

if(!nh_curr)
	{
	nh_head=nh_curr=nhtemp;
	}
	else
	{
	nh_curr->next=nhtemp;
	nh_curr->not_final=1;
	nh_curr=nhtemp;
	}

if(!*pnt2) break;
pnt=(++pnt2);
}

diff_hier(is_trans, nh_head, GLOBALS->nhold_vcd_saver_c_1);
free_hier();
GLOBALS->nhold_vcd_saver_c_1=nh_head;

	{
	char *mti_sv_patch = strstr(nh_curr->name, "]["); 	/* case is: #implicit-var###VarElem:ram_di[0.0] [63:0] */
	if(mti_sv_patch)
		{
		char *t = calloc_2(1, strlen(nh_curr->name) + 1 + 1);

		*mti_sv_patch = 0;
		sprintf(t, "%s] %s", nh_curr->name, mti_sv_patch+1);

		free_2(nh_curr->name);
		nh_curr->name = t;
		}

	if((nh_curr->name[0] == '\\') && (nh_curr->name[1] == '#'))
		{
		return(nh_curr->name+1);
		}
	}

return(nh_curr->name);
}


/****************************************/
/***                                  ***/
/*** output in timing analyzer format ***/
/***                                  ***/
/****************************************/

static void write_hptr_trace(Trptr t, int *whichptr, TimeType tmin, TimeType tmax)
{
nptr n = t->n.nd;
hptr *ha = n->harray;
int numhist = n->numhist;
int i;
unsigned char h_val = AN_X;
gboolean first;
gboolean invert = ((t->flags&TR_INVERT) != 0);
int edges = 0;

first = TRUE;
for(i=0;i<numhist;i++)
	{
	if(ha[i]->time < tmin)
		{
		}
	else
	if(ha[i]->time > tmax)
		{
		break;
		}
	else
		{
		if((ha[i]->time != tmin) || (!first))
			{
			edges++;
			}

		first = FALSE;
		}
	}


first = TRUE;
for(i=0;i<numhist;i++)
	{
	if(ha[i]->time < tmin)
		{
		h_val = invert ? AN_USTR_INV[ha[i]->v.h_val] : AN_USTR[ha[i]->v.h_val];
		}
	else
	if(ha[i]->time > tmax)
		{
		break;
		}
	else
		{
		if(first)
			{
			gboolean skip_this = (ha[i]->time == tmin);

			if(skip_this)
				{
				h_val = invert ? AN_USTR_INV[ha[i]->v.h_val] : AN_USTR[ha[i]->v.h_val];
				}

			w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
				"Digital_Signal\n"
				"     Position:          %d\n"
				"     Height:            24\n"
				"     Space_Above:       24\n"
				"     Name:              %s\n"
				"     Start_State:       %c\n"
				"     Number_Edges:      %d\n"
				"     Rise_Time:         0.2\n"
				"     Fall_Time:         0.2\n",
					*whichptr,
					t->name,
					h_val,
					edges);
			first = FALSE;

			if(skip_this)
				{
				continue;
				}
			}

		h_val = invert ? AN_USTR_INV[ha[i]->v.h_val] : AN_USTR[ha[i]->v.h_val];
		w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1, "          Edge:               "TTFormat".0 %c\n", ha[i]->time, h_val);
		}
	}

if(first)
	{
	/* need to emit blank trace */
	w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
		"Digital_Signal\n"
		"     Position:          %d\n"
		"     Height:            24\n"
		"     Space_Above:       24\n"
		"     Name:              %s\n"
		"     Start_State:       %c\n"
		"     Number_Edges:      %d\n"
		"     Rise_Time:         10.0\n"
		"     Fall_Time:         10.0\n",
			*whichptr,
			t->name,
			h_val,
			edges);
	}

(*whichptr)++;
}

/***/

static void format_value_string(char *s)
{
char *s_orig = s;

if((s)&&(*s))
	{
	gboolean is_all_z = TRUE;
	gboolean is_all_x = TRUE;

	while(*s)
		{
		if((*s != 'z') && (*s != 'Z'))
			{
			is_all_z = FALSE;
			}
		if((*s != 'x') && (*s != 'X'))
			{
			is_all_x = FALSE;
			}

		/* if(isspace(*s)) *s='_'; ...not needed */
		s++;
		}

	if(is_all_z)
		{
		*(s_orig++) = 'Z';
		*(s_orig) = 0;
		}
	else
	if(is_all_x)
		{
		*(s_orig++) = 'X';
		*(s_orig) = 0;
		}
	}
}

static char *get_hptr_vector_val(Trptr t, hptr h)
{
char *ascii = NULL;

if(h->time < LLDescriptor(0))
	{
	ascii=strdup_2("X");
	}
else
if(h->flags&HIST_REAL)
	{
        if(!(h->flags&HIST_STRING))
        	{
#ifdef WAVE_HAS_H_DOUBLE
                ascii=convert_ascii_real(t, &h->v.h_double);
#else
                ascii=convert_ascii_real(t, (double *)h->v.h_vector);
#endif
                }
                else
                {
                ascii=convert_ascii_string((char *)h->v.h_vector);
                }
	}
        else
        {
        ascii=convert_ascii_vec(t,h->v.h_vector);
        }

format_value_string(ascii);
return(ascii);
}

static const char *vcdsav_dtypes[] = { "Bin", "Hex", "Text" };

static int determine_trace_data_type(char *s, int curtype)
{
int i;

if((s) && (curtype != VCDSAV_IS_TEXT))
	{
	int len = strlen(s);
	for(i=0;i<len;i++)
		{
		int ch = (int)((unsigned char)s[i]);

		if(strchr("01xXzZhHuUwWlL-?_", ch))
			{
			/* nothing */
			}
		else if(strchr("23456789aAbBcCdDeEfF", ch))
			{
			curtype = VCDSAV_IS_HEX;
			}
		else
			{
			curtype = VCDSAV_IS_TEXT;
			break;
			}
		}
	}

return(curtype);
}


static void write_hptr_trace_vector(Trptr t, int *whichptr, TimeType tmin, TimeType tmax)
{
nptr n = t->n.nd;
hptr *ha = n->harray;
int numhist = n->numhist;
int i;
char *h_val = NULL;
gboolean first;
int edges = 0;
int curtype = VCDSAV_IS_BIN;

first = TRUE;
for(i=0;i<numhist;i++)
	{
	if(ha[i]->time < tmin)
		{
		}
	else
	if(ha[i]->time > tmax)
		{
		break;
		}
	else
		{
		char *s = get_hptr_vector_val(t, ha[i]);
		if(s)
			{
			curtype = determine_trace_data_type(s, curtype);
			free_2(s);
			}

		if((ha[i]->time != tmin) || (!first))
			{
			edges++;
			}

		first = FALSE;
		}
	}


first = TRUE;
for(i=0;i<numhist;i++)
	{
	if(ha[i]->time < tmin)
		{
		if(h_val) free_2(h_val);
		h_val = get_hptr_vector_val(t, ha[i]);
		}
	else
	if(ha[i]->time > tmax)
		{
		break;
		}
	else
		{
		if(first)
			{
			gboolean skip_this = (ha[i]->time == tmin);

			if(skip_this)
				{
				if(h_val) free_2(h_val);
				h_val = get_hptr_vector_val(t, ha[i]);
				}

			w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
				"Digital_Bus\n"
				"     Position:          %d\n"
				"     Height:            24\n"
				"     Space_Above:       24\n"
				"     Name:              %s\n"
				"     Start_State:       %s\n"
				"     State_Format:      %s\n"
				"     Number_Edges:      %d\n"
				"     Rise_Time:         0.2\n"
				"     Fall_Time:         0.2\n",
					*whichptr,
					t->name,
					h_val,
					vcdsav_dtypes[curtype],
					edges);
			first = FALSE;

			if(skip_this)
				{
				continue;
				}
			}

		if(h_val) free_2(h_val);
		h_val = get_hptr_vector_val(t, ha[i]);
		w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1, "          Edge:               "TTFormat".0 %s\n", ha[i]->time, h_val);
		}
	}

if(first)
	{
	/* need to emit blank trace */
	w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
		"Digital_Bus\n"
		"     Position:          %d\n"
		"     Height:            24\n"
		"     Space_Above:       24\n"
		"     Name:              %s\n"
		"     Start_State:       %s\n"
		"     State_Format:      %s\n"
		"     Number_Edges:      %d\n"
		"     Rise_Time:         10.0\n"
		"     Fall_Time:         10.0\n",
			*whichptr,
			t->name,
			h_val,
			vcdsav_dtypes[curtype],
			edges);
	}

if(h_val) free_2(h_val);
(*whichptr)++;
}

/***/

static char *get_vptr_vector_val(Trptr t, vptr v)
{
char *ascii = NULL;

if(v->time < LLDescriptor(0))
	{
	ascii=strdup_2("X");
	}
else
        {
        ascii=convert_ascii(t,v);
        }

if(!ascii)
	{
	ascii=strdup_2("X");
	}

format_value_string(ascii);
return(ascii);
}


static void write_vptr_trace(Trptr t, int *whichptr, TimeType tmin, TimeType tmax)
{
vptr *ha = t->n.vec->vectors;
int numhist = t->n.vec->numregions;
int i;
char *h_val = NULL;
gboolean first;
int edges = 0;
int curtype = VCDSAV_IS_BIN;

first = TRUE;
for(i=0;i<numhist;i++)
	{
	if(ha[i]->time < tmin)
		{
		}
	else
	if(ha[i]->time > tmax)
		{
		break;
		}
	else
		{
		char *s = get_vptr_vector_val(t, ha[i]);
		if(s)
			{
			curtype = determine_trace_data_type(s, curtype);
			free_2(s);
			}

		if((ha[i]->time != tmin) || (!first))
			{
			edges++;
			}

		first = FALSE;
		}
	}


first = TRUE;
for(i=0;i<numhist;i++)
	{
	if(ha[i]->time < tmin)
		{
		if(h_val) free_2(h_val);
		h_val = get_vptr_vector_val(t, ha[i]);
		}
	else
	if(ha[i]->time > tmax)
		{
		break;
		}
	else
		{
		if(first)
			{
			gboolean skip_this = (ha[i]->time == tmin);

			if(skip_this)
				{
				if(h_val) free_2(h_val);
				h_val = get_vptr_vector_val(t, ha[i]);
				}

			w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
				"Digital_Bus\n"
				"     Position:          %d\n"
				"     Height:            24\n"
				"     Space_Above:       24\n"
				"     Name:              %s\n"
				"     Start_State:       %s\n"
				"     State_Format:      %s\n"
				"     Number_Edges:      %d\n"
				"     Rise_Time:         0.2\n"
				"     Fall_Time:         0.2\n",
					*whichptr,
					t->name,
					h_val,
					vcdsav_dtypes[curtype],
					edges);
			first = FALSE;

			if(skip_this)
				{
				continue;
				}
			}

		if(h_val) free_2(h_val);
		h_val = get_vptr_vector_val(t, ha[i]);
		w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1, "          Edge:               "TTFormat".0 %s\n", ha[i]->time, h_val);
		}
	}

if(first)
	{
	/* need to emit blank trace */
	w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
		"Digital_Bus\n"
		"     Position:          %d\n"
		"     Height:            24\n"
		"     Space_Above:       24\n"
		"     Name:              %s\n"
		"     Start_State:       %s\n"
		"     State_Format:      %s\n"
		"     Number_Edges:      %d\n"
		"     Rise_Time:         10.0\n"
		"     Fall_Time:         10.0\n",
			*whichptr,
			t->name,
			h_val,
			vcdsav_dtypes[curtype],
			edges);
	}

if(h_val) free_2(h_val);
(*whichptr)++;
}


static void write_tim_tracedata(Trptr t, int *whichptr, TimeType tmin, TimeType tmax)
{
if(!(t->flags&(TR_EXCLUDE|TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
	{
        GLOBALS->shift_timebase=t->shift;
        if(!t->vector)
        	{
                if(!t->n.nd->extvals)
                	{
			write_hptr_trace(t, whichptr, tmin, tmax); /* single-bit */
                        }
                        else
                        {
			write_hptr_trace_vector(t, whichptr, tmin, tmax); /* multi-bit */
                        }
		}
                else
                {
		write_vptr_trace(t, whichptr, tmin, tmax); /* synthesized/concatenated vector */
                }
	}
}


int do_timfile_save(const char *fname)
{
const char *time_prefix=WAVE_SI_UNITS;
const double negpow[] = { 1.0, 1.0e-3, 1.0e-6, 1.0e-9, 1.0e-12, 1.0e-15, 1.0e-18, 1.0e-21 };
char *pnt;
int offset;
Trptr t = GLOBALS->traces.first;
int i = 1; /* trace index in the .tim file */
TimeType tmin, tmax;

errno = 0;

if((GLOBALS->tims.marker > LLDescriptor(0)) && (GLOBALS->tims.baseline > LLDescriptor(0)))
	{
	if(GLOBALS->tims.marker < GLOBALS->tims.baseline)
		{
	        tmin = GLOBALS->tims.marker;
	        tmax = GLOBALS->tims.baseline;
		}
		else
		{
	        tmax = GLOBALS->tims.marker;
	        tmin = GLOBALS->tims.baseline;
		}
	}
	else
	{
        tmin = GLOBALS->min_time;
        tmax = GLOBALS->max_time;
	}

GLOBALS->f_vcd_saver_c_1 = fopen(fname, "wb");
if(!GLOBALS->f_vcd_saver_c_1)
	{
        return(VCDSAV_FILE_ERROR);
        }

pnt=strchr(time_prefix, (int)GLOBALS->time_dimension);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

w32redirect_fprintf(0, GLOBALS->f_vcd_saver_c_1,
	"Timing Analyzer Settings\n"
	"     Time_Scale:        %E\n"
	"     Time_Per_Division: %E\n"
	"     NumberDivisions:   10\n"
	"     Start_Time:        "TTFormat".0\n"
	"     End_Time:          "TTFormat".0\n",

	negpow[offset],
	(tmax-tmin) / 10.0,
	tmin,
	tmax
	);

while(t)
	{
	write_tim_tracedata(t, &i, tmin, tmax);
        t = GiveNextTrace(t);
	}

fclose(GLOBALS->f_vcd_saver_c_1);
GLOBALS->f_vcd_saver_c_1 = NULL;

return(errno ? VCDSAV_FILE_ERROR : VCDSAV_OK);
}

