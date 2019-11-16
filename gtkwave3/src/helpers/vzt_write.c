/*
 * Copyright (c) 2003-2015 Tony Bybell.
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

#ifdef _AIX
#pragma alloca
#endif

#include <config.h>
#include <wavealloca.h>

#if defined(__CYGWIN__) || defined(__MINGW32__)
#undef HAVE_RPC_XDR_H
#endif

#if HAVE_RPC_XDR_H
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif
#include "vzt_write.h"


/*
 * in-place sort to keep chained facs from migrating...
 */
static void wave_mergesort(struct vzt_wr_symbol **a, struct vzt_wr_symbol **b, int lo, int hi)
{
int i, j, k;

if (lo<hi)
	{
        int mid=(lo+hi)/2;

        wave_mergesort(a, b, lo, mid);
        wave_mergesort(a, b, mid+1, hi);

    	i=0; j=lo;
    	while (j<=mid)
		{
        	b[i++]=a[j++];
		}

    	i=0; k=lo;
    	while ((k<j)&&(j<=hi))
		{
        	if (strcmp(b[i]->name, a[j]->name) <= 0)
			{
            		a[k++]=b[i++];
			}
			else
			{
	            	a[k++]=a[j++];
			}
		}

    	while (k<j)
		{
        	a[k++]=b[i++];
		}
	}
}

static void wave_msort(struct vzt_wr_symbol **a, int num)
{
struct vzt_wr_symbol **b = malloc(((num/2)+1) * sizeof(struct vzt_wr_symbol *));

wave_mergesort(a, b, 0, num-1);

free(b);
}


/*
 * gz/bz2 calls
 */
static _VZT_WR_INLINE void *vzt_gzdopen(struct vzt_wr_trace *lt, int fd, const char *mode)
{
if(lt)
	{
	lt->ztype = lt->ztype_cfg;	/* shadow config at file open */

	switch(lt->ztype)
		{
		case VZT_WR_IS_GZ:	return(gzdopen(fd, mode));
		case VZT_WR_IS_BZ2:	return(BZ2_bzdopen(fd, mode));
		case VZT_WR_IS_LZMA:
		default:
					return(LZMA_fdopen(fd, mode));
		}
	}

return(NULL);
}

static _VZT_WR_INLINE int vzt_gzclose(struct vzt_wr_trace *lt, void *file)
{
if(lt)
	{
	switch(lt->ztype)
		{
		case VZT_WR_IS_GZ:	return(gzclose(file));
		case VZT_WR_IS_BZ2:	BZ2_bzclose(file); return(0);
		case VZT_WR_IS_LZMA:
		default:
					LZMA_close(file); return(0);
		}
	}

return(0);
}

static _VZT_WR_INLINE int vzt_gzflush(struct vzt_wr_trace *lt, void *file, int flush)
{
if(lt)
	{
	switch(lt->ztype)
		{
		case VZT_WR_IS_GZ:	return(gzflush(file, flush));
		case VZT_WR_IS_BZ2:	return(BZ2_bzflush(file));
		case VZT_WR_IS_LZMA:
		default:
					return(0); /* no real need to do a LZMA_flush(file) as the dictionary is so big */
		}
	}

return(0);
}

static _VZT_WR_INLINE int vzt_gzwrite(struct vzt_wr_trace *lt, void *file, void* buf, unsigned len)
{
if(lt)
	{
	switch(lt->ztype)
		{
		case VZT_WR_IS_GZ:	return(gzwrite(file, buf, len));
		case VZT_WR_IS_BZ2:	return(BZ2_bzwrite(file, buf, len));
		case VZT_WR_IS_LZMA:
		default:
					return(LZMA_write(file, buf, len));
		}
	}

return(0);
}


/************************ splay ************************/

#define cmp_l(i,j) ((int)(-(i<j) + (i>j)))
#define cmp_l_lt(i,j) (i<j)
#define cmp_l_gt(i,j) (i>j)

static int vzt_wr_dsvzt_success;

static vzt_wr_dsvzt_Tree * vzt_wr_dsvzt_splay (vztint32_t i, vzt_wr_dsvzt_Tree * t) {
/* Simple top down splay, not requiring i to be in the tree t.  */
/* What it does is described above.                             */
    vzt_wr_dsvzt_Tree N, *l, *r, *y;
    int dir;

    vzt_wr_dsvzt_success = 0;

    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
	dir = cmp_l(i, t->item);
	if (dir < 0) {
	    if (t->left == NULL) break;
	    if (cmp_l_lt(i, t->left->item)) {
		y = t->left;                           /* rotate right */
		t->left = y->right;
		y->right = t;
		t = y;
		if (t->left == NULL) break;
	    }
	    r->left = t;                               /* link right */
	    r = t;
	    t = t->left;
	} else if (dir > 0) {
	    if (t->right == NULL) break;
	    if (cmp_l_gt(i, t->right->item)) {
		y = t->right;                          /* rotate left */
		t->right = y->left;
		y->left = t;
		t = y;
		if (t->right == NULL) break;
	    }
	    l->right = t;                              /* link left */
	    l = t;
	    t = t->right;
	} else {
	    vzt_wr_dsvzt_success=1;
	    break;
	}
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}


static vzt_wr_dsvzt_Tree * vzt_wr_dsvzt_insert(vztint32_t i, vzt_wr_dsvzt_Tree * t, vztint32_t val) {
/* Insert i into the tree t, unless it's already there.    */
/* Return a pointer to the resulting tree.                 */
    vzt_wr_dsvzt_Tree * n;
    int dir;

    n = (vzt_wr_dsvzt_Tree *) calloc (1, sizeof (vzt_wr_dsvzt_Tree));
    if (n == NULL) {
	fprintf(stderr, "dsvzt_insert: ran out of memory, exiting.\n");
	exit(255);
    }
    n->item = i;
    n->val  = val;
    if (t == NULL) {
	n->left = n->right = NULL;
	return n;
    }
    t = vzt_wr_dsvzt_splay(i,t);
    dir = cmp_l(i,t->item);
    if (dir<0) {
	n->left = t->left;
	n->right = t;
	t->left = NULL;
	return n;
    } else if (dir>0) {
	n->right = t->right;
	n->left = t;
	t->right = NULL;
	return n;
    } else { /* We get here if it's already in the tree */
             /* Don't add it again                      */
	free(n);
	return t;
    }
}

/************************ splay ************************/

static int vzt2_wr_dsvzt_success;

static vzt2_wr_dsvzt_Tree * vzt2_wr_dsvzt_splay (char *i, vzt2_wr_dsvzt_Tree * t) {
/* Simple top down splay, not requiring i to be in the tree t.  */
/* What it does is described above.                             */
    vzt2_wr_dsvzt_Tree N, *l, *r, *y;
    int dir;

    vzt2_wr_dsvzt_success = 0;

    if (t == NULL) return t;
    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
	dir = strcmp(i, t->item);
	if (dir < 0) {
	    if (t->left == NULL) break;
	    if (strcmp(i, t->left->item)<0) {
		y = t->left;                           /* rotate right */
		t->left = y->right;
		y->right = t;
		t = y;
		if (t->left == NULL) break;
	    }
	    r->left = t;                               /* link right */
	    r = t;
	    t = t->left;
	} else if (dir > 0) {
	    if (t->right == NULL) break;
	    if (strcmp(i, t->right->item)>0) {
		y = t->right;                          /* rotate left */
		t->right = y->left;
		y->left = t;
		t = y;
		if (t->right == NULL) break;
	    }
	    l->right = t;                              /* link left */
	    l = t;
	    t = t->right;
	} else {
	    vzt2_wr_dsvzt_success=1;
	    break;
	}
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}


static vzt2_wr_dsvzt_Tree * vzt2_wr_dsvzt_insert(char *i, vzt2_wr_dsvzt_Tree * t, unsigned int val) {
/* Insert i into the tree t, unless it's already there.    */
/* Return a pointer to the resulting tree.                 */
    vzt2_wr_dsvzt_Tree * n;
    int dir;

    n = (vzt2_wr_dsvzt_Tree *) calloc (1, sizeof (vzt2_wr_dsvzt_Tree));
    if (n == NULL) {
	fprintf(stderr, "dsvzt_insert: ran out of memory, exiting.\n");
	exit(255);
    }
    n->item = i;
    n->val  = val;
    if (t == NULL) {
	n->left = n->right = NULL;
	return n;
    }
    t = vzt2_wr_dsvzt_splay(i,t);
    dir = strcmp(i,t->item);
    if (dir<0) {
	n->left = t->left;
	n->right = t;
	t->left = NULL;
	return n;
    } else if (dir>0) {
	n->right = t->right;
	n->left = t;
	t->right = NULL;
	return n;
    } else { /* We get here if it's already in the tree */
             /* Don't add it again                      */
	free(n);
	return t;
    }
}

/************************ splay ************************/

/*
 * functions which emit various big endian
 * data to a file
 */
static int vzt_wr_emit_u8(struct vzt_wr_trace *lt, int value)
{
unsigned char buf[1];
int nmemb;

buf[0] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 1, lt->handle);
lt->position+=nmemb;
return(nmemb);
}


static int vzt_wr_emit_u16(struct vzt_wr_trace *lt, int value)
{
unsigned char buf[2];
int nmemb;

buf[0] = (value>>8) & 0xff;
buf[1] = value & 0xff;
nmemb = fwrite(buf, sizeof(char), 2, lt->handle);
lt->position+=nmemb;
return(nmemb);
}


static int vzt_wr_emit_u32(struct vzt_wr_trace *lt, int value)
{
unsigned char buf[4];
int nmemb;

buf[0] = (value>>24) & 0xff;
buf[1] = (value>>16) & 0xff;
buf[2] = (value>>8) & 0xff;
buf[3] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 4, lt->handle);
lt->position+=nmemb;
return(nmemb);
}


static int vzt_wr_emit_u64(struct vzt_wr_trace *lt, int valueh, int valuel)
{
int rc;

if((rc=vzt_wr_emit_u32(lt, valueh)))
	{
	rc=vzt_wr_emit_u32(lt, valuel);
	}

return(rc);
}


/*
 * gzfunctions which emit various big endian
 * data to a file.  (lt->position needs to be
 * fixed up on gzclose so the tables don't
 * get out of sync!)
 */
static int gzwrite_buffered(struct vzt_wr_trace *lt)
{
int rc = 1;

if(lt->gzbufpnt > VZT_WR_GZWRITE_BUFFER)
	{
	rc = vzt_gzwrite(lt, lt->zhandle, lt->gzdest, lt->gzbufpnt);
	rc = rc ? 1 : 0;
	lt->gzbufpnt = 0;
	}

return(rc);
}

static void gzflush_buffered(struct vzt_wr_trace *lt, int doclose)
{
if(lt->gzbufpnt)
	{
	vzt_gzwrite(lt, lt->zhandle, lt->gzdest, lt->gzbufpnt);
	lt->gzbufpnt = 0;
	if(!doclose)
		{
		vzt_gzflush(lt, lt->zhandle, Z_SYNC_FLUSH);
		}
	}

if(doclose)
	{
	vzt_gzclose(lt, lt->zhandle);
	}
}


static int vzt_wr_emit_u8z(struct vzt_wr_trace *lt, int value)
{
int nmemb;

lt->gzdest[lt->gzbufpnt++] = value & 0xff;

nmemb=gzwrite_buffered(lt);
lt->zpackcount++;
lt->position++;
return(nmemb);
}


static int vzt_wr_emit_u16z(struct vzt_wr_trace *lt, int value)
{
int nmemb;

lt->gzdest[lt->gzbufpnt++] = (value>>8) & 0xff;
lt->gzdest[lt->gzbufpnt++] = value & 0xff;
nmemb = gzwrite_buffered(lt);
lt->zpackcount+=2;
lt->position+=2;
return(nmemb);
}


static int vzt_wr_emit_u32z(struct vzt_wr_trace *lt, int value)
{
int nmemb;

lt->gzdest[lt->gzbufpnt++] = (value>>24) & 0xff;
lt->gzdest[lt->gzbufpnt++] = (value>>16) & 0xff;
lt->gzdest[lt->gzbufpnt++] = (value>>8) & 0xff;
lt->gzdest[lt->gzbufpnt++] = value & 0xff;
nmemb=gzwrite_buffered(lt);

lt->zpackcount+=4;
lt->position+=4;
return(nmemb);
}

static int vzt_wr_emit_u32rz(struct vzt_wr_trace *lt, int value)
{
int nmemb;

lt->gzdest[lt->gzbufpnt++] = value & 0xff;
lt->gzdest[lt->gzbufpnt++] = (value>>8) & 0xff;
lt->gzdest[lt->gzbufpnt++] = (value>>16) & 0xff;
lt->gzdest[lt->gzbufpnt++] = (value>>24) & 0xff;
nmemb=gzwrite_buffered(lt);

lt->zpackcount+=4;
lt->position+=4;
return(nmemb);
}

#if 0
static int vzt_wr_emit_u64z(struct vzt_wr_trace *lt, int valueh, int valuel)
{
int rc;

if((rc=vzt_wr_emit_u32z(lt, valueh)))
	{
	rc=vzt_wr_emit_u32z(lt, valuel);
	}

return(rc);
}
#endif

static int vzt_wr_emit_stringz(struct vzt_wr_trace *lt, char *value)
{
int rc=1;
do
	{
        rc&=vzt_wr_emit_u8z(lt, *value);
        } while(*(value++));
return(rc);
}


static int vzt_wr_emit_uv32z(struct vzt_wr_trace *lt, unsigned int v)
{
int nmemb;
unsigned int nxt;
unsigned int oldpnt = lt->gzbufpnt;

while((nxt = v>>7))
        {
        lt->gzdest[lt->gzbufpnt++] = (v&0x7f);
        v = nxt;
        }
lt->gzdest[lt->gzbufpnt++] = (v&0x7f) | 0x80;

lt->zpackcount+=(lt->gzbufpnt - oldpnt);
lt->position+=(lt->gzbufpnt - oldpnt);
nmemb=gzwrite_buffered(lt);

return(nmemb);
}

static int vzt_wr_emit_uv64z(struct vzt_wr_trace *lt, vztint64_t v)
{
int nmemb;
vztint64_t nxt;
unsigned int oldpnt = lt->gzbufpnt;

while((nxt = v>>7))
        {
        lt->gzdest[lt->gzbufpnt++] = (v&0x7f);
        v = nxt;
        }
lt->gzdest[lt->gzbufpnt++] = (v&0x7f) | 0x80;

lt->zpackcount+=(lt->gzbufpnt - oldpnt);
lt->position+=(lt->gzbufpnt - oldpnt);
nmemb=gzwrite_buffered(lt);

return(nmemb);
}




/*
 * hash/symtable manipulation
 */
static int vzt_wr_hash(const char *s)
{
const char *p;
char ch;
unsigned int h=0, h2=0, pos=0, g;
for(p=s;*p;p++)
        {
	ch=*p;
	h2<<=3;
	h2-=((unsigned int)ch+(pos++));		/* this handles stranded vectors quite well.. */

        h=(h<<4)+ch;
        if((g=h&0xf0000000))
                {
                h=h^(g>>24);
                h=h^g;
                }
        }

h^=h2;						/* combine the two hashes */
return(h%VZT_WR_SYMPRIME);
}


static struct vzt_wr_symbol *vzt_wr_symadd(struct vzt_wr_trace *lt, const char *name, int hv)
{
struct vzt_wr_symbol *s;

s=(struct vzt_wr_symbol *)calloc(1,sizeof(struct vzt_wr_symbol));
strcpy(s->name=(char *)malloc((s->namlen=strlen(name))+1),name);
s->next=lt->sym[hv];
lt->sym[hv]=s;
return(s);
}


static struct vzt_wr_symbol *vzt_wr_symfind(struct vzt_wr_trace *lt, const char *s)
{
int hv;
struct vzt_wr_symbol *temp;

hv=vzt_wr_hash(s);
if(!(temp=lt->sym[hv])) return(NULL); /* no hash entry, add here wanted to add */

while(temp)
        {
        if(!strcmp(temp->name,s))
                {
                return(temp); /* in table already */
                }
        if(!temp->next) break;
        temp=temp->next;
        }

return(NULL); /* not found, add here if you want to add*/
}


/*
 * compress facs to a prefix count + string + 0x00
 */
static void vzt_wr_compress_fac(struct vzt_wr_trace *lt, char *str)
{
int i;
int len = strlen(str);
int minlen = (len<lt->compress_fac_len) ? len : lt->compress_fac_len;

if(minlen>65535) minlen=65535;    /* keep in printable range--most hierarchies won't be this big anyway */

if(lt->compress_fac_str)
        {
        for(i=0;i<minlen;i++)
                {
                if(lt->compress_fac_str[i]!=str[i]) break;
                }
	vzt_wr_emit_u16z(lt, i);
	vzt_wr_emit_stringz(lt, str+i);
        free(lt->compress_fac_str);
        }
        else
        {
	vzt_wr_emit_u16z(lt, 0);
	vzt_wr_emit_stringz(lt, str);
        }

lt->compress_fac_str = (char *) malloc((lt->compress_fac_len=len)+1);
strcpy(lt->compress_fac_str, str);
}


/*
 * emit facs in sorted order along with geometry
 * and sync table info
 */

static void strip_brack(struct vzt_wr_symbol *s)
{
char *lastch = s->name+s->namlen - 1;
if(*lastch!=']') return;
if(s->namlen<3) return;
lastch--;
while(lastch!=s->name)
	{
        if(*lastch=='.')
                {
                return; /* MTI SV [0.3] notation for implicit vars */
                }

	if(*lastch=='[')
		{
		*lastch=0x00;
		return;
		}
	lastch--;
	}
return;
}


static void vzt_wr_emitfacs(struct vzt_wr_trace *lt)
{
int i;

if((lt)&&(lt->numfacs))
	{
	struct vzt_wr_symbol *s = lt->symchain;
	struct vzt_wr_symbol **aliascache = calloc(lt->numalias ? lt->numalias : 1, sizeof(struct vzt_wr_symbol *));
	int aliases_encountered, facs_encountered;

	lt->sorted_facs = (struct vzt_wr_symbol **)calloc(lt->numfacs, sizeof(struct vzt_wr_symbol *));

	if(lt->sorted_facs && aliascache)
		{
		if(lt->do_strip_brackets)
		for(i=0;i<lt->numfacs;i++)
			{
			lt->sorted_facs[lt->numfacs - i - 1] = s;	/* facs were chained backwards so reverse to restore bitslicing */
			strip_brack(s);
			s=s->symchain;
			}
		else
		for(i=0;i<lt->numfacs;i++)
			{
			lt->sorted_facs[lt->numfacs - i - 1] = s;	/* facs were chained backwards so reverse to restore bitslicing */
			s=s->symchain;
			}
		wave_msort(lt->sorted_facs, lt->numfacs);

		/* move facs up */
		aliases_encountered = 0, facs_encountered = 0;
		for(i=0;i<lt->numfacs;i++)
			{
			if((lt->sorted_facs[i]->flags&VZT_WR_SYM_F_ALIAS)==0)
				{
				lt->sorted_facs[facs_encountered] = lt->sorted_facs[i];
				facs_encountered++;
				}
				else
				{
				aliascache[aliases_encountered] = lt->sorted_facs[i];
				aliases_encountered++;
				}
			}
		/* then append the aliases */
		for(i=0;i<aliases_encountered;i++)
			{
			lt->sorted_facs[facs_encountered+i] = aliascache[i];
			}


		for(i=0;i<lt->numfacs;i++)
			{
			lt->sorted_facs[i]->facnum = i;
			}

		if(!lt->timezero)
			{
			vzt_wr_emit_u32(lt, lt->numfacs);	/* uncompressed */
			}
			else
			{
			vzt_wr_emit_u32(lt, 0);			/* uncompressed, flag to insert extra parameters */
			vzt_wr_emit_u32(lt, 8);			/* uncompressed 8 counts timezero and on */
			vzt_wr_emit_u32(lt, lt->numfacs);	/* uncompressed */
			vzt_wr_emit_u64(lt, (lt->timezero >> 32) & 0xffffffffL, lt->timezero & 0xffffffffL); /* uncompressed */
			}
		vzt_wr_emit_u32(lt, lt->numfacbytes);	/* uncompressed */
		vzt_wr_emit_u32(lt, lt->longestname);	/* uncompressed */

		lt->facname_offset=lt->position;

		vzt_wr_emit_u32(lt, 0); 		/* uncompressed : placeholder for zfacnamesize     */
		vzt_wr_emit_u32(lt, 0);		/* uncompressed : placeholder for zfacname_predec_size */
		vzt_wr_emit_u32(lt, 0); 		/* uncompressed : placeholder for zfacgeometrysize */
		vzt_wr_emit_u8(lt, lt->timescale);	/* timescale (-9 default == nsec) */

		fflush(lt->handle);
		lt->zfacname_size = lt->position;
		lt->zhandle = vzt_gzdopen(lt, dup(fileno(lt->handle)), "wb9");

		lt->zpackcount = 0;
		for(i=0;i<lt->numfacs;i++)
			{
		 	vzt_wr_compress_fac(lt, lt->sorted_facs[i]->name);
			free(lt->sorted_facs[i]->name);
			lt->sorted_facs[i]->name = NULL;
			}
		free(lt->compress_fac_str); lt->compress_fac_str=NULL;
		lt->compress_fac_len=0;
		lt->zfacname_predec_size = lt->zpackcount;

		gzflush_buffered(lt, 1);
		fseeko(lt->handle, 0L, SEEK_END);
		lt->position=ftello(lt->handle);
		lt->zfacname_size = lt->position - lt->zfacname_size;

		lt->zhandle = vzt_gzdopen(lt, dup(fileno(lt->handle)), "wb9");

		lt->facgeometry_offset = lt->position;
		for(i=0;i<lt->numfacs;i++)
			{
			if((lt->sorted_facs[i]->flags&VZT_WR_SYM_F_ALIAS)==0)
				{
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->rows);
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->msb);
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->lsb);
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->flags & VZT_WR_SYM_MASK);
				}
				else
				{
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->aliased_to->facnum);
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->msb);
				vzt_wr_emit_u32z(lt, lt->sorted_facs[i]->lsb);
				vzt_wr_emit_u32z(lt, VZT_WR_SYM_F_ALIAS);
				}
			}

		gzflush_buffered(lt, 1);
		fseeko(lt->handle, 0L, SEEK_END);
		lt->position=ftello(lt->handle);
		lt->break_header_size = lt->position;			/* in case we need to emit multiple vzts with same header */
		lt->zfacgeometry_size = lt->position - lt->facgeometry_offset;

		fseeko(lt->handle, lt->facname_offset, SEEK_SET);
		vzt_wr_emit_u32(lt, lt->zfacname_size);		/* backpatch sizes... */
		vzt_wr_emit_u32(lt, lt->zfacname_predec_size);
		vzt_wr_emit_u32(lt, lt->zfacgeometry_size);

		lt->numfacs = facs_encountered;				/* don't process alias value changes ever */
		}

	if(aliascache) free(aliascache);
	}
}


/*
 * initialize the trace and get back an lt context
 */
struct vzt_wr_trace *vzt_wr_init(const char *name)
{
struct vzt_wr_trace *lt=(struct vzt_wr_trace *)calloc(1, sizeof(struct vzt_wr_trace));

if((!name)||(!(lt->handle=fopen(name, "wb"))))
	{
	free(lt);
	lt=NULL;
	}
	else
	{
	lt->vztname = strdup(name);

	vzt_wr_emit_u16(lt, VZT_WR_HDRID);
	vzt_wr_emit_u16(lt, VZT_WR_VERSION);
	vzt_wr_emit_u8 (lt, VZT_WR_GRANULE_SIZE);	/* currently 32 */
	lt->timescale = -9;
	lt->maxgranule = VZT_WR_GRANULE_NUM;
	lt->timetable = calloc(lt->maxgranule * VZT_WR_GRANULE_SIZE, sizeof(vzttime_t));
	vzt_wr_set_compression_depth(lt, 4);		/* set fast/loose compression depth, user can fix this any time after init */
	lt->initial_value = 'x';

	lt->multi_state = 1;
	}

return(lt);
}


/*
 * force trace to two state
 */
void vzt_wr_force_twostate(struct vzt_wr_trace *lt)
{
if((lt)&&(!lt->symchain))
	{
	lt->multi_state = 0;
	}
}


/*
 * setting break size
 */
void vzt_wr_set_break_size(struct vzt_wr_trace *lt, off_t siz)
{
if(lt)
	{
	lt->break_size = siz;
	}
}


/*
 * set initial value of trace (0, 1, x, z) only legal vals
 */
void vzt_wr_set_initial_value(struct vzt_wr_trace *lt, char value)
{
if(lt)
	{
	switch(value)
		{
		case '0':
		case '1':
		case 'x':
		case 'z':	break;
		case 'Z':	value = 'z'; break;
		default:	value = 'x'; break;
		}

	lt->initial_value = value;
	}
}


/*
 * maint function for finding a symbol if it exists
 */
struct vzt_wr_symbol *vzt_wr_symbol_find(struct vzt_wr_trace *lt, const char *name)
{
struct vzt_wr_symbol *s=NULL;

if((lt)&&(name)) s=vzt_wr_symfind(lt, name);
return(s);
}


/*
 * add a trace (if it doesn't exist already)
 */
struct vzt_wr_symbol *vzt_wr_symbol_add(struct vzt_wr_trace *lt, const char *name, unsigned int rows, int msb, int lsb, int flags)
{
struct vzt_wr_symbol *s;
int i, len;
int flagcnt;

if((!lt)||(lt->sorted_facs)) return(NULL);

flagcnt = ((flags&VZT_WR_SYM_F_INTEGER)!=0) + ((flags&VZT_WR_SYM_F_DOUBLE)!=0) + ((flags&VZT_WR_SYM_F_STRING)!=0);

if((flagcnt>1)||(!lt)||(!name)||(vzt_wr_symfind(lt, name))) return (NULL);

if(!(flags & (VZT_WR_SYM_F_INTEGER|VZT_WR_SYM_F_STRING|VZT_WR_SYM_F_DOUBLE)))
	{
	len = (msb<lsb) ? (lsb-msb+1) : (msb-lsb+1);
	}
	else
	{
	len = (flags & (VZT_WR_SYM_F_INTEGER|VZT_WR_SYM_F_STRING)) ? 32 : 64;
	}

s=vzt_wr_symadd(lt, name, vzt_wr_hash(name));
s->rows = rows;
s->flags = flags&(~VZT_WR_SYM_F_ALIAS);	/* aliasing makes no sense here.. */

s->prev  = (vzt_wr_dsvzt_Tree **)calloc(len, sizeof(vzt_wr_dsvzt_Tree *));
s->chg =   (vztint32_t *)calloc(len, sizeof(vztint32_t));

if(lt->multi_state)
	{
	s->prevx = (vzt_wr_dsvzt_Tree **)calloc(len, sizeof(vzt_wr_dsvzt_Tree *));
	s->chgx =  (vztint32_t *)calloc(len, sizeof(vztint32_t));
	}

if(!flagcnt)
	{
	s->msb = msb;
	s->lsb = lsb;
	}
s->len = len;

if(!(flags & (VZT_WR_SYM_F_INTEGER|VZT_WR_SYM_F_STRING|VZT_WR_SYM_F_DOUBLE)))
	{
	if((lt->initial_value == '1')||(lt->initial_value == 'z'))
		{
		for(i=0;i<s->len;i++)
			{
			s->chg[i] = ~0;
			}
		}

	if(lt->multi_state)
		{
		if((lt->initial_value == 'x')||(lt->initial_value == 'z'))
			{
			for(i=0;i<s->len;i++)
				{
				s->chgx[i] = ~0;
				}
			}
		}
	}

s->symchain = lt->symchain;
lt->symchain = s;

lt->numfacs++;

if((len=strlen(name)) > lt->longestname) lt->longestname = len;
lt->numfacbytes += (len+1);

return(s);
}

/*
 * add an alias trace (if it doesn't exist already and orig is found)
 */
struct vzt_wr_symbol *vzt_wr_symbol_alias(struct vzt_wr_trace *lt, const char *existing_name, const char *alias, int msb, int lsb)
{
struct vzt_wr_symbol *s, *sa;
int len;
int bitlen;
int flagcnt;

if((!lt)||(!existing_name)||(!alias)||(!(s=vzt_wr_symfind(lt, existing_name)))||(vzt_wr_symfind(lt, alias))) return (NULL);

if(lt->sorted_facs) return(NULL);

while(s->aliased_to)	/* find root alias */
	{
	s=s->aliased_to;
	}

flagcnt = ((s->flags&VZT_WR_SYM_F_INTEGER)!=0) + ((s->flags&VZT_WR_SYM_F_DOUBLE)!=0) + ((s->flags&VZT_WR_SYM_F_STRING)!=0);
bitlen = (msb<lsb) ? (lsb-msb+1) : (msb-lsb+1);
if((!flagcnt)&&(bitlen!=s->len)) return(NULL);

sa=vzt_wr_symadd(lt, alias, vzt_wr_hash(alias));
sa->flags = VZT_WR_SYM_F_ALIAS;	/* only point this can get set */
sa->aliased_to = s;

if(!flagcnt)
	{
	sa->msb = msb;
	sa->lsb = lsb;
	sa->len = bitlen;
	}

sa->symchain = lt->symchain;
lt->symchain = sa;
lt->numfacs++;
lt->numalias++;
if((len=strlen(alias)) > lt->longestname) lt->longestname = len;
lt->numfacbytes += (len+1);

return(sa);
}


/*
 * set current time/granule updating
 */
int vzt_wr_inc_time_by_delta(struct vzt_wr_trace *lt, unsigned int timeval)
{
return(vzt_wr_set_time64(lt, lt->maxtime + (vzttime_t)timeval));
}

int vzt_wr_set_time(struct vzt_wr_trace *lt, unsigned int timeval)
{
return(vzt_wr_set_time64(lt, (vzttime_t)timeval));
}

int vzt_wr_inc_time_by_delta64(struct vzt_wr_trace *lt, vzttime_t timeval)
{
return(vzt_wr_set_time64(lt, lt->maxtime + timeval));
}


/*
 * file size limiting/header cloning...
 */
static void vzt_wr_emit_do_breakfile(struct vzt_wr_trace *lt)
{
unsigned int len = strlen(lt->vztname);
int i;
char *tname = malloc(len + 30);
FILE *f2, *clone;
off_t cnt, seg;
char buf[32768];

for(i=len;i>0;i--)
	{
	if(lt->vztname[i]=='.') break;
	}

if(!i)
	{
	sprintf(tname, "%s_%03d.vzt", lt->vztname, ++lt->break_number);
	}
	else
	{
	memcpy(tname, lt->vztname, i);
	sprintf(tname+i, "_%03d.vzt", ++lt->break_number);
	}

f2 = fopen(tname, "wb");
if(!f2)	/* if error, keep writing to same output file...sorry */
	{
	free(tname);
	return;
	}

clone = fopen(lt->vztname, "rb");
if(!clone)
	{ /* this should never happen */
	fclose(f2);
	unlink(tname);
	free(tname);
	return;
	}

/* clone original header */
for(cnt = 0; cnt < lt->break_header_size; cnt += sizeof(buf))
	{
	seg = lt->break_header_size - cnt;
	if(seg > (off_t)sizeof(buf))
		{
		seg = sizeof(buf);
		}

	if(fread(buf, seg, 1, clone))
		{
		if(!fwrite(buf, seg, 1, f2)) break; /* write error! */
		}
	}

fclose(clone);
fclose(lt->handle);
lt->handle = f2;
free(tname);
}


static void vzt_wr_recurse_reorder_dict(vzt_wr_dsvzt_Tree *t, struct vzt_wr_trace *lt, vztint32_t *newval, vztint32_t *bpnt, int depth)
{
int i, j;

if(t->left)
	{
	vzt_wr_recurse_reorder_dict(t->left, lt, newval, bpnt, depth);
	}

*bpnt = t->item;

if(t->child)
	{
	vzt_wr_recurse_reorder_dict(t->child, lt, newval, bpnt+1, depth+1);
	}
	else
	{
	vztint32_t *bpnt2 = bpnt - depth + 1;
	t->val = *newval;		/* resequence the dict entries in lexical order */
	*newval = *newval+1;

	if(!lt->rle)
		{
		for(i=0;i<depth;i++)
			{
	 		vztint32_t k = *(bpnt2++);
			vzt_wr_emit_u32rz(lt, k);
			}
		}
		else
		{
		vztint32_t prev = lt->rle_start;
		vztint32_t run = 0;

		lt->rle_start = (*bpnt2) & 1;

		for(i=0;i<depth;i++)
			{
	 		vztint32_t k = *(bpnt2++);
			for(j=0;j<32;j++)
				{
				if(prev == (k&1))
					{
					run++;
					}
					else
					{
					prev = (k&1);
					vzt_wr_emit_uv32z(lt, run);
					run = 1;
					}

				k >>= 1;
				}
			}

		vzt_wr_emit_uv32z(lt, run);
		}
	}

if(t->right)
	{
	vzt_wr_recurse_reorder_dict(t->right, lt, newval, bpnt, depth);
	}
}


static void vzt_wr_recurse_free_dict(vzt_wr_dsvzt_Tree *t)
{
if(t->left)
	{
	vzt_wr_recurse_free_dict(t->left);
	}

if(t->child)
	{
	vzt_wr_recurse_free_dict(t->child);
	}

if(t->right)
	{
	vzt_wr_recurse_free_dict(t->right);
	}

free(t);
}


/*
 * emit granule
 */
void vzt_wr_flush_granule(struct vzt_wr_trace *lt, int do_finalize)
{
int i, j;
vztsint32_t k;
int val;
unsigned int numticks;

if(!lt->emitted) /* only happens if there are no value changes */
        {
        vzt_wr_emitfacs(lt);
        lt->emitted = 1;
	}

if(!lt->timegranule)
	{
	vzt_wr_dsvzt_Tree *t=NULL;

	val = 0;

	for(j=0;j<lt->numfacs;j++)
		{
		struct vzt_wr_symbol *s = lt->sorted_facs[j];
		for(i=0;i<s->len;i++)
			{
			t = vzt_wr_dsvzt_splay(s->chg[i], t);
			if(!vzt_wr_dsvzt_success)
				{
				t = vzt_wr_dsvzt_insert(s->chg[i], t, val++);
				}

			k = s->chg[i];
			s->chg[i] = k>>31;

			s->prev[i] = t;
			}
		}

	if(lt->multi_state)
	for(j=0;j<lt->numfacs;j++)
		{
		struct vzt_wr_symbol *s = lt->sorted_facs[j];
		for(i=0;i<s->len;i++)
			{
			lt->use_multi_state |= s->chgx[i];
			t = vzt_wr_dsvzt_splay(s->chgx[i], t);
			if(!vzt_wr_dsvzt_success)
				{
				t = vzt_wr_dsvzt_insert(s->chgx[i], t, val++);
				}

			k = s->chgx[i];
			s->chgx[i] = k>>31;

			s->prevx[i] = t;
			}
		}

	lt->dict = t;
	}
	else
	{
	vzt_wr_dsvzt_Tree *t;

	val = 0;

	for(j=0;j<lt->numfacs;j++)
		{
		struct vzt_wr_symbol *s = lt->sorted_facs[j];
		for(i=0;i<s->len;i++)
			{
			t = s->prev[i]->child;
			t = vzt_wr_dsvzt_splay(s->chg[i], t);
			if(!vzt_wr_dsvzt_success)
				{
				t = vzt_wr_dsvzt_insert(s->chg[i], t, val++);
				}

			k = s->chg[i];
			s->chg[i] = k>>31;

			s->prev[i]->child = t;
			s->prev[i] = t;
			}
		}

	if(lt->multi_state)
	for(j=0;j<lt->numfacs;j++)
		{
		struct vzt_wr_symbol *s = lt->sorted_facs[j];
		for(i=0;i<s->len;i++)
			{
			t = s->prevx[i]->child;
			lt->use_multi_state |= s->chgx[i];
			t = vzt_wr_dsvzt_splay(s->chgx[i], t);
			if(!vzt_wr_dsvzt_success)
				{
				t = vzt_wr_dsvzt_insert(s->chgx[i], t, val++);
				}

			k = s->chgx[i];
			s->chgx[i] = k>>31;

			s->prevx[i]->child = t;
			s->prevx[i] = t;
			}
		}
	}


numticks = lt->timegranule * VZT_WR_GRANULE_SIZE + lt->timepos;
lt->timepos = 0;
lt->timegranule++;
if((lt->timegranule >= lt->maxgranule)||(do_finalize))
	{
	off_t clen, unclen;
	vztint32_t newval = 0;
        int attempt_break_state = 2;

        do      {
                fseeko(lt->handle, 0L, SEEK_END);
                lt->current_chunk=lt->position = ftello(lt->handle);

                if((lt->break_size)&&(attempt_break_state==2)&&(lt->position >= lt->break_size)&&(lt->position != lt->break_header_size))
                        {
                        vzt_wr_emit_do_breakfile(lt);
                        attempt_break_state--;
                        }
                        else
                        {
                        attempt_break_state = 0;
                        }
                } while(attempt_break_state);

	/* flush everything here */
        fseeko(lt->handle, 0L, SEEK_END);
        lt->current_chunk=lt->position = ftello(lt->handle);
        vzt_wr_emit_u32(lt, 0);        /* size of this section (uncompressed) */
        vzt_wr_emit_u32(lt, 0);        /* size of this section (compressed)   */
        vzt_wr_emit_u64(lt, 0, 0);     /* begin time of section               */
        vzt_wr_emit_u64(lt, 0, 0);     /* end time of section                 */
        fflush(lt->handle);
        lt->current_chunkz = lt->position;

	lt->zhandle = vzt_gzdopen(lt, dup(fileno(lt->handle)), lt->zmode);
	lt->zpackcount = 0;

	if((lt->lasttime - lt->firsttime + 1) == numticks)
		{
	        vzt_wr_emit_uv32z(lt, 0); 	  /* special case for cycle simulation */
		}
		else
		{
	        vzt_wr_emit_uv32z(lt, numticks); /* number of time ticks */
		for(i=0;i<((int)numticks);i++)
			{
		        vzt_wr_emit_uv64z(lt, i ? lt->timetable[i] - lt->timetable[i-1] : lt->timetable[i]); /* emit delta */
			}
		gzflush_buffered(lt, 0);
		}

        vzt_wr_emit_uv32z(lt, lt->timegranule);        /* number of 32-bit sections */
	lt->timegranule = 0;

        vzt_wr_emit_uv32z(lt, val);      		/* number of dict entries */
	while((lt->zpackcount & 3) != 0)
		{
		vzt_wr_emit_u8z(lt, 0);	/* pad to word boundary for machines which need aligned data on reads */
		}

		{
		vztint32_t * buf = alloca(lt->maxgranule * sizeof(vztint32_t));
		memset(buf, 0, lt->maxgranule * sizeof(vztint32_t));
		if(lt->rle) lt->rle_start = 0;
		vzt_wr_recurse_reorder_dict(lt->dict, lt, &newval, buf, 1);
		}
	gzflush_buffered(lt, 0);

	vzt_wr_emit_u8z(lt, (lt->multi_state)&&(lt->use_multi_state)); /* indicates number of bitplanes past twostate */
	while((lt->zpackcount & 3) != 0)
		{
		vzt_wr_emit_u8z(lt, 0);	/* pad to word boundary for machines which need aligned data on reads */
		}

	for(j=0;j<lt->numfacs;j++)
		{
		struct vzt_wr_symbol *s = lt->sorted_facs[j];
		for(i=0;i<s->len;i++)
			{
			vzt_wr_emit_u32rz(lt, s->prev[i]->val);
			}
		}

	if((lt->multi_state)&&(lt->use_multi_state))
		{
		for(j=0;j<lt->numfacs;j++)
			{
			struct vzt_wr_symbol *s = lt->sorted_facs[j];
			for(i=0;i<s->len;i++)
				{
				vzt_wr_emit_u32rz(lt, s->prevx[i]->val);
				}
			}
		lt->use_multi_state = 0;
		}
	gzflush_buffered(lt, 0);

	vzt_wr_emit_uv32z(lt, lt->numstrings);
	if(lt->numstrings)
		{
		vzt2_wr_dsvzt_Tree *ds, *ds2;
	        ds = lt->str_head;
	        for(i=0;i<lt->numstrings;i++)
	                {
	                /* fprintf(stderr, "%8d %8d) '%s'\n", ds->val, i, ds->item); */
	                if(ds->val != ((vztint32_t)i))
	                        {
	                        fprintf(stderr, "internal error line %d\n", __LINE__);
	                        exit(255);
	                        }

	                vzt_wr_emit_stringz(lt, ds->item);
	                ds2 = ds->next;
	                free(ds->item);
	                free(ds);
	                ds = ds2;
	                }
	        lt->str_head = lt->str_curr = lt->str = NULL;
		lt->numstrings = 0;
		}
	gzflush_buffered(lt, 1);

        fseeko(lt->handle, 0L, SEEK_END);
        lt->position = ftello(lt->handle);
	unclen = lt->zpackcount;
	clen = lt->position - lt->current_chunkz;
        fseeko(lt->handle, lt->current_chunk, SEEK_SET);
        vzt_wr_emit_u32(lt, unclen);        				/* size of this section (uncompressed) */
        vzt_wr_emit_u32(lt, clen);        				/* size of this section (compressed)   */

	if(!lt->rle)
		{
	        vzt_wr_emit_u64(lt, (lt->firsttime >> 32) & 0xffffffffL, lt->firsttime & 0xffffffffL); /* begin time  */
	        vzt_wr_emit_u64(lt, (lt->lasttime >> 32) & 0xffffffffL,  lt->lasttime & 0xffffffffL);  /* end time    */
		}
		else	/* inverted time is the marker the reader needs to look at to see that RLE is used */
		{
	        vzt_wr_emit_u64(lt, (lt->lasttime >> 32) & 0xffffffffL,  lt->lasttime & 0xffffffffL);  /* end time    */
	        vzt_wr_emit_u64(lt, (lt->firsttime >> 32) & 0xffffffffL, lt->firsttime & 0xffffffffL); /* begin time  */
		}
        fflush(lt->handle);

	vzt_wr_recurse_free_dict(lt->dict);
	lt->dict = NULL;
	}
}


int vzt_wr_set_time64(struct vzt_wr_trace *lt, vzttime_t timeval)
{
int rc=0;

if(lt)
	{
	if(lt->timeset)
		{
		if(timeval > lt->maxtime)
			{
			if(lt->bumptime)
				{
				lt->bumptime = 0;

				if(!lt->flush_valid)
					{
					lt->timepos++;
					}
					else
					{
					lt->flush_valid = 0;
					}

				if(lt->timepos == VZT_WR_GRANULE_SIZE)
					{
					vzt_wr_flush_granule(lt, 0);
					}
				}

			lt->timetable[lt->timepos + lt->timegranule * VZT_WR_GRANULE_SIZE] = timeval;
			lt->lasttime = timeval;
			}
		}
		else
		{
		lt->timeset = 1;
		lt->mintime = lt->maxtime = timeval;

		lt->timetable[lt->timepos + lt->timegranule * VZT_WR_GRANULE_SIZE] = timeval;
		}

	if( (!lt->timepos) && (!lt->timegranule) )
		{
		lt->firsttime = timeval;
		lt->lasttime = timeval;
		}

	lt->granule_dirty = 1;
	rc = 1;
	}

return(rc);
}


/*
 * sets trace timescale as 10**x seconds
 */
void vzt_wr_set_timescale(struct vzt_wr_trace *lt, int timescale)
{
if(lt)
	{
	lt->timescale = timescale;
	}
}


/*
 * set number of granules per section
 * (can modify dynamically but size never can decrease)
 */
void vzt_wr_set_maxgranule(struct vzt_wr_trace *lt, unsigned int maxgranule)
{
if(lt)
	{
	if(!maxgranule) maxgranule = 8;
	if(maxgranule > lt->maxgranule)
		{
		vzttime_t *t = calloc(maxgranule * VZT_WR_GRANULE_SIZE, sizeof(vzttime_t));
		memcpy(t, lt->timetable, lt->maxgranule * VZT_WR_GRANULE_SIZE * sizeof(vzttime_t));
		free(lt->timetable); lt->timetable = t;
		lt->maxgranule = maxgranule;
		}
	}
}


/*
 * Sets bracket stripping (useful for VCD conversions of
 * bitblasted nets)
 */
void vzt_wr_symbol_bracket_stripping(struct vzt_wr_trace *lt, int doit)
{
if(lt)
	{
	lt->do_strip_brackets = (doit!=0);
	}
}



static char *vzt_wr_expand_integer_to_bits(unsigned int len, int value)
{
static char s[33];
char *p = s;
unsigned int i;

if(len>32) len=32;

len--;

for(i=0;i<=len;i++)
	{
	*(p++) = '0' | ((value & (1<<(len-i)))!=0);
	}
*p = 0;

return(s);
}


int vzt_wr_emit_value_int(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, int value)
{
int rc=0;

if((!lt)||(lt->blackout)||(!s)||(row)) return(rc);

return(vzt_wr_emit_value_bit_string(lt, s, row, vzt_wr_expand_integer_to_bits(s->len, value)));
}


int vzt_wr_emit_value_double(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, double value)
{
char xdrdata[8];
#if HAVE_RPC_XDR_H
XDR x;
#else
const vztint32_t endian_matchword = 0x12345678;
#endif
vztint32_t msk, msk_n;
int i;

if((!lt)||(lt->blackout)||(!s)||(!(s->flags&VZT_WR_SYM_F_DOUBLE))||(row)) return(0);

if(!lt->emitted)
	{
	vzt_wr_emitfacs(lt);
	lt->emitted = 1;

	if(!lt->timeset)
		{
		vzt_wr_set_time(lt, 0);
		}
	}

while(s->aliased_to)	/* find root alias if exists */
	{
	s=s->aliased_to;
	}

#if HAVE_RPC_XDR_H
xdrmem_create(&x, xdrdata, sizeof(xdrdata), XDR_ENCODE);
xdr_double(&x, &value);
#else
/* byte ordering in windows is reverse of XDR (on x86, that is) */
if(*((char *)&endian_matchword) == 0x78)
	{
	for(i=0;i<8;i++)
	        {
	        xdrdata[i] = ((char *)&value)[7-i];
	        }
	}
	else
	{
	memcpy(xdrdata, &value, sizeof(double));	/* big endian, don't bytereverse */
	}
#endif

lt->bumptime = 1;

msk     = (~0U <<  lt->timepos);
msk_n   = ~msk;

for(i=0;i<s->len;i++)
	{
	int byte = i/8;
	int bit  = 7-(i&7);

	s->chg[i] &= msk_n;
	if(xdrdata[byte]&(1<<bit))
		{
		s->chg[i] |= msk;
		}
	}

lt->granule_dirty = 1;

return(1);
}


int vzt_wr_emit_value_string(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, char *value)
{
int rc=0;
int idx;
vztint32_t msk, msk_n;
int i;

if((!lt)||(lt->blackout)||(!s)||(!value)||(row)) return(rc);

if(!lt->emitted)
	{
	vzt_wr_emitfacs(lt);
	lt->emitted = 1;

	if(!lt->timeset)
		{
		vzt_wr_set_time(lt, 0);
		}
	}

while(s->aliased_to)	/* find root alias if exists */
	{
	s=s->aliased_to;
	}

lt->str = vzt2_wr_dsvzt_splay (value, lt->str);

if(!vzt2_wr_dsvzt_success)
	{
        char *vcopy = strdup(value);
	if(!lt->str_curr)
		{
	        lt->str = vzt2_wr_dsvzt_insert(strdup(""), NULL, lt->numstrings++); /* zeroth string means no value change in future blocks */
                lt->str_head =  lt->str_curr = lt->str;
		}

        lt->str = vzt2_wr_dsvzt_insert(vcopy, lt->str, lt->numstrings);
	lt->str_curr->next = lt->str;
        lt->str_curr = lt->str;

	idx = lt->numstrings;
        lt->numstrings++;
        }
        else
        {
        idx = lt->str->val;
        }


lt->bumptime = 1;

msk     = (~0U <<  lt->timepos);
msk_n   = ~msk;

for(i=0;i<s->len;i++)
	{
	s->chg[i] &= msk_n;
	if(idx & (1 << (s->len - i - 1)))
		{
		s->chg[i] |= msk;
		}
	}

lt->granule_dirty = 1;

return(rc);
}


int vzt_wr_emit_value_bit_string(struct vzt_wr_trace *lt, struct vzt_wr_symbol *s, unsigned int row, char *value)
{
int rc=0;
char *vfix;
int valuelen;
int i;
vztint32_t msk, msk_n;

if((!lt)||(lt->blackout)||(!s)||(!value)||(!*value)||(row)) return(rc);

if(!lt->emitted)
	{
	vzt_wr_emitfacs(lt);
	lt->emitted = 1;

	if(!lt->timeset)
		{
		vzt_wr_set_time(lt, 0);
		}
	}

while(s->aliased_to)	/* find root alias if exists */
	{
	s=s->aliased_to;
	}

valuelen = strlen(value);	/* ensure string is proper length */
if(valuelen == s->len)
	{
	vfix = alloca(s->len+1);
	strcpy(vfix, value);
	value = vfix;
	}
	else
	{
	vfix = alloca(s->len+1);

	if(valuelen < s->len)
		{
		int lendelta = s->len - valuelen;
		memset(vfix, (value[0]!='1') ? value[0] : '0', lendelta);
		strcpy(vfix+lendelta, value);
		}
		else
		{
		memcpy(vfix, value, s->len);
		vfix[s->len] = 0;
		}

	value = vfix;
	}

msk     = (~0U <<  lt->timepos);
msk_n   = ~msk;

if(!lt->multi_state)
	{
	for(i=0;i<s->len;i++)
		{
		unsigned char ch = value[i];
		if(ch>'1')
			{
			ch |= 1;
			}

		s->chg[i] &= msk_n;
		if(ch&1)
			{
			s->chg[i] |= msk;
			}
		}
	}
	else
	{
	for(i=0;i<s->len;i++)
		{
		/* 0 = 00, 1 = 01, x = 10, z = 11 */
		unsigned char ch = value[i];
		if((ch=='z')||(ch=='Z'))
			{
			ch |= 1;
			}
		s->chg[i] &= msk_n;
		if(ch&1)
			{
			s->chg[i] |= msk;
			}

		s->chgx[i] &= msk_n;
		if(ch>'1')
			{
			s->chgx[i] |= msk;
			}
		}
	}

lt->bumptime = 1;
lt->granule_dirty = 1;

return(rc);
}


/*
 * dumping control
 */
void vzt_wr_set_dumpoff(struct vzt_wr_trace *lt)
{
int i, j;
vztint32_t msk, msk_n;

if(lt)
	{
	msk     = (~0U <<  lt->timepos);
	msk_n   = ~msk;

	for(j=0;j<lt->numfacs;j++)
		{
		struct vzt_wr_symbol *s = lt->sorted_facs[j];
		for(i=0;i<s->len;i++)
			{
			s->chg[i] &= msk_n;
			}

		if(lt->multi_state)
			{
			if(!(s->flags & (VZT_WR_SYM_F_INTEGER|VZT_WR_SYM_F_STRING|VZT_WR_SYM_F_DOUBLE)))
				{
				for(i=0;i<s->len;i++)
					{
					s->chgx[i] |= msk;
					}
				}
			}
			else
			{
			for(i=0;i<s->len;i++)
				{
				s->chgx[i] &= msk_n;	/* simply precautionary: in case someone does assign an int to x */
				}
			}
		}

	lt->blackout = 1;
	}
}


void vzt_wr_set_dumpon(struct vzt_wr_trace *lt)
{
if(lt)
	{
	lt->blackout = 0;
	}
}


/*
 * flush the trace...
 */
void vzt_wr_flush(struct vzt_wr_trace *lt)
{
if(lt)
	{
	if((lt->timegranule)||(lt->timepos > 0))
		{
		if(lt->granule_dirty)
			{
			lt->timepos++;
			vzt_wr_flush_granule(lt, 1);
			}
		}
	}
}


/*
 * close out the trace and fixate it
 */
void vzt_wr_close(struct vzt_wr_trace *lt)
{
if(lt)
	{
	if(lt->granule_dirty)
		{
		lt->timepos++;
		vzt_wr_flush_granule(lt, 1);
		}

	if(lt->symchain)
		{
		struct vzt_wr_symbol *s = lt->symchain;
		struct vzt_wr_symbol *s2;

		while(s)
			{
			if(s->name) { free(s->name); }
			if(s->prev) { free(s->prev); }
			if(s->chg)  { free(s->chg);  }
			if(s->prevx){ free(s->prevx);}
			if(s->chgx) { free(s->chgx); }
			s2=s->symchain;
			free(s);
			s=s2;
			}

		lt->symchain = NULL;
		}

	free(lt->vztname);
	free(lt->timetable);
	free(lt->sorted_facs);
	fclose(lt->handle);
	free(lt);
	}

}

/*
 * set compression depth
 */
void vzt_wr_set_compression_depth(struct vzt_wr_trace *lt, unsigned int depth)
{
if(lt)
	{
	if(depth > 9) depth = 9;
	sprintf(lt->zmode, "wb%d", depth);
	}
}


/*
 * set compression type
 */
void vzt_wr_set_compression_type(struct vzt_wr_trace *lt, unsigned int type)
{
if(lt)
	{
	if((type == VZT_WR_IS_GZ) || (type == VZT_WR_IS_BZ2) || (type == VZT_WR_IS_LZMA))
		{
		lt->ztype_cfg = type;
		}
	}
}


/*
 * set rle mode type
 */
void vzt_wr_set_rle(struct vzt_wr_trace *lt, unsigned int mode)
{
if(lt)
	{
	lt->rle = (mode != 0);
	}
}


/*
 * time zero offset
 */
void vzt_wr_set_timezero(struct vzt_wr_trace *lt, vztsint64_t timeval)
{
if(lt)
        {
        lt->timezero = timeval;
        }
}


