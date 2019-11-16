/*
 * Copyright (c) Tony Bybell and Concept Engineering GmbH 2008-2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <config.h>

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "splay.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

extern ds_Tree *flattened_mod_list_root;
void bwlogbox(char *title, int width, ds_Tree *t, int display_mode);

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
    register const char *p;
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
    register char c;
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
 *	pointer array must be freed by calling free().
 *
 *	*argcPtr will get filled in with the number of valid elements
 *	in the array.  Note: *argcPtr is only modified if the procedure
 *	returns not NULL.
 * ----------------------------------------------------------------------------
 */
char** zSplitTclList(const char* list, int* argcPtr) {
    char** argv;
    const char* l;
    register char* p;
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
    argv = malloc(i);

    for (i = 0, p = (char*) (argv+size); *list != 0;  i++) {
	ok = tclFindElement(list, &element, &list, &elSize, &brace);

	if (!ok) {
	    free(argv);
	    return NULL;
	}
	if (*element == 0) break;

	if (i >= size) {
	    free(argv);
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
    register const char *p;
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
    register char *p = dst;

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
    else                    flagPtr = malloc(argc*sizeof(int));
    numChars = 1;

    for (i=0; i<argc; i++) numChars += tclScanElement(argv[i], &flagPtr[i]) + 1;

    result = malloc(numChars);

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

    if (flagPtr != localFlags) free(flagPtr);
    return result;
}

/**********************************************************************/
/*** Dnd, this is specific to rtlbrowse, the above was from gtkwave ***/
/**********************************************************************/

#define WAVE_DRAG_TAR_NAME_0         "text/plain"
#define WAVE_DRAG_TAR_INFO_0         0

#define WAVE_DRAG_TAR_NAME_1         "text/uri-list"         /* not url-list */
#define WAVE_DRAG_TAR_INFO_1         1

#define WAVE_DRAG_TAR_NAME_2         "STRING"
#define WAVE_DRAG_TAR_INFO_2         2

static gboolean DNDDragMotionCB(
        GtkWidget *widget, GdkDragContext *dc,
        gint xx, gint yy, guint tt,
        gpointer data
)
{
(void)widget;
(void)xx;
(void)yy;
(void)data;
#ifdef WAVE_USE_GTK2
	GdkDragAction suggested_action;

        /* Respond with default drag action (status). First we check
         * the dc's list of actions. If the list only contains
         * move, copy, or link then we select just that, otherwise we
         * return with our default suggested action.
         * If no valid actions are listed then we respond with 0.
         */
        suggested_action = GDK_ACTION_MOVE;

        /* Only move? */
        if(dc->actions == GDK_ACTION_MOVE)
            gdk_drag_status(dc, GDK_ACTION_MOVE, tt);
        /* Only copy? */
        else if(dc->actions == GDK_ACTION_COPY)
            gdk_drag_status(dc, GDK_ACTION_COPY, tt);
        /* Only link? */
        else if(dc->actions == GDK_ACTION_LINK)
            gdk_drag_status(dc, GDK_ACTION_LINK, tt);
        /* Other action, check if listed in our actions list? */
        else if(dc->actions & suggested_action)
            gdk_drag_status(dc, suggested_action, tt);
        /* All else respond with 0. */
        else
            gdk_drag_status(dc, 0, tt);
#else
(void)dc;
(void)tt;
#endif

return(FALSE);
}

static void DNDBeginCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;
}

static void DNDEndCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;
}

/*
 *      DND "drag_data_received" handler. When DNDDataRequestCB()
 *	calls gtk_selection_data_set() to send out the data, this function
 *	receives it and is responsible for handling it.
 *
 *	This is also the only DND callback function where the given
 *	inputs may reflect those of the drop target so we need to check
 *	if this is the same structure or not.
 */
static void DNDDataReceivedCB(
	GtkWidget *widget, GdkDragContext *dc,
	gint x, gint y, GtkSelectionData *selection_data,
	guint info, guint t, gpointer data) {
(void)x;
(void)y;
(void)t;

    gboolean same;
    GtkWidget *source_widget;

    if((widget == NULL) || (data == NULL) || (dc == NULL)) return;

    /* Important, check if we actually got data.  Sometimes errors
     * occure and selection_data will be NULL.
     */
    if(selection_data == NULL)     return;
    if(selection_data->length < 0) return;

    /* Source and target widgets are the same? */
    source_widget = gtk_drag_get_source_widget(dc);
    same = (source_widget == widget) ? TRUE : FALSE;

    if(same)
	{
	return;
	}

    /* Now check if the data format type is one that we support
     * (remember, data format type, not data type).
     *
     * We check this by testing if info matches one of the info
     * values that we have defined.
     *
     * Note that we can also iterate through the atoms in:
     *	GList *glist = dc->targets;
     *
     *	while(glist != NULL)
     *	{
     *	    gchar *name = gdk_atom_name((GdkAtom)glist->data);
     *	     * strcmp the name to see if it matches
     *	     * one that we support
     *	     *
     *	    glist = glist->next;
     *	}
     */
    if((info == WAVE_DRAG_TAR_INFO_0) ||
       (info == WAVE_DRAG_TAR_INFO_1) ||
       (info == WAVE_DRAG_TAR_INFO_2))
    {
    int impcnt = 0;
    ds_Tree *ft = NULL;
    int argc = 0;
    char**zs = zSplitTclList((const char *)selection_data->data, &argc);
    if(zs)
	{
        int i;
	for(i=0;i<argc;i++)
		{
		if((!strncmp("net ", zs[i], 4)) || (!strncmp("netBus ", zs[i], 7)))
			{
			char *stemp = strdup(zs[i]);
			char *ss = strchr(stemp, ' ') + 1;
			char *sl = strrchr(stemp, ' ');
			char *pnt = ss;

			if(sl)
				{
				*sl = 0;
				while(*pnt)
					{
					if(*pnt == ' ') { *pnt = '.'; }
					pnt++;
					}
				}

			ft = flattened_mod_list_root;
			while(ft)
			        {
			        if(!strcmp(ss, ft->fullname))
			                {
					if(!ft->dnd_to_import)
						{
				                ft->dnd_to_import = 1;
						impcnt++;
						}
			                break;
			                }

			        ft = ft->next_flat;
			        }

			free(stemp);
			}
		}
	free(zs);
	}

    if(impcnt)
	{
	ds_Tree **fta = calloc(impcnt, sizeof(ds_Tree *));
	int i = 0;

	while(ft)
	        {
	        if(ft->dnd_to_import)
	                {
			ft->dnd_to_import = 0;
			fta[i++] = ft;

			if(i == impcnt) break;
	                }

	        ft = ft->next_flat;
	        }

	for(i=impcnt-1;i>=0;i--) /* reverse list so it is forwards in rtlbrowse */
		{
		if(fta[i]) /* scan-build */
			{
		        bwlogbox(fta[i]->fullname, 640 + 8*8, fta[i], 0);
			}
		}

	free(fta);
	}
    }
}

void setup_dnd(GtkWidget *wid)
{
	GtkTargetEntry target_entry[3];

        target_entry[0].target = WAVE_DRAG_TAR_NAME_0;
        target_entry[0].flags = 0;
        target_entry[0].info = WAVE_DRAG_TAR_INFO_0;
        target_entry[1].target = WAVE_DRAG_TAR_NAME_1;
        target_entry[1].flags = 0;
        target_entry[1].info = WAVE_DRAG_TAR_INFO_1;
        target_entry[2].target = WAVE_DRAG_TAR_NAME_2;
        target_entry[2].flags = 0;
        target_entry[2].info = WAVE_DRAG_TAR_INFO_2;

        gtk_drag_dest_set(
                GTK_WIDGET(wid),
                GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                GTK_DEST_DEFAULT_DROP,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
                GDK_ACTION_COPY
                );

        gtk_signal_connect(GTK_OBJECT(wid), "drag_data_received", GTK_SIGNAL_FUNC(DNDDataReceivedCB), GTK_WIDGET(wid));
        gtk_signal_connect(GTK_OBJECT(wid), "drag_motion", GTK_SIGNAL_FUNC(DNDDragMotionCB), GTK_WIDGET(wid));
        gtk_signal_connect(GTK_OBJECT(wid), "drag_begin", GTK_SIGNAL_FUNC(DNDBeginCB), GTK_WIDGET(wid));
        gtk_signal_connect(GTK_OBJECT(wid), "drag_end", GTK_SIGNAL_FUNC(DNDEndCB), GTK_WIDGET(wid));
}

