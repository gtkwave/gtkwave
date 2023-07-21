/*
 * Copyright (c) 2001 Tony Bybell.
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

/*
 * debug.c 01feb99ajb
 * malloc debugs added on 13jul99ajb
 */
#include <config.h>
#include "v2l_debug_lxt2.h"


#ifdef DEBUG_MALLOC	/* normally this should be undefined..this is *only* for finding stray allocations/frees */
	static struct memchunk *mem=NULL;
	static size_t mem_total=0;
	static int mem_chunks=0;

	static void mem_addnode(void *ptr, size_t size)
	{
	struct memchunk *m;

	m=(struct memchunk *)malloc(sizeof(struct memchunk));
	m->ptr=ptr;
	m->size=size;
	m->next=mem;

	mem=m;
	mem_total+=size;
	mem_chunks++;

	fprintf(stderr,"mem_addnode:  TC:%05d TOT:%010d PNT:%010p LEN:+%d\n",mem_chunks,mem_total,ptr,size);
	}

	static void mem_freenode(void *ptr)
	{
	struct memchunk *m, *mprev=NULL;
	m=mem;

	while(m)
		{
		if(m->ptr==ptr)
			{
			if(mprev)
				{
				mprev->next=m->next;
				}
				else
				{
				mem=m->next;
				}

			mem_total=mem_total-m->size;
			mem_chunks--;
			fprintf(stderr,"mem_freenode: TC:%05d TOT:%010d PNT:%010p LEN:-%d\n",mem_chunks,mem_total,ptr,m->size);
			free(m);
			return;
			}
		mprev=m;
		m=m->next;
		}

	fprintf(stderr,"mem_freenode: PNT:%010p *INVALID*\n",ptr);
	sleep(1);
	}
#endif


/*
 * wrapped malloc family...
 */
void *malloc_2(size_t size)
{
void *ret;
ret=malloc(size);
if(ret)
	{
	DEBUG_M(mem_addnode(ret,size));
	}
	else
	{
	fprintf(stderr, "FATAL ERROR : Out of memory, sorry.\n");
	exit(1);
	}

return(ret);
}

void *realloc_2(void *ptr, size_t size)
{
void *ret;
ret=realloc(ptr, size);
if(ret)
	{
	DEBUG_M(mem_freenode(ptr));
	DEBUG_M(mem_addnode(ret,size));
	}
	else
	{
	fprintf(stderr, "FATAL ERROR : Out of memory, sorry.\n");
	exit(1);
	}

return(ret);
}

void *calloc_2(size_t nmemb, size_t size)
{
void *ret;
ret=calloc(nmemb, size);
if(ret)
	{
	DEBUG_M(mem_addnode(ret, nmemb*size));
	}
	else
	{
	fprintf(stderr, "FATAL ERROR: Out of memory, sorry.\n");
	exit(1);
	}

return(ret);
}

void free_2(void *ptr)
{
if(ptr)
	{
	DEBUG_M(mem_freenode(ptr));
	free(ptr);
	}
	else
	{
	fprintf(stderr, "WARNING: Attempt to free NULL pointer caught.\n");
	}
}


/*
 * atoi 64-bit version..
 * y/on     default to '1'
 * n/nonnum default to '0'
 */
TimeType atoi_64(char *str)
{
TimeType val=0;
unsigned char ch, nflag=0;

#if 0
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
#endif

while((ch=*(str++)))
	{
	if((ch>='0')&&(ch<='9'))
		{
		val=(val*10+(ch&15));
		}
	else
	if((ch=='-')&&(val==0)&&(!nflag))
		{
		nflag=1;
		}
	else
	if(val)
		{
		break;
		}
	}
return(nflag?(-val):val);
}

