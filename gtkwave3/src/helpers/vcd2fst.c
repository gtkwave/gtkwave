/*
 * Copyright (c) 2009-2014 Tony Bybell.
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

#include <config.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "fst/fstapi.h"
#include "../../contrib/rtlbrowse/jrb.h"
#include "wave_locale.h"

#ifdef EXTLOAD_SUFFIX
#ifdef EXTCONV_PATH
#define VCD2FST_EXTLOAD_CONV
#endif
#endif

#ifdef EXT2LOAD_SUFFIX
#ifdef EXT2CONV_PATH
#define VCD2FST_EXT2LOAD_CONV
#endif
#endif

#ifdef EXT3LOAD_SUFFIX
#ifdef EXT3CONV_PATH
#define VCD2FST_EXT3LOAD_CONV
#endif
#endif

#if defined(VCD2FST_EXTLOAD_CONV) || defined(VCD2FST_EXT2LOAD_CONV) || defined(VCD2FST_EXT3LOAD_CONV)
#define VCD2FST_EXTLOADERS_CONV
#endif

static uint32_t var_direction_idx = 0;
static unsigned char *var_direction = NULL;


static void *realloc_2(void *ptr, size_t siz) /* cppcheck */
{
void *pnt = realloc(ptr, siz);
if(!pnt)
	{
	fprintf(stderr, "ERROR: Out of memory in realloc(), exiting!\n"); /* normally free(ptr) here */
	exit(255);
	}

return(pnt);
}


/*********************************************************/
/*** vvv extload component type name determination vvv ***/
/*********************************************************/

#if defined(VCD2FST_EXTLOAD_CONV)

#ifdef _WAVE_HAVE_JUDY
#include <Judy.h>
Pvoid_t  PJArray = NULL;
#else
JRB comp_name_jrb = NULL;
#endif

static const char *fst_scope_name = NULL;
static uint32_t numfacs = 0;

static char *get_info(FILE *extload)
{
static char sbuff[65537];
char * rc;

for(;;)
        {
        rc = fgets(sbuff, 65536, extload);
	if(!rc)
                {
                return(NULL);
                }

	switch(rc[0])
		{
                case 'v':
                        if(!strncmp("var creation cnt", rc, 16))
                                {
                                char *pnt = strchr(rc+16, ':');
                                if(pnt)
                                        {
                                        pnt++;
                                        sscanf(pnt, "%u", &numfacs);
                                        }
                                }
			break;

		default:
			break;
		}
	}
}

static char *get_scopename(void *xc, FILE *extload)
{
static char sbuff[65537];
char * rc;
#ifdef _WAVE_HAVE_JUDY
PPvoid_t PPValue;
#else
JRB str;
Jval jv;
#endif

for(;;)
        {
        rc = fgets(sbuff, 65536, extload);
        if(rc)
		{
                if(isspace(rc[0]))
                        {
                        char sbuff2[65537];

                        sbuff2[0] = 0;

                        if(strstr(rc+1, "Struct Name:"))
                                {
                                sscanf(rc+14,"%s", sbuff2);
                                if(sbuff2[0])
                                        {
                                        sprintf(rc, "Scope: vcd_struct %s NULL\n", sbuff2);
                                        }
                                }
                        else
                        if(strstr(rc+1, "Struct End"))
                                {
                                sprintf(rc, "Upscope:\n");
                                }
			}
		}
	else
                {
                return(NULL);
                }

	if(rc[0] == 'V')
		{
                if(!strncmp("Var: ", rc, 5))
                        {
                        char *pnt = rc + 5;
                        char *pntd = strrchr(pnt, ':');

                        if(pntd)
                                {
                                unsigned char vd = FST_VD_IMPLICIT;

                                pntd = strchr(pntd, ' ');
                                if(pntd)
                                        {
                                        pntd++;
                                        if(*pntd == 'o')
                                                {
                                                vd = FST_VD_OUTPUT;
                                                }
                                        else
                                        if(!strncmp(pntd, "in", 2))
                                                {
                                                vd = (pntd[2] == 'p') ? FST_VD_INPUT : FST_VD_INOUT;
                                                }
                                        }

				var_direction[var_direction_idx++] = vd;
                                }
			}
		}
	else
        if(rc[0] == 'S')
                {
                if(!strncmp(rc, "Scope:", 6))
                        {
                        char vht[2048];
                        char cname[2048];
                        char ctype[2048];
			int mtype = FST_ST_VCD_MODULE;

			cname[0] = ctype[1] = 0;

                        sscanf(rc+6, "%s %s %s", vht, cname, ctype+1);
			if(!strncmp("vcd_", vht, 4))
				{
				switch(vht[4])
					{
					case 'g':	mtype = FST_ST_VCD_GENERATE; break; /* other code looks for non-modules to replace type with */
					case 's':	mtype = FST_ST_VCD_STRUCT;   break; /* other code looks for non-modules to replace type with */
					default:	break;
					}
				}
			else
			if(!strncmp("sv_", vht, 3))
				{
				switch(vht[3])
					{
					case 'i':	mtype = FST_ST_VCD_INTERFACE; break; /* other code looks for non-modules to replace type with */
					default:	break;
					}
				}
                        else
                        if(!strncmp(vht, "vhdl_", 5))
                                {
                                switch(vht[5])
                                        {
                                        case 'a':       mtype = FST_ST_VHDL_ARCHITECTURE; break;
                                        case 'r':       mtype = FST_ST_VHDL_RECORD; break;
                                        case 'b':       mtype = FST_ST_VHDL_BLOCK; break;
                                        case 'g':       mtype = FST_ST_VHDL_GENERATE; break;
                                        case 'i':       mtype = FST_ST_VHDL_IF_GENERATE; break;
                                        case 'f':       mtype = (vht[6] == 'u') ? FST_ST_VHDL_FUNCTION : FST_ST_VHDL_FOR_GENERATE; break;
                                        case 'p':       mtype = (!strncmp(vht+6, "roces", 5)) ? FST_ST_VHDL_PROCESS: FST_ST_VHDL_PROCEDURE; break;
                                        default:        break;
                                        }
                                }

			ctype[0] = mtype + 1; /* bias for zero terminated string */

                        fst_scope_name = fstReaderPushScope(xc, cname, NULL);

			/* process fst_scope_name + cname vs ctype here */
			if((strcmp(ctype+1, "NULL") && strcmp(cname, ctype+1)) || (mtype != FST_ST_VCD_MODULE))
				{
#ifdef _WAVE_HAVE_JUDY
				PPValue = JudySLIns(&PJArray, (uint8_t *)fst_scope_name, PJE0);
				if(!*((char **)PPValue))
					{
					*((char **)PPValue) = strdup(ctype);
					}
#else
				char cstring[65537];
				strcpy(cstring, fst_scope_name);
				str = jrb_find_str(comp_name_jrb, cstring);
				if(!str)
					{
					jv.s = strdup(ctype);
					jrb_insert_str(comp_name_jrb, strdup(cstring), jv);
					}
#endif
				}
                        }
                }
        else
        if(rc[0] == 'U')
                {
                fst_scope_name = fstReaderPopScope(xc);
                }
        }

return(rc);
}


static void iter_scope(char *fname)
{
char sbuff[65537];
FILE *extload;
void *xc = fstReaderOpenForUtilitiesOnly();

sprintf(sbuff, "%s -info %s 2>&1", EXTLOAD_PATH, fname);
extload = popen(sbuff, "r");
if(extload)
	{
	while(get_info(extload));
	pclose(extload);
	}

if(numfacs)
	{
	var_direction = calloc(numfacs, sizeof(unsigned char));
	var_direction_idx = 0;
	}

sprintf(sbuff, "%s -tree %s 2>&1", EXTLOAD_PATH, fname);
extload = popen(sbuff, "r");
if(extload)
	{
	while(get_scopename(xc, extload));
	pclose(extload);
	}

var_direction_idx = 0;

fstReaderClose(xc); /* corresponds to fstReaderOpenForUtilitiesOnly() */
}


static void dealloc_scope(void)
{
#ifdef _WAVE_HAVE_JUDY
PPvoid_t PPValue;

if(PJArray)
        {
        char Index[65537];
	Index[0] = 0;

        for (PPValue  = JudySLFirst (PJArray, (uint8_t *)Index, PJE0);
                 PPValue != (PPvoid_t) NULL;
                 PPValue  = JudySLNext  (PJArray, (uint8_t *)Index, PJE0))
            {
		free(*(char **)PPValue);
            }

        JudySLFreeArray(&PJArray, PJE0);
        PJArray = NULL;
        }
#else
if(comp_name_jrb)
	{
        JRB node;
	char *Index;

        jrb_traverse(node, comp_name_jrb)
            {
                Index = node->key.s;
                free(Index);
                Index = node->val.s;
                free(Index);
            }

        jrb_free_tree(comp_name_jrb);
        comp_name_jrb = NULL;
	}
#endif
}

#endif

/*********************************************************/
/*** ^^^ extload component type name determination ^^^ ***/
/*********************************************************/

static uint64_t atoi_2(const unsigned char *s)
{
uint64_t res = 0;
unsigned char ch;

ch = *s - '0';
while(*s && (ch > 9))
        {
        s++;
	ch = *s - '0';
        }

while(ch < 10)
        {
        s++;
        res *= 10;
        res += ch;
	ch = *s - '0';
        }

return(res);
}


static inline int getline_replace(char **wbuf, char **buf, size_t *len, FILE *f)
{
char *fgets_rc;

if(!*wbuf)
	{
	*len = 32767;
	*wbuf = malloc((*len) + 1);
	(*wbuf)[*len] = 1;
	}

(*wbuf)[0] = 0;
fgets_rc = fgets(*wbuf, (*len) + 1, f);
while(((*wbuf)[*len] != 1) && !feof(f))
	{
	/* fprintf(stderr, "overflow %d\n", (int)(*len)); */
	*wbuf = realloc_2(*wbuf, (*len) * 2 + 1);
	(*wbuf)[(*len) * 2] = 1;

	fgets_rc = fgets(*wbuf + (*len), (*len) + 1, f);
	*len = 2 * (*len);
	}

*buf = *wbuf;
while(*(buf)[0]==' ') { (*buf)++; } /* verilator leading spaces fix */

if((!(*buf)[0])||(!fgets_rc))
	{
	return(0);
	}
	else
	{
	return(1);
	}
}

JRB vcd_ids = NULL;

static unsigned int vcdid_hash(char *s, int len)
{
unsigned int val=0;
int i;

s += len;

for(i=0;i<len;i++)
        {
	val *= 94;
        val += ((unsigned char)*(--s)) - 32;
        }

return(val);
}

int pack_type = FST_WR_PT_LZ4;  /* set to fstWriterPackType */
int compression_explicitly_set = 0;
int repack_all = 0; /* 0 is normal, 1 does the repack (via fstapi) at end */
int parallel_mode = 0; /* 0 is is single threaded, 1 is multi-threaded */


#ifdef VCD2FST_EXTLOADERS_CONV
static int suffix_check(const char *s, const char *sfx)
{
size_t sfxlen = strlen(sfx);
return((strlen(s)>=sfxlen)&&(!strcasecmp(s+strlen(s)-sfxlen,sfx)));
}
#endif


int fst_main(char *vname, char *fstname)
{
FILE *f;
char *buf = NULL, *wbuf = NULL;
size_t glen = 0;
void *ctx;
int line = 0;
int ss;
fstHandle returnedhandle;
JRB node;
uint64_t prev_tim = 0;
ssize_t bin_fixbuff_len = 65537;
char *bin_fixbuff = NULL;
int hash_kill = 0;
unsigned int hash_max = 0;
int *node_len_array = NULL;
int is_popen = 0;
#ifdef VCD2FST_EXTLOAD_CONV
int is_extload = 0;
void *xc = NULL;
#endif
int port_encountered = 0;

bin_fixbuff = malloc(bin_fixbuff_len);

if(!strcmp("-", vname))
	{
	f = stdin;
	}
	else
	{
#ifdef VCD2FST_EXTLOAD_CONV
	if(suffix_check(vname, "."EXTLOAD_SUFFIX) || suffix_check(vname, "."EXTLOAD_SUFFIX".gz") || suffix_check(vname, "."EXTLOAD_SUFFIX".bz2"))
		{
		sprintf(bin_fixbuff, EXTCONV_PATH" %s", vname);
		f = popen(bin_fixbuff, "r");
		is_popen = 1;
		is_extload = 1;
#ifndef _WAVE_HAVE_JUDY
		comp_name_jrb = make_jrb();
#endif
		iter_scope(vname);
		}
		else
#endif
		{
#ifdef VCD2FST_EXT2LOAD_CONV
		if(suffix_check(vname, "."EXT2LOAD_SUFFIX))
			{
			sprintf(bin_fixbuff, EXT2CONV_PATH" %s", vname);
			f = popen(bin_fixbuff, "r");
			is_popen = 1;
			}
			else
#endif
#ifdef VCD2FST_EXT3LOAD_CONV
		if(suffix_check(vname, "."EXT3LOAD_SUFFIX))
			{
			sprintf(bin_fixbuff, EXT3CONV_PATH" %s", vname);
			f = popen(bin_fixbuff, "r");
			is_popen = 1;
			}
			else
#endif
			{
			f = fopen(vname, "rb");
			}
		}
	}

if(!f)
	{
	printf("Could not open '%s', exiting.\n", vname);
	free(bin_fixbuff); bin_fixbuff = NULL;
	free(vname); free(fstname);
	exit(255);
	}

ctx = fstWriterCreate(fstname, 1);

if(!ctx)
	{
	printf("Could not open '%s', exiting.\n", fstname);
	free(bin_fixbuff); bin_fixbuff = NULL;
	free(vname); free(fstname);
	fclose(f);
	exit(255);
	}

#if defined(VCD2FST_EXTLOAD_CONV)
if(is_popen && is_extload)
	{
	xc = fstReaderOpenForUtilitiesOnly();
	}
#endif

vcd_ids = make_jrb();
fstWriterSetPackType(ctx, pack_type);
fstWriterSetRepackOnClose(ctx, repack_all);
fstWriterSetParallelMode(ctx, parallel_mode);

while(!feof(f))
	{
	char *buf1;

	ss = getline_replace(&wbuf, &buf, &glen, f);
	if(!ss)
		{
		break;
		}
	line++;

	if(buf[0] != '$') continue;
	buf1 = buf + 1;

	if(!strncmp(buf1, "var", 3))
		{
		char *st = strtok(buf+5, " \t");
		enum fstVarType vartype;
		int len;
		char *nam;
		unsigned int hash;

		if(!st)
			{
			continue; /* variable declaration not on a single line */
			}

		vartype = FST_VT_VCD_WIRE;
		switch(st[0])
			{
			case 'w':
				if(!strcmp(st, "wire"))
					{
					}
				else
				if(!strcmp(st, "wand"))
					{
					vartype = FST_VT_VCD_WAND;
					}
				else
				if(!strcmp(st, "wor"))
					{
					vartype = FST_VT_VCD_WOR;
					}
				break;

			case 'r':
				if(!strcmp(st, "reg"))
					{
					vartype = FST_VT_VCD_REG;
					}
				else
				if(!strcmp(st, "real"))
					{
					vartype = FST_VT_VCD_REAL;
					}
				else
				if(!strcmp(st, "real_parameter"))
					{
					vartype = FST_VT_VCD_REAL_PARAMETER;
					}
				else
				if(!strcmp(st, "realtime"))
					{
					vartype = FST_VT_VCD_REALTIME;
					}
				break;

			case 'p':
				if(!strcmp(st, "parameter"))
					{
					vartype = FST_VT_VCD_PARAMETER;
					}
				else
				if(!strcmp(st, "port"))
					{
					vartype = FST_VT_VCD_PORT;
					port_encountered = 1;
					}
				break;

			case 'i':
				if(!strcmp(st, "integer"))
					{
					vartype = FST_VT_VCD_INTEGER;
					}
				else
				if(!strcmp(st, "int"))
					{
					vartype = FST_VT_SV_INT;
					}
				break;

			case 'e':
				if(!strcmp(st, "event"))
					{
					vartype = FST_VT_VCD_EVENT;
					}
				else
				if(!strcmp(st, "enum"))
					{
					vartype = FST_VT_SV_ENUM;
					}
				break;

			case 'b':
				if(!strcmp(st, "bit"))
					{
					vartype = FST_VT_SV_BIT;
					}
				else
				if(!strcmp(st, "byte"))
					{
					vartype = FST_VT_SV_BYTE;
					}
				break;

			case 'l':
				if(!strcmp(st, "logic"))
					{
					vartype = FST_VT_SV_LOGIC;
					}
				else
				if(!strcmp(st, "longint"))
					{
					vartype = FST_VT_SV_LONGINT;
					}
				break;

			case 's':
				if(!strcmp(st, "supply1"))
					{
					vartype = FST_VT_VCD_SUPPLY1;
					}
				else
				if(!strcmp(st, "supply0"))
					{
					vartype = FST_VT_VCD_SUPPLY0;
					}
				else
				if(!strcmp(st, "string"))
					{
					vartype = FST_VT_GEN_STRING;
					}
				else
				if(!strcmp(st, "shortint"))
					{
					vartype = FST_VT_SV_SHORTINT;
					}
				else
				if(!strcmp(st, "shortreal"))
					{
					vartype = FST_VT_SV_SHORTREAL;
					}
				else
				if(!strcmp(st, "sparray"))
					{
					vartype = FST_VT_VCD_SPARRAY;
					}
				break;

			case 't':
				if(!strcmp(st, "time"))
					{
					vartype = FST_VT_VCD_TIME;
					}
				else
				if(!strcmp(st, "tri"))
					{
					vartype = FST_VT_VCD_TRI;
					}
				else
				if(!strcmp(st, "triand"))
					{
					vartype = FST_VT_VCD_TRIAND;
					}
				else
				if(!strcmp(st, "trior"))
					{
					vartype = FST_VT_VCD_TRIOR;
					}
				else
				if(!strcmp(st, "trireg"))
					{
					vartype = FST_VT_VCD_TRIREG;
					}
				else
				if(!strcmp(st, "tri0"))
					{
					vartype = FST_VT_VCD_TRI0;
					}
				else
				if(!strcmp(st, "tri1"))
					{
					vartype = FST_VT_VCD_TRI1;
					}
				break;

			default:
				break;
			}

		st = strtok(NULL, " \t");
		len = atoi(st);
		switch(vartype)
			{
			case FST_VT_VCD_PORT:
				if(*st == '[') /* VCS extension, so reparse */
					{
					int p_hi = atoi(st+1);
					int p_lo = p_hi;
					char *p_colon = strchr(st+1, ':');
					if(p_colon)
						{
						p_lo = atoi(p_colon+1);
						}

					if(p_hi > p_lo)
						{
						len = p_hi - p_lo + 1;
						}
						else
						{
						len = p_lo - p_hi + 1;
						}
					}

				len = (len * 3) + 2;
				break;
			case FST_VT_GEN_STRING: len = 0; break;
			case FST_VT_VCD_EVENT: len = (len != 0) ? len : 1;  break;
			default:
				if(len == 0) { len = 1; }
				break;
			}

		st = strtok(NULL, " \t"); /* vcdid */
		hash = vcdid_hash(st, strlen(st));

		if(hash == (hash_max+1))
			{
			hash_max = hash;
			}
		else
		if((hash>0)&&(hash<=hash_max))
			{
			/* general case with aliases */
			}
		else
			{
			hash_kill = 1;
			}

		nam = strtok(NULL, " \t"); /* name */
		st = strtok(NULL, " \t"); /* $end */

		if(st)
			{
			if(strncmp(st, "$end", 4))
				{
				*(st-1) = ' ';
				}

			node = jrb_find_int(vcd_ids, hash);
			if(!node)
				{
				Jval val;
				returnedhandle = fstWriterCreateVar(ctx, vartype, !var_direction ? FST_VD_IMPLICIT : var_direction[var_direction_idx++], len, nam, 0);
				val.i = returnedhandle;
				jrb_insert_int(vcd_ids, hash, val)->val2.i = len;
				}
				else
				{
				fstWriterCreateVar(ctx, vartype, !var_direction ? FST_VD_IMPLICIT : var_direction[var_direction_idx++], node->val2.i, nam, node->val.i);
				}

#if defined(VCD2FST_EXTLOAD_CONV)
			if(var_direction)
				{
				if(var_direction_idx == numfacs)
					{
					free(var_direction);
					var_direction = NULL;
					}
				}
#endif
			}
		}
	else
	if(!strncmp(buf1, "scope", 5))
		{
		char *st = strtok(buf+6, " \t");
		enum fstScopeType scopetype = FST_ST_VCD_MODULE;

		switch(st[0])
			{
			case 'm':
				if(!strcmp(st, "module"))
					{
					}
				break;

			case 't':
				if(!strcmp(st, "task"))
					{
					scopetype = FST_ST_VCD_TASK;
					}
				break;

			case 'f':
				if(!strcmp(st, "function"))
					{
					scopetype = FST_ST_VCD_FUNCTION;
					}
				else
				if(!strcmp(st, "fork"))
					{
					scopetype = FST_ST_VCD_FORK;
					}
				break;

			case 'b':
				if(!strcmp(st, "begin"))
					{
					scopetype = FST_ST_VCD_BEGIN;
					}
				break;

			case 'g':
				if(!strcmp(st, "generate"))
					{
					scopetype = FST_ST_VCD_GENERATE;
					}
				break;

			case 's':
				if(!strcmp(st, "struct"))
					{
					scopetype = FST_ST_VCD_STRUCT;
					}
				break;

			case 'u':
				if(!strcmp(st, "union"))
					{
					scopetype = FST_ST_VCD_UNION;
					}
				break;

			case 'c':
				if(!strcmp(st, "class"))
					{
					scopetype = FST_ST_VCD_CLASS;
					}
				break;

			case 'i':
				if(!strcmp(st, "interface"))
					{
					scopetype = FST_ST_VCD_INTERFACE;
					}
				break;

			case 'p':
				if(!strcmp(st, "package"))
					{
					scopetype = FST_ST_VCD_PACKAGE;
					}
				else
				if(!strcmp(st, "program"))
					{
					scopetype = FST_ST_VCD_PROGRAM;
					}
				break;

			case 'v':
				if(!strcmp(st, "vhdl_architecture"))
					{
					scopetype = FST_ST_VHDL_ARCHITECTURE;
					}
				else
				if(!strcmp(st, "vhdl_procedure"))
					{
					scopetype = FST_ST_VHDL_PROCEDURE;
					}
				else
				if(!strcmp(st, "vhdl_function"))
					{
					scopetype = FST_ST_VHDL_FUNCTION;
					}
				else
				if(!strcmp(st, "vhdl_record"))
					{
					scopetype = FST_ST_VHDL_RECORD;
					}
				else
				if(!strcmp(st, "vhdl_process"))
					{
					scopetype = FST_ST_VHDL_PROCESS;
					}
				else
				if(!strcmp(st, "vhdl_block"))
					{
					scopetype = FST_ST_VHDL_BLOCK;
					}
				else
				if(!strcmp(st, "vhdl_for_generate"))
					{
					scopetype = FST_ST_VHDL_FOR_GENERATE;
					}
				else
				if(!strcmp(st, "vhdl_if_generate"))
					{
					scopetype = FST_ST_VHDL_IF_GENERATE;
					}
				else
				if(!strcmp(st, "vhdl_generate"))
					{
					scopetype = FST_ST_VHDL_GENERATE;
					}
				break;

			default:
				break;
			}

		st = strtok(NULL, " \t");

#if defined(VCD2FST_EXTLOAD_CONV)
#ifdef _WAVE_HAVE_JUDY
		if(PJArray)
			{
			const char *fst_scope_name2 = fstReaderPushScope(xc, st, NULL);
			PPvoid_t PPValue = JudySLGet(PJArray, (uint8_t *)fst_scope_name2, PJE0);

			if(PPValue)
				{
				unsigned char st_replace = (*((unsigned char *)*PPValue)) - 1;
				if(st_replace != FST_ST_VCD_MODULE)
					{
					scopetype = st_replace;
					}

				if((scopetype == FST_ST_VCD_GENERATE)||(scopetype == FST_ST_VCD_STRUCT))
					{
					PPValue = NULL;
					}

				fstWriterSetScope(ctx, scopetype, st, PPValue ? ((char *)(*PPValue)+1) : NULL);
				}
				else
				{
				fstWriterSetScope(ctx, scopetype, st, NULL);
				}
			}
#else
		if(comp_name_jrb)
			{
			const char *fst_scope_name2 = fstReaderPushScope(xc, st, NULL);
			char cstring[65537];
			JRB str;

			strcpy(cstring, fst_scope_name2);
			str = jrb_find_str(comp_name_jrb, cstring);

			if(str)
				{
				unsigned char st_replace = str->val.s[0] - 1;
				if(st_replace != FST_ST_VCD_MODULE)
					{
					scopetype = st_replace;
					}

				if((scopetype == FST_ST_VCD_GENERATE)||(scopetype == FST_ST_VCD_STRUCT))
					{
					str = NULL;
					}

				fstWriterSetScope(ctx, scopetype, st, str ? (str->val.s+1) : NULL);
				}
				else
				{
				fstWriterSetScope(ctx, scopetype, st, NULL);
				}
			}
#endif
			else
#endif
			{
			fstWriterSetScope(ctx, scopetype, st, NULL);
			}
		}
	else
	if(!strncmp(buf1, "upscope", 7))
		{
		fstWriterSetUpscope(ctx);
#if defined(VCD2FST_EXTLOAD_CONV)
		if(xc)
			{
			fstReaderPopScope(xc);
			}
#endif
		}
	else
	if(!strncmp(buf1, "endd", 4))
		{
#if defined(VCD2FST_EXTLOAD_CONV)
#ifdef _WAVE_HAVE_JUDY
		if(PJArray)
#else
		if(comp_name_jrb)
#endif
			{
			dealloc_scope();
			}
#endif
		if(port_encountered && (!compression_explicitly_set) && (pack_type == FST_WR_PT_LZ4)) /* EVCD data compresses far better with fastlz, so use if not directed explicitly */
			{
			fstWriterSetPackType(ctx, (pack_type = FST_WR_PT_FASTLZ));
			}
		break;
		}
	else
	if(!strncmp(buf1, "timezero", 8))
		{
		char *pnt;
		int64_t tzero = 0;

		if((pnt = strstr(buf, "$end")))
			{
			*pnt = 0;
			sscanf(buf+10, "%"SCNd64, &tzero);
			}
		else
                        {
                        ss = getline_replace(&wbuf, &buf, &glen, f);
                        if(!ss)
                                {
                                break;
                                }
                        line++;
			sscanf(buf, "%"SCNd64, &tzero);
                        }

		fstWriterSetTimezero(ctx, tzero);
		}
	else
	if(!strncmp(buf1, "timescale", 9))
		{
		char *pnt;
		char *num = NULL;
		int exp = -9;
		int tv = 1;

		if((pnt = strstr(buf, "$end")))
			{
			*pnt = 0;
			num = strchr(buf, '1');
			if(!num)
				{
				num = strchr(buf, '0'); /* verilator */
				if(num) { *num = '1'; }
				}
			}

		if(!num)
			{
			ss = getline_replace(&wbuf, &buf, &glen, f);
			if(!ss)
				{
				break;
				}
			line++;
			num = buf;
			}

		pnt = num;
		while(*pnt)
			{
			int mat = 0;
			switch(*pnt)
				{
				case 'm': exp = -3;  mat = 1; break;
				case 'u': exp = -6;  mat = 1; break;
				case 'n': exp = -9;  mat = 1; break;
				case 'p': exp = -12; mat = 1; break;
				case 'f': exp = -15; mat = 1; break;
				case 'a': exp = -18; mat = 1; break;
				case 'z': exp = -21; mat = 1; break;
				case 's': exp =  0;  mat = 1; break;
				default: break;
				}
			if(mat) break;
			pnt++;
			}

		tv = atoi(num);
		if(tv == 10)
			{
			exp++;
			}
		else
		if(tv == 100)
			{
			exp+=2;
			}

		fstWriterSetTimescale(ctx, exp);
		}
	else
	if(!strncmp(buf1, "date", 4))
		{
		char *pnt, *rsp;
		int found = 0;

		if((pnt = strstr(buf, "$end")))
			{
                        *pnt = 0;
			pnt = buf + 5;
			while(*pnt && ((*pnt)==' ')) { pnt++; }
			while((rsp = strrchr(pnt, ' ')))
				{
				if(*(rsp+1) == 0)
					{
					*rsp = 0;
					}
					else
					{
					break;
					}
				}
			if(strlen(pnt)) { found = 1; }
			}
		else
			{
			pnt = buf + 5;
			while(*pnt && ((*pnt)==' ')) { pnt++; }
			while((rsp = strrchr(pnt, ' ')))
				{
				if(*(rsp+1) == 0)
					{
					*rsp = 0;
					}
					else
					{
					break;
					}
				}
			if(strlen(pnt) > 3) { found = 1; }
			}

		if(!found)
			{
			ss = getline_replace(&wbuf, &buf, &glen, f);
			if(!ss)
				{
				break;
				}
			line++;
			pnt = buf;
			}

		while(*pnt == '\t') pnt++;
		fstWriterSetDate(ctx, pnt);
		}
	else
	if((!strncmp(buf1, "version", 7)) || (!strncmp(buf1, "comment", 7)))
		{
		char *pnt, *crpnt, *rsp;
		int is_version = (buf[1] == 'v');

                if((pnt = strstr(buf, "$end")))
                        {
                        *pnt = 0;
			pnt = buf+8;
			while(*pnt && ((*pnt)==' ')) { pnt++; }
			while((rsp = strrchr(pnt, ' ')))
				{
				if(*(rsp+1) == 0)
					{
					*rsp = 0;
					}
					else
					{
					break;
					}
				}
                        }
			else
			{
			ss = getline_replace(&wbuf, &buf, &glen, f);
			if(!ss)
				{
				break;
				}
			line++;
			pnt = buf;
			}

		while(*pnt == '\t') pnt++;
		crpnt = strchr(pnt, '\n');
		if(crpnt) *crpnt = 0;
		crpnt = strchr(pnt, '\r');
		if(crpnt) *crpnt = 0;

		if(is_version)
			{
			fstWriterSetVersion(ctx, pnt);
			}
			else
			{
			fstWriterSetComment(ctx, pnt);
			}
		}
	}

if((!hash_kill) && (vcd_ids))
	{
	unsigned int hash;

	node_len_array = calloc(hash_max + 1, sizeof(int));

	for(hash=1;hash<=hash_max;hash++)
		{
		node = jrb_find_int(vcd_ids, hash);
		if(node)
			{
			node_len_array[hash] = node->val2.i;
			}
			else
			{
			node_len_array[hash] = 1; /* should never happen */
			}
		}

	jrb_free_tree(vcd_ids);
	vcd_ids = NULL;
	}
	else
	{
	hash_kill = 1; /* scan-build */
	}

for(;;) /* was while(!feof(f)) */
	{
	unsigned int hash;
	uint64_t tim;
	char *nl, *sp;
	double doub;

	ss = getline_replace(&wbuf, &buf, &glen, f);
	if(!ss)
		{
		break;
		}

	nl = buf;
	while(*nl)
		{
		if((*nl == '\n') || (*nl == '\r'))
			{
			*nl = 0;
			break;
			}
		nl++;
		}

	switch(buf[0])
		{
		case '0':
		case '1':
		case 'x':
		case 'z':
			hash = vcdid_hash(buf+1, nl - (buf+1));
			if(!hash_kill)
				{
				fstWriterEmitValueChange(ctx, hash, buf);
				}
				else
				{
				node = jrb_find_int(vcd_ids, hash);
				if(node)
					{
					fstWriterEmitValueChange(ctx, node->val.i, buf);
					}
					else
					{
					}
				}
			break;

		case 'b':
			{ /* this block replaces the single statement sp = strchr(buf, ' ');        */
			  /* as the odds are the VCD ID will be small compared to the vector length */
			char *sp_scan = nl;
			sp = NULL;

			/* if(buf != sp_scan) [can't happen or switch() wouldn't get here] */
				{
				while(buf != --sp_scan)
					{
					if(*sp_scan == ' ')
						{
						sp = sp_scan;
						break;
						}
					}
				}
			}

			if(!sp) break;
			*sp = 0;
			hash = vcdid_hash(sp+1, nl - (sp+1));
			if(!hash_kill)
				{
				int bin_len = sp - (buf + 1); /* strlen(buf+1) */
				int node_len = node_len_array[hash];

				if(bin_len >= node_len)
					{
					fstWriterEmitValueChange(ctx, hash, buf+1);
					}
					else
					{
					int delta = node_len - bin_len;

					if(node_len >= bin_fixbuff_len)
						{
						bin_fixbuff_len = node_len + 1;
						bin_fixbuff = realloc_2(bin_fixbuff, bin_fixbuff_len);
						}

					memset(bin_fixbuff, buf[1] != '1' ? buf[1] : '0', delta);
					memcpy(bin_fixbuff + delta, buf+1, bin_len);
					fstWriterEmitValueChange(ctx, hash, bin_fixbuff);
					}
				}
				else
				{
				node = jrb_find_int(vcd_ids, hash);
				if(node)
					{
					int bin_len = sp - (buf + 1); /* strlen(buf+1) */
					int node_len = node->val2.i;
					if(bin_len >= node_len)
						{
						fstWriterEmitValueChange(ctx, node->val.i, buf+1);
						}
						else
						{
						int delta = node_len - bin_len;

						if(node_len >= bin_fixbuff_len)
							{
							bin_fixbuff_len = node_len + 1;
							bin_fixbuff = realloc_2(bin_fixbuff, bin_fixbuff_len);
							}

						memset(bin_fixbuff, buf[1] != '1' ? buf[1] : '0', delta);
						memcpy(bin_fixbuff + delta, buf+1, bin_len);
						fstWriterEmitValueChange(ctx, node->val.i, bin_fixbuff);
						}
					}
					else
					{
					}
				}
			break;

		case 's':
			sp = strchr(buf, ' ');
			if(!sp) break;
			*sp = 0;
			hash = vcdid_hash(sp+1, nl - (sp+1));
			if(!hash_kill)
				{
				int bin_len = sp - (buf + 1); /* strlen(buf+1) */

				bin_len = fstUtilityEscToBin(NULL, (unsigned char *)(buf+1), bin_len);
				fstWriterEmitVariableLengthValueChange(ctx, hash, buf+1, bin_len);
				}
				else
				{
				node = jrb_find_int(vcd_ids, hash);
				if(node)
					{
					int bin_len = sp - (buf + 1); /* strlen(buf+1) */

					bin_len = fstUtilityEscToBin(NULL, (unsigned char *)(buf+1), bin_len);
					fstWriterEmitVariableLengthValueChange(ctx, node->val.i, buf+1, bin_len);
					}
					else
					{
					}
				}
			break;

		case 'p':
			{
			char *src = buf+1;
			char *pnt;
			int pchar = 0;
			int p_len = strlen(src);

			if(p_len >= bin_fixbuff_len)
				{
				bin_fixbuff_len = p_len + 1;
				bin_fixbuff = realloc_2(bin_fixbuff, bin_fixbuff_len);
				}
			pnt = bin_fixbuff;

			for(;;)
				{
				if(!*src) break;
				if(isspace((int)(unsigned char)*src))
					{
					if(pchar != ' ') { *(pnt++) = pchar = ' '; }
					src++;
					continue;
					}
				*(pnt++) = pchar = *(src++);
				}
			*pnt = 0;

			sp = strchr(bin_fixbuff, ' ');
			if(!sp) break;
			sp = strchr(sp+1, ' ');
			if(!sp) break;
			sp = strchr(sp+1, ' ');
			if(!sp) break;
			*sp = 0;

			hash = vcdid_hash(sp+1, strlen(sp+1)); /* nl is no longer good here */
			if(!hash_kill)
				{
				fstWriterEmitValueChange(ctx, hash, bin_fixbuff);
				}
				else
				{
				node = jrb_find_int(vcd_ids, hash);
				if(node)
					{
					fstWriterEmitValueChange(ctx, node->val.i, bin_fixbuff);
					}
					else
					{
					}
				}
			}
			break;

		case 'r':
			sp = strchr(buf, ' ');
			if(!sp) break;
			hash = vcdid_hash(sp+1, nl - (sp+1));
			if(!hash_kill)
				{
		                sscanf(buf+1,"%lg",&doub);
				fstWriterEmitValueChange(ctx, hash, &doub);
				}
				else
				{
				node = jrb_find_int(vcd_ids, hash);
				if(node)
					{
			                sscanf(buf+1,"%lg",&doub);
					fstWriterEmitValueChange(ctx, node->val.i, &doub);
					}
					else
					{
					}
				}
			break;

		case 'h': /* same as 01xz above but moved down here as it's less common */
		case 'u':
		case 'w':
		case 'l':
		case '-':
			hash = vcdid_hash(buf+1, nl - (buf+1));
			if(!hash_kill)
				{
				fstWriterEmitValueChange(ctx, hash, buf);
				}
				else
				{
				node = jrb_find_int(vcd_ids, hash);
				if(node)
					{
					fstWriterEmitValueChange(ctx, node->val.i, buf);
					}
					else
					{
					}
				}
			break;

		case '#':
			tim = atoi_2((unsigned char *)(buf+1));
			if((tim >= prev_tim)||(!prev_tim))
				{
				prev_tim = tim;
				fstWriterEmitTimeChange(ctx, tim);
				}
			break;

		default:
			if(!strncmp(buf, "$dumpon", 7))
				{
				fstWriterEmitDumpActive(ctx, 1);
				}
			else
			if(!strncmp(buf, "$dumpoff", 8))
				{
				fstWriterEmitDumpActive(ctx, 0);
				}
			else
			if(!strncmp(buf, "$dumpvars", 9))
				{
				/* nothing */
				}
			else
				{
				/* printf("FST '%s'\n", buf); */
				}
			break;
		}
	}

fstWriterClose(ctx);

#if defined(VCD2FST_EXTLOAD_CONV)
if(xc)
	{
	fstReaderClose(xc);
	}
#endif


if(vcd_ids)
	{
	jrb_free_tree(vcd_ids);
	vcd_ids = NULL;
	}

free(bin_fixbuff); bin_fixbuff = NULL;
free(wbuf); wbuf = NULL;
free(node_len_array); node_len_array = NULL;

if(f != stdin)
	{
	if(is_popen)
		{
		pclose(f);
		}
		else
		{
		fclose(f);
		}
	}

return(0);
}


void print_help(char *nam)
{
#ifdef VCD2FST_EXTLOADERS_CONV

int slen;
char *ucase_ext = calloc(1, 1024);
int i;

ucase_ext[0] = 0;

#if defined(VCD2FST_EXTLOAD_CONV)
strcat(ucase_ext, "/");
strcat(ucase_ext, EXTLOAD_SUFFIX);
#endif

#if defined(VCD2FST_EXT2LOAD_CONV)
strcat(ucase_ext, "/");
strcat(ucase_ext, EXT2LOAD_SUFFIX);
#endif

#if defined(VCD2FST_EXT3LOAD_CONV)
strcat(ucase_ext, "/");
strcat(ucase_ext, EXT3LOAD_SUFFIX);
#endif

slen = strlen(ucase_ext);

for(i=0;i<slen;i++)
	{
	ucase_ext[i] = toupper(ucase_ext[i]);
	}

#endif


#ifdef __linux__
printf(
"Usage: %s [OPTION]... [VCDFILE] [FSTFILE]\n\n"
#ifdef VCD2FST_EXTLOADERS_CONV
"  -v, --vcdname=FILE         specify VCD%s input filename\n"
#else
"  -v, --vcdname=FILE         specify VCD input filename\n"
#endif
"  -f, --fstname=FILE         specify FST output filename\n"
"  -4, --fourpack             use lz4 algorithm for speed (default)\n"
"  -F, --fastpack             use fastlz algorithm for speed\n"
"  -Z, --zlibpack             use zlib algorithm for size\n"
"  -c, --compress             zlib compress entire file on close\n"
"  -p, --parallel             enable parallel mode\n"
"  -h, --help                 display this help then exit\n\n"

"Note that VCDFILE and FSTFILE are optional provided the\n"
"--vcdname and --fstname options are specified.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam
#ifdef VCD2FST_EXTLOADERS_CONV
,ucase_ext
#endif
);
#else
printf(
"Usage: %s [OPTION]... [VCDFILE] [FSTFILE]\n\n"
#ifdef VCD2FST_EXTLOADERS_CONV
"  -v FILE                    specify VCD%s input filename\n"
#else
"  -v FILE                    specify VCD input filename\n"
#endif
"  -f FILE                    specify FST output filename\n"
"  -4                         use lz4 algorithm for speed (default)\n"
"  -F                         use fastlz algorithm for speed\n"
"  -Z                         use zlib algorithm for size\n"
"  -c                         zlib compress entire file on close\n"
"  -p                         enable parallel mode\n"
"  -h                         display this help then exit\n\n"

"Note that VCDFILE and FSTFILE are optional provided the\n"
"--vcdname and --fstname options are specified.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam
#ifdef VCD2FST_EXTLOADERS_CONV
,ucase_ext
#endif
);
#endif

#ifdef VCD2FST_EXTLOADERS_CONV
free(ucase_ext);
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
		{"fstname", 1, 0, 'f'},
		{"fastpack", 0, 0, 'F'},
		{"fourpack", 0, 0, '4'},
		{"zlibpack", 0, 0, 'Z'},
		{"compress", 0, 0, 'c'},
		{"parallel", 0, 0, 'p'},
                {"help", 0, 0, 'h'},
                {0, 0, 0, 0}
                };

        c = getopt_long (argc, argv, "v:f:ZF4cph", long_options, &option_index);
#else
        c = getopt      (argc, argv, "v:f:ZF4cph");
#endif

        if (c == -1) break;     /* no more args */

        switch (c)
                {
		case 'v':
			if(vname) free(vname);
                        vname = malloc(strlen(optarg)+1);
                        strcpy(vname, optarg);
			break;

		case 'f':
			if(lxname) free(lxname);
                        lxname = malloc(strlen(optarg)+1);
                        strcpy(lxname, optarg);
			break;

		case 'Z':
			compression_explicitly_set = 1;
			pack_type = FST_WR_PT_ZLIB;
			break;

		case 'F':
			compression_explicitly_set = 1;
			pack_type = FST_WR_PT_FASTLZ;
			break;

		case '4':
			compression_explicitly_set = 1;
			pack_type = FST_WR_PT_LZ4;
			break;

		case 'c':
			repack_all = 1;
			break;

		case 'p':
			parallel_mode = 1;
			break;

                case 'h':
			print_help(argv[0]);
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
                        vname = malloc(strlen(argv[optind])+1);
                        strcpy(vname, argv[optind++]);
                        }
                else if(!lxname)
                        {
                        lxname = malloc(strlen(argv[optind])+1);
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

fst_main(vname, lxname);

free(vname); free(lxname);

return(0);
}

