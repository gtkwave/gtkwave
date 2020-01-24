/*
 * Copyright (c) Tony Bybell 2009-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <stdio.h>
#include "fstapi.h"
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
#include "fstapi.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"
#include "fst.h"


#define FST_RDLOAD "FSTLOAD | "

/******************************************************************/

/*
 * doubles going into histent structs are NEVER freed so this is OK..
 * (we are allocating as many entries that fit in 4k minus the size of the two
 * bookkeeping void* pointers found in the malloc_2/free_2 routines in
 * debug.c)
 */
#ifdef _WAVE_HAVE_JUDY
#define FST_DOUBLE_GRANULARITY ( (4*1024) / sizeof(double) )
#else
#define FST_DOUBLE_GRANULARITY ( ( (4*1024)-(2*sizeof(void *)) ) / sizeof(double) )
#endif

#ifndef WAVE_HAS_H_DOUBLE
static void *double_slab_calloc(void)
{
if(GLOBALS->double_curr_fst==GLOBALS->double_fini_fst)
        {
        GLOBALS->double_curr_fst=(double *)calloc_2(FST_DOUBLE_GRANULARITY, sizeof(double));
        GLOBALS->double_fini_fst=GLOBALS->double_curr_fst+FST_DOUBLE_GRANULARITY;
        }

return((void *)(GLOBALS->double_curr_fst++));
}
#endif

/*
 * reverse equality mem compare
 */
static int memrevcmp(int i, const char *s1, const char *s2)
{
i--;
for(;i>=0;i--)
        {
        if(s1[i] != s2[i]) break;
        }

return(i+1);
}



/*
 * fast itoa for decimal numbers
 */
static char* itoa_2(int value, char* result)
{
char* ptr = result, *ptr1 = result, tmp_char;
int tmp_value;

do {
        tmp_value = value;
        value /= 10;
        *ptr++ = "9876543210123456789" [9 + (tmp_value - value * 10)];
} while ( value );

if (tmp_value < 0) *ptr++ = '-';
result = ptr;
*ptr-- = '\0';
while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
}
return(result);
}


/*
 * preformatted sprintf statements which remove parsing latency
 */
static int sprintf_2_sd(char *s, char *c, int d)
{
char *s2 = s;

while(*c)
        {
        *(s2++) = *(c++);
        }
*(s2++) = '[';
s2 = itoa_2(d, s2);
*(s2++) = ']';
*s2 = 0;

return(s2 - s);
}


static int sprintf_2_sdd(char *s, char *c, int d, int d2)
{
char *s2 = s;

while(*c)
        {
        *(s2++) = *(c++);
        }
*(s2++) = '[';
s2 = itoa_2(d, s2);
*(s2++) = ':';
s2 = itoa_2(d2, s2);
*(s2++) = ']';
*s2 = 0;

return(s2 - s);
}

/******************************************************************/


static struct fstHier *extractNextVar(void *xc, int *msb, int *lsb, char **nam, int *namlen, unsigned int *nnam_max)
{
struct fstHier *h;
const char *pnts;
char *pnt, *pntd, *lb_last = NULL, *col_last = NULL; /* *rb_last = NULL; */ /* scan-build : unused */
int acc;
char *s;
unsigned char ttype;
int sdt = FST_SDT_NONE;
int svt = FST_SVT_NONE;
long sxt = 0;

while((h = fstReaderIterateHier(xc)))
        {
        switch(h->htyp)
                {
                case FST_HT_SCOPE:
                        GLOBALS->fst_scope_name = fstReaderPushScope(xc, h->u.scope.name, GLOBALS->mod_tree_parent);
			GLOBALS->fst_scope_name_len = fstReaderGetCurrentScopeLen(xc);

			switch(h->u.scope.typ)
				{
				case FST_ST_VCD_MODULE:		ttype = TREE_VCD_ST_MODULE; break;
				case FST_ST_VCD_TASK:		ttype = TREE_VCD_ST_TASK; break;
				case FST_ST_VCD_FUNCTION:	ttype = TREE_VCD_ST_FUNCTION; break;
				case FST_ST_VCD_BEGIN:		ttype = TREE_VCD_ST_BEGIN; break;
				case FST_ST_VCD_FORK:		ttype = TREE_VCD_ST_FORK; break;
				case FST_ST_VCD_GENERATE:	ttype = TREE_VCD_ST_GENERATE; break;
				case FST_ST_VCD_STRUCT:		ttype = TREE_VCD_ST_STRUCT; break;
				case FST_ST_VCD_UNION:		ttype = TREE_VCD_ST_UNION; break;
				case FST_ST_VCD_CLASS:		ttype = TREE_VCD_ST_CLASS; break;
				case FST_ST_VCD_INTERFACE:	ttype = TREE_VCD_ST_INTERFACE; break;
				case FST_ST_VCD_PACKAGE:	ttype = TREE_VCD_ST_PACKAGE; break;
				case FST_ST_VCD_PROGRAM:	ttype = TREE_VCD_ST_PROGRAM; break;

				case FST_ST_VHDL_ARCHITECTURE:	ttype = TREE_VHDL_ST_ARCHITECTURE; break;
				case FST_ST_VHDL_PROCEDURE:	ttype = TREE_VHDL_ST_PROCEDURE;    break;
				case FST_ST_VHDL_FUNCTION:	ttype = TREE_VHDL_ST_FUNCTION;     break;
				case FST_ST_VHDL_RECORD:	ttype = TREE_VHDL_ST_RECORD;       break;
				case FST_ST_VHDL_PROCESS:	ttype = TREE_VHDL_ST_PROCESS;      break;
				case FST_ST_VHDL_BLOCK:		ttype = TREE_VHDL_ST_BLOCK;        break;
				case FST_ST_VHDL_FOR_GENERATE:	ttype = TREE_VHDL_ST_GENFOR;       break;
				case FST_ST_VHDL_IF_GENERATE:	ttype = TREE_VHDL_ST_GENIF;        break;
				case FST_ST_VHDL_GENERATE:	ttype = TREE_VHDL_ST_GENERATE;     break;
				case FST_ST_VHDL_PACKAGE:	ttype = TREE_VHDL_ST_PACKAGE;      break;

				default:			ttype = TREE_UNKNOWN; break;
				}

			allocate_and_decorate_module_tree_node(ttype, h->u.scope.name, h->u.scope.component, h->u.scope.name_length, h->u.scope.component_length,
				GLOBALS->stem_valid ? GLOBALS->stem_struct_base_siz : 0,
				GLOBALS->istem_valid ? GLOBALS->istem_struct_base_siz : 0);
			GLOBALS->stem_valid = GLOBALS->istem_valid = 0;
                        break;
                case FST_HT_UPSCOPE:
			GLOBALS->mod_tree_parent = fstReaderGetCurrentScopeUserInfo(xc);
                        GLOBALS->fst_scope_name = fstReaderPopScope(xc);
			GLOBALS->fst_scope_name_len = fstReaderGetCurrentScopeLen(xc);
                        break;

                case FST_HT_VAR:
                        /* GLOBALS->fst_scope_name = fstReaderGetCurrentFlatScope(xc); */
			/* GLOBALS->fst_scope_name_len = fstReaderGetCurrentScopeLen(xc); */

			if(h->u.var.name_length > (*nnam_max))
				{
				free_2(*nam); *nam = malloc_2(((*nnam_max) = h->u.var.name_length) + 1);
				}
			s = *nam;
			pnts = h->u.var.name;
			pntd = s;
			while(*pnts)
				{
				if(*pnts != ' ')
					{
					if(*pnts == '[') { lb_last = pntd; col_last = NULL; }
					else if(*pnts == ':') { col_last = pntd; }
					/* else if(*pnts == ']') { rb_last = pntd; } */ /* scan-build : unused */

					*(pntd++) = *pnts;
					}
				pnts++;
				}
			*pntd = 0; *namlen = pntd - s;

			if(!lb_last)
				{
				*msb = *lsb = -1;
				}
			else if	((h->u.var.length > 1) && !col_last && lb_last) /* add for NC arrays that don't add final explicit bitrange like VCS does */
				{
				lb_last = NULL;
				*msb = h->u.var.length -1;
				*lsb = 0;
				}
			else
				{
				int sgna = 1, sgnb = 1;
				pnt = lb_last + 1;
				acc = 0;
				while(isdigit((int)(unsigned char)*pnt) || (*pnt == '-'))
					{
					if(*pnt != '-')
						{
						acc *= 10;
						acc += (*pnt - '0');
						}
						else
						{
						sgna = -1;
						}
					pnt++;
					}

				*msb = acc * sgna;
				if(!col_last)
					{
					*lsb = acc;
					}
				else
					{
					pnt = col_last + 1;
					acc = 0;
					while(isdigit((int)(unsigned char)*pnt) || (*pnt == '-'))
						{
						if(*pnt != '-')
							{
							acc *= 10;
							acc += (*pnt - '0');
							}
							else
							{
							sgnb = -1;
							}
						pnt++;
						}
					*lsb = acc * sgnb;
					}
				}

			if(lb_last)
				{
				*lb_last = 0;
				if((lb_last - s) < (*namlen))
					{
					*namlen = lb_last - s;
					}
				}
			*nam = s;

			h->u.var.svt_workspace = svt;
			h->u.var.sdt_workspace = sdt;
			h->u.var.sxt_workspace = sxt;

			return(h);
                        break;

		case FST_HT_ATTRBEGIN:	/* currently ignored for most cases except VHDL variable vartype/datatype creation */
			if(h->u.attr.typ == FST_AT_MISC)
				{
				if(h->u.attr.subtype == FST_MT_SUPVAR)
					{
					if(h->u.attr.name)
						{
						JRB subvar_jrb_node;
						char *attr_pnt;

						if(GLOBALS->fst_filetype == FST_FT_VHDL)
							{
							char *lc_p = attr_pnt = strdup_2(h->u.attr.name);

							while(*lc_p)
								{
								*lc_p = tolower(*lc_p); /* convert attrib name to lowercase for VHDL */
								lc_p++;
								}
							}
							else
							{
							attr_pnt = NULL;
							}

						/* sxt points to actual type name specified in FST file */
						subvar_jrb_node = jrb_find_str(GLOBALS->subvar_jrb, attr_pnt ? attr_pnt : h->u.attr.name);
						if(subvar_jrb_node)
							{
							sxt = subvar_jrb_node->val.ui;
							}
							else
							{
							Jval jv;

							if(GLOBALS->subvar_jrb_count != WAVE_VARXT_MAX_ID)
								{
								sxt = jv.ui = ++GLOBALS->subvar_jrb_count;
								/* subvar_jrb_node = */ jrb_insert_str(GLOBALS->subvar_jrb, strdup_2(attr_pnt ? attr_pnt : h->u.attr.name), jv);
								}
								else
								{
								sxt = 0;
								if(!GLOBALS->subvar_jrb_count_locked)
									{
									fprintf(stderr, FST_RDLOAD"Max number (%d) of type attributes reached, please increase WAVE_VARXT_MAX_ID.\n", WAVE_VARXT_MAX_ID);
									GLOBALS->subvar_jrb_count_locked = 1;
									}
								}
							}

						if(attr_pnt)
							{
							free_2(attr_pnt);
							}
						}

					svt = h->u.attr.arg >> FST_SDT_SVT_SHIFT_COUNT;
					sdt = h->u.attr.arg & (FST_SDT_ABS_MAX-1);
					GLOBALS->supplemental_datatypes_encountered = 1;
					GLOBALS->supplemental_vartypes_encountered |= ((svt != FST_SVT_NONE) && (svt != FST_SVT_VHDL_SIGNAL));
					}
				else
				if(h->u.attr.subtype == FST_MT_SOURCEISTEM)
					{
					uint32_t istem_path_number = (uint32_t)h->u.attr.arg_from_name;
					uint32_t istem_line_number = (uint32_t)h->u.attr.arg;

					if(istem_path_number <= GLOBALS->stem_path_string_table_siz) /* prevent overflows from malformed writers */
						{
						GLOBALS->istem_valid = 1;
						if(!GLOBALS->istem_struct_base)
							{
							GLOBALS->istem_struct_base_siz_alloc = 1;
							GLOBALS->istem_struct_base_siz = 0;
							GLOBALS->istem_struct_base = malloc_2(GLOBALS->istem_struct_base_siz_alloc * sizeof(struct stem_struct_t));
							}

						if(GLOBALS->istem_struct_base_siz == GLOBALS->istem_struct_base_siz_alloc)
							{
							GLOBALS->istem_struct_base_siz_alloc *= 2;
							GLOBALS->istem_struct_base = realloc_2(GLOBALS->istem_struct_base, GLOBALS->istem_struct_base_siz_alloc * sizeof(struct stem_struct_t));
							}

						GLOBALS->istem_struct_base[GLOBALS->istem_struct_base_siz].stem_idx = istem_path_number - 1;
						GLOBALS->istem_struct_base[GLOBALS->istem_struct_base_siz].stem_line_number = istem_line_number;
						GLOBALS->istem_struct_base_siz++;
						}
					}
				else
				if(h->u.attr.subtype == FST_MT_SOURCESTEM)
					{
					uint32_t stem_path_number = (uint32_t)h->u.attr.arg_from_name;
					uint32_t stem_line_number = (uint32_t)h->u.attr.arg;

					if(stem_path_number <= GLOBALS->stem_path_string_table_siz) /* prevent overflows from malformed writers */
						{
						GLOBALS->stem_valid = 1;
						if(!GLOBALS->stem_struct_base)
							{
							GLOBALS->stem_struct_base_siz_alloc = 1;
							GLOBALS->stem_struct_base_siz = 0;
							GLOBALS->stem_struct_base = malloc_2(GLOBALS->stem_struct_base_siz_alloc * sizeof(struct stem_struct_t));
							}

						if(GLOBALS->stem_struct_base_siz == GLOBALS->stem_struct_base_siz_alloc)
							{
							GLOBALS->stem_struct_base_siz_alloc *= 2;
							GLOBALS->stem_struct_base = realloc_2(GLOBALS->stem_struct_base, GLOBALS->stem_struct_base_siz_alloc * sizeof(struct stem_struct_t));
							}

						GLOBALS->stem_struct_base[GLOBALS->stem_struct_base_siz].stem_idx = stem_path_number - 1;
						GLOBALS->stem_struct_base[GLOBALS->stem_struct_base_siz].stem_line_number = stem_line_number;
						GLOBALS->stem_struct_base_siz++;
						}
					}
				else
				if(h->u.attr.subtype == FST_MT_PATHNAME)
					{
					if(h->u.attr.name && ((GLOBALS->stem_path_string_table_siz+1) == h->u.attr.arg))
						{
						/* == check against h->u.attr.arg is a sanity check against the writer */
						if(!GLOBALS->stem_path_string_table)
							{
							GLOBALS->stem_path_string_table_alloc = 1;
							GLOBALS->stem_path_string_table_siz = 0;
							GLOBALS->stem_path_string_table = malloc_2(GLOBALS->stem_path_string_table_alloc * sizeof(char *));
							}

						if(GLOBALS->stem_path_string_table_siz == GLOBALS->stem_path_string_table_alloc)
							{
							GLOBALS->stem_path_string_table_alloc *= 2;
							GLOBALS->stem_path_string_table = realloc_2(GLOBALS->stem_path_string_table, GLOBALS->stem_path_string_table_alloc * sizeof(char *));
							}

						GLOBALS->stem_path_string_table[GLOBALS->stem_path_string_table_siz] = strdup_2(h->u.attr.name);
						GLOBALS->stem_path_string_table_siz++;
						}
					}
				else
				if(h->u.attr.subtype == FST_MT_VALUELIST)
					{
					if(h->u.attr.name)
						{
						/* format is concatenations of [m b xs xe valstring] */
						if(GLOBALS->fst_synclock_str) 
							{
							free_2(GLOBALS->fst_synclock_str);			
							}
						GLOBALS->fst_synclock_str = strdup_2(h->u.attr.name);
						}
					}
				else
				if(h->u.attr.subtype == FST_MT_ENUMTABLE)
					{
					if(h->u.attr.name)
						{
						GLOBALS->queued_xl_enum_filter = h->u.attr.arg; /* consumed by next enum variable definition */

						if(h->u.attr.name_length)
							{
							struct fstETab *fe = fstUtilityExtractEnumTableFromString(h->u.attr.name);
							/* currently fe->name is unused */
							if(fe)
								{
								uint32_t ie;
#ifdef _WAVE_HAVE_JUDY
								Pvoid_t e = (Pvoid_t) NULL;
								for(ie=0; ie<fe->elem_count; ie++)
									{
									PPvoid_t pv = JudyHSIns(&e, fe->val_arr[ie], strlen(fe->val_arr[ie]), NULL);
									if(*pv)
										{
										free_2(*pv);
										}
									*pv = (void *)strdup_2(fe->literal_arr[ie]);
									}
#else
								struct xl_tree_node *e = NULL;

								for(ie=0; ie<fe->elem_count; ie++)
									{
									e = xl_insert(fe->val_arr[ie], e, fe->literal_arr[ie]);
									}
#endif

								if(GLOBALS->xl_enum_filter)
									{
									GLOBALS->num_xl_enum_filter++;
									GLOBALS->xl_enum_filter = realloc_2(GLOBALS->xl_enum_filter, GLOBALS->num_xl_enum_filter * sizeof(struct xl_tree_node *));
									}
									else
									{
									GLOBALS->num_xl_enum_filter++;
									GLOBALS->xl_enum_filter = malloc_2(sizeof(struct xl_tree_node *));
									}

								GLOBALS->xl_enum_filter[GLOBALS->num_xl_enum_filter-1] = e;

								if((unsigned int)GLOBALS->num_xl_enum_filter != h->u.attr.arg)
									{
									fprintf(stderr, FST_RDLOAD"Internal error, nonsequential enum tables definition encountered, exiting.\n");
									exit(255);
									}

								fstUtilityFreeEnumTable(fe);
								}
							}
						}
					}
				}
			break;

		case FST_HT_ATTREND:
			break;

		default:
			break;
                }
        }

*namlen = 0;
*nam = NULL;
return(NULL);
}


static void fst_append_graft_chain(int len, char *nam, int which, struct tree *par)
{
struct tree *t = talloc_2(sizeof(struct tree) + len + 1);

memcpy(t->name, nam, len+1);
t->t_which = which;

t->child = par;
t->next = GLOBALS->terminals_tchain_tree_c_1;
GLOBALS->terminals_tchain_tree_c_1 = t;
}


/*
 * mainline
 */
TimeType fst_main(char *fname, char *skip_start, char *skip_end)
{
int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;
signed char scale;
int numalias = 0;
int numvars = 0;
struct symbol *sym_block = NULL;
struct Node *node_block = NULL;
struct fstHier *h = NULL;
int msb, lsb;
char *nnam = NULL;
uint32_t activity_idx, num_activity_changes;
struct tree *npar = NULL;
char **f_name = NULL;
int   *f_name_len = NULL, *f_name_max_len = NULL;
int allowed_to_autocoalesce;
unsigned int nnam_max = 0;

int f_name_build_buf_len = 128;
char *f_name_build_buf = malloc_2(f_name_build_buf_len + 1);

GLOBALS->fst_fst_c_1 = fstReaderOpen(fname);
if(!GLOBALS->fst_fst_c_1)
        {
	return(LLDescriptor(0));	/* look at GLOBALS->fst_fst_c_1 in caller for success status... */
        }
/* SPLASH */                            splash_create();

allowed_to_autocoalesce = (strstr(fstReaderGetVersionString(GLOBALS->fst_fst_c_1), "Icarus") == NULL);
if(!allowed_to_autocoalesce)
	{
	GLOBALS->autocoalesce = 0;
	}

scale=(signed char)fstReaderGetTimescale(GLOBALS->fst_fst_c_1);
exponent_to_time_scale(scale);

f_name = calloc_2(F_NAME_MODULUS+1,sizeof(char *));
f_name_len = calloc_2(F_NAME_MODULUS+1,sizeof(int));
f_name_max_len = calloc_2(F_NAME_MODULUS+1,sizeof(int));

nnam_max = 16;
nnam = malloc_2(nnam_max + 1);

GLOBALS->fst_filetype = fstReaderGetFileType(GLOBALS->fst_fst_c_1);
if(GLOBALS->fst_filetype == FST_FT_VHDL)
	{
	GLOBALS->is_vhdl_component_format = 1;
	}

GLOBALS->subvar_jrb = make_jrb(); /* only used for attributes such as generated in VHDL, etc. */
GLOBALS->synclock_jrb = make_jrb(); /* only used for synthetic clocks */

GLOBALS->numfacs=fstReaderGetVarCount(GLOBALS->fst_fst_c_1);
GLOBALS->mvlfacs_fst_c_3=(struct fac *)calloc_2(GLOBALS->numfacs,sizeof(struct fac));
GLOBALS->fst_table_fst_c_1=(struct lx2_entry *)calloc_2(GLOBALS->numfacs, sizeof(struct lx2_entry));
sym_block = (struct symbol *)calloc_2(GLOBALS->numfacs, sizeof(struct symbol));
node_block=(struct Node *)calloc_2(GLOBALS->numfacs,sizeof(struct Node));
GLOBALS->facs=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));
GLOBALS->mvlfacs_fst_alias = calloc_2(GLOBALS->numfacs,sizeof(fstHandle));
GLOBALS->mvlfacs_fst_rvs_alias = calloc_2(GLOBALS->numfacs,sizeof(fstHandle));

if(!GLOBALS->fast_tree_sort)
	{
	GLOBALS->do_hier_compress = 0;
	}
	else
	{
	hier_auto_enable(); /* enable if greater than threshold */
	}

init_facility_pack();

fprintf(stderr, FST_RDLOAD"Processing %d facs.\n", GLOBALS->numfacs);
/* SPLASH */                            splash_sync(1, 5);

GLOBALS->first_cycle_fst_c_3 = (TimeType) fstReaderGetStartTime(GLOBALS->fst_fst_c_1) * GLOBALS->time_scale;
GLOBALS->last_cycle_fst_c_3 = (TimeType) fstReaderGetEndTime(GLOBALS->fst_fst_c_1) * GLOBALS->time_scale;
GLOBALS->total_cycles_fst_c_3 = GLOBALS->last_cycle_fst_c_3 - GLOBALS->first_cycle_fst_c_3 + 1;
GLOBALS->global_time_offset = fstReaderGetTimezero(GLOBALS->fst_fst_c_1) * GLOBALS->time_scale;

/* blackout region processing */
num_activity_changes = fstReaderGetNumberDumpActivityChanges(GLOBALS->fst_fst_c_1);
for(activity_idx = 0; activity_idx < num_activity_changes; activity_idx++)
	{
	uint32_t activity_idx2;
	uint64_t ct = fstReaderGetDumpActivityChangeTime(GLOBALS->fst_fst_c_1, activity_idx);
	unsigned char ac = fstReaderGetDumpActivityChangeValue(GLOBALS->fst_fst_c_1, activity_idx);

	if(ac == 1) continue;
	if((activity_idx+1) == num_activity_changes)
		{
		struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));
		bt->bstart = (TimeType)(ct * GLOBALS->time_scale);
		bt->bend = (TimeType)(GLOBALS->last_cycle_fst_c_3 * GLOBALS->time_scale);
                bt->next = GLOBALS->blackout_regions;

                GLOBALS->blackout_regions = bt;

		/* activity_idx = activity_idx2; */ /* scan-build says is dead + assigned garbage value , which is true : code does not need to mirror for() loop below */
		break;
		}

	for(activity_idx2 = activity_idx+1; activity_idx2 < num_activity_changes; activity_idx2++)
		{
		uint64_t ct2 = fstReaderGetDumpActivityChangeTime(GLOBALS->fst_fst_c_1, activity_idx2);
		ac = fstReaderGetDumpActivityChangeValue(GLOBALS->fst_fst_c_1, activity_idx2);
		if((ac == 0) && (activity_idx2 == (num_activity_changes-1)))
			{
			ac = 1;
			ct2 = GLOBALS->last_cycle_fst_c_3;
			}

		if(ac == 1)
			{
			struct blackout_region_t *bt = calloc_2(1, sizeof(struct blackout_region_t));
			bt->bstart = (TimeType)(ct * GLOBALS->time_scale);
			bt->bend = (TimeType)(ct2 * GLOBALS->time_scale);
	                bt->next = GLOBALS->blackout_regions;

	                GLOBALS->blackout_regions = bt;

			activity_idx = activity_idx2;
			break;
			}
		}

	}


/* do your stuff here..all useful info has been initialized by now */

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

for(i=0;i<GLOBALS->numfacs;i++)
        {
	char buf[65537];
	char *str;
	struct fac *f;
	int hier_len, name_len, tlen;
	unsigned char nvt, nvd, ndt;
	int longest_nam_candidate = 0;
	char *fnam;
	int len_subst = 0;

	h = extractNextVar(GLOBALS->fst_fst_c_1, &msb, &lsb, &nnam, &name_len, &nnam_max);
	if(!h)
		{
		/* this should never happen */
		fstReaderIterateHierRewind(GLOBALS->fst_fst_c_1);
		h = extractNextVar(GLOBALS->fst_fst_c_1, &msb, &lsb, &nnam, &name_len, &nnam_max);
		if(!h)
			{
		        fprintf(stderr, FST_RDLOAD"Exiting, missing or malformed names table encountered.\n");
        		exit(255);
			}
		}

	npar = GLOBALS->mod_tree_parent;
	hier_len = GLOBALS->fst_scope_name ? GLOBALS->fst_scope_name_len : 0;
	if(hier_len)
		{
		tlen = hier_len + 1 + name_len;
		if(tlen > f_name_max_len[i&F_NAME_MODULUS])
			{
			if(f_name[i&F_NAME_MODULUS]) free_2(f_name[i&F_NAME_MODULUS]);
			f_name_max_len[i&F_NAME_MODULUS] = tlen;
			fnam = malloc_2(tlen + 1);
			}
			else
			{
			fnam = f_name[i&F_NAME_MODULUS];
			}

		memcpy(fnam, GLOBALS->fst_scope_name, hier_len);
		fnam[hier_len] = GLOBALS->hier_delimeter;
		memcpy(fnam + hier_len + 1, nnam, name_len + 1);
		}
		else
		{
		tlen = name_len;
		if(tlen > f_name_max_len[i&F_NAME_MODULUS])
			{
			if(f_name[i&F_NAME_MODULUS]) free_2(f_name[i&F_NAME_MODULUS]);
			f_name_max_len[i&F_NAME_MODULUS] = tlen;
			fnam = malloc_2(tlen + 1);
			}
			else
			{
			fnam = f_name[i&F_NAME_MODULUS];
			}

		memcpy(fnam, nnam, name_len + 1);
		}

	f_name[i&F_NAME_MODULUS] = fnam;
	f_name_len[i&F_NAME_MODULUS] = tlen;

        if((h->u.var.length > 1) && (msb == -1) && (lsb == -1))
		{
		node_block[i].msi = h->u.var.length - 1;
		node_block[i].lsi = 0;
		}
		else
		{
		unsigned int abslen = (msb >= lsb) ? (msb - lsb + 1) : (lsb - msb + 1);

		if((h->u.var.length > abslen) && !(h->u.var.length % abslen)) /* check if 2d array */
			{
			/* printf("h->u.var.length: %d, abslen: %d '%s'\n", h->u.var.length, abslen, fnam); */
			len_subst = 1;
			}

		node_block[i].msi = msb;
		node_block[i].lsi = lsb;
		}
	GLOBALS->mvlfacs_fst_c_3[i].len = h->u.var.length;

	if(h->u.var.length)
		{
		switch(h->u.var.direction)
			{
			case FST_VD_INPUT:		nvd = ND_DIR_IN; GLOBALS->nonimplicit_direction_encountered = 1; break;
			case FST_VD_OUTPUT:		nvd = ND_DIR_OUT; GLOBALS->nonimplicit_direction_encountered = 1; break;
			case FST_VD_INOUT:		nvd = ND_DIR_INOUT; GLOBALS->nonimplicit_direction_encountered = 1; break;
			case FST_VD_BUFFER:		nvd = ND_DIR_BUFFER; GLOBALS->nonimplicit_direction_encountered = 1; break;
			case FST_VD_LINKAGE:		nvd = ND_DIR_LINKAGE; GLOBALS->nonimplicit_direction_encountered = 1; break;

			case FST_VD_IMPLICIT:
			default:			nvd = ND_DIR_IMPLICIT; break;
			}

		switch(h->u.var.typ)
			{
	                case FST_VT_VCD_EVENT: 		nvt = ND_VCD_EVENT; break;
	                case FST_VT_VCD_INTEGER: 	nvt = ND_VCD_INTEGER; break;
	                case FST_VT_VCD_PARAMETER: 	nvt = ND_VCD_PARAMETER; break;
	                case FST_VT_VCD_REAL: 		nvt = ND_VCD_REAL; break;
	                case FST_VT_VCD_REAL_PARAMETER: nvt = ND_VCD_REAL_PARAMETER; break;
	                case FST_VT_VCD_REALTIME:       nvt = ND_VCD_REALTIME; break;
	                case FST_VT_VCD_REG: 		nvt = ND_VCD_REG; break;
	                case FST_VT_VCD_SUPPLY0: 	nvt = ND_VCD_SUPPLY0; break;
	                case FST_VT_VCD_SUPPLY1: 	nvt = ND_VCD_SUPPLY1; break;
	                case FST_VT_VCD_TIME: 		nvt = ND_VCD_TIME; break;
	                case FST_VT_VCD_TRI: 		nvt = ND_VCD_TRI; break;
	                case FST_VT_VCD_TRIAND: 	nvt = ND_VCD_TRIAND; break;
	                case FST_VT_VCD_TRIOR: 		nvt = ND_VCD_TRIOR; break;
	                case FST_VT_VCD_TRIREG: 	nvt = ND_VCD_TRIREG; break;
	                case FST_VT_VCD_TRI0: 		nvt = ND_VCD_TRI0; break;
	                case FST_VT_VCD_TRI1: 		nvt = ND_VCD_TRI1; break;
	                case FST_VT_VCD_WAND: 		nvt = ND_VCD_WAND; break;
	                case FST_VT_VCD_WIRE: 		nvt = ND_VCD_WIRE; break;
	                case FST_VT_VCD_WOR: 		nvt = ND_VCD_WOR; break;
	                case FST_VT_VCD_PORT: 		nvt = ND_VCD_PORT; break;

	                case FST_VT_GEN_STRING:		nvt = ND_GEN_STRING; break;

			case FST_VT_SV_BIT:             nvt = ND_SV_BIT; break;
			case FST_VT_SV_LOGIC:           nvt = ND_SV_LOGIC; break;
			case FST_VT_SV_INT:             nvt = ND_SV_INT; break;
			case FST_VT_SV_SHORTINT:        nvt = ND_SV_SHORTINT; break;
			case FST_VT_SV_LONGINT:         nvt = ND_SV_LONGINT; break;
			case FST_VT_SV_BYTE:            nvt = ND_SV_BYTE; break;
			case FST_VT_SV_ENUM:            nvt = ND_SV_ENUM; break;
			case FST_VT_SV_SHORTREAL:       nvt = ND_SV_SHORTREAL; break;

			default: 			nvt = ND_UNSPECIFIED_DEFAULT; break;
			}

		switch(h->u.var.typ)
			{
			case FST_VT_VCD_PARAMETER:
			case FST_VT_VCD_INTEGER:
			case FST_VT_SV_INT:
			case FST_VT_SV_SHORTINT:
			case FST_VT_SV_LONGINT:
				GLOBALS->mvlfacs_fst_c_3[i].flags = VZT_RD_SYM_F_INTEGER;
				break;

			case FST_VT_VCD_REAL:
			case FST_VT_VCD_REAL_PARAMETER:
			case FST_VT_VCD_REALTIME:
			case FST_VT_SV_SHORTREAL:
				GLOBALS->mvlfacs_fst_c_3[i].flags = VZT_RD_SYM_F_DOUBLE;
				break;

			case FST_VT_GEN_STRING:
				GLOBALS->mvlfacs_fst_c_3[i].flags = VZT_RD_SYM_F_STRING;
				GLOBALS->mvlfacs_fst_c_3[i].len = 2;
				break;

			default:
				GLOBALS->mvlfacs_fst_c_3[i].flags = VZT_RD_SYM_F_BITS;
				break;
			}
		}
		else /* convert any variable length records into strings */
		{
		nvt = ND_GEN_STRING;
		nvd = ND_DIR_IMPLICIT;
		GLOBALS->mvlfacs_fst_c_3[i].flags = VZT_RD_SYM_F_STRING;
		GLOBALS->mvlfacs_fst_c_3[i].len = 2;
		}

	if(GLOBALS->fst_synclock_str)
		{
		if(GLOBALS->mvlfacs_fst_c_3[i].len == 1) /* currently only for single bit signals */
			{
			Jval syn_jv;

			GLOBALS->mvlfacs_fst_c_3[i].flags |= VZT_RD_SYM_F_SYNVEC; /* special meaning for this in FST loader--means synthetic signal! */
			syn_jv.s = GLOBALS->fst_synclock_str;
			jrb_insert_int(GLOBALS->synclock_jrb, i, syn_jv);
			}
			else
			{
			free_2(GLOBALS->fst_synclock_str);
			}

		GLOBALS->fst_synclock_str = NULL; /* under malloc_2() control for true if() branch, so not lost */
		}

	if(h->u.var.is_alias)
		{
		GLOBALS->mvlfacs_fst_c_3[i].node_alias = h->u.var.handle - 1; /* subtract 1 to scale it with gtkwave-style numbering */
		GLOBALS->mvlfacs_fst_c_3[i].flags |= VZT_RD_SYM_F_ALIAS;
		numalias++;
		}
	else
		{
		GLOBALS->mvlfacs_fst_rvs_alias[numvars] = i;
		GLOBALS->mvlfacs_fst_c_3[i].node_alias = numvars;
		numvars++;
		}

	f=GLOBALS->mvlfacs_fst_c_3+i;

	if((f->len>1)&& (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) )
		{
		int len=sprintf_2_sdd(buf, f_name[(i)&F_NAME_MODULUS],node_block[i].msi, node_block[i].lsi);

		if(len_subst) /* preserve 2d in name, but make 1d internally */
			{
			node_block[i].msi = h->u.var.length-1;
			node_block[i].lsi = 0;
			}

		longest_nam_candidate = len;

		if(!GLOBALS->do_hier_compress)
			{
			str=malloc_2(len+1);
			}
			else
			{
			if(len > f_name_build_buf_len)
				{
				free_2(f_name_build_buf); f_name_build_buf = malloc_2((f_name_build_buf_len=len)+1);
				}
			str = f_name_build_buf;
			}

		if(!GLOBALS->alt_hier_delimeter)
			{
			memcpy(str, buf, len+1);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
		s=&sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		prevsymroot = prevsym = NULL;

		if(GLOBALS->fast_tree_sort)
			{
			len = sprintf_2_sdd(buf, nnam,node_block[i].msi, node_block[i].lsi);
			fst_append_graft_chain(len, buf, i, npar);
			}
		}
	else
		{
		int gatecmp = (f->len==1) && (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) && (node_block[i].msi!=-1) && (node_block[i].lsi!=-1);
		int revcmp = gatecmp && (i) && (f_name_len[(i)&F_NAME_MODULUS] == f_name_len[(i-1)&F_NAME_MODULUS]) && (!memrevcmp(f_name_len[(i)&F_NAME_MODULUS], f_name[(i)&F_NAME_MODULUS], f_name[(i-1)&F_NAME_MODULUS]));

		if(gatecmp)
			{
			int len = sprintf_2_sd(buf, f_name[(i)&F_NAME_MODULUS],node_block[i].msi);

			longest_nam_candidate = len;
			if(!GLOBALS->do_hier_compress)
				{
				str=malloc_2(len+1);
				}
				else
				{
				if(len > f_name_build_buf_len)
					{
					free_2(f_name_build_buf); f_name_build_buf = malloc_2((f_name_build_buf_len=len)+1);
					}
				str = f_name_build_buf;
				}

			if(!GLOBALS->alt_hier_delimeter)
				{
				memcpy(str, buf, len+1);
				}
				else
				{
				strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
				}
			s=&sym_block[i];
		        symadd_name_exists_sym_exists(s,str,0);
			if((allowed_to_autocoalesce)&&(prevsym)&&(revcmp)&&(!strchr(f_name[(i)&F_NAME_MODULUS], '\\')))	/* allow chaining for search functions.. */
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

			if(GLOBALS->fast_tree_sort)
				{
				len = sprintf_2_sd(buf, nnam,node_block[i].msi);
				fst_append_graft_chain(len, buf, i, npar);
				}
			}
			else
			{
			int len = f_name_len[(i)&F_NAME_MODULUS];

			longest_nam_candidate = len;
			if(!GLOBALS->do_hier_compress)
				{
				str=malloc_2(len+1);
				}
				else
				{
				if(len > f_name_build_buf_len)
					{
					free_2(f_name_build_buf); f_name_build_buf = malloc_2((f_name_build_buf_len=len)+1);
					}
				str = f_name_build_buf;
				}

			if(!GLOBALS->alt_hier_delimeter)
				{
				memcpy(str, f_name[(i)&F_NAME_MODULUS], len+1);
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
				if(f->len != 0)
					{
					node_block[i].msi = f->len - 1;
					node_block[i].lsi = 0;
					GLOBALS->mvlfacs_fst_c_3[i].len = f->len;
					}
					else
					{
					node_block[i].msi = 31;
					node_block[i].lsi = 0;
					GLOBALS->mvlfacs_fst_c_3[i].len = 32;
					}
			}

			if(GLOBALS->fast_tree_sort)
				{
				fst_append_graft_chain(strlen(nnam), nnam, i, npar);
				}
			}
		}

        if(longest_nam_candidate > GLOBALS->longestname) GLOBALS->longestname = longest_nam_candidate;

        GLOBALS->facs[i]=&sym_block[i];
        n=&node_block[i];

	if(GLOBALS->queued_xl_enum_filter)
		{
		Jval jv;
		jv.ui = GLOBALS->queued_xl_enum_filter;
		GLOBALS->queued_xl_enum_filter = 0;

		if(!GLOBALS->enum_nptrs_jrb) GLOBALS->enum_nptrs_jrb = make_jrb();
		jrb_insert_vptr(GLOBALS->enum_nptrs_jrb, n, jv);
		}

	if(GLOBALS->do_hier_compress)
		{
		n->nname = compress_facility((unsigned char *)s->name, longest_nam_candidate);
		/* free_2(s->name); ...removed as f_name_build_buf is now used */
		s->name = n->nname;
		}
		else
		{
		n->nname=s->name;
		}

        n->mv.mvlfac = GLOBALS->mvlfacs_fst_c_3+i;
	GLOBALS->mvlfacs_fst_c_3[i].working_node = n;
	n->vardir = nvd;
	n->varxt  = h->u.var.sxt_workspace;
	if((h->u.var.svt_workspace == FST_SVT_NONE) && (h->u.var.sdt_workspace == FST_SDT_NONE))
		{
		n->vartype = nvt;
		}
		else
		{
		switch(h->u.var.svt_workspace)
			{
			case FST_SVT_VHDL_SIGNAL:	nvt = ND_VHDL_SIGNAL; break;
			case FST_SVT_VHDL_VARIABLE:	nvt = ND_VHDL_VARIABLE; break;
			case FST_SVT_VHDL_CONSTANT:	nvt = ND_VHDL_CONSTANT; break;
			case FST_SVT_VHDL_FILE:		nvt = ND_VHDL_FILE; break;
			case FST_SVT_VHDL_MEMORY:	nvt = ND_VHDL_MEMORY; break;
			default:			break; /* keep what exists */
			}
		n->vartype = nvt;

		switch(h->u.var.sdt_workspace)
			{
			case FST_SDT_VHDL_BOOLEAN:		ndt = ND_VDT_VHDL_BOOLEAN; break;
			case FST_SDT_VHDL_BIT:			ndt = ND_VDT_VHDL_BIT; break;
			case FST_SDT_VHDL_BIT_VECTOR:   	ndt = ND_VDT_VHDL_BIT_VECTOR; break;
			case FST_SDT_VHDL_STD_ULOGIC:   	ndt = ND_VDT_VHDL_STD_ULOGIC; break;
			case FST_SDT_VHDL_STD_ULOGIC_VECTOR: 	ndt = ND_VDT_VHDL_STD_ULOGIC_VECTOR; break;
			case FST_SDT_VHDL_STD_LOGIC:       	ndt = ND_VDT_VHDL_STD_LOGIC; break;
			case FST_SDT_VHDL_STD_LOGIC_VECTOR:  	ndt = ND_VDT_VHDL_STD_LOGIC_VECTOR; break;
			case FST_SDT_VHDL_UNSIGNED:          	ndt = ND_VDT_VHDL_UNSIGNED; break;
			case FST_SDT_VHDL_SIGNED:            	ndt = ND_VDT_VHDL_SIGNED; break;
			case FST_SDT_VHDL_INTEGER:           	ndt = ND_VDT_VHDL_INTEGER; break;
			case FST_SDT_VHDL_REAL:              	ndt = ND_VDT_VHDL_REAL; break;
			case FST_SDT_VHDL_NATURAL:           	ndt = ND_VDT_VHDL_NATURAL; break;
			case FST_SDT_VHDL_POSITIVE:          	ndt = ND_VDT_VHDL_POSITIVE; break;
			case FST_SDT_VHDL_TIME:              	ndt = ND_VDT_VHDL_TIME; break;
			case FST_SDT_VHDL_CHARACTER:         	ndt = ND_VDT_VHDL_CHARACTER; break;
			case FST_SDT_VHDL_STRING:            	ndt = ND_VDT_VHDL_STRING; break;
			default:				ndt = ND_VDT_NONE; break;
			}

		n->vardt = ndt;
		}

	if((f->len>1)||(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
		{
		n->extvals = 1;
		}

        n->head.time=-1;        /* mark 1st node as negative time */
        n->head.v.h_val=AN_X;
        s->n=n;
        }			/* for(i) of facs parsing */


if(f_name_max_len) { free_2(f_name_max_len); f_name_max_len = NULL; }
if(nnam) { free_2(nnam); nnam = NULL; }
if(f_name_build_buf) { free_2(f_name_build_buf); f_name_build_buf = NULL; }

for(i=0;i<=F_NAME_MODULUS;i++)
	{
	if(f_name[i])
		{
		free_2(f_name[i]);
		f_name[i] = NULL;
		}
	}
free_2(f_name); f_name = NULL;
free_2(f_name_len); f_name_len = NULL;

if(numvars != GLOBALS->numfacs) { GLOBALS->mvlfacs_fst_rvs_alias = realloc_2(GLOBALS->mvlfacs_fst_rvs_alias, numvars * sizeof(fstHandle)); }

if(GLOBALS->subvar_jrb_count) /* generate lookup table for typenames explicitly given as attributes */
	{
	JRB subvar_jrb_node = NULL;
	GLOBALS->subvar_pnt = calloc_2(GLOBALS->subvar_jrb_count + 1, sizeof(char *));

	jrb_traverse(subvar_jrb_node, GLOBALS->subvar_jrb)
		{
		GLOBALS->subvar_pnt[subvar_jrb_node->val.ui] = subvar_jrb_node->key.s;
		}
	}

if(GLOBALS->istem_struct_base)
	{
	if(GLOBALS->istem_struct_base_siz != GLOBALS->istem_struct_base_siz_alloc)
		{
		GLOBALS->istem_struct_base_siz_alloc = GLOBALS->istem_struct_base_siz;
		GLOBALS->istem_struct_base = realloc_2(GLOBALS->istem_struct_base, GLOBALS->istem_struct_base_siz_alloc * sizeof(struct stem_struct_t));
		}
	}

if(GLOBALS->stem_struct_base)
	{
	if(GLOBALS->stem_struct_base_siz != GLOBALS->stem_struct_base_siz_alloc)
		{
		GLOBALS->stem_struct_base_siz_alloc = GLOBALS->stem_struct_base_siz;
		GLOBALS->stem_struct_base = realloc_2(GLOBALS->stem_struct_base, GLOBALS->stem_struct_base_siz_alloc * sizeof(struct stem_struct_t));
		}
	}

if(GLOBALS->stem_path_string_table)
	{
	if(GLOBALS->stem_path_string_table_siz != GLOBALS->stem_path_string_table_alloc)
		{
		GLOBALS->stem_path_string_table_alloc = GLOBALS->stem_path_string_table_siz;
		GLOBALS->stem_path_string_table = realloc_2(GLOBALS->stem_path_string_table, GLOBALS->stem_path_string_table_alloc * sizeof(char *));
		}
	}

decorated_module_cleanup(); /* ...also now in gtk2_treesearch.c */
freeze_facility_pack();
iter_through_comp_name_table();

fprintf(stderr, FST_RDLOAD"Built %d signal%s and %d alias%s.\n",
	numvars, (numvars == 1) ? "" : "s",
	numalias, (numalias == 1) ? "" : "es");

GLOBALS->fst_maxhandle = numvars;

if(GLOBALS->fast_tree_sort)
        {
/* SPLASH */                            splash_sync(2, 5);
        fprintf(stderr, FST_RDLOAD"Building facility hierarchy tree.\n");

        init_tree();
        treegraft(&GLOBALS->treeroot);

/* SPLASH */                            splash_sync(3, 5);

        fprintf(stderr, FST_RDLOAD"Sorting facility hierarchy tree.\n");
        treesort(GLOBALS->treeroot, NULL);
/* SPLASH */                            splash_sync(4, 5);
        order_facs_from_treesort(GLOBALS->treeroot, &GLOBALS->facs);

/* SPLASH */                            splash_sync(5, 5);
        GLOBALS->facs_are_sorted=1;
        }
        else
	{
/* SPLASH */                            splash_sync(2, 5);
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst, ch;
		int esc = 0;

	        subst=GLOBALS->facs[i]->name;
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
	fprintf(stderr, FST_RDLOAD"Sorting facilities at hierarchy boundaries.\n");
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
	fprintf(stderr, FST_RDLOAD"Building facility hierarchy tree.\n");

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

#if 0
{
int num_dups = 0;
for(i=0;i<GLOBALS->numfacs-1;i++)
	{
	if(!strcmp(GLOBALS->facs[i]->name, GLOBALS->facs[i+1]->name))
		{
		fprintf(stderr, FST_RDLOAD"DUPLICATE FAC: '%s'\n", GLOBALS->facs[i]->name);
		num_dups++;
		}
	}

if(num_dups)
	{
	fprintf(stderr, FST_RDLOAD"Exiting, %d duplicate signals are present.\n", num_dups);
	exit(255);
	}
}
#endif

GLOBALS->min_time = GLOBALS->first_cycle_fst_c_3; GLOBALS->max_time=GLOBALS->last_cycle_fst_c_3;
GLOBALS->is_lx2 = LXT2_IS_FST;

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

	fstReaderSetLimitTimeRange(GLOBALS->fst_fst_c_1, b_start, b_end);
	GLOBALS->min_time = b_start;
	GLOBALS->max_time = b_end;
	}

fstReaderIterBlocksSetNativeDoublesOnCallback(GLOBALS->fst_fst_c_1, 1); /* to avoid bin -> ascii -> bin double swap */

/* SPLASH */                            splash_finalize();
return(GLOBALS->max_time);
}


/*
 * conversion from evcd -> vcd format
 */
static void evcd_memcpy(char *dst, const char *src, int len)
{
static const char *evcd="DUNZduLHXTlh01?FAaBbCcf";
static const char  *vcd="01xz0101xz0101xzxxxxxxz";

char ch;
int i, j;

for(j=0;j<len;j++)
        {
	ch=*src;
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
}


/*
 * fst callback (only does bits for now)
 */
static void fst_callback2(void *user_callback_data_pointer, uint64_t tim, fstHandle txidx, const unsigned char *value, uint32_t plen)
{
(void)user_callback_data_pointer;

fstHandle facidx = GLOBALS->mvlfacs_fst_rvs_alias[--txidx];
struct HistEnt *htemp;
struct lx2_entry *l2e = GLOBALS->fst_table_fst_c_1+facidx;
struct fac *f = GLOBALS->mvlfacs_fst_c_3+facidx;

GLOBALS->busycnt_fst_c_2++;
if(GLOBALS->busycnt_fst_c_2==WAVE_BUSY_ITER)
	{
	busy_window_refresh();
	GLOBALS->busycnt_fst_c_2 = 0;
	}

/* fprintf(stderr, "%lld %d '%s'\n", tim, facidx, value); */

if(!(f->flags&(VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
	{
	unsigned char vt = ND_UNSPECIFIED_DEFAULT;
	if(f->working_node)
		{
		vt = f->working_node->vartype;
		}

	if(f->len>1)
	        {
		char *h_vector = (char *)malloc_2(f->len);
		if(vt != ND_VCD_PORT)
			{
			memcpy(h_vector, value, f->len);
			}
			else
			{
			evcd_memcpy(h_vector, (const char *)value, f->len);
			}

		if((l2e->histent_curr)&&(l2e->histent_curr->v.h_vector)) /* remove duplicate values */
			{
			if((!memcmp(l2e->histent_curr->v.h_vector, h_vector, f->len))&&(!GLOBALS->vcd_preserve_glitches))
				{
				free_2(h_vector);
				return;
				}
			}

		htemp = histent_calloc();
		htemp->v.h_vector = h_vector;
	        }
	        else
	        {
		unsigned char h_val;

		if(vt != ND_VCD_PORT)
			{
			switch(*value)
				{
                                case '0':	h_val = AN_0; break;
                                case '1':	h_val = AN_1; break;
                                case 'X':
                                case 'x':	h_val = AN_X; break;
                                case 'Z':
                                case 'z':	h_val = AN_Z; break;
                                case 'H':
                                case 'h':	h_val = AN_H; break;
                                case 'U':
                                case 'u':	h_val = AN_U; break;
                                case 'W':
                                case 'w':	h_val = AN_W; break;
                                case 'L':
                                case 'l':	h_val = AN_L; break;
				case '-':	h_val = AN_DASH; break;

                                default:        h_val = AN_X; break;
				}
			}
			else
			{
			char membuf[1];
			evcd_memcpy(membuf, (const char *)value, 1);
			switch(*membuf)
				{
				case '0':	h_val = AN_0; break;
				case '1':	h_val = AN_1; break;
				case 'Z':
				case 'z':	h_val = AN_Z; break;
				default:	h_val = AN_X; break;
				}
			}

		if((vt != ND_VCD_EVENT) && (l2e->histent_curr)) /* remove duplicate values */
			{
			if((l2e->histent_curr->v.h_val == h_val) && (!GLOBALS->vcd_preserve_glitches))
				{
				return;
				}
			}

		htemp = histent_calloc();
		htemp->v.h_val = h_val;
	        }
	}
else if(f->flags&VZT_RD_SYM_F_DOUBLE)
	{
	if((l2e->histent_curr)&&(l2e->histent_curr->v.h_vector)) /* remove duplicate values */
		{
#ifdef WAVE_HAS_H_DOUBLE
		if(!memcmp(&l2e->histent_curr->v.h_double, value, sizeof(double)))
#else
		if(!memcmp(l2e->histent_curr->v.h_vector, value, sizeof(double)))
#endif
			{
			if((!GLOBALS->vcd_preserve_glitches)&&(!GLOBALS->vcd_preserve_glitches_real))
				{
				return;
				}
			}
		}

	/* if(fstReaderIterBlocksSetNativeDoublesOnCallback is disabled...)

	double *d = double_slab_calloc();
	sscanf(value, "%lg", d);
	htemp = histent_calloc();
	htemp->v.h_vector = (char *)d;

	otherwise...
	*/

	htemp = histent_calloc();
#ifdef WAVE_HAS_H_DOUBLE
	memcpy(&htemp->v.h_double, value, sizeof(double));
#else
	htemp->v.h_vector = double_slab_calloc();
	memcpy(htemp->v.h_vector, value, sizeof(double));
#endif
	htemp->flags = HIST_REAL;
	}
else	/* string */
	{
	unsigned char *s = malloc_2(plen + 1);
	uint32_t pidx;

	for(pidx=0;pidx<plen;pidx++)
		{
		unsigned char ch = value[pidx];

#if 0
		/* for now do not convert to printable unless done in vcd + lxt loaders also */
		if((ch < ' ') || (ch > '~'))
			{
			ch = '.';
			}
#endif

		s[pidx] = ch;
		}
	s[pidx] = 0;

	if((l2e->histent_curr)&&(l2e->histent_curr->v.h_vector)) /* remove duplicate values */
		{
		if((!strcmp(l2e->histent_curr->v.h_vector, (const char *)value)) && (!GLOBALS->vcd_preserve_glitches))
			{
			free(s);
			return;
			}
		}

	htemp = histent_calloc();
	htemp->v.h_vector = (char *)s;
	htemp->flags = HIST_REAL|HIST_STRING;
	}


htemp->time = (tim) * (GLOBALS->time_scale);

if(l2e->histent_curr) /* scan-build : was l2e->histent_head */
	{
	l2e->histent_curr->next = htemp; /* scan-build : this is ok given how it's used */
	l2e->histent_curr = htemp;
	}
	else
	{
	l2e->histent_head = l2e->histent_curr = htemp;
	}

l2e->numtrans++;
}


static void fst_callback(void *user_callback_data_pointer, uint64_t tim, fstHandle txidx, const unsigned char *value)
{
fst_callback2(user_callback_data_pointer, tim, txidx, value, 0);
}


/*
 * this is the black magic that handles aliased signals...
 */
static void fst_resolver(nptr np, nptr resolve)
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
 * actually import a fst trace but don't do it if it's already been imported
 */
void import_fst_trace(nptr np)
{
hptr htemp, htempx=NULL, histent_tail;
int len, i;
struct fac *f;
int txidx;
nptr nold = np;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f - GLOBALS->mvlfacs_fst_c_3;
if(np->mv.mvlfac->flags&VZT_RD_SYM_F_ALIAS)
	{
	txidx = GLOBALS->mvlfacs_fst_c_3[txidx].node_alias; /* this is to map to fstHandles, so even non-aliased are remapped */
	txidx = GLOBALS->mvlfacs_fst_rvs_alias[txidx];
	np = GLOBALS->mvlfacs_fst_c_3[txidx].working_node;

	if(!(f=np->mv.mvlfac))
		{
		fst_resolver(nold, np);
		return;	/* already imported */
		}
	}


if(!(f->flags&VZT_RD_SYM_F_SYNVEC)) /* block debug message for synclk */
	{
	int flagged = HIER_DEPACK_STATIC;
	char *str = hier_decompress_flagged(np->nname, &flagged);
	fprintf(stderr, "Import: %s\n", str); /* normally this never happens */
	}


/* new stuff */
len = np->mv.mvlfac->len;

/* check here for array height in future */

if(!(f->flags&VZT_RD_SYM_F_SYNVEC))
	{
	fstReaderSetFacProcessMask(GLOBALS->fst_fst_c_1, GLOBALS->mvlfacs_fst_c_3[txidx].node_alias+1);
	fstReaderIterBlocks2(GLOBALS->fst_fst_c_1, fst_callback, fst_callback2, NULL, NULL);
	fstReaderClrFacProcessMask(GLOBALS->fst_fst_c_1, GLOBALS->mvlfacs_fst_c_3[txidx].node_alias+1);
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
	if(!(f->flags&VZT_RD_SYM_F_DOUBLE))
		{
		if(!(f->flags&VZT_RD_SYM_F_STRING))
			{
			htemp->v.h_vector = (char *)malloc_2(len);
			for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_X;
			}
			else
			{
			htemp->v.h_vector = strdup_2("UNDEF");
			htemp->flags = HIST_REAL|HIST_STRING;
			}
		}
		else
		{
#ifdef WAVE_HAS_H_DOUBLE
		htemp->v.h_double = strtod("NaN", NULL);
#else
                double *d = malloc_2(sizeof(double));
                *d = strtod("NaN", NULL);
                htemp->v.h_vector = (char *)d;
#endif
                htemp->flags = HIST_REAL;
		}
        htempx = htemp;
	}
	else
	{
	htemp->v.h_val = AN_X;		/* x */
        htempx = htemp;
	}
htemp->time = MAX_HISTENT_TIME-1;
htemp->next = histent_tail;

if(GLOBALS->fst_table_fst_c_1[txidx].histent_curr)
	{
	GLOBALS->fst_table_fst_c_1[txidx].histent_curr->next = htemp;
	htemp = GLOBALS->fst_table_fst_c_1[txidx].histent_head;
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
        if(f->flags&VZT_RD_SYM_F_STRING)
		{
		np->head.flags |= HIST_STRING;
		}
        }

	{
        struct HistEnt *htemp2 = histent_calloc();
        htemp2->time = -1;
        if(len>1)
        	{
                htemp2->v.h_vector = htempx->v.h_vector;
		htemp2->flags = htempx->flags;
                }
                else
                {
                htemp2->v.h_val = htempx->v.h_val;
		}
	htemp2->next = htemp;
        htemp = htemp2;
        GLOBALS->fst_table_fst_c_1[txidx].numtrans++;
        }

np->head.time  = -2;
np->head.next = htemp;
np->numhist=GLOBALS->fst_table_fst_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

memset(GLOBALS->fst_table_fst_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

np->curr = histent_tail;
np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

if(nold!=np)
	{
	fst_resolver(nold, np);
	}
}


/*
 * decompress [m b xs xe valstring]... format string into trace
 */
static void expand_synvec(int txidx, const char *s)
{
char *scopy = NULL;
char *pnt, *pnt2;
double m, b;
uint64_t xs, xe, xi;
char *vs;
uint64_t tim;
uint64_t tim_max;
int vslen;
int vspnt;
unsigned char value[2] = {0, 0};
unsigned char pval = 0;

scopy = strdup_2(s);
vs = calloc_2(1, strlen(s) + 1); /* will never be as big as original string */
pnt = scopy;

while(*pnt)
	{
	if(*pnt != '[') { pnt++; continue; }
	pnt++;

	pnt2 = strchr(pnt, ']');
	if(!pnt2) break;
	*pnt2 = 0;

	/* printf("PNT: %s\n", pnt); */
	int rc = sscanf(pnt, "%lg %lg %"SCNu64" %"SCNu64" %s", &m, &b, &xs, &xe, vs);
	if(rc == 5)
		{
		vslen = strlen(vs);
		vspnt = 0;

		tim_max = 0;
		for(xi = xs; xi <= xe; xi++)
			{
			tim = (xi * m) + b;
			/* fprintf(stderr, "#%"PRIu64" '%c'\n", tim, vs[vspnt]); */
			value[0] = vs[vspnt];
			if(value[0] != pval) /* collapse new == old value transitions so new is ignored */
				{
				if((tim >= tim_max) || (xi == xs))
					{
					fst_callback2(NULL, tim, txidx, value, 0);
					tim_max = tim;
					}
				pval = value[0];
				}
			vspnt++; vspnt = (vspnt == vslen) ? 0 : vspnt; /* modulus on repeating clock */
			}
		}
		else
		{
		break;
		}

	pnt = pnt2 + 1;
	}

free_2(vs);
free_2(scopy);
}


/*
 * pre-import many traces at once so function above doesn't have to iterate...
 */
void fst_set_fac_process_mask(nptr np)
{
struct fac *f;
int txidx;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f-GLOBALS->mvlfacs_fst_c_3;

if(np->mv.mvlfac->flags&VZT_RD_SYM_F_ALIAS)
	{
	txidx = GLOBALS->mvlfacs_fst_c_3[txidx].node_alias;
	txidx = GLOBALS->mvlfacs_fst_rvs_alias[txidx];
	np = GLOBALS->mvlfacs_fst_c_3[txidx].working_node;

	if(!(np->mv.mvlfac)) return;	/* already imported */
	}

if(np->mv.mvlfac->flags&VZT_RD_SYM_F_SYNVEC)
	{
	JRB fi = jrb_find_int(GLOBALS->synclock_jrb, txidx);
	if(fi)
		{
		expand_synvec(GLOBALS->mvlfacs_fst_c_3[txidx].node_alias+1, fi->val.s);
		import_fst_trace(np);
		return; /* import_fst_trace() will construct the trailer */
		}
	}

/* check here for array height in future */
	{
	fstReaderSetFacProcessMask(GLOBALS->fst_fst_c_1, GLOBALS->mvlfacs_fst_c_3[txidx].node_alias+1);
	GLOBALS->fst_table_fst_c_1[txidx].np = np;
	}
}


void fst_import_masked(void)
{
unsigned int txidxi;
int i, cnt;
hptr htempx = NULL;

cnt = 0;
for(txidxi=0;txidxi<GLOBALS->fst_maxhandle;txidxi++)
	{
	if(fstReaderGetFacProcessMask(GLOBALS->fst_fst_c_1, txidxi+1))
		{
		cnt++;
		}
	}

if(!cnt)
	{
	return;
	}

if(cnt>100)
	{
	fprintf(stderr, FST_RDLOAD"Extracting %d traces\n", cnt);
	}

set_window_busy(NULL);
fstReaderIterBlocks2(GLOBALS->fst_fst_c_1, fst_callback, fst_callback2, NULL, NULL);
set_window_idle(NULL);

for(txidxi=0;txidxi<GLOBALS->fst_maxhandle;txidxi++)
	{
	if(fstReaderGetFacProcessMask(GLOBALS->fst_fst_c_1, txidxi+1))
		{
		int txidx = GLOBALS->mvlfacs_fst_rvs_alias[txidxi];
		struct HistEnt *htemp, *histent_tail;
		struct fac *f = GLOBALS->mvlfacs_fst_c_3+txidx;
		int len = f->len;
		nptr np = GLOBALS->fst_table_fst_c_1[txidx].np;

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
			if(!(f->flags&VZT_RD_SYM_F_DOUBLE))
				{
				if(!(f->flags&VZT_RD_SYM_F_STRING))
					{
					htemp->v.h_vector = (char *)malloc_2(len);
					for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_X;
					}
					else
					{
					htemp->v.h_vector = strdup_2("UNDEF");
					htemp->flags = HIST_REAL|HIST_STRING;
					}
				htempx = htemp;
				}
				else
				{
#ifdef WAVE_HAS_H_DOUBLE
				htemp->v.h_double = strtod("NaN", NULL);
#else
				double *d = malloc_2(sizeof(double));

				*d = strtod("NaN", NULL);
				htemp->v.h_vector = (char *)d;
#endif
				htemp->flags = HIST_REAL;
				htempx = htemp;
				}
			}
			else
			{
			htemp->v.h_val = AN_X;		/* x */
			htempx = htemp;
			}
		htemp->time = MAX_HISTENT_TIME-1;
		htemp->next = histent_tail;

		if(GLOBALS->fst_table_fst_c_1[txidx].histent_curr)
			{
			GLOBALS->fst_table_fst_c_1[txidx].histent_curr->next = htemp;
			htemp = GLOBALS->fst_table_fst_c_1[txidx].histent_head;
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
		        if(f->flags&VZT_RD_SYM_F_STRING)
				{
				np->head.flags |= HIST_STRING;
				}
		        }

                        {
                        struct HistEnt *htemp2 = histent_calloc();
                        htemp2->time = -1;
                        if(len>1)
                                {
                                htemp2->v.h_vector = htempx->v.h_vector;
                                htemp2->flags = htempx->flags;
                                }
                                else
                                {
                                htemp2->v.h_val = htempx->v.h_val;
				}
                        htemp2->next = htemp;
                        htemp = htemp2;
                        GLOBALS->fst_table_fst_c_1[txidx].numtrans++;
                        }

		np->head.time  = -2;
		np->head.next = htemp;
		np->numhist=GLOBALS->fst_table_fst_c_1[txidx].numtrans +2 /*endcap*/ +1 /*frontcap*/;

		memset(GLOBALS->fst_table_fst_c_1+txidx, 0, sizeof(struct lx2_entry));	/* zero it out */

		np->curr = histent_tail;
		np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */
		fstReaderClrFacProcessMask(GLOBALS->fst_fst_c_1, txidxi+1);
		}
	}
}

