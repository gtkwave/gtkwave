/*
 * Copyright (c) 2003-2013 Tony Bybell.
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

#define FST_VCD_WRITE_BUF_SIZ (2 * 1024 * 1024)

void print_help(char *nam)
{
#ifdef __linux__
printf(
"Usage: %s [OPTION]... [FSTFILE]\n\n"
"  -f, --fstname=FILE         specify FST input filename\n"
"  -o, --output=FILE          specify output filename\n"
"  -e, --extensions           emit FST extensions to VCD\n"
"  -h, --help                 display this help then exit\n\n"
"VCD is emitted to stdout if output filename is unspecified.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#else
printf(
"Usage: %s [OPTION]... [FSTFILE]\n\n"
"  -f                         specify FST input filename\n"
"  -o                         specify output filename\n"
"  -e                         emit FST extensions to VCD\n"
"  -h                         display this help then exit\n\n"
"VCD is emitted to stdout if output filename is unspecified.\n\n"
"Report bugs to <"PACKAGE_BUGREPORT">.\n",nam);
#endif

exit(0);
}


int main(int argc, char **argv)
{
char opt_errors_encountered=0;
char *fstname=NULL;
char *outname=NULL;
char *fvbuf=NULL;
int c;
struct fstReaderContext *xc;
FILE *fv;
int use_extensions = 0;

WAVE_LOCALE_FIX

while (1)
        {
#ifdef __linux__
        int option_index = 0;

        static struct option long_options[] =
                {
		{"extensions", 0, 0, 'e'},
		{"fstname", 1, 0, 'f'},
		{"output", 1, 0, 'o'},
                {"help", 0, 0, 'h'},
                {0, 0, 0, 0}
                };

        c = getopt_long (argc, argv, "ef:o:h", long_options, &option_index);
#else
        c = getopt      (argc, argv, "ef:o:h");
#endif

        if (c == -1) break;     /* no more args */

        switch (c)
                {
		case 'e':
			use_extensions = 1;
			break;

		case 'f':
			if(fstname) free(fstname);
                        fstname = malloc(strlen(optarg)+1);
                        strcpy(fstname, optarg);
			break;

		case 'o':
			if(outname) free(outname);
                        outname = malloc(strlen(optarg)+1);
                        strcpy(outname, optarg);
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
                if(!fstname)
                        {
                        fstname = malloc(strlen(argv[optind])+1);
                        strcpy(fstname, argv[optind++]);
                        }
			else
			{
			break;
			}
                }
        }

if(!fstname)
        {
        print_help(argv[0]);
        }


xc = fstReaderOpen(fstname);

if(!xc)
	{
	fprintf(stderr, "Could not open '%s', exiting.\n", fstname);
	exit(255);
	}

if(outname)
	{
	fv = fopen(outname, "wb");
	if(!fv)
		{
		fprintf(stderr, "Could not open '%s', exiting.\n", outname);
		perror("Why");
		exit(255);
		}
	fvbuf = malloc(FST_VCD_WRITE_BUF_SIZ);
	setvbuf(fv, fvbuf, _IOFBF, FST_VCD_WRITE_BUF_SIZ);
	}
	else
	{
	fv = stdout;
	}

fstReaderSetVcdExtensions(xc, use_extensions);	/* TRUE is incompatible with vfast and other tools */
if(!fstReaderProcessHier(xc, fv))		/* these 3 lines do all the VCD writing work */
	{
	fprintf(stderr, "could not process hierarchy for '%s', exiting.\n", fstname);
	exit(255);
	}
fstReaderSetFacProcessMaskAll(xc);		/* these 3 lines do all the VCD writing work */
fstReaderIterBlocks(xc, NULL, NULL, fv);	/* these 3 lines do all the VCD writing work */

fstReaderClose(xc);

if(outname)
	{
	free(outname);
	fclose(fv);
	}

free(fvbuf);
free(fstname);

exit(0);
}
