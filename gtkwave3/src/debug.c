/*
 * Copyright (c) Tony Bybell 1999-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


/*
 * debug.c 01feb99ajb
 * malloc debugs added on 13jul99ajb
 * malloc tracking added on 05aug07ajb for 3.1 series
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "debug.h"
#ifdef _WAVE_HAVE_JUDY
#include <Judy.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#include "fstapi.h"
#include "lxt2_read.h"
#include "lxt.h"
#include "vzt_read.h"
#endif

#undef free_2
#undef malloc_2
#undef realloc_2
#undef calloc_2

#ifdef _WAVE_HAVE_JUDY
void free_outstanding(void)
{
Pvoid_t  PJArray = (Pvoid_t)GLOBALS->alloc2_chain;
int rcValue;
Word_t Index;
#ifdef DEBUG_PRINTF
int ctr = 0;

printf("\n*** cleanup ***\n");
printf("Freeing %d chunks\n", GLOBALS->outstanding);
system("date");
#endif

if(GLOBALS->s_selected)
	{
	destroy_s_selected();
	}

Index = 0;
for (rcValue = Judy1First(PJArray, &Index, PJE0); rcValue != 0; rcValue = Judy1Next(PJArray, &Index, PJE0))
	{
	free((void *)Index);
#ifdef DEBUG_PRINTF
	ctr++;
#endif
	}
Judy1FreeArray(&PJArray, PJE0);

GLOBALS->alloc2_chain = NULL;
GLOBALS->outstanding = 0;

#ifdef DEBUG_PRINTF
printf("Freed %d chunks\n", ctr);
system("date");
#endif
}
#else
void free_outstanding(void)
{
void **t = (void **)GLOBALS->alloc2_chain;
void **t2;
int ctr = 0;

#ifdef DEBUG_PRINTF
printf("\n*** cleanup ***\n");
printf("Freeing %d chunks\n", GLOBALS->outstanding);
system("date");
#endif

while(t)
	{
	t2 = (void **) *(t+1);
	free(t);
	t = t2;
	ctr++;
	}

GLOBALS->alloc2_chain = NULL;
GLOBALS->outstanding = 0;

#ifdef DEBUG_PRINTF
printf("Freed %d chunks\n", ctr);
system("date");
#endif
}
#endif

/*
 * wrapped malloc family...
 */
#ifdef _WAVE_HAVE_JUDY
void *malloc_2(size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
void *ret;

ret=malloc(size);
if(ret)
        {
	Judy1Set ((Pvoid_t)&GLOBALS->alloc2_chain, (Word_t)ret, PJE0);

        GLOBALS->outstanding++;

        return(ret);
        }
        else
        {
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "FATAL ERROR: malloc_2() Out of memory, sorry. ['%s', %d]\n", filename, lineno);
#else
        fprintf(stderr, "FATAL ERROR: malloc_2() Out of memory, sorry.\n");
#endif
        exit(1);
        }
}
#else
void *malloc_2(size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
void *ret;

ret=malloc(size + 2*sizeof(void *));
if(ret)
	{
	void **ret2 = (void **)ret;
	*(ret2+0) = NULL;
	*(ret2+1) = GLOBALS->alloc2_chain;
	if(GLOBALS->alloc2_chain)
		{
		*(GLOBALS->alloc2_chain+0) = ret2;
		}
	GLOBALS->alloc2_chain = ret2;

	GLOBALS->outstanding++;

	return((char *)ret + 2*sizeof(void *));
	}
	else
	{
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "FATAL ERROR: malloc_2() Out of memory, sorry. ['%s', %d]\n", filename, lineno);
#else
	fprintf(stderr, "FATAL ERROR: malloc_2() Out of memory, sorry.\n");
#endif
	exit(1);
	}
}
#endif

#ifdef _WAVE_HAVE_JUDY
void *realloc_2(void *ptr, size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
void *ret=realloc(ptr, size);

if(ret)
        {
	if(ptr != ret)
		{
		Judy1Unset ((Pvoid_t)&GLOBALS->alloc2_chain, (Word_t)ptr, PJE0);
		Judy1Set ((Pvoid_t)&GLOBALS->alloc2_chain, (Word_t)ret, PJE0);
		}

        return(ret);
        }
        else
        {
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "FATAL ERROR: realloc_2() Out of memory, sorry. ['%s', %d]\n", filename, lineno);
#else
        fprintf(stderr, "FATAL ERROR: realloc_2() Out of memory, sorry.\n");
#endif
        exit(1);
        }
}
#else
void *realloc_2(void *ptr, size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
void *ret;

void **ret2 = ((void **)ptr) - 2;
void **prv = (void **)*(ret2+0);
void **nxt = (void **)*(ret2+1);

if(prv)
	{
        *(prv+1) = nxt;
        }
        else
        {
        GLOBALS->alloc2_chain = nxt;
        }

if(nxt)
	{
        *(nxt+0) = prv;
        }

ret=realloc((char *)ptr - 2*sizeof(void *), size + 2*sizeof(void *));

ret2 = (void **)ret;
*(ret2+0) = NULL;
*(ret2+1) = GLOBALS->alloc2_chain;
if(GLOBALS->alloc2_chain)
	{
	*(GLOBALS->alloc2_chain+0) = ret2;
	}
GLOBALS->alloc2_chain = ret2;

if(ret)
	{
	return((char *)ret + 2*sizeof(void *));
	}
	else
	{
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "FATAL ERROR: realloc_2() Out of memory, sorry. ['%s', %d]\n", filename, lineno);
#else
	fprintf(stderr, "FATAL ERROR: realloc_2() Out of memory, sorry.\n");
#endif
	exit(1);
	}
}
#endif

#ifdef _WAVE_HAVE_JUDY
void *calloc_2_into_context(struct Global *g, size_t nmemb, size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
void *ret;

ret=calloc(nmemb, size);
if(ret)
	{
	Judy1Set ((Pvoid_t)&g->alloc2_chain, (Word_t)ret, PJE0);

	g->outstanding++;

	return(ret);
	}
	else
	{
#ifdef DEBUG_MALLOC_LINES
	fprintf(stderr, "FATAL ERROR: calloc_2() Out of memory, sorry. ['%s', %d]\n", filename, lineno);
#else
	fprintf(stderr, "FATAL ERROR: calloc_2() Out of memory, sorry.\n");
#endif
	exit(1);
	}
}
#else
void *calloc_2_into_context(struct Global *g, size_t nmemb, size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
void *ret;

ret=calloc(1, (nmemb * size) + 2*sizeof(void *));
if(ret)
	{
	void **ret2 = (void **)ret;
	*(ret2+0) = NULL;
	*(ret2+1) = g->alloc2_chain;
	if(g->alloc2_chain)
		{
		*(g->alloc2_chain+0) = ret2;
		}
	g->alloc2_chain = ret2;

	g->outstanding++;

	return((char *)ret + 2*sizeof(void *));
	}
	else
	{
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "FATAL ERROR: calloc_2() Out of memory, sorry. ['%s', %d]\n", filename, lineno);
#else
	fprintf(stderr, "FATAL ERROR: calloc_2() Out of memory, sorry.\n");
#endif
	exit(1);
	}
}
#endif

void *calloc_2(size_t nmemb, size_t size
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
return(calloc_2_into_context(GLOBALS, nmemb, size
#ifdef DEBUG_MALLOC_LINES
, filename, lineno
#endif
));
}


#ifdef _WAVE_HAVE_JUDY
void free_2(void *ptr
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
if(ptr)
	{
	int delstat = Judy1Unset ((Pvoid_t)&GLOBALS->alloc2_chain, (Word_t)ptr, PJE0);

	if(delstat)
		{
		GLOBALS->outstanding--;
		free(ptr);
		}
		else
		{
#ifdef DEBUG_MALLOC_LINES
		printf("JUDYMEM | free to non-malloc'd address %p blocked ['%s', %d]\n", ptr, filename, lineno);
#else
		printf("JUDYMEM | free to non-malloc'd address %p blocked\n", ptr);
#endif
		}
	}
	else
	{
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "WARNING: Attempt to free NULL pointer caught. ['%s', %d]\n", filename, lineno);
#else
	fprintf(stderr, "WARNING: Attempt to free NULL pointer caught.\n");
#endif
	}
}
#else
void free_2(void *ptr
#ifdef DEBUG_MALLOC_LINES
, char *filename, int lineno
#endif
)
{
if(ptr)
	{
	void **ret2 = ((void **)ptr) - 2;
	void **prv = (void **)*(ret2+0);
	void **nxt = (void **)*(ret2+1);

	if(prv)
		{
		*(prv+1) = nxt;
		}
		else
		{
		GLOBALS->alloc2_chain = nxt;
		}

	if(nxt)
		{
		*(nxt+0) = prv;
		}

	GLOBALS->outstanding--;

	free((char *)ptr - 2*sizeof(void *));
	}
	else
	{
#ifdef DEBUG_MALLOC_LINES
        fprintf(stderr, "WARNING: Attempt to free NULL pointer caught. ['%s', %d]\n", filename, lineno);
#else
	fprintf(stderr, "WARNING: Attempt to free NULL pointer caught.\n");
#endif
	}
}
#endif


#ifdef DEBUG_MALLOC_LINES
#define malloc_2(x) malloc_2((x),__FILE__,__LINE__)
#endif


char *strdup_2(const char *s)
{
char *s2 = NULL;

if(s)
	{
	int nbytes = strlen(s) + 1;
	s2 = malloc_2(nbytes);
	memcpy(s2, s, nbytes);
	}

return(s2);
}

char *strdup_2s(const char *s)
{
char *s2 = NULL;

if(s)
	{
        int len = strlen(s);
        s2 = malloc(len+2);
        memcpy(s2, s, len);
        s2[len++] = ' ';
        s2[len] = 0;
	}

return(s2);
}


/*
 * atoi 64-bit version..
 * y/on     default to '1'
 * n/nonnum default to '0'
 */
TimeType atoi_64(const char *str)
{
TimeType val=0;
unsigned char ch, nflag=0;
int consumed = 0;

GLOBALS->atoi_cont_ptr=NULL;

switch(*str)
	{
	case 'y':
	case 'Y':
		return(LLDescriptor(1));

	case 'o':
	case 'O':
		str++;
		ch=*str;
		if((ch=='n')||(ch=='N'))
			return(LLDescriptor(1));
		else	return(LLDescriptor(0));

	case 'n':
	case 'N':
		return(LLDescriptor(0));
		break;

	default:
		break;
	}

while((ch=*(str++)))
	{
	if((ch>='0')&&(ch<='9'))
		{
		val=(val*10+(ch&15));
		consumed = 1;
		}
	else
	if((ch=='-')&&(val==0)&&(!nflag))
		{
		nflag=1;
		consumed = 1;
		}
	else
	if(consumed)
		{
		GLOBALS->atoi_cont_ptr=str-1;
		break;
		}
	}
return(nflag?(-val):val);
}


/*
 * wrapped tooltips
 */
void gtk_tooltips_set_tip_2(GtkTooltips *tooltips, GtkWidget *widget,
	const gchar *tip_text, const gchar *tip_private)
{
if(!GLOBALS->disable_tooltips)
	{
	gtk_tooltips_set_tip(tooltips, widget, tip_text, tip_private);
	}
}


void gtk_tooltips_set_delay_2(GtkTooltips *tooltips, guint delay)
{
if(!GLOBALS->disable_tooltips)
	{
	gtk_tooltips_set_delay(tooltips, delay);
	}
}


GtkTooltips* gtk_tooltips_new_2(void)
{
if(!GLOBALS->disable_tooltips)
	{
	return(gtk_tooltips_new());
	}
	else
	{
	return(NULL);
	}
}


char *tmpnam_2(char *s, int *fd)
{
(void)s;

#if defined _MSC_VER || defined __MINGW32__
char *fname = NULL;
TCHAR szTempFileName[MAX_PATH];
TCHAR lpTempPathBuffer[MAX_PATH];
DWORD dwRetVal = 0;
UINT uRetVal = 0;

*fd = -1;

dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
if((dwRetVal > MAX_PATH) || (dwRetVal == 0))
    	{
	fprintf(stderr, "GetTempPath() failed\n");
    	}
	else
	{
	uRetVal = GetTempFileName(lpTempPathBuffer, TEXT("GTKW"), 0, szTempFileName);
    	if (uRetVal == 0)
    		{
        	fprintf(stderr, "GetTempFileName() failed\n");
		}
		else
		{
		fname = strdup_2(szTempFileName);
		}
	}

return(fname);

#else

char *backpath = "gtkwaveXXXXXX";
char *tmpspace;
int len = strlen(P_tmpdir);
int i;

unsigned char slash = '/';
for(i=0;i<len;i++)
	{
	if((P_tmpdir[i] == '\\') || (P_tmpdir[i] == '/'))
		{
		slash = P_tmpdir[i];
		break;
		}
	}

tmpspace = malloc_2(len + 1 + strlen(backpath) + 1);
sprintf(tmpspace, "%s%c%s", P_tmpdir, slash, backpath);
*fd = mkstemp(tmpspace);
if(*fd<0)
	{
	fprintf(stderr, "tmpnam_2() could not create tempfile, exiting.\n");
	perror("Why");
	exit(255);
	}

return(tmpspace);

#endif
}


void wave_gtk_window_set_title(GtkWindow *window, const gchar *title, int typ, int pct)
{
if(window && title)
	{
	switch(typ)
		{
		case WAVE_SET_TITLE_MODIFIED:
			{
			const char *pfx = "[Modified] ";
			char *t = wave_alloca(strlen(pfx) + strlen(title) + 1);

			strcpy(t, pfx);
			strcat(t, title);
			gtk_window_set_title(window, t);
			}
			break;

		case WAVE_SET_TITLE_LOADING:
			{
			char *t = wave_alloca(64 + strlen(title) + 1); /* make extra long */

			sprintf(t, "[Loading %d%%] %s", pct, title);
			gtk_window_set_title(window, t);
			}
			break;

		case WAVE_SET_TITLE_NONE:
		default:
			gtk_window_set_title(window, title);
			break;
		}
	}
}



char *realpath_2(const char *path, char *resolved_path)
{
#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED || defined __CYGWIN__ || defined HAVE_REALPATH

#if (defined(__MACH__) && defined(__APPLE__))
if(!resolved_path)
	{
	resolved_path = malloc(PATH_MAX+1); /* fixes bug on Leopard when resolved_path == NULL */
	}
#endif

return(realpath(path, resolved_path));

#else
#ifdef __MINGW32__
if(!resolved_path)
	{
	resolved_path = malloc(PATH_MAX+1);
	}
return(_fullpath(resolved_path, path, PATH_MAX));
#else
return(NULL);
#endif
#endif
}


/******************************************************/

#ifdef HAVE_SYS_STAT_H

int determine_gtkwave_filetype(const char *path)
{
struct stat buf;
int rc = G_FT_UNKNOWN;

#ifdef EXTLOAD_SUFFIX
if(suffix_check(GLOBALS->loaded_file_name, "."EXTLOAD_SUFFIX))
	{
	return(rc);
	}
#endif

#ifdef AET2_IS_PRESENT
if(suffix_check(GLOBALS->loaded_file_name, ".aet") || suffix_check(GLOBALS->loaded_file_name, ".ae2"))
	{
	return(rc);
	}
#endif

memset(&buf, 0, sizeof(struct stat));
if(stat(path, &buf) == 0)
	{
	if(S_ISREG(buf.st_mode))
		{
		FILE *f = fopen(path, "rb");
		if(f)
			{
			int hdr[2] = { 0, 0 };
			unsigned int magic_word;

			hdr[0] = fgetc(f);
			hdr[1] = fgetc(f);

			if((hdr[0] != EOF) && (hdr[1] != EOF))
				{
				magic_word = (hdr[0] * 256) + hdr[1];

				switch(magic_word)
					{
					case LT_HDRID:		rc = G_FT_LXT; break;
					case LXT2_RD_HDRID:	rc = G_FT_LXT2; break;
					case VZT_RD_HDRID:	rc = G_FT_VZT; break;
					default: 		break;
					}

				if(rc == G_FT_UNKNOWN)
					{
					if(hdr[0] == FST_BL_ZWRAPPER)
						{
						rc = G_FT_FST;
						}
					else if(hdr[0] == FST_BL_HDR)
						{
						unsigned char e_ch[8];
						int i, c;
						double fst_real_test = (2.7182818284590452354);
						int nfa, nfb;

						for(i=0;i<23;i++)
							{
							if(fgetc(f) == EOF) goto chk_ex;
							}


						for(i=0;i<8;i++)
							{
							e_ch[i] = c = fgetc(f);
							if(c == EOF) goto chk_ex;
							}

						nfa = nfb = 0;
						for(i=0;i<8;i++)
							{
							if(e_ch[i] == ((unsigned char *)&fst_real_test)[i])
								{
								nfa++;
								}
							if(e_ch[7-i] == ((unsigned char *)&fst_real_test)[i])
								{
								nfb++;
								}
							}

						if((nfa == 8) || (nfb == 8))
							{
							rc = G_FT_FST;
							}
						}
					}

				}

chk_ex:			fclose(f);
			}
		}
	}

errno = 0;
return(rc);
}

#else

int determine_gtkwave_filetype(const char *path)
{
return(G_FT_UNKNOWN);
}

#endif

/******************************************************/
