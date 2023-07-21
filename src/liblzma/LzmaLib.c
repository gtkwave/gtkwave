/*
 * Copyright (c) 2009 Tony Bybell.
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
#include <ctype.h>
#ifdef _WAVE_HAVE_XZ
#include "lzma.h"
#endif
#include "LzmaLib.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

#define LZMA_BLOCK_LEN (4*1024*1024)
#define LZMA_DECODER_SIZE (256*1024*1024)

enum lzma_state_t { LZMA_STATE_WRITE, LZMA_STATE_READ_ERROR, 
			LZMA_STATE_READ_INIT, LZMA_STATE_READ_GETBLOCK, LZMA_STATE_READ_GETBYTES };

struct lzma_handle_t
{
int fd;
unsigned int offs, blklen;
unsigned int depth;
enum lzma_state_t state;
unsigned int blksiz;
unsigned char *mem, *dmem;
size_t write_cnt, read_cnt;
};


static void LZMA_write_varint(struct lzma_handle_t *h, size_t v)
{
size_t nxt;
unsigned char buf[16];
unsigned char *pnt = buf;

while((nxt = v>>7))
        {  
        *(pnt++) = (v&0x7f);
        v = nxt;
        }
*(pnt++) = (v&0x7f) | 0x80;

h->write_cnt += write(h->fd, buf, pnt-buf);
}


#ifdef _WAVE_HAVE_XZ
/* ifdef is warnings fix if XZ is not present */
static size_t LZMA_read_varint(struct lzma_handle_t *h)
{
unsigned char buf[16];
int idx = 0;
size_t rc = 0;

for(;;)
	{
	h->read_cnt += read(h->fd, buf+idx, 1);
	if(buf[idx++] & 0x80) break;
	}

do
	{
	idx--;
	rc <<= 7;
	rc |= (buf[idx] & 0x7f);
	}
	while(idx);

return(rc);
}
#endif

static size_t LZMA_write_compress(struct lzma_handle_t *h, unsigned char *mem, size_t len)
{
#ifdef _WAVE_HAVE_XZ
size_t srclen = len;
size_t destlen = h->blksiz;

lzma_stream strm = LZMA_STREAM_INIT;
lzma_options_lzma  preset;
lzma_ret lrc;
size_t wcnt;

lzma_lzma_preset(&preset, h->depth);

lrc = lzma_alone_encoder(&strm, &preset);
if(lrc != LZMA_OK)
	{
	fprintf(stderr, "Error in lzma_alone_encoder(), exiting!\n");
	exit(255);
	}

strm.next_in = mem;
strm.avail_in = len;
strm.next_out = h->dmem;
strm.avail_out = destlen;

lrc = lzma_code(&strm, LZMA_FINISH);
lzma_end(&strm);

if(((lrc == LZMA_OK)||(lrc == LZMA_STREAM_END))&&(strm.total_out<srclen))
	{
	LZMA_write_varint(h, srclen);
	LZMA_write_varint(h, strm.total_out);

	wcnt = write(h->fd, h->dmem, strm.total_out);
	h->write_cnt += wcnt;
	return(wcnt);
	}
	else
	{
	LZMA_write_varint(h, srclen);
	LZMA_write_varint(h, 0);

	wcnt = write(h->fd, mem, len);
	h->write_cnt += wcnt;
	return(wcnt);
	}
#else
(void)h;
(void)mem;
(void)len;

fprintf(stderr, "LZMA support was not compiled into this executable, sorry.\n");
exit(255);
#endif
}


void *LZMA_fdopen(int fd, const char *mode)
{
static const char z7[] = "z7";
struct lzma_handle_t *h = calloc(1, sizeof(struct lzma_handle_t)); /* scan-build flagged malloc, add to h->write_cnt below */

h->fd = fd;
h->offs = 0;
h->depth = 4;

if(mode[0] == 'w')
	{
	h->blksiz = LZMA_BLOCK_LEN;
	h->mem = malloc(h->blksiz);
	h->dmem = malloc(h->blksiz);

	if(mode[1])
		{
		if(isdigit((int)(unsigned char)mode[1]))
			{
			h->depth = mode[1] - '0';
			}
		else if(mode[2])
			{
			if(isdigit((int)(unsigned char)mode[2]))
				{
				h->depth = mode[2] - '0';
				}
			}
		}

	h->state = LZMA_STATE_WRITE;
	h->write_cnt += write(h->fd, z7, 2);
	return(h);
	}
else
if(mode[0] == 'r')
	{
	h->blksiz = 0; /* allocate as needed in the reader */
	h->mem = NULL;
	h->dmem = NULL;
	h->state = LZMA_STATE_READ_INIT;
	return(h);
	}
else
	{
	close(h->fd);
	free(h->dmem);
	free(h->mem);
	free(h);
	return(NULL);
	}
}


size_t LZMA_flush(void *handle)
{
struct lzma_handle_t *h = (struct lzma_handle_t *)handle;
if((h) && (h->offs))
	{
	LZMA_write_compress(h, h->mem, h->offs);
	h->offs = 0;
	}
return(0);
}


void LZMA_close(void *handle)
{
struct lzma_handle_t *h = (struct lzma_handle_t *)handle;
if(h)
	{
	if(h->state == LZMA_STATE_WRITE)
		{
		LZMA_flush(h);
		LZMA_write_varint(h, 0);
		}
	if(h->dmem)
		{
		free(h->dmem);
		}
	if(h->mem)
		{
		free(h->mem);
		}
	close(h->fd);
	free(h);
	}
}


size_t LZMA_write(void *handle, void *mem, size_t len)
{
struct lzma_handle_t *h = (struct lzma_handle_t *)handle;

if(h->state == LZMA_STATE_WRITE)
	{
	while((h)&&(len))
		{
		if((h->offs + len) <= h->blksiz)
			{
			memcpy(h->mem + h->offs, mem, len);
			h->offs += len;
			break;
			}
			else
			{
			size_t new_len = h->blksiz - h->offs;
			if(new_len)
				{
				memcpy(h->mem + h->offs, mem, new_len);
				}
			LZMA_write_compress(h, h->mem, h->blksiz);
			h->offs = 0;
			len -= new_len;
			mem = ((char *)mem) + new_len;
			}
		}
	}

return(len);
}


size_t LZMA_read(void *handle, void *mem, size_t len)
{
#ifdef _WAVE_HAVE_XZ
struct lzma_handle_t *h = (struct lzma_handle_t *)handle;
size_t rc = 0;
char hdr[2] = {0, 0};
size_t srclen, dstlen;

if(h)
	{
	top:
	switch(h->state)
		{
		case LZMA_STATE_READ_INIT:
			h->read_cnt += read(h->fd, hdr, 2);
			if((hdr[0] == 'z') && (hdr[1] == '7'))
				{
				h->state = LZMA_STATE_READ_GETBLOCK;
				}
				else
				{
				h->state = LZMA_STATE_READ_ERROR;
				}
			goto top;
			break;

		case LZMA_STATE_READ_GETBLOCK:
			dstlen = LZMA_read_varint(h);
			if(!dstlen)
				{
				return(0);
				}

			if(dstlen > h->blksiz) /* reallocate buffers if ones in stream data are larger */
				{
				if(h->dmem)
					{
					free(h->dmem);
					}
				if(h->mem)
					{
					free(h->mem);
					}
				h->blksiz = dstlen; 
				h->mem = malloc(h->blksiz);
				h->dmem = malloc(h->blksiz);
				}

			srclen = LZMA_read_varint(h);

			if(!srclen)
				{
				h->read_cnt += (rc = read(h->fd, h->mem, dstlen));
				h->blklen = rc;
				h->offs = 0;
				}
				else
				{
				lzma_stream strm = LZMA_STREAM_INIT;
				lzma_ret lrc;

				h->read_cnt += (rc = read(h->fd, h->dmem, srclen));

				lrc = lzma_alone_decoder(&strm, LZMA_DECODER_SIZE);
				if(lrc != LZMA_OK)
					{
					fprintf(stderr, "Error in lzma_alone_decoder(), exiting!\n");
					exit(255);
					}

				strm.next_in = h->dmem;
				strm.avail_in = srclen;
				strm.next_out = h->mem;
				strm.avail_out = h->blksiz;

				lrc = lzma_code(&strm, LZMA_RUN);
				lzma_end(&strm);

				if((lrc == LZMA_OK)||(lrc == LZMA_STREAM_END))
					{
					dstlen = strm.total_out;

					h->blklen = dstlen;
					h->offs = 0;
					}
					else
					{
					h->state = LZMA_STATE_READ_ERROR;
					goto top;
					}
				}

			if(len <= dstlen)
				{
				memcpy(mem, h->mem, len);
				h->offs = len;
				rc = len;
				h->state = LZMA_STATE_READ_GETBYTES;
				}
				else
				{
				memcpy(mem, h->mem, dstlen);
				rc = dstlen + LZMA_read(h, ((char *)mem) + dstlen, len - dstlen);
				}
			break;

		case LZMA_STATE_READ_GETBYTES:
			if((len + h->offs) < h->blklen)
				{
				memcpy(mem, h->mem + h->offs, len);
				h->offs += len;
				rc = len;
				}
			else
			if((len + h->offs) == h->blklen)
				{
				memcpy(mem, h->mem + h->offs, len);
				h->offs = 0;
				rc = len;
				h->state = LZMA_STATE_READ_GETBLOCK;
				}
			else
				{
				size_t cpylen = h->blklen - h->offs;
				memcpy(mem, h->mem + h->offs, cpylen);
				h->state = LZMA_STATE_READ_GETBLOCK;
				rc = cpylen + LZMA_read(h, ((char *)mem) + cpylen, len - cpylen);
				}
			break;

		case LZMA_STATE_READ_ERROR:
		default:
			break;
		}
	}

return(rc);

#else
(void)handle;
(void)mem;
(void)len;

fprintf(stderr, "LZMA support was not compiled into this executable, sorry.\n");
exit(255);
#endif
}
