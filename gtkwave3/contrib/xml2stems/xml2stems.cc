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
#include <queue>
using namespace std;

#define BUF_SIZ 65536

std::map <string, string>* parse_xml_tags(char *s, int *oneline)
{
char sprev;
std::map <string, string> *fId = new std::map <string, string>;
if(oneline) *oneline = 0;

while(*s)
	{
	if(!isspace(*s))
		{
		char *tag_s = s;
		char *tag_e = NULL;
		char *val_s = NULL;
		char *val_e = NULL;

		sprev = 0;
		while(*s)
			{
			if(!s) goto bot;

			if(*s == '>')
				{
				if(sprev == '/')
					{
					if(oneline) *oneline = 1;
					}
	
				goto bot;
				}
			sprev = *s;

			if(*s == '=') { tag_e = s; break; }
			s++;
			}

		while(*s)
			{
			if(!s) goto bot;
			if(*s == '"') { val_s = ++s; break; }
			s++;
			}

		while(*s)
			{
			if(!s) goto bot;
			if(*s == '"') { val_e = s; break; }
			s++;
			}

		if(tag_e && val_e)
			{
			*tag_e = 0;
			*val_e = 0;
			fId->insert(pair <string, string> (tag_s, val_s));
			}
		}
	s++;
	}

bot: return(fId);
}



void xml2stems(FILE *fi, FILE *fo, int is_verilator_sim)
{
std::map <string, string> fId;
std::stack<string> mId;
queue<string> bQueue;
int in_files = 0;
int geninst = 0;
int func_nesting_cnt = 0;

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
		case 'b':
			if(!strncmp(pnt, "begin", 5) && !func_nesting_cnt)
				{
				if(!endtag)
					{
					char *s = pnt + 5;
					int oneline = 0;
					std::map <string, string>*xmt = parse_xml_tags(s, &oneline);
					if(xmt)
						{
						const char *nam = (*xmt)[string("name")].c_str();
						const char *fl = (*xmt)[string("fl")].c_str();

						if(!oneline) 
							{
							bQueue.push((*xmt)[string("name")]);
							if(fl && nam && nam[0])
								{
								char fl_dup[strlen(fl)+1];
								const char *s = fl; char *d = fl_dup;
								while(isalpha(*s)) { *(d++) = *(s++); }
								*d = 0;
	
								unsigned int lineno = atoi(s);
								const char *mnam = fId[fl_dup].c_str();

								fprintf(fo, "++ begin %s file %s line %d\n", nam, mnam, lineno);

								char *genname = (char *)malloc(strlen(nam) + 32);
								sprintf(genname, "%s:G%d", nam, geninst++);
								fprintf(fo, "++ comp %s type %s parent %s\n", genname, genname, mId.top().c_str()); 
								fprintf(fo, "++ module %s file %s lines %d - %d\n", genname, mnam, lineno, lineno); /* don't need line number it truly ends at */
								mId.push(genname);
								free(genname);
								}
							}

						delete xmt;
						}
					}
				else
					{
					string bs = bQueue.front();
					bQueue.pop();
					const char *nam = bs.c_str();
					if(nam && nam[0])
						{
						mId.pop();
						fprintf(fo, "++ endbegin %s\n", nam);
						}
					}
				
				}
			break;

		case 'm':
			if(!strncmp(pnt, "module", 6) && strncmp(pnt, "module_files", 12))
				{
				if(!endtag)
					{
					char *s = pnt + 6;

                                	std::map <string, string>*xmt = parse_xml_tags(s, NULL);
					if(xmt)
						{
						const char *fl = (*xmt)[string("fl")].c_str();
						const char *nam = (*xmt)[string("name")].c_str();
						const char *tms = (*xmt)[string("topModule")].c_str();

						if(fl && nam && tms)
							{
							int tm = (tms[0] == '1') ? is_verilator_sim : 0;

							mId.push(nam);

							char fl_dup[strlen(fl)+1];
							const char *s = fl; char *d = fl_dup;
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
						delete xmt;
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
			if(!strncmp(pnt, "func", 4))
				{
				func_nesting_cnt = (!endtag) ? (func_nesting_cnt+1) : (func_nesting_cnt-1);
				}
			else
			if(!strncmp(pnt, "files", 5))
				{
				in_files = (!endtag);
				}
			else
				{
				if((!endtag) && (in_files) && (!strncmp(pnt, "file", 4)))
					{
					char *s = pnt + 4;
	                                std::map <string, string>*xmt = parse_xml_tags(s, NULL);
	
	                                if(xmt)
	                                       	{
	                                        const char *cod = (*xmt)[string("id")].c_str();
	                                        const char *fil = (*xmt)[string("filename")].c_str();
	
						if(cod && fil)
							{
							fId.insert(pair <string, string> (cod, fil));
							}
	
						delete xmt;
						}
					}
				}
			break;

		case 'i':
			if((!endtag) && (!strncmp(pnt, "instance", 8)))
				{
				char *s = pnt + 8;
				std::map <string, string>*xmt = parse_xml_tags(s, NULL);
			
				if(xmt)
					{
					const char *nam = (*xmt)[string("name")].c_str();
					const char *defnam = (*xmt)[string("defName")].c_str();
					if(!mId.empty() && nam && defnam)
						{
						fprintf(fo, "++ comp %s type %s parent %s\n", nam, defnam, mId.top().c_str()); 
						}

					delete xmt;
					}
				}
			break;


		case 'p':
			if(!strncmp(pnt, "primitive", 9))
				{
				if(!endtag)
					{
					char *s = pnt + 9;

                                	std::map <string, string>*xmt = parse_xml_tags(s, NULL);
					if(xmt)
						{
						const char *fl = (*xmt)[string("fl")].c_str();
						const char *nam = (*xmt)[string("name")].c_str();
	
						if(fl && nam)
							{
							mId.push(nam);
	
							char fl_dup[strlen(fl)+1];
							const char *s = fl; char *d = fl_dup;
							while(isalpha(*s)) { *(d++) = *(s++); }
							*d = 0;
	
							unsigned int lineno = atoi(s);
							const char *mnam = fId[fl_dup].c_str();
							fprintf(fo, "++ udp %s file %s lines %d - %d\n", nam, mnam, lineno, lineno); /* don't need line number it truly ends at */
							}

						delete xmt;
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
