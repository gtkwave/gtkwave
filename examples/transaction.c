/*
 * Copyright (c) 2010 Tony Bybell.
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
 * to compile: gcc -o transaction transaction.c -DHAVE_INTTYPES_H
 * then in this directory run: gtkwave transaction.gtkw
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#define OPTIONAL_DEBUG if(1)

/*
 * some structs we'll be using for event harvesting from the VCD that
 * gtkwave sends via stdin into this executable
 */
struct tim_t
{
struct tim_t *next;

uint64_t tim;
int val;
int delta;
};

struct event_t
{
struct event_t *next;

uint64_t tim;
char *name;
};


int main(void)
{
uint64_t min_time = 0, max_time = 0;
uint64_t tim = 0;
int prevtim = 0;
int prevval = 255;
struct tim_t *t_head = NULL, *t_curr = NULL;
struct tim_t *t_tmp;
int hcnt;
uint64_t control_start = 0, control_end = 0;
int blks;
int my_arg = 0;

struct event_t *hdr_head = NULL, *hdr_curr = NULL;
struct event_t *data_head = NULL, *data_curr = NULL;

OPTIONAL_DEBUG { fprintf(stderr, "*** t_filter executable init ***\n"); }

top:	/* main control loop */

hcnt = 0;
control_start = 0;
control_end = 0;
blks = 0;

while(1) /* reading input from stdin until data_end comment received */
	{
	char buf[1025];
	char *pnt = NULL;

	buf[0] = 0;
	pnt = fgets(buf, 1024, stdin); /* CACHE (VIA puts()) THIS INPUT TO A FILE TO DEVELOP YOUR CLIENT OFFLINE FROM GTKWAVE! */

	if(buf[0])
		{
		pnt = buf;
		while(*pnt) /* strip off end of line character */
			{
			if(*pnt != '\n')
				{
				pnt++;
				}
				else
				{
				*pnt = 0;
				break;
				}
			}

		if(buf[0] == '#') /* look for and extract time value */
			{
			char *str = buf+1;
			unsigned char ch;

			tim=0;
			while((ch=*(str++)))
			        {
			        if((ch>='0')&&(ch<='9'))
			                {
			                tim=(tim*10+(ch&15));
			                }
				}
			}
		else
		if(buf[0] == 'b') /* extract the binary value of the "val" symbol */
			{
			int collect = 0;
			pnt = buf+1;

			while(*pnt && (*pnt != ' '))
				{
				collect <<= 1;
				collect |= ((*pnt) & 1);
				pnt++;
				}
			
			if((prevval ^ collect) & 128)
				{
				t_tmp = calloc(1, sizeof(struct tim_t));
				t_tmp->tim = tim;
				t_tmp->val = collect;
				t_tmp->delta = tim - prevtim;

				prevtim = tim;

				if(!t_curr)
					{
					t_head = t_curr = t_tmp;
					}
					else
					{
					t_curr = t_curr->next = t_tmp;
					}
				}
			prevval = collect;
			}
		else
		if(buf[0] == '$') /* directive processing */
			{
			if(strstr(buf, "$comment"))
				{
				if((pnt = strstr(buf, "args")))
					{
					char *lhq = strchr(pnt, '\"');
					if(lhq) my_arg = atoi(lhq+1);

					OPTIONAL_DEBUG { fprintf(stderr, "args: %s\n", buf); }
					}
				else
				if((pnt = strstr(buf, "min_time")))
					{
					sscanf(pnt + 9, "%"SCNu64, &min_time);
					OPTIONAL_DEBUG { fprintf(stderr, "min: %d\n", (int)min_time); }
					}
				else
				if((pnt = strstr(buf, "max_time")))
					{
					sscanf(pnt + 9, "%"SCNu64, &max_time);
					OPTIONAL_DEBUG { fprintf(stderr, "max: %d\n", (int)max_time); }
					}
				else
				if(strstr(buf, "data_end"))
					{
					break;
					}
				}
			}
		}

	if(feof(stdin)) /* broken pipe coming up */
		{
		OPTIONAL_DEBUG { fprintf(stderr, "*** Terminated t_filter executable\n"); }
		exit(0);
		}
	}

printf("$name Decoded FSK\n"); /* 1st trace name */
printf("#0\n");

{
uint64_t p_tim = 0;
int state = 0;
int byte_remain = 0;
int bcnt = 0;

t_tmp = t_head;
while(t_tmp)
	{
	if(t_tmp->delta == 16)
		{
		int sync_cnt = 0;
		uint64_t t_start = p_tim;
		int i;
		for(i=0;i<16;i++)
			{
			if(t_tmp->delta == 16) { sync_cnt++; p_tim = t_tmp->tim; t_tmp = t_tmp->next; } else { break; }
			}
		if(sync_cnt==16)
			{
			if(!my_arg)
				{
				printf("#%"PRIu64" ?darkblue?Sync\n", t_start); /* write out sync xact */
				}
				else
				{
				printf("#%"PRIu64" ?blue?Sync\n", t_start); /* write out sync xact */
				}
			printf("#%"PRIu64"\n", p_tim - 4); 		/* 'z' midline is no value after time */
			printf("MA%"PRIu64" Start\n", t_start);		/* set position/name for marker A */
			control_start = t_start;
			goto found_sync;
			}
		continue;
		}
	p_tim = t_tmp->tim;
	t_tmp = t_tmp->next;
	}

found_sync:

state = 0; byte_remain = 11;
/* printf("MB%"PRIu64" Num Blocks\n", p_tim); */

while(t_tmp)
	{
	int i;
	int collect = 0;
	uint64_t t_start = p_tim;

	if((state == 0) && (byte_remain == 10))
		{
		struct event_t *evt = calloc(1, sizeof(struct event_t));
		char buf[32];

		evt->tim = p_tim;
		sprintf(buf, "H%d", hcnt/2);
		evt->name = strdup(buf);

		if(!control_end) { control_end = p_tim; }

		if(!hdr_head) { hdr_head = hdr_curr = evt; }
		else { hdr_curr = hdr_curr->next = evt; }

		/**/
		if(data_curr)
			{
			evt = calloc(1, sizeof(struct event_t));
			evt->tim = p_tim;
	                evt->name = NULL;

			data_curr = data_curr->next = evt;
			}
		}

	if((state == 1) && (byte_remain == 64))
		{
		struct event_t *evt = calloc(1, sizeof(struct event_t));
		char buf[32];

		evt->tim = p_tim;
		sprintf(buf, "D%d:", hcnt/2);
		evt->name = calloc(1, 74);
		strcpy(evt->name, buf);

		if(!data_head) { data_head = data_curr = evt; }
		else { data_curr = data_curr->next = evt; }

		/**/
		if(hdr_curr)
			{
			evt = calloc(1, sizeof(struct event_t));
			evt->tim = p_tim;
	                evt->name = NULL;

			hdr_curr = hdr_curr->next = evt;
			}

		hcnt++;
		}


	for(i=0;(t_tmp) && (i<8);i++)
		{
		collect <<= 1;
		if(t_tmp->delta == 16) 
			{ 
			collect |= 1; 
			t_tmp = t_tmp->next; 
			}
			else
			{
			}

		p_tim = t_tmp->tim; t_tmp = t_tmp->next;
		}

	if(!bcnt)
		{
		printf("#%"PRIu64" ?gray24?%02X\n", t_start, collect);
		blks = collect;
		bcnt++;
		}
		else
		{
		if(state == 1)
			{
			char conv = ((collect < ' ') || (collect >= 127)) ? '.' : collect;
			int slen = strlen(data_curr->name);
			
			data_curr->name[slen] = conv;

			if((collect >= ' ') && (collect < 127))
				{
				printf("#%"PRIu64" ?darkgreen?%c\n", t_start, (unsigned char)collect);
				}
				else
				{
				printf("#%"PRIu64" ?darkred?%02X\n", t_start, collect);
				}

			}
			else
			{
			printf("#%"PRIu64" ?purple3?%02X\n", t_start, collect);
			}
		}	

	printf("#%"PRIu64"\n", p_tim - 4);

	byte_remain--;
	if(!byte_remain)
		{
		if(state == 0)
			{
			state = 1; byte_remain = 64;
			}
			else
			{
			state = 0; byte_remain = 10;
			}
		}

	}

}

t_tmp = t_head;			/* free up memory allocated */
while(t_tmp)
        {
	t_curr = t_tmp->next;
	free(t_tmp);
	t_tmp = t_curr;
	}
t_head = t_curr = NULL;

printf("$next\n");		/* indicate there is a next trace */
printf("$name Control\n");	/* next trace name */
printf("#0\n");       
if(!my_arg)
	{
	printf("#%"PRIu64" ?darkblue?%02X blks\n", control_start, blks);
	}
	else
	{
	printf("#%"PRIu64" ?blue?%02X blks\n", control_start, blks);
	}
printf("#%"PRIu64"\n", control_end);


printf("$next\n");		/* indicate there is a next trace */
printf("$name Headers\n");	/* next trace name */
printf("#0\n");
while(hdr_head)
	{
	if(hdr_head->name)
		{
		printf("#%"PRIu64" ?purple3?%s\n", hdr_head->tim, hdr_head->name);
		free(hdr_head->name);
		}
		else
		{
		printf("#%"PRIu64"\n", hdr_head->tim);
		}

	hdr_curr = hdr_head->next;
	free(hdr_head);
	hdr_head = hdr_curr;
	}


printf("$next\n");		/* indicate there is a next trace */
printf("$name Data Payload\n");	/* next trace name */
printf("#0\n");
while(data_head)
	{
	if(data_head->name)
		{
		printf("#%"PRIu64" ?darkgreen?%s\n", data_head->tim, data_head->name);
		free(data_head->name);
		}
		else
		{
		printf("#%"PRIu64"\n", data_head->tim);
		}

	data_curr = data_head->next;
	free(data_head);
	data_head = data_curr;
	}



printf("$finish\n");		/* directive to return control to gtkwave */
fflush(stdout);			/* ensure nothing is stuck in output buffers which could hang gtkwave */

OPTIONAL_DEBUG { fprintf(stderr, "back to gtkwave...\n"); }

goto top;			/* loop forever in order to process next xact query */

return(0);
}
