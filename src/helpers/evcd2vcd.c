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

#include "../../contrib/rtlbrowse/jrb.h"
#include "wave_locale.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <unistd.h>

ssize_t getline_replace(char **buf, size_t *len, FILE *f)
{
char *fgets_rc;

if(!*buf)
	{
	*buf = malloc(32768);
	*len = 32767;
	}

(*buf)[0] = 0;
fgets_rc = fgets(*buf, 32767, f);
if((!(*buf)[0])||(!fgets_rc))
	{
	return(-1);
	}
	else
	{
	return(1);
	}
}

JRB vcd_ids = NULL;

static unsigned int vcdid_hash(char *s)
{
unsigned int val=0;
int i;
int len = strlen(s);

s+=(len-1);

for(i=0;i<len;i++)
        {
        val *= 94;                              /* was 94 but XL uses '!' as right hand side chars which act as leading zeros */
        val += (((unsigned char)*s) - 32);

        s--;
        }

return(val);
}

static char *vcdid_unhash(unsigned int value)
{
static char buf[16];
char *pnt = buf;
unsigned int vmod;

/* zero is illegal for a value...it is assumed they start at one */
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
return(buf);
}


static void evcd_strcpy(char *dst, char *src, int dir)
{
static const char *evcd="DUNZduLHXTlh01?FAaBbCcf";
static const char *vcdi="01xz01zzzzzz01xz0011xxz";
static const char *vcdo="zzzzzz01xz0101xz1x0x01z";
const char  *vcd = dir ? vcdo : vcdi;
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

*dst=0; /* null terminate destination */
}


int evcd_main(char *vname)
{
FILE *f;
char *buf = NULL;
size_t glen;
int line = 0;
ssize_t ss;
JRB node;
char bin_fixbuff[32769];
char bin_fixbuff2[32769];

if(!strcmp("-", vname))
	{
	f = stdin;
	}
	else
	{
	f = fopen(vname, "rb");
	}

if(!f)
	{
	printf("could not open '%s', exiting.\n", vname);
	exit(255);
	}

vcd_ids = make_jrb();

while(!feof(f))
	{
	ss = getline_replace(&buf, &glen, f);
	if(ss == -1)
		{
		break;
		}
	line++;

	if(!strncmp(buf, "$var", 4))
		{
		char *st = strtok(buf+5, " \t");
		int len;
		char *nam;
		unsigned int hash;
		char *lbrack;

		switch(st[0])
			{
			case 'p':
				if(!strcmp(st, "port"))
					{
					break;
					}
				/* fallthrough */
			default:
				fprintf(stderr, "'%s' is an unsupported data type, exiting.\n", st);
				exit(255);
				break;
			}

		st = strtok(NULL, " \t");
                if(*st == '[') /* VCS extension */
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
			else
			{
			len = atoi(st);
			}

		st = strtok(NULL, " \t"); /* vcdid */
		hash = vcdid_hash(st);

		nam = strtok(NULL, " \t"); /* name */
		st = strtok(NULL, " \t"); /* $end */

		if(strncmp(st, "$end", 4))
			{
			*(st-1) = ' ';
			}

		node = jrb_find_int(vcd_ids, hash);
		if(!node)
			{
			Jval val;
			jrb_insert_int(vcd_ids, hash, val)->val2.i = len;
			}

		lbrack = strchr(nam, '[');
		if(!lbrack)
			{
			printf("$var wire %d %s %s_I $end\n", len, vcdid_unhash(hash * 2), nam);
			printf("$var wire %d %s %s_O $end\n", len, vcdid_unhash(hash * 2 + 1), nam);
			}
			else
			{
			*lbrack = 0;
			printf("$var wire %d %s %s_I", len, vcdid_unhash(hash * 2), nam);
			printf("[%s $end", lbrack+1);
			printf("$var wire %d %s %s_O", len, vcdid_unhash(hash * 2 + 1), nam);
			printf("[%s $end", lbrack+1);
			}
		}
	else
	if(!strncmp(buf, "$scope", 6))
		{
		printf("%s", buf);
		}
	else
	if(!strncmp(buf, "$upscope", 8))
		{
		printf("%s", buf);
		}
	else
	if(!strncmp(buf, "$endd", 5))
		{
		printf("%s", buf);
		break;
		}
	else
	if(!strncmp(buf, "$timescale", 10))
		{
                char *pnt;
                ss = getline_replace(&buf, &glen, f);
                if(ss == -1)
                        {
                        break;
                        }
                line++;
                pnt = buf;
		printf("$timescale\n%s$end\n", pnt);
		}
	else
	if(!strncmp(buf, "$date", 5))
		{
                char *pnt;
                ss = getline_replace(&buf, &glen, f);
                if(ss == -1)
                        {
                        break;
                        }
                line++;
                pnt = buf;
		printf("$date\n%s$end\n", pnt);
		}
	else
	if(!strncmp(buf, "$version", 8))
		{
                char *pnt;
                ss = getline_replace(&buf, &glen, f);
                if(ss == -1)
                        {
                        break;
                        }
                line++;
                pnt = buf;
		printf("$version\n%s$end\n", pnt);
		}
	}

while(!feof(f))
	{
	unsigned int hash;
	size_t len;
	char *nl, *sp;

	ss = getline_replace(&buf, &len, f);
	if(ss == -1)
		{
		break;
		}
	nl = strchr(buf, '\n');
	if(nl) *nl = 0;
	nl = strchr(buf, '\r');
	if(nl) *nl = 0;

	switch(buf[0])
		{
		case 'p':
			{
			char *src = buf+1;
			char *pnt = bin_fixbuff;
			int pchar = 0;

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
			sp = strchr(sp+1, ' ');
			sp = strchr(sp+1, ' ');
			*sp = 0;
			hash = vcdid_hash(sp+1);

			node = jrb_find_int(vcd_ids, hash);
			if(node)
				{
				bin_fixbuff[node->val2.i] = 0;
				if(node->val2.i == 1)
					{
					int dir;
					for(dir = 0; dir < 2; dir++)
						{
						evcd_strcpy(bin_fixbuff2, bin_fixbuff, dir);
						printf("%c%s\n", bin_fixbuff2[0], vcdid_unhash(hash*2+dir));
						}
					}
					else
					{
					int dir;
					for(dir = 0; dir < 2; dir++)
						{
						evcd_strcpy(bin_fixbuff2, bin_fixbuff, dir);
						printf("b%s %s\n", bin_fixbuff2, vcdid_unhash(hash*2+dir));
						}
					}
				}
				else
				{
				}
			}
			break;

		case '#':
			printf("%s\n", buf);
			break;

		default:
			if(!strncmp(buf, "$dumpon", 7))
				{
				printf("%s\n", buf);
				}
			else
			if(!strncmp(buf, "$dumpoff", 8))
				{
				printf("%s\n", buf);
				}
			else
			if(!strncmp(buf, "$dumpvars", 9))
				{
				printf("%s\n", buf);
				}
			else
				{
				/* printf("EVCD '%s'\n", buf); */
				}
			break;
		}
	}

if(buf)
	{
	free(buf);
	}
if(f != stdin) fclose(f);

exit(0);
}


void print_help(char *nam)
{
#ifdef __linux__
printf(
"Usage: %s [OPTION]... [EVCDFILE]\n\n"
"  -f, --filename=FILE        specify EVCD input filename\n"
"  -h, --help                 display this help then exit\n\n"

"Note that EVCDFILE is optional provided the --filename\n"
"option is specified.  VCD is emitted to stdout.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#else
printf(
"Usage: %s [OPTION]... [EVCDFILE]\n\n"
"  -f FILE                    specify EVCD input filename\n"
"  -h                         display this help then exit\n\n"

"Note that EVCDFILE is optional provided the --filename\n"
"option is specified.  VCD is emitted to stdout.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#endif

exit(0);
}


int main(int argc, char **argv)
{
char opt_errors_encountered=0;
char *vname=NULL;
int c;

WAVE_LOCALE_FIX

while (1)
        {
#ifdef __linux__
        int option_index = 0;

        static struct option long_options[] =
                {
		{"filename", 1, 0, 'f'},
                {"help", 0, 0, 'h'},
                {0, 0, 0, 0}
                };

        c = getopt_long (argc, argv, "f:h", long_options, &option_index);
#else
        c = getopt      (argc, argv, "f:h");
#endif

        if (c == -1) break;     /* no more args */

        switch (c)
                {
		case 'f':
			if(vname) free(vname);
                        vname = malloc(strlen(optarg)+1);
                        strcpy(vname, optarg);
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
			else
			{
			break;
			}
                }
        }

if(!vname)
        {
        print_help(argv[0]);
        }

evcd_main(vname);

free(vname);

return(0);
}

