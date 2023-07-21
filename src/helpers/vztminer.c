/*
 * Copyright (c) 2003-2009 Tony Bybell.
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

#include "vzt_read.h"

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "wave_locale.h"

static char *match = NULL;
static uint32_t matchlen = 0;
static int names_only = 0;
static char *killed_list = NULL;
char killed_value = 1;

char vcd_blackout;

void vcd_callback(struct vzt_rd_trace **lt, vztint64_t *pnt_time, vztint32_t *pnt_facidx, char **pnt_value)
{
struct vzt_rd_geometry *g = vzt_rd_get_fac_geometry(*lt, *pnt_facidx);

/* fprintf(stderr, VZT_RD_LLD" %d %s\n", *pnt_time, *pnt_facidx, *pnt_value); */

if(!(*pnt_value)[0])
	{
	if(!vcd_blackout)
		{
		vcd_blackout = 1;
		/* printf("$dumpoff\n"); */
		}

	return;
	}
	else
	{
	if(vcd_blackout)
		{
		vcd_blackout = 0;
		/* printf("$dumpon\n"); */
		}
	}

if(g->len >= matchlen)
	{
	if(!killed_list[*pnt_facidx])
		{
		if((!match) || (strstr(*pnt_value, match)))
			{
			if(g->len > 1)
				{
				if(!names_only)
					{
					printf("#"VZT_RD_LLD" %s["VZT_RD_LD":"VZT_RD_LD"] %s\n", *pnt_time, vzt_rd_get_facname(*lt, *pnt_facidx), g->msb, g->lsb, *pnt_value);
					}
					else
					{
					printf("%s["VZT_RD_LD":"VZT_RD_LD"]\n", vzt_rd_get_facname(*lt, *pnt_facidx), g->msb, g->lsb);
					}
				}
				else
				{
				if(g->msb < 0)
					{
					if(!names_only)
						{
						printf("#"VZT_RD_LLD" %s %s\n", *pnt_time, vzt_rd_get_facname(*lt, *pnt_facidx), *pnt_value);
						}
						else
						{
						printf("%s\n", vzt_rd_get_facname(*lt, *pnt_facidx));
						}
					}
					else
					{
					if(!names_only)
						{
						printf("#"VZT_RD_LLD" %s["VZT_RD_LD"] %s\n", *pnt_time, vzt_rd_get_facname(*lt, *pnt_facidx), g->msb, *pnt_value);
						}
						else
						{
						printf("%s["VZT_RD_LD"]\n", vzt_rd_get_facname(*lt, *pnt_facidx), g->msb);
						}
					}
				}
			if(killed_value)
				{
				vzt_rd_clr_fac_process_mask(*lt, *pnt_facidx);
				killed_list[*pnt_facidx] = 1;
				}
			}
		}
	}
}


int process_vzt(char *fname)
{
struct vzt_rd_trace *lt;

lt=vzt_rd_init(fname);
if(lt)
	{
	int numfacs;

	vzt_rd_vectorize(lt);			/* coalesce bitblasted vectors */
	numfacs = vzt_rd_get_num_facs(lt);
	killed_list = calloc(numfacs, sizeof(char));
	vzt_rd_set_fac_process_mask_all(lt);
	vzt_rd_set_max_block_mem_usage(lt, 0);	/* no need to cache blocks */

	vzt_rd_iter_blocks(lt, vcd_callback, NULL);

	vzt_rd_close(lt);
	free(killed_list);
	}
	else
	{
	fprintf(stderr, "vzt_rd_init failed\n");
	return(255);
	}

return(0);
}

/*******************************************************************************/

void print_help(char *nam)
{
#ifdef __linux__
printf(
"Usage: %s [OPTION]... [VZTFILE]\n\n"
"  -d, --dumpfile=FILE        specify VZT input dumpfile\n"
"  -m, --match                bitwise match value\n"
"  -x, --hex                  hex match value\n"
"  -n, --namesonly            emit facsnames only (gtkwave savefile)\n"
"  -c, --comprehensive        do not stop after first match\n"
"  -h, --help                 display this help then exit\n\n"
"First occurrence of facnames with times and matching values are emitted to\nstdout.  Using -n generates a gtkwave save file.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#else
printf(
"Usage: %s [OPTION]... [VZTFILE]\n\n"
"  -d                         specify VZT input dumpfile\n"
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

rc=process_vzt(lxname);
free(lxname);

return(rc);
}

