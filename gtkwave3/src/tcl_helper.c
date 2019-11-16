/*
 * Copyright (c) Tony Bybell and Concept Engineering GmbH 2008-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if WAVE_USE_GTK2
#include <glib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "gtk12compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "busy.h"
#include "debug.h"
#include "hierpack.h"
#include "menu.h"
#include "tcl_helper.h"
#include "tcl_np.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#if (defined(__MACH__) && defined(__APPLE__))
#include <mach-o/dyld.h>
#endif

/*----------------------------------------------------------------------
 * tclBackslash -- Figure out how to handle a backslash sequence in tcl list.
 *
 * Results:
 *      The return value is the character that should be substituted
 *      in place of the backslash sequence that starts at src.  If
 *      readPtr isn't NULL then it is filled in with a count of the
 *      number of characters in the backslash sequence.
 *----------------------------------------------------------------------
 */
static char tclBackslash(const char* src, int* readPtr) {
    const char* p = src+1;
    char result;
    int count = 2;

    switch (*p) {
	/*
	 * Note: in the conversions below, use absolute values (e.g.,
	 * 0xa) rather than symbolic values (e.g. \n) that get converted
	 * by the compiler.  It's possible that compilers on some
	 * platforms will do the symbolic conversions differently, which
	 * could result in non-portable Tcl scripts.
	 */
	case 'a': result = 0x7; break;
	case 'b': result = 0x8; break;
	case 'f': result = 0xc; break;
	case 'n': result = 0xa; break;
	case 'r': result = 0xd; break;
	case 't': result = 0x9; break;
	case 'v': result = 0xb; break;
	case 'x':
	    if (isxdigit((int)(unsigned char)p[1])) {
		char* end;

		result = (char)strtoul(p+1, &end, 16);
		count = end - src;
	    } else {
		count = 2;
		result = 'x';
	    }
	    break;
	case '\n':
	    do {
		p++;
	    } while ((*p == ' ') || (*p == '\t'));
	    result = ' ';
	    count = p - src;
	    break;
	case 0:
	    result = '\\';
	    count = 1;
	    break;
	default:
	    /* Check for an octal number \oo?o? */
	    if (isdigit((int)(unsigned char)*p)) {
		result = *p - '0';
		p++;
		if (!isdigit((int)(unsigned char)*p)) break;

		count = 3;
		result = (result << 3) + (*p - '0');
		p++;
		if (!isdigit((int)(unsigned char)*p)) break;

		count = 4;
		result = (result << 3) + (*p - '0');
		break;
	    }
	    result = *p;
	    count = 2;
	    break;
    }
    if (readPtr) *readPtr = count;
    return result;
}


/*----------------------------------------------------------------------
 * tclFindElement -- locate the first (or next) element in the list.
 *
 * Results:
 *  The return value is normally 1 (OK), which means that the
 *  element was successfully located.  If 0 (ERROR) is returned
 *  it means that list didn't have proper tcl list structure
 *  (no detailed error message).
 *
 *  If 1 (OK) is returned, then *elementPtr will be set to point
 *  to the first element of list, and *nextPtr will be set to point
 *  to the character just after any white space following the last
 *  character that's part of the element.  If this is the last argument
 *  in the list, then *nextPtr will point to the NULL character at the
 *  end of list.  If sizePtr is non-NULL, *sizePtr is filled in with
 *  the number of characters in the element.  If the element is in
 *  braces, then *elementPtr will point to the character after the
 *  opening brace and *sizePtr will not include either of the braces.
 *  If there isn't an element in the list, *sizePtr will be zero, and
 *  both *elementPtr and *termPtr will refer to the null character at
 *  the end of list.  Note:  this procedure does NOT collapse backslash
 *  sequences.
 *----------------------------------------------------------------------
 */
static int tclFindElement(const char* list, const char** elementPtr,
			  const char** nextPtr, int* sizePtr, int *bracePtr) {
    const char *p;
    int openBraces = 0;
    int inQuotes = 0;
    int size;

    /*
     * Skim off leading white space and check for an opening brace or
     * quote.
     */
    while (isspace((int)(unsigned char)*list)) list++;

    if (*list == '{') {			/* } */
	openBraces = 1;
	list++;
    } else if (*list == '"') {
	inQuotes = 1;
	list++;
    }
    if (bracePtr) *bracePtr = openBraces;

    /*
     * Find the end of the element (either a space or a close brace or
     * the end of the string).
     */
    for (p=list; 1; p++) {
	switch (*p) {

	    /*
	     * Open brace: don't treat specially unless the element is
	     * in braces.  In this case, keep a nesting count.
	     */
	    case '{':
		if (openBraces) openBraces++;
		break;

	    /*
	     * Close brace: if element is in braces, keep nesting
	     * count and quit when the last close brace is seen.
	     */
	    case '}':
		if (openBraces == 1) {
		    size = p - list;
		    p++;
		    if (isspace((int)(unsigned char)*p) || (*p == 0)) goto done;

		    /* list element in braces followed by garbage instead of
		     * space
		     */
		    return 0/*ERROR*/;
		} else if (openBraces) {
		    openBraces--;
		}
		break;

	    /*
	     * Backslash:  skip over everything up to the end of the
	     * backslash sequence.
	     */
	    case '\\': {
		int siz;

		tclBackslash(p, &siz);
		p += siz - 1;
		break;
	    }

	    /*
	     * Space: ignore if element is in braces or quotes;  otherwise
	     * terminate element.
	     */
	    case ' ':
	    case '\f':
	    case '\n':
	    case '\r':
	    case '\t':
	    case '\v':
		if ((openBraces == 0) && !inQuotes) {
		    size = p - list;
		    goto done;
		}
		break;

	    /*
	     * Double-quote:  if element is in quotes then terminate it.
	     */
	    case '"':
		if (inQuotes) {
		    size = p-list;
		    p++;
		    if (isspace((int)(unsigned char)*p) || (*p == 0)) goto done;

		    /* list element in quotes followed by garbage instead of
		     * space
		     */
		    return 0/*ERROR*/;
		}
		break;

	    /*
	     * End of list:  terminate element.
	     */
	    case 0:
		if (openBraces || inQuotes) {
		    /* unmatched open brace or quote in list */
		    return 0/*ERROR*/;
		}
		size = p - list;
		goto done;
	}
    }

  done:
    while (isspace((int)(unsigned char)*p)) p++;

    *elementPtr = list;
    *nextPtr = p;
    if (sizePtr) *sizePtr = size;
    return 1/*OK*/;
}


/*----------------------------------------------------------------------
 * tclCopyAndCollapse -- Copy a string and eliminate any backslashes that
 *			 aren't in braces.
 *
 * Results:
 *  Count characters get copied from src to dst. Along the way, if
 *  backslash sequences are found outside braces, the backslashes are
 *  eliminated in the copy. After scanning count chars from source, a
 *  null character is placed at the end of dst.
 *----------------------------------------------------------------------
 */
static void tclCopyAndCollapse(int count, const char *src, char *dst)
{
    char c;
    int numRead;

    for (c = *src; count > 0; src++, c = *src, count--) {
	if (c == '\\') {
	    *dst = tclBackslash(src, &numRead);
	    dst++;
	    src += numRead-1;
	    count -= numRead-1;
	} else {
	    *dst = c;
	    dst++;
	}
    }
    *dst = 0;
}


/* ----------------------------------------------------------------------------
 * zSplitTclList - Splits a list up into its constituent fields.
 *
 * Results:
 *	The return value is a pointer to a list of element points,
 *	which means that the list was successfully split up.
 *	If NULL is returned, it means that "list" didn't have proper tcl list
 *	structure (we don't return an error message about the details).
 *
 *	This procedure does allocate a single memory block
 *	by calling malloc to store both, the the argv pointer array and
 *	the extracted list elements.  The returned element
 *	pointer array must be freed by calling free_2().
 *
 *	*argcPtr will get filled in with the number of valid elements
 *	in the array.  Note: *argcPtr is only modified if the procedure
 *	returns not NULL.
 * ----------------------------------------------------------------------------
 */
char** zSplitTclList(const char* list, int* argcPtr) {
    char** argv;
    const char* l;
    char* p;
    int size, i, ok, elSize, brace;
    const char *element;

    /*
     * Figure out how much space to allocate.  There must be enough
     * space for both the array of pointers and also for a copy of
     * the list.  To estimate the number of pointers needed, count
     * the number of space characters in the list.
     */
    for (size = 1, l = list; *l != 0; l++) {
	if (isspace((int)(unsigned char)*l)) size++;
    }
    size++;				/* Leave space for final NULL */

    i = (size * sizeof(char*)) + (l - list) + 1;
    argv = malloc_2(i);

    for (i = 0, p = (char*) (argv+size); *list != 0;  i++) {
	ok = tclFindElement(list, &element, &list, &elSize, &brace);

	if (!ok) {
	    free_2(argv);
	    return NULL;
	}
	if (*element == 0) break;

	if (i >= size) {
	    free_2(argv);
	    /* internal error in zSplitTclList */
	    return NULL;
	}
	argv[i] = p;
	if (brace) {
	    strncpy(p, element, elSize);
	    p += elSize;
	    *p = 0;
	    p++;
	} else {
	    tclCopyAndCollapse(elSize, element, p);
	    p += elSize+1;
	}
    }
    argv[i] = NULL;
    *argcPtr = i;
    return argv;
}


/*----------------------------------------------------------------------
 * tclScanElement -- scan a tcl list string to see what needs to be done.
 *
 *  This procedure is a companion procedure to tclConvertElement.
 *
 * Results:
 *  The return value is an overestimate of the number of characters
 *  that will be needed by tclConvertElement to produce a valid
 *  list element from string.  The word at *flagPtr is filled in
 *  with a value needed by tclConvertElement when doing the actual
 *  conversion.
 *
 *
 * This procedure and tclConvertElement together do two things:
 *
 * 1. They produce a proper list, one that will yield back the
 * argument strings when evaluated or when disassembled with
 * zSplitTclList.  This is the most important thing.
 *
 * 2. They try to produce legible output, which means minimizing the
 * use of backslashes (using braces instead).  However, there are
 * some situations where backslashes must be used (e.g. an element
 * like "{abc": the leading brace will have to be backslashed.  For
 * each element, one of three things must be done:
 *
 * (a) Use the element as-is (it doesn't contain anything special
 * characters).  This is the most desirable option.
 *
 * (b) Enclose the element in braces, but leave the contents alone.
 * This happens if the element contains embedded space, or if it
 * contains characters with special interpretation ($, [, ;, or \),
 * or if it starts with a brace or double-quote, or if there are
 * no characters in the element.
 *
 * (c) Don't enclose the element in braces, but add backslashes to
 * prevent special interpretation of special characters.  This is a
 * last resort used when the argument would normally fall under case
 * (b) but contains unmatched braces.  It also occurs if the last
 * character of the argument is a backslash or if the element contains
 * a backslash followed by newline.
 *
 * The procedure figures out how many bytes will be needed to store
 * the result (actually, it overestimates).  It also collects information
 * about the element in the form of a flags word.
 *----------------------------------------------------------------------
 */
#define DONT_USE_BRACES  1
#define USE_BRACES       2
#define BRACES_UNMATCHED 4

static int tclScanElement(const char* string, int* flagPtr) {
    const char *p;
    int nestingLevel = 0;
    int flags = 0;

    if (string == NULL) string = "";

    p = string;
    if ((*p == '{') || (*p == '"') || (*p == 0)) {	/* } */
	flags |= USE_BRACES;
    }
    for ( ; *p != 0; p++) {
	switch (*p) {
	    case '{':
		nestingLevel++;
		break;
	    case '}':
		nestingLevel--;
		if (nestingLevel < 0) {
		    flags |= DONT_USE_BRACES | BRACES_UNMATCHED;
		}
		break;
	    case '[':
	    case '$':
	    case ';':
	    case ' ':
	    case '\f':
	    case '\r':
	    case '\t':
	    case '\v':
		flags |= USE_BRACES;
		break;
	    case '\n':		/* lld: dont want line breaks inside a list */
		flags |= DONT_USE_BRACES;
		break;
	    case '\\':
		if ((p[1] == 0) || (p[1] == '\n')) {
		    flags = DONT_USE_BRACES | BRACES_UNMATCHED;
		} else {
		    int size;

		    tclBackslash(p, &size);
		    p += size-1;
		    flags |= USE_BRACES;
		}
		break;
	}
    }
    if (nestingLevel != 0) {
	flags = DONT_USE_BRACES | BRACES_UNMATCHED;
    }
    *flagPtr = flags;

    /* Allow enough space to backslash every character plus leave
     * two spaces for braces.
     */
    return 2*(p-string) + 2;
}


/*----------------------------------------------------------------------
 *
 * tclConvertElement - convert a string into a list element
 *
 *  This is a companion procedure to tclScanElement.  Given the
 *  information produced by tclScanElement, this procedure converts
 *  a string to a list element equal to that string.
 *
 *  See the comment block at tclScanElement above for details of how this
 *  works.
 *
 * Results:
 *  Information is copied to *dst in the form of a list element
 *  identical to src (i.e. if zSplitTclList is applied to dst it
 *  will produce a string identical to src).  The return value is
 *  a count of the number of characters copied (not including the
 *  terminating NULL character).
 *----------------------------------------------------------------------
 */
static int tclConvertElement(const char* src, char* dst, int flags)
{
    char *p = dst;

    if ((src == NULL) || (*src == 0)) {
	p[0] = '{';
	p[1] = '}';
	p[2] = 0;
	return 2;
    }
    if ((flags & USE_BRACES) && !(flags & DONT_USE_BRACES)) {
	*p = '{';
	p++;
	for ( ; *src != 0; src++, p++) {
	    *p = *src;
	}
	*p = '}';
	p++;
    } else {
	if (*src == '{') {		/* } */
	    /* Can't have a leading brace unless the whole element is
	     * enclosed in braces.  Add a backslash before the brace.
	     * Furthermore, this may destroy the balance between open
	     * and close braces, so set BRACES_UNMATCHED.
	     */
	    p[0] = '\\';
	    p[1] = '{';			/* } */
	    p += 2;
	    src++;
	    flags |= BRACES_UNMATCHED;
	}
	for (; *src != 0 ; src++) {
	    switch (*src) {
		case ']':
		case '[':
		case '$':
		case ';':
		case ' ':
		case '\\':
		case '"':
		    *p = '\\';
		    p++;
		    break;
		case '{':
		case '}':
		    /* It may not seem necessary to backslash braces, but
		     * it is.  The reason for this is that the resulting
		     * list element may actually be an element of a sub-list
		     * enclosed in braces, so there may be a brace mismatch
		     * if the braces aren't backslashed.
		     */
		    if (flags & BRACES_UNMATCHED) {
			*p = '\\';
			p++;
		    }
		    break;
		case '\f':
		    *p = '\\';
		    p++;
		    *p = 'f';
		    p++;
		    continue;
		case '\n':
		    *p = '\\';
		    p++;
		    *p = 'n';
		    p++;
		    continue;
		case '\r':
		    *p = '\\';
		    p++;
		    *p = 'r';
		    p++;
		    continue;
		case '\t':
		    *p = '\\';
		    p++;
		    *p = 't';
		    p++;
		    continue;
		case '\v':
		    *p = '\\';
		    p++;
		    *p = 'v';
		    p++;
		    continue;
	    }
	    *p = *src;
	    p++;
	}
    }
    *p = '\0';
    return p-dst;
}


/* ============================================================================
 * zMergeTclList - Creates a tcl list from a set of element strings.
 *
 *	Given a collection of strings, merge them together into a
 *	single string that has proper Tcl list structured (i.e.
 *	zSplitTclList may be used to retrieve strings equal to the
 *	original elements).
 *	The merged list is stored in dynamically-allocated memory.
 *
 * Results:
 *      The return value is the address of a dynamically-allocated string.
 * ============================================================================
 */
char* zMergeTclList(int argc, const char** argv) {
    enum  {LOCAL_SIZE = 20};
    int   localFlags[LOCAL_SIZE];
    int*  flagPtr;
    int   numChars;
    int   i;
    char* result;
    char* dst;

    /* Pass 1: estimate space, gather flags */
    if (argc <= LOCAL_SIZE) flagPtr = localFlags;
    else                    flagPtr = malloc_2(argc*sizeof(int));
    numChars = 1;

    for (i=0; i<argc; i++) numChars += tclScanElement(argv[i], &flagPtr[i]) + 1;

    result = malloc_2(numChars);

    /* Pass two: copy into the result area */
    dst = result;
    for (i = 0; i < argc; i++) {
	numChars = tclConvertElement(argv[i], dst, flagPtr[i]);
	dst += numChars;
	*dst = ' ';
	dst++;
    }
    if (dst == result) *dst = 0;
    else                dst[-1] = 0;

    if (flagPtr != localFlags) free_2(flagPtr);
    return result;
}


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX functions for data coming into gtkwave XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */


/* ----------------------------------------------------------------------------
 * determine_trace_from_y - finds trace under the marker
 *
 * Results:
 *      Returns Trptr which corresponds to the mouse pointer y position.
 * ----------------------------------------------------------------------------
 */

static Trptr determine_trace_from_y(void)
{
Trptr t;
int trwhich, trtarget;
GdkModifierType state;
gdouble x, y;
#ifdef WAVE_USE_GTK2
gint xi, yi;
#else
GdkEventMotion event[1];
event[0].deviceid = GDK_CORE_POINTER;
#endif


if(GLOBALS->dnd_tgt_on_signalarea_treesearch_gtk2_c_1)
	{
	WAVE_GDK_GET_POINTER(GLOBALS->signalarea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	if((x<0)||(y<0)||(x>=GLOBALS->signalarea->allocation.width)||(y>=GLOBALS->signalarea->allocation.height)) return(NULL);
	}
else
if(GLOBALS->dnd_tgt_on_wavearea_treesearch_gtk2_c_1)
	{
	WAVE_GDK_GET_POINTER(GLOBALS->wavearea->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	if((x<0)||(y<0)||(x>=GLOBALS->wavearea->allocation.width)||(y>=GLOBALS->wavearea->allocation.height)) return(NULL);
	}
else
	{
	return(NULL);
	}

if((t=GLOBALS->traces.first))
        {
        while(t)
                {
                t->flags&=~TR_HIGHLIGHT;
                t=t->t_next;
                }
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

trtarget = ((int)y / (int)GLOBALS->fontheight) - 2;
if(trtarget < 0)
	{
	return(NULL);
	}
	else
	{
	t=GLOBALS->topmost_trace;
	}

trwhich=0;
while(t)
	{
        if((trwhich<trtarget)&&(GiveNextTrace(t)))
        	{
                trwhich++;
                t=GiveNextTrace(t);
                }
                else
                {
                break;
                }
	}

return(t);
}


/* ----------------------------------------------------------------------------
 * check_gtkwave_directive_from_tcl_list - parses tcl list for any gtkwave
 * directives
 *
 * Results:
 *      Returns decomposed list or NULL if not applicable.  Number of items
 *      is passed back through pointer in l.
 * ----------------------------------------------------------------------------
 */

static char **check_gtkwave_directive_from_tcl_list(char *s, int *l)
{
char** elem = NULL;

elem = zSplitTclList(s, l);

if(elem)
        {
        if((!elem[0]) || strcmp("gtkwave", elem[0])) /* scan-build: elem[0] is NULL */
		{
		free_2(elem);
		elem = NULL;
		}
	}
return(elem);
}


/* ----------------------------------------------------------------------------
 * make_net_name_from_tcl_list - creates gtkwave-style name from tcl list entry
 *
 * Results:
 *      Returns suitable name or NULL if not applicable.
 * ----------------------------------------------------------------------------
 */

static char *make_net_name_from_tcl_list(char *s, char **unescaped_str)
{
char *s_new = NULL;
char *s_new2 = NULL;
int len;
int i,l;
char** elem = NULL;
char *pnt, *pnt2;
int esc = 0;

if(!s)
	{
	return(NULL);
	}

elem = zSplitTclList(s, &l);

if(elem)
	{
	if ((elem[0]) &&  ((!strcmp("net", elem[0])) || (!strcmp("netBus", elem[0])))) /* scan-build: elem[0] can be NULL */
		{
		char delim_str[2];

		delim_str[0] = GLOBALS->hier_delimeter;
		delim_str[1] = 0;

		len = 0;
		for(i=1;i<l;i++)
			{
			len += (strlen(elem[i])) + 1;
			}

		s_new = calloc_2(1, len);
		for(i=1;i<l;i++)
			{
			strcat(s_new, elem[i]);
			if(i!=(l-1)) strcat(s_new, delim_str);
			}

		free_2(elem);
		}
		else
		{
		free_2(elem);
		return(NULL);
		}

	pnt = s_new;
	while(*pnt)
		{
		if( (!isalnum((int)(unsigned char)*pnt)) && (!isspace((int)(unsigned char)*pnt)) && (*pnt != GLOBALS->hier_delimeter) )
			{
			esc++;
			}
		pnt++;
		}

	if(esc)
		{
		s_new2 = calloc_2(1, len + esc);
		pnt = s_new;
		pnt2 = s_new2;
		while(*pnt)
			{
			if( (!isalnum((int)(unsigned char)*pnt)) && (!isspace((int)(unsigned char)*pnt)) && (*pnt != GLOBALS->hier_delimeter) )
				{
				*(pnt2++) = '\\';
				}

			*(pnt2++) = *(pnt++);
			}
		*unescaped_str = s_new;
		/* free_2(s_new); */
		s_new = s_new2;
		}
		else
		{
		*unescaped_str = s_new;
		}
	}

return(s_new);
}


/* ----------------------------------------------------------------------------
 * process_tcl_list - given a tcl list, inserts traces into viewer
 *
 * Results:
 *      Inserts traces if found in dumpfile, returns number of traces inserted
 * ----------------------------------------------------------------------------
 */

void process_tcl_list_2(struct symbol *s, int which_msb, int which_lsb)
{
Trptr t;
nptr nexp;
int i;
TraceFlagsType default_flags = GLOBALS->default_flags;
bvptr v;

Trptr    buffer;            /* cut/copy buffer of traces */
Trptr    bufferlast;        /* last element of bufferchain */
int      buffercount;       /* number of traces in buffer */

GLOBALS->default_flags = TR_HIGHLIGHT;

buffer = GLOBALS->traces.buffer; /* copy cut buffer to make re-entrant */
bufferlast = GLOBALS->traces.bufferlast;
buffercount = GLOBALS->traces.buffercount;

GLOBALS->traces.buffer = NULL;
GLOBALS->traces.bufferlast = NULL;
GLOBALS->traces.buffercount = 0;

t=GLOBALS->traces.first;
while(t)
        {
        if(t->flags&TR_HIGHLIGHT)	/* copy TR_HIGHLIGHT->TR_RSVD and clear TR_HIGHLIGHT */
		{
	        t->flags&=(~TR_HIGHLIGHT);
		t->flags|=TR_RSVD;
		}
		else
		{
	        t->flags&=(~TR_RSVD);
		}

        t=t->t_next;
        }


if(which_msb <= which_lsb)
	{
	for(i=which_msb;i<=which_lsb;i++)
		{
	        nexp = ExtractNodeSingleBit(s->n, i);
	        if(nexp)  
	 		{
	                AddNode(nexp, NULL);
	                }
	                else
	                {
	                AddNodeUnroll(s->n, NULL);
	                }
		}
	}
	else
	{
	for(i=which_msb;i>=which_lsb;i--)
		{
	        nexp = ExtractNodeSingleBit(s->n, i);
	        if(nexp)  
	 		{
	                AddNode(nexp, NULL);
	                }
	                else
	                {
	                AddNodeUnroll(s->n, NULL);
	                }
		}
	}

v = combine_traces(1, NULL); /* down */
  if (v)
    	{
      	AddVector(v, NULL);
      	free_2(v->bits->name);
      	v->bits->name=NULL;
                
      	t = GLOBALS->traces.last;
                
      	RemoveTrace(t, 0);

      	/* t is now the composite signal trace */
                  
	create_group("unused_0", t);
	CloseTrace(t);
    	}

t=GLOBALS->traces.first;
while(t)
        {
	t->flags &= ~TR_HIGHLIGHT;

        if(t->flags&TR_RSVD) /* copy TR_RSVD->TR_HIGHLIGHT and clear TR_RSVD */
		{
	        t->flags|=TR_HIGHLIGHT;
		t->flags&=(~TR_RSVD);
		}

        t=t->t_next;
        }

GLOBALS->traces.buffer = buffer; /* restore cut buffer */
GLOBALS->traces.bufferlast = bufferlast;
GLOBALS->traces.buffercount = buffercount;

GLOBALS->default_flags = default_flags;
}


int process_tcl_list(char *sl, gboolean track_mouse_y)
{
char *s_new = NULL;
char *this_regex = "\\(\\[.*\\]\\)*$";
char *entry_suffixed;
int c, i, ii;
char **list;
char **s_new_list;
char **most_recent_lbrack_list;
char **most_recent_colon_list;
int *match_idx_list;
int *match_type_list;
Trptr t = NULL;
int found = 0;
int lbrack_adj;
int net_processing_is_off = 0;
int unesc_len;
int curr_srch_idx = 0;
char *unescaped_str = NULL;

if(!sl)
	{
	return(0);
	}

list = zSplitTclList(sl, &c);
if(!list)
	{
	return(0);
	}

read_save_helper_relative_init(NULL); /* should be passing canonicalized filter names here...so no need for relative processing */

s_new_list = calloc_2(c, sizeof(char *));
match_idx_list = calloc_2(c, sizeof(int *));
match_type_list = calloc_2(c, sizeof(int *));
most_recent_lbrack_list = calloc_2(c, sizeof(char *));
most_recent_colon_list = calloc_2(c, sizeof(char *));

GLOBALS->default_flags=TR_RJUSTIFY;
GLOBALS->default_fpshift=0;

GLOBALS->strace_current_window = 0; /* in case there are shadow traces; in reality this should never happen */

for(ii=0;ii<c;ii++)
	{
	s_new = make_net_name_from_tcl_list(list[ii], &unescaped_str);
	if(s_new)
		{
		if(net_processing_is_off) continue;
		}
	else
		{
		int ngl;
		char **gdirect = check_gtkwave_directive_from_tcl_list(list[ii], &ngl);
		if(gdirect)
			{
			switch(ngl)
				{
				case 3:
				 	if(!strcmp(gdirect[1], "PID"))
						{
						pid_t pid = atoi(gdirect[2]);
						if(pid == getpid())	/* block self-drags in XEmbed */
							{
							free_2(gdirect);
							goto cleanup;
							}
						}
					 else if(!strcmp(gdirect[1], "NET"))
						{
						net_processing_is_off = !strcmp(gdirect[2], "OFF");
						}
					 else if(!strcmp(gdirect[1], "SAVELIST"))
						{
						int is;
						for(is = 0; is < 4; is++)
							{
							char *pnt = gdirect[2];
							char *nxt_hd = pnt;

							if(is == 1)
								{
								if(found)
									{
							                if(GLOBALS->is_lx2)
										{
										lx2_import_masked();
										}

									if(track_mouse_y)
										{
										t = determine_trace_from_y();
										if(t)
											{
											t->flags |=  TR_HIGHLIGHT;
											}
										}

									memcpy(&GLOBALS->tcache_treesearch_gtk2_c_2,&GLOBALS->traces,sizeof(Traces));
									GLOBALS->traces.total=0;
									GLOBALS->traces.first=GLOBALS->traces.last=NULL;

									continue;
									}
									else
									{
									goto cleanup;
									}
								}
							else
							if(is==3)
								{
								goto paste_routine;
								}
							else /* (is == 0) or (is == 2) */
							for(;;)
								{
								if(*pnt == 0)
									{
									if(!(*nxt_hd))
										{
										break;
										}

									if((!is)&&(GLOBALS->is_lx2)) { parsewavline_lx2(nxt_hd, NULL, 0); found++; } else { parsewavline(nxt_hd, NULL, 0); }
									break;
									}
								else
								if(*pnt == '\n')
									{
									*pnt = 0;
									if((!is)&&(GLOBALS->is_lx2)) { parsewavline_lx2(nxt_hd, NULL, 0); found++; } else { parsewavline(nxt_hd, NULL, 0); }
									*pnt = '\n';
									nxt_hd = pnt+1;
									pnt++;
									}
								else
									{
									pnt++;
									}
								}
							}
						}
					 break;

				default: break;
				}

			free_2(gdirect);
			}

		continue;
		}
	s_new_list[ii] = s_new;

	lbrack_adj = 0;
	most_recent_lbrack_list[ii] = strrchr(s_new, '[');
	if((most_recent_lbrack_list[ii])&&(most_recent_lbrack_list[ii] != s_new))
		{
		char *chp = most_recent_lbrack_list[ii]-1;
		if(*chp == '\\')
			{
			most_recent_lbrack_list[ii] = chp;
			lbrack_adj = 1;
			}

		most_recent_colon_list[ii]=strchr(most_recent_lbrack_list[ii], ':');
		}

	unesc_len = strlen(unescaped_str);
	for(i=0;i<GLOBALS->numfacs;i++)
	        {
	        int was_packed = HIER_DEPACK_ALLOC;
	        char *hfacname = NULL;

       		hfacname = hier_decompress_flagged(GLOBALS->facs[curr_srch_idx]->name, &was_packed);

	        if(!strncmp(unescaped_str, hfacname, unesc_len))
	                {
			int hfacname_len = strlen(hfacname);
			if((unesc_len == hfacname_len) || ((hfacname_len > unesc_len) && (hfacname[unesc_len] == '[')))
				{
				found++;
				match_idx_list[ii] = curr_srch_idx;
				match_type_list[ii] = 1; /* match was on normal search */
			        if(was_packed) { free_2(hfacname); }
				if(s_new != unescaped_str) { free_2(unescaped_str); }
				goto import;
		                }
			}

		curr_srch_idx++;
		if(curr_srch_idx == GLOBALS->numfacs) curr_srch_idx = 0; /* optimization for rtlbrowse as names should be in order */

	        if(was_packed) { free_2(hfacname); }
	        }

	if(s_new != unescaped_str) { free_2(unescaped_str); }

	entry_suffixed=wave_alloca(2+strlen(s_new)+strlen(this_regex)+1);
	*entry_suffixed=0x00;
	strcpy(entry_suffixed, "\\<");
	strcat(entry_suffixed,s_new);
	strcat(entry_suffixed,this_regex);

	wave_regex_compile(entry_suffixed, WAVE_REGEX_DND);
	for(i=0;i<GLOBALS->numfacs;i++)
	        {
	        int was_packed = HIER_DEPACK_ALLOC;
	        char *hfacname = NULL;

       		hfacname = hier_decompress_flagged(GLOBALS->facs[i]->name, &was_packed);

	        if(wave_regex_match(hfacname, WAVE_REGEX_DND))
	                {
			found++;
			match_idx_list[ii] = i;
			match_type_list[ii] = 1; /* match was on normal search */
		        if(was_packed) { free_2(hfacname); }
			goto import;
	                }

	        if(was_packed) { free_2(hfacname); }
	        }

	if(most_recent_lbrack_list[ii])
		{
		*most_recent_lbrack_list[ii] = 0;

		entry_suffixed=wave_alloca(2+strlen(s_new)+strlen(this_regex)+1);
		*entry_suffixed=0x00;
		strcpy(entry_suffixed, "\\<");
		strcat(entry_suffixed,s_new);
		strcat(entry_suffixed,this_regex);

		wave_regex_compile(entry_suffixed, WAVE_REGEX_DND);
		for(i=0;i<GLOBALS->numfacs;i++)
		        {
		        int was_packed = HIER_DEPACK_ALLOC;
		        char *hfacname = NULL;

        		hfacname = hier_decompress_flagged(GLOBALS->facs[i]->name, &was_packed);

		        if(wave_regex_match(hfacname, WAVE_REGEX_DND))
		                {
				found++;
				match_idx_list[ii] = i;
				match_type_list[ii] = 2+lbrack_adj; /* match was on lbrack removal */
			        if(was_packed) { free_2(hfacname); }
				goto import;
		                }

		        if(was_packed) { free_2(hfacname); }
		        }
		}

	import:
	if(match_type_list[ii])
		{
		struct symbol *s = GLOBALS->facs[match_idx_list[ii]];
		struct symbol *schain = s->vec_root;

		if(GLOBALS->is_lx2)
			{
			if(schain)
				{
				while(schain)
					{
					lx2_set_fac_process_mask(schain->n);
					schain = schain-> vec_chain;
					}
				}
				else
				{
	                        lx2_set_fac_process_mask(s->n);
				}
			}
		}
	}

if(!found) goto cleanup;

if(GLOBALS->is_lx2)
	{
	lx2_import_masked();
	}

if(track_mouse_y)
	{
	t = determine_trace_from_y();
	if(t)
		{
		t->flags |=  TR_HIGHLIGHT;
		}
	}

memcpy(&GLOBALS->tcache_treesearch_gtk2_c_2,&GLOBALS->traces,sizeof(Traces));
GLOBALS->traces.total=0;
GLOBALS->traces.first=GLOBALS->traces.last=NULL;

for(ii=0;ii<c;ii++)
	{
	if(match_type_list[ii])
		{
		struct symbol *s = GLOBALS->facs[match_idx_list[ii]];

		if((match_type_list[ii] >= 2)&&(s->n->extvals))
			{
			nptr nexp;

			int bit_msb = atoi(most_recent_lbrack_list[ii]+1 + (match_type_list[ii] == 3)); /* == 3 for adjustment when lbrack is escaped */
			int bit_lsb; /* = bit_msb; scan-build: never read */
			int which_msb, which_lsb, cnt;

			if(s->n->lsi > s->n->msi)
				{
				for(which_msb=0,cnt=s->n->msi ; cnt<=s->n->lsi ; cnt++,which_msb++)
					{
					if(cnt==bit_msb) break;
					}
				}
				else
				{
				for(which_msb=0,cnt=s->n->msi ; cnt>=s->n->lsi ; cnt--,which_msb++)
					{
					if(cnt==bit_msb) break;
					}
				}
			which_lsb = which_msb;

			/* Need to fix this to extract more than a single bit as in the case of a subrange of an existing vector! */
			if(most_recent_colon_list[ii])
				{
				bit_lsb = atoi(most_recent_colon_list[ii]+1);

				if(s->n->lsi > s->n->msi)
					{
					for(which_lsb=0,cnt=s->n->msi ; cnt<=s->n->lsi ; cnt++,which_lsb++)
						{
						if(cnt==bit_lsb) break;
						}
					}
					else
					{
					for(which_lsb=0,cnt=s->n->msi ; cnt>=s->n->lsi ; cnt--,which_lsb++)
						{
						if(cnt==bit_lsb) break;
						}
					}
				}

			if(which_msb == which_lsb)
				{
				nexp = ExtractNodeSingleBit(s->n, which_msb);
				*most_recent_lbrack_list[ii] = '[';
		                if(nexp)
		                        {
		                        AddNode(nexp, NULL);
		                        }
					else
					{
					AddNodeUnroll(s->n, NULL);
					}
				}
				else
				{
				process_tcl_list_2(s, which_msb, which_lsb); /* is complicated, so split out to its own function */
				}
			}
			else
			{
			struct symbol *schain = s->vec_root;

			if(!schain)
				{
				AddNodeUnroll(s->n, NULL);
				}
				else
				{
				int len = 0;
				while(schain) { len++; schain = schain->vec_chain; }
				add_vector_chain(s->vec_root, len);
				}
			}
		}
	}

paste_routine:

GLOBALS->default_flags=TR_RJUSTIFY;
GLOBALS->default_fpshift=0;

GLOBALS->traces.buffercount=GLOBALS->traces.total;
GLOBALS->traces.buffer=GLOBALS->traces.first;
GLOBALS->traces.bufferlast=GLOBALS->traces.last;
GLOBALS->traces.first=GLOBALS->tcache_treesearch_gtk2_c_2.first;
GLOBALS->traces.last=GLOBALS->tcache_treesearch_gtk2_c_2.last;
GLOBALS->traces.total=GLOBALS->tcache_treesearch_gtk2_c_2.total;

if((t) || (!track_mouse_y))
	{
	PasteBuffer();
	}
	else
	{
	PrependBuffer();
	}

GLOBALS->traces.buffercount=GLOBALS->tcache_treesearch_gtk2_c_2.buffercount;
GLOBALS->traces.buffer=GLOBALS->tcache_treesearch_gtk2_c_2.buffer;
GLOBALS->traces.bufferlast=GLOBALS->tcache_treesearch_gtk2_c_2.bufferlast;

if(track_mouse_y)
	{
	if(t)
		{
		t->flags &= ~TR_HIGHLIGHT;
		}
	}

cleanup:
for(ii=0;ii<c;ii++)
	{
	if(s_new_list[ii]) free_2(s_new_list[ii]);
	}
free_2(s_new_list);
free_2(match_idx_list);
free_2(match_type_list);
free_2(most_recent_colon_list);
free_2(most_recent_lbrack_list);
free_2(list);

EnsureGroupsMatch();

return(found);
}


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX functions for data exiting from gtkwave XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */


/* ----------------------------------------------------------------------------
 * make_gtkwave_pid - generates gtkwave pid (necessary when using twinwave as
 * the XEmbed protocol seems to be dropping the source widget for drags which
 * causes drops to occur twice), this should only need to be called by
 * add_dnd_from_signal_window().
 *
 * Results:
 *      generated tcl list string containing gtkwave PID for drop filtering
 *      also contains current marker time
 * ----------------------------------------------------------------------------
 */

static char *make_gtkwave_pid(void)
{
#if !defined __MINGW32__ && !defined _MSC_VER
char pidstr[257];

sprintf(pidstr, "{gtkwave PID %d} ", getpid());

if(GLOBALS->tims.marker != -1)
	{
	char mrkbuf[128];
	reformat_time(mrkbuf, GLOBALS->tims.marker, GLOBALS->time_dimension);
	sprintf(pidstr+strlen(pidstr), "{marker %s} ", mrkbuf);
	}

return(strdup_2(pidstr));
#else
return(NULL);
#endif
}


/* ----------------------------------------------------------------------------
 * make_single_tcl_list_name - generates tcl name from a gtkwave one
 *
 * Results:
 *      generated tcl list string
 * ----------------------------------------------------------------------------
 */

char *make_single_tcl_list_name(char *s, char *opt_value, int promote_to_bus, int preserve_range)
{
char *rpnt = NULL;
char *pnt, *pnt2;
int delim_cnt = 0;
char *lbrack=NULL, *colon=NULL, *rbrack=NULL;
const char **names = NULL;
char *tcllist = NULL;
int tcllist_len;
int names_idx = 0;
char is_bus = 0;

if(s)
	{
	int len;
	int was_packed = HIER_DEPACK_ALLOC;
	char *s2;

	s = hier_decompress_flagged(s, &was_packed);
	len = strlen(s);
	s2 = wave_alloca(len+1);

	strcpy(s2, s);
	if(was_packed)
		{
		free_2(s);
		s = NULL;
		}

	pnt = s2;

	while(*pnt)
		{
		if(*pnt == GLOBALS->hier_delimeter)
			{
			delim_cnt++;
			}
		else if(*pnt == '[') { lbrack = pnt; }
		else if(*pnt == ':') { colon  = pnt; }
		else if(*pnt == ']') { rbrack = pnt; }

		pnt++;
		}

	if(!preserve_range) /* added for gtkwave::addSignalsFromList */
		{
		if((lbrack && colon && rbrack && ((colon-lbrack)>0) && ((rbrack - colon)>0) && ((rbrack-lbrack)>0)) || (lbrack && promote_to_bus))
			{
			is_bus = 1;
			*lbrack = 0;
			/* len = lbrack - s2; */ /* scan-build */
			}
		}

	names = calloc_2(delim_cnt+1, sizeof(char *));
	pnt = s2;
	names[0] = pnt;
	while(*pnt)
		{
		if(*pnt == GLOBALS->hier_delimeter)
			{
			*pnt = 0;
			names_idx++;
			pnt++;
			if(*pnt) { names[names_idx] = pnt; }
			}
			else
			{
			pnt++;
			}
		}


	tcllist = zMergeTclList(delim_cnt+1, names);
	tcllist_len = strlen(tcllist);
	free_2(names);

	if(!opt_value)
		{
		if(is_bus)
			{
			len = 8 + strlen(tcllist) + 1 + 1 + 1; /* "{netBus ...} " + trailing null char */

			/* pnt = s2; */ /* scan-build */
			rpnt = malloc_2(len+1);
			strcpy(rpnt, "{netBus ");
			pnt2 = rpnt + 8;
			}
			else
			{
			len = 5 + strlen(tcllist) + 1 + 1 + 1; /* "{net ...} " + trailing null char */

			/* pnt = s2; */ /* scan-build */
			rpnt = malloc_2(len+1);
			strcpy(rpnt, "{net ");
			pnt2 = rpnt + 5;
			}
		}
		else
		{
		int len_value = strlen(opt_value);

		if(is_bus)
			{
			len = 15 + (len_value + 1) + strlen(tcllist) + 1 + 1 + 1; /* "{netBusValue 0x...} " + trailing null char */

			/* pnt = s2; */ /* scan-build */
			rpnt = malloc_2(len+1);
			sprintf(rpnt, "{netBusValue 0x%s ", opt_value);
			pnt2 = rpnt + 15 + (len_value + 1);
			}
			else
			{
			len = 10 + (len_value + 1) + strlen(tcllist) + 1 + 1 + 1; /* "{netValue ...} " + trailing null char */

			/* pnt = s2; */ /* scan-build */
			rpnt = malloc_2(len+1);
			sprintf(rpnt, "{netValue %s ", opt_value);
			pnt2 = rpnt + 10 + (len_value + 1);
			}
		}

	strcpy(pnt2, tcllist);
	strcpy(pnt2 + tcllist_len, "} ");

	free_2(tcllist);
	}

return(rpnt);
}


/* ----------------------------------------------------------------------------
 * give_value_string - generates value string from trace @ markertime
 *
 * Results:
 *      generated value which is similar to that generated in the signalwindow
 *      pane when the marker button is pressed.  note that this modifies the
 *      flags so it is always TR_RJUSTIFY | TR_HEX
 * ----------------------------------------------------------------------------
 */
static char *give_value_string(Trptr t)
{
char *rc = NULL;
TraceFlagsType flags;
int f_filter, p_filter;

if(t)
	{
	flags = t->flags;
	f_filter = t->f_filter;
	p_filter = t->p_filter;

	t->flags = TR_RJUSTIFY | TR_HEX;
	t->f_filter = 0;
	t->p_filter = 0;

	if(GLOBALS->tims.marker != -1)
		{
		if(t->vector)
			{
			/* this is currently unused as vectors are exploded into single bits */
			vptr v = bsearch_vector(t->n.vec, GLOBALS->tims.marker - t->shift);
                        rc = convert_ascii(t, v);
			}
			else
			{
			hptr h_ptr = bsearch_node(t->n.nd, GLOBALS->tims.marker - t->shift);
			if(h_ptr)
				{
				if(!t->n.nd->extvals)
					{
					rc = (char *)calloc_2(2, 2*sizeof(char));
					rc[0] = AN_STR[h_ptr->v.h_val];
					}
					else
					{
					if(h_ptr->flags&HIST_REAL)
						{
						if(!(h_ptr->flags&HIST_STRING))
							{
#ifdef WAVE_HAS_H_DOUBLE
							rc = convert_ascii_real(t, &h_ptr->v.h_double);
#else
							rc = convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
							}
							else
							{
							rc = convert_ascii_string((char *)h_ptr->v.h_vector);
							}
						}
						else
						{
		                        	rc = convert_ascii_vec(t,h_ptr->v.h_vector);
						}
					}
				}
			}
		}

	t->flags = flags;
	t->f_filter = f_filter;
	t->p_filter = p_filter;
	}

return(rc);
}


/* ----------------------------------------------------------------------------
 * add_dnd_from_searchbox - generates tcl names from selected searchbox ones
 *
 * Results:
 *      tcl list containing all generated names
 * ----------------------------------------------------------------------------
 */

char *add_dnd_from_searchbox(void)
{
int i;
char *one_entry = NULL, *mult_entry = NULL;
unsigned int mult_len = 0;

for(i=0;i<GLOBALS->num_rows_search_c_2;i++)
        {
        struct symbol *s, *t;
        s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->clist_search_c_3), i);
        if(get_s_selected(s))
                {
                if((!s->vec_root)||(!GLOBALS->autocoalesce))
                        {
                        }
                        else
                        {
                        t=s->vec_root;
			set_s_selected(t, 1);
			t=t->vec_chain;
                        while(t)
                                {
                                if(get_s_selected(t))
                                        {
                                        set_s_selected(t, 0);
                                        }
                                t=t->vec_chain;
                                }
                        }
                }
        }

for(i=0;i<GLOBALS->num_rows_search_c_2;i++)
        {
        int len;
        struct symbol *s, *t;
        s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(GLOBALS->clist_search_c_3), i);
        if(get_s_selected(s))
                {
                if((!s->vec_root)||(!GLOBALS->autocoalesce))
                        {
			one_entry = make_single_tcl_list_name(s->n->nname, NULL, 0, 0);
			WAVE_OE_ME
                        }
                        else
                        {
                        len=0;
                        t=s->vec_root;
                        while(t)
                                {
				one_entry = make_single_tcl_list_name(t->n->nname, NULL, 1, 0);
				WAVE_OE_ME

                                if(get_s_selected(t))
                                        {
                                        if(len) set_s_selected(t, 0);
                                        }
                                /* len++; */ /* scan-build : artifact of below being removed */
				break; /* t=t->vec_chain; ...no longer needed because of for() loop above and handler in process_tcl_list() */
                                }
                        }
                }
        }
return(mult_entry);
}


/* ----------------------------------------------------------------------------
 * add_dnd_from_signal_window - generates tcl names from selected sigwin ones
 *
 * Results:
 *      tcl list containing all generated names
 * ----------------------------------------------------------------------------
 */
char *add_dnd_from_signal_window(void)
{
return(add_traces_from_signal_window(FALSE));
}


/* ----------------------------------------------------------------------------
 * add_traces_from_signal_window - generates tcl names from all sigwin ones
 *
 * Results:
 *      tcl list containing all generated names, does not contain
 *      {gtkwave NET OFF} directive as this is intended for tcl program usage.
 * ----------------------------------------------------------------------------
 */

char *add_traces_from_signal_window(gboolean is_from_tcl_command)
{
Trptr t;
char *one_entry = NULL, *mult_entry = NULL;
unsigned int mult_len = 0;
char *netoff = "{gtkwave NET OFF} ";
char *trace_val = NULL;
static const char xfwd[AN_COUNT]= AN_NORMAL;
char trace_val_vec_single[2] = { 0, 0 };

if(is_from_tcl_command)
	{
	mult_entry = strdup_2("");
	}

t=GLOBALS->traces.first;
while(t)
	{
        if( (!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH))) && ((t->flags & TR_HIGHLIGHT)||is_from_tcl_command) )
		{
                if(t->vector)
                        {
                        int i;
                        nptr *nodes;
                        vptr v = (GLOBALS->tims.marker != -1) ? bsearch_vector(t->n.vec, GLOBALS->tims.marker - t->shift) : NULL;
			unsigned char *bits = v ? (v->v) : NULL;
			char *first_str = NULL;
			int coalesce_pass = 1;

                        nodes=t->n.vec->bits->nodes;

			for(i=0;i<t->n.vec->bits->nnbits;i++)
				{
				if(!nodes[i]->expansion)
					{
					nptr n = nodes[i];
                                        char *str = append_array_row(n);
                                        char *p = strrchr(str, '[');
                                        if(p) { *p = 0; } else { coalesce_pass = 0; break; }

					if(!i)
						{
						first_str = strdup_2(str);
						}
						else
						{
						if((!first_str /*scan-build */) || strcmp(str, first_str)) { coalesce_pass = 0; break; }
						}
					}
					else
					{
					coalesce_pass = 0;
					}
				}

			if(coalesce_pass)
				{
				if(t->n.vec->bits->nnbits < 2)
					{
					coalesce_pass = 0;
					}
					else
					{
					nptr nl = nodes[0];
					char *strl = append_array_row(nl);
					char *pl = strrchr(strl, '[');
					int lidx = atoi(pl+1);

					nptr nr = nodes[t->n.vec->bits->nnbits - 1];
					char *strr = append_array_row(nr);
					char *pr = strrchr(strr, '[');
					int ridx = atoi(pr+1);

					int first_str_len = strlen(first_str ? first_str : (first_str = strdup_2("INTERNAL_ERROR"))); /* : case added for scan-build */
					char *newname = malloc_2(first_str_len + 40);

					sprintf(newname, "%s[%d:%d]", first_str, lidx, ridx); /* this disappears in make_single_tcl_list_name() but might be used in future code */

					if(!mult_entry) { one_entry = make_gtkwave_pid(); WAVE_OE_ME one_entry = strdup_2(netoff); WAVE_OE_ME}
					one_entry = is_from_tcl_command ? strdup_2s(newname) : make_single_tcl_list_name(newname, NULL, 0, 0);
					WAVE_OE_ME
					if(!is_from_tcl_command)
						{
						trace_val = give_value_string(t);
						if(trace_val)
							{
							one_entry = make_single_tcl_list_name(newname, trace_val, 0, 0);
							WAVE_OE_ME
							free_2(trace_val);
							}
						}

					free_2(newname);
					}

				free_2(first_str);
				first_str = NULL;
				}

			if(!coalesce_pass)
                        for(i=0;i<t->n.vec->bits->nnbits;i++)
                                {
                                if(nodes[i]->expansion)
                                        {
		                        int which, cnt;
					int bit = nodes[i]->expansion->parentbit;
					nptr n = nodes[i]->expansion->parent;
					char *str = append_array_row(n);
					char *p = strrchr(str, '[');
					if(p) { *p = 0; }

                        		if(n->lsi > n->msi)
                                		{
                                		for(which=0,cnt=n->lsi ; cnt>=n->msi ; cnt--,which++)
                                        		{
                                        		if(cnt==bit) break;
                                        		}
                                		}
                                		else
                                		{
                                		for(which=0,cnt=n->msi ; cnt>=n->lsi ; cnt--,which++)
                                        		{
                                        		if(cnt==bit) break;
                                        		}
                                		}

					sprintf(str+strlen(str), "[%d]", which);
					if(!mult_entry) { one_entry = make_gtkwave_pid(); WAVE_OE_ME one_entry = strdup_2(netoff); WAVE_OE_ME }
					one_entry = is_from_tcl_command ? strdup_2s(str) : make_single_tcl_list_name(str, NULL, 0, 0);
					WAVE_OE_ME

					if((bits)&&(!is_from_tcl_command))
						{
						int bitnum = bits[i];

						if(bitnum < 0) bitnum = AN_DASH;
						else if(bitnum >= AN_COUNT) bitnum = AN_DASH;

						trace_val_vec_single[0] = AN_STR[(int)xfwd[bitnum]];
						one_entry = make_single_tcl_list_name(str, trace_val_vec_single, 0, 0);
						WAVE_OE_ME
						}
                                        }
                                        else
                                        {
					if(!mult_entry) { one_entry = make_gtkwave_pid(); WAVE_OE_ME one_entry = strdup_2(netoff); WAVE_OE_ME}
					one_entry = is_from_tcl_command ? strdup_2s(append_array_row(nodes[i])) : make_single_tcl_list_name(append_array_row(nodes[i]), NULL, 0, 0);
					WAVE_OE_ME
					if(!is_from_tcl_command)
						{
						trace_val = give_value_string(t);
						if(trace_val)
							{
							one_entry = make_single_tcl_list_name(append_array_row(nodes[i]), trace_val, 0, 0);
							WAVE_OE_ME
							free_2(trace_val);
							}
						}
                                        }
                                }
                        }
			else
			{
			if(t->n.nd->expansion)
				{
	                        int which, cnt;
				int bit = t->n.nd->expansion->parentbit;
				nptr n = t->n.nd->expansion->parent;
				char *str = append_array_row(n);
				char *p = strrchr(str, '[');
				if(p) { *p = 0; }

                       		if(n->lsi > n->msi)
                               		{
                               		for(which=0,cnt=n->lsi ; cnt>=n->msi ; cnt--,which++)
                                       		{
                                       		if(cnt==bit) break;
                                       		}
                               		}
                               		else
                               		{
                               		for(which=0,cnt=n->msi ; cnt>=n->lsi ; cnt--,which++)
                                       		{
                                       		if(cnt==bit) break;
                                       		}
                               		}

				sprintf(str+strlen(str), "[%d]", which);
				if(!mult_entry) { one_entry = make_gtkwave_pid(); WAVE_OE_ME one_entry = strdup_2(netoff); WAVE_OE_ME}
				one_entry = is_from_tcl_command ? strdup_2s(str) : make_single_tcl_list_name(str, NULL, 0, 0);
				WAVE_OE_ME
				if(!is_from_tcl_command)
					{
					trace_val = give_value_string(t);
					if(trace_val)
						{
						one_entry = make_single_tcl_list_name(str, trace_val, 0, 0);
						WAVE_OE_ME
						free_2(trace_val);
						}
					}
				}
				else
				{
				if(!mult_entry) { one_entry = make_gtkwave_pid(); WAVE_OE_ME one_entry = strdup_2(netoff); WAVE_OE_ME}
				one_entry = is_from_tcl_command ? strdup_2s(append_array_row(t->n.nd)) : make_single_tcl_list_name(append_array_row(t->n.nd), NULL, 0, 0);
				WAVE_OE_ME
				if(!is_from_tcl_command)
					{
					trace_val = give_value_string(t);
					if(trace_val)
						{
						one_entry = make_single_tcl_list_name(append_array_row(t->n.nd), trace_val, 0, 0);
						WAVE_OE_ME
						free_2(trace_val);
						}
					}
				}
			}
		}
		else
		{
		if(!mult_entry)
			{
			one_entry = strdup_2(netoff); WAVE_OE_ME
			}
		}
	t = t->t_next;
	}

return(mult_entry);
}


/* ----------------------------------------------------------------------------
 * sig_selection_foreach_dnd - generates tcl names from iterated clist ones
 *
 * Results:
 *      tcl list containing all generated names coalesced back into *data
 * ----------------------------------------------------------------------------
 */

#if WAVE_USE_GTK2
static void
sig_selection_foreach_dnd
                      (GtkTreeModel *model,
                       GtkTreePath *path,
                       GtkTreeIter *iter,
                       gpointer data)
{
(void)path;

  struct tree *sel;
  int i;
  int low, high;
  struct iter_dnd_strings *it;
  char *one_entry, *mult_entry;
  unsigned int mult_len;
  enum { NAME_COLUMN, TREE_COLUMN, N_COLUMNS };

  it = (struct iter_dnd_strings *)data;
  one_entry = it->one_entry;
  mult_entry = it->mult_entry;
  mult_len = it->mult_len;

  /* Get the tree.  */
  gtk_tree_model_get (model, iter, TREE_COLUMN, &sel, -1);

  if(!sel) return;

  low = fetchlow(sel)->t_which;
  high = fetchhigh(sel)->t_which;

  /* If signals are vectors, iterate through them if so.  */
  for(i=low;i<=high;i++)
        {
        struct symbol *s;
        s=GLOBALS->facs[i];
        if((s->vec_root)&&(GLOBALS->autocoalesce))
                {
		struct symbol *t = s->vec_root;
                while(t)
			{
                        one_entry = make_single_tcl_list_name(t->n->nname, NULL, 1, 0);
                        WAVE_OE_ME
                        break; /* t=t->vec_chain; ...no longer needed as this is resolved in process_tcl_list() */
                        }
                }
		else
		{
                one_entry = make_single_tcl_list_name(s->n->nname, NULL, 0, 0);
                WAVE_OE_ME
		}
        }

  it->one_entry = one_entry;
  it->mult_entry = mult_entry;
  it->mult_len = mult_len;
}
#endif

/* ----------------------------------------------------------------------------
 * add_dnd_from_tree_window - generates tcl names from selected tree clist ones
 *
 * Results:
 *      tcl list containing all generated names
 * ----------------------------------------------------------------------------
 */

char *add_dnd_from_tree_window(void)
{
#if WAVE_USE_GTK2
struct iter_dnd_strings it;

memset(&it, 0, sizeof(struct iter_dnd_strings));
gtk_tree_selection_selected_foreach(GLOBALS->sig_selection_treesearch_gtk2_c_1, &sig_selection_foreach_dnd, (gpointer)&it);

return(it.mult_entry);
#else
return(NULL);
#endif
}


/* ----------------------------------------------------------------------------
 * make_message - printf() which mallocs into a string
 *
 * Results:
 *      dynamically allocated string
 * ----------------------------------------------------------------------------
 */

static char *make_message (const char *fmt, ...)
{
  /* Guess we need no more than 100 bytes. */
  int n, size = 100;
  char *p, *np;
  va_list ap;

  if ((p = malloc_2(size)) == NULL)
    return NULL;

  while (1)
    {
      /* Try to print in the allocated space. */
      va_start (ap, fmt);
      n = vsnprintf (p, size, fmt, ap);
      va_end (ap);
      /* If that worked, return the string. */
      if (n > -1 && n < size)
	return p;
      /* Else try again with more space. */
      if (n > -1)		/* glibc 2.1 */
	size = n + 1;		/* precisely what is needed */
      else			/* glibc 2.0 */
	size *= 2;		/* twice the old size */
      if ((np = realloc_2(p, size)) == NULL)
	{
	  free (p);
	  return NULL;
	}
      else
	{
	  p = np;
	}
    }
}


/* ----------------------------------------------------------------------------
 * emit_gtkwave_savefile_formatted_entries_in_tcl_list - performs as named
 *
 * Results:
 *      tcl list which mimics a gtkwave save file for cut and paste entries
 *      which is later iteratively run through the normal gtkwave save file
 *      loader parsewavline() on the distant end.  the reason this is
 *      necessary is in order to pass attribute and concatenation information
 *      along to the distant end.
 * ----------------------------------------------------------------------------
 */

char *emit_gtkwave_savefile_formatted_entries_in_tcl_list(Trptr t, gboolean use_tcl_mode) {
	char *one_entry, *mult_entry = NULL;
	unsigned int mult_len = 0;
	TraceFlagsType prev_flags = 0;

	TraceFlagsType def=0;
	TimeType prevshift=LLDescriptor(0);
	char is_first = 1;
	char flag_skip;

	while(t)
		{
		flag_skip = 0;

		if(use_tcl_mode)
		  {
		    if(IsSelected(t) || (t->t_grp && IsSelected(t->t_grp)))
		      {
			/* members of closed groups may not be highlighted */
			/* so propogate highlighting here */
			t->flags |= TR_HIGHLIGHT;
		      }
		    else
		      {
			if((prev_flags & TR_ANALOGMASK) && (t->flags &TR_ANALOG_BLANK_STRETCH))
			  {
			    flag_skip = 1;
			  }
			else
			  {
			    t = t->t_next;
			    continue;
			  }
		      }
		  }

		if((t->flags!=def)||(is_first))
			{
			is_first = 0;
			if((t->flags & TR_PTRANSLATED) && (!t->p_filter)) t->flags &= (~TR_PTRANSLATED);
			if((t->flags & TR_FTRANSLATED) && (!t->f_filter)) t->flags &= (~TR_FTRANSLATED);
			one_entry = make_message("@%"TRACEFLAGSPRIFMT"\n",(def=t->flags) & ~TR_HIGHLIGHT);
			WAVE_OE_ME
			if(!flag_skip) prev_flags = def;
			}

		if(t->t_color)
			{
			one_entry = make_message("[color] %d\n", t->t_color);
			WAVE_OE_ME
			}

		if(t->t_fpdecshift)
			{
			one_entry = make_message("[fpshift_count] %d\n", t->t_fpdecshift);
			WAVE_OE_ME
			}

		if((t->shift)||((prevshift)&&(!t->shift)))
			{
			one_entry = make_message(">"TTFormat"\n", t->shift);
			WAVE_OE_ME
			}
		prevshift=t->shift;

		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			if(t->flags & TR_FTRANSLATED)
				{
				if(t->f_filter && GLOBALS->filesel_filter[t->f_filter])
					{
					one_entry = make_message("^%d %s\n", t->f_filter, GLOBALS->filesel_filter[t->f_filter]);
					WAVE_OE_ME
					}
					else
					{
					one_entry = make_message("^%d %s\n", 0, "disabled");
					WAVE_OE_ME
					}
				}
			else
			if(t->flags & TR_PTRANSLATED)
				{
				if(t->p_filter && GLOBALS->procsel_filter[t->p_filter])
					{
					one_entry = make_message("^>%d %s\n", t->p_filter, GLOBALS->procsel_filter[t->p_filter]);
					WAVE_OE_ME
					}
					else
					{
					one_entry = make_message("^>%d %s\n", 0, "disabled");
					WAVE_OE_ME
					}
				}

			/* NOT an else! */
			if(t->flags & TR_TTRANSLATED)
				{
                                if(t->transaction_args)
                                        {
                                        one_entry = make_message("[transaction_args] \"%s\"\n", t->transaction_args);
					WAVE_OE_ME
                                        }
                                        else
                                        {
                                        one_entry = make_message("[transaction_args] \"%s\"\n", "");
					WAVE_OE_ME
                                        }

				if(t->t_filter && GLOBALS->ttranssel_filter[t->t_filter])
					{
					one_entry = make_message("^<%d %s\n", t->t_filter, GLOBALS->ttranssel_filter[t->t_filter]);
					WAVE_OE_ME
					}
					else
					{
					one_entry = make_message("^<%d %s\n", 0, "disabled");
					WAVE_OE_ME
					}
				}

			if(t->vector && !(t->n.vec->transaction_cache && t->n.vec->transaction_cache->transaction_nd))
				{
				int i;
				nptr *nodes;
				bptr bits;
				baptr ba;

				if(HasAlias(t)) { one_entry = make_message("+{%s} ",t->name_full);
				                  WAVE_OE_ME }
				bits = t->n.vec->bits;
				ba = bits ? bits->attribs : NULL;

				one_entry = make_message("%c{%s}", ba ? ':' : '#',
					t->n.vec->transaction_cache ? t->n.vec->transaction_cache->bvname : t->n.vec->bvname);
				WAVE_OE_ME

				nodes=t->n.vec->bits->nodes;
				for(i=0;i<t->n.vec->bits->nnbits;i++)
					{
					if(nodes[i]->expansion)
						{
						one_entry = make_message(" (%d)%s",nodes[i]->expansion->parentbit, append_array_row(nodes[i]->expansion->parent));
						WAVE_OE_ME
						}
						else
						{
						one_entry = make_message(" %s",append_array_row(nodes[i]));
						WAVE_OE_ME
						}
					if(ba)
						{
						one_entry = make_message(" "TTFormat" %"TRACEFLAGSPRIFMT, ba[i].shift, ba[i].flags);
						WAVE_OE_ME
						}
					}

				one_entry = make_message("\n");
				WAVE_OE_ME
				}
				else
				{
				  nptr nd = (t->vector && t->n.vec->transaction_cache && t->n.vec->transaction_cache->transaction_nd) ?
						t->n.vec->transaction_cache->transaction_nd : t->n.nd;

				  if(HasAlias(t))
					{
					if(nd->expansion)
						{
						one_entry = make_message("+{%s} (%d)%s\n",t->name_full,nd->expansion->parentbit, append_array_row(nd->expansion->parent));
						WAVE_OE_ME
						}
						else
						{
						one_entry = make_message("+{%s} %s\n",t->name_full,append_array_row(nd));
						WAVE_OE_ME
						}
					}
					else
					{
					if(nd->expansion)
						{
						one_entry = make_message("(%d)%s\n",nd->expansion->parentbit, append_array_row(nd->expansion->parent));
						WAVE_OE_ME
						}
						else
						{
						one_entry = make_message("%s\n",append_array_row(nd));
						WAVE_OE_ME
						}
					}
				}
			}
			else
			{
			if(!t->name) { one_entry = make_message("-\n"); WAVE_OE_ME }
			else { one_entry = make_message("-%s\n",t->name); WAVE_OE_ME }
			}

		t=t->t_next;
		}

if(mult_entry)
	{
	const char *hdr = "{gtkwave SAVELIST ";
	int hdr_len = strlen(hdr);
	const char *av[1];
	char *zm;
	int zm_len;

	av[0] = mult_entry;
	zm = zMergeTclList(1, av);
	zm_len = strlen(zm);

	free_2(mult_entry);

	mult_entry = malloc_2(hdr_len + zm_len + 2 + 1);
	memcpy(mult_entry, hdr, hdr_len);
	memcpy(mult_entry + hdr_len, zm, zm_len);
	strcpy(mult_entry + hdr_len + zm_len, "} ");

	free_2(zm);
	}

return(mult_entry);
}


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX functions for URL (not TCL list) handling XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

enum GtkwaveFtype { WAVE_FTYPE_UNKNOWN, WAVE_FTYPE_DUMPFILE, WAVE_FTYPE_STEMSFILE, WAVE_FTYPE_SAVEFILE };

/* ----------------------------------------------------------------------------
 * determine_ftype - examines filename (perhaps initial contents) and
 *      determines file type
 *
 * Results:
 *      enum of ftype determination
 * ----------------------------------------------------------------------------
 */
static int determine_ftype(char *s, char **dotpnt)
{
char *pnt = s;
char *dot = NULL, *dot2 = NULL;
int ftype = WAVE_FTYPE_UNKNOWN;

while(*pnt)
	{
	if(*pnt == '.') { dot2 = dot; dot = pnt; }
	pnt++;
	}

*dotpnt = dot;

if(dot)
	{
	if(!strcasecmp("sav", dot+1))
		{
		ftype = WAVE_FTYPE_SAVEFILE;
		}
	else
	if(!strcasecmp("gtkw", dot+1))
		{
		ftype = WAVE_FTYPE_SAVEFILE;
		}
	else
	if(!strcasecmp("stems", dot+1))
		{
		ftype = WAVE_FTYPE_STEMSFILE;
		}
	else /* detect dumpfile type */
	if	(
#ifdef EXTLOAD_SUFFIX
                (!strcasecmp(EXTLOAD_SUFFIX, dot+1)) ||
#endif
		(!strcasecmp("vcd", dot+1)) ||
		(!strcasecmp("dmp", dot+1)) ||
		(!strcasecmp("lxt", dot+1)) ||
		(!strcasecmp("lx2", dot+1)) ||
		(!strcasecmp("lxt2", dot+1)) ||
		(!strcasecmp("vzt", dot+1)) ||
		(!strcasecmp("fst", dot+1)) ||
		(!strcasecmp("ghw", dot+1)) ||
		(!strcasecmp("aet", dot+1)) ||	/* ignore .aet? filename types */
		(!strcasecmp("ae2", dot+1))
		)
		{
		ftype = WAVE_FTYPE_DUMPFILE;
		}
	else
	if(dot2)
		{
		if	(
			(!strcasecmp("ghw.gz", dot2+1))  ||
			(!strcasecmp("ghw.bz2", dot2+1)) ||
			(!strcasecmp("ghw.bz2", dot2+1)) ||
			(!strcasecmp("vcd.gz", dot2+1))  ||
			(!strcasecmp("vcd.zip", dot2+1))
			)
			{
			ftype = WAVE_FTYPE_DUMPFILE;
			}
		}
	}
	else
	{
	FILE *f = fopen(s, "rb");
	if(f)
		{
		int ch0 = getc(f);
		int ch1 = getc(f);

		if(ch0 == EOF) { ch0 = ch1 = 0; }
		else
		if(ch1 == EOF) { ch1 = 0; }

		if((ch0 == '+') && (ch1 == '+'))
			{
			ftype = WAVE_FTYPE_STEMSFILE; /* stems file */
			}
		else
		if(ch0 == '[')
			{
			ftype = WAVE_FTYPE_SAVEFILE; /* save file */
			}

		fclose(f);
		}
	}

return(ftype);
}

/* ----------------------------------------------------------------------------
 * process_url_file - examines filename and performs appropriate side-effect
 *
 * Results:
 *      Loads save file, new dump file, or stems file viewer
 * ----------------------------------------------------------------------------
 */
int process_url_file(char *s)
{
static int processing_missing_file = 0; /* in case of malformed save files causing recursion */

int rc = 0;
char *dotpnt = NULL;
int ftype = determine_ftype(s, &dotpnt);

switch(ftype)
	{
	case WAVE_FTYPE_SAVEFILE:
		if((GLOBALS->loaded_file_type == MISSING_FILE)&&(!processing_missing_file))
			{
			gboolean modified;
			int opt_vcd;
			char *dfn = extract_dumpname_from_save_file(s, &modified, &opt_vcd);
			if(dfn)
				{
				char *dfn_local = strdup(dfn);
				free_2(dfn);

				processing_missing_file = 1;
				if(process_url_file(dfn_local))
					{
					GLOBALS->dumpfile_is_modified = modified;
					}
				free(dfn_local);
				processing_missing_file = 0;
				}
			}

		GLOBALS->fileselbox_text = &GLOBALS->filesel_writesave;
	        GLOBALS->filesel_ok=1;
        	if(*GLOBALS->fileselbox_text) free_2(*GLOBALS->fileselbox_text);
        	*GLOBALS->fileselbox_text=(char *)strdup_2(s);

#ifndef MAC_INTEGRATION
		GLOBALS->block_xy_update = 1;
#else
		GLOBALS->block_xy_update = (GLOBALS->num_notebook_pages > 1); /* let window always resize if 1 tab */
#endif
		wave_gconf_client_set_string("/current/savefile", s);
		read_save_helper(s, NULL, NULL, NULL, NULL, NULL);
		GLOBALS->block_xy_update = 0;
		rc = 1;
		break;

	case WAVE_FTYPE_STEMSFILE:
#if !defined _MSC_VER && !defined __MINGW32__
		GLOBALS->fileselbox_text = &GLOBALS->stems_name;
	        GLOBALS->filesel_ok=1;
        	if(*GLOBALS->fileselbox_text) free_2(*GLOBALS->fileselbox_text);
        	*GLOBALS->fileselbox_text=(char *)strdup_2(s);

		menu_read_stems_cleanup(NULL, NULL);
#endif
		rc = 1;
		break;

	case WAVE_FTYPE_DUMPFILE:
		GLOBALS->fileselbox_text = &GLOBALS->filesel_newviewer_menu_c_1;
	        GLOBALS->filesel_ok=1;
        	if(*GLOBALS->fileselbox_text) free_2(*GLOBALS->fileselbox_text);
        	*GLOBALS->fileselbox_text=(char *)strdup_2(s);

		menu_new_viewer_tab_cleanup(NULL, NULL);
		rc = 1;
		break;

	default:
		break;
	}

return(rc);
}

/* ----------------------------------------------------------------------------
 * uri_cmp - qsort compare function that ensures save files and stems are
 *      ordered after their respective dumpfiles
 *
 * Results:
 *      returns correct sort order for processing (based on name and
 *      GtkwaveFtype
 * ----------------------------------------------------------------------------
 */
#ifdef WAVE_USE_GTK2
static int uri_cmp(const void *v1, const void *v2)
{
char *s1 = *(char **)v1;
char *s2 = *(char **)v2;
char *d1, *d2;
int typ1 = determine_ftype(s1, &d1);
int typ2 = determine_ftype(s2, &d2);
int rc;

if(!d1 || !d2)
	{
	return(strcmp(s1, s2));
	}

*d1 = 0; *d2 = 0;

rc = strcmp(s1, s2);
if(!rc)
	{
	rc = (typ1 - typ2); /* use suffix ftype to manipulate sort */
	}

*d1 = '.'; *d2 = '.';

return(rc);
}
#endif

/* ----------------------------------------------------------------------------
 * process_url_list - examines list of URLs and processes if valid files
 *
 * Results:
 *      Indicates if any URLs were processed
 * ----------------------------------------------------------------------------
 */

int process_url_list(char *s)
{
int is_url = 0;
#if WAVE_USE_GTK2
int i;
int url_cnt = 0;
char pch = 0;

char *nxt_hd = s;
char *pnt = s;
char *path;
char **url_list = g_malloc(sizeof(gchar *)); /* deliberate g_funcs() as context can swap out from under us */

if(*pnt == '{') { g_free(url_list); return(0); } /* exit early if tcl list */

for(;;)
	{
	if(*pnt == 0)
		{
		if(!(*nxt_hd))
			{
			break;
			}

		path = g_filename_from_uri(nxt_hd, NULL, NULL);
		if(path) { url_list[url_cnt++] = path; url_list = g_realloc(url_list, (url_cnt+1) * sizeof(gchar *)); }
		break;
		}
	else
	if((*pnt == '\n')||(*pnt == '\r'))
		{
		if((pch != '\n') && (pch != '\r'))
			{
			char sav = *pnt;
			*pnt = 0;

			path = g_filename_from_uri(nxt_hd, NULL, NULL);
			if(path) { url_list[url_cnt++] = path; url_list = g_realloc(url_list, (url_cnt+1) * sizeof(gchar *)); }
			*pnt = sav;
			}
		pch = *pnt;
		nxt_hd = pnt+1;
		pnt++;
		}
	else
		{
		pch = *pnt;
		pnt++;
		}
	}

if(url_list)
	{
	if(url_cnt > 2)
		{
		qsort(url_list, url_cnt, sizeof(struct gchar *), uri_cmp);
		}
	else
	if(url_cnt == 2) /* in case there are only 2 files, make the savefile last */
		{
		char *d1, *d2;
		int typ1 = determine_ftype(url_list[0], &d1);
		int typ2 = determine_ftype(url_list[1], &d2);

		if(typ1 > typ2)
			{
			char *tmp_swap = url_list[0];
			url_list[0] = url_list[1];
			url_list[1] = tmp_swap;
			}
		}

	for(i=0;i<url_cnt;i++)
		{
		is_url += process_url_file(url_list[i]);
		g_free(url_list[i]);
		}

	g_free(url_list);
	}

#else
(void)s;
#endif
return(is_url);
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX functions for embedding TCL interpreter XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

#if defined(HAVE_LIBTCL)

/* declare variables that will be shared with the repscript */
#undef WAVE_TCLCB_M
#define WAVE_TCLCB_M(a,b,c) a
static const char *tclcb_var_names[] = { WAVE_TCLCB_MACRO_EXPANSION };

#undef WAVE_TCLCB_M
#define WAVE_TCLCB_M(a,b,c) b
static const int tclcb_var_flags[] = { WAVE_TCLCB_MACRO_EXPANSION };

#undef WAVE_TCLCB_M
#define WAVE_TCLCB_M(a,b,c) c
static const char *tclcb_var_inits[] = { WAVE_TCLCB_MACRO_EXPANSION };

static void declare_tclcb_variables(Tcl_Interp *interp)
{
(void)interp;

int i = 0 ;
while(tclcb_var_flags[i] != -1)
	{
	gtkwavetcl_setvar(tclcb_var_names[i], tclcb_var_inits[i], tclcb_var_flags[i]) ;
	i++;
	}
}


static gboolean setvar_timer(gpointer arg)
{
static gboolean run_once = FALSE;

if(run_once == FALSE) /* avoid any race conditions with the toolkit for uninitialized data */
        {
        run_once = TRUE;
        return(TRUE);
        }
        else
        {
        static gboolean in_timer = FALSE;

        if(!GLOBALS->busy_busy_c_1 && !in_timer)
                {
                Tcl_Interp *interp = (Tcl_Interp *)arg;
                const char *tv = NULL;

                in_timer = TRUE;
                tv = Tcl_GetVar(interp, WAVE_TCLCB_TIMER_PERIOD, WAVE_TCLCB_TIMER_PERIOD_FLAGS);
		if(tv)
			{
	                gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_TIMER_PERIOD,tv,WAVE_TCLCB_TIMER_PERIOD_FLAGS);
	                tv = Tcl_GetVar(interp, WAVE_TCLCB_TIMER_PERIOD, WAVE_TCLCB_TIMER_PERIOD_FLAGS);
			}
                in_timer = FALSE;

		if(tv)
			{
	                g_timeout_add(atoi(tv), setvar_timer, arg);
			}
                return(FALSE);
                }
                else
                {
                return(TRUE);
                }
        }
}


static void init_setvar_timer(Tcl_Interp *interp)
{
g_timeout_add(atoi(WAVE_TCLCB_TIMER_PERIOD_INIT), setvar_timer, (gpointer)interp);
}



static int menu_func(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
gtkwave_mlist_t *ife = (gtkwave_mlist_t *)clientData;
int i;
struct wave_script_args *old_wave_script_args = GLOBALS->wave_script_args; /* stackable args */
char fexit = GLOBALS->enable_fast_exit;

if(GLOBALS->in_tcl_callback) /* don't allow callbacks to call menu functions (yet) */
	{
	char reportString[1024];
	char menuItem[512];
	Tcl_Obj *aobj;

	char *src = ife->path;
	char *dst = menuItem;

	while(*src)
		{
		*dst = (*src != ' ') ? *src : '_';
		src++;
		dst++;
		}
	*dst = 0;

	sprintf(reportString, "gtkwave::%s prohibited in callback", menuItem);
	gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,reportString,WAVE_TCLCB_ERROR_FLAGS);

	aobj = Tcl_NewStringObj(reportString, -1);
	Tcl_SetObjResult(interp, aobj);
	return(TCL_ERROR);
	}

GLOBALS->wave_script_args = NULL;
GLOBALS->enable_fast_exit = 1;

if(objc > 1)
	{
	struct wave_script_args *wc = NULL;

	for(i=1;i<objc;i++)
		{
		char *s = Tcl_GetString(objv[i]);
		int slen = strlen(s);
		struct wave_script_args *w = wave_alloca(sizeof(struct wave_script_args) + slen + 1);
			/*  alloca used in case we context switch and get our allocator ripped out from under us -- the call stack won't go away */
		if(slen)
			{
			strcpy(w->payload, s); /* scan-build complains but it thinks payload[1] is the actual memory allocated */
			}
		w->curr = NULL; /* yes, curr is only ever used for the 1st struct, but there is no sense creating head/follower structs for this */
		w->next = NULL;

		if(!GLOBALS->wave_script_args)
			{
			GLOBALS->wave_script_args = w;
			w->curr = w;
			}
			else
			{
			if(wc)	/* scan-build: suppress warning, this will never happen */
				{
				wc->next = w; /* we later really traverse through curr->next from the head pointer */
				}
			}

		wc = w;
		}

	if(!GLOBALS->wave_script_args) /* create a dummy list in order to keep requesters from popping up in file.c, etc. */
		{
		GLOBALS->wave_script_args = wave_alloca(sizeof(struct wave_script_args) + 1);
		GLOBALS->wave_script_args->curr = NULL;
		GLOBALS->wave_script_args->next = NULL;
		GLOBALS->wave_script_args->payload[0] = 0;
		}

	ife->callback();
	gtkwave_main_iteration();

	GLOBALS->wave_script_args = NULL;
	}
	else
	{
	ife->callback();
	gtkwave_main_iteration();
	}

GLOBALS->enable_fast_exit = fexit;
GLOBALS->wave_script_args = old_wave_script_args;
return(TCL_OK); /* signal error with rc=TCL_ERROR, Tcl_Obj *aobj = Tcl_NewStringObj(reportString, -1); Tcl_SetObjResult(interp, aobj); */
}

/* XXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX  RPC Tcl Variant  XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXX */

char *rpc_script_execute(const char *nam)
{
char *tpnt = NULL;
char *s;

if((nam) && (strlen(nam)) && (!GLOBALS->tcl_running))
	{
	int tclrc;
	int nlen = strlen(nam);
	char *tcl_cmd = wave_alloca(8 + nlen + 1 + 1);
	strcpy(tcl_cmd, "source {");
	strcpy(tcl_cmd+8, nam);
        strcpy(tcl_cmd+8+nlen, "}");

	GLOBALS->tcl_running = 1;
	tclrc = Tcl_Eval (GLOBALS->interp, tcl_cmd);
	GLOBALS->tcl_running = 0;

	if(tclrc != TCL_OK)
		{
		tpnt = strdup_2(Tcl_GetStringResult (GLOBALS->interp));
		}
		else
		{
		tpnt = strdup_2("TCL_OK");
		}
	}

if(!tpnt) tpnt = strdup_2("TCL_ERROR : no filename specified");

s = malloc_2(strlen("--script ") + strlen(tpnt) + 1 + 1);
sprintf(s, "%s%s\n", "--script ", tpnt);
free_2(tpnt);

return(s);
}


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX  Bluespec Tcl Variant  XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

void gtkUpdate(ClientData ignore)
{
(void)ignore;

  while (gtk_events_pending()) { gtk_main_iteration(); }
  Tcl_CreateTimerHandler(50,gtkUpdate, (ClientData) NULL); /* ajb: was 0 period ...caused 100% CPU spike */
}

int  gtkwaveInterpreterInit(Tcl_Interp *interp) {
  int i;
  char commandName[128];
  gtkwave_mlist_t *ife;
  int num_menu_items;

#ifdef WAVE_TCL_STUBIFY
/* not needed...this does a double init if enabled:  set_globals_interp(); */
#else
  if(Tcl_Init(interp) == TCL_ERROR) return TCL_ERROR;
  if(Tk_Init(interp) == TCL_ERROR) return TCL_ERROR;
  Tcl_SetVar(interp,"tcl_rcFileName","~/.wishrc",TCL_GLOBAL_ONLY);
#endif

  strcpy(commandName, "gtkwave::");

  ife = retrieve_menu_items_array(&num_menu_items);
  for(i=0;i<num_menu_items;i++)
    {
      if(ife[i].callback)
	{
	  char *pnt = commandName + 9;
	  strcpy(pnt, ife[i].path);
	  while(*pnt)
	    {
	      if(*pnt==' ') *pnt='_';
	      pnt++;
	    }

	  Tcl_CreateObjCommand(interp, commandName,
			       (Tcl_ObjCmdProc *)menu_func,
			       (ClientData)(ife+i), (Tcl_CmdDeleteProc *)NULL);
	}
    }


  for (i = 0; gtkwave_commands[i].func != NULL; i++)
    {
      strcpy(commandName + 9, gtkwave_commands[i].cmdstr);

      Tcl_CreateObjCommand(interp, commandName,
			   (Tcl_ObjCmdProc *)gtkwave_commands[i].func,
			   (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
    }

  declare_tclcb_variables(interp) ;

  /* hide the "wish" window */
  Tcl_Eval(interp, "wm withdraw .");

  Tcl_Eval(interp,
    "puts \"Interpreter id is [file tail $::argv0]\"");

  if (GLOBALS->tcl_init_cmd)
    {
      Tcl_Eval(interp, GLOBALS->tcl_init_cmd);
    }

  Tcl_CreateTimerHandler(50,gtkUpdate, (ClientData) NULL);
  init_setvar_timer(interp);

  return TCL_OK;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX  Simpod Tcl Variant  XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXX */

static gboolean repscript_timer(gpointer dummy)
{
(void)dummy;

static gboolean run_once = FALSE;

if(run_once == FALSE) /* avoid any race conditions with the toolkit for uninitialized data */
        {
        run_once = TRUE;
        return(TRUE);
        }

if((GLOBALS->repscript_name) && (!GLOBALS->tcl_running))
	{
	int tclrc;
	int nlen = strlen(GLOBALS->repscript_name);
	char *tcl_cmd = wave_alloca(8 + nlen + 1 + 1);
	strcpy(tcl_cmd, "source {");
	strcpy(tcl_cmd+8, GLOBALS->repscript_name);
        strcpy(tcl_cmd+8+nlen, "}");

	GLOBALS->tcl_running = 1;
	tclrc = Tcl_Eval (GLOBALS->interp, tcl_cmd);
	GLOBALS->tcl_running = 0;
#if WAVE_TCL_CHECK_VERSION(8,5,0)
	if(tclrc != TCL_OK) {
	  Tcl_Obj *options;
	  Tcl_Obj *key;
	  Tcl_Obj *stackTrace;

	  fprintf(stderr, "GTKWAVE | %s\n",
		  Tcl_GetStringResult (GLOBALS->interp));
	  options = Tcl_GetReturnOptions(GLOBALS->interp, tclrc);
	  key = Tcl_NewStringObj("-errorinfo", -1);
	  Tcl_IncrRefCount(key);
	  Tcl_DictObjGet(NULL, options, key, &stackTrace);
	  Tcl_DecrRefCount(key);
	  fprintf(stderr, "TCL Stack Trace\n%s\n",
		  Tcl_GetStringFromObj(stackTrace, NULL)) ;
	  /* Do something with stackTrace */
	}
#else
	if(tclrc != TCL_OK) { fprintf (stderr, "GTKWAVE | %s\n", Tcl_GetStringResult (GLOBALS->interp)); }
#endif

	return(TRUE);
	}
	else
	{
	return(FALSE);
	}
}


void set_globals_interp(char *me, int install_tk)
{
#ifdef WAVE_TCL_STUBIFY
  if(NpCreateMainInterp(me, install_tk))
	{
	GLOBALS->interp = NpGetMainInterp();
	}
	else
	{
	fprintf(stderr, "GTKWAVE | Error, failed to find Tcl/Tk runtime libraries.\n");
	fprintf(stderr, "GTKWAVE | Set the environment variable TCL_PLUGIN_DLL to point to\n");
	fprintf(stderr, "GTKWAVE | the Tcl shared object file.\n");
	exit(255);
	}
#else
(void)me;
(void)install_tk;
GLOBALS->interp = Tcl_CreateInterp();
#endif
}


void make_tcl_interpreter(char *argv[])
{
  int i;
  char commandName[32768];
  gtkwave_mlist_t *ife;
  int num_menu_items;
#if !((defined(__MACH__) && defined(__APPLE__)))
  int n = 0;
#endif

#ifndef WAVE_TCL_STUBIFY
  Tcl_FindExecutable(argv[0]);
#endif
#if (defined(__MACH__) && defined(__APPLE__))
  {
  uint32_t size = sizeof(commandName);
  if(_NSGetExecutablePath(commandName, &size) == 0)
	{
	set_globals_interp(commandName, 0);
	}
	else
	{
	char *p = calloc_2(1, size+1);
	size++;
  	if(_NSGetExecutablePath(p, &size) == 0)
		{
		set_globals_interp(p, 0);
		}
		else
		{
		fprintf(stderr, "GTKWAVE | Problem with _NSGetExecutablePath, exiting.\n");
		exit(255);
		}
	free_2(p);
	}
  }
#else
#ifdef WIN32
  if(!GetModuleFileName(NULL, commandName, 256))
    n = -1 ;
#else
  n = readlink("/proc/self/exe", commandName, 256) ;
#endif
  if(n == -1) {
    fprintf(stderr, "GTKWAVE | Tcl_Init error: Failed to get my fullpath\n");
    exit(EXIT_FAILURE);
  } else {
    commandName[n] = '\0' ;
  }
  set_globals_interp(commandName, 0);
#endif

#ifndef WAVE_TCL_STUBIFY
  if (TCL_OK != Tcl_Init(GLOBALS->interp))
    {
      fprintf(stderr, "GTKWAVE | Tcl_Init error: %s\n", Tcl_GetStringResult (GLOBALS->interp));
      exit(EXIT_FAILURE);
    }
#endif

  strcpy(commandName, "gtkwave::");

  ife = retrieve_menu_items_array(&num_menu_items);
  for(i=0;i<num_menu_items;i++)
    {
      if(ife[i].callback)
	{
	  char *pnt = commandName + 9;
	  strcpy(pnt, ife[i].path);
	  while(*pnt)
	    {
	      if(*pnt==' ') *pnt='_';
	      pnt++;
	    }

	  Tcl_CreateObjCommand(GLOBALS->interp, commandName,
			       (Tcl_ObjCmdProc *)menu_func,
			       (ClientData)(ife+i), (Tcl_CmdDeleteProc *)NULL);
	}
    }


  for (i = 0; gtkwave_commands[i].func != NULL; i++)
    {
      strcpy(commandName + 9, gtkwave_commands[i].cmdstr);

      Tcl_CreateObjCommand(GLOBALS->interp, commandName,
                (Tcl_ObjCmdProc *)gtkwave_commands[i].func,
			   (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
    }

  declare_tclcb_variables(GLOBALS->interp);

  if(GLOBALS->repscript_name)
    {
      FILE *f = fopen(GLOBALS->repscript_name, "rb");
      if(f)
	{
	  fclose(f);
	  g_timeout_add(GLOBALS->repscript_period, repscript_timer, NULL);
	}
      else
	{
	  fprintf(stderr, "GTKWAVE | Could not open repscript '%s', exiting.\n", GLOBALS->repscript_name);
	  perror("Why");
	  exit(255);
	}
    }

  init_setvar_timer(GLOBALS->interp);
}


/* blocking version which keeps recursive setvars from happening */
const char *gtkwavetcl_setvar(const char *name1, const char *val, int flags)
{
const char *rc = NULL;
if(GLOBALS->interp && !GLOBALS->in_tcl_callback)
        {
	GLOBALS->in_tcl_callback = 1;
        rc = Tcl_SetVar(GLOBALS->interp, name1, val, flags);
	GLOBALS->in_tcl_callback = 0;
        }

return(rc);
}

/* version which would be used, for example by timer interrupts */
const char *gtkwavetcl_setvar_nonblocking(const char *name1, const char *val, int flags)
{
const char *rc = NULL;
if(GLOBALS->interp)
        {
        rc = Tcl_SetVar(GLOBALS->interp, name1, val, flags);
        }

return(rc);
}


#else

void make_tcl_interpreter(char *argv[])
{
(void)argv;

/* nothing */
}


const char *gtkwavetcl_setvar(const char *name1, const char *val, int flags)
{
(void)name1;
(void)val;
(void) flags;

return(NULL);
}


const char *gtkwavetcl_setvar_nonblocking(const char *name1, const char *val, int flags)
{
(void) name1;
(void) val;
(void) flags;

return(NULL);
}


char *rpc_script_execute(const char *nam)
{
(void) nam;

return(strdup_2("--script TCL_ERROR : Tcl support not compiled into gtkwave\n"));
}


#endif


