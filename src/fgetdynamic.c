/*
 * Copyright (c) Tony Bybell 1999-2013.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <string.h>
#include <ctype.h>
#include "fgetdynamic.h"
#include "debug.h"


char *fgetmalloc(FILE *handle)
{
struct vlist_t *v;
char *pnt = NULL;
int i, ch;

v = vlist_create(sizeof(char));

do
	{
	for(;;)
		{
		ch=fgetc(handle);
		if((ch==EOF)||(ch==0x00)||(ch=='\n')||(ch=='\r')) break;

		pnt = (char *)vlist_alloc(&v, 0);
		*pnt = (char)ch;
		}
	} while(!pnt && ((ch=='\n')||(ch=='\r'))); /* fix for \n\r on \n systems */

GLOBALS->fgetmalloc_len = vlist_size(v);

if(!GLOBALS->fgetmalloc_len)
	{
	pnt = NULL;
	}
	else
	{
	pnt=malloc_2(GLOBALS->fgetmalloc_len+1);
	for(i=0;i<GLOBALS->fgetmalloc_len;i++)
		{
		pnt[i] = *((char *)vlist_locate(v, i));
		}
	pnt[i] = 0;
	}

vlist_destroy(v);
return(pnt);
}


/*
 * remove any leading and trailing spaces
 */
static char *stripspaces(char *s)
{
int len;

if(s)
	{
	char *s2 = s + strlen(s) - 1;
	while(isspace((int)(unsigned char)*s2) && (s2 != s)) { *s2 = 0; s2--; }

	s2 = s;
	while(*s2 && isspace((int)(unsigned char)*s2)) { s2++; }

	if((len = strlen(s2)))
		{
		char *s3 = malloc_2(len + 1);
		strcpy(s3, s2);
		free_2(s);
		s = s3;

		GLOBALS->fgetmalloc_len = len;
		}
		else
		{
		free_2(s); s = NULL;
		GLOBALS->fgetmalloc_len = 0;
		}
	}

return(s);
}


char *fgetmalloc_stripspaces(FILE *handle)
{
char *s = fgetmalloc(handle);
return(stripspaces(s));
}


/*
 * variants for tcl argument passing which really aren't fgetdynamic-ish functions...
 * the struct wave_script_args * passed in was generated in tcl_helper.c.
 */
char *wave_script_args_fgetmalloc(struct wave_script_args *w)
{
char *pnt;

if((!w)||(!w->curr)) return(NULL);

pnt = malloc_2(strlen(w->curr->payload)+1);
strcpy(pnt, w->curr->payload);

w->curr = w->curr->next;
return(pnt);
}


char *wave_script_args_fgetmalloc_stripspaces(struct wave_script_args *w)
{
char *s = wave_script_args_fgetmalloc(w);
return(stripspaces(s));
}

