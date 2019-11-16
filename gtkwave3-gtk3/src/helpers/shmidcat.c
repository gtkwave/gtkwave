/*
 * Copyright (c) 2006-2009 Tony Bybell.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined _MSC_VER && !defined __MINGW32__
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#ifdef __MINGW32__
#include <windows.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef _MSC_VER
#ifndef __MINGW32__
#include <stdint.h>
#else
#include <windows.h>
#endif
#endif


#include "wave_locale.h"

#if !defined _MSC_VER

/* size *must* match in gtkwave */
#define WAVE_PARTIAL_VCD_RING_BUFFER_SIZE (1024*1024)

char *buf_top, *buf_curr, *buf;
char *consume_ptr;


unsigned int get_8(char *p)
{
if(p >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
	{
	p-= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
	}

return((unsigned int)((unsigned char)*p));
}

unsigned int get_32(char *p)
{
unsigned int rc;

rc =	(get_8(p++) << 24);
rc |=	(get_8(p++) << 16);
rc |=	(get_8(p++) <<  8);
rc |=	(get_8(p)   <<  0);

return(rc);
}

void put_8(char *p, unsigned int v)
{
if(p >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
        {
        p -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
        }

*p = (unsigned char)v;
}

void put_32(char *p, unsigned int v)
{
put_8(p++, (v>>24));
put_8(p++, (v>>16));
put_8(p++, (v>>8));
put_8(p,   (v>>0));
}

int consume(void)	/* for testing only...similar code also is on the receiving end in gtkwave */
{
char mybuff[32769];
int rc;

if((rc = *consume_ptr))
	{
	unsigned int len = get_32(consume_ptr+1);
	unsigned int i;

	for(i=0;i<len;i++)
		{
		mybuff[i] = get_8(consume_ptr+i+5);
		}
	mybuff[i] = 0;
	printf("%s", mybuff);

	*consume_ptr = 0;
	consume_ptr = consume_ptr+i+5;
	if(consume_ptr >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
	        {
	        consume_ptr -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
	        }
	}

return(rc);
}


void emit_string(char *s)
{
int len = strlen(s);
uintptr_t l_top, l_curr;
int consumed;
int blksiz;

for(;;)
	{
	while(!*buf_top)
		{
		if((blksiz = get_32(buf_top+1)))
			{
			buf_top += 1 + 4 + blksiz;
			if(buf_top >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
			        {
			        buf_top -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
			        }
			}
			else
			{
			break;
			}
		}

	l_top = (uintptr_t)buf_top;
	l_curr = (uintptr_t)buf_curr;

	if(l_curr >= l_top)
		{
		consumed = l_curr - l_top;
		}
		else
		{
		consumed = (l_curr + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE) - l_top;
		}

	if((consumed + len + 16) > WAVE_PARTIAL_VCD_RING_BUFFER_SIZE) /* just a guardband, it's oversized */
		{
#ifdef __MINGW32__
		Sleep(10);
#else
		struct timeval tv;

	        tv.tv_sec = 0;
	        tv.tv_usec = 1000000 / 100;
	        select(0, NULL, NULL, NULL, &tv);
#endif
		continue;
		}
		else
		{
		char *ss, *sd;
		put_32(buf_curr + 1, len);

		sd = buf_curr + 1 + 4;
		ss = s;
		while(*ss)
			{
			put_8(sd, *ss);
			ss++;
			sd++;
			}
		put_8(sd, 0);	/* next valid */
		put_32(sd+1, 0);	/* next len */
		put_8(buf_curr, 1); /* current valid */

                buf_curr += 1 + 4 + len;
                if(buf_curr >= (buf + WAVE_PARTIAL_VCD_RING_BUFFER_SIZE))
                        {
                        buf_curr -= WAVE_PARTIAL_VCD_RING_BUFFER_SIZE;
                        }

		break;
		}
	}
}


/*
 * example driver code.  this merely copies from stdin to the shared memory block.
 * emit_string() will ensure that buffer overruns do not occur; all you have to
 * do is write the block with the provision that the last character in the block is
 * a newline so that the VCD parser doesn't get lost.  (in effect, when we run out
 * of buffer, gtkwave thinks it's EOF, but we restart again later.  if the last
 * character is a newline, we EOF on a null string which is OK.)
 * the shared memory ID will print on stdout.  pass that on to gtkwave for reading.
 */
int main(int argc, char **argv)
{
int buf_strlen = 0;
char l_buf[32769];
FILE *f;
#ifdef __MINGW32__
char mapName[65];
HANDLE hMapFile;
#else
struct shmid_ds ds;
#endif
int shmid;

WAVE_LOCALE_FIX

if(argc != 1)
	{
	f = fopen(argv[1], "rb");
	if(!f)
		{
		fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
		perror("Why");
		exit(255);
		}
	}
	else
	{
	f = stdin;
	}

#ifdef __MINGW32__
shmid = getpid();
sprintf(mapName, "shmidcat%d", shmid);
hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE, mapName);
if(hMapFile != NULL)
	{
	buf_top = buf_curr = buf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE);
#else
shmid = shmget(0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE, IPC_CREAT | 0600 );
if(shmid >= 0)
	{
	buf_top = buf_curr = buf = shmat(shmid, NULL, 0);
#endif
	memset(buf, 0, WAVE_PARTIAL_VCD_RING_BUFFER_SIZE);

#ifdef __linux__
	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy, linux allows queuing up destruction now */
#endif

	printf("%08X\n", shmid);
	fflush(stdout);

	consume_ptr = buf;

	while(!feof(f))
		{
		char *s = fgets(l_buf+buf_strlen, 32768-buf_strlen, f);

		if(!s)
			{
#ifdef __MINGW32__
			Sleep(200);
#else
	                struct timeval tv;

	                tv.tv_sec = 0;
	                tv.tv_usec = 1000000 / 5;
	                select(0, NULL, NULL, NULL, &tv);
#endif
			continue;
			}

		if(strchr(l_buf+buf_strlen, '\n') || strchr(l_buf+buf_strlen, '\r'))
			{
			emit_string(l_buf);
			buf_strlen = 0;
			}
			else
			{
			buf_strlen += strlen(l_buf+buf_strlen);
			/* fprintf(stderr, "update len to: %d\n", buf_strlen); */
			}
		}

#ifndef __linux__
#ifdef __MINGW32__
	UnmapViewOfFile(buf);
	CloseHandle(hMapFile);
#else
	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
#endif
#endif
	}

return(0);
}

#else

int main(int argc, char **argv)
{
#if defined _MSC_VER
fprintf(stderr, "Sorry, this doesn't run under Win32!\n");
#endif

fprintf(stderr, "If you find that this program works on your platform, please report this to the maintainers.\n");

return(255);
}

#endif


