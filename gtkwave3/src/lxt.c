/*
 * Copyright (c) Tony Bybell 2001-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <stdio.h>
#include "zlib.h"
#include "bzlib.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#include <io.h>
#endif

#include"globals.h"
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "symbol.h"
#include "vcd.h"
#include "lxt.h"
#include "bsearch.h"
#include "debug.h"
#include "hierpack.h"

/****************************************************************************/

/*
 * functions which emit various big endian
 * data to a file
 */

static int lt_emit_u8(FILE *handle, int value)
{
unsigned char buf[1];
int nmemb;

buf[0] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 1, handle);
GLOBALS->fpos_lxt_c_1+=nmemb;
return(nmemb);
}


static int lt_emit_u16(FILE *handle, int value)
{
unsigned char buf[2];
int nmemb;

buf[0] = (value>>8) & 0xff;
buf[1] = value & 0xff;
nmemb = fwrite(buf, sizeof(char), 2, handle);
GLOBALS->fpos_lxt_c_1+=nmemb;
return(nmemb);
}


static int lt_emit_u24(FILE *handle, int value)
{
unsigned char buf[3];
int nmemb;

buf[0] = (value>>16) & 0xff;
buf[1] = (value>>8) & 0xff;
buf[2] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 3, handle);
GLOBALS->fpos_lxt_c_1+=nmemb;
return(nmemb);
}


static int lt_emit_u32(FILE *handle, int value)
{
unsigned char buf[4];
int nmemb;

buf[0] = (value>>24) & 0xff;
buf[1] = (value>>16) & 0xff;
buf[2] = (value>>8) & 0xff;
buf[3] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 4, handle);
GLOBALS->fpos_lxt_c_1+=nmemb;
return(nmemb);
}

/****************************************************************************/

#define LXTHDR "LXTLOAD | "



				/* its mmap() variant doesn't use file descriptors    */
#if defined __MINGW32__ || defined _MSC_VER
#define PROT_READ (0)
#define MAP_SHARED (0)

HANDLE hIn, hInMap;

char *win_fname = NULL;

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
close(fd);
hIn = CreateFile(win_fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
if(hIn == INVALID_HANDLE_VALUE)
{
	fprintf(stderr, "Could not open '%s' with CreateFile(), exiting.\n", win_fname);
	exit(255);
}

hInMap = CreateFileMapping(hIn, NULL, PAGE_READONLY, 0, 0, NULL);
return(MapViewOfFile(hInMap, FILE_MAP_READ, 0, 0, 0));
}

int munmap(void *start, size_t length)
{
UnmapViewOfFile(start);
CloseHandle(hInMap); hInMap = NULL;
CloseHandle(hIn); hIn = NULL;
return(0);
}


#endif

/****************************************************************************/

#ifdef _WAVE_BE32

/*
 * reconstruct 8/16/24/32 bits out of the lxt's representation
 * of a big-endian integer.  this is for 32-bit PPC so no byte
 * swizzling needs to be done at all.  for 24-bit ints, we have no danger of
 * running off the end of memory provided we do the "-1" trick
 * since we'll never read a 24-bit int at the very start of a file which
 * means that we'll have a 32-bit word that we can read.
 */

inline static unsigned int get_byte(offset) {
  return ((unsigned int)(*((unsigned char *)(GLOBALS->mm_lxt_c_1)+(offset))));
}

inline static unsigned int get_16(offset) {
  return ((unsigned int)(*((unsigned short *)(((unsigned char *)(GLOBALS->mm_lxt_c_1))
                +(offset)))));
}

inline static unsigned int get_32(offset) {
  return (*(unsigned int *)(((unsigned char *)(GLOBALS->mm_lxt_c_1))+(offset)));
}

inline static unsigned int get_24(offset) {
  return ((get_32((offset)-1)<<8)>>8);
}

inline static unsigned int get_64(offset) {
  return ((((UTimeType)get_32(offset))<<32)|((UTimeType)get_32((offset)+4)));
}

#else

/*
 * reconstruct 8/16/24/32 bits out of the lxt's representation
 * of a big-endian integer.  this should work on all architectures.
 */
#define get_byte(offset) 	((unsigned int)(*((unsigned char *)GLOBALS->mm_lxt_c_1+offset)))

static unsigned int get_16(off_t offset)
{
unsigned char *nn=(unsigned char *)GLOBALS->mm_lxt_c_1+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)nn);
return((m1<<8)|m2);
}

static unsigned int get_24(off_t offset)
{
unsigned char *nn=(unsigned char *)GLOBALS->mm_lxt_c_1+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)(nn++));
unsigned int m3=*((unsigned char *)nn);
return((m1<<16)|(m2<<8)|m3);
}

static unsigned int get_32(off_t offset)
{
unsigned char *nn=(unsigned char *)GLOBALS->mm_lxt_c_1+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)(nn++));
unsigned int m3=*((unsigned char *)(nn++));
unsigned int m4=*((unsigned char *)nn);
return((m1<<24)|(m2<<16)|(m3<<8)|m4);
}

static UTimeType get_64(off_t offset)
{
return(
(((UTimeType)get_32(offset))<<32)
|((UTimeType)get_32(offset+4))
);
}

#endif

/****************************************************************************/


static void create_double_endian_mask(int offset)
{
double d;
int i, j;
static double p = 3.14159;

d= *((double *)((unsigned char *)GLOBALS->mm_lxt_c_1+offset));
if(p==d)
	{
	GLOBALS->double_is_native_lxt_c_1=1;
	}
	else
	{
	char *remote, *here;

	remote = (char *)&d;
	here = (char *)&p;
	for(i=0;i<8;i++)
		{
		for(j=0;j<8;j++)
			{
			if(here[i]==remote[j])
				{
				GLOBALS->double_mask_lxt_c_1[i]=j;
				break;
				}
			}
		}
	}
}

static char *swab_double_via_mask(int offset)
{
char swapbuf[8];
char *pnt = malloc_2(8*sizeof(char));
int i;

memcpy(swapbuf, ((unsigned char *)GLOBALS->mm_lxt_c_1+offset), 8);
for(i=0;i<8;i++)
	{
	pnt[i]=swapbuf[GLOBALS->double_mask_lxt_c_1[i]];
	}

return(pnt);
}

/****************************************************************************/

/*
 * convert 0123 to an mvl character representation
 */
static unsigned char convert_mvl(int value)
{
return("01zxhuwl-xxxxxxx"[value&15]);
}


/*
 * given an offset into the aet, calculate the "time" of that
 * offset in simulation.  this func similar to how gtkwave can
 * find the most recent valuechange that corresponds with
 * an arbitrary timevalue.
 */
static int compar_mvl_timechain(const void *s1, const void *s2)
{
int key, obj, delta;
int rv;

key=*((int *)s1);
obj=*((int *)s2);

if((obj<=key)&&(obj>GLOBALS->max_compare_time_tc_lxt_c_2))
        {
        GLOBALS->max_compare_time_tc_lxt_c_2=obj;
        GLOBALS->max_compare_pos_tc_lxt_c_2=(int *)s2 - GLOBALS->positional_information_lxt_c_1;
        }

delta=key-obj;
if(delta<0) rv=-1;
else if(delta>0) rv=1;
else rv=0;

return(rv);
}

static TimeType bsearch_mvl_timechain(int key)
{
GLOBALS->max_compare_time_tc_lxt_c_2=-1; GLOBALS->max_compare_pos_tc_lxt_c_2=-1;

if(bsearch((void *)&key, (void *)GLOBALS->positional_information_lxt_c_1, GLOBALS->total_cycles_lxt_c_2, sizeof(int), compar_mvl_timechain))
	{
	/* nothing, all side effects are in bsearch */
	}

if((GLOBALS->max_compare_pos_tc_lxt_c_2<=0)||(GLOBALS->max_compare_time_tc_lxt_c_2<0))
        {
        GLOBALS->max_compare_pos_tc_lxt_c_2=0; /* aix bsearch fix */
        }

return(GLOBALS->time_information[GLOBALS->max_compare_pos_tc_lxt_c_2]);
}


/*
 * build up decompression dictionary for MVL2 facs >16 bits ...
 */
static void build_dict(void)
{
gzFile zhandle;
off_t offs = GLOBALS->zdictionary_offset_lxt_c_1+24;
int total_mem, rc;
unsigned int i;
char *decmem=NULL;
char *pnt;
#if defined __MINGW32__ || defined _MSC_VER
FILE *tmp;
#endif

GLOBALS->dict_num_entries_lxt_c_1 = get_32(GLOBALS->zdictionary_offset_lxt_c_1+0);
GLOBALS->dict_string_mem_required_lxt_c_1 = get_32(GLOBALS->zdictionary_offset_lxt_c_1+4);
GLOBALS->dict_16_offset_lxt_c_1 = get_32(GLOBALS->zdictionary_offset_lxt_c_1+8);
GLOBALS->dict_24_offset_lxt_c_1 = get_32(GLOBALS->zdictionary_offset_lxt_c_1+12);
GLOBALS->dict_32_offset_lxt_c_1 = get_32(GLOBALS->zdictionary_offset_lxt_c_1+16);
GLOBALS->dict_width_lxt_c_1 = get_32(GLOBALS->zdictionary_offset_lxt_c_1+20);

DEBUG(fprintf(stderr, LXTHDR"zdictionary_offset = %08x\n", GLOBALS->zdictionary_offset_lxt_c_1));
DEBUG(fprintf(stderr, LXTHDR"zdictionary_predec_size = %08x\n\n", GLOBALS->zdictionary_predec_size_lxt_c_1));
DEBUG(fprintf(stderr, LXTHDR"dict_num_entries = %d\n", GLOBALS->dict_num_entries_lxt_c_1));
DEBUG(fprintf(stderr, LXTHDR"dict_string_mem_required = %d\n", GLOBALS->dict_string_mem_required_lxt_c_1));
DEBUG(fprintf(stderr, LXTHDR"dict_16_offset = %d\n", GLOBALS->dict_16_offset_lxt_c_1));
DEBUG(fprintf(stderr, LXTHDR"dict_24_offset = %d\n", GLOBALS->dict_24_offset_lxt_c_1));
DEBUG(fprintf(stderr, LXTHDR"dict_32_offset = %d\n", GLOBALS->dict_32_offset_lxt_c_1));

fprintf(stderr, LXTHDR"Dictionary compressed MVL2 change records detected...\n");

#if defined __MINGW32__ || defined _MSC_VER
{
unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+offs;
tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
fwrite(t, GLOBALS->zdictionary_predec_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
zhandle = gzdopen(dup(fileno(tmp)), "rb");
}
#else
if(offs!=lseek(GLOBALS->fd_lxt_c_1, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"dict lseek error at offset %08x\n", (unsigned int)offs); exit(255); }
zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif
decmem = malloc_2(total_mem = GLOBALS->dict_string_mem_required_lxt_c_1);

rc=gzread(zhandle, decmem, total_mem);
DEBUG(printf(LXTHDR"section offs for name decompression = %08x of len %d\n", offs, GLOBALS->dict_num_entries_lxt_c_1));
DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));
if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

GLOBALS->dict_string_mem_array_lxt_c_1 = (char **)calloc_2(GLOBALS->dict_num_entries_lxt_c_1, sizeof(char *));
pnt = decmem;
for(i=0;i<GLOBALS->dict_num_entries_lxt_c_1;i++)
	{
	GLOBALS->dict_string_mem_array_lxt_c_1[i]=pnt;
	pnt+=(strlen(pnt)+1);
	DEBUG(printf(LXTHDR"Dict %d: '1%s'\n", i, GLOBALS->dict_string_mem_array_lxt_c_1[i]));
	}

gzclose(zhandle);
#if defined __MINGW32__ || defined _MSC_VER
fclose(tmp);
#endif

fprintf(stderr, LXTHDR"...expanded %d entries from %08x into %08x bytes.\n", GLOBALS->dict_num_entries_lxt_c_1, GLOBALS->zdictionary_predec_size_lxt_c_1, GLOBALS->dict_string_mem_required_lxt_c_1);
}


/*
 * decompress facility names and build up fac geometry
 */
static void build_facs(char *fname, char **f_name, struct Node *node_block)
{
char *buf, *bufprev = NULL, *bufcurr;
off_t offs=GLOBALS->facname_offset_lxt_c_1+8;
int i, j, clone;
char *pnt = NULL;
int total_mem = get_32(GLOBALS->facname_offset_lxt_c_1+4);
gzFile zhandle = NULL;
char *decmem=NULL;

#if defined __MINGW32__ || defined _MSC_VER
FILE *tmp;
#endif

buf=malloc_2(total_mem);
pnt=bufprev=buf;

if(GLOBALS->zfacname_size_lxt_c_1)
	{
	int rc;
#if defined __MINGW32__ || defined _MSC_VER
	unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+offs;
	tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
	fwrite(t, GLOBALS->zfacname_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
	zhandle = gzdopen(dup(fileno(tmp)), "rb");
#else
	if(offs!=lseek(GLOBALS->fd_lxt_c_1, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"zfacname lseek error at offset %08x\n", (unsigned int)offs); exit(255); }
	zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif

	GLOBALS->mmcache_lxt_c_1 = GLOBALS->mm_lxt_c_1;
	decmem = malloc_2(total_mem = GLOBALS->zfacname_predec_size_lxt_c_1); GLOBALS->mm_lxt_c_1 = decmem;

	rc=gzread(zhandle, decmem, total_mem);
	DEBUG(printf(LXTHDR"section offs for name decompression = %08x of len %d\n", offs, GLOBALS->zfacname_size_lxt_c_1));
	DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));
	if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

	offs=0;	/* we're in our new memory region now.. */
	}

fprintf(stderr, LXTHDR"Building %d facilities.\n", GLOBALS->numfacs);
for(i=0;i<GLOBALS->numfacs;i++)
	{
        clone=get_16(offs);  offs+=2;
	bufcurr=pnt;
	for(j=0;j<clone;j++)
		{
		*(pnt++) = *(bufprev++);
		}
        while((*(pnt++)=get_byte(offs++)));
        f_name[i]=bufcurr;
	DEBUG(printf(LXTHDR"Encountered facility %d: '%s'\n", i, bufcurr));
	bufprev=bufcurr;
        }

if(GLOBALS->zfacname_size_lxt_c_1)
	{
	GLOBALS->mm_lxt_c_1 = GLOBALS->mmcache_lxt_c_1;
	free_2(decmem); decmem = NULL;
	gzclose(zhandle);
#if defined __MINGW32__ ||  defined _MSC_VER
	fclose(tmp);
#endif
	}

if(!GLOBALS->facgeometry_offset_lxt_c_1)
	{
	fprintf(stderr, "LXT '%s' is missing a facility geometry section, exiting.\n", fname);
	exit(255);
	}

offs=GLOBALS->facgeometry_offset_lxt_c_1;

if(GLOBALS->zfacgeometry_size_lxt_c_1)
	{
	int rc;

#if defined __MINGW32__ || defined _MSC_VER
	unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+offs;
	tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
	fwrite(t, GLOBALS->zfacgeometry_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
	zhandle = gzdopen(dup(fileno(tmp)), "rb");
#else
	if(offs!=lseek(GLOBALS->fd_lxt_c_1, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"zfacgeometry lseek error at offset %08x\n", (unsigned int)offs); exit(255); }
	zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif
	GLOBALS->mmcache_lxt_c_1 = GLOBALS->mm_lxt_c_1;
	total_mem = GLOBALS->numfacs * 16;
	decmem = malloc_2(total_mem); GLOBALS->mm_lxt_c_1 = decmem;

	rc=gzread(zhandle, decmem, total_mem);
	DEBUG(printf(LXTHDR"section offs for facgeometry decompression = %08x of len %d\n", offs, GLOBALS->zfacgeometry_size_lxt_c_1));
	DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));
	if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

	offs=0;	/* we're in our new memory region now.. */
	}

for(i=0;i<GLOBALS->numfacs;i++)
	{
	GLOBALS->mvlfacs_lxt_c_2[i].node_alias=get_32(offs);
	node_block[i].msi=get_32(offs+4);
	node_block[i].lsi=get_32(offs+8);
	GLOBALS->mvlfacs_lxt_c_2[i].flags=get_32(offs+12);
	GLOBALS->mvlfacs_lxt_c_2[i].len=(node_block[i].lsi>node_block[i].msi)?(node_block[i].lsi-node_block[i].msi+1):(node_block[i].msi-node_block[i].lsi+1);

	if(GLOBALS->mvlfacs_lxt_c_2[i].len>GLOBALS->lt_len_lxt_c_1) GLOBALS->lt_len_lxt_c_1 = GLOBALS->mvlfacs_lxt_c_2[i].len;
	DEBUG(printf(LXTHDR"%s[%d:%d]\n", f_name[i], node_block[i].msi, node_block[i].lsi));

	offs+=0x10;
	}

GLOBALS->lt_buf_lxt_c_1 = malloc_2(GLOBALS->lt_len_lxt_c_1>255 ? GLOBALS->lt_len_lxt_c_1 : 256);	/* in order to keep trivial LXT files from overflowing their buffer */

if(GLOBALS->zfacgeometry_size_lxt_c_1)
	{
	GLOBALS->mm_lxt_c_1 = GLOBALS->mmcache_lxt_c_1;
	free_2(decmem); decmem = NULL;
	gzclose(zhandle);
#if defined __MINGW32__ || defined _MSC_VER
	fclose(tmp);
#endif
	}
}


/*
 * build up the lastchange entries so we can start to walk
 * through the aet..
 */
static void build_facs2(char *fname)
{
int i;
off_t offs;
int chg;
int maxchg=0, maxindx=0;
int last_position;
TimeType last_time;
char *decmem = NULL;
int total_mem;
gzFile zhandle = NULL;

#if defined __MINGW32__ || defined _MSC_VER
FILE *tmp;
#endif

if((!GLOBALS->time_table_offset_lxt_c_1)&&(!GLOBALS->time_table_offset64_lxt_c_1))
	{
	fprintf(stderr, "LXT '%s' is missing a time table section, exiting.\n", fname);
	exit(255);
	}

if((GLOBALS->time_table_offset_lxt_c_1)&&(GLOBALS->time_table_offset64_lxt_c_1))
	{
	fprintf(stderr, "LXT '%s' has both 32 and 64-bit time table sections, exiting.\n", fname);
	exit(255);
	}

if(GLOBALS->time_table_offset_lxt_c_1)
	{
	offs = GLOBALS->time_table_offset_lxt_c_1;

	DEBUG(printf(LXTHDR"Time table position: %08x\n", GLOBALS->time_table_offset_lxt_c_1 + 12));
	GLOBALS->total_cycles_lxt_c_2=get_32(offs+0);
	DEBUG(printf(LXTHDR"Total cycles: %d\n", GLOBALS->total_cycles_lxt_c_2));

	if(GLOBALS->ztime_table_size_lxt_c_1)
		{
		int rc;

#if defined __MINGW32__ || defined _MSC_VER
		unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+offs+4;
		tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
		fwrite(t, GLOBALS->ztime_table_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
		zhandle = gzdopen(dup(fileno(tmp)), "rb");
#else
		if((offs+4)!=lseek(GLOBALS->fd_lxt_c_1, offs+4, SEEK_SET)) { fprintf(stderr, LXTHDR"ztime_table lseek error at offset %08x\n", (unsigned int)offs); exit(255); }
		zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif
		GLOBALS->mmcache_lxt_c_1 = GLOBALS->mm_lxt_c_1;
		total_mem = 4 + 4 + (GLOBALS->total_cycles_lxt_c_2 * 4) + (GLOBALS->total_cycles_lxt_c_2 * 4);
		decmem = malloc_2(total_mem); GLOBALS->mm_lxt_c_1 = decmem;

		rc=gzread(zhandle, decmem, total_mem);
		DEBUG(printf(LXTHDR"section offs for timetable decompression = %08x of len %d\n", offs, GLOBALS->ztime_table_size_lxt_c_1));
		DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));
		if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

		offs=0;	/* we're in our new memory region now.. */
		}
		else
		{
		offs+=4; /* skip past count to make consistent view between compressed/uncompressed */
		}

	GLOBALS->first_cycle_lxt_c_2=get_32(offs);
	DEBUG(printf(LXTHDR"First cycle: %d\n", GLOBALS->first_cycle_lxt_c_2));
	GLOBALS->last_cycle_lxt_c_2=get_32(offs+4);
	DEBUG(printf(LXTHDR"Last cycle: %d\n", GLOBALS->last_cycle_lxt_c_2));
	DEBUG(printf(LXTHDR"Total cycles (actual): %d\n", GLOBALS->last_cycle_lxt_c_2-GLOBALS->first_cycle_lxt_c_2+1));

	/* rebuild time table from its deltas... */

	GLOBALS->positional_information_lxt_c_1 = (int *)malloc_2(GLOBALS->total_cycles_lxt_c_2 * sizeof(int));
	last_position=0;
	offs+=8;
	for(i=0;i<GLOBALS->total_cycles_lxt_c_2;i++)
		{
		last_position = GLOBALS->positional_information_lxt_c_1[i] = get_32(offs) + last_position;
		offs+=4;
		}
	GLOBALS->time_information =       (TimeType *)malloc_2(GLOBALS->total_cycles_lxt_c_2 * sizeof(TimeType));
	last_time=LLDescriptor(0);
	for(i=0;i<GLOBALS->total_cycles_lxt_c_2;i++)
		{
		last_time = GLOBALS->time_information[i] = ((TimeType)get_32(offs)) + last_time;
		GLOBALS->time_information[i] *= (GLOBALS->time_scale);
		offs+=4;
		}

	if(GLOBALS->ztime_table_size_lxt_c_1)
		{
		GLOBALS->mm_lxt_c_1 = GLOBALS->mmcache_lxt_c_1;
		free_2(decmem); decmem = NULL;
		gzclose(zhandle);
#if defined __MINGW32__ || defined _MSC_VER
		fclose(tmp);
#endif
		}
	}
	else	/* 64-bit read */
	{
	offs = GLOBALS->time_table_offset64_lxt_c_1;

	DEBUG(printf(LXTHDR"Time table position: %08x\n", GLOBALS->time_table_offset64_lxt_c_1 + 20));

	GLOBALS->total_cycles_lxt_c_2=(TimeType)((unsigned int)get_32(offs+0));
	DEBUG(printf(LXTHDR"Total cycles: %d\n", GLOBALS->total_cycles_lxt_c_2));

	if(GLOBALS->ztime_table_size_lxt_c_1)
		{
		int rc;

#if defined __MINGW32__ || defined _MSC_VER
		unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+offs+4;
		tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
		fwrite(t, GLOBALS->ztime_table_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
		zhandle = gzdopen(dup(fileno(tmp)), "rb");
#else
		if((offs+4)!=lseek(GLOBALS->fd_lxt_c_1, offs+4, SEEK_SET)) { fprintf(stderr, LXTHDR"ztime_table lseek error at offset %08x\n", (unsigned int)offs); exit(255); }
		zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif
		GLOBALS->mmcache_lxt_c_1 = GLOBALS->mm_lxt_c_1;
		total_mem = 8 + 8 + (GLOBALS->total_cycles_lxt_c_2 * 4) + (GLOBALS->total_cycles_lxt_c_2 * 8);
		decmem = malloc_2(total_mem); GLOBALS->mm_lxt_c_1 = decmem;

		rc=gzread(zhandle, decmem, total_mem);
		DEBUG(printf(LXTHDR"section offs for timetable decompression = %08x of len %d\n", offs, GLOBALS->ztime_table_size_lxt_c_1));
		DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));
		if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

		offs=0;	/* we're in our new memory region now.. */
		}
		else
		{
		offs+=4; /* skip past count to make consistent view between compressed/uncompressed */
		}

	GLOBALS->first_cycle_lxt_c_2=get_64(offs);
	DEBUG(printf(LXTHDR"First cycle: %d\n", GLOBALS->first_cycle_lxt_c_2));
	GLOBALS->last_cycle_lxt_c_2=get_64(offs+8);
	DEBUG(printf(LXTHDR"Last cycle: %d\n", GLOBALS->last_cycle_lxt_c_2));
	DEBUG(printf(LXTHDR"Total cycles (actual): %lld\n", GLOBALS->last_cycle_lxt_c_2-GLOBALS->first_cycle_lxt_c_2+1));

	/* rebuild time table from its deltas... */

	GLOBALS->positional_information_lxt_c_1 = (int *)malloc_2(GLOBALS->total_cycles_lxt_c_2 * sizeof(int));
	last_position=0;
	offs+=16;
	for(i=0;i<GLOBALS->total_cycles_lxt_c_2;i++)
		{
		last_position = GLOBALS->positional_information_lxt_c_1[i] = get_32(offs) + last_position;
		offs+=4;
		}
	GLOBALS->time_information =       (TimeType *)malloc_2(GLOBALS->total_cycles_lxt_c_2 * sizeof(TimeType));
	last_time=LLDescriptor(0);
	for(i=0;i<GLOBALS->total_cycles_lxt_c_2;i++)
		{
		last_time = GLOBALS->time_information[i] = ((TimeType)get_64(offs)) + last_time;
		GLOBALS->time_information[i] *= (GLOBALS->time_scale);
		offs+=8;
		}

	if(GLOBALS->ztime_table_size_lxt_c_1)
		{
		GLOBALS->mm_lxt_c_1 = GLOBALS->mmcache_lxt_c_1;
		free_2(decmem); decmem = NULL;
		gzclose(zhandle);
#if defined __MINGW32__ || defined _MSC_VER
		fclose(tmp);
#endif
		}
	}

if(GLOBALS->sync_table_offset_lxt_c_1)
	{
	offs = GLOBALS->sync_table_offset_lxt_c_1;

	if(GLOBALS->zsync_table_size_lxt_c_1)
		{
		int rc;
#if defined __MINGW32__ || defined _MSC_VER
		unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+offs;
		tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
		fwrite(t, GLOBALS->zsync_table_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
		zhandle = gzdopen(dup(fileno(tmp)), "rb");
#else
		if(offs!=lseek(GLOBALS->fd_lxt_c_1, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"zsync_table lseek error at offset %08x\n", (unsigned int)offs); exit(255); }
		zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif
		GLOBALS->mmcache_lxt_c_1 = GLOBALS->mm_lxt_c_1;
		decmem = malloc_2(total_mem = GLOBALS->numfacs * 4); GLOBALS->mm_lxt_c_1 = decmem;

		rc=gzread(zhandle, decmem, total_mem);
		DEBUG(printf(LXTHDR"section offs for synctable decompression = %08x of len %d\n", offs, GLOBALS->zsync_table_size_lxt_c_1));
		DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));
		if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

		offs=0;	/* we're in our new memory region now.. */
		}

	for(i=0;i<GLOBALS->numfacs;i++)
		{
		chg=get_32(offs);  offs+=4;
		if(chg>maxchg) {maxchg=chg; maxindx=i; }
		GLOBALS->lastchange[i]=chg;
		}

	if(GLOBALS->zsync_table_size_lxt_c_1)
		{
		GLOBALS->mm_lxt_c_1 = GLOBALS->mmcache_lxt_c_1;
		free_2(decmem); decmem = NULL;
		gzclose(zhandle);
#if defined __MINGW32__ || defined _MSC_VER
		fclose(tmp);
#endif
		}

	GLOBALS->maxchange_lxt_c_1=maxchg;
	GLOBALS->maxindex_lxt_c_1=maxindx;
	}

if(GLOBALS->zchg_size_lxt_c_1)
	{
	/* we don't implement the tempfile version for windows... */
#if !defined __MINGW32__ && !defined _MSC_VER
	if(GLOBALS->zchg_predec_size_lxt_c_1 > LXT_MMAP_MALLOC_BOUNDARY)
		{
		int fd_dummy;
		char *nam = tmpnam_2(NULL, &fd_dummy);
		FILE *tmp = fopen(nam, "wb");
		unsigned int len=GLOBALS->zchg_predec_size_lxt_c_1;
		int rc;
		char buf[32768];
		int fd2 = open(nam, O_RDONLY);
		char testbyte[2]={0,0};
		char is_bz2;

		unlink(nam);
		if(fd_dummy >=0) close(fd_dummy);

		fprintf(stderr, LXTHDR"Compressed change records detected, making tempfile...\n");
		if(GLOBALS->change_field_offset_lxt_c_1 != lseek(GLOBALS->fd_lxt_c_1, GLOBALS->change_field_offset_lxt_c_1, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", (unsigned int)GLOBALS->change_field_offset_lxt_c_1); exit(255); }

		is_bz2 = (read(GLOBALS->fd_lxt_c_1, &testbyte, 2))&&(testbyte[0]=='B')&&(testbyte[1]=='Z');

		if(GLOBALS->change_field_offset_lxt_c_1 != lseek(GLOBALS->fd_lxt_c_1, GLOBALS->change_field_offset_lxt_c_1, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", (unsigned int)GLOBALS->change_field_offset_lxt_c_1); exit(255); }

		if(is_bz2)
			{
			zhandle = BZ2_bzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");

			while(len)
				{
				int siz = (len>32768) ? 32768 : len;
				rc = BZ2_bzread(zhandle, buf, siz);
				if(rc!=siz) { fprintf(stderr, LXTHDR"gzread error to tempfile %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				if(1 != fwrite(buf, siz, 1, tmp)) { fprintf(stderr, LXTHDR"fwrite error to tempfile, exiting.\n"); exit(255); };
				len -= siz;
				}

			fprintf(stderr, LXTHDR"...expanded %08x into %08x bytes.\n", GLOBALS->zchg_size_lxt_c_1, GLOBALS->zchg_predec_size_lxt_c_1);
			BZ2_bzclose(zhandle);
			}
			else
			{
			zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");

			while(len)
				{
				int siz = (len>32768) ? 32768 : len;
				rc = gzread(zhandle, buf, siz);
				if(rc!=siz) { fprintf(stderr, LXTHDR"gzread error to tempfile %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				if(1 != fwrite(buf, siz, 1, tmp)) { fprintf(stderr, LXTHDR"fwrite error to tempfile, exiting.\n"); exit(255); };
				len -= siz;
				}

			fprintf(stderr, LXTHDR"...expanded %08x into %08x bytes.\n", GLOBALS->zchg_size_lxt_c_1, GLOBALS->zchg_predec_size_lxt_c_1);
			gzclose(zhandle);
			}
		munmap(GLOBALS->mm_lxt_c_1, GLOBALS->f_len_lxt_c_1); close(GLOBALS->fd_lxt_c_1);
		fflush(tmp);
		fseeko(tmp, 0, SEEK_SET);
		fclose(tmp);

		GLOBALS->fd_lxt_c_1 = fd2;
		GLOBALS->mm_lxt_c_1=mmap(NULL, GLOBALS->zchg_predec_size_lxt_c_1, PROT_READ, MAP_SHARED, GLOBALS->fd_lxt_c_1, 0);
		GLOBALS->mm_lxt_mmap_addr = GLOBALS->mm_lxt_c_1;
		GLOBALS->mm_lxt_mmap_len = GLOBALS->zchg_predec_size_lxt_c_1;
		GLOBALS->mm_lxt_c_1=(void *)((char *)GLOBALS->mm_lxt_c_1-4); /* because header and version don't exist in packed change records */
		}
		else
#endif
		{
		unsigned int len=GLOBALS->zchg_predec_size_lxt_c_1;
		int rc;
		char *buf = malloc_2(GLOBALS->zchg_predec_size_lxt_c_1);
		char *pnt = buf;
		char testbyte[2]={0,0};
		char is_bz2;

		fprintf(stderr, LXTHDR"Compressed change records detected...\n");
#if defined __MINGW32__ || defined _MSC_VER
		{
		unsigned char *t = (char *)GLOBALS->mm_lxt_c_1+GLOBALS->change_field_offset_lxt_c_1;
		tmp = tmpfile(); if(!tmp) { fprintf(stderr, LXTHDR"could not open decompression tempfile, exiting.\n"); exit(255); }
		fwrite(t, GLOBALS->zchg_size_lxt_c_1, 1, tmp); fseek(tmp, 0, SEEK_SET);
		is_bz2 = (get_byte(GLOBALS->change_field_offset_lxt_c_1)=='B') && (get_byte(GLOBALS->change_field_offset_lxt_c_1+1)=='Z');
		}
#else
		if(GLOBALS->change_field_offset_lxt_c_1 != lseek(GLOBALS->fd_lxt_c_1, GLOBALS->change_field_offset_lxt_c_1, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", (unsigned int)GLOBALS->change_field_offset_lxt_c_1); exit(255); }

		is_bz2 = (read(GLOBALS->fd_lxt_c_1, &testbyte, 2))&&(testbyte[0]=='B')&&(testbyte[1]=='Z');

		if(GLOBALS->change_field_offset_lxt_c_1 != lseek(GLOBALS->fd_lxt_c_1, GLOBALS->change_field_offset_lxt_c_1, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", (unsigned int)GLOBALS->change_field_offset_lxt_c_1); exit(255); }
#endif

		if(is_bz2)
			{
#if defined __MINGW32__ || defined _MSC_VER
			zhandle = BZ2_bzdopen(dup(fileno(tmp)), "rb");
#else
			zhandle = BZ2_bzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif

			while(len)
				{
				int siz = (len>32768) ? 32768 : len;
				rc = BZ2_bzread(zhandle, pnt, siz);
				if(rc!=siz) { fprintf(stderr, LXTHDR"BZ2_bzread error to buffer %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				pnt += siz;
				len -= siz;
				}

			fprintf(stderr, LXTHDR"...expanded %08x into %08x bytes.\n", GLOBALS->zchg_size_lxt_c_1, GLOBALS->zchg_predec_size_lxt_c_1);
			BZ2_bzclose(zhandle);
			}
			else
			{
#if defined __MINGW32__ || defined _MSC_VER
			zhandle = gzdopen(dup(fileno(tmp)), "rb");
#else
			zhandle = gzdopen(dup(GLOBALS->fd_lxt_c_1), "rb");
#endif
			while(len)
				{
				int siz = (len>32768) ? 32768 : len;
				rc = gzread(zhandle, pnt, siz);
				if(rc!=siz) { fprintf(stderr, LXTHDR"gzread error to buffer %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				pnt += siz;
				len -= siz;
				}

			fprintf(stderr, LXTHDR"...expanded %08x into %08x bytes.\n", GLOBALS->zchg_size_lxt_c_1, GLOBALS->zchg_predec_size_lxt_c_1);
			gzclose(zhandle);
#if defined __MINGW32__ || defined _MSC_VER
			fclose(tmp);
#endif
			}

		munmap(GLOBALS->mm_lxt_c_1, GLOBALS->f_len_lxt_c_1);
#if !defined __MINGW32__ && !defined _MSC_VER
		close(GLOBALS->fd_lxt_c_1);
#endif

		GLOBALS->fd_lxt_c_1=-1;
		GLOBALS->mm_lxt_c_1=buf-4; /* because header and version don't exist in packed change records */
		}
	}


if(!GLOBALS->sync_table_offset_lxt_c_1)
	{
	off_t vlen = GLOBALS->zchg_predec_size_lxt_c_1 ? GLOBALS->zchg_predec_size_lxt_c_1+4 : 0;
	unsigned int numfacs_bytes;
	unsigned int num_records = 0;
	unsigned int last_change_delta, numbytes;
	int *positional_compar = GLOBALS->positional_information_lxt_c_1;
        int *positional_kill_pnt = GLOBALS->positional_information_lxt_c_1 + GLOBALS->total_cycles_lxt_c_2;
        char positional_kill = 0;

	unsigned int dict_16_offset_new = 0;
	unsigned int dict_24_offset_new = 0;
	unsigned int dict_32_offset_new = 0;

	char *nam;
	FILE *tmp;
	int recfd;
	int fd_dummy;

	offs = GLOBALS->zchg_predec_size_lxt_c_1 ? 4 : 0;
	fprintf(stderr, LXTHDR"Linear LXT encountered...\n");

	if(!GLOBALS->zchg_predec_size_lxt_c_1)
		{
		fprintf(stderr, LXTHDR"Uncompressed linear LXT not supported, exiting.\n");
		exit(255);
		}

	if(GLOBALS->numfacs >= 256*65536)
        	{
                numfacs_bytes = 3;
                }
        else
        if(GLOBALS->numfacs >= 65536)
                {
                numfacs_bytes = 2;
               	}
        else
        if(GLOBALS->numfacs >= 256)
               	{
               	numfacs_bytes = 1;
		}
        else
               	{
               	numfacs_bytes = 0;
               	}

	nam = tmpnam_2(NULL, &fd_dummy);
	tmp = fopen(nam, "wb");
	GLOBALS->fpos_lxt_c_1 = 4;	/* fake 4 bytes padding */
	recfd = open(nam, O_RDONLY);

	unlink(nam);
	if(fd_dummy >=0) close(fd_dummy);


	while(offs < vlen)
		{
		int facidx = 0;
		unsigned char cmd;
		off_t offscache2, offscache3;
		unsigned int height;
		unsigned char cmdkill;

		num_records++;

                /* remake time vs position table on the fly */
                if(!positional_kill)
                        {
                        if(offs == *positional_compar)
                                {
                                *positional_compar = GLOBALS->fpos_lxt_c_1;
                                positional_compar++;
                                if(positional_compar == positional_kill_pnt) positional_kill = 1;
                                }
                        }

		switch(numfacs_bytes&3)
			{
			case 0: facidx = get_byte(offs); break;
			case 1: facidx = get_16(offs); break;
			case 2: facidx = get_24(offs); break;
			case 3: facidx = get_32(offs); break;
			}

		if(facidx>GLOBALS->numfacs)
			{
			fprintf(stderr, LXTHDR"Facidx %d out of range (vs %d) at offset %08x, exiting.\n", facidx, GLOBALS->numfacs, (unsigned int)offs);
			exit(255);
			}

		offs += (numfacs_bytes+1);

		cmdkill = GLOBALS->mvlfacs_lxt_c_2[facidx].flags & (LT_SYM_F_DOUBLE|LT_SYM_F_STRING);
		if(!cmdkill)
			{
			cmd = get_byte(offs);

			if(cmd>0xf)
				{
				fprintf(stderr, LXTHDR"Command byte %02x invalid at offset %08x, exiting.\n", cmd, (unsigned int)offs);
				exit(0);
				}

			offs++;
			}
			else
			{
			cmd=0;
			}

		offscache2 = offs;

		height = GLOBALS->mvlfacs_lxt_c_2[facidx].node_alias;
		if(height)
			{
			if(height >= 256*65536)
			        {
			        offs += 4;
			        }
			else
			if(height >= 65536)
			        {
			        offs += 3;
			        }
			else
			if(height >= 256)
			        {
			        offs += 2;
			        }
			else
			        {
			        offs += 1;
			        }
			}


		offscache3 = offs;
		if(!dict_16_offset_new)
			{
			if (offs == GLOBALS->dict_16_offset_lxt_c_1) { dict_16_offset_new = GLOBALS->fpos_lxt_c_1; }
			}
		else
		if(!dict_24_offset_new)
			{
			if (offs == GLOBALS->dict_24_offset_lxt_c_1) { dict_24_offset_new = GLOBALS->fpos_lxt_c_1; }
			}
		else
		if(!dict_32_offset_new)
			{
			if (offs == GLOBALS->dict_32_offset_lxt_c_1) { dict_32_offset_new = GLOBALS->fpos_lxt_c_1; }
			}


		/* printf("%08x : %04x %02x (%d) %s[%d:%d]\n", offscache, facidx, cmd, mvlfacs[facidx].len, mvlfacs[facidx].f_name, mvlfacs[facidx].msb, mvlfacs[facidx].lsb); */

		if(!cmdkill)
		switch(cmd)
			{
			unsigned int modlen;
			case 0x0:
			modlen = (!(GLOBALS->mvlfacs_lxt_c_2[facidx].flags&LT_SYM_F_INTEGER)) ? GLOBALS->mvlfacs_lxt_c_2[facidx].len : 32;
			if((GLOBALS->dict_string_mem_array_lxt_c_1) && (modlen>GLOBALS->dict_width_lxt_c_1))
				{
				if((!GLOBALS->dict_16_offset_lxt_c_1)||(offscache3<GLOBALS->dict_16_offset_lxt_c_1))
					{
					offs += 1;
					}
				else
				if((!GLOBALS->dict_24_offset_lxt_c_1)||(offscache3<GLOBALS->dict_24_offset_lxt_c_1))
					{
					offs += 2;
					}
				else
				if((!GLOBALS->dict_32_offset_lxt_c_1)||(offscache3<GLOBALS->dict_32_offset_lxt_c_1))
					{
					offs += 3;
					}
				else
					{
					offs += 4;
					}
				}
				else
				{
				offs += (modlen + 7)/8; /* was offs += (GLOBALS->mvlfacs_lxt_c_2[facidx].len + 7)/8 which is wrong for integers! */
				}

			break;


			case 0x1:
					offs += (GLOBALS->mvlfacs_lxt_c_2[facidx].len + 3)/4;
					break;

			case 0x2:
					offs += (GLOBALS->mvlfacs_lxt_c_2[facidx].len + 1)/2;
					break;

			case 0x3:
			case 0x4:
			case 0x5:
			case 0x6:
			case 0x7:
			case 0x8:
			case 0x9:
			case 0xa:
			case 0xb:	break;	/* single byte, no extra "skip" */


			case 0xc:
			case 0xd:
			case 0xe:
			case 0xf:	offs += ((cmd&3)+1);	/* skip past numbytes_trans */
					break;
			}
		else
			{
			/* cmdkill = 1 for strings + reals skip bytes */
			if(GLOBALS->mvlfacs_lxt_c_2[facidx].flags & LT_SYM_F_DOUBLE)
				{
				offs += 8;
				}
				else	/* strings */
				{
				while(get_byte(offs)) offs++;
				offs++;
				}
			}

		last_change_delta = GLOBALS->fpos_lxt_c_1 - GLOBALS->lastchange[facidx] - 2;
		GLOBALS->lastchange[facidx] = GLOBALS->fpos_lxt_c_1;

		GLOBALS->maxchange_lxt_c_1=GLOBALS->fpos_lxt_c_1;
		GLOBALS->maxindex_lxt_c_1=facidx;

        	if(last_change_delta >= 256*65536)
                	{
                	numbytes = 3;
                	}
        	else
        	if(last_change_delta >= 65536)
                	{
                	numbytes = 2;
                	}
        	else
        	if(last_change_delta >= 256)
                	{
                	numbytes = 1;
                	}
        	else
                	{
                	numbytes = 0;
                	}

		lt_emit_u8(tmp, (numbytes<<4) | cmd);
	        switch(numbytes&3)
	                {
	                case 0: lt_emit_u8(tmp, last_change_delta); break;
	                case 1: lt_emit_u16(tmp, last_change_delta); break;
	                case 2: lt_emit_u24(tmp, last_change_delta); break;
	                case 3: lt_emit_u32(tmp, last_change_delta); break;
                	}

		if(offs-offscache2)
			{
			GLOBALS->fpos_lxt_c_1 += fwrite((char *)GLOBALS->mm_lxt_c_1+offscache2, 1, offs-offscache2, tmp);	/* copy rest of relevant info */
			}
		}

	GLOBALS->dict_16_offset_lxt_c_1 = dict_16_offset_new;
	GLOBALS->dict_24_offset_lxt_c_1 = dict_24_offset_new;
	GLOBALS->dict_32_offset_lxt_c_1 = dict_32_offset_new;

        fflush(tmp);
        fseeko(tmp, 0, SEEK_SET);
        fclose(tmp);
	fprintf(stderr, LXTHDR"%d linear records converted into %08x bytes.\n", num_records, GLOBALS->fpos_lxt_c_1-4);

#if !defined __MINGW32__ && !defined _MSC_VER
	if(GLOBALS->zchg_predec_size_lxt_c_1 > LXT_MMAP_MALLOC_BOUNDARY)
		{
		munmap((char *)GLOBALS->mm_lxt_c_1+4, GLOBALS->zchg_predec_size_lxt_c_1); close(GLOBALS->fd_lxt_c_1);
		GLOBALS->mm_lxt_mmap_addr = NULL;
		GLOBALS->mm_lxt_mmap_len = 0;
		}
		else
#endif
		{
		free_2((char *)GLOBALS->mm_lxt_c_1+4);
		}

	GLOBALS->fd_lxt_c_1 = recfd;
#if defined __MINGW32__ || defined _MSC_VER
	win_fname = nam;
#endif
        GLOBALS->mm_lxt_c_1=mmap(NULL, GLOBALS->fpos_lxt_c_1-4, PROT_READ, MAP_SHARED, recfd, 0);
	GLOBALS->mm_lxt_mmap_addr = GLOBALS->mm_lxt_c_1;
	GLOBALS->mm_lxt_mmap_len = GLOBALS->fpos_lxt_c_1-4;
        GLOBALS->mm_lxt_c_1=(void *)((char *)GLOBALS->mm_lxt_c_1-4); /* because header and version don't exist in packed change records */
	}
}


/*
 * given a fac+offset, return the binary data for it
 */
static char *parse_offset(struct fac *which, off_t offs)
{
int v, v2;
unsigned int j;
int k;
unsigned int l;
char *pnt;
char repeat;

l=which->len;
pnt = GLOBALS->lt_buf_lxt_c_1;
v=get_byte(offs);
v2=v&0x0f;

switch(v2)
	{
	case 0x00:	/* MVL2 */
			{
			unsigned int msk;
			unsigned int bitcnt=0;
			int ch;

			if((GLOBALS->dict_string_mem_array_lxt_c_1) && (l>GLOBALS->dict_width_lxt_c_1))
				{
				unsigned int dictpos;
				unsigned int ld;

				offs += ((v>>4)&3)+2;	/* skip value */

				if((!GLOBALS->dict_16_offset_lxt_c_1)||(offs<GLOBALS->dict_16_offset_lxt_c_1))
					{
					dictpos = get_byte(offs);
					}
				else
				if((!GLOBALS->dict_24_offset_lxt_c_1)||(offs<GLOBALS->dict_24_offset_lxt_c_1))
					{
					dictpos = get_16(offs);
					}
				else
				if((!GLOBALS->dict_32_offset_lxt_c_1)||(offs<GLOBALS->dict_32_offset_lxt_c_1))
					{
					dictpos = get_24(offs);
					}
				else
					{
					dictpos = get_32(offs);
					}

				if(dictpos <= GLOBALS->dict_num_entries_lxt_c_1)
					{
					ld = strlen(GLOBALS->dict_string_mem_array_lxt_c_1[dictpos]);
					for(j=0;j<(l-(ld+1));j++)
						{
						*(pnt++) = '0';
						}
					*(pnt++) = '1';
					memcpy(pnt, GLOBALS->dict_string_mem_array_lxt_c_1[dictpos], ld);
					}
					else
					{
					fprintf(stderr, LXTHDR"dict entry at offset %08x [%d] out of range, ignoring!\n", dictpos, (unsigned int)offs);
					for(j=0;j<l;j++)
						{
						*(pnt++) = '0';
						}
					}
				}
				else
				{
				offs += ((v>>4)&3)+2;	/* skip value */
				for(j=0;;j++)
					{
					ch=get_byte(offs+j);
					msk=0x80;
					for(k=0;k<8;k++)
						{
						*(pnt++)= (ch&msk) ? '1' : '0';
						msk>>=1;
						bitcnt++;
						if(bitcnt==l) goto bail;
						}
					}
				}
			}
			break;

	case 0x01:	/* MVL4 */
			{
			unsigned int bitcnt=0;
			int ch;
			int rsh;

			offs += ((v>>4)&3)+2;	/* skip value */
			for(j=0;;j++)
				{
				ch=get_byte(offs+j);
				rsh=6;
				for(k=0;k<4;k++)
					{
					*(pnt++)=convert_mvl((ch>>rsh)&0x3);
					rsh-=2;
					bitcnt++;
					if(bitcnt==l) goto bail;
					}
				}
			}
			break;

	case 0x02:	/* MVL9 */
			{
			unsigned int bitcnt=0;
			int ch;
			int rsh;

			offs += ((v>>4)&3)+2;	/* skip value */
			for(j=0;;j++)
				{
				ch=get_byte(offs+j);
				rsh=4;
				for(k=0;k<2;k++)
					{
					*(pnt++)=convert_mvl(ch>>rsh);
					rsh-=4;
					bitcnt++;
					if(bitcnt==l) goto bail;
					}
				}
			}
			break;

	case 0x03:	/* mvl repeat expansions */
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
			repeat = convert_mvl(v2-3);
			for(j=0;j<l;j++)
				{
				*(pnt++)=repeat;
				}
			break;

	default:
			fprintf(stderr, LXTHDR"Unknown %02x at offset: %08x\n", v, (unsigned int)offs);
			exit(255);
	}

bail:
return(GLOBALS->lt_buf_lxt_c_1);
}


/*
 * mainline
 */
TimeType lxt_main(char *fname)
{
int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;
off_t tagpnt;
int tag;
struct symbol *sym_block = NULL;
struct Node *node_block = NULL;
char **f_name = NULL;

GLOBALS->fd_lxt_c_1=open(fname, O_RDONLY);
if(GLOBALS->fd_lxt_c_1<0)
        {
        fprintf(stderr, "Could not open '%s', exiting.\n", fname);
        vcd_exit(255);
        }

GLOBALS->f_len_lxt_c_1=lseek(GLOBALS->fd_lxt_c_1, (off_t)0, SEEK_END);
#if defined __MINGW32__ || defined _MSC_VER
win_fname = fname;
#endif
GLOBALS->mm_lxt_c_1=mmap(NULL, GLOBALS->f_len_lxt_c_1, PROT_READ, MAP_SHARED, GLOBALS->fd_lxt_c_1, 0);
GLOBALS->mm_lxt_mmap_addr = GLOBALS->mm_lxt_c_1;
GLOBALS->mm_lxt_mmap_len = GLOBALS->f_len_lxt_c_1;

if(get_16((off_t)0)!=LT_HDRID) /* scan-build, assign to i= from get_16 removed */
	{
	fprintf(stderr, "Not an LXT format AET, exiting.\n");
	vcd_exit(255);
	}

if((GLOBALS->version_lxt_c_1=get_16((off_t)2))>LT_VERSION)
	{
	fprintf(stderr, "Version %d of LXT format AETs not supported, exiting.\n", GLOBALS->version_lxt_c_1);
	vcd_exit(255);
	}

if(get_byte(GLOBALS->f_len_lxt_c_1-1)!=LT_TRLID)
	{
	fprintf(stderr, "LXT '%s' is truncated, exiting.\n", fname);
	vcd_exit(255);
	}

DEBUG(printf(LXTHDR"Loading LXT '%s'...\n", fname));
DEBUG(printf(LXTHDR"Len: %d\n", (unsigned int)GLOBALS->f_len_lxt_c_1));

/* SPLASH */                            splash_create();

tagpnt = GLOBALS->f_len_lxt_c_1-2;
while((tag=get_byte(tagpnt))!=LT_SECTION_END)
	{
	off_t offset = get_32(tagpnt-4);
	tagpnt-=5;

	switch(tag)
		{
		case LT_SECTION_CHG:			GLOBALS->change_field_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_CHG at %08x\n", offset)); break;
		case LT_SECTION_SYNC_TABLE:		GLOBALS->sync_table_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_SYNC_TABLE at %08x\n", offset)); break;
		case LT_SECTION_FACNAME:		GLOBALS->facname_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_FACNAME at %08x\n", offset)); break;
		case LT_SECTION_FACNAME_GEOMETRY:	GLOBALS->facgeometry_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_FACNAME_GEOMETRY at %08x\n", offset)); break;
		case LT_SECTION_TIMESCALE:		GLOBALS->timescale_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIMESCALE at %08x\n", offset)); break;
		case LT_SECTION_TIME_TABLE:		GLOBALS->time_table_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIME_TABLE at %08x\n", offset)); break;
		case LT_SECTION_TIME_TABLE64:		GLOBALS->time_table_offset64_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIME_TABLE64 at %08x\n", offset)); break;
		case LT_SECTION_INITIAL_VALUE:		GLOBALS->initial_value_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_INITIAL_VALUE at %08x\n", offset)); break;
		case LT_SECTION_DOUBLE_TEST:		GLOBALS->double_test_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_DOUBLE_TEST at %08x\n", offset)); break;

		case LT_SECTION_ZFACNAME_PREDEC_SIZE:	GLOBALS->zfacname_predec_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZFACNAME_PREDEC_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZFACNAME_SIZE:		GLOBALS->zfacname_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZFACNAME_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZFACNAME_GEOMETRY_SIZE:	GLOBALS->zfacgeometry_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZFACNAME_GEOMETRY_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZSYNC_SIZE:		GLOBALS->zsync_table_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZSYNC_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZTIME_TABLE_SIZE:	GLOBALS->ztime_table_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZTIME_TABLE_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZCHG_PREDEC_SIZE:	GLOBALS->zchg_predec_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZCHG_PREDEC_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZCHG_SIZE:		GLOBALS->zchg_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZCHG_SIZE = %08x\n", offset)); break;

		case LT_SECTION_ZDICTIONARY:		GLOBALS->zdictionary_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZDICTIONARY = %08x\n", offset)); break;
		case LT_SECTION_ZDICTIONARY_SIZE:	GLOBALS->zdictionary_predec_size_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZDICTIONARY_SIZE = %08x\n", offset)); break;

		case LT_SECTION_EXCLUDE_TABLE:		GLOBALS->exclude_offset_lxt_c_1=offset; DEBUG(printf(LXTHDR"LT_SECTION_EXCLUDE_TABLE = %08x\n", offset)); break;

		case LT_SECTION_TIMEZERO:		GLOBALS->lxt_timezero_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIMEZERO = %08x\n", offset)); break;

		default: fprintf(stderr, "Skipping unknown section tag %02x.\n", tag); break;
		}
	}

if(GLOBALS->lxt_timezero_offset)
	{
	GLOBALS->global_time_offset = get_64(GLOBALS->lxt_timezero_offset);
	}

if(GLOBALS->exclude_offset_lxt_c_1)
	{
	off_t offset = GLOBALS->exclude_offset_lxt_c_1;
	int ix, num_blackouts = get_32(offset);
	TimeType bs, be;
	struct blackout_region_t *bt;

	offset+=4;

	for(ix=0;ix<num_blackouts;ix++)
		{
		bs = get_64(offset); offset+=8;
		be = get_64(offset); offset+=8;

		bt = calloc_2(1, sizeof(struct blackout_region_t));
		bt->bstart = bs;
		bt->bend = be;

		bt->next = GLOBALS->blackout_regions;

		GLOBALS->blackout_regions = bt;
		}
	}

if(GLOBALS->double_test_offset_lxt_c_1)
	{
	create_double_endian_mask(GLOBALS->double_test_offset_lxt_c_1);
	}

if(GLOBALS->timescale_offset_lxt_c_1)
	{
	signed char scale;

	scale=(signed char)get_byte(GLOBALS->timescale_offset_lxt_c_1);
	exponent_to_time_scale(scale);
	}
	else
	{
	GLOBALS->time_dimension = 'n';
	}

if(!GLOBALS->facname_offset_lxt_c_1)
	{
	fprintf(stderr, "LXT '%s' is missing a facility name section, exiting.\n", fname);
	vcd_exit(255);
	}

GLOBALS->numfacs=get_32(GLOBALS->facname_offset_lxt_c_1);
DEBUG(printf(LXTHDR"Number of facs: %d\n", GLOBALS->numfacs));
GLOBALS->mvlfacs_lxt_c_2=(struct fac *)calloc_2(GLOBALS->numfacs,sizeof(struct fac));
GLOBALS->resolve_lxt_alias_to=(struct Node **)calloc_2(GLOBALS->numfacs,sizeof(struct Node *));
GLOBALS->lastchange=(unsigned int *)calloc_2(GLOBALS->numfacs,sizeof(unsigned int));
f_name = calloc_2(GLOBALS->numfacs,sizeof(char *));

if(GLOBALS->initial_value_offset_lxt_c_1)
	{
	switch(get_byte(GLOBALS->initial_value_offset_lxt_c_1))
		{
                case 0:         GLOBALS->initial_value_lxt_c_1 = AN_0; break;
                case 1:         GLOBALS->initial_value_lxt_c_1 = AN_1; break;
                case 2:         GLOBALS->initial_value_lxt_c_1 = AN_Z; break;
                case 4:         GLOBALS->initial_value_lxt_c_1 = AN_H; break;
                case 5:         GLOBALS->initial_value_lxt_c_1 = AN_U; break;
                case 6:         GLOBALS->initial_value_lxt_c_1 = AN_W; break;
                case 7:         GLOBALS->initial_value_lxt_c_1 = AN_L; break;
                case 8:         GLOBALS->initial_value_lxt_c_1 = AN_DASH; break;
		default:	GLOBALS->initial_value_lxt_c_1 = AN_X; break;
		}
	}
	else
	{
	GLOBALS->initial_value_lxt_c_1 = AN_X;
	}


if(GLOBALS->zdictionary_offset_lxt_c_1)
	{
	if(GLOBALS->zdictionary_predec_size_lxt_c_1)
		{
		build_dict();
		}
		else
		{
		fprintf(stderr, "LXT '%s' is missing a zdictionary_predec_size chunk, exiting.\n", fname);
		vcd_exit(255);
		}
	}

sym_block = (struct symbol *)calloc_2(GLOBALS->numfacs, sizeof(struct symbol));
node_block = (struct Node *)calloc_2(GLOBALS->numfacs,sizeof(struct Node));

build_facs(fname, f_name, node_block);
/* SPLASH */                            splash_sync(1, 5);
build_facs2(fname);
/* SPLASH */                            splash_sync(2, 5);

/* do your stuff here..all useful info has been initialized by now */

if(!GLOBALS->hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        GLOBALS->hier_delimeter='.';
        }

for(i=0;i<GLOBALS->numfacs;i++)
        {
	char buf[4096];
	char *str;
	struct fac *f;

	if(GLOBALS->mvlfacs_lxt_c_2[i].flags&LT_SYM_F_ALIAS)
		{
		int alias = GLOBALS->mvlfacs_lxt_c_2[i].node_alias;
		f=GLOBALS->mvlfacs_lxt_c_2+alias;

		while(f->flags&LT_SYM_F_ALIAS)
			{
			f=GLOBALS->mvlfacs_lxt_c_2+f->node_alias;
			}
		}
		else
		{
		f=GLOBALS->mvlfacs_lxt_c_2+i;
		}

	if((f->len>1)&& (!(f->flags&(LT_SYM_F_INTEGER|LT_SYM_F_DOUBLE|LT_SYM_F_STRING))) )
		{
		int len = sprintf(buf, "%s[%d:%d]", f_name[i],node_block[i].msi, node_block[i].lsi);
		str=malloc_2(len+1);
		if(!GLOBALS->alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
			}
                s=&sym_block[i];
                symadd_name_exists_sym_exists(s,str,0);
		prevsymroot = prevsym = NULL;
		}
		else
		{
		int gatecmp = (f->len==1) && (!(f->flags&(LT_SYM_F_INTEGER|LT_SYM_F_DOUBLE|LT_SYM_F_STRING))) && (node_block[i].msi!=-1) && (node_block[i].lsi!=-1);
		int revcmp = gatecmp && (i) && (!strcmp(f_name[i], f_name[i-1]));

		if(gatecmp)
			{
			int len = sprintf(buf, "%s[%d]", f_name[i],node_block[i].msi);
			str=malloc_2(len+1);
			if(!GLOBALS->alt_hier_delimeter)
				{
				strcpy(str, buf);
				}
				else
				{
				strcpy_vcdalt(str, buf, GLOBALS->alt_hier_delimeter);
				}
	                s=&sym_block[i];
	                symadd_name_exists_sym_exists(s,str,0);
			if((prevsym)&&(revcmp)&&(!strchr(f_name[i], '\\'))) /* allow chaining for search functions.. */
				{
				prevsym->vec_root = prevsymroot;
				prevsym->vec_chain = s;
				s->vec_root = prevsymroot;
				prevsym = s;
				}
				else
				{
				prevsymroot = prevsym = s;
				}
			}
			else
			{
			str=malloc_2(strlen(f_name[i])+1);
			if(!GLOBALS->alt_hier_delimeter)
				{
				strcpy(str, f_name[i]);
				}
				else
				{
				strcpy_vcdalt(str, f_name[i], GLOBALS->alt_hier_delimeter);
				}
	                s=&sym_block[i];
	                symadd_name_exists_sym_exists(s,str,0);
			prevsymroot = prevsym = NULL;

			if(f->flags&LT_SYM_F_INTEGER)
				{
				node_block[i].msi=31;
				node_block[i].lsi=0;
				GLOBALS->mvlfacs_lxt_c_2[i].len=32;
				}
			}
		}

	n=&node_block[i];
        n->nname=s->name;
        n->mv.mvlfac = GLOBALS->mvlfacs_lxt_c_2+i;

	if((f->len>1)||(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
		{
		n->extvals = 1;
		}

        n->head.time=-1;        /* mark 1st node as negative time */
        n->head.v.h_val=AN_X;
        s->n=n;
        }

free_2(f_name[0]);	/* the start of the big decompression buffer */
for(i=0;i<GLOBALS->numfacs;i++)
	{
	f_name[i] = NULL;
	}
free_2(f_name); f_name = NULL;

/* SPLASH */                            splash_sync(3, 5);
GLOBALS->facs=(struct symbol **)malloc_2(GLOBALS->numfacs*sizeof(struct symbol *));
for(i=0;i<GLOBALS->numfacs;i++)
	{
	char *subst, ch;
	int len;
	int esc = 0;

	GLOBALS->facs[i]=&sym_block[i];
        if((len=strlen(subst=GLOBALS->facs[i]->name))>GLOBALS->longestname) GLOBALS->longestname=len;
	while((ch=(*subst)))
		{
#ifdef WAVE_HIERFIX
		if(ch==GLOBALS->hier_delimeter) { *subst=(!esc) ? VCDNAM_HIERSORT : VCDNAM_ESCAPE; }	/* forces sort at hier boundaries */
#else
                if((ch==GLOBALS->hier_delimeter)&&(esc)) { *subst = VCDNAM_ESCAPE; }    /* forces sort at hier boundaries */
#endif
		else if(ch=='\\') { esc = 1; GLOBALS->escaped_names_found_vcd_c_1 = 1; }
		subst++;
		}
	}

fprintf(stderr, LXTHDR"Sorting facilities at hierarchy boundaries...");
wave_heapsort(GLOBALS->facs,GLOBALS->numfacs);
fprintf(stderr, "sorted.\n");

#ifdef WAVE_HIERFIX
for(i=0;i<GLOBALS->numfacs;i++)
	{
	char *subst, ch;

	subst=GLOBALS->facs[i]->name;
	while((ch=(*subst)))
		{
		if(ch==VCDNAM_HIERSORT) { *subst=GLOBALS->hier_delimeter; }	/* restore back to normal */
		subst++;
		}

#ifdef DEBUG_FACILITIES
	printf("%-4d %s\n",i,facs[i]->name);
#endif
	}
#endif

GLOBALS->facs_are_sorted=1;

fprintf(stderr, LXTHDR"Building facility hierarchy tree...");
/* SPLASH */                            splash_sync(4, 5);
init_tree();
for(i=0;i<GLOBALS->numfacs;i++)
{
char *nf = GLOBALS->facs[i]->name;
build_tree_from_name(nf, i);
}
/* SPLASH */                            splash_sync(5, 5);

if(GLOBALS->escaped_names_found_vcd_c_1)
	{
	for(i=0;i<GLOBALS->numfacs;i++)
		{
		char *subst, ch;

		subst=GLOBALS->facs[i]->name;
		while((ch=(*subst)))
			{
			if(ch==VCDNAM_ESCAPE) { *subst=GLOBALS->hier_delimeter; }	/* restore back to normal */
			subst++;
			}

#ifdef DEBUG_FACILITIES
		printf("%-4d %s\n",i,facs[i]->name);
#endif
		}
	}

treegraft(&GLOBALS->treeroot);
treesort(GLOBALS->treeroot, NULL);

if(GLOBALS->escaped_names_found_vcd_c_1)
        {
        treenamefix(GLOBALS->treeroot);
        }

fprintf(stderr, "built.\n\n");

#ifdef DEBUG_FACILITIES
treedebug(GLOBALS->treeroot,"");
#endif

GLOBALS->min_time = GLOBALS->first_cycle_lxt_c_2*GLOBALS->time_scale; GLOBALS->max_time=GLOBALS->last_cycle_lxt_c_2*GLOBALS->time_scale;
fprintf(stderr, "["TTFormat"] start time.\n["TTFormat"] end time.\n", GLOBALS->min_time, GLOBALS->max_time);
GLOBALS->is_lxt = ~0;

if(GLOBALS->blackout_regions)
        {
        struct blackout_region_t *bt = GLOBALS->blackout_regions;
        while(bt)
                {
                bt->bstart *= GLOBALS->time_scale;
                bt->bend *= GLOBALS->time_scale;
                bt = bt->next;
                }
        }

/* SPLASH */                            splash_finalize();
return(GLOBALS->max_time);
}


/*
 * this is the black magic that handles aliased signals...
 */
static void lxt_resolver(nptr np, nptr resolve)
{
np->extvals = resolve->extvals;
np->msi = resolve->msi;
np->lsi = resolve->lsi;
memcpy(&np->head, &resolve->head, sizeof(struct HistEnt));
np->curr = resolve->curr;
np->harray = resolve->harray;
np->numhist = resolve->numhist;
np->mv.mvlfac=NULL;
}


/*
 * actually import an lxt trace but don't do it if
 * 1) it's already been imported
 * 2) an alias of this trace has been imported--instead
 *    copy over the relevant info and be done with it.
 */
void import_lxt_trace(nptr np)
{
off_t offs, offsdelta;
int v, w;
TimeType tmval;
TimeType prevtmval;
struct HistEnt *htemp;
struct HistEnt *histent_head, *histent_tail;
char *parsed;
int len, i, j;
struct fac *f;

if(!(f=np->mv.mvlfac)) return;	/* already imported */

if(np->mv.mvlfac->flags&LT_SYM_F_ALIAS)
	{
	int alias = np->mv.mvlfac->node_alias;
	f=GLOBALS->mvlfacs_lxt_c_2+alias;

	if(GLOBALS->resolve_lxt_alias_to[alias])
		{
		if(!GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2]) GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2] = GLOBALS->resolve_lxt_alias_to[alias];
		}
		else
		{
		GLOBALS->resolve_lxt_alias_to[alias] = np;
		}

	while(f->flags&LT_SYM_F_ALIAS)
		{
		f=GLOBALS->mvlfacs_lxt_c_2+f->node_alias;

		if(GLOBALS->resolve_lxt_alias_to[f->node_alias])
			{
			if(!GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2]) GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2] = GLOBALS->resolve_lxt_alias_to[f->node_alias];
			}
			else
			{
			GLOBALS->resolve_lxt_alias_to[f->node_alias] = np;
			}
		}
	}

/* f is the head minus any aliases, np->mv.mvlfac is us... */
if(GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2])	/* in case we're an alias head for later.. */
	{
	lxt_resolver(np, GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2]);
	return;
	}

GLOBALS->resolve_lxt_alias_to[np->mv.mvlfac - GLOBALS->mvlfacs_lxt_c_2] = np;	/* in case we're an alias head for later.. */
offs=GLOBALS->lastchange[f-GLOBALS->mvlfacs_lxt_c_2];

tmval=LLDescriptor(-1);
prevtmval = LLDescriptor(-1);
len = np->mv.mvlfac->len;

histent_tail = htemp = histent_calloc();

if(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING))
	{
        htemp->v.h_vector = strdup_2((f->flags&LT_SYM_F_DOUBLE) ? "NaN" : "UNDEF");
        htemp->flags = HIST_REAL;
        if(f->flags&LT_SYM_F_STRING) htemp->flags |= HIST_STRING;
	}
	else
	{
	if(len>1)
		{
		htemp->v.h_vector = (char *)malloc_2(len);
		for(i=0;i<len;i++) htemp->v.h_vector[i] = AN_Z;
		}
		else
		{
		htemp->v.h_val = AN_Z;		/* z */
		}
	}
htemp->time = MAX_HISTENT_TIME;

histent_head = histent_calloc();
if(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING))
	{
        histent_head->v.h_vector = strdup_2((f->flags&LT_SYM_F_DOUBLE) ? "NaN" : "UNDEF");
        histent_head->flags = HIST_REAL;
        if(f->flags&LT_SYM_F_STRING) histent_head->flags |= HIST_STRING;
	}
	else
	{
	if(len>1)
		{
		histent_head->v.h_vector = (char *)malloc_2(len);
		for(i=0;i<len;i++) histent_head->v.h_vector[i] = AN_X;
		}
		else
		{
		histent_head->v.h_val = AN_X; /* x */
		}
	}
histent_head->time = MAX_HISTENT_TIME-1;
histent_head->next = htemp;	/* x */

np->numhist=2;

if(f->node_alias < 1)	/* sorry, arrays not supported */
while(offs)
	{
	unsigned char val = 0;

	if( (w=((v=get_byte(offs))&0xF)) >0xb)
		{
		off_t offsminus1, offsminus2, offsminus3;
		TimeType delta, time_offsminus1;
		int skip;

		switch(v&0xF0)
			{
			case 0x00:
				skip = 2;
				offsdelta=get_byte(offs+1);
				break;
			case 0x10:
				skip = 3;
				offsdelta=get_16(offs+1);
				break;
			case 0x20:
				skip = 4;
				offsdelta=get_24(offs+1);
				break;
			case 0x30:
				skip = 5;
				offsdelta=get_32(offs+1);
				break;
			default:
				fprintf(stderr, "Unknown %02x at offset: %08x\n", v, (unsigned int)offs);
				exit(0);
			}
		offsminus1 = offs-offsdelta-2;

		switch(get_byte(offsminus1)&0xF0)
			{
			case 0x00:
				offsdelta=get_byte(offsminus1+1);
				break;
			case 0x10:
				offsdelta=get_16(offsminus1+1);
				break;
			case 0x20:
				offsdelta=get_24(offsminus1+1);
				break;
			case 0x30:
				offsdelta=get_32(offsminus1+1);
				break;
			default:
				fprintf(stderr, "Unknown %02x at offset: %08x\n", get_byte(offsminus1), (unsigned int)offsminus1);
				exit(0);
			}
		offsminus2 = offsminus1-offsdelta-2;

		delta = (time_offsminus1=bsearch_mvl_timechain(offsminus1)) - bsearch_mvl_timechain(offsminus2);

		if(len>1)
			{
			DEBUG(fprintf(stderr, "!!! DELTA = %lld\n", delta));
			DEBUG(fprintf(stderr, "!!! offsminus1 = %08x\n", offsminus1));

			if(!GLOBALS->lxt_clock_compress_to_z)
				{
				int vval = get_byte(offsminus1)&0xF;
				int reps = 0;
				int rcnt;
				unsigned int reconstructm1 = 0;
				unsigned int reconstructm2 = 0;
				unsigned int reconstructm3 = 0;
				unsigned int rle_delta[2];
				int ix;

				if((vval!=0)&&(vval!=3)&&(vval!=4))
					{
					fprintf(stderr, "Unexpected clk compress byte %02x at offset: %08x\n", get_byte(offsminus1), (unsigned int)offsminus1);
					exit(0);
					}

				switch(w&3)
					{
					case 0:	reps = get_byte(offs+skip); break;
					case 1: reps = get_16(offs+skip); break;
					case 2: reps = get_24(offs+skip); break;
					case 3: reps = get_32(offs+skip); break;
					}

				reps++;

				DEBUG(fprintf(stderr, "!!! reps = %d\n", reps));

				parsed=parse_offset(f, offsminus1);
				for(ix=0;ix<len;ix++)
					{
					reconstructm1 <<= 1;
					reconstructm1 |= (parsed[ix]&1);
					}

				DEBUG(fprintf(stderr, "!!! M1 = '%08x'\n", reconstructm1));

				parsed=parse_offset(f, offsminus2);
				for(ix=0;ix<len;ix++)
					{
					reconstructm2 <<= 1;
					reconstructm2 |= (parsed[ix]&1);
					}

				DEBUG(fprintf(stderr, "!!! M2 = '%08x'\n", reconstructm2));

				switch(get_byte(offsminus2)&0xF0)
					{
					case 0x00:
						offsdelta=get_byte(offsminus2+1);
						break;
					case 0x10:
						offsdelta=get_16(offsminus2+1);
						break;
					case 0x20:
						offsdelta=get_24(offsminus2+1);
						break;
					case 0x30:
						offsdelta=get_32(offsminus2+1);
						break;
					default:
						fprintf(stderr, "Unknown %02x at offset: %08x\n", get_byte(offsminus2), (unsigned int)offsminus2);
						exit(0);
					}
				offsminus3 = offsminus2-offsdelta-2;

				parsed=parse_offset(f, offsminus3);
				for(ix=0;ix<len;ix++)
					{
					reconstructm3 <<= 1;
					reconstructm3 |= (parsed[ix]&1);
					}

				DEBUG(fprintf(stderr, "!!! M3 = '%08x'\n", reconstructm3));

				rle_delta[0] = reconstructm2 - reconstructm3;
				rle_delta[1] = reconstructm1 - reconstructm2;

				DEBUG(fprintf(stderr, "!!! RLE0 = '%08x'\n", rle_delta[0]));
				DEBUG(fprintf(stderr, "!!! RLE1 = '%08x'\n", rle_delta[1]));

				DEBUG(fprintf(stderr, "!!! time_offsminus1 = %lld\n", time_offsminus1));

				tmval = time_offsminus1 + (delta * reps);
				for(rcnt=0;rcnt<reps;rcnt++)
					{
					int k;
					int jx = (reps - rcnt);
					unsigned int res = reconstructm1 +
						((jx/2)+(jx&0))*rle_delta[1] +
						((jx/2)+(jx&1))*rle_delta[0];

					DEBUG(fprintf(stderr, "!!! %lld -> '%08x'\n", tmval, res));

					for(k=0;k<len;k++)
						{
						parsed[k] = ((1<<(len-k-1)) & res) ? '1' : '0';
						}

					htemp = histent_calloc();
					htemp->v.h_vector = (char *)malloc_2(len);
					memcpy(htemp->v.h_vector, parsed, len);
					htemp->time = tmval;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;

					tmval-=delta;
					}
				}
				else	/* compress to z on multibit */
				{
				int ix;

				htemp = histent_calloc();
				htemp->v.h_vector = (char *)malloc_2(len);
				for(ix=0;ix<len;ix++) { htemp->v.h_vector[ix] = 'z'; }
				tmval = time_offsminus1 + delta;
				htemp->time = tmval;
				htemp->next = histent_head;
				histent_head = htemp;
				np->numhist++;
				}

			offs = offsminus1;	/* no need to recalc it again! */
			continue;
			}
			else
			{
			if(!GLOBALS->lxt_clock_compress_to_z)
				{
				int vval = get_byte(offsminus1)&0xF;
				int reps = 0;
				int rcnt;

				if((vval<3)||(vval>4))
					{
					fprintf(stderr, "Unexpected clk compress byte %02x at offset: %08x\n", get_byte(offsminus1), (unsigned int)offsminus1);
					exit(0);
					}

				switch(w&3)
					{
					case 0:	reps = get_byte(offs+skip); break;
					case 1: reps = get_16(offs+skip); break;
					case 2: reps = get_24(offs+skip); break;
					case 3: reps = get_32(offs+skip); break;
					}

				reps++;
				vval = (reps & 1) ^ (vval==4);	/* because x3='0', x4='1' */
				vval = (vval==0) ? AN_0 : AN_1;

				tmval = time_offsminus1 + (delta * reps);
				for(rcnt=0;rcnt<reps;rcnt++)
					{
					if(vval!=histent_head->v.h_val)
						{
						htemp = histent_calloc();
						htemp->v.h_val = vval;
						htemp->time = tmval;
						htemp->next = histent_head;
						histent_head = htemp;
						np->numhist++;
						}
						else
						{
						histent_head->time = tmval;
						}

					tmval-=delta;
					vval= (vval==AN_0) ? AN_1: AN_0;
					}
				}
				else
				{
				int vval=AN_Z;

				if(vval!=histent_head->v.h_val)
					{
					htemp = histent_calloc();
					htemp->v.h_val = vval;
					htemp->time = time_offsminus1 + delta;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
					}
					else
					{
					histent_head->time = time_offsminus1 + delta;
					}
				tmval = time_offsminus1 + delta;
				}
			}

		offs = offsminus1;	/* no need to recalc it again! */
		continue;
		}
	else
	if((tmval=bsearch_mvl_timechain(offs))!=prevtmval)	/* get rid of glitches (if even possible) */
		{
		DEBUG(printf(LXTHDR"offs: %08x is time %08x\n", offs, tmval));
		if(!(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
			{
			parsed=parse_offset(f, offs);

			if(len==1)
				{
				switch(parsed[0])
					{
				        case '0':       val = AN_0; break;
				        case 'x':	val = AN_X; break;
				        case 'z':	val = AN_Z; break;
				        case '1':       val = AN_1; break;
                                        case 'h':       val = AN_H; break;
                                        case 'u':       val = AN_U; break;
                                        case 'w':       val = AN_W; break;
                                        case 'l':       val = AN_L; break;
                                        case '-':       val = AN_DASH; break;
				        }

				if(val!=histent_head->v.h_val)
					{
					htemp = histent_calloc();
					htemp->v.h_val = val;
					htemp->time = tmval;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
					}
					else
					{
					histent_head->time = tmval;
					}
				}
				else
				{
				if(memcmp(parsed, histent_head->v.h_vector, len))
					{
					htemp = histent_calloc();
					htemp->v.h_vector = (char *)malloc_2(len);
					memcpy(htemp->v.h_vector, parsed, len);
					htemp->time = tmval;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
					}
					else
					{
					histent_head->time = tmval;
					}
				}
			}
		else
		if(f->flags&LT_SYM_F_DOUBLE)
			{
			int offs_dbl = offs + ((get_byte(offs)>>4)&3)+2;   /* skip value */

			htemp = histent_calloc();
			htemp->flags = HIST_REAL;

			if(GLOBALS->double_is_native_lxt_c_1)
				{
#ifdef WAVE_HAS_H_DOUBLE
				memcpy(&htemp->v.h_double, ((char *)GLOBALS->mm_lxt_c_1+offs_dbl), sizeof(double));
#else
				htemp->v.h_vector = ((char *)GLOBALS->mm_lxt_c_1+offs_dbl);
				DEBUG(printf(LXTHDR"Added double '%.16g'\n", *((double *)(GLOBALS->mm_lxt_c_1+offs_dbl))));
#endif
				}
				else
				{
#ifdef WAVE_HAS_H_DOUBLE
				double *h_d = (double *)swab_double_via_mask(offs_dbl);
				htemp->v.h_double = *h_d;
				free_2(h_d);
#else
				htemp->v.h_vector = swab_double_via_mask(offs_dbl);
				DEBUG(printf(LXTHDR"Added bytefixed double '%.16g'\n", *((double *)(htemp->v.h_vector))));
#endif
				}
			htemp->time = tmval;
			htemp->next = histent_head;
			histent_head = htemp;
			np->numhist++;
			}
			else
			{						/* defaults to if(f->flags&LT_SYM_F_STRING) */
			int offs_str = offs + ((get_byte(offs)>>4)&3)+2;   /* skip value */

			htemp = histent_calloc();
			htemp->flags = HIST_REAL|HIST_STRING;

			htemp->v.h_vector = ((char *)GLOBALS->mm_lxt_c_1+offs_str);
			DEBUG(printf(LXTHDR"Added string '%s'\n", (unsigned char *)GLOBALS->mm_lxt_c_1+offs_str));
			htemp->time = tmval;
			htemp->next = histent_head;
			histent_head = htemp;
			np->numhist++;
			}
		}

	prevtmval = tmval;

/*	v=get_byte(offs); */
	switch(v&0xF0)
		{
		case 0x00:
			offsdelta=get_byte(offs+1);
			break;

		case 0x10:
			offsdelta=get_16(offs+1);
			break;

		case 0x20:
			offsdelta=get_24(offs+1);
			break;

		case 0x30:
			offsdelta=get_32(offs+1);
			break;

		default:
			fprintf(stderr, "Unknown %02x at offset: %08x\n", v, (unsigned int)offs);
			exit(0);
		}

	offs = offs-offsdelta-2;
	}

np->mv.mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

for(j=0;j>-2;j--)
	{
	if(tmval!=GLOBALS->first_cycle_lxt_c_2)
		{
		char init;

		htemp = histent_calloc();

		if(!(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
			{
			if(GLOBALS->initial_value_offset_lxt_c_1)
				{
				init = GLOBALS->initial_value_lxt_c_1;
				}
				else
				{
				init = AN_X; /* x if unspecified */
				}

			if(len>1)
				{
				char *pnt = htemp->v.h_vector = (char *)malloc_2(len);	/* zeros */
				int ix;
				for(ix=0;ix<len;ix++)
					{
					*(pnt++) = init;
					}
				}
				else
				{
				htemp->v.h_val = init;
				}
			}
			else
			{
			htemp->flags = HIST_REAL;
			if(f->flags&LT_SYM_F_STRING) htemp->flags |= HIST_STRING;
			}

		htemp->time = GLOBALS->first_cycle_lxt_c_2+j;
		htemp->next = histent_head;
		histent_head = htemp;
		np->numhist++;
		}

	tmval=GLOBALS->first_cycle_lxt_c_2+1;
	}

if(!(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
	{
	if(len>1)
		{
		np->head.v.h_vector = (char *)malloc_2(len);
		for(i=0;i<len;i++) np->head.v.h_vector[i] = AN_X;
		}
		else
		{
		np->head.v.h_val = AN_X;                     /* 'x' */
		}
	}
	else
	{
	np->head.flags = HIST_REAL;
	if(f->flags&LT_SYM_F_STRING) np->head.flags |= HIST_STRING;
	}
np->head.time  = -2;
np->head.next = histent_head;
np->curr = histent_tail;
np->numhist++;
}

