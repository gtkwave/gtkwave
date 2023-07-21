/*
 * Copyright (c) Tony Bybell 2008-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "hierpack.h"

#define VLI_SIZE (10)

static void out_c(unsigned char ch)
{
if((GLOBALS->fmem_buf_offs+1) >= GLOBALS->fmem_buf_siz)
	{
	GLOBALS->fmem_buf = realloc_2(GLOBALS->fmem_buf, GLOBALS->fmem_buf_siz = 2 * GLOBALS->fmem_buf_siz);
	}

GLOBALS->fmem_buf[GLOBALS->fmem_buf_offs++] = ch;
}


static int enc_var(size_t v, unsigned char *buf)
{
size_t nxt;
unsigned char *pnt = buf+VLI_SIZE;

while((nxt = v>>7))
        {
        *(--pnt) = (v&0x7f) | 0x80;
        v = nxt;
        }
*(--pnt) = (v&0x7f);
return(buf+VLI_SIZE-pnt);
}


static void out_write(unsigned char *s, int len)
{
int i;

for(i=0;i<len;i++)
	{
	out_c(s[i]);
	}
}


void init_facility_pack(void)
{
if(GLOBALS->do_hier_compress)
	{
	fprintf(stderr, "FACPACK | Using compressed facilities\n");

	GLOBALS->fmem_buf_offs = 0;
	GLOBALS->fmem_buf_siz = 1024*1024;
	GLOBALS->fmem_buf = malloc_2(GLOBALS->fmem_buf_siz);

	GLOBALS->hp_buf_siz = 1024;
	GLOBALS->hp_buf = calloc_2(GLOBALS->hp_buf_siz, sizeof(unsigned char));
	GLOBALS->hp_offs = calloc_2(GLOBALS->hp_buf_siz, sizeof(size_t));
	}
}


void freeze_facility_pack(void)
{
if(GLOBALS->do_hier_compress)
	{
	free_2(GLOBALS->hp_buf); GLOBALS->hp_buf = NULL;
	free_2(GLOBALS->hp_offs); GLOBALS->hp_offs = NULL;
	GLOBALS->hp_buf_siz = 0;

	if(GLOBALS->fmem_buf)
		{
		GLOBALS->fmem_buf = realloc_2(GLOBALS->fmem_buf, GLOBALS->hp_prev);
		}
	fprintf(stderr, "FACPACK | Compressed %lu to %lu bytes.\n", (unsigned long)GLOBALS->fmem_uncompressed_siz, (unsigned long)GLOBALS->hp_prev);
	}
}


char *compress_facility(unsigned char *key, unsigned int len)
{
size_t mat = 0;
size_t plen;
size_t i;
unsigned char vli[VLI_SIZE];

if(len > GLOBALS->hp_buf_siz)
	{
	GLOBALS->hp_buf_siz = len;
	GLOBALS->hp_buf = realloc_2(GLOBALS->hp_buf, GLOBALS->hp_buf_siz * sizeof(unsigned char));
	GLOBALS->hp_offs = realloc_2(GLOBALS->hp_offs, GLOBALS->hp_buf_siz * sizeof(size_t));
	}

GLOBALS->fmem_uncompressed_siz += (len + 1);

for(i=0;i<=len;i++)
	{
	if(!key[i]) break;

        mat = i;
        if(key[i] != GLOBALS->hp_buf[i]) break;
        }

if(!mat)
	{
        GLOBALS->hp_prev += (plen = enc_var(mat, vli));
        out_write(vli+VLI_SIZE-plen, plen);
        }
        else
        {
        size_t back = GLOBALS->hp_prev - GLOBALS->hp_offs[mat-1];

        plen = enc_var(back, vli);
        if(mat > plen)
        	{
                GLOBALS->hp_prev += plen;
                out_write(vli+VLI_SIZE-plen, plen);
                }
                else
                {
                mat = 0;
                GLOBALS->hp_prev += (plen = enc_var(mat, vli));
                out_write(vli+VLI_SIZE-plen, plen);
                }
        }

out_c(0);
GLOBALS->hp_prev++;

for(i=mat;i<len;i++)
	{
        out_c(key[i]);
        GLOBALS->hp_buf[i] = key[i];
        GLOBALS->hp_offs[i] = GLOBALS->hp_prev;
        GLOBALS->hp_prev++;
        }
GLOBALS->hp_buf[i] = 0; GLOBALS->hp_offs[i] = 0;

return( (((GLOBALS->hp_prev-1)<<1)|1) + ((char *)NULL) ); /* flag value with |1 to indicate is compressed */
}


char *hier_decompress_flagged(char *n, int *was_packed)
{
size_t dcd;
size_t dcd2;
size_t val;
char *str;
int ob;
int shamt;
int avoid_strdup = *was_packed;

*was_packed = GLOBALS->do_hier_compress;

if(!GLOBALS->do_hier_compress)
	{
	return(n);
	}

dcd = n - ((char *)NULL);

if(!(dcd & 1)) /* value was flagged with |1 to indicate is compressed; malloc never returns this */
	{
	*was_packed = 0;
	return(n);
	}
dcd >>= 1;

str = GLOBALS->module_tree_c_1;
ob = GLOBALS->longestname + 1;

str[--ob] = 0;

do
	{
        while(GLOBALS->fmem_buf[dcd])
                {
                str[--ob] = GLOBALS->fmem_buf[dcd];
                dcd--;
                }

        dcd2 = --dcd;
        val = 0;
        shamt = 0;
        for(;;)
        	{
                val |= ((GLOBALS->fmem_buf[dcd2] & 0x7f) << shamt); shamt += 7;
                if(!(GLOBALS->fmem_buf[dcd2] & 0x80)) break;
                dcd2--;
                }

	dcd = dcd2 - val;
        } while(val);

return((avoid_strdup != HIER_DEPACK_ALLOC) ? (str+ob) : strdup_2(str+ob));
}


void hier_auto_enable(void)
{
if((!GLOBALS->do_hier_compress) && (!GLOBALS->disable_auto_comphier) && (GLOBALS->numfacs >= HIER_AUTO_ENABLE_CNT))
	{
	GLOBALS->do_hier_compress = 1;
	}
}
