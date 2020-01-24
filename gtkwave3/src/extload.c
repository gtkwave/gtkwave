/*
 * Copyright (c) Tony Bybell 2009-2017.
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

#include "fsdb_wrapper_api.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "vcd.h"
#include "lxt2_read.h"
#include "vzt_read.h"
#include "lxt.h"
#include "extload.h"
#include "debug.h"
#include "busy.h"
#include "hierpack.h"

#ifdef WAVE_FSDB_READER_IS_PRESENT
/* experimental new code that uses FST's on the fly fast tree build algorithm */
#define WAVE_USE_FSDB_FST_BRIDGE
#endif


#ifndef EXTLOAD_SUFFIX

const char *extload_loader_fail_msg = "Sorry, EXTLOAD support was not compiled into this executable, exiting.\n\n";

TimeType extload_main(char *fname, char *skip_start, char *skip_end)
{
(void)fname;
(void)skip_start;
(void)skip_end;

fprintf(stderr, "%s", extload_loader_fail_msg);
exit(255);

return(0); /* for vc++ */
}

void import_extload_trace(nptr np)
{
(void)np;

fprintf(stderr, "%s", extload_loader_fail_msg);
exit(255);
}

void fsdb_import_masked(void)
{
fprintf(stderr, "%s", extload_loader_fail_msg);
exit(255);
}

void fsdb_set_fac_process_mask(nptr np)
{
(void)np;

fprintf(stderr, "%s", extload_loader_fail_msg);
exit(255);
}

#else

/******************************************************************/

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

#ifndef WAVE_FSDB_READER_IS_PRESENT
static int last_modification_check(void)
{
#ifdef HAVE_SYS_STAT_H
struct stat buf;
int rc;

errno = 0;
rc = stat(GLOBALS->loaded_file_name, &buf);

if(GLOBALS->extload_lastmod)
	{
	if(GLOBALS->extload_already_errored)
		{
		return(0);
		}
	else
	if(rc != 0)
		{
		fprintf(stderr, EXTLOAD"stat error on '%s'\n", GLOBALS->loaded_file_name);
		perror("Why");
		errno = 0;
		GLOBALS->extload_already_errored = 1;
		return(0);
		}
	else
	if(GLOBALS->extload_lastmod != buf.st_mtime)
		{
		fprintf(stderr, EXTLOAD"file '%s' was modified!\n", GLOBALS->loaded_file_name);
		GLOBALS->extload_already_errored = 1;
		return(0);
		}
		else
		{
		return(1);
		}
	}
	else
	{
	GLOBALS->extload_lastmod = buf.st_mtime;
	return(1);
	}

#else
return(1);
#endif
}
#endif

#ifndef WAVE_USE_FSDB_FST_BRIDGE
#ifdef WAVE_FSDB_READER_IS_PRESENT
static char *get_varname(char *sbuff, unsigned char *vtp, unsigned char *vdp, int i, int *patched_len)
{
#else
static char *get_varname(unsigned char *vtp, unsigned char *vdp, int i, int *patched_len)
{
static char sbuff[65537];
#endif
char * rc;
int vt, vt_len;

*patched_len = 0; /* zero says is ok, otherwise size overrides msi/lsi */

#ifndef WAVE_FSDB_READER_IS_PRESENT
for(;;)
#endif
	{
#ifndef WAVE_FSDB_READER_IS_PRESENT
	rc = fgets(sbuff, 65536, GLOBALS->extload);
	if(rc)
		{
		if(isspace(rc[0]))
			{
			char *snp;
			char sbuff2[65537];

			sbuff2[0] = 0;

			if((snp=strstr(rc+1, "Struct Name:")))
				{
				sscanf(rc+14,"%s", sbuff2);
				if(sbuff2[0])
					{
					sprintf(rc, "Scope: vcd_struct %s NULL\n", sbuff2);
					}
				}
			else
			if((snp=strstr(rc+1, "Struct End")))
				{
				sprintf(rc, "Upscope:\n");
				}
			}
		}
	else
		{
		return(NULL);
		}
#else
	rc = sbuff;
#endif

	if((rc[0] == 'V') && (i >= 0))
		{
#ifndef WAVE_FSDB_READER_IS_PRESENT
		if(!strncmp("Var: ", rc, 5))
#endif
			{
			char *pnt = rc + 5;
			char *last_l = NULL;
			char typ[64];
			char *esc = NULL;
			char *lb = NULL;
			char *colon = NULL;
			char *rb = NULL;
			int state = 0;
			char *vtyp_nam;
			char *cpyto;
			char *pntd;
			char *typ_src = pnt;
			char *typ_dst = typ;

			/* following code replaces: sscanf(rc + 5, "%s", typ) */
			while(*typ_src && !isspace(*typ_src))
				{
				*(typ_dst++) = *(typ_src++);
				}
			*typ_dst = 0;

			while(*pnt)
				{
				if((pnt[0] == 'l') && (pnt[1] == ':'))
					{
					last_l = pnt;
					}
				else if(pnt[0] == '\\')
					{
					esc = pnt;
					}
				else if(!last_l)
					{
					if(pnt[0] == '[')
						{
						lb = pnt;
						colon = NULL;
						rb = NULL;
						state = 1;
						}
					else if(pnt[0] == ']')
						{
						rb = pnt;
						state = 0;
						if(!isspace(pnt[1]))
							{
							lb = colon = rb = NULL;
							}
						if(pnt[1] == '[') esc = pnt; /* pretend we're escaped to handle 2d */
						}
					else if(pnt[0] == ':')
						{
						if(state)
							{
							colon = pnt;
							}
						}
					}

				pnt++;
				}

			if(last_l)
				{
				unsigned int l, r;
				/* char s1[32]; */
				unsigned int d2;
				/* sscanf(last_l+2, "%u r:%u %s %u", &l, &r, s1, &d2); */
					
				char *ps = last_l+2;
				char *l_pnt, *r_pnt, *d2_pnt;

				while(*ps &&  isspace(*ps)) { ps++; }
				l_pnt = ps;
				while(*ps && !isspace(*ps)) { ps++; }
				while(*ps &&  isspace(*ps)) { ps++; }
				r_pnt = ps;					
				while(*ps && !isspace(*ps)) { ps++; }
				while(*ps &&  isspace(*ps)) { ps++; }
				/* s1_pnt = ps;	*/
				while(*ps && !isspace(*ps)) { ps++; }
				while(*ps &&  isspace(*ps)) { ps++; }
				d2_pnt = ps;

				l = atoi(l_pnt);
				r = atoi(r_pnt+2);
				d2 = atoi(d2_pnt);

				GLOBALS->extload_idcodes[i] = d2;
				if(GLOBALS->extload_inv_idcodes[d2] == 0) GLOBALS->extload_inv_idcodes[d2] = i+1; /* root alias */

				if(!strcmp("vcd_real", typ))
					{
					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_DOUBLE;
					GLOBALS->extload_node_block[i].msi=0;
					GLOBALS->extload_node_block[i].lsi=0;
					GLOBALS->mvlfacs_vzt_c_3[i].len=64;
					}
				else
				if(!strcmp("vcd_integer", typ))
					{
					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_INTEGER;
					GLOBALS->extload_node_block[i].msi=0;
					GLOBALS->extload_node_block[i].lsi=0;
					GLOBALS->mvlfacs_vzt_c_3[i].len=32;
					}
				else
					{
					int len_parse = 1;

					GLOBALS->mvlfacs_vzt_c_3[i].len=(l>r) ? (l-r+1) : (r-l+1);

					if(esc && lb && rb)
						{
						GLOBALS->extload_node_block[i].msi = atoi(lb+1);
						if(colon)
							{
							GLOBALS->extload_node_block[i].lsi = atoi(colon+1);
							}
							else
							{
							GLOBALS->extload_node_block[i].lsi = GLOBALS->extload_node_block[i].msi;
							}

						len_parse = (GLOBALS->extload_node_block[i].msi > GLOBALS->extload_node_block[i].lsi)
								? (GLOBALS->extload_node_block[i].msi - GLOBALS->extload_node_block[i].lsi + 1)
								: (GLOBALS->extload_node_block[i].lsi - GLOBALS->extload_node_block[i].msi + 1);

						if((GLOBALS->mvlfacs_vzt_c_3[i].len > len_parse) && !(GLOBALS->mvlfacs_vzt_c_3[i].len % len_parse)) /* check if 2d array */
							{
							/* printf("len_parse: %d vs len: %d\n", len_parse, GLOBALS->mvlfacs_vzt_c_3[i].len); */
							*patched_len = GLOBALS->mvlfacs_vzt_c_3[i].len;
							}
							else /* original, non-2d behavior */
							{
							if(len_parse != GLOBALS->mvlfacs_vzt_c_3[i].len)
								{
								GLOBALS->extload_node_block[i].msi=l;
								GLOBALS->extload_node_block[i].lsi=r;
								}
							}
						}
						else
						{
						if(lb && !l && !r) /* fix for stranded signals */
							{
							GLOBALS->extload_node_block[i].msi=atoi(lb+1);
							GLOBALS->extload_node_block[i].lsi=atoi(lb+1);
							}
							else
							{
							GLOBALS->extload_node_block[i].msi=l;
							GLOBALS->extload_node_block[i].lsi=r;
							}
						}

					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_BITS;
					}
				}

			/* now extract directional/type information */
			pnt = rc + 5;
			vtyp_nam = pnt;
			cpyto = sbuff;
			pntd = strrchr(last_l ? last_l : pnt, ':');

			if(pntd)
				{
				unsigned char vd = ND_DIR_IMPLICIT;

				pntd = strchr(pntd, ' ');
				if(pntd)
					{
					pntd++;
					if(*pntd == 'o')
						{
						vd = ND_DIR_OUT;
						GLOBALS->nonimplicit_direction_encountered = 1;
						}
					else
					if(!strncmp(pntd, "in", 2))
						{
						vd = (pntd[2] == 'p') ? ND_DIR_IN : ND_DIR_INOUT;
						GLOBALS->nonimplicit_direction_encountered = 1;
						}
					}

				if(vdp) { *vdp = vd; }
				}

			while(*pnt)
				{
				if(!isspace(*pnt))
					{
					pnt++;
					}
					else
					{
					break;
					}
				}

			/* is space */
			/* vvv extract vartype vvv */
			if(vtp)
				{
				*pnt = 0;
				vt_len = pnt-vtyp_nam;
				if(vt_len > 4)
					{
					if(!strncmp(vtyp_nam, "vcd_", 4))
						{
						vt = vcd_keyword_code(vtyp_nam + 4, vt_len - 4);
						if(vt == V_STRINGTYPE) vt = V_WIRE;
						}
						else
						{
						if(!strcmp(vtyp_nam, "stream"))
							{
							GLOBALS->extload_idcodes[i] = 0; /* kill being able to read stream variables [transactions] for now */
							}

						vt = V_WIRE;
						}
					}
					else
					{
					vt = V_WIRE;
					}

				*vtp = vt;
				*pnt = ' ';
				}
			/* ^^^ extract vartype ^^^ */

			while(*pnt)
				{
				if(isspace(*pnt))
					{
					pnt++;
					}
					else
					{
					break;
					}
				}
			if(*pnt)
				{
				while(*pnt)
					{
					/* if((*pnt == '[')||(isspace(*pnt))) break; */

 					if(isspace(*pnt)) break;
					if((*pnt == '[') && (pnt == strrchr(pnt, '[')) && (GLOBALS->mvlfacs_vzt_c_3[i].flags != VZT_RD_SYM_F_DOUBLE)) /* fix for arrays */
						{
						/* now to fix possible generate... */
						char *pnt2 = pnt;
						char lastch = *pnt2;
						int colon_seen = 0;

						pnt2++;
						while(*pnt2 && !isspace(*pnt2) && (*pnt2 != '['))
							{
							lastch = *pnt2; pnt2++;
							if(lastch == ':') { colon_seen = 1; }
							};

						if(lastch == ']') /* fix for NC verilog arrays */
							{
							int rng;

							if(colon_seen) break;

							rng = GLOBALS->extload_node_block[i].msi - GLOBALS->extload_node_block[i].lsi;
							if(!rng)
								{
								break;
								}
							}
						}

					if(*pnt == '\\') /* this is not strictly correct, but fixes generic ranges from icarus */
						{
						pnt++;
						continue;
						}
					*(cpyto++) = *(pnt++);
					}
				*cpyto = 0;
				return(sbuff);
				}
			}
		}
	else
        if(rc[0] == 'S')
                {
#ifndef WAVE_FSDB_READER_IS_PRESENT
		if(!strncmp(rc, "Scope:", 6))
#endif
			{
			char vht[2048];
			char cname[2048];
			char ctype[2048];
			unsigned char ttype;

			vht[0] = vht[4] = vht[5] = cname[0] = ctype[0] = 0;
			sscanf(rc+6, "%s %s %s", vht, cname, ctype);
                        GLOBALS->fst_scope_name = fstReaderPushScope(GLOBALS->extload_xc, cname, GLOBALS->mod_tree_parent);
			if(!strcmp(ctype, "NULL")) { ctype[0] = 0; }

			if(!strncmp(vht, "vcd_", 4))
				{
		                switch(vht[4])
					{
		                        case 'm':       ttype = TREE_VCD_ST_MODULE; break;
		                        case 't':       ttype = TREE_VCD_ST_TASK; break;
		                        case 'f':	ttype = (vht[5] == 'u') ? TREE_VCD_ST_FUNCTION : TREE_VCD_ST_FORK; break;
		                        case 'b':       ttype = TREE_VCD_ST_BEGIN; break;
		                        case 'g':       ttype = TREE_VCD_ST_GENERATE; break;
					case 's':	ttype = TREE_VCD_ST_STRUCT; break;
		                        default:        ttype = TREE_UNKNOWN; break;
		                        }
				}
			else
			if(!strncmp(vht, "sv_", 3))
				{
		                switch(vht[3])
					{
		                        case 'i':       ttype = TREE_VCD_ST_INTERFACE; break;
		                        default:        ttype = TREE_UNKNOWN; break;
		                        }
				}
			else
			if(!strncmp(vht, "vhdl_", 5))
				{
				switch(vht[5])
					{
					case 'a':	ttype = TREE_VHDL_ST_ARCHITECTURE; break;
					case 'r':	ttype = TREE_VHDL_ST_RECORD; break;
					case 'b':	ttype = TREE_VHDL_ST_BLOCK; break;
					case 'g':	ttype = TREE_VHDL_ST_GENERATE; break;
					case 'i':	ttype = TREE_VHDL_ST_GENIF; break;
					case 'f':	ttype = (vht[6] == 'u') ? TREE_VHDL_ST_FUNCTION : TREE_VHDL_ST_GENFOR; break;
					case 'p':	ttype = (!strncmp(vht+6, "roces", 5)) ? TREE_VHDL_ST_PROCESS: TREE_VHDL_ST_PROCEDURE; break;
					default:	ttype = TREE_UNKNOWN; break;
					}
				}
				else
				{
				ttype = TREE_UNKNOWN;
				}

	                allocate_and_decorate_module_tree_node(ttype, cname, ctype,  strlen(cname), strlen(ctype), 0, 0);
			}
		}
	else
        if(rc[0] == 'U')
                {
                GLOBALS->mod_tree_parent = fstReaderGetCurrentScopeUserInfo(GLOBALS->extload_xc);
                GLOBALS->fst_scope_name = fstReaderPopScope(GLOBALS->extload_xc);
		}
	}

return(NULL);
}
#endif

#ifndef WAVE_USE_FSDB_FST_BRIDGE
#ifdef WAVE_FSDB_READER_IS_PRESENT
static void process_extload_variable(char *s_gv)
#else
static void process_extload_variable(void)
#endif
{
int i;
unsigned char vt, nvt;
unsigned char vd;
struct Node *n;
struct symbol *s;
char buf[65537];
char *str;
struct fac *f;
char *fnam;
int flen;
int longest_nam_candidate = 0;
int patched_len = 0;

i = GLOBALS->extload_i;

if(i<0)
	{
#ifdef WAVE_FSDB_READER_IS_PRESENT
	fnam = get_varname(s_gv, &GLOBALS->extload_vt_prev, &GLOBALS->extload_vd_prev, 0, &patched_len);
	flen = strlen(fnam);

	if(GLOBALS->extload_hlen)
		{
		GLOBALS->extload_namecache[0 & F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[0 & F_NAME_MODULUS]=GLOBALS->extload_hlen+1+flen+1);
		memcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS], GLOBALS->fst_scope_name, GLOBALS->extload_hlen);
		*(GLOBALS->extload_namecache[0 & F_NAME_MODULUS]+GLOBALS->extload_hlen) = '.';
		strcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS]+GLOBALS->extload_hlen+1, fnam);
		GLOBALS->extload_namecache_lens[0 & F_NAME_MODULUS]=GLOBALS->extload_hlen + 1 + flen;
		GLOBALS->extload_namecache_patched[0 & F_NAME_MODULUS]=patched_len;
		}
	else
		{
		GLOBALS->extload_namecache[0 & F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[0 & F_NAME_MODULUS]=flen+1);
		strcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS], fnam);
		GLOBALS->extload_namecache_lens[0 & F_NAME_MODULUS] = flen;
		GLOBALS->extload_namecache_patched[0 & F_NAME_MODULUS]=patched_len;
		}
#else
	fnam = get_varname(&GLOBALS->extload_vt_prev, &GLOBALS->extload_vd_prev, 0, &patched_len);
	flen = strlen(fnam);

	GLOBALS->extload_namecache[0 & F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[0 & F_NAME_MODULUS]=flen+1);
	strcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS], fnam);
	GLOBALS->extload_namecache_lens[0 & F_NAME_MODULUS] = flen;
	GLOBALS->extload_namecache_patched[0 & F_NAME_MODULUS]=patched_len;
#endif
	}
else
	{
	vt = GLOBALS->extload_vt_prev;
	vd = GLOBALS->extload_vd_prev;
	if(i!=(GLOBALS->numfacs-1))
		{
#ifdef WAVE_FSDB_READER_IS_PRESENT
		fnam = get_varname(s_gv, &GLOBALS->extload_vt_prev, &GLOBALS->extload_vd_prev, i+1, &patched_len);
		flen = strlen(fnam);

		if(GLOBALS->extload_hlen)
			{
			if(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] < (GLOBALS->extload_hlen+1+flen+1))
				{
				if(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]) free_2(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]);
				GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] = GLOBALS->extload_hlen+1+flen+1);
				}

			memcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS], GLOBALS->fst_scope_name, GLOBALS->extload_hlen);
			*(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]+GLOBALS->extload_hlen) = '.';
			strcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]+GLOBALS->extload_hlen+1, fnam);
			GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS] = GLOBALS->extload_hlen + 1 + flen;
			GLOBALS->extload_namecache_patched[(i+1)&F_NAME_MODULUS] = patched_len;
			}
		else
			{
			if(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] < (flen+1))
				{
				if(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS])free_2(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]);
				GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] = flen+1);
				}
			strcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS], fnam);
			GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS] = flen;
			GLOBALS->extload_namecache_patched[(i+1)&F_NAME_MODULUS] = patched_len;
			}
#else
		fnam = get_varname(&GLOBALS->extload_vt_prev, &GLOBALS->extload_vd_prev, i+1, &patched_len);
		flen = strlen(fnam);

		if(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] < (flen+1))
			{
			GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] = flen+1);
			}
		strcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS], fnam);
			GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS] = flen;
		GLOBALS->extload_namecache_patched[(i+1)&F_NAME_MODULUS] = patched_len;
#endif
		}

	f=GLOBALS->mvlfacs_vzt_c_3+i;

	if((f->len>1)&& (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) )
		{
		int len=sprintf_2_sdd(buf, GLOBALS->extload_namecache[i&F_NAME_MODULUS],GLOBALS->extload_node_block[i].msi, GLOBALS->extload_node_block[i].lsi);
		if(GLOBALS->extload_namecache_patched[i&F_NAME_MODULUS]) /* 2d */
			{
                        GLOBALS->extload_node_block[i].msi=GLOBALS->extload_namecache_patched[i&F_NAME_MODULUS] - 1;
                        GLOBALS->extload_node_block[i].lsi=0;
			}
		longest_nam_candidate = len;

                if(!GLOBALS->do_hier_compress)
                        {
                        str=malloc_2(len+1);
                        }   
                        else
                        {
                        if(len > GLOBALS->f_name_build_buf_len)
                                {
                                free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = malloc_2((GLOBALS->f_name_build_buf_len=len)+1);
                                }
                        str = GLOBALS->f_name_build_buf;
                        }

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
		s=&GLOBALS->extload_sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		GLOBALS->extload_prevsymroot = GLOBALS->extload_prevsym = NULL;
		}
	else if (
			((f->len==1)&&(!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))&&
			((i!=GLOBALS->numfacs-1)&&(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS]==GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS])&&(!memrevcmp(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS],GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]))))
			||
			(((i!=0)&&(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS]==GLOBALS->extload_namecache_lens[(i-1)&F_NAME_MODULUS])&&(!memrevcmp(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS], GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->extload_namecache[(i-1)&F_NAME_MODULUS]))) &&
			(GLOBALS->extload_node_block[i].msi!=-1)&&(GLOBALS->extload_node_block[i].lsi!=-1))
		)
		{
		int len = sprintf_2_sd(buf, GLOBALS->extload_namecache[i&F_NAME_MODULUS],GLOBALS->extload_node_block[i].msi);
		longest_nam_candidate = len;

                if(!GLOBALS->do_hier_compress)
                        {
                        str=malloc_2(len+1);
                        }
                        else
                        {
                        if(len > GLOBALS->f_name_build_buf_len)
                                {
                                free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = malloc_2((GLOBALS->f_name_build_buf_len=len)+1);
                                }
                        str = GLOBALS->f_name_build_buf;
                        }

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
		s=&GLOBALS->extload_sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		if((GLOBALS->extload_prevsym)&&(i>0)&&(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS]==GLOBALS->extload_namecache_lens[(i-1)&F_NAME_MODULUS])&&(!memrevcmp(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS], GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->extload_namecache[(i-1)&F_NAME_MODULUS])))	/* allow chaining for search functions.. */
			{
			GLOBALS->extload_prevsym->vec_root = GLOBALS->extload_prevsymroot;
			GLOBALS->extload_prevsym->vec_chain = s;
			s->vec_root = GLOBALS->extload_prevsymroot;
			GLOBALS->extload_prevsym = s;
			}
			else
			{
			GLOBALS->extload_prevsymroot = GLOBALS->extload_prevsym = s;
			}
		}
		else
		{
		int len = GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS];

		longest_nam_candidate = len;
                if(!GLOBALS->do_hier_compress)
                        {
                        str=malloc_2(len+1);
                        }
                        else
                        {   
                        if(len > GLOBALS->f_name_build_buf_len)
                                {
                                free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = malloc_2((GLOBALS->f_name_build_buf_len=len)+1);
                                }
                        str = GLOBALS->f_name_build_buf;
                        }

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, GLOBALS->extload_namecache[i&F_NAME_MODULUS]);
			}
			else
			{
			strcpy_vcdalt(str, GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->alt_hier_delimeter);
			}
		s=&GLOBALS->extload_sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		GLOBALS->extload_prevsymroot = GLOBALS->extload_prevsym = NULL;

		if(f->flags&VZT_RD_SYM_F_INTEGER)
			{
			GLOBALS->extload_node_block[i].msi=31;
			GLOBALS->extload_node_block[i].lsi=0;
			GLOBALS->mvlfacs_vzt_c_3[i].len=32;
			}
		}

        n=&GLOBALS->extload_node_block[i];

	if(longest_nam_candidate > GLOBALS->longestname) GLOBALS->longestname = longest_nam_candidate;

        if(GLOBALS->do_hier_compress)
                {
                n->nname = compress_facility((unsigned char *)s->name, longest_nam_candidate);
                /* free_2(s->name); ...removed as GLOBALS->f_name_build_buf is now used */
                s->name = n->nname;
                }
                else
                {
                n->nname=s->name;
                }

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

	switch(vt)
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
	        case V_STRINGTYPE:      nvt = ND_GEN_STRING; break;
	        default:                nvt = ND_UNSPECIFIED_DEFAULT; break;
	        }
	n->vartype = nvt;
	n->vardir = vd;
        }

GLOBALS->extload_i++;
}
#endif

#ifndef WAVE_USE_FSDB_FST_BRIDGE
#ifdef WAVE_FSDB_READER_IS_PRESENT
static void extload_hiertree_callback(void *pnt)
{
int patched_len = 0;

char *s = (char *)pnt;

if((GLOBALS->extload_curr_tree < GLOBALS->extload_max_tree) || (!GLOBALS->extload_max_tree))
	{
	switch(s[0])
		{
		case 'S':
		case 'U': 	get_varname(s, NULL, NULL, -1, &patched_len);
				GLOBALS->extload_hlen = GLOBALS->fst_scope_name ? strlen(GLOBALS->fst_scope_name) : 0;
				break;
	
		case 'V':	process_extload_variable(s);
				break;
	
		case 'E':
	                {
			GLOBALS->extload_curr_tree++;
			fprintf(stderr, EXTLOAD"End tree #%d: %d vs %d symbols\n", GLOBALS->extload_curr_tree, GLOBALS->extload_i + 1, GLOBALS->numfacs);
			if((GLOBALS->extload_curr_tree == GLOBALS->extload_max_tree) && (GLOBALS->extload_max_tree))
				{
				if(GLOBALS->numfacs > (GLOBALS->extload_i + 1))
					{
					fprintf(stderr, EXTLOAD"Max tree count of %d processed, freeing extra memory.\n", GLOBALS->extload_max_tree);
					GLOBALS->numfacs = GLOBALS->extload_i + 1;

					/* make sure these match the corresponding calloc_2 in extload_main_2! */
					GLOBALS->mvlfacs_vzt_c_3=(struct fac *)realloc_2(GLOBALS->mvlfacs_vzt_c_3, GLOBALS->numfacs * sizeof(struct fac));
					GLOBALS->vzt_table_vzt_c_1=(struct lx2_entry *)realloc_2(GLOBALS->vzt_table_vzt_c_1, GLOBALS->numfacs * sizeof(struct lx2_entry));
					GLOBALS->extload_sym_block = (struct symbol *)realloc_2(GLOBALS->extload_sym_block, GLOBALS->numfacs * sizeof(struct symbol));
					GLOBALS->extload_node_block=(struct Node *)realloc_2(GLOBALS->extload_node_block, GLOBALS->numfacs * sizeof(struct Node));
					GLOBALS->extload_idcodes=(unsigned int *)realloc_2(GLOBALS->extload_idcodes, GLOBALS->numfacs * sizeof(unsigned int));
					}
				}
			}
	
		default:	break;
		}
	}
}
#endif
#endif

/* ///////////////////////////////////////////////////////////////////////////////////////////////////////////////// */

#ifdef WAVE_USE_FSDB_FST_BRIDGE
static char *get_varname2(struct fstHier *fh, unsigned char *vtp, unsigned char *vdp, int i, int *patched_len)
{
char *sbuff = NULL;
static char zbuf[65537]; /* OK as this does not need to be re-entrant */

*patched_len = 0; /* zero says is ok, otherwise size overrides msi/lsi */

	{
	if((fh->htyp == FST_HT_VAR) && (i >= 0))
		{
			{
			const char *pnt;
			/* const char *esc = NULL; */
			const char *lb = NULL;
			const char *colon = NULL;
			const char *rb = NULL;
			int state = 0;
			char *cpyto;

			pnt = (fh->u.var.name[fh->u.var.name_length-1] == ']') ? fh->u.var.name : &fh->u.var.name[fh->u.var.name_length];
			while(*pnt)
				{
				if(pnt[0] == '\\')
					{
					/* esc = pnt; */
					}
				else
					{
					if(pnt[0] == '[')
						{
						lb = pnt;
						colon = NULL;
						rb = NULL;
						state = 1;
						}
					else if(pnt[0] == ']')
						{
						rb = pnt;
						state = 0;
						/* if(pnt[1] == '[') esc = pnt; */ /* pretend we're escaped to handle 2d */
						}
					else if(pnt[0] == ':')
						{
						if(state)
							{
							colon = pnt;
							}
						}
					}
				pnt++;
				}


				{
				unsigned int l, r;
				unsigned int d2;

				if(lb)
					{
					if(rb)
						{
						l = atoi(lb);
						r = atoi(rb);
						}
						else
						{
						l = r = 0;
						}
					}
					else
					{				
					l = fh->u.var.length - 1;
					r = 0;
					}
				d2 = fh->u.var.handle;

				GLOBALS->extload_idcodes[i] = d2;
				if(GLOBALS->extload_inv_idcodes[d2] == 0) GLOBALS->extload_inv_idcodes[d2] = i+1; /* root alias */

				if(fh->u.var.typ == FST_VT_VCD_REAL)
					{
					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_DOUBLE;
					GLOBALS->extload_node_block[i].msi=0;
					GLOBALS->extload_node_block[i].lsi=0;
					GLOBALS->mvlfacs_vzt_c_3[i].len=64;
					}
				else
				if(fh->u.var.typ == FST_VT_VCD_INTEGER)
					{
					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_INTEGER;
					GLOBALS->extload_node_block[i].msi=0;
					GLOBALS->extload_node_block[i].lsi=0;
					GLOBALS->mvlfacs_vzt_c_3[i].len=32;
					}
				else
					{
					int len_parse = 1;

					GLOBALS->mvlfacs_vzt_c_3[i].len = fh->u.var.length;

					/* if(esc && lb && rb) */
					if(lb && rb)
						{
						GLOBALS->extload_node_block[i].msi = atoi(lb+1);
						if(colon)
							{
							GLOBALS->extload_node_block[i].lsi = atoi(colon+1);
							}
							else
							{
							GLOBALS->extload_node_block[i].lsi = GLOBALS->extload_node_block[i].msi;
							}

						len_parse = (GLOBALS->extload_node_block[i].msi > GLOBALS->extload_node_block[i].lsi)
								? (GLOBALS->extload_node_block[i].msi - GLOBALS->extload_node_block[i].lsi + 1)
								: (GLOBALS->extload_node_block[i].lsi - GLOBALS->extload_node_block[i].msi + 1);

						if((GLOBALS->mvlfacs_vzt_c_3[i].len > len_parse) && !(GLOBALS->mvlfacs_vzt_c_3[i].len % len_parse)) /* check if 2d array */
							{
							/* printf("len_parse: %d vs len: %d\n", len_parse, GLOBALS->mvlfacs_vzt_c_3[i].len); */
							*patched_len = GLOBALS->mvlfacs_vzt_c_3[i].len;
							}
							else /* original, non-2d behavior */
							{
							if(len_parse != GLOBALS->mvlfacs_vzt_c_3[i].len)
								{
								GLOBALS->extload_node_block[i].msi=l;
								GLOBALS->extload_node_block[i].lsi=r;
								}
							}
						}
						else
						{
						if(lb && !l && !r) /* fix for stranded signals */
							{
							GLOBALS->extload_node_block[i].msi=atoi(lb+1);
							GLOBALS->extload_node_block[i].lsi=atoi(lb+1);
							}
							else
							{
							GLOBALS->extload_node_block[i].msi=l;
							GLOBALS->extload_node_block[i].lsi=r;
							}
						}

					GLOBALS->mvlfacs_vzt_c_3[i].flags = VZT_RD_SYM_F_BITS;
					}
				}

			/* now extract directional/type information */
			if(vdp)
				{
				switch(fh->u.var.direction)
					{
					case FST_VD_INPUT:  *vdp = ND_DIR_IN;  GLOBALS->nonimplicit_direction_encountered = 1; break;
					case FST_VD_OUTPUT: *vdp = ND_DIR_OUT; GLOBALS->nonimplicit_direction_encountered = 1; break;
					case FST_VD_INOUT:  *vdp = ND_DIR_INOUT; GLOBALS->nonimplicit_direction_encountered = 1; break;
					case FST_VD_IMPLICIT:
					default:	    *vdp = ND_DIR_IMPLICIT; break;
					}
				}
			
			if(vtp)
				{
				*vtp = fh->u.var.typ;
				}

			pnt = fh->u.var.name;
			sbuff = cpyto = zbuf;
			sbuff[0] = 0;

			if(*pnt)
				{
				while(*pnt)
					{
					if(isspace(*pnt))
						{
						if(fh->u.var.typ == FST_VT_VCD_REAL)
							{
							pnt++;
							continue;
							}
							else
							{
							break;
							}
						}
					if((*pnt == '[') && (pnt == strrchr(pnt, '[')) && (fh->u.var.typ != FST_VT_VCD_REAL)) /* fix for arrays */
						{
						/* now to fix possible generate... */
						const char *pnt2 = pnt;
						char lastch = *pnt2;
						int colon_seen = 0;

						pnt2++;
						while(*pnt2 && !isspace(*pnt2) && (*pnt2 != '['))
							{
							lastch = *pnt2; pnt2++;
							if(lastch == ':') { colon_seen = 1; }
							};

						if(lastch == ']') /* fix for NC verilog arrays */
							{
							int rng;

							if(colon_seen) break;

							rng = GLOBALS->extload_node_block[i].msi - GLOBALS->extload_node_block[i].lsi;
							if(!rng)
								{
								break;
								}
							}
						}

					if(*pnt == '\\') /* this is not strictly correct, but fixes generic ranges from icarus */
						{
						pnt++;
						continue;
						}
					*(cpyto++) = *(pnt++);
					}
				*cpyto = 0;
				return(sbuff);
				}
			}
		}
	else
        if(fh->htyp == FST_HT_SCOPE)
                {
			{
			unsigned char ttype;

                        GLOBALS->fst_scope_name = fstReaderPushScope(GLOBALS->extload_xc, fh->u.scope.name, GLOBALS->mod_tree_parent);

			switch(fh->u.scope.typ)
				{
				case FST_ST_VCD_MODULE: ttype = TREE_VCD_ST_MODULE; break;
				case FST_ST_VCD_TASK: ttype = TREE_VCD_ST_TASK; break;
				case FST_ST_VCD_FUNCTION: ttype = TREE_VCD_ST_FUNCTION; break;
				case FST_ST_VCD_FORK: ttype = TREE_VCD_ST_FORK; break;
				case FST_ST_VCD_BEGIN: ttype = TREE_VCD_ST_BEGIN; break;
				case FST_ST_VCD_GENERATE: ttype = TREE_VCD_ST_GENERATE; break;
				case FST_ST_VCD_STRUCT: ttype = TREE_VCD_ST_STRUCT; break;
				case FST_ST_VCD_INTERFACE: ttype = TREE_VCD_ST_INTERFACE; break;				
				case FST_ST_VHDL_ARCHITECTURE: ttype = TREE_VHDL_ST_ARCHITECTURE; break;
				case FST_ST_VHDL_RECORD: ttype = TREE_VHDL_ST_RECORD; break;
				case FST_ST_VHDL_BLOCK: ttype = TREE_VHDL_ST_BLOCK; break;
				case FST_ST_VHDL_GENERATE: ttype = TREE_VHDL_ST_GENERATE; break;
				case FST_ST_VHDL_IF_GENERATE: ttype = TREE_VHDL_ST_GENIF; break;
				case FST_ST_VHDL_FUNCTION: ttype = TREE_VHDL_ST_FUNCTION; break;
				case FST_ST_VHDL_FOR_GENERATE: ttype = TREE_VHDL_ST_GENFOR; break;
				case FST_ST_VHDL_PROCEDURE: ttype = TREE_VHDL_ST_PROCEDURE; break;
				case FST_ST_VHDL_PROCESS: ttype = TREE_VHDL_ST_PROCESS; break;
				default: ttype = TREE_UNKNOWN; break;
				}

	                allocate_and_decorate_module_tree_node(ttype, fh->u.scope.name, fh->u.scope.component, fh->u.scope.name_length, fh->u.scope.component_length, 0, 0);
			}
		}
	else
        if(fh->htyp == FST_HT_UPSCOPE)
                {
                GLOBALS->mod_tree_parent = fstReaderGetCurrentScopeUserInfo(GLOBALS->extload_xc);
                GLOBALS->fst_scope_name = fstReaderPopScope(GLOBALS->extload_xc);
		}
	}

return(NULL);
}

static void fsdb_append_graft_chain(int len, char *nam, int which, struct tree *par)
{
struct tree *t = talloc_2(sizeof(struct tree) + len + 1);

memcpy(t->name, nam, len+1);
t->t_which = which;

t->child = par;
t->next = GLOBALS->terminals_tchain_tree_c_1;
GLOBALS->terminals_tchain_tree_c_1 = t;
}


static void process_extload_variable2(struct fstHier *s_gv)
{
int i;
unsigned char vt, nvt;
unsigned char vd;
struct Node *n;
struct symbol *s;
char buf[65537];
char *str;
struct fac *f;
char *fnam = NULL;
int flen;
int longest_nam_candidate = 0;
int patched_len = 0;
struct tree *npar;

static char fnam_prev[65537]; /* OK as this does not need to be re-entrant */

i = GLOBALS->extload_i;

if(i<0)
	{
	fnam = get_varname2(s_gv, &GLOBALS->extload_vt_prev, &GLOBALS->extload_vd_prev, 0, &patched_len);
	flen = strlen(fnam);
	npar = GLOBALS->mod_tree_parent;

	if(fnam) strcpy(fnam_prev, fnam);

	if(GLOBALS->extload_hlen)
		{
		GLOBALS->extload_namecache[0 & F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[0 & F_NAME_MODULUS]=GLOBALS->extload_hlen+1+flen+1);
		memcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS], GLOBALS->fst_scope_name, GLOBALS->extload_hlen);
		*(GLOBALS->extload_namecache[0 & F_NAME_MODULUS]+GLOBALS->extload_hlen) = '.';
		strcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS]+GLOBALS->extload_hlen+1, fnam);
		GLOBALS->extload_namecache_lens[0 & F_NAME_MODULUS]=GLOBALS->extload_hlen + 1 + flen;
		GLOBALS->extload_namecache_patched[0 & F_NAME_MODULUS]=patched_len;
		GLOBALS->extload_npar[0 & F_NAME_MODULUS]=npar;
		}
	else
		{
		GLOBALS->extload_namecache[0 & F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[0 & F_NAME_MODULUS]=flen+1);
		strcpy(GLOBALS->extload_namecache[0 & F_NAME_MODULUS], fnam);
		GLOBALS->extload_namecache_lens[0 & F_NAME_MODULUS] = flen;
		GLOBALS->extload_namecache_patched[0 & F_NAME_MODULUS]=patched_len;
		GLOBALS->extload_npar[0 & F_NAME_MODULUS]=npar;
		}
	}
else
	{
	vt = GLOBALS->extload_vt_prev;
	vd = GLOBALS->extload_vd_prev;
	if(i!=(GLOBALS->numfacs-1))
		{
		fnam = get_varname2(s_gv, &GLOBALS->extload_vt_prev, &GLOBALS->extload_vd_prev, i+1, &patched_len);
		flen = strlen(fnam);
		npar = GLOBALS->mod_tree_parent;

		if(GLOBALS->extload_hlen)
			{
			if(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] < (GLOBALS->extload_hlen+1+flen+1))
				{
				if(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]) free_2(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]);
				GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] = GLOBALS->extload_hlen+1+flen+1);
				}

			memcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS], GLOBALS->fst_scope_name, GLOBALS->extload_hlen);
			*(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]+GLOBALS->extload_hlen) = '.';
			strcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]+GLOBALS->extload_hlen+1, fnam);
			GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS] = GLOBALS->extload_hlen + 1 + flen;
			GLOBALS->extload_namecache_patched[(i+1)&F_NAME_MODULUS] = patched_len;
			GLOBALS->extload_npar[(i+1) & F_NAME_MODULUS]=npar;
			}
		else
			{
			if(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] < (flen+1))
				{
				if(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS])free_2(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]);
				GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]=malloc_2(GLOBALS->extload_namecache_max[(i+1)&F_NAME_MODULUS] = flen+1);
				}
			strcpy(GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS], fnam);
			GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS] = flen;
			GLOBALS->extload_namecache_patched[(i+1)&F_NAME_MODULUS] = patched_len;
			GLOBALS->extload_npar[(i+1) & F_NAME_MODULUS]=npar;
			}
		}

	f=GLOBALS->mvlfacs_vzt_c_3+i;

	if((f->len>1)&& (!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING))) )
		{
		int len=sprintf_2_sdd(buf, GLOBALS->extload_namecache[i&F_NAME_MODULUS],GLOBALS->extload_node_block[i].msi, GLOBALS->extload_node_block[i].lsi);
		if(GLOBALS->extload_namecache_patched[i&F_NAME_MODULUS]) /* 2d */
			{
                        GLOBALS->extload_node_block[i].msi=GLOBALS->extload_namecache_patched[i&F_NAME_MODULUS] - 1;
                        GLOBALS->extload_node_block[i].lsi=0;
			}
		longest_nam_candidate = len;

                if(!GLOBALS->do_hier_compress)
                        {
                        str=malloc_2(len+1);
                        }   
                        else
                        {
                        if(len > GLOBALS->f_name_build_buf_len)
                                {
                                free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = malloc_2((GLOBALS->f_name_build_buf_len=len)+1);
                                }
                        str = GLOBALS->f_name_build_buf;
                        }

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
		s=&GLOBALS->extload_sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		GLOBALS->extload_prevsymroot = GLOBALS->extload_prevsym = NULL;

                if(GLOBALS->fast_tree_sort)
                       	{
                        len = sprintf_2_sdd(buf, fnam_prev,GLOBALS->extload_node_block[i].msi, GLOBALS->extload_node_block[i].lsi);
                        fsdb_append_graft_chain(len, buf, i, GLOBALS->extload_npar[i & F_NAME_MODULUS]);
			if(fnam) strcpy(fnam_prev, fnam);
                        }
		}
	else if (
			((f->len==1)&&(!(f->flags&(VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))&&
			((i!=GLOBALS->numfacs-1)&&(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS]==GLOBALS->extload_namecache_lens[(i+1)&F_NAME_MODULUS])&&(!memrevcmp(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS],GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->extload_namecache[(i+1)&F_NAME_MODULUS]))))
			||
			(((i!=0)&&(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS]==GLOBALS->extload_namecache_lens[(i-1)&F_NAME_MODULUS])&&(!memrevcmp(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS], GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->extload_namecache[(i-1)&F_NAME_MODULUS]))) &&
			(GLOBALS->extload_node_block[i].msi!=-1)&&(GLOBALS->extload_node_block[i].lsi!=-1))
		)
		{
		int len = sprintf_2_sd(buf, GLOBALS->extload_namecache[i&F_NAME_MODULUS],GLOBALS->extload_node_block[i].msi);
		longest_nam_candidate = len;

                if(!GLOBALS->do_hier_compress)
                        {
                        str=malloc_2(len+1);
                        }
                        else
                        {
                        if(len > GLOBALS->f_name_build_buf_len)
                                {
                                free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = malloc_2((GLOBALS->f_name_build_buf_len=len)+1);
                                }
                        str = GLOBALS->f_name_build_buf;
                        }

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
		s=&GLOBALS->extload_sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		if((GLOBALS->extload_prevsym)&&(i>0)&&(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS]==GLOBALS->extload_namecache_lens[(i-1)&F_NAME_MODULUS])&&(!memrevcmp(GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS], GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->extload_namecache[(i-1)&F_NAME_MODULUS])))	/* allow chaining for search functions.. */
			{
			GLOBALS->extload_prevsym->vec_root = GLOBALS->extload_prevsymroot;
			GLOBALS->extload_prevsym->vec_chain = s;
			s->vec_root = GLOBALS->extload_prevsymroot;
			GLOBALS->extload_prevsym = s;
			}
			else
			{
			GLOBALS->extload_prevsymroot = GLOBALS->extload_prevsym = s;
			}

                if(GLOBALS->fast_tree_sort)
                       	{
                        len = sprintf_2_sd(buf, fnam_prev, GLOBALS->extload_node_block[i].msi);
                        fsdb_append_graft_chain(len, buf, i, GLOBALS->extload_npar[i & F_NAME_MODULUS]);
			if(fnam) strcpy(fnam_prev, fnam);
                       	}

		}
		else
		{
		int len = GLOBALS->extload_namecache_lens[i&F_NAME_MODULUS];

		longest_nam_candidate = len;
                if(!GLOBALS->do_hier_compress)
                        {
                        str=malloc_2(len+1);
                        }
                        else
                        {   
                        if(len > GLOBALS->f_name_build_buf_len)
                                {
                                free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = malloc_2((GLOBALS->f_name_build_buf_len=len)+1);
                                }
                        str = GLOBALS->f_name_build_buf;
                        }

		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, GLOBALS->extload_namecache[i&F_NAME_MODULUS]);
			}
			else
			{
			strcpy_vcdalt(str, GLOBALS->extload_namecache[i&F_NAME_MODULUS], GLOBALS->alt_hier_delimeter);
			}
		s=&GLOBALS->extload_sym_block[i];
	        symadd_name_exists_sym_exists(s,str,0);
		GLOBALS->extload_prevsymroot = GLOBALS->extload_prevsym = NULL;

		if(f->flags&VZT_RD_SYM_F_INTEGER)
			{
			GLOBALS->extload_node_block[i].msi=31;
			GLOBALS->extload_node_block[i].lsi=0;
			GLOBALS->mvlfacs_vzt_c_3[i].len=32;
			}

                if(GLOBALS->fast_tree_sort)
                       	{
                        fsdb_append_graft_chain(strlen(fnam_prev), fnam_prev, i, GLOBALS->extload_npar[i & F_NAME_MODULUS]);
			if(fnam) strcpy(fnam_prev, fnam);
                       	}
		}

        n=&GLOBALS->extload_node_block[i];

	if(longest_nam_candidate > GLOBALS->longestname) GLOBALS->longestname = longest_nam_candidate;

        if(GLOBALS->do_hier_compress)
                {
                n->nname = compress_facility((unsigned char *)s->name, longest_nam_candidate);
                /* free_2(s->name); ...removed as GLOBALS->f_name_build_buf is now used */
                s->name = n->nname;
                }
                else
                {
                n->nname=s->name;
                }

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

	switch(vt)
	        {
	        case FST_VT_VCD_EVENT:  	nvt = ND_VCD_EVENT; break;
	        case FST_VT_VCD_PARAMETER: 	nvt = ND_VCD_PARAMETER; break;
	        case FST_VT_VCD_INTEGER:        nvt = ND_VCD_INTEGER; break;
	        case FST_VT_VCD_REAL:           nvt = ND_VCD_REAL; break;
	        case FST_VT_VCD_REG:            nvt = ND_VCD_REG; break;
	        case FST_VT_VCD_SUPPLY0:        nvt = ND_VCD_SUPPLY0; break;
	        case FST_VT_VCD_SUPPLY1:        nvt = ND_VCD_SUPPLY1; break;
	        case FST_VT_VCD_TIME:           nvt = ND_VCD_TIME; break;
	        case FST_VT_VCD_TRI:            nvt = ND_VCD_TRI; break;
	        case FST_VT_VCD_TRIAND:         nvt = ND_VCD_TRIAND; break;
	        case FST_VT_VCD_TRIOR:          nvt = ND_VCD_TRIOR; break;
	        case FST_VT_VCD_TRIREG:         nvt = ND_VCD_TRIREG; break;
	        case FST_VT_VCD_TRI0:           nvt = ND_VCD_TRI0; break;
	        case FST_VT_VCD_TRI1:           nvt = ND_VCD_TRI1; break;
	        case FST_VT_VCD_WAND:           nvt = ND_VCD_WAND; break;
	        case FST_VT_VCD_WIRE:           nvt = ND_VCD_WIRE; break;
	        case FST_VT_VCD_WOR:            nvt = ND_VCD_WOR; break;
	        case FST_VT_VCD_PORT:           nvt = ND_VCD_PORT; break;
	        case FST_VT_GEN_STRING:         nvt = ND_GEN_STRING; break;
	        default:                nvt = ND_UNSPECIFIED_DEFAULT; break;
	        }
	n->vartype = nvt;
	n->vardir = vd;
        }

GLOBALS->extload_i++;
}

static void extload_hiertree_callback2(void *pnt)
{
int patched_len = 0;

struct fstHier *s = (struct fstHier *)pnt;

if((GLOBALS->extload_curr_tree < GLOBALS->extload_max_tree) || (!GLOBALS->extload_max_tree))
	{
	switch(s->htyp)
		{
		case FST_HT_SCOPE:
		case FST_HT_UPSCOPE: 	get_varname2(s, NULL, NULL, -1, &patched_len);
				GLOBALS->extload_hlen = GLOBALS->fst_scope_name ? strlen(GLOBALS->fst_scope_name) : 0;
				break;
	
		case FST_HT_VAR:	
					process_extload_variable2(s);
				break;
	
		case FST_HT_TREEEND:
	                {
			GLOBALS->extload_curr_tree++;
			fprintf(stderr, EXTLOAD"End tree #%d: %d vs %d symbols\n", GLOBALS->extload_curr_tree, GLOBALS->extload_i + 1, GLOBALS->numfacs);
			if((GLOBALS->extload_curr_tree == GLOBALS->extload_max_tree) && (GLOBALS->extload_max_tree))
				{
				if(GLOBALS->numfacs > (GLOBALS->extload_i + 1))
					{
					fprintf(stderr, EXTLOAD"Max tree count of %d processed, freeing extra memory.\n", GLOBALS->extload_max_tree);
					GLOBALS->numfacs = GLOBALS->extload_i + 1;

					/* make sure these match the corresponding calloc_2 in extload_main_2! */
					GLOBALS->mvlfacs_vzt_c_3=(struct fac *)realloc_2(GLOBALS->mvlfacs_vzt_c_3, GLOBALS->numfacs * sizeof(struct fac));
					GLOBALS->vzt_table_vzt_c_1=(struct lx2_entry *)realloc_2(GLOBALS->vzt_table_vzt_c_1, GLOBALS->numfacs * sizeof(struct lx2_entry));
					GLOBALS->extload_sym_block = (struct symbol *)realloc_2(GLOBALS->extload_sym_block, GLOBALS->numfacs * sizeof(struct symbol));
					GLOBALS->extload_node_block=(struct Node *)realloc_2(GLOBALS->extload_node_block, GLOBALS->numfacs * sizeof(struct Node));
					GLOBALS->extload_idcodes=(unsigned int *)realloc_2(GLOBALS->extload_idcodes, GLOBALS->numfacs * sizeof(unsigned int));
					}
				}
			}
	
		default:	break;
		}
	}
}

#endif
/* ///////////////////////////////////////////////////////////////////////////////////////////////////////////////// */


/*
 * mainline
 */
static TimeType extload_main_2(char *fname, char *skip_start, char *skip_end)
{
int max_idcode;
#ifndef WAVE_FSDB_READER_IS_PRESENT
char sbuff[65537];
unsigned int msk = 0;
#endif

int i;

if(!(GLOBALS->extload=fopen(fname, "rb")))
	{
	GLOBALS->extload_already_errored = 1;
	return(LLDescriptor(0)); 	/* look at GLOBALS->vzt_vzt_c_1 in caller for success status... */
	}
fclose(GLOBALS->extload);


/* SPLASH */                            splash_create();

#ifdef WAVE_FSDB_READER_IS_PRESENT
GLOBALS->extload_ffr_ctx = fsdbReaderOpenFile(GLOBALS->loaded_file_name);
GLOBALS->is_lx2 = LXT2_IS_FSDB;

if(GLOBALS->extload_ffr_ctx)
	{
	int rv;
	int mult;
	char scale;
	uint64_t tim;
	struct fsdbReaderGetStatistics_t *gs;
	int success_count = 0;
	int attempt_count = 0;

	attempt_count++;
	rv = fsdbReaderExtractScaleUnit(GLOBALS->extload_ffr_ctx, &mult, &scale);
	if(rv)
		{
		GLOBALS->time_scale = mult;
		GLOBALS->time_dimension = tolower(scale);
		success_count++;
		}

	attempt_count++;
	rv = fsdbReaderGetMinFsdbTag64(GLOBALS->extload_ffr_ctx, &tim);
	if(rv)
		{
		GLOBALS->min_time = tim;
		success_count++;
		}

	attempt_count++;
	rv = fsdbReaderGetMaxFsdbTag64(GLOBALS->extload_ffr_ctx, &tim);
	if(rv)
		{
		GLOBALS->max_time = tim;
		if(GLOBALS->max_time == LLDescriptor(0))
			{
			GLOBALS->max_time = LLDescriptor(1);
			}
		success_count++;
		}

	attempt_count++;
	gs = fsdbReaderGetStatistics(GLOBALS->extload_ffr_ctx);
	if(gs)
		{
		GLOBALS->numfacs = gs->varCount;
		free(gs);
		success_count++;
		}

	attempt_count++;
	max_idcode = fsdbReaderGetMaxVarIdcode(GLOBALS->extload_ffr_ctx);
	if(max_idcode)
		{
		success_count++;
		}
	else
		{
		max_idcode = GLOBALS->numfacs; /* for 1.x format files */
		success_count++;
		}

	if(attempt_count != success_count)
		{
		fprintf(stderr, EXTLOAD"Could not initialize '%s' properly.\n", fname);
		GLOBALS->extload_already_errored = 1;
		return(LLDescriptor(0));
		}
	}
	else
	{
	fprintf(stderr, EXTLOAD"Could not initialize '%s' properly.\n", fname);
	GLOBALS->extload_already_errored = 1;
	return(LLDescriptor(0));
	}

#else
int patched_len = 0;

last_modification_check();
sprintf(sbuff, "%s -info %s 2>&1", EXTLOAD_PATH, fname);
GLOBALS->extload = popen(sbuff, "r");
for(;;)
	{
	char * rc = fgets(sbuff, 65536, GLOBALS->extload);
	if(!rc) break;

	switch(rc[0])
		{
		case 's':
			if(!strncmp("scale unit", rc, 10))
				{
				char *pnt = strchr(rc+10, ':');

				if(pnt)
					{
					pnt++;
					GLOBALS->time_scale = atoi(pnt);
					GLOBALS->time_dimension = 'n';
					while(*pnt)
						{
						if(isalpha(*pnt))
							{
							GLOBALS->time_dimension = tolower(*pnt);
							break;
							}
						pnt++;
						}

					msk |= 1;
					}
				}
			break;

		case 'm':
			if(!strncmp("minimum xtag", rc, 12))
				{
				char *pnt = strchr(rc+12, '(');
				if(pnt)
					{
					unsigned int lo = 0, hi = 0;
					pnt++;
					sscanf(pnt, "%u %u", &hi, &lo);
					GLOBALS->min_time = (TimeType)((((UTimeType)hi)<<32) + ((UTimeType)lo));

					msk |= 2;
					}
				}
			else
			if(!strncmp("maximum xtag", rc, 12))
				{
				char *pnt = strchr(rc+12, '(');
				if(pnt)
					{
					unsigned int lo = 0, hi = 0;
					pnt++;
					sscanf(pnt, "%u %u", &hi, &lo);
					GLOBALS->max_time = (TimeType)((((UTimeType)hi)<<32) + ((UTimeType)lo));
					if(GLOBALS->max_time == LLDescriptor(0))
						{
						GLOBALS->max_time = LLDescriptor(1);
						}

					msk |= 4;
					}
				}
			else
			if(!strncmp("max var idcode", rc, 14))
				{
				char *pnt = strchr(rc+14, ':');
				if(pnt)
					{
					pnt++;
					sscanf(pnt, "%d", &max_idcode);

					msk |= 8;
					}
				}

			break;

		case 'v':
			if(!strncmp("var creation cnt", rc, 16))
				{
				char *pnt = strchr(rc+16, ':');
				if(pnt)
					{
					pnt++;
					sscanf(pnt, "%d", &GLOBALS->numfacs);

					msk |= 16;
					}
				}

		case 'f':
			if(!strncmp("file status", rc, 11))
				{
				char *pnt = strchr(rc+11, ':');
				if(pnt)
					{
					pnt++;
					if(strstr(pnt, "finished"))
						{
						msk |= 32;
						}
					}
				}
			break;

		default:
			break;
		}
	}
pclose(GLOBALS->extload);

if(msk != (1+2+4+8+16+32))
	{
	fprintf(stderr, EXTLOAD"Could not initialize '%s' properly.\n", fname);
	if((msk & (1+2+4+8+16+32)) == (1+2+4+8+16))
		{
		fprintf(stderr, EXTLOAD"File is not finished dumping.\n");
		}
	GLOBALS->extload_already_errored = 1;
	return(LLDescriptor(0));
	}

#endif

GLOBALS->min_time *= GLOBALS->time_scale;
GLOBALS->max_time *= GLOBALS->time_scale;

/* make sure these match the corresponding realloc_2 in extload_hiertree_callback! */
GLOBALS->mvlfacs_vzt_c_3=(struct fac *)calloc_2(GLOBALS->numfacs,sizeof(struct fac));
GLOBALS->vzt_table_vzt_c_1=(struct lx2_entry *)calloc_2(GLOBALS->numfacs, sizeof(struct lx2_entry));
GLOBALS->extload_namecache=(char **)calloc_2(F_NAME_MODULUS+1, sizeof(char *));
GLOBALS->extload_namecache_max=(int *)calloc_2(F_NAME_MODULUS+1, sizeof(int));
GLOBALS->extload_namecache_lens=(int *)calloc_2(F_NAME_MODULUS+1, sizeof(int));
GLOBALS->extload_namecache_patched=(int *)calloc_2(F_NAME_MODULUS+1, sizeof(int));
GLOBALS->extload_sym_block = (struct symbol *)calloc_2(GLOBALS->numfacs, sizeof(struct symbol));
GLOBALS->extload_node_block=(struct Node *)calloc_2(GLOBALS->numfacs,sizeof(struct Node));
GLOBALS->extload_idcodes=(unsigned int *)calloc_2(GLOBALS->numfacs, sizeof(unsigned int));
GLOBALS->extload_inv_idcodes=(int *)calloc_2(max_idcode+1, sizeof(int));
#ifdef WAVE_USE_FSDB_FST_BRIDGE
GLOBALS->extload_npar=(struct tree **)calloc_2(F_NAME_MODULUS+1, sizeof(struct tree *));
#endif

if(!GLOBALS->fast_tree_sort)
        {
        GLOBALS->do_hier_compress = 0;
        }
	else
	{
	hier_auto_enable(); /* enable if greater than threshold */
	}

GLOBALS->f_name_build_buf_len = 128;
GLOBALS->f_name_build_buf = malloc_2(GLOBALS->f_name_build_buf_len + 1);

init_facility_pack();


/* SPLASH */                            splash_sync(1, 5);

#ifdef WAVE_FSDB_READER_IS_PRESENT

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

GLOBALS->extload_xc = fstReaderOpenForUtilitiesOnly();

GLOBALS->extload_i=-1;
#ifdef WAVE_USE_FSDB_FST_BRIDGE
fsdbReaderReadScopeVarTree2(GLOBALS->extload_ffr_ctx, extload_hiertree_callback2);
process_extload_variable2(NULL); /* flush out final cached variable */
#else
fsdbReaderReadScopeVarTree(GLOBALS->extload_ffr_ctx, extload_hiertree_callback);
process_extload_variable(NULL); /* flush out final cached variable */
#endif

decorated_module_cleanup(); /* ...also now in gtk2_treesearch.c */
iter_through_comp_name_table();

for(i=0;i<=F_NAME_MODULUS;i++)
	{
	if(GLOBALS->extload_namecache[i])
		{
		free_2(GLOBALS->extload_namecache[i]);
		GLOBALS->extload_namecache[i] = NULL;
		}
	}
free_2(GLOBALS->extload_namecache); GLOBALS->extload_namecache = NULL;
free_2(GLOBALS->extload_namecache_max); GLOBALS->extload_namecache_max = NULL;
free_2(GLOBALS->extload_namecache_lens); GLOBALS->extload_namecache_lens = NULL;
free_2(GLOBALS->extload_namecache_patched); GLOBALS->extload_namecache_patched = NULL;
#ifdef WAVE_USE_FSDB_FST_BRIDGE
free_2(GLOBALS->extload_npar); GLOBALS->extload_npar = NULL;
#endif

fstReaderClose(GLOBALS->extload_xc); /* corresponds to fstReaderOpenForUtilitiesOnly() */

#else

if(!last_modification_check()) { GLOBALS->extload_already_errored = 1; return(LLDescriptor(0)); }
sprintf(sbuff, "%s -hier_tree %s 2>&1", EXTLOAD_PATH, fname);
GLOBALS->extload = popen(sbuff, "r");

/* do your stuff here..all useful info has been initialized by now */

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

GLOBALS->extload_xc = fstReaderOpenForUtilitiesOnly();

for(GLOBALS->extload_i=-1;(GLOBALS->numfacs) && (GLOBALS->extload_i<GLOBALS->numfacs);)
	{
	process_extload_variable();
	}
while(get_varname(&GLOBALS->extload_vt_prev, NULL, -1, &patched_len)); /* read through end to process all upscopes */

decorated_module_cleanup(); /* ...also now in gtk2_treesearch.c */
iter_through_comp_name_table();

for(i=0;i<=F_NAME_MODULUS;i++)
	{
	if(GLOBALS->extload_namecache[i])
		{
		free_2(GLOBALS->extload_namecache[i]);
		GLOBALS->extload_namecache[i] = NULL;
		}
	}
free_2(GLOBALS->extload_namecache); GLOBALS->extload_namecache = NULL;
free_2(GLOBALS->extload_namecache_max); GLOBALS->extload_namecache_max = NULL;
free_2(GLOBALS->extload_namecache_lens); GLOBALS->extload_namecache_lens = NULL;
pclose(GLOBALS->extload);

fstReaderClose(GLOBALS->extload_xc); /* corresponds to fstReaderOpenForUtilitiesOnly() */

#endif

/* SPLASH */                            splash_sync(2, 5);
if(GLOBALS->f_name_build_buf) { free_2(GLOBALS->f_name_build_buf); GLOBALS->f_name_build_buf = NULL; }
freeze_facility_pack();

GLOBALS->facs=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));

if(GLOBALS->fast_tree_sort)
        {
        for(i=0;i<GLOBALS->numfacs;i++)
                {
                GLOBALS->facs[i]=&GLOBALS->extload_sym_block[i];
                }

/* SPLASH */                            splash_sync(3, 5);
        fprintf(stderr, EXTLOAD"Building facility hierarchy tree.\n");

        init_tree();
#ifndef WAVE_USE_FSDB_FST_BRIDGE
        for(i=0;i<GLOBALS->numfacs;i++)
                {
                int was_packed = HIER_DEPACK_STATIC; /* no need to free_2() afterward then */
                char *sb = hier_decompress_flagged(GLOBALS->facs[i]->name, &was_packed);
                build_tree_from_name(sb, i);
                }
#endif
/* SPLASH */                            splash_sync(4, 5);
        treegraft(&GLOBALS->treeroot);

        fprintf(stderr, EXTLOAD"Sorting facility hierarchy tree.\n");
        treesort(GLOBALS->treeroot, NULL);
/* SPLASH */                            splash_sync(5, 5);
        order_facs_from_treesort(GLOBALS->treeroot, &GLOBALS->facs);

        GLOBALS->facs_are_sorted=1;
        }
        else
	{
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst;
#ifdef WAVE_HIERFIX
		char ch;
#endif
		int len;

		GLOBALS->facs[i]=&GLOBALS->extload_sym_block[i];
		subst=GLOBALS->facs[i]->name;
	        if((len=strlen(subst))>GLOBALS->longestname) GLOBALS->longestname=len;
#ifdef WAVE_HIERFIX
		while((ch=(*subst)))
			{
			if(ch==GLOBALS->hier_delimeter) { *subst=VCDNAM_HIERSORT; }	/* forces sort at hier boundaries */
			subst++;
			}
#endif
		}

/* SPLASH */                            splash_sync(3, 5);
	fprintf(stderr, EXTLOAD"Sorting facilities at hierarchy boundaries.\n");
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
	fprintf(stderr, EXTLOAD"Building facility hierarchy tree.\n");

	init_tree();
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *nf = GLOBALS->facs[i]->name;
	        build_tree_from_name(nf, i);
		}
/* SPLASH */                            splash_sync(5, 5);
	treegraft(&GLOBALS->treeroot);
	treesort(GLOBALS->treeroot, NULL);
	}

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

	GLOBALS->min_time = b_start;
	GLOBALS->max_time = b_end;
	}

/* SPLASH */                            splash_finalize();
return(GLOBALS->max_time);
}


TimeType extload_main(char *fname, char *skip_start, char *skip_end)
{
TimeType tt = extload_main_2(fname, skip_start, skip_end);

if(!tt)
	{
        if(GLOBALS->extload_ffr_ctx)
                {
#ifdef WAVE_FSDB_READER_IS_PRESENT
                fsdbReaderClose(GLOBALS->extload_ffr_ctx);
#endif
                GLOBALS->extload_ffr_ctx = NULL;
                }
	}

return(tt);
}


/*
 * extload callback (only does bits for now)
 */
static void extload_callback(TimeType *tim, int *facidx, char **value)
{
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
static void ext_resolver(nptr np, nptr resolve)
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
 * actually import a extload trace but don't do it if it's already been imported
 */
void import_extload_trace(nptr np)
{
struct HistEnt *htemp, *htempx=NULL, *histent_tail;
int len, i;
struct fac *f;
int txidx, txidx_in_trace;
nptr nold = np;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f - GLOBALS->mvlfacs_vzt_c_3;
txidx_in_trace = GLOBALS->extload_idcodes[txidx];

if(GLOBALS->extload_inv_idcodes[txidx_in_trace] < 0)
	{
	txidx = (-GLOBALS->extload_inv_idcodes[txidx_in_trace]) - 1;

	np = GLOBALS->mvlfacs_vzt_c_3[txidx].working_node;

	if(!(f=np->mv.mvlfac))
		{
		ext_resolver(nold, np);
		return;	/* already imported */
		}
	}

GLOBALS->extload_inv_idcodes[txidx_in_trace] = - (txidx + 1);

#ifndef WAVE_FSDB_READER_IS_PRESENT
fprintf(stderr, EXTLOAD"Import: %s\n", np->nname);
#endif

/* new stuff */
len = np->mv.mvlfac->len;

#ifdef WAVE_FSDB_READER_IS_PRESENT
if(0)
{
/* process transactions here */
}
else /* "normal" VC data */
{
void *hdl;

/* fsdbReaderAddToSignalList(GLOBALS->extload_ffr_ctx, txidx_in_trace); */
/* fsdbReaderLoadSignals(GLOBALS->extload_ffr_ctx); */

hdl = fsdbReaderCreateVCTraverseHandle(GLOBALS->extload_ffr_ctx, txidx_in_trace);
if(fsdbReaderHasIncoreVC(GLOBALS->extload_ffr_ctx, hdl))
	{
	TimeType mxt_max = -2;
	TimeType mxt = (TimeType)fsdbReaderGetMinXTag(GLOBALS->extload_ffr_ctx, hdl);
	int rc_xtag = fsdbReaderGotoXTag(GLOBALS->extload_ffr_ctx, hdl, mxt);

	while(rc_xtag && (mxt >= mxt_max)) /* malformed traces sometimes backtrack time */
		{
		void *val_ptr;
		char *b;
		if(!fsdbReaderGetVC(GLOBALS->extload_ffr_ctx, hdl, &val_ptr))
			{
			break;
			}

		b = fsdbReaderTranslateVC(hdl, val_ptr);
		extload_callback(&mxt, &txidx, &b);

		if(!fsdbReaderGotoNextVC(GLOBALS->extload_ffr_ctx, hdl))
			{
			break;
			}

		mxt_max = mxt;
		mxt = (TimeType)fsdbReaderGetXTag(GLOBALS->extload_ffr_ctx, hdl, &rc_xtag);
		}
	}
fsdbReaderFree(GLOBALS->extload_ffr_ctx, hdl);
/* fsdbReaderUnloadSignals(GLOBALS->extload_ffr_ctx); */
}

#else

if(last_modification_check()) /* place array height check here in an "&&" branch, sorry, arrays not supported */
	{
	char sbuff[65537];
	TimeType tim;

	sprintf(sbuff, "%s -vc -vidcode %d %s 2>&1", EXTLOAD_PATH, txidx_in_trace, GLOBALS->loaded_file_name);
	GLOBALS->extload = popen(sbuff, "r");

	for(;;)
		{
	        char *rc = fgets(sbuff, 65536, GLOBALS->extload);
	        if(!rc)
	                {
	                break;
	                }

		if(isdigit(rc[0]))
			{
			rc = strchr(rc, '(');
			if(rc)
				{
				unsigned int lo = 0, hi = 0;
				sscanf(rc+1, "%u %u", &hi, &lo);
				tim = (TimeType)((((UTimeType)hi)<<32) + ((UTimeType)lo));

				rc = strchr(rc+1, ')');
				if(rc)
					{
					rc = strchr(rc+1, ':');
					if(rc)
						{
						char *rtn, *pnt;
						rc += 2;

						rtn = rc;
						while(*rtn)
							{
							if(isspace(*rtn)) { *rtn = 0; break; }
							rtn++;
							}

						pnt = rc;
						while(*pnt)
							{
							switch(*pnt)
								{
								case 'Z':
								case '3': *pnt = 'z'; break;

								case 'X':
								case '2': *pnt = 'x'; break;

								default: break;
								}

							pnt++;
							}

						extload_callback(&tim, &txidx, &rc);
						}
					}
				}
			}
		}

	pclose(GLOBALS->extload);
	}
#endif

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
htempx = htemp;
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
	                htemp2->v.h_vector = htempx->v.h_vector;
	                }
	                else
	                {
	                htemp2->v.h_val = htempx->v.h_val;
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

if(nold!=np)
	{
	ext_resolver(nold, np);
	}
}


void fsdb_import_masked(void)
{
#ifdef WAVE_FSDB_READER_IS_PRESENT
fsdbReaderLoadSignals(GLOBALS->extload_ffr_ctx);
GLOBALS->extload_ffr_import_count = 0;
#endif
}

void fsdb_set_fac_process_mask(nptr np)
{
#ifdef WAVE_FSDB_READER_IS_PRESENT
struct fac *f;
int txidx, txidx_in_trace;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

txidx = f - GLOBALS->mvlfacs_vzt_c_3;
txidx_in_trace = GLOBALS->extload_idcodes[txidx];

if(GLOBALS->extload_inv_idcodes[txidx_in_trace] > 0)
	{
	if(!GLOBALS->extload_ffr_import_count)
		{
		fsdbReaderUnloadSignals(GLOBALS->extload_ffr_ctx);
		fsdbReaderResetSignalList(GLOBALS->extload_ffr_ctx);
		}
	GLOBALS->extload_ffr_import_count++;

	fsdbReaderAddToSignalList(GLOBALS->extload_ffr_ctx, txidx_in_trace);
	}
#else
(void)np;
#endif
}

#endif

