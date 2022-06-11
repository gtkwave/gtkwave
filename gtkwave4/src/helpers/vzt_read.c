/*
 * Copyright (c) 2003-2016 Tony Bybell.
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

#if defined(__CYGWIN__) || defined(__MINGW32__)
#undef HAVE_RPC_XDR_H
#endif

#if HAVE_RPC_XDR_H
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif
#include "vzt_read.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

/****************************************************************************/

static int is_big_endian(void)
{
union
	{
	vztint32_t u32;
	unsigned char c[sizeof(vztint32_t)];
	} u;

u.u32 = 1;
return(u.c[sizeof(vztint32_t)-1] == 1);
}

/****************************************************************************/

struct vzt_ncycle_autosort
{
struct vzt_ncycle_autosort *next;
};

struct vzt_pth_args
{
struct vzt_rd_trace *lt;
struct vzt_rd_block *b;
};

struct vzt_synvec_chain
{
vztint32_t num_entries;
vztint32_t chain[1];
};


#ifdef PTHREAD_CREATE_DETACHED
_VZT_RD_INLINE static int vzt_rd_pthread_mutex_init(struct vzt_rd_trace *lt, pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
if(lt->pthreads) { pthread_mutex_init(mutex, mutexattr); }
return(0);
}

_VZT_RD_INLINE static void vzt_rd_pthread_mutex_lock(struct vzt_rd_trace *lt, pthread_mutex_t *mx)
{
if(lt->pthreads) { pthread_mutex_lock(mx); }
}

_VZT_RD_INLINE static void vzt_rd_pthread_mutex_unlock(struct vzt_rd_trace *lt, pthread_mutex_t *mx)
{
if(lt->pthreads) { pthread_mutex_unlock(mx); }
}

_VZT_RD_INLINE static void vzt_rd_pthread_mutex_destroy(struct vzt_rd_trace *lt, pthread_mutex_t *mutex)
{
if(lt->pthreads) { pthread_mutex_destroy(mutex); }
}

_VZT_RD_INLINE static void vzt_rd_pthread_create(struct vzt_rd_trace *lt, pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
if(lt->pthreads)
	{
	pthread_attr_init(attr);
	pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
	pthread_create(thread, attr, start_routine, arg);
	}
}
#else
#define vzt_rd_pthread_mutex_init(a, b, c)
#define vzt_rd_pthread_mutex_lock(a, b)
#define vzt_rd_pthread_mutex_unlock(a, b)
#define vzt_rd_pthread_mutex_destroy(a, b)
#define vzt_rd_pthread_create(a, b, c, d, e)
#endif

/****************************************************************************/

#ifdef _WAVE_BE32

/*
 * reconstruct 8/16/32/64 bits out of the vzt's representation
 * of a big-endian integer.  this is for 32-bit PPC so no byte
 * swizzling needs to be done at all.
 */

#define vzt_rd_get_byte(mm,offset)    ((unsigned int)(*((unsigned char *)(mm)+(offset))))
#define vzt_rd_get_16(mm,offset)      ((unsigned int)(*((unsigned short *)(((unsigned char *)(mm))+(offset)))))
#define vzt_rd_get_32(mm,offset)      (*(unsigned int *)(((unsigned char *)(mm))+(offset)))
#define vzt_rd_get_64(mm,offset)      ((((vztint64_t)vzt_rd_get_32((mm),(offset)))<<32)|((vztint64_t)vzt_rd_get_32((mm),(offset)+4)))

#else

/*
 * reconstruct 8/16/24/32 bits out of the vzt's representation
 * of a big-endian integer.  this should work on all architectures.
 */
#define vzt_rd_get_byte(mm,offset) 	((unsigned int)(*((unsigned char *)(mm)+(offset))))

static unsigned int vzt_rd_get_16(void *mm, int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)nn);
return((m1<<8)|m2);
}

static unsigned int vzt_rd_get_32(void *mm, int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)(nn++));
unsigned int m3=*((unsigned char *)(nn++));
unsigned int m4=*((unsigned char *)nn);
return((m1<<24)|(m2<<16)|(m3<<8)|m4);
}

static vztint64_t vzt_rd_get_64(void *mm, int offset)
{
return(
(((vztint64_t)vzt_rd_get_32(mm,offset))<<32)
|((vztint64_t)vzt_rd_get_32(mm,offset+4))
);
}

#endif

static unsigned int vzt_rd_get_32r(void *mm, int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned int m4=*((unsigned char *)(nn++));
unsigned int m3=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)(nn++));
unsigned int m1=*((unsigned char *)nn);
return((m1<<24)|(m2<<16)|(m3<<8)|m4);
}


static vztint32_t vzt_rd_get_v32(char **mmx)
{
signed char *c;
signed char *beg;
vztint32_t val;
signed char **mm = (signed char **)mmx;

c = *mm;
beg = c;

if(*c>=0)
	{
	while(*c>=0) c++;
	*mm = c+1;

	val = (vztint32_t)(*c&0x7f);
	do
		{
		val <<= 7;
		val |= (vztint32_t)*(--c);
		} while (c!=beg);
	}
	else
	{
	*mm = c+1;
	val = (vztint32_t)(*c&0x7f);
	}

return(val);
}

static vztint64_t vzt_rd_get_v64(char **mmx)
{
signed char *c;
signed char *beg;
vztint64_t val;
signed char **mm = (signed char **)mmx;

c = *mm;
beg = c;

if(*c>=0)
	{
	while(*c>=0) c++;
	*mm = c+1;

	val = (vztint64_t)(*c&0x7f);
	do
		{
		val <<= 7;
		val |= (vztint64_t)*(--c);
		} while (c!=beg);
	}
	else
	{
	val = (vztint64_t)(*c&0x7f);
	*mm = c+1;
	}

return(val);
}


#if 0
_VZT_RD_INLINE static vztint32_t vzt_rd_get_v32_rvs(signed char **mm)
{
signed char *c = *mm;
vztint32_t val;

val = (vztint32_t)(*(c--)&0x7f);

while(*c>=0)
	{
	val <<= 7;
	val |= (vztint32_t)*(c--);
	}

*mm = c;
return(val);
}
#endif

/****************************************************************************/

/*
 * fast SWAR ones count for 32 bits
 */
_VZT_RD_INLINE static vztint32_t
vzt_rd_ones_cnt(vztint32_t x)
{
x -= ((x >> 1) & 0x55555555);
x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
x = (((x >> 4) + x) & 0x0f0f0f0f);
return((x * 0x01010101) >> 24);
}

/*
 * total zero count to the right of the first rightmost one bit
 * encountered.  its intended use is to
 * "return the bitposition of the least significant 1 in vztint32_t"
 * (use x &= ~(x&-x) to clear out that bit quickly)
 */
_VZT_RD_INLINE static vztint32_t
vzt_rd_tzc(vztint32_t x)
{
return (vzt_rd_ones_cnt((x & -x) - 1));
}

/****************************************************************************/

/*
 * i2c utility function
 */
unsigned int vzt_rd_expand_bits_to_integer(int len, char *s)
{
unsigned int v = 0;
int i;

for(i=0;i<len;i++)
	{
	v <<= 1;
	v |= ((*s) & 1);
	s++;
	}

return(v);
}

/****************************************************************************/

/*
 * process a single block
 */
static void vzt_rd_block_vch_decode(struct vzt_rd_trace *lt, struct vzt_rd_block *b)
{
vzt_rd_pthread_mutex_lock(lt, &b->mutex);

if((!b->times)&&(b->mem))
{
vztint64_t *times=NULL;
vztint32_t *change_dict=NULL;
vztint32_t *val_dict=NULL;
unsigned int num_time_ticks, num_sections, num_dict_entries;
char *pnt = b->mem;
vztint32_t i, j, m, num_dict_words;
/* vztint32_t *block_end = (vztint32_t *)(pnt + b->uncompressed_siz); */
vztint32_t *val_tmp;
unsigned int num_bitplanes;
uintptr_t padskip;

num_time_ticks = vzt_rd_get_v32(&pnt);
/* fprintf(stderr, "* num_time_ticks = %d\n", num_time_ticks); */
if(num_time_ticks != 0)
	{
	vztint64_t cur_time;
	times = malloc(num_time_ticks * sizeof(vztint64_t));
	times[0] = cur_time = vzt_rd_get_v64(&pnt);
	for(i=1;i<num_time_ticks;i++)
		{
		vztint64_t delta = vzt_rd_get_v64(&pnt);
		cur_time += delta;
		times[i] = cur_time;
		}
	}
	else
	{
	vztint64_t cur_time = b->start;

	num_time_ticks = b->end - b->start + 1;
	times = malloc(num_time_ticks * sizeof(vztint64_t));

	for(i=0;i<num_time_ticks;i++)
		{
		times[i] = cur_time++;
		}
	}

num_sections = vzt_rd_get_v32(&pnt);
num_dict_entries = vzt_rd_get_v32(&pnt);
padskip = ((uintptr_t)pnt)&3; pnt += (padskip) ? 4-padskip : 0; /* skip pad to next 4 byte boundary */

/* fprintf(stderr, "num_sections: %d, num_dict_entries: %d\n", num_sections, num_dict_entries); */

if(b->rle)
	{
	vztint32_t *curr_dec_dict;
	vztint32_t first_bit = 0, curr_bit = 0;
	vztint32_t runlen;

	val_dict = calloc(1, b->num_rle_bytes = (num_dict_words = num_sections * num_dict_entries) * sizeof(vztint32_t));
	curr_dec_dict = val_dict;

	vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
	lt->block_mem_consumed += b->num_rle_bytes;
	vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);

	for(i=0;i<num_dict_entries;i++)
		{
		vztint32_t curr_dec_bit = 0, curr_dec_word = 0;
		for(;;)
			{
			runlen = vzt_rd_get_v32(&pnt);
			if(!runlen)
				{
				first_bit = first_bit ^ 1;
				}
			curr_bit ^= 1;
			if((!curr_dec_word)&&(!curr_dec_bit))
				{
				curr_bit = first_bit;
				}

			for(j=0;j<runlen;j++)
				{
				if(curr_bit) *curr_dec_dict |= (1<<curr_dec_bit);
				curr_dec_bit++;
				if(curr_dec_bit != 32) continue;

				curr_dec_bit = 0;
				curr_dec_dict++;
				curr_dec_word++;
				if(curr_dec_word == num_sections) goto iloop;
				}
			}
		iloop: i+=0; /* deliberate...only provides a jump target to loop bottom */
		}

	goto bpcalc;
	}

val_dict = (vztint32_t *) pnt;
pnt = (char *)(val_dict + (num_dict_words = num_dict_entries * num_sections));

bpcalc:
num_bitplanes = vzt_rd_get_byte(pnt, 0) + 1;	pnt++;
b->multi_state = (num_bitplanes > 1);
padskip = ((uintptr_t)pnt)&3; pnt += (padskip) ? 4-padskip : 0; /* skip pad to next 4 byte boundary */
b->vindex = (vztint32_t *)(pnt);

if(is_big_endian()) /* have to bswap the value changes on big endian machines... */
	{
	if(!b->rle)
		{
		for(i=0;i<num_dict_words;i++)
			{
			val_dict[i] = vzt_rd_get_32r(val_dict + i, 0);
			}
		}

	val_tmp = b->vindex;
	for(i=0;i<num_bitplanes;i++)
		{
		for(j=0;j<lt->total_values;j++)
			{
			*val_tmp = vzt_rd_get_32r(val_tmp, 0);
			val_tmp++;
			}
		}
	}

pnt = (char *)(b->vindex + num_bitplanes * lt->total_values);
b->num_str_entries = vzt_rd_get_v32(&pnt);

if(b->num_str_entries)
	{
	b->sindex = calloc(b->num_str_entries, sizeof(char *));
	for(i=0;i<b->num_str_entries;i++)
		{
		b->sindex[i] = pnt;
		pnt += (strlen(pnt) + 1);
		}
	}


num_dict_words = (num_sections * num_dict_entries) * sizeof(vztint32_t);
change_dict = malloc(num_dict_words ? num_dict_words : sizeof(vztint32_t)); /* scan-build */
m = 0;
for(i=0;i<num_dict_entries;i++)
	{
	vztint32_t pbit = 0;
	for(j=0;j<num_sections;j++)
		{
                vztint32_t k = val_dict[m];
                vztint32_t l = k^((k<<1)^pbit);
		change_dict[m++] = l;
		pbit = k >> 31;
		}
	}

b->val_dict = val_dict;
b->change_dict = change_dict;
b->times = times;
b->num_time_ticks = num_time_ticks;
b->num_dict_entries = num_dict_entries;
b->num_sections = num_sections;
}

vzt_rd_pthread_mutex_unlock(lt, &b->mutex);
}


static int vzt_rd_block_vch_free(struct vzt_rd_trace *lt, struct vzt_rd_block *b, int killed)
{
vzt_rd_pthread_mutex_lock(lt, &b->mutex);

if(killed) b->killed = killed;	/* never allocate ever again (in case we prefetch on process kill) */

if((b->rle) && (b->val_dict))
	{
	free(b->val_dict); b->val_dict = NULL;

	vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
	lt->block_mem_consumed -= b->num_rle_bytes;
	vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);
	}

if(b->mem) { free(b->mem); b->mem = NULL; }
if(b->change_dict) { free(b->change_dict); b->change_dict = NULL; }
if(b->times) { free(b->times); b->times = NULL; }
if(b->sindex) { free(b->sindex); b->sindex = NULL; }

vzt_rd_pthread_mutex_unlock(lt, &b->mutex);
return(1);
}


vztint32_t vzt_rd_next_value_chg_time(struct vzt_rd_trace *lt, struct vzt_rd_block *b, vztint32_t time_offset, vztint32_t facidx)
{
unsigned int i;
vztint32_t len = lt->len[facidx];
vztint32_t vindex_offset = lt->vindex_offset[facidx];
vztint32_t vindex_offset_x = vindex_offset + lt->total_values;
vztint32_t old_time_offset = time_offset;
int word = time_offset / 32;
int bit  = (time_offset & 31) + 1;
int row_size = b->num_sections;
vztint32_t *valpnt, *valpnt_x;
vztint32_t change_msk;

if((time_offset>=(b->num_time_ticks-1))||(facidx>lt->numrealfacs)) return(time_offset);

time_offset &= ~31;

for(;word<row_size;word++)
	{
	if(bit != 32)
		{
		change_msk = 0;

		if(!(lt->flags[facidx]&VZT_RD_SYM_F_SYNVEC))
			{
			if(b->multi_state)
				{
				for(i=0;i<len;i++)
					{
					valpnt = b->change_dict + (b->vindex[vindex_offset+i] * row_size + word);
					valpnt_x = b->change_dict + (b->vindex[vindex_offset_x+i] * row_size + word);
					change_msk |= *valpnt;
					change_msk |= *valpnt_x;
					}
				}
				else
				{
				for(i=0;i<len;i++)
					{
					valpnt = b->change_dict + (b->vindex[vindex_offset+i] * row_size + word);
					change_msk |= *valpnt;
					}
				}
			}
			else
			{
			if(b->multi_state)
				{
				for(i=0;i<len;i++)
					{
					if((facidx+i)>=lt->numfacs) break;

					vindex_offset = lt->vindex_offset[facidx+i];
					vindex_offset_x = vindex_offset + lt->total_values;

					valpnt = b->change_dict + (b->vindex[vindex_offset] * row_size + word);
					valpnt_x = b->change_dict + (b->vindex[vindex_offset_x] * row_size + word);
					change_msk |= *valpnt;
					change_msk |= *valpnt_x;
					}
				}
				else
				{
				for(i=0;i<len;i++)
					{
					if((facidx+i)>=lt->numfacs) break;

					vindex_offset = lt->vindex_offset[facidx+i];

					valpnt = b->change_dict + (b->vindex[vindex_offset] * row_size + word);
					change_msk |= *valpnt;
					}
				}
			}

		change_msk >>= bit;
		if(change_msk)
			{
			return( (change_msk & 1 ? 0 : vzt_rd_tzc(change_msk)) + time_offset + bit );
			}
		}

	time_offset += 32;
	bit = 0;
	}

return(old_time_offset);
}

int vzt_rd_fac_value(struct vzt_rd_trace *lt, struct vzt_rd_block *b, vztint32_t time_offset, vztint32_t facidx, char *value)
{
vztint32_t len = lt->len[facidx];
unsigned int i;
int word = time_offset / 32;
int bit  = time_offset & 31;
int row_size = b->num_sections;
vztint32_t *valpnt;
vztint32_t *val_base;

if((time_offset>b->num_time_ticks)||(facidx>lt->numrealfacs)) return(0);

val_base = b->val_dict + word;

if(!(lt->flags[facidx]&VZT_RD_SYM_F_SYNVEC))
	{
	vztint32_t vindex_offset = lt->vindex_offset[facidx];

	if(b->multi_state)
		{
		vztint32_t vindex_offset_x = vindex_offset + lt->total_values;
		vztint32_t *valpnt_x;
		int which;

		for(i=0;i<len;i++)
			{
			valpnt   = val_base + (b->vindex[vindex_offset++] * row_size);
			valpnt_x = val_base + (b->vindex[vindex_offset_x++] * row_size);

			which = (((*valpnt_x >> bit) & 1) << 1) | ((*valpnt >> bit) & 1);
			value[i] = "01xz"[which];
			}
		}
		else
		{
		for(i=0;i<len;i++)
			{
			valpnt = val_base + (b->vindex[vindex_offset++] * row_size);
			value[i] = '0' | ((*valpnt >> bit) & 1);
			}
		}
	}
	else
	{
	vztint32_t vindex_offset;

	if(b->multi_state)
		{
		vztint32_t vindex_offset_x;
		vztint32_t *valpnt_x;
		int which;

		for(i=0;i<len;i++)
			{
			if((facidx+i)>=lt->numfacs) break;

			vindex_offset = lt->vindex_offset[facidx+i];
			vindex_offset_x = vindex_offset + lt->total_values;

			valpnt   = val_base + (b->vindex[vindex_offset] * row_size);
			valpnt_x = val_base + (b->vindex[vindex_offset_x] * row_size);

			which = (((*valpnt_x >> bit) & 1) << 1) | ((*valpnt >> bit) & 1);
			value[i] = "01xz"[which];
			}
		}
		else
		{
		for(i=0;i<len;i++)
			{
			if((facidx+i)>=lt->numfacs) break;

			vindex_offset = lt->vindex_offset[facidx+i];

			valpnt = val_base + (b->vindex[vindex_offset] * row_size);
			value[i] = '0' | ((*valpnt >> bit) & 1);
			}
		}
	}
value[i] = 0;

return(1);
}

static void vzt_rd_double_xdr(char *pnt, char *buf)
{
int j;
#if HAVE_RPC_XDR_H
XDR x;
#else
const vztint32_t endian_matchword = 0x12345678;
#endif
double d;
char xdrdata[8] = { 0,0,0,0,0,0,0,0 }; /* scan-build */

for(j=0;j<64;j++)
	{
	int byte = j/8;
	int bit = 7-(j&7);
	if(pnt[j]=='1')
		{
		xdrdata[byte] |= (1<<bit);
		}
		else
		{
		xdrdata[byte] &= ~(1<<bit);
		}
	}

#if HAVE_RPC_XDR_H
xdrmem_create(&x, xdrdata, sizeof(xdrdata), XDR_DECODE);
xdr_double(&x, &d);
#else
/* byte ordering in windows is reverse of XDR (on x86, that is) */
if(*((char *)&endian_matchword) == 0x78)
	{
	for(j=0;j<8;j++)
		{
		((char *)&d)[j] = xdrdata[7-j];
		}
	}
	else
	{
	memcpy(&d, xdrdata, sizeof(double)); /* big endian, don't bytereverse */
	}
#endif

sprintf(buf, "%.16g", d);
}


static unsigned int vzt_rd_make_sindex(char *pnt)
{
unsigned int spnt=0, bpos;

for(bpos=0;bpos<32;bpos++)
	{
	spnt<<=1;
	spnt|=(pnt[bpos]&1);
	}
return(spnt);
}


/*
 * exploit locality of reference for when monotonic time per fac is needed
 * (gtkwave) rather than monotonic time ordering over the whole trace
 * (converting to vcd)
 */
int vzt_rd_process_block_linear(struct vzt_rd_trace *lt, struct vzt_rd_block *b)
{
int i, i2;
vztint32_t idx;
char *pnt=lt->value_current_sector, *pnt2=lt->value_previous_sector;
char buf[32];
char *bufpnt;

vzt_rd_block_vch_decode(lt, b);
vzt_rd_pthread_mutex_lock(lt, &b->mutex);

for(idx=0;idx<lt->numrealfacs;idx++)
        {
        int process_idx = idx/8;
        int process_bit = idx&7;

        if(lt->process_mask[process_idx]&(1<<process_bit))
                {
		i = 0;
		for(;;)	{
			vzt_rd_fac_value(lt, b, i, idx, pnt);

			if((!i)&&(b->prev)&&(!b->prev->exclude_block))
				{
				vzt_rd_fac_value(lt, b->prev, b->prev->num_time_ticks - 1, idx, pnt2); /* get last val of prev sector */
				if(strcmp(pnt, pnt2))
					{
					goto do_vch;
					}
				}
				else
				{
do_vch:				if(!(lt->flags[idx] & (VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
					{
					lt->value_change_callback(&lt, &b->times[i], &idx, &pnt);
					}
					else
					{
					if(lt->flags[idx] & VZT_RD_SYM_F_DOUBLE)
						{
						bufpnt = buf;
						vzt_rd_double_xdr(pnt, buf);
						lt->value_change_callback(&lt, &b->times[i], &idx, &bufpnt);
						}
					else
						{
						unsigned int spnt=vzt_rd_make_sindex(pnt);
						char *msg = ((!i)&(!b->prev)) ? "UNDEF" : b->sindex[spnt];
						lt->value_change_callback(&lt, &b->times[i], &idx, &msg);
						}
					}
				}

			i2 = vzt_rd_next_value_chg_time(lt, b, i, idx);
			if(i==i2) break;
			i=i2;
			}
		}
	}

vzt_rd_pthread_mutex_unlock(lt, &b->mutex);
return(1);
}

/*
 * most clients want this
 */
int vzt_rd_process_block(struct vzt_rd_trace *lt, struct vzt_rd_block *b)
{
unsigned int i, i2;
vztint32_t idx;
char *pnt=lt->value_current_sector, *pnt2=lt->value_previous_sector;
char buf[32];
char *bufpnt;

struct vzt_ncycle_autosort **autosort;
struct vzt_ncycle_autosort *deadlist=NULL;
struct vzt_ncycle_autosort *autofacs= calloc(lt->numrealfacs ? lt->numrealfacs : 1, sizeof(struct vzt_ncycle_autosort)); /* fix for scan-build on lt->numrealfacs */

vzt_rd_block_vch_decode(lt, b);
vzt_rd_pthread_mutex_lock(lt, &b->mutex);

autosort = calloc(b->num_time_ticks, sizeof(struct vzt_ncycle_autosort *));
for(i=0;i<b->num_time_ticks;i++) autosort[i]=NULL;
deadlist=NULL;

for(idx=0;idx<lt->numrealfacs;idx++)
        {
        int process_idx = idx/8;
        int process_bit = idx&7;

        if(lt->process_mask[process_idx]&(1<<process_bit))
                {
		i = 0;

		vzt_rd_fac_value(lt, b, i, idx, pnt);

		if((!i)&&(b->prev)&&(!b->prev->exclude_block))
			{
			vzt_rd_fac_value(lt, b->prev, b->prev->num_time_ticks - 1, idx, pnt2); /* get last val of prev sector */
			if(strcmp(pnt, pnt2))
				{
				goto do_vch_0;
				}
			}
			else
			{
do_vch_0:		if(!(lt->flags[idx] & (VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
				{
				lt->value_change_callback(&lt, &b->times[i], &idx, &pnt);
				}
				else
				{
				if(lt->flags[idx] & VZT_RD_SYM_F_DOUBLE)
					{
					bufpnt = buf;
					vzt_rd_double_xdr(pnt, buf);
					lt->value_change_callback(&lt, &b->times[i], &idx, &bufpnt);
					}
				else
					{
					unsigned int spnt=vzt_rd_make_sindex(pnt);
					char *msg = ((!i)&(!b->prev)) ? "UNDEF" : b->sindex[spnt];
					lt->value_change_callback(&lt, &b->times[i], &idx, &msg);
					}
				}
			}

		i2 = vzt_rd_next_value_chg_time(lt, b, i, idx);
                if(i2)
                        {
                        struct vzt_ncycle_autosort *t = autosort[i2];

                        autofacs[idx].next = t;
                        autosort[i2] = autofacs+idx;
                        }
                        else
                        {
                        struct vzt_ncycle_autosort *t = deadlist;
                        autofacs[idx].next = t;
                        deadlist = autofacs+idx;
                        }
		}
	}

for(i = 1; i < b->num_time_ticks; i++)
        {
        struct vzt_ncycle_autosort *t = autosort[i];

        if(t)
                {
                while(t)
                        {
                        struct vzt_ncycle_autosort *tn = t->next;

                        idx = t-autofacs;

			vzt_rd_fac_value(lt, b, i, idx, pnt);
			if(!(lt->flags[idx] & (VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
				{
				lt->value_change_callback(&lt, &b->times[i], &idx, &pnt);
				}
				else
				{
				if(lt->flags[idx] & VZT_RD_SYM_F_DOUBLE)
					{
					bufpnt = buf;
					vzt_rd_double_xdr(pnt, buf);
					lt->value_change_callback(&lt, &b->times[i], &idx, &bufpnt);
					}
				else
					{
					unsigned int spnt=vzt_rd_make_sindex(pnt);
					char *msg = ((!i)&(!b->prev)) ? "UNDEF" : b->sindex[spnt];
					lt->value_change_callback(&lt, &b->times[i], &idx, &msg);
					}
				}

			i2 = vzt_rd_next_value_chg_time(lt, b, i, idx);

                        if(i2!=i)
                                {
                                struct vzt_ncycle_autosort *ta = autosort[i2];

                                autofacs[idx].next = ta;
                                autosort[i2] = autofacs+idx;
                                }
                                else
                                {
                                struct vzt_ncycle_autosort *ta = deadlist;
                                autofacs[idx].next = ta;
                                deadlist = autofacs+idx;
                                }

                        t = tn;
                        }
                }
        }

vzt_rd_pthread_mutex_unlock(lt, &b->mutex);

free(autofacs);
free(autosort);

return(1);
}

/****************************************************************************/

/*
 * null callback used when a user passes NULL as an argument to vzt_rd_iter_blocks()
 */
void vzt_rd_null_callback(struct vzt_rd_trace **lt, vztint64_t *pnt_time, vztint32_t *pnt_facidx, char **pnt_value)
{
(void) lt;
(void) pnt_time;
(void) pnt_facidx;
(void) pnt_value;

/* fprintf(stderr, VZT_RDLOAD"%lld %d %s\n", *pnt_time, *pnt_facidx, *pnt_value); */
}

/****************************************************************************/

/*
 * return number of facs in trace
 */
_VZT_RD_INLINE vztint32_t vzt_rd_get_num_facs(struct vzt_rd_trace *lt)
{
return(lt ? lt->numfacs : 0);
}


/*
 * return fac geometry for a given index
 */
struct vzt_rd_geometry *vzt_rd_get_fac_geometry(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	lt->geometry.rows = lt->rows[facidx];
	lt->geometry.msb = lt->msb[facidx];
	lt->geometry.lsb = lt->lsb[facidx];
	lt->geometry.flags = lt->flags[facidx];
	lt->geometry.len = lt->len[facidx];
	return(&lt->geometry);
	}
	else
	{
	return(NULL);
	}
}


/*
 * return partial fac geometry for a given index
 */
_VZT_RD_INLINE vztint32_t vzt_rd_get_fac_rows(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	return(lt->rows[facidx]);
	}
	else
	{
	return(0);
	}
}


_VZT_RD_INLINE vztsint32_t vzt_rd_get_fac_msb(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	return(lt->msb[facidx]);
	}
	else
	{
	return(0);
	}
}


_VZT_RD_INLINE vztsint32_t vzt_rd_get_fac_lsb(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	return(lt->lsb[facidx]);
	}
	else
	{
	return(0);
	}
}


_VZT_RD_INLINE vztint32_t vzt_rd_get_fac_flags(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	return(lt->flags[facidx]);
	}
	else
	{
	return(0);
	}
}


_VZT_RD_INLINE vztint32_t vzt_rd_get_fac_len(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	return(lt->len[facidx]);
	}
	else
	{
	return(0);
	}
}


_VZT_RD_INLINE vztint32_t vzt_rd_get_alias_root(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if((lt)&&(facidx<lt->numfacs))
	{
	while(lt->flags[facidx] & VZT_RD_SYM_F_ALIAS)
		{
		facidx = lt->rows[facidx];	/* iterate to next alias */
		}
	return(facidx);
	}
	else
	{
	return(~((vztint32_t)0));
	}
}


/*
 * time queries
 */
_VZT_RD_INLINE vztint64_t vzt_rd_get_start_time(struct vzt_rd_trace *lt)
{
return(lt ? lt->start : 0);
}


_VZT_RD_INLINE vztint64_t vzt_rd_get_end_time(struct vzt_rd_trace *lt)
{
return(lt ? lt->end : 0);
}


_VZT_RD_INLINE char vzt_rd_get_timescale(struct vzt_rd_trace *lt)
{
return(lt ? lt->timescale : 0);
}


_VZT_RD_INLINE vztsint64_t vzt_rd_get_timezero(struct vzt_rd_trace *lt)
{
return(lt ? lt->timezero : 0);
}


/*
 * extract facname from prefix-compressed table.  this
 * performs best when extracting facs with monotonically
 * increasing indices...
 */
char *vzt_rd_get_facname(struct vzt_rd_trace *lt, vztint32_t facidx)
{
char *pnt;
unsigned int clonecnt, j;

if(lt)
	{
	if((facidx==(lt->faccache->old_facidx+1))||(!facidx))
		{
		if(!facidx)
			{
			lt->faccache->n = lt->zfacnames;
			lt->faccache->bufcurr[0] = 0;
			lt->faccache->bufprev[0] = 0;
			}

		if(facidx!=lt->numfacs)
			{
			pnt = lt->faccache->bufcurr;
			lt->faccache->bufcurr = lt->faccache->bufprev;
			lt->faccache->bufprev = pnt;

			clonecnt=vzt_rd_get_16(lt->faccache->n, 0);  lt->faccache->n+=2;
			pnt=lt->faccache->bufcurr;

			for(j=0;j<clonecnt;j++)
				{
				*(pnt++) = lt->faccache->bufprev[j];
				}

			while((*(pnt++)=vzt_rd_get_byte(lt->faccache->n++,0)));
			lt->faccache->old_facidx = facidx;
			return(lt->faccache->bufcurr);
			}
			else
			{
			return(NULL);	/* no more left */
			}
		}
		else
		{
		if(facidx<lt->numfacs)
			{
			int strt;

			if(facidx==lt->faccache->old_facidx)
				{
				return(lt->faccache->bufcurr);
				}

			if(facidx>(lt->faccache->old_facidx+1))
				{
				strt = lt->faccache->old_facidx+1;
				}
				else
				{
				strt=0;
				}

			for(j=strt;j<facidx;j++)
				{
				vzt_rd_get_facname(lt, j);
				}

			return(vzt_rd_get_facname(lt, j));
			}
		}
	}

return(NULL);
}


/*
 * iter mask manipulation util functions
 */
_VZT_RD_INLINE int vzt_rd_get_fac_process_mask(struct vzt_rd_trace *lt, vztint32_t facidx)
{
if(lt)
	{
	if(facidx<lt->numfacs)
		{
		int process_idx = facidx/8;
		int process_bit = facidx&7;

		return( (lt->process_mask[process_idx]&(1<<process_bit)) != 0 );
		}
	}
return(0);
}


_VZT_RD_INLINE int vzt_rd_set_fac_process_mask(struct vzt_rd_trace *lt, vztint32_t facidx)
{
int rc=0;

if(lt)
	{
	if(facidx<lt->numfacs)
		{
		int idx = facidx/8;
		int bitpos = facidx&7;

		if(!(lt->flags[facidx]&VZT_RD_SYM_F_ALIAS))
			{
			lt->process_mask[idx] |= (1<<bitpos);
			}
			else
			{
			lt->process_mask[idx] &= (~(1<<bitpos));
			}
		}
	rc=1;
	}

return(rc);
}


_VZT_RD_INLINE int vzt_rd_clr_fac_process_mask(struct vzt_rd_trace *lt, vztint32_t facidx)
{
int rc=0;

if(lt)
	{
	if(facidx<lt->numfacs)
		{
		int idx = facidx/8;
		int bitpos = facidx&7;

		lt->process_mask[idx] &= (~(1<<bitpos));
		}
	rc=1;
	}

return(rc);
}


_VZT_RD_INLINE int vzt_rd_set_fac_process_mask_all(struct vzt_rd_trace *lt)
{
int rc=0;
unsigned int i;

if(lt)
	{
	memset(lt->process_mask, 0xff, (lt->numfacs+7)/8);
	rc=1;

	for(i=0;i<lt->numfacs;i++)
		{
		if((!lt->len[i])||(lt->flags[i]&VZT_RD_SYM_F_ALIAS)) vzt_rd_clr_fac_process_mask(lt, i);
		}
	}

return(rc);
}


_VZT_RD_INLINE int vzt_rd_clr_fac_process_mask_all(struct vzt_rd_trace *lt)
{
int rc=0;

if(lt)
	{
	memset(lt->process_mask, 0x00, (lt->numfacs+7)/8);
	rc=1;
	}

return(rc);
}


/*
 * block memory set/get used to control buffering
 */
_VZT_RD_INLINE vztint64_t vzt_rd_set_max_block_mem_usage(struct vzt_rd_trace *lt, vztint64_t block_mem_max)
{
vztint64_t rc = lt->block_mem_max;
lt->block_mem_max = block_mem_max;
return(rc);
}


vztint64_t vzt_rd_get_block_mem_usage(struct vzt_rd_trace *lt)
{
vztint64_t mem;

vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
mem = lt->block_mem_consumed;
vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);

return(mem);
}


/*
 * return total number of blocks
 */
_VZT_RD_INLINE unsigned int vzt_rd_get_num_blocks(struct vzt_rd_trace *lt)
{
return(lt->numblocks);
}

/*
 * return number of active blocks
 */
unsigned int vzt_rd_get_num_active_blocks(struct vzt_rd_trace *lt)
{
int blk=0;

if(lt)
	{
	struct vzt_rd_block *b = lt->block_head;

	while(b)
		{
		if((!b->short_read_ignore)&&(!b->exclude_block))
			{
			blk++;
			}

		b=b->next;
		}
	}

return(blk);
}

/****************************************************************************/

static int vzt_rd_det_gzip_type(FILE *handle)
{
unsigned char cbuf[2] = { 0, 0 };

off_t off = ftello(handle);
if(!fread(cbuf, 1, 2, handle)) { cbuf[0] = cbuf[1] = 0; }
fseeko(handle, off, SEEK_SET);

if((cbuf[0] == 0x1f) && (cbuf[1] == 0x8b))
	{
	return(VZT_RD_IS_GZ);
	}

if((cbuf[0] == 'z') && (cbuf[1] == '7'))
	{
	return(VZT_RD_IS_LZMA);
	}

return(VZT_RD_IS_BZ2);
}

/****************************************************************************/

static void vzt_rd_decompress_blk(struct vzt_rd_trace *lt, struct vzt_rd_block *b, int reopen)
{
unsigned int rc;
void *zhandle;
FILE *handle;
if(reopen)
	{
	handle = fopen(lt->filename, "rb");
	}
	else
	{
	handle = lt->handle;
	}
fseeko(handle, b->filepos, SEEK_SET);

vzt_rd_pthread_mutex_lock(lt, &b->mutex);

if((!b->killed)&&(!b->mem))
	{
	b->mem = malloc(b->uncompressed_siz);

	switch(b->ztype)
		{
		case VZT_RD_IS_GZ:
			zhandle = gzdopen(dup(fileno(handle)), "rb");
			rc=gzread(zhandle, b->mem, b->uncompressed_siz);
			gzclose(zhandle);
			break;

		case VZT_RD_IS_BZ2:
			zhandle = BZ2_bzdopen(dup(fileno(handle)), "rb");
			rc=BZ2_bzread(zhandle, b->mem, b->uncompressed_siz);
			BZ2_bzclose(zhandle);
			break;

		case VZT_RD_IS_LZMA:
		default:
			zhandle = LZMA_fdopen(dup(fileno(handle)), "rb");
			rc=LZMA_read(zhandle, b->mem, b->uncompressed_siz);
			LZMA_close(zhandle);
			break;
		}
	if(rc!=b->uncompressed_siz)
		{
		fprintf(stderr, VZT_RDLOAD"short read on block %p %d vs "VZT_RD_LD" (exp), ignoring\n", (void *)b, rc, b->uncompressed_siz);
		free(b->mem); b->mem=NULL;
		b->short_read_ignore = 1;
		}
		else
		{
		vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
		lt->block_mem_consumed += b->uncompressed_siz;
		vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);
		}
	}

vzt_rd_pthread_mutex_unlock(lt, &b->mutex);

if(reopen)
	{
	fclose(handle);
	}
}


static void *vzt_rd_decompress_blk_pth_actual(void *args)
{
struct vzt_pth_args *vpa = (struct vzt_pth_args *)args;
vzt_rd_decompress_blk(vpa->lt, vpa->b, 1);
vzt_rd_block_vch_decode(vpa->lt, vpa->b);
free(vpa);

return(NULL);
}

static void vzt_rd_decompress_blk_pth(struct vzt_rd_trace *lt, struct vzt_rd_block *b)
{
struct vzt_pth_args *vpa = malloc(sizeof(struct vzt_pth_args));
vpa->lt = lt;
vpa->b = b;

vzt_rd_pthread_create(lt, &b->pth, &b->pth_attr, vzt_rd_decompress_blk_pth_actual, vpa); /* cppcheck misfires thinking vpa is not freed even though vzt_rd_decompress_blk_pth_actual() does it */
}

/*
 * block iteration...purge/reload code here isn't sophisticated as it
 * merely caches the FIRST set of blocks which fit in lt->block_mem_max.
 * n.b., returns number of blocks processed
 */
int vzt_rd_iter_blocks(struct vzt_rd_trace *lt,
	void (*value_change_callback)(struct vzt_rd_trace **lt, vztint64_t *time, vztint32_t *facidx, char **value),
	void *user_callback_data_pointer)
{
struct vzt_rd_block *b, *bpre;
int blk=0, blkfinal=0;
int processed = 0;
struct vzt_rd_block *bcutoff=NULL, *bfinal=NULL;

if(lt)
	{
	lt->value_change_callback = value_change_callback ? value_change_callback : vzt_rd_null_callback;
	lt->user_callback_data_pointer = user_callback_data_pointer;

	b = lt->block_head;
	blk=0;

	while(b)
		{
		if((!b->mem)&&(!b->short_read_ignore)&&(!b->exclude_block))
			{
			if(processed<5)
				{
				int gate = (processed==4) && b->next;

				fprintf(stderr, VZT_RDLOAD"block [%d] processing "VZT_RD_LLD" / "VZT_RD_LLD"%s\n", blk, b->start, b->end, gate ? " ..." : "");
				if(gate) { bcutoff = b; }
				}

			processed++;

			if(lt->pthreads)
				{
				int count = lt->pthreads;
				/* prefetch next *empty* block(s) on an alternate thread */
				bpre = b->next;
				while(bpre)
					{
					if((!bpre->mem)&&(!bpre->short_read_ignore)&&(!bpre->exclude_block))
						{
						vzt_rd_decompress_blk_pth(lt, bpre);
						count--;
						if(!count) break;
						}
					bpre = bpre->next;
					}
				}

			vzt_rd_decompress_blk(lt, b, 0);
			bfinal=b;
			blkfinal = blk;
			}

		if(b->mem)
			{
			if(lt->process_linear)
				{
				vzt_rd_process_block_linear(lt, b);
				}
				else
				{
				vzt_rd_process_block(lt, b);
				}

			if(lt->numblocks > 2)	/* no sense freeing up when not so many blocks */
				{
				vztint64_t block_mem_consumed;

				vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
				block_mem_consumed = lt->block_mem_consumed;
				vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);

				if(block_mem_consumed > lt->block_mem_max)
					{
					if(b->prev)
						{
						vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
						lt->block_mem_consumed -= b->prev->uncompressed_siz;
						vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);
						vzt_rd_block_vch_free(lt, b->prev, 0);
						}
					}
				}
			}

		blk++;
		b=b->next;
		}
	}

if((bcutoff)&&(bfinal!=bcutoff))
	{
	fprintf(stderr, VZT_RDLOAD"block [%d] processed "VZT_RD_LLD" / "VZT_RD_LLD"\n", blkfinal, bfinal->start, bfinal->end);
	}

return(blk);
}


/*
 * callback access to the user callback data pointer (if required)
 */
_VZT_RD_INLINE void *vzt_rd_get_user_callback_data_pointer(struct vzt_rd_trace *lt)
{
if(lt)
	{
	return(lt->user_callback_data_pointer);
	}
	else
	{
	return(NULL);
	}
}


/*
 * limit access to certain timerange in file
 * and return number of active blocks
 */
unsigned int vzt_rd_limit_time_range(struct vzt_rd_trace *lt, vztint64_t strt_time, vztint64_t end_time)
{
vztint64_t tmp_time;
int blk=0;

if(lt)
	{
	struct vzt_rd_block *b = lt->block_head;
	struct vzt_rd_block *bprev = NULL;
	int state = 0;

	if(strt_time > end_time)
		{
		tmp_time = strt_time;
		strt_time = end_time;
		end_time = tmp_time;
		}

	while(b)
		{
		switch(state)
			{
			case 0: if(b->end >= strt_time)
					{
					state = 1;
					if((b->start > strt_time) && (bprev))
						{
						bprev->exclude_block = 0;
						blk++;
						}
					}
				break;

			case 1: if(b->start > end_time) state = 2;
				break;

			default: break;
			}

		if((state==1) && (!b->short_read_ignore))
			{
			b->exclude_block = 0;
			blk++;
			}
			else
			{
			b->exclude_block = 1;
			}

		bprev = b;
		b = b->next;
		}

	}

return(blk);
}

/*
 * unrestrict access to the whole file
 * and return number of active blocks
 */
unsigned int vzt_rd_unlimit_time_range(struct vzt_rd_trace *lt)
{
int blk=0;

if(lt)
	{
	struct vzt_rd_block *b = lt->block_head;

	while(b)
		{
		b->exclude_block = 0;

		if(!b->short_read_ignore)
			{
			blk++;
			}

		b=b->next;
		}
	}

return(blk);
}


/*
 * mode switch for linear accessing
 */
void vzt_rd_process_blocks_linearly(struct vzt_rd_trace *lt, int doit)
{
if(lt)
	{
	lt->process_linear = (doit != 0);
	}
}

/****************************************************************************/

/*
 * initialize the trace, get compressed facnames, get geometries,
 * and get block offset/size/timestart/timeend...
 */
struct vzt_rd_trace *vzt_rd_init_smp(const char *name, unsigned int num_cpus)
{
struct vzt_rd_trace *lt=(struct vzt_rd_trace *)calloc(1, sizeof(struct vzt_rd_trace));
unsigned int i;
unsigned int vindex_offset;

if(!(lt->handle=fopen(name, "rb")))
        {
	vzt_rd_close(lt);
        lt=NULL;
        }
        else
	{
	vztint16_t id = 0, version = 0;

	lt->filename = strdup(name);
	lt->block_mem_max = VZT_RD_MAX_BLOCK_MEM_USAGE;    /* cutoff after this number of bytes and force flush */

	if(num_cpus<1) num_cpus = 1;
	if(num_cpus>8) num_cpus = 8;
	lt->pthreads = num_cpus - 1;

	setvbuf(lt->handle, (char *)NULL, _IONBF, 0);	/* keeps gzip from acting weird in tandem with fopen */

	if(!fread(&id, 2, 1, lt->handle)) { id = 0; }
	if(!fread(&version, 2, 1, lt->handle)) { id = 0; }
	if(!fread(&lt->granule_size, 1, 1, lt->handle)) { id = 0; }

	if(vzt_rd_get_16(&id,0) != VZT_RD_HDRID)
		{
		fprintf(stderr, VZT_RDLOAD"*** Not a vzt file ***\n");
		vzt_rd_close(lt);
	        lt=NULL;
		}
	else
	if((version=vzt_rd_get_16(&version,0)) > VZT_RD_VERSION)
		{
		fprintf(stderr, VZT_RDLOAD"*** Version %d vzt not supported ***\n", version);
		vzt_rd_close(lt);
	        lt=NULL;
		}
	else
	if(lt->granule_size > VZT_RD_GRANULE_SIZE)
		{
		fprintf(stderr, VZT_RDLOAD"*** Granule size of %d (>%d) not supported ***\n", lt->granule_size, VZT_RD_GRANULE_SIZE);
		vzt_rd_close(lt);
	        lt=NULL;
		}
	else
		{
		size_t rcf;
		unsigned int rc;
		char *m;
		off_t pos, fend;
		unsigned int t;
		struct vzt_rd_block *b;

		vzt_rd_pthread_mutex_init(lt, &lt->mutex, NULL);

		rcf = fread(&lt->numfacs, 4, 1, lt->handle);		lt->numfacs = rcf ? vzt_rd_get_32(&lt->numfacs,0) : 0;

		if(!lt->numfacs)
			{
			vztint32_t num_expansion_bytes;

			rcf = fread(&num_expansion_bytes, 4, 1, lt->handle); num_expansion_bytes = rcf ? vzt_rd_get_32(&num_expansion_bytes,0) : 0;
			rcf = fread(&lt->numfacs, 4, 1, lt->handle); lt->numfacs = rcf ? vzt_rd_get_32(&lt->numfacs,0) : 0;
			if(num_expansion_bytes >= 8)
				{
				rcf = fread(&lt->timezero, 8, 1, lt->handle); lt->timezero = rcf ? vzt_rd_get_64(&lt->timezero,0) : 0;
				if(num_expansion_bytes > 8)
					{
					/* future version? */
					fseeko(lt->handle, num_expansion_bytes - 8, SEEK_CUR);
					}
				}
				else
				{
				/* malformed */
				fseeko(lt->handle, num_expansion_bytes, SEEK_CUR);
				}
			}

		rcf = fread(&lt->numfacbytes, 4, 1, lt->handle); lt->numfacbytes = rcf ? vzt_rd_get_32(&lt->numfacbytes,0) : 0;
		rcf = fread(&lt->longestname, 4, 1, lt->handle);	lt->longestname = rcf ? vzt_rd_get_32(&lt->longestname,0) : 0;
		rcf = fread(&lt->zfacnamesize, 4, 1, lt->handle);	lt->zfacnamesize = rcf ? vzt_rd_get_32(&lt->zfacnamesize,0) : 0;
		rcf = fread(&lt->zfacname_predec_size, 4, 1, lt->handle);lt->zfacname_predec_size = rcf ? vzt_rd_get_32(&lt->zfacname_predec_size,0) : 0;
		rcf = fread(&lt->zfacgeometrysize, 4, 1, lt->handle);	lt->zfacgeometrysize = rcf ? vzt_rd_get_32(&lt->zfacgeometrysize,0) : 0;
		rcf = fread(&lt->timescale, 1, 1, lt->handle);		if(!rcf) lt->timescale = 0; /* no swap necessary */

		fprintf(stderr, VZT_RDLOAD VZT_RD_LD" facilities\n", lt->numfacs);
		pos = ftello(lt->handle);
		/* fprintf(stderr, VZT_RDLOAD"gzip facnames start at pos %d (zsize=%d)\n", pos, lt->zfacnamesize); */

		lt->process_mask = calloc(1, lt->numfacs/8+1);

		switch(vzt_rd_det_gzip_type(lt->handle))
			{
			case VZT_RD_IS_GZ:
				lt->zhandle = gzdopen(dup(fileno(lt->handle)), "rb");
				m=(char *)malloc(lt->zfacname_predec_size);
				rc=gzread(lt->zhandle, m, lt->zfacname_predec_size);
				gzclose(lt->zhandle); lt->zhandle=NULL;
				break;

			case VZT_RD_IS_BZ2:
				lt->zhandle = BZ2_bzdopen(dup(fileno(lt->handle)), "rb");
				m=(char *)malloc(lt->zfacname_predec_size);
				rc=BZ2_bzread(lt->zhandle, m, lt->zfacname_predec_size);
				BZ2_bzclose(lt->zhandle); lt->zhandle=NULL;
				break;

			case VZT_RD_IS_LZMA:
			default:
				lt->zhandle = LZMA_fdopen(dup(fileno(lt->handle)), "rb");
				m=(char *)malloc(lt->zfacname_predec_size);
				rc=LZMA_read(lt->zhandle, m, lt->zfacname_predec_size);
				LZMA_close(lt->zhandle); lt->zhandle=NULL;
				break;
			}

		if(rc!=lt->zfacname_predec_size)
			{
			fprintf(stderr, VZT_RDLOAD"*** name section mangled %d (act) vs "VZT_RD_LD" (exp)\n", rc, lt->zfacname_predec_size);
			free(m);

			vzt_rd_close(lt);
		        lt=NULL;
			return(lt);
			}

		lt->zfacnames = m;

                lt->faccache = calloc(1, sizeof(struct vzt_rd_facname_cache));
                lt->faccache->old_facidx = lt->numfacs;   /* causes vzt_rd_get_facname to initialize its unroll ptr as this is always invalid */
                lt->faccache->bufcurr = malloc(lt->longestname+1);
                lt->faccache->bufprev = malloc(lt->longestname+1);

		fseeko(lt->handle, pos = pos+lt->zfacnamesize, SEEK_SET);
		/* fprintf(stderr, VZT_RDLOAD"seeking to geometry at %d (0x%08x)\n", pos, pos); */

		switch(vzt_rd_det_gzip_type(lt->handle))
			{
			case VZT_RD_IS_GZ:
				lt->zhandle = gzdopen(dup(fileno(lt->handle)), "rb");
				t = lt->numfacs * 4 * sizeof(vztint32_t);
				m=(char *)malloc(t);
				rc=gzread(lt->zhandle, m, t);
				gzclose(lt->zhandle); lt->zhandle=NULL;
				break;

			case VZT_RD_IS_BZ2:
				lt->zhandle = BZ2_bzdopen(dup(fileno(lt->handle)), "rb");
				t = lt->numfacs * 4 * sizeof(vztint32_t);
				m=(char *)malloc(t);
				rc=BZ2_bzread(lt->zhandle, m, t);
				BZ2_bzclose(lt->zhandle); lt->zhandle=NULL;
				break;

			case VZT_RD_IS_LZMA:
			default:
				lt->zhandle = LZMA_fdopen(dup(fileno(lt->handle)), "rb");
				t = lt->numfacs * 4 * sizeof(vztint32_t);
				m=(char *)malloc(t);
				rc=LZMA_read(lt->zhandle, m, t);
				LZMA_close(lt->zhandle); lt->zhandle=NULL;
				break;
			}

		if(rc!=t)
			{
			fprintf(stderr, VZT_RDLOAD"*** geometry section mangled %d (act) vs %d (exp)\n", rc, t);
			free(m);

			vzt_rd_close(lt);
		        lt=NULL;
			return(lt);
			}

		pos = pos+lt->zfacgeometrysize;

		lt->rows = malloc(lt->numfacs * sizeof(vztint32_t));
		lt->msb = malloc(lt->numfacs * sizeof(vztsint32_t));
		lt->lsb = malloc(lt->numfacs * sizeof(vztsint32_t));
		lt->flags = malloc(lt->numfacs * sizeof(vztint32_t));
		lt->len = malloc(lt->numfacs * sizeof(vztint32_t));
		lt->vindex_offset = malloc(lt->numfacs * sizeof(vztint32_t));
		lt->longest_len = 32; /* big enough for decoded double in vzt_rd_process_block_single_factime() */

		for(i=0;i<lt->numfacs;i++)
			{
			int j;

			lt->rows[i] = vzt_rd_get_32(m+i*16, 0);
			lt->msb[i] = vzt_rd_get_32(m+i*16, 4);
			lt->lsb[i] = vzt_rd_get_32(m+i*16, 8);
			lt->flags[i] = vzt_rd_get_32(m+i*16, 12) & VZT_RD_SYM_MASK; /* strip out unsupported bits */

			j = (!(lt->flags[i] & VZT_RD_SYM_F_ALIAS)) ? i : vzt_rd_get_alias_root(lt, i);

			if(!(lt->flags[i] & (VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_STRING|VZT_RD_SYM_F_DOUBLE)))
				{
				lt->len[i] = (lt->msb[j] <= lt->lsb[j]) ? (lt->lsb[j] - lt->msb[j] + 1) : (lt->msb[j] - lt->lsb[j] + 1);
				}
				else
				{
				lt->len[i] = (lt->flags[j] & (VZT_RD_SYM_F_INTEGER|VZT_RD_SYM_F_STRING)) ? 32 : 64;
				}

			if(lt->len[i] > lt->longest_len)
				{
				lt->longest_len = lt->len[i];
				}
			}

		vindex_offset = 0; /* offset in value table */
		for(lt->numrealfacs=0; lt->numrealfacs<lt->numfacs; lt->numrealfacs++)
			{
			if(lt->flags[lt->numrealfacs] & VZT_RD_SYM_F_ALIAS)
				{
				break;
				}
			lt->vindex_offset[lt->numrealfacs] = vindex_offset;
			vindex_offset += lt->len[lt->numrealfacs];
			}
		lt->total_values = vindex_offset;
		fprintf(stderr, VZT_RDLOAD"Total value bits: "VZT_RD_LD"\n", lt->total_values);

		if(lt->numrealfacs > lt->numfacs) lt->numrealfacs = lt->numfacs;

		lt->value_current_sector = malloc(lt->longest_len + 1);
		lt->value_previous_sector = malloc(lt->longest_len + 1);

		free(m);

		for(;;)
			{
			fseeko(lt->handle, 0L, SEEK_END);
			fend=ftello(lt->handle);
			if(pos>=fend) break;

			fseeko(lt->handle, pos, SEEK_SET);
			/* fprintf(stderr, VZT_RDLOAD"seeking to block at %d (0x%08x)\n", pos, pos); */

			b=calloc(1, sizeof(struct vzt_rd_block));
			b->last_rd_value_idx = ~0;

			rcf = fread(&b->uncompressed_siz, 4, 1, lt->handle);	b->uncompressed_siz = rcf ? vzt_rd_get_32(&b->uncompressed_siz,0) : 0;
			rcf = fread(&b->compressed_siz, 4, 1, lt->handle);	b->compressed_siz = rcf ? vzt_rd_get_32(&b->compressed_siz,0) : 0;
			rcf = fread(&b->start, 8, 1, lt->handle);		b->start = rcf ? vzt_rd_get_64(&b->start,0) : 0;
			rcf = fread(&b->end, 8, 1, lt->handle);			b->end = rcf ? vzt_rd_get_64(&b->end,0) : 0;
			pos = ftello(lt->handle);

			if((b->rle = (b->start > b->end)))
				{
				vztint64_t tb = b->start;
				b->start = b->end;
				b->end = tb;
				}

			b->ztype = vzt_rd_det_gzip_type(lt->handle);
			/* fprintf(stderr, VZT_RDLOAD"block gzip start at pos %d (0x%08x)\n", pos, pos); */
			if(pos>=fend)
				{
				free(b);
				break;
				}

			b->filepos = pos; /* mark startpos for later in case we purge it from memory */
			/* fprintf(stderr, VZT_RDLOAD"un/compressed size: %d/%d\n", b->uncompressed_siz, b->compressed_siz); */

			if((b->uncompressed_siz)&&(b->compressed_siz)&&(b->end))
				{
				/* fprintf(stderr, VZT_RDLOAD"block [%d] %lld / %lld\n", lt->numblocks, b->start, b->end); */
				fseeko(lt->handle, b->compressed_siz, SEEK_CUR);

				lt->numblocks++;
				if(lt->numblocks <= lt->pthreads)
					{
					vzt_rd_pthread_mutex_init(lt, &b->mutex, NULL);
					vzt_rd_decompress_blk_pth(lt, b); /* prefetch first block */
					}

				if(lt->block_curr)
					{
					b->prev = lt->block_curr;
					lt->block_curr->next = b;
					lt->block_curr = b;
					lt->end = b->end;
					}
					else
					{
					lt->block_head = lt->block_curr = b;
					lt->start = b->start;
					lt->end = b->end;
					}
				}
				else
				{
				free(b);
				break;
				}

			pos+=b->compressed_siz;
			}

		if(lt->numblocks)
			{
			fprintf(stderr, VZT_RDLOAD"Read %d block header%s OK\n", lt->numblocks, (lt->numblocks!=1) ? "s" : "");

			fprintf(stderr, VZT_RDLOAD"["VZT_RD_LLD"] start time\n", lt->start);
			fprintf(stderr, VZT_RDLOAD"["VZT_RD_LLD"] end time\n", lt->end);
			fprintf(stderr, VZT_RDLOAD"\n");

			lt->value_change_callback = vzt_rd_null_callback;
			}
			else
			{
			vzt_rd_close(lt);
			lt=NULL;
			}
		}
	}

return(lt);
}

/*
 * experimental, only really useful for vztminer right now...
 */
void vzt_rd_vectorize(struct vzt_rd_trace *lt)
{
if((!lt)||(lt->vectorize)||(lt->numfacs<2))
	{
	return;
	}
	else
	{
	unsigned int old_longest_len = lt->longest_len;
	int pmxlen = 31;
	char *pbuff = malloc(pmxlen+1);
	char *pname;
	int plen, plen2;
	unsigned int i;
	int pidx;
	int num_after_combine = lt->numfacs;
	int num_synvecs = 0;
	int num_synalias = 0;
	struct vzt_synvec_chain **synvec_chain = calloc(lt->numfacs, sizeof(struct vzt_synvec_chain *));

	for(i=0;i<lt->numfacs-1;i++)
		{
		unsigned int j;

		if(lt->len[i] != 1) continue;

		pname = vzt_rd_get_facname(lt, i);
		plen = strlen(pname);
		if(plen > pmxlen)
			{
			free(pbuff);
			pbuff = malloc(plen+1);
			}

		memcpy(pbuff, pname, plen);
		pbuff[plen] = 0;
		pidx = lt->msb[i];

		for(j=i+1;j<lt->numfacs-1;j++)
			{
			pname = vzt_rd_get_facname(lt, j);
			plen2 = strlen(pname);
			if((plen != plen2) || (strcmp(pbuff, pname)) || (lt->len[j] != 1) || ((pidx != lt->msb[j]-1) && (pidx != lt->msb[j]+1)))
				{
				i = j-1;
				break;
				}

			pidx = lt->msb[j];
			lt->len[i] += lt->len[j];
			lt->lsb[i] = lt->lsb[j];
			lt->len[j] = 0;
			num_after_combine--;

			if(lt->len[i] > lt->longest_len)
				{
				lt->longest_len = lt->len[i];
				}
			}
		}

	free(pbuff); /* scan-build */

	for(i=lt->numrealfacs;i<lt->numfacs;i++)
		{
		if(lt->flags[i] & VZT_RD_SYM_F_ALIAS)	/* not necessary, only for sanity */
			{
			unsigned int j = vzt_rd_get_alias_root(lt, i);
			unsigned int k, l;

			if(lt->len[i])
				{
				if((lt->len[i]==1) && (lt->len[j]==1))
					{
					/* nothing to do, single bit alias */
					continue;
					}

				if(lt->len[i]==lt->len[j])
					{
					unsigned int nfm1 = lt->numfacs-1;
					if((i != nfm1) && (j != nfm1))
						{
						if(lt->len[i+1] && lt->len[j+1])
							{
							/* nothing to do, multi-bit alias */
							continue;
							}
						}
					}

				if(1)	/* seems like this doesn't need to be conditional (for now) */
					{
					lt->vindex_offset[i] = lt->vindex_offset[j];
					for(k=1;k<lt->len[i];k++)
						{
						if((k+i) <= (lt->numfacs-1))
							{
							lt->vindex_offset[k+i] = lt->vindex_offset[vzt_rd_get_alias_root(lt, k+i)];
							}
						}

					if(synvec_chain[j])
						{
						int alias_found = 0;

						for(k=0;k<synvec_chain[j]->num_entries;k++)
							{
							vztint32_t idx = synvec_chain[j]->chain[k];

							if(lt->len[i] == lt->len[idx])
								{
								for(l=0;l<lt->len[i];l++)
									{
									if((idx+l)<=(lt->numfacs-1))
										{
										if(lt->vindex_offset[idx+l] != lt->vindex_offset[i+l]) break;
										}
									}

								if(l == (lt->len[i]))
									{
									lt->rows[i] = idx;			/* already exists */
									num_synalias++;
									alias_found = 1;
									break;
									}
								}
							}

						if(!alias_found)
							{
							synvec_chain[j] = realloc(synvec_chain[j], sizeof(struct vzt_synvec_chain) +
									synvec_chain[j]->num_entries * sizeof(vztint32_t));

							synvec_chain[j]->chain[synvec_chain[j]->num_entries++] = i;
							lt->flags[i] |= VZT_RD_SYM_F_SYNVEC;
							lt->flags[i] &= ~VZT_RD_SYM_F_ALIAS;
							lt->numrealfacs = i+1;				/* bump up to end */
							num_synvecs++;
							}
						}
						else
						{
						synvec_chain[j] = malloc(sizeof(struct vzt_synvec_chain));
						if(synvec_chain[j]) /* scan-build : deref of null pointer below */
							{
							synvec_chain[j]->num_entries = 1;
							synvec_chain[j]->chain[0] = i;
							}

						lt->flags[i] |= VZT_RD_SYM_F_SYNVEC;
						lt->flags[i] &= ~VZT_RD_SYM_F_ALIAS;
						lt->numrealfacs = i+1;				/* bump up to end */
						num_synvecs++;
						}
					}
				}
			}
		}

	for(i=0;i<lt->numfacs;i++)
		{
		if(synvec_chain[i]) free(synvec_chain[i]);
		}

	free(synvec_chain);

	fprintf(stderr, VZT_RDLOAD"%d facilities (after vectorization)\n", num_after_combine);
	if(num_synvecs)
		{
		fprintf(stderr, VZT_RDLOAD"%d complex vectors synthesized\n", num_synvecs);
		if(num_synalias) fprintf(stderr, VZT_RDLOAD"%d complex aliases synthesized\n", num_synalias);
		}
	fprintf(stderr, VZT_RDLOAD"\n");

	if(lt->longest_len != old_longest_len)
		{
		free(lt->value_current_sector); free(lt->value_previous_sector);

		lt->value_current_sector = malloc(lt->longest_len + 1);
		lt->value_previous_sector = malloc(lt->longest_len + 1);
		}
	}
}


struct vzt_rd_trace *vzt_rd_init(const char *name)
{
return(vzt_rd_init_smp(name, 1));
}


/*
 * free up/deallocate any resources still left out there:
 * blindly do it based on NULL pointer comparisons (ok, since
 * calloc() is used to allocate all structs) as performance
 * isn't an issue for this set of cleanup code
 */
void vzt_rd_close(struct vzt_rd_trace *lt)
{
if(lt)
	{
	struct vzt_rd_block *b, *bt;

	if(lt->process_mask) { free(lt->process_mask); lt->process_mask=NULL; }

	if(lt->rows) { free(lt->rows); lt->rows=NULL; }
	if(lt->msb) { free(lt->msb); lt->msb=NULL; }
	if(lt->lsb) { free(lt->lsb); lt->lsb=NULL; }
	if(lt->flags) { free(lt->flags); lt->flags=NULL; }
	if(lt->len) { free(lt->len); lt->len=NULL; }
	if(lt->vindex_offset) { free(lt->vindex_offset); lt->vindex_offset=NULL; }
	if(lt->zfacnames) { free(lt->zfacnames); lt->zfacnames=NULL; }

	if(lt->value_current_sector) { free(lt->value_current_sector); lt->value_current_sector=NULL; }
	if(lt->value_previous_sector) { free(lt->value_previous_sector); lt->value_previous_sector=NULL; }

	if(lt->faccache)
		{
		if(lt->faccache->bufprev) { free(lt->faccache->bufprev); lt->faccache->bufprev=NULL; }
		if(lt->faccache->bufcurr) { free(lt->faccache->bufcurr); lt->faccache->bufcurr=NULL; }

		free(lt->faccache); lt->faccache=NULL;
		}

	b=lt->block_head;
	while(b)
		{
		bt=b->next;
		vzt_rd_block_vch_free(lt, b, 1);
		vzt_rd_pthread_mutex_destroy(lt, &b->mutex);

		free(b);
		b=bt;
		}

	lt->block_head=lt->block_curr=NULL;

	if(lt->zhandle) { gzclose(lt->zhandle); lt->zhandle=NULL; }
	if(lt->handle) { fclose(lt->handle); lt->handle=NULL; }
	if(lt->filename) { free(lt->filename); lt->filename=NULL; }

	vzt_rd_pthread_mutex_destroy(lt, &lt->mutex);

	free(lt);
	}
}


/*******************************************/

/*
 * read single fac value at a given time
 */
static char *vzt_rd_process_block_single_factime(struct vzt_rd_trace *lt, struct vzt_rd_block *b, vztint64_t simtime, vztint32_t idx)
{
unsigned int i;
char *pnt=lt->value_current_sector;	/* convenient workspace mem */
char *buf = lt->value_previous_sector;  /* convenient workspace mem */
char *rcval = NULL;

vzt_rd_block_vch_decode(lt, b);
vzt_rd_pthread_mutex_lock(lt, &b->mutex);

if((b->last_rd_value_simtime == simtime) && (b->last_rd_value_idx != ((vztint32_t)~0)))
	{
	i = b->last_rd_value_idx;
	}
	else
	{
	int i_ok = 0;

	for(i=0;i<b->num_time_ticks;i++) /* for(i=b->num_time_ticks-1; i>=0; i--) */
		{
		if(simtime == b->times[i])
			{
			i_ok = i; break;
			}

		if(simtime < b->times[i]) break;
		i_ok = i;

		/* replace with maximal bsearch if this caching doesn't work out */
		}

	b->last_rd_value_idx = i_ok;
	b->last_rd_value_simtime = simtime;
	}

vzt_rd_fac_value(lt, b, i, idx, pnt);

if(!(lt->flags[idx] & (VZT_RD_SYM_F_DOUBLE|VZT_RD_SYM_F_STRING)))
	{
	rcval = pnt;
	}
	else
	{
	if(lt->flags[idx] & VZT_RD_SYM_F_DOUBLE)
		{
		vzt_rd_double_xdr(pnt, buf);
		rcval = buf;
		}
	else
		{
		unsigned int spnt=vzt_rd_make_sindex(pnt);
		rcval = b->sindex[spnt];
		}
	}

vzt_rd_pthread_mutex_unlock(lt, &b->mutex);
return(rcval);
}


char *vzt_rd_value(struct vzt_rd_trace *lt, vztint64_t simtime, vztint32_t idx)
{
struct vzt_rd_block *b, *b2;
char *rcval = NULL;

if(lt)
	{
	b = lt->block_head;

	if((simtime == lt->last_rd_value_simtime) && (lt->last_rd_value_block))
		{
		b = lt->last_rd_value_block;
		goto b_chk;
		}
		else
		{
		lt->last_rd_value_simtime = simtime;
		}

	while(b)
		{
		if((b->start > simtime) || (b->end < simtime))
			{
			b = b->next;
			continue;
			}

		lt->last_rd_value_block = b;
b_chk:		if((!b->mem)&&(!b->short_read_ignore))
			{
			vzt_rd_decompress_blk(lt, b, 0);
			}

		if(b->mem)
			{
			rcval = vzt_rd_process_block_single_factime(lt, b, simtime, idx);
			break;
			}

		b=b->next;
		}
	}
	else
	{
	return(NULL);
	}

if((b)&&(lt->numblocks > 2))	/* no sense freeing up when not so many blocks */
	{
	vztint64_t block_mem_consumed;

	vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
	block_mem_consumed = lt->block_mem_consumed;
	vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);

	if(block_mem_consumed > lt->block_mem_max)
		{
		b2 = lt->block_head;
		while(b2)
			{
			if((block_mem_consumed > lt->block_mem_max) && (b2 != b))
				{
				vzt_rd_pthread_mutex_lock(lt, &lt->mutex);
				lt->block_mem_consumed -= b2->uncompressed_siz;
				vzt_rd_pthread_mutex_unlock(lt, &lt->mutex);
				vzt_rd_block_vch_free(lt, b2, 0);
				}

			b2 = b2->next;
			}
		}
	}

return(rcval);
}

