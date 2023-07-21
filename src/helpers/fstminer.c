/*
 * Copyright (c) 2012-2014 Tony Bybell.
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
#include "fst/fstapi.h"

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "wave_locale.h"

static char *match = NULL;
static uint32_t matchlen = 0;
static int names_only = 0;
static char *killed_list = NULL;
char killed_value = 1;
static char **fac_names = NULL;
static unsigned int *scope_idx = NULL;

static char **scope_names = NULL;
long allocated_scopes = 1;

static void strcpy_no_space(char *d, const char *s)
{
while(*s)
	{
	char ch = *(s++);
	if(ch != ' ')
		{
		*(d++) = ch;
		}
	}
*d = 0;
}


static void extractVarNames(void *xc)
{
struct fstHier *h;
char *s;
const char *fst_scope_name = NULL;
int fst_scope_name_len = 0;
intptr_t snum = 0;
intptr_t max_snum = 0;

while((h = fstReaderIterateHier(xc)))
        {
        switch(h->htyp)
                {
                case FST_HT_SCOPE:
			snum = ++max_snum;
                        fst_scope_name = fstReaderPushScope(xc, h->u.scope.name, (void *)(snum));
			/* fst_scope_name_len = fstReaderGetCurrentScopeLen(xc); scan-build */

			if(snum >= allocated_scopes)
				{
				long new_allocated_scopes = allocated_scopes * 2;
				char **scope_names_2 = calloc(new_allocated_scopes, sizeof(char *));

				memcpy(scope_names_2, scope_names, allocated_scopes * sizeof(char *));
				free(scope_names);

				scope_names = scope_names_2;
				allocated_scopes = new_allocated_scopes;
				}

			scope_names[snum] = strdup(fst_scope_name);
                        break;
                case FST_HT_UPSCOPE:
                        /* fst_scope_name = scan-build */ fstReaderPopScope(xc);
			fst_scope_name_len = fstReaderGetCurrentScopeLen(xc);
			snum = fst_scope_name_len ? (intptr_t)fstReaderGetCurrentScopeUserInfo(xc) : 0;
                        break;
                case FST_HT_VAR:
			if(!h->u.var.is_alias)
				{
				scope_idx[h->u.var.handle] = snum;

				s = fac_names[h->u.var.handle] = malloc(h->u.var.name_length + 1);
				strcpy_no_space(s, h->u.var.name);
				}
                }
        }
}


static char *get_facname(void *lt, fstHandle pnt_facidx)
{
(void) lt;

if(scope_idx[pnt_facidx] && scope_names[scope_idx[pnt_facidx]])
	{
	char *fst_scope_name = scope_names[scope_idx[pnt_facidx]];
	int fst_scope_name_len = strlen(fst_scope_name);
	int fst_signal_name = strlen(fac_names[pnt_facidx]);
	char *s = malloc(fst_scope_name_len + 1 + fst_signal_name + 1);
	memcpy(s, fst_scope_name, fst_scope_name_len);
	s[fst_scope_name_len] = '.';
	memcpy(s + fst_scope_name_len + 1, fac_names[pnt_facidx], fst_signal_name + 1);
	return(s);
	}
	else
	{
	return(strdup(fac_names[pnt_facidx]));
	}
}


static void vcd_callback2(void *lt, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value, uint32_t plen)
{
if(plen >= matchlen)
	{
	if(!killed_list[pnt_facidx])
		{
		if((!match) || (pnt_value /* scan-build */ && (strstr((const char *)pnt_value, match))))
			{
			char *fn;
			fn = get_facname(lt, pnt_facidx);

			if(!names_only)
				{
				printf("#%"PRIu64" %s %s\n", pnt_time, fn, pnt_value);
				}
				else
				{
				printf("%s\n", fn);
				}
			free(fn);

			if(killed_value)
				{
				fstReaderClrFacProcessMask(lt, pnt_facidx);
				killed_list[pnt_facidx] = 1;
				}
			}
		}
	}
}


static void vcd_callback(void *lt, uint64_t pnt_time, fstHandle pnt_facidx, const unsigned char *pnt_value)
{
uint32_t plen;

if(pnt_value)
	{
	plen = strlen((const char *)pnt_value);
	}
	else
	{
	plen = 0;
	}

vcd_callback2(lt, pnt_time, pnt_facidx, pnt_value, plen);
}


int process_fst(char *fname)
{
void *lt;
int i;

lt=fstReaderOpen(fname);
if(lt)
	{
	int numfacs;

	numfacs = fstReaderGetVarCount(lt) + 1;
	killed_list = calloc(numfacs, sizeof(char));

	fac_names = calloc(numfacs, sizeof(char *));
	scope_names = calloc(allocated_scopes, sizeof(char *));
	scope_idx = calloc(numfacs, sizeof(unsigned int));

	extractVarNames(lt);

	fstReaderSetFacProcessMaskAll(lt);
	fstReaderIterBlocks2(lt, vcd_callback, vcd_callback2, lt, NULL);

	for(i=0;i<allocated_scopes;i++) { free(scope_names[i]); }
	free(scope_names);
	free(scope_idx);

	for(i=0;i<numfacs;i++) { free(fac_names[i]); }
	free(fac_names);

	fstReaderClose(lt);
	free(killed_list);
	}
	else
	{
	fprintf(stderr, "lt=fstReaderOpen failed\n");
	return(255);
	}

return(0);
}

/*******************************************************************************/

void print_help(char *nam)
{
#ifdef __linux__
printf(
"Usage: %s [OPTION]... [FSTFILE]\n\n"
"  -d, --dumpfile=FILE        specify FST input dumpfile\n"
"  -m, --match                bitwise match value\n"
"  -x, --hex                  hex match value\n"
"  -n, --namesonly            emit facsnames only (gtkwave savefile)\n"
"  -c, --comprehensive        do not stop after first match\n"
"  -h, --help                 display this help then exit\n\n"
"First occurrence of facnames with times and matching values are emitted to\nstdout.  Using -n generates a gtkwave save file.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#else
printf(
"Usage: %s [OPTION]... [FSTFILE]\n\n"
"  -d                         specify FST input dumpfile\n"
"  -m                         bitwise match value\n"
"  -x                         hex match value\n"
"  -n                         emit facsnames only\n"
"  -c                         do not stop after first match\n"
"  -h                         display this help then exit (gtkwave savefile)\n\n"
"First occurrence of facnames with times and matching values are emitted to\nstdout.  Using -n generates a gtkwave save file.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#endif

exit(0);
}


int main(int argc, char **argv)
{
char opt_errors_encountered=0;
char *lxname=NULL;
int c;
int rc;
uint32_t i, j, k;
int comprehensive = 0;


WAVE_LOCALE_FIX

while (1)
        {
#ifdef __linux__
        int option_index = 0;

        static struct option long_options[] =
                {
		{"dumpfile", 1, 0, 'd'},
		{"match", 1, 0, 'm'},
		{"hex", 1, 0, 'x'},
		{"namesonly", 0, 0, 'n'},
		{"comprehensive", 0, 0, 'c'},
                {"help", 0, 0, 'h'},
                {0, 0, 0, 0}
                };

        c = getopt_long (argc, argv, "d:m:x:nch", long_options, &option_index);
#else
        c = getopt      (argc, argv, "d:m:x:nch");
#endif

        if (c == -1) break;     /* no more args */

        switch (c)
                {
		case 'c':
			comprehensive = 1;
			break;

		case 'n':
			names_only = 1;
			break;

		case 'd':
			if(lxname) free(lxname);
                        lxname = malloc(strlen(optarg)+1);
                        strcpy(lxname, optarg);
			break;

		case 'm': if(match) free(match);
			match = malloc((matchlen = strlen(optarg))+1);
			strcpy(match, optarg);
			break;

		case 'x': if(match) free(match);
			match = malloc((matchlen = 4*strlen(optarg))+1);
			for(i=0,k=0;i<matchlen;i+=4,k++)
				{
				int ch = tolower((int)(unsigned char)optarg[k]);

				if(ch=='z')
					{
					for(j=0;j<4;j++)
						{
						match[i+j] = 'z';
						}
					}
				else
				if((ch>='0')&&(ch<='9'))
					{
					ch -= '0';
					for(j=0;j<4;j++)
						{
						match[i+j] = ((ch>>(3-j))&1) + '0';
						}
					}
				else
				if((ch>='a')&&(ch<='f'))
					{
					ch = ch - 'a' + 10;
					for(j=0;j<4;j++)
						{
						match[i+j] = ((ch>>(3-j))&1) + '0';
						}
					}
				else /* "x" */
					{
					for(j=0;j<4;j++)
						{
						match[i+j] = 'x';
						}
					}
				}
			match[matchlen] = 0;
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

if(!names_only && comprehensive)
	{
	killed_value = 0;
	}

if(opt_errors_encountered)
        {
        print_help(argv[0]);
        }

if (optind < argc)
        {
        while (optind < argc)
                {
                if(lxname)
                        {
			free(lxname);
			}
                lxname = malloc(strlen(argv[optind])+1);
                strcpy(lxname, argv[optind++]);
                }
        }

if(!lxname)
        {
        print_help(argv[0]);
        }

rc=process_fst(lxname);
free(match);
free(lxname);

return(rc);
}
