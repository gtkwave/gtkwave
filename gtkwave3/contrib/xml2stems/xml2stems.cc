/*
 * Copyright (c) 2018 Tony Bybell.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <map>
#include <stack>
using namespace std;

#define BUF_SIZ 65536


void xml2stems(FILE *fi, FILE *fo, int is_verilator_sim)
{
std::map <string, string> fId;
std::stack<string> mId;

while(!feof(fi))
	{
	char buf[BUF_SIZ + 1];
	int endtag;
	char *ln = fgets(buf, BUF_SIZ, fi);	
	char *pnt = ln;
	if(!pnt) goto bot;

	while(*pnt)
		{
		if(*pnt == ' ') { pnt++; continue; } else { break; }
		}
	if(*pnt != '<') goto bot;

	pnt++;
	endtag = (*pnt == '/');
	pnt += endtag;

	switch(*pnt)
		{
		case 'm':
			if(!strncmp(pnt, "module", 6))
				{
				if(!endtag)
					{
					char *qts[6];
					char *s = pnt + 6;
					int numqt = 0;
					int tm = 0;

					while(*s)
						{
						if(*s == '"')
							{
							qts[numqt++] = s;
							if(numqt == 6) break;
							}
						s++;
						}
	
					if(numqt == 6)
						{
						if(strstr(qts[3]+1, "topModule=\"1\""))
							{
							numqt = 4;
							tm = is_verilator_sim;
							}
						}

					if(numqt == 4)
						{
						char *fl = qts[0] + 1;
						char *nam = qts[2] + 1;
						qts[1][0] = qts[3][0] = 0;
	
						mId.push(nam);

						char fl_dup[strlen(fl)+1];
						char *s = fl; char *d = fl_dup;
						while(isalpha(*s)) { *(d++) = *(s++); }
						*d = 0;

						unsigned int lineno = atoi(s);
						const char *mnam = fId[fl_dup].c_str();
						fprintf(fo, "++ module %s file %s lines %d - %d\n", nam, mnam, lineno, lineno); /* don't need line number it truly ends at */
						if(tm)
							{
							fprintf(fo, "++ module TOP file %s lines %d - %d\n", "(VerilatorTop)", 1, 1);
							fprintf(fo, "++ comp %s type %s parent TOP\n", nam, nam);
							}
						}
					}
					else
					{
					if(!mId.empty())
						{
						mId.pop();
						}
					}
				}

		case 'f':
			if((!endtag) && (!strncmp(pnt, "file id", 7)))
				{
				char *qts[4];
				char *s = pnt + 7;
				int numqt = 0;
			
				while(*s)
					{
					if(*s == '"')
						{
						qts[numqt++] = s;
						if(numqt == 4) break;
						}
					s++;
					}

				if(numqt == 4)
					{
					char *cod = qts[0] + 1;
					char *fil = qts[2] + 1;
					qts[1][0] = qts[3][0] = 0;

					fId.insert(pair <string, string> (cod, fil));
					}
				}
			break;

		case 'i':
			if((!endtag) && (!strncmp(pnt, "instance", 8)))
				{
				char *qts[6];
				char *s = pnt + 8;
				int numqt = 0;
			
				while(*s)
					{
					if(*s == '"')
						{
						qts[numqt++] = s;
						if(numqt == 6) break;
						}
					s++;
					}

				if(numqt == 6)
					{
					char *cod = qts[0] + 1;
					char *nam = qts[2] + 1;
					char *defnam = qts[4] + 1;
					qts[1][0] = qts[3][0] = qts[5][0] = 0;

					if(!mId.empty())
						{
						fprintf(fo, "++ comp %s type %s parent %s\n", nam, defnam, mId.top().c_str()); 
						}
					}
				}
			break;


		case 'p':
			if(!strncmp(pnt, "primitive", 9))
				{
				if(!endtag)
					{
					char *qts[4];
					char *s = pnt + 9;
					int numqt = 0;

					while(*s)
						{
						if(*s == '"')
							{
							qts[numqt++] = s;
							if(numqt == 6) break;
							}
						s++;
						}
	
					if(numqt == 4)
						{
						char *fl = qts[0] + 1;
						char *nam = qts[2] + 1;
						qts[1][0] = qts[3][0] = 0;
	
						mId.push(nam);

						char fl_dup[strlen(fl)+1];
						char *s = fl; char *d = fl_dup;
						while(isalpha(*s)) { *(d++) = *(s++); }
						*d = 0;

						unsigned int lineno = atoi(s);
						const char *mnam = fId[fl_dup].c_str();
						fprintf(fo, "++ udp %s file %s lines %d - %d\n", nam, mnam, lineno, lineno); /* don't need line number it truly ends at */
						}
					}
					else
					{
					if(!mId.empty())
						{
						mId.pop();
						}
					}
				}
			break;
			
		default:
			break;
		}

	bot: 1;
	}
}


int main(int argc, char **argv)
{
FILE *fi, *fo;
int rc = 0;
int is_verilator_sim = 0;

while(argc > 1)
	{
	if(*argv[1] != '-')
		{
		break;
		}
	else
	if(!strcmp(argv[1], "--"))
		{
		argc--;
		argv++;
		break;
		}
	else
	if(!strcmp(argv[1], "-V") || !strcmp(argv[1], "--vl_sim"))
		{
		is_verilator_sim = 1;
		argc--;
		argv++;
		}
	else
	if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
		{
		argc = 1;
		break;
		}
	else
		{
		fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], argv[1]);
		argc = 1;
		break;		
		}
	}

switch(argc)
	{
	case 2:
		fi = (!strcmp("-", argv[1])) ? stdin : fopen(argv[1], "rb");
		if(fi)
			{
			fo = stdout;
			xml2stems(fi, fo, is_verilator_sim);
			if(fi != stdin) fclose(fi);
			}
			else
			{
			fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
			perror("Why");
			rc = 255;
			}
		break;

	case 3:
		fi = (!strcmp("-", argv[1])) ? stdin : fopen(argv[1], "rb");
		if(fi)
			{
			fo = fopen(argv[2], "wb");
			if(fo)
				{
				xml2stems(fi, fo, is_verilator_sim);
				if(fi != stdin) fclose(fi);
				fclose(fo);
				}
				else
				{
				if(fi != stdin) fclose(fi);
				fprintf(stderr, "Could not open '%s', exiting.\n", argv[2]);
				perror("Why");
				rc = 255;
				}
			}
			else
			{
			fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
			perror("Why");
			rc = 255;
			}
		break;

	default:
		printf("Usage: %s [OPTION] infile.xml [outfile.stems]\n\n"

                        "  -V, --vl_sim               add TOP hierarchy for Verilator sim\n"
			"  -h, --help                 display this help then exit\n\n"

			"Converts Verilator XML file to rtlbrowse stems format.  For example:\n\n"
			"verilator -Wno-fatal des.v -xml-only --bbox-sys\n"
			"xml2stems obj_dir/Vdes.xml des.stems\n"
			"gtkwave -t des.stems des.fst\n\n"
			"Use - as input filename for stdin.\n"
			"Omitting optional stems outfile name emits to stdout.\n", 
			argv[0]);
		break;
	}

bot:
return(rc);
}
