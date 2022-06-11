/*
 * Copyright (c) 2004-2009 Tony Bybell.
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

struct namehier
{
struct namehier *next;
char *name;
char not_final;
};

static struct namehier *nhold=NULL;


void free_hier(void)
{
struct namehier *nhtemp;

while(nhold)
	{
	nhtemp=nhold->next;
	free(nhold->name);
	free(nhold);
	nhold=nhtemp;
	}
}

/*
 * navigate up and down the scope hierarchy and
 * emit the appropriate vcd scope primitives
 */
static void diff_hier(FILE *fv, struct namehier *nh1, struct namehier *nh2)
{
if(!nh2)
	{
	while((nh1)&&(nh1->not_final))
		{
		fprintf(fv, "$scope module %s $end\n", nh1->name);
		nh1=nh1->next;
		}
	return;
	}

for(;;)
	{
	if((nh1->not_final==0)&&(nh2->not_final==0)) /* both are equal */
		{
		break;
		}

	if(nh2->not_final==0)	/* old hier is shorter */
		{
		while((nh1)&&(nh1->not_final))
			{
			fprintf(fv, "$scope module %s $end\n", nh1->name);
			nh1=nh1->next;
			}
		break;
		}

	if(nh1->not_final==0)	/* new hier is shorter */
		{
		while((nh2)&&(nh2->not_final))
			{
			fprintf(fv, "$upscope $end\n");
			nh2=nh2->next;
			}
		break;
		}

	if(strcmp(nh1->name, nh2->name))
		{
		/* prune old hier */
		while((nh2)&&(nh2->not_final))
			{
			fprintf(fv, "$upscope $end\n");
			nh2=nh2->next;
			}

		/* add new hier */
		while((nh1)&&(nh1->not_final))
			{
			fprintf(fv, "$scope module %s $end\n", nh1->name);
			nh1=nh1->next;
			}
		break;
		}

	nh1=nh1->next;
	nh2=nh2->next;
	}
}


/*
 * output scopedata for a given name if needed, return pointer to name string
 */
char *fv_output_hier(FILE *fv, char *name)
{
char *pnt, *pnt2;
char *s;
int len;
struct namehier *nh_head=NULL, *nh_curr=NULL, *nhtemp;
char esc = '.';

pnt=pnt2=name;

for(;;)
{
while((*pnt2!=esc)&&(*pnt2)) { if(*pnt2=='\\') { esc = 0; } pnt2++; }
s=(char *)calloc(1,(len=pnt2-pnt)+1);
memcpy(s, pnt, len);
nhtemp=(struct namehier *)calloc(1,sizeof(struct namehier));
nhtemp->name=s;

if(!nh_curr)
	{
	nh_head=nh_curr=nhtemp;
	}
	else
	{
	nh_curr->next=nhtemp;
	nh_curr->not_final=1;
	nh_curr=nhtemp;
	}

if(!*pnt2) break;
pnt=(++pnt2);
}

diff_hier(fv, nh_head, nhold);
free_hier();
nhold=nh_head;

return(nh_curr->name);
}

