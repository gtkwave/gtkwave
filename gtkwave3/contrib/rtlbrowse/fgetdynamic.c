/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifdef _AIX
#pragma alloca
#endif

#include <config.h>
#include "fgetdynamic.h"
#include <stdlib.h>

#if HAVE_ALLOCA_H
#include <alloca.h>
#elif defined(__GNUC__)
#ifndef alloca
#define alloca __builtin_alloca
#endif
#elif defined(_MSC_VER)
#include <malloc.h>
#define alloca _alloca
#endif

int fgetmalloc_len;

char *fgetmalloc(FILE *handle)
{
char *pnt, *pnt2;
struct alloc_bytechain *bytechain_root=NULL, *bytechain_current=NULL;
int ch;

fgetmalloc_len=0;

for(;;)
	{
	ch=fgetc(handle);
	if((ch==EOF)||(ch==0x00)||(ch=='\n')||(ch=='\r')) break;
	fgetmalloc_len++;
	if(bytechain_current)
		{
		bytechain_current->next=alloca(sizeof(struct alloc_bytechain));
		bytechain_current=bytechain_current->next;
		bytechain_current->val=(char)ch;
		bytechain_current->next=NULL;
		}
		else
		{
		bytechain_root=bytechain_current=alloca(sizeof(struct alloc_bytechain));
		bytechain_current->val=(char)ch;
		bytechain_current->next=NULL;
		}
	}

if(!fgetmalloc_len)
	{
	return(NULL);
	}
	else
	{
	pnt=pnt2=(char *)malloc(fgetmalloc_len+1);
	while(bytechain_root)
		{
		*(pnt2++)=bytechain_root->val;
		bytechain_root=bytechain_root->next;
		}
	*(pnt2)=0;

	return(pnt);
	}


}

