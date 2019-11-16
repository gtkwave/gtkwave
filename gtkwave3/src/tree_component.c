/*
 * Copyright (c) Tony Bybell 2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "tree_component.h"

#ifdef _WAVE_HAVE_JUDY

/* Judy version */

void iter_through_comp_name_table(void)
{
Pvoid_t  PJArray = GLOBALS->comp_name_judy;
PPvoid_t PPValue;

if(GLOBALS->comp_name_judy)
	{
	char *mem = malloc_2(GLOBALS->comp_name_total_stringmem);
	char **idx = GLOBALS->comp_name_idx = calloc_2(GLOBALS->comp_name_serial, sizeof(char *));
	char *Index = calloc_2(GLOBALS->comp_name_longest + 1, sizeof(char));
	char *pnt = mem;

	for (PPValue  = JudySLFirst (PJArray, (uint8_t *)Index, PJE0);
	         PPValue != (PPvoid_t) NULL;
	         PPValue  = JudySLNext  (PJArray, (uint8_t *)Index, PJE0))
	    {
		int slen = strlen(Index);

		memcpy(pnt, Index, slen+1);
	        idx[(*(char **)PPValue) - ((char *)NULL)] = pnt;
		pnt += (slen + 1);
	    }

	free_2(Index);
	JudySLFreeArray(&GLOBALS->comp_name_judy, PJE0);
	GLOBALS->comp_name_judy = NULL;
	}
}


int add_to_comp_name_table(const char *s, int slen)
{
PPvoid_t PPValue = JudySLGet(GLOBALS->comp_name_judy, (uint8_t *)s, PJE0);

if(PPValue)
	{
        return((*(char **)PPValue) - ((char *)NULL) + 1);
        }

GLOBALS->comp_name_total_stringmem += (slen + 1);

if(slen > GLOBALS->comp_name_longest)
	{
	GLOBALS->comp_name_longest = slen;
	}

PPValue = JudySLIns(&GLOBALS->comp_name_judy, (uint8_t *)s, PJE0);
*((char **)PPValue) = ((char *)NULL) + GLOBALS->comp_name_serial;

return(++GLOBALS->comp_name_serial);	/* always nonzero */
}

#else

/* JRB alternate (not as memory efficient initially) */

void iter_through_comp_name_table(void)
{
if(GLOBALS->comp_name_jrb)
	{
	char *mem = malloc_2(GLOBALS->comp_name_total_stringmem);
	char **idx = GLOBALS->comp_name_idx = calloc_2(GLOBALS->comp_name_serial, sizeof(char *));
	char *Index;
	char *pnt = mem;
	JRB node;

	jrb_traverse(node, GLOBALS->comp_name_jrb)
	    {
		Index = node->key.s;
		int slen = strlen(Index);

		memcpy(pnt, Index, slen+1);
		free_2(Index);
	        idx[node->val.i] = pnt;
		pnt += (slen + 1);
	    }

	jrb_free_tree(GLOBALS->comp_name_jrb);
	GLOBALS->comp_name_jrb = NULL;
	}
}

int add_to_comp_name_table(const char *s, int slen)
{
JRB str;
Jval jv;

if(!GLOBALS->comp_name_jrb) { GLOBALS->comp_name_jrb = make_jrb(); }

str = jrb_find_str(GLOBALS->comp_name_jrb, s);

if(str)
	{
	return(str->val.i + 1);
	}

GLOBALS->comp_name_total_stringmem += (slen + 1);

if(slen > GLOBALS->comp_name_longest)
	{
	GLOBALS->comp_name_longest = slen;
	}

jv.i = GLOBALS->comp_name_serial;
jrb_insert_str(GLOBALS->comp_name_jrb, strdup_2(s), jv);

return(++GLOBALS->comp_name_serial);	/* always nonzero */
}

#endif

