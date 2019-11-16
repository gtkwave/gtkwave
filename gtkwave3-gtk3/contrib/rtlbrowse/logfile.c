/*
 * Copyright (c) Tony Bybell 1999-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <string.h>
#include "splay.h"
#include "vlex.h"
#include "jrb.h"
#include "wavelink.h"

#if GTK_CHECK_VERSION(3,0,0)
#define XXX_GTK_STOCK_GO_BACK "go-previous"
#define XXX_GTK_STOCK_GO_FORWARD "go-next"
#define XXX_GTK_STOCK_CLOSE "window-close"
#else
#define XXX_GTK_STOCK_GO_BACK GTK_STOCK_GO_BACK
#define XXX_GTK_STOCK_GO_FORWARD GTK_STOCK_GO_FORWARD
#define XXX_GTK_STOCK_CLOSE GTK_STOCK_CLOSE
#endif

extern ds_Tree *flattened_mod_list_root;
extern struct gtkwave_annotate_ipc_t *anno_ctx;
extern GtkWidget *notebook;
extern int verilog_2005;

TimeType old_marker = 0;
unsigned old_marker_set = 0;

#define set_winsize(w,x,y) gtk_window_set_default_size(GTK_WINDOW(w),(x),(y))


/* only for use locally */
struct wave_logfile_lines_t
{
struct wave_logfile_lines_t *next;
int line_no, tok;
char *text;
};

struct logfile_context_t
{
ds_Tree *which;
char *title;
int display_mode;
int width;

JRB varnames;
int resolved;
};

struct text_find_t
{
struct text_find_t *next;
GtkWidget *text, *window, *button;
struct logfile_context_t *ctx;

gint line, offs;	/* of insert marker */
gint srch_line, srch_offs; /* for search, to avoid duplication */

GtkTextTag *bold_tag;
GtkTextTag *dgray_tag, *lgray_tag;
GtkTextTag *blue_tag, *fwht_tag;
GtkTextTag *mono_tag;
GtkTextTag *size_tag;
};


void bwlogbox(char *title, int width, ds_Tree *t, int display_mode);
void bwlogbox_2(struct logfile_context_t *ctx, GtkWidget *window, GtkWidget *button, GtkWidget *text);
gboolean update_ctx_when_idle(gpointer textview_or_dummy);

static struct text_find_t *text_root = NULL;
static struct text_find_t *selected_text_via_tab = NULL;
static GtkWidget *matchcase_checkbutton = NULL;
static gboolean matchcase_active = FALSE;
static char *fontname_logfile = NULL;

/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static gpointer fontx = NULL;

static GtkTextIter iterx;
static GtkTextTag *bold_tag = NULL;
static GtkTextTag *dgray_tag = NULL, *lgray_tag = NULL;
static GtkTextTag *blue_tag = NULL, *fwht_tag = NULL;
static GtkTextTag *mono_tag = NULL;
static GtkTextTag *size_tag = NULL;

static void *pressWindow = NULL;
static gint pressX = 0;
static gint pressY = 0;

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
pressWindow = (void *)gtk_widget_get_window(widget);
pressX = event->x;
pressY = event->y;

return(FALSE);
}


static gint expose_event_local(GtkWidget *widget, GdkEventExpose *event)
{
(void)event;

struct text_find_t *tr = text_root;

while(tr)
	{
	if(tr->window == widget)
		{
		if(selected_text_via_tab != tr)
			{
			selected_text_via_tab = tr;
			/* printf("Expose: %08x '%s'\n", widget, tr->ctx->title); */
			}
		return(FALSE);
		}
	tr = tr->next;
	}
selected_text_via_tab = NULL;

return(FALSE);
}

#if GTK_CHECK_VERSION(3,0,0)
static gint draw_event(GtkWidget *widget, cairo_t *cr, gpointer      user_data)
{
(void) cr;
(void) user_data;

return(expose_event_local(widget, NULL));
}
#endif


GtkWidget *
XXX_gtk_toolbar_insert_stock (GtkToolbar *toolbar,
                          const gchar *stock_id,
                          const char *tooltip_text,
                          const char *tooltip_private_text,
                          GCallback callback,
                          gpointer user_data,
                          gint position)
{
  (void) tooltip_private_text;
  GtkToolItem *button;

#if GTK_CHECK_VERSION(3,0,0)
  GtkWidget *icon_widget = gtk_image_new_from_icon_name(stock_id, GTK_ICON_SIZE_BUTTON);

  gtk_widget_show(icon_widget);
  button = gtk_tool_button_new(icon_widget, NULL);
#else
  button = gtk_tool_button_new_from_stock (stock_id);
#endif
  gtk_tool_item_set_tooltip_text (button, tooltip_text);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
                      button,
                      position);

  g_signal_connect(XXX_GTK_OBJECT(button), "clicked", G_CALLBACK(callback), user_data);
  return(GTK_WIDGET(button));
}


void read_insert_position(struct text_find_t *tr)
{
GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
GtkTextIter iter;

gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);

tr->line = gtk_text_iter_get_line(&iter);
tr->offs = gtk_text_iter_get_line_offset(&iter);
}

void set_insert_position(struct text_find_t *tr)
{
GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
GtkTextIter iter;
gint llen;

gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);
gtk_text_iter_set_line(&iter, tr->line);
llen = gtk_text_iter_get_chars_in_line (&iter);
tr->offs = (tr->offs > llen) ? llen : tr->offs;
gtk_text_iter_set_line_offset(&iter, tr->offs);

gtk_text_buffer_place_cursor(tb, &iter);
}

static void
forward_chars_with_skipping (GtkTextIter *iter,
                             gint         count)
{

  gint i;

  g_return_if_fail (count >= 0);

  i = count;

  while (i > 0)
    {
      gboolean ignored = FALSE;

      if (gtk_text_iter_get_char (iter) == 0xFFFC)
        ignored = TRUE;

      gtk_text_iter_forward_char (iter);

      if (!ignored)
        --i;
    }
}

static gboolean iter_forward_search_caseins(
                              const GtkTextIter *iter,
                              gchar             *str,
                              GtkTextIter       *match_start,
                              GtkTextIter       *match_end)
{
GtkTextIter start = *iter;
GtkTextIter next;
gchar *line_text, *found, *pnt;
gint offset;
gchar *strcaseins;

if(!str) return(FALSE);

pnt = strcaseins = strdup(str);
while(*pnt)
	{
	*pnt = toupper((int)(unsigned char)*pnt);
	pnt++;
	}

for(;;)
	{
	next = start;
	gtk_text_iter_forward_line (&next);

	/* No more text in buffer */
	if (gtk_text_iter_equal (&start, &next))
		{
		free(strcaseins);
	      	return(FALSE);
	    	}

	pnt = line_text = gtk_text_iter_get_visible_text (&start, &next);
	while(*pnt)
		{
		*pnt = toupper((int)(unsigned char)*pnt);
		pnt++;
		}
	found = strstr(line_text, strcaseins);
	if(found)
		{
		gchar cached = *found;
		*found = 0;
		offset = g_utf8_strlen (line_text, -1);
		*found = cached;
		break;
		}
	g_free (line_text);

	start = next;
	}

*match_start = start;
forward_chars_with_skipping (match_start, offset);

offset = g_utf8_strlen (str, -1);
*match_end = *match_start;
forward_chars_with_skipping (match_end, offset);

free(strcaseins);
return(TRUE);
}


void tr_search_forward(char *str, gboolean noskip)
{
struct text_find_t *tr = selected_text_via_tab;

if((tr) && (tr->text))
	{
	GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
	GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
	GtkTextIter iter;
	gboolean found = FALSE;
	GtkTextIter match_start;
	GtkTextIter match_end;

	gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);

	tr->line = gtk_text_iter_get_line(&iter);
	tr->offs = gtk_text_iter_get_line_offset(&iter);

	if(!noskip)
	if((tr->line == tr->srch_line) && (tr->offs == tr->srch_offs))
		{
		gtk_text_iter_forward_char(&iter);
		}


	if(str)
		{
		if(!matchcase_active)
			{
			found = iter_forward_search_caseins(&iter, str, &match_start, &match_end);
			}
			else
			{
			found = gtk_text_iter_forward_search(&iter, str, GTK_TEXT_SEARCH_TEXT_ONLY, &match_start, &match_end, NULL);
			}
		if(!found)
			{
			gtk_text_buffer_get_start_iter(tb, &iter);
			if(!matchcase_active)
				{
				found = iter_forward_search_caseins(&iter, str, &match_start, &match_end);
				}
				else
				{
				found = gtk_text_iter_forward_search(&iter, str, GTK_TEXT_SEARCH_TEXT_ONLY, &match_start, &match_end, NULL);
				}
			}
		}

	if(found)
		{
		gtk_text_buffer_select_range(tb, &match_start, &match_end);
		read_insert_position(tr);
		tr->srch_line = tr->line;
		tr->srch_offs = tr->offs;

		/* tm = gtk_text_buffer_get_insert(tb); */ /* scan-build : never read */

		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(tr->text), &match_start, 0.0, TRUE, 0.0, 0.5);
		}
		else
		{
		gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);
		gtk_text_buffer_select_range(tb, &iter, &iter);
		}
	}
}


static gboolean iter_backward_search_caseins(
                              const GtkTextIter *iter,
                              gchar             *str,
                              GtkTextIter       *match_start,
                              GtkTextIter       *match_end)
{
GtkTextIter start = *iter;
GtkTextIter next;
gchar *line_text;
int offset;
int cmpval;

if(!str) return(FALSE);
offset = g_utf8_strlen (str, -1);

for(;;)
	{
	if(gtk_text_iter_is_start(&start))
		{
		break;
		}

	next = start;
	forward_chars_with_skipping (&next, offset);
        line_text = gtk_text_iter_get_visible_text (&start, &next);
	cmpval = strcasecmp(str, line_text);
	g_free(line_text);
        if(!cmpval)
		{
		*match_start = start;
		*match_end = next;
		return(TRUE);
		}

	gtk_text_iter_backward_char(&start);
	}

return(FALSE);
}


void tr_search_backward(char *str)
{
struct text_find_t *tr = selected_text_via_tab;

if((tr) && (tr->text))
	{
	GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tr->text));
	GtkTextMark *tm = gtk_text_buffer_get_insert(tb);
	GtkTextIter iter;
	gboolean found = FALSE;
	GtkTextIter match_start;
	GtkTextIter match_end;

	gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);

	tr->line = gtk_text_iter_get_line(&iter);
	tr->offs = gtk_text_iter_get_line_offset(&iter);

	if((tr->line == tr->srch_line) && (tr->offs == tr->srch_offs))
		{
		gtk_text_iter_backward_char(&iter);
		}

	if(str)
		{
		if(!matchcase_active)
			{
			found = iter_backward_search_caseins(&iter, str, &match_start, &match_end);
			}
			else
			{
			found = gtk_text_iter_backward_search(&iter, str, GTK_TEXT_SEARCH_TEXT_ONLY, &match_start, &match_end, NULL);
			}
		if(!found)
			{
			gtk_text_buffer_get_end_iter(tb, &iter);
			if(!matchcase_active)
				{
				found = iter_backward_search_caseins(&iter, str, &match_start, &match_end);
				}
				else
				{
				found = gtk_text_iter_backward_search(&iter, str, GTK_TEXT_SEARCH_TEXT_ONLY, &match_start, &match_end, NULL);
				}
			}
		}

	if(found)
		{
		gtk_text_buffer_select_range(tb, &match_start, &match_end);
		read_insert_position(tr);
		tr->srch_line = tr->line;
		tr->srch_offs = tr->offs;

		/* tm = gtk_text_buffer_get_insert(tb); */ /* scan-build : never read */
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(tr->text), &match_start, 0.0, TRUE, 0.0, 0.5);
		}
		else
		{
		gtk_text_buffer_get_iter_at_mark(tb, &iter, tm);
		gtk_text_buffer_select_range(tb, &iter, &iter);
		}
	}
}


static char *search_string = NULL;

static void search_backward(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

tr_search_backward(search_string);
}

static gboolean forward_noskip = FALSE;
static void search_forward(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

tr_search_forward(search_string, forward_noskip);
}

/* Signal callback for the filter widget.
   This catch the return key to update the signal area.  */
static
gboolean find_edit_cb (GtkWidget *widget, GdkEventKey *ev, gpointer *data)
{
(void)data;

  /* Maybe this test is too strong ?  */
  if (ev->keyval == GDK_KEY_Return)
    {
      const char *t = gtk_entry_get_text (GTK_ENTRY (widget));

      if(search_string) { free(search_string); search_string = NULL; }
      if (t == NULL || *t == 0)
	{
	}
      else
        {
	search_string = strdup(t);
        }

    search_forward(NULL, NULL);
    }
  return FALSE;
}

static void toggle_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

matchcase_active = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0);
tr_search_forward(search_string, TRUE);
}


static
void press_callback (GtkWidget *widget, gpointer *data)
{
GdkEventKey ev;

ev.keyval = GDK_KEY_Return;

forward_noskip = TRUE;
find_edit_cb (widget, &ev, data);
forward_noskip = FALSE;
}


void create_toolbar(GtkWidget *table)
    {
    GtkWidget *find_label;
    GtkWidget *find_entry;
    GtkWidget *tb;
    GtkWidget *stock;
#if !GTK_CHECK_VERSION(3,0,0)
    GtkStyle  *style;
#endif
    GtkWidget *hbox;
    int tb_pos = 0;
#if GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_box_set_homogeneous (GTK_BOX(hbox), FALSE);
#else
    hbox = gtk_hbox_new (FALSE, 1);
#endif
    gtk_widget_show (hbox);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_grid_attach (GTK_GRID (table), hbox, 0, 255, 255, 1);
    gtk_widget_set_hexpand (GTK_WIDGET (hbox), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET (hbox), FALSE); /* otherwise the bottom part stretches */
#else
    gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 255, 256,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
#endif

    find_label = gtk_label_new ("Find:");
    gtk_widget_show (find_label);
    gtk_box_pack_start (GTK_BOX (hbox), find_label, FALSE, FALSE, 0);

    find_entry = gtk_entry_new ();
    gtk_widget_show (find_entry);

    g_signal_connect(XXX_GTK_OBJECT(find_entry), "changed", G_CALLBACK(press_callback), NULL);
    g_signal_connect(XXX_GTK_OBJECT (find_entry), "key_press_event", (GCallback) find_edit_cb, NULL);
    gtk_box_pack_start (GTK_BOX (hbox), find_entry, FALSE, FALSE, 0);

    tb = gtk_toolbar_new();
#if !GTK_CHECK_VERSION(3,0,0)
    style = gtk_widget_get_style(tb);
    style->xthickness = style->ythickness = 0;
    gtk_widget_set_style (tb, style);
#endif
    gtk_widget_show (tb);

    gtk_toolbar_set_style(GTK_TOOLBAR(tb), GTK_TOOLBAR_ICONS);
    stock = XXX_gtk_toolbar_insert_stock(GTK_TOOLBAR(tb),
                                                 XXX_GTK_STOCK_GO_BACK,
                                                 "Search Back",
                                                 NULL,
                                                 G_CALLBACK(search_backward),
                                                 NULL,
                                                 tb_pos++);

#if !GTK_CHECK_VERSION(3,0,0)
    style = gtk_widget_get_style(stock);
    style->xthickness = style->ythickness = 0;
    gtk_widget_set_style (stock, style);
#endif
    gtk_widget_show(stock);

    stock = XXX_gtk_toolbar_insert_stock(GTK_TOOLBAR(tb),
                                                 XXX_GTK_STOCK_GO_FORWARD,
                                                 "Search Forward",
                                                 NULL,
                                                 G_CALLBACK(search_forward),
                                                 NULL,
                                                 tb_pos); /* scan-build: removed ++ on tb_pos as never read */

#if !GTK_CHECK_VERSION(3,0,0)
    style = gtk_widget_get_style(stock);
    style->xthickness = style->ythickness = 0;
    gtk_widget_set_style (stock, style);
#endif
    gtk_widget_show(stock);

    gtk_box_pack_start (GTK_BOX (hbox), tb, FALSE, FALSE, 0);

    matchcase_checkbutton = gtk_check_button_new_with_label("Match case");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(matchcase_checkbutton), matchcase_active);
    g_signal_connect (XXX_GTK_OBJECT (matchcase_checkbutton), "toggled", G_CALLBACK(toggle_callback), NULL);
    gtk_widget_show(matchcase_checkbutton);
    gtk_box_pack_start (GTK_BOX (hbox), matchcase_checkbutton, FALSE, FALSE, 0);
    }



static char *tmpnam_rtlbrowse(char *s, int *fd)
{
#if defined __MINGW32__

*fd = -1;
return(tmpnam(s));

#else
(void)s;

char *backpath = "gtkwaveXXXXXX";
char *tmpspace;
int len = strlen(P_tmpdir);
int i;

unsigned char slash = '/';
for(i=0;i<len;i++)
        {
        if((P_tmpdir[i] == '\\') || (P_tmpdir[i] == '/'))
                {
                slash = P_tmpdir[i];
                break;
                }
        }

tmpspace = malloc(len + 1 + strlen(backpath) + 1);
sprintf(tmpspace, "%s%c%s", P_tmpdir, slash, backpath);
*fd = mkstemp(tmpspace);
if(*fd<0)
        {
        fprintf(stderr, "tmpnam_rtlbrowse() could not create tempfile, exiting.\n");
        perror("Why");
        exit(255);
        }

return(tmpspace);

#endif
}


static int is_identifier(char ch)
{
int rc = ((ch>='a')&&(ch<='z')) || ((ch>='A')&&(ch<='Z')) || ((ch>='0')&&(ch<='9')) || (ch == '_') || (ch == '$');

return(rc);
}


static char *hexify(char *s)
{
int len = strlen(s);

if(len < 4)
	{
	char *s2 = malloc(len+1+1);
	int idx;

	s2[0]='b';
	for(idx = 0; idx < len; idx++)
		{
		s2[idx+1] = toupper((int)(unsigned char)s[idx]);
		}
	s2[idx+1] = 0;

	free(s);
	return(s2);
	}
	else
	{
	int skip = len & 3;
	int siz = ((len + 3) / 4) + 1;
	char *sorig = s;
	char *s2 = calloc(1, siz);
	int idx;
	char *pnt = s2;
	char arr[5] = { 0, 0, 0, 0, 0 }; /* scan-build : but arr[4] should work ok */

	while(*s)
		{
		char isx, isz;
		int val;

		if(!skip)
			{
			arr[0] = toupper((int)(unsigned char)*(s++));
			arr[1] = toupper((int)(unsigned char)*(s++));
			arr[2] = toupper((int)(unsigned char)*(s++));
			arr[3] = toupper((int)(unsigned char)*(s++));
			}
			else
			{
			int j = 3;
			for(idx = skip-1; idx>=0; idx--)
				{
				arr[j] = toupper((int)(unsigned char)s[idx]);
				j--;
				}
			for(idx = j; idx >= 0; idx--)
				{
				arr[idx] = ((arr[j+1] == 'X') || (arr[j+1] == 'Z')) ? arr[j+1] : '0';
				}

			s+=skip;
			skip = 0;
			}

		isx = isz = 0;
		val = 0;
		for(idx=0; idx<4; idx++)
			{
			val <<= 1;
			if(arr[idx] == '0') continue;
			if(arr[idx] == '1') { val |= 1; continue; }
			if(arr[idx] == 'Z') { isz++; continue; }
			isx++;
			}

		if(isx)
			{
			*(pnt++) = (isx==4) ? 'x' : 'X';
			}
		else
		if(isz)
			{
			*(pnt++) = (isz==4) ? 'z' : 'Z';
			}
		else
			{
			*(pnt++) = "0123456789ABCDEF"[val];
			}
		}

	free(sorig);
	return(s2);
	}
}


int fst_alpha_strcmpeq(const char *s1, const char *s2)
{
for(;;)
	{
	char c1 = *s1;
	char c2 = *s2;

	switch(c1)
		{
		case ' ':
		case '\t':
		case '[':
			c1 = 0;

		default:
			break;
		}

	switch(c2)
		{
		case ' ':
		case '\t':
		case '[':
			c2 = 0;

		default:
			break;
		}

	if(c1 != c2) { return(1); }
	if(!c1) break;

	s1++; s2++;
	}

return(0);
}


/* for dnd */
#define WAVE_DRAG_TAR_NAME_0         "text/plain"
#define WAVE_DRAG_TAR_INFO_0         0

#define WAVE_DRAG_TAR_NAME_1         "text/uri-list"         /* not url-list */
#define WAVE_DRAG_TAR_INFO_1         1

#define WAVE_DRAG_TAR_NAME_2         "STRING"
#define WAVE_DRAG_TAR_INFO_2         2


static void DNDBeginCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

/* nothing */
}

static void DNDEndCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

/* nothing */
}

static void DNDDataDeleteCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

/* nothing */
}

static void DNDDataRequestCB(
        GtkWidget *widget, GdkDragContext *dc,
        GtkSelectionData *selection_data, guint info, guint t,
        gpointer data
)
{
(void)dc;
(void)t;
(void)info;

struct logfile_context_t *ctx = (struct logfile_context_t *)data;
GtkWidget *text = (GtkWidget *)widget;
gchar *sel = NULL;

gchar *sel2 = NULL;
GtkTextIter start;
GtkTextIter end;
int ok = 0;
char ch;

if((!text)||(!ctx)) return;

if (gtk_text_buffer_get_selection_bounds (gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)), &start, &end))
	{
	ok = 1;
	}
else
if(((void *)gtk_widget_get_window(widget)) == pressWindow)
	{
	GtkTextView *text_view = GTK_TEXT_VIEW(text);
	gint buffer_x, buffer_y;
	gint s_trailing, e_trailing;

	gtk_text_view_window_to_buffer_coords(text_view, GTK_TEXT_WINDOW_WIDGET, pressX, pressY, &buffer_x, &buffer_y);

	gtk_text_view_get_iter_at_position  (text_view, &start, &s_trailing, buffer_x, buffer_y);
	gtk_text_view_get_iter_at_position  (text_view, &end,   &e_trailing, buffer_x, buffer_y);
	gtk_text_iter_forward_char(&end);
	ok = 1;
	}

pressWindow = NULL;

if(ok)
       	{
       	if(gtk_text_iter_compare (&start, &end) < 0)
               	{
               	sel = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              &start, &end, FALSE);

		if(sel && strlen(sel))
			{
			while(gtk_text_iter_backward_char(&start))
				{
		               	sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              &start, &end, FALSE);
				if(!sel2) break;
				ch = *sel2;
				g_free(sel2);
				if(!is_identifier(ch))
					{
					gtk_text_iter_forward_char(&start);
					break;
					}
				}

			gtk_text_iter_backward_char(&end);
			for(;;)
				{
				gtk_text_iter_forward_char(&end);
		               	sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              	&start, &end, FALSE);
				if(!sel2) break;
				ch = *(sel2 + strlen(sel2) - 1);
				g_free(sel2);
				if(!is_identifier(ch))
					{
					gtk_text_iter_backward_char(&end);
					break;
					}
				}

		 	sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
						&start, &end, FALSE);

			g_free(sel);
			sel = sel2;
			sel2 = NULL;
			}

		/* gtk_text_buffer_delete_selection (gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)), 0, 0); ...no need to delete because of update_ctx_when_idle() */
		}

	}


if(sel)
	{
	JRB strs, node;
	int fd;
	char *fname = tmpnam_rtlbrowse("rtlbrowse", &fd);
	FILE *handle = fopen(fname, "wb");
	int lx;
	int degate = 0;
	int cnt = 0;

	if(!handle)
		{
        	fprintf(stderr, "Could not open cutpaste file '%s'\n", fname);
        	return;
        	}
	fprintf(handle, "%s", sel);
	fclose(handle);
	if(fd>=0) close(fd);

	v_preproc_name = fname;
	strs = make_jrb();
    	while((lx = yylex()))
        	{
        	char *pnt = yytext;

		if(!verilog_2005)
			{
			if(lx == V_KW_2005) lx = V_ID;
			}

                if(lx==V_ID)
                        {
			if(!degate)
				{
				JRB str = jrb_find_str(strs, pnt);
				Jval jv;
				jv.v = NULL;
				if(!str)
					{
					jrb_insert_str(strs, strdup(pnt), jv);
					cnt++;
					}
				}
                        }
		else if(lx==V_IGNORE)
			{
			if(*pnt == '[') degate = 1;
			else if(*pnt == ']') degate = 0;
			}
		}

	unlink(fname);
	free(fname);

	if(cnt)
		{
		int title_len = 5 + strlen(ctx->title) + 1;
		char *tpnt = calloc(1, title_len + 1);
		char *op = ctx->title;
		char *np = tpnt;
		char *mlist = NULL;
		int mlen = 0;

		strcpy(np, "{net ");
		np+=5;
		while(*op)
			{
			if(*op == '.') *np = ' '; else *np = *op;
			op++;
			np++;
			}
		*np = ' ';

		jrb_traverse(node, strs)
			{
			int slen = strlen(node->key.s);
			int singlen = title_len + slen + 2;
			char *singlist = calloc(1, singlen + 1);
			memcpy(singlist, tpnt, title_len);
			strcpy(singlist + title_len, node->key.s);
			strcpy(singlist + title_len + slen, "} ");

			if(mlist)
				{
				mlist = realloc(mlist, mlen + singlen + 1);
				strcpy(mlist + mlen, singlist);
				mlen += singlen;
				}
				else
				{
				mlist = strdup(singlist);
				mlen = singlen;
				}

			free(singlist);
			free(node->key.s);
			}

		gtk_selection_data_set(selection_data,GDK_SELECTION_TYPE_STRING, 8, (guchar*)mlist, mlen);
		free(mlist);

		update_ctx_when_idle(text);

		free(tpnt);
		}
		else
		{
		update_ctx_when_idle(text);
		}

	jrb_free_tree(strs);
	g_free(sel);
	}

}




void log_text(GtkWidget *text, gpointer font, char *str)
{
(void)font;

gtk_text_buffer_insert_with_tags (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &iterx,
                                 str, -1, mono_tag, size_tag, NULL);
}

void log_text_bold(GtkWidget *text, gpointer font, char *str)
{
(void)font;

gtk_text_buffer_insert_with_tags (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &iterx,
                                 str, -1, bold_tag, mono_tag, size_tag, fwht_tag, blue_tag, NULL);
}

void log_text_active(GtkWidget *text, gpointer font, char *str)
{
(void)font;

gtk_text_buffer_insert_with_tags (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &iterx,
                                 str, -1, dgray_tag, mono_tag, size_tag, NULL);
}

void log_text_prelight(GtkWidget *text, gpointer font, char *str)
{
(void)font;

gtk_text_buffer_insert_with_tags (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &iterx,
                                 str, -1, lgray_tag, mono_tag, size_tag, NULL);
}

static void
log_realize_text (GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

/* nothing for now */
}


/* lxt2 iteration handling... (lxt2 doesn't have a direct "value at" function) */
static JRB lx2vals = NULL;

static void lx2_iter_fn(struct lxt2_rd_trace **lt, lxtint64_t *pnt_time, lxtint32_t *pnt_facidx, char **pnt_value)
{
(void)lt;

if(*pnt_time <= (lxtint64_t)anno_ctx->marker)
	{
	JRB node = jrb_find_int(lx2vals, *pnt_facidx);
	Jval jv;

	if(node)
		{
		free(node->val.s);
		node->val.s = strdup(*pnt_value);
		}
		else
		{
		jv.s = strdup(*pnt_value);
		jrb_insert_int(lx2vals, *pnt_facidx, jv);
		}
	}
}


static void
import_doubleclick(GtkWidget *text, char *s)
{
struct text_find_t *t = text_root;
char *s2;
ds_Tree *ft = flattened_mod_list_root;

while(t)
	{
	if(text == t->text)
		{
		break;
		}
	t = t->next;
	}

if(!t) return;

s2 = malloc(strlen(t->ctx->which->fullname) + 1 + strlen(s) + 1);
sprintf(s2, "%s.%s", t->ctx->which->fullname, s);

while(ft)
	{
	if(!strcmp(s2, ft->fullname))
		{
		bwlogbox(ft->fullname, 640 + 8*8, ft, 0);
		break;
		}

	ft = ft->next_flat;
	}

free(s2);
}


static gboolean
button_release_event (GtkWidget *text, GdkEventButton *event)
{
(void)event;

gchar *sel;

GtkTextIter start;
GtkTextIter end;

if (gtk_text_buffer_get_selection_bounds (gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                         &start, &end))
       {
       if(gtk_text_iter_compare (&start, &end) < 0)
               {
               sel = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              &start, &end, FALSE);

		if(sel)
			{
			if(strlen(sel))
			{
			int i, len = strlen(sel);
			char *sel2;
			char ch;

			for(i=0;i<len;i++)
				{
				if(!is_identifier(sel[i])) goto bail;
				}

			while(gtk_text_iter_backward_char(&start))
				{
		               	sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              &start, &end, FALSE);
				if(!sel2) break;
				ch = *sel2;
				g_free(sel2);
				if(!is_identifier(ch))
					{
					gtk_text_iter_forward_char(&start);
					break;
					}
				}

			gtk_text_iter_backward_char(&end);
			for(;;)
				{
				gtk_text_iter_forward_char(&end);
		               	sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                                              	&start, &end, FALSE);
				if(!sel2) break;
				ch = *(sel2 + strlen(sel2) - 1);
				g_free(sel2);
				if(!is_identifier(ch))
					{
					gtk_text_iter_backward_char(&end);
					break;
					}
				}

		 	sel2 = gtk_text_buffer_get_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
						&start, &end, FALSE);


			/* oec->set_selection(oe, lft, rgh); */

			import_doubleclick(text, sel2);
			g_free(sel2);
			}
bail:			g_free(sel);
			}
		}
	}


return(FALSE); /* call remaining handlers... */
}

static        gint
scroll_event( GtkWidget * widget, GdkEventScroll * event, gpointer text)
{
(void)widget;

  GtkTextView *text_view = GTK_TEXT_VIEW(text);
  /* GtkAdjustment *hadj = text_view->hadjustment; */
  GtkAdjustment *vadj = YYY_gtk_text_view_get_vadjustment(YYY_GTK_TEXT_VIEW(text_view));

  gdouble s_val = gtk_adjustment_get_step_increment(vadj);
  gdouble p_val = gtk_adjustment_get_page_increment(vadj);

  switch ( event->direction )
  {
    case GDK_SCROLL_UP:
	gtk_adjustment_set_value(vadj, gtk_adjustment_get_value(vadj) - s_val);
	if(gtk_adjustment_get_value(vadj) < gtk_adjustment_get_lower(vadj)) gtk_adjustment_set_value(vadj, gtk_adjustment_get_lower(vadj));

	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "changed");
        g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "value_changed");

      break;

    case GDK_SCROLL_DOWN:
	gtk_adjustment_set_value(vadj, gtk_adjustment_get_value(vadj) + s_val);
	if(gtk_adjustment_get_value(vadj) > gtk_adjustment_get_upper(vadj) - p_val) gtk_adjustment_set_value(vadj, gtk_adjustment_get_upper(vadj) - p_val);

	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "changed");
        g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "value_changed");

    default:
      break;
  }
  return(TRUE);
}


/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_log_text (GtkWidget **textpnt)
{
GtkWidget *text;
GtkWidget *scrolled_window;

text = gtk_text_view_new ();
gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), &iterx);
bold_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "bold",
                                      	"weight", PANGO_WEIGHT_BOLD, NULL);
dgray_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "dk_gray_background",
			      "background", "dark gray", NULL);
lgray_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "lt_gray_background",
			      "background", "light gray", NULL);
fwht_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "white_foreground",
			      "foreground", "white", NULL);
blue_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "blue_background",
			      "background", "blue", NULL);
#ifdef MAC_INTEGRATION
mono_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "monospace",
					"family", "monospace", NULL);
size_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "fsiz",
					"size", 10 * PANGO_SCALE, NULL);
#else
mono_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "monospace",
					"family", "monospace", NULL);
size_tag = gtk_text_buffer_create_tag (gtk_text_view_get_buffer(GTK_TEXT_VIEW (text)), "fsiz",
					"size", 8 * PANGO_SCALE, NULL);
#endif
*textpnt = text;

gtk_widget_set_size_request(GTK_WIDGET(text), 100, 100);
gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
gtk_widget_show (text);

scrolled_window = gtk_scrolled_window_new (NULL, NULL);
gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
gtk_container_add (GTK_CONTAINER (scrolled_window), text);
gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 5);
gtk_widget_show(scrolled_window);

/* Add a handler to put a message in the text widget when it is realized */
g_signal_connect (XXX_GTK_OBJECT (text), "realize",
                        G_CALLBACK (log_realize_text), NULL);

g_signal_connect(XXX_GTK_OBJECT(text), "button_release_event",
                       G_CALLBACK(button_release_event), NULL);

g_signal_connect (XXX_GTK_OBJECT (text), "scroll_event",G_CALLBACK(scroll_event), text);
gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_CHAR);
return(scrolled_window);
}

/***********************************************************************************/

static void ok_callback(GtkWidget *widget, struct logfile_context_t *ctx)
{
(void)widget;

  bwlogbox(ctx->which->fullname, ctx->width, ctx->which, (ctx->display_mode == 0));
}


gboolean update_ctx_when_idle(gpointer textview_or_dummy)
{
struct text_find_t *t;

if((anno_ctx) && (anno_ctx->cygwin_remote_kill))
	{
	anno_ctx->cygwin_remote_kill = 0;
	exit(0); /* remote kill command from gtkwave */
	}

if(textview_or_dummy == NULL)
	{
	if(anno_ctx)
		{
		if((anno_ctx->marker_set != old_marker_set) || (old_marker != anno_ctx->marker))
			{
			old_marker_set = anno_ctx->marker_set;
			old_marker = anno_ctx->marker;
			}
			else
			{
			return(TRUE);
			}
		}
		else
		{
		return(TRUE);
		}
	}

t = text_root;
while(t)
	{
	if(textview_or_dummy)
		{
		if(textview_or_dummy != (gpointer)(t->text))
			{
			t = t->next;
			continue;
			}
		}

	if(t->window)
		{
		if((!t->ctx->display_mode) || (textview_or_dummy))
			{
			GtkTextIter st_iter, en_iter;

			GtkAdjustment *vadj = YYY_gtk_text_view_get_vadjustment(YYY_GTK_TEXT_VIEW (t->text));
			gdouble vvalue = gtk_adjustment_get_value(vadj);

			read_insert_position(t);
			gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW (t->text)), &st_iter);
			gtk_text_buffer_get_end_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW (t->text)), &en_iter);
			gtk_text_buffer_delete(gtk_text_view_get_buffer(GTK_TEXT_VIEW (t->text)), &st_iter, &en_iter);

			gtk_text_buffer_get_start_iter (gtk_text_view_get_buffer(GTK_TEXT_VIEW (t->text)), &iterx);

			bold_tag = t->bold_tag;
			dgray_tag = t->dgray_tag;
			lgray_tag = t->lgray_tag;
			blue_tag = t->blue_tag;
			fwht_tag = t->fwht_tag;
			mono_tag = t->mono_tag;
			size_tag = t->size_tag;

			bwlogbox_2(t->ctx, NULL, t->button, t->text);
			set_insert_position(t);

			gtk_adjustment_set_value(vadj, vvalue);
			g_signal_emit_by_name (XXX_GTK_OBJECT (vadj), "changed");
			g_signal_emit_by_name (XXX_GTK_OBJECT (vadj), "value_changed");
			}
		}
	t = t->next;
	}

return(TRUE);
}


static void destroy_callback(GtkWidget *widget, gpointer dummy)
{
(void)dummy;

struct text_find_t *t = text_root, *tprev = NULL;
struct logfile_context_t *ctx = NULL;
int which = (notebook != NULL);
GtkWidget *matched = NULL;

while(t)
	{
	if((which ? t->button : t->window)==widget)
		{
		matched = t->window;
		if(t==selected_text_via_tab) selected_text_via_tab = NULL;
		if(tprev)	/* prune out struct text_find_t */
			{
			tprev->next = t->next;
			ctx = t->ctx;
			free(t);
			break;
			}
			else
			{
			text_root = t->next;
			ctx = t->ctx;
			free(t);
			break;
			}
		}

	tprev = t;
	t = t->next;
	}

if(matched) gtk_widget_destroy(matched);

if(ctx)
	{
	JRB node;
	JRB varnames = ctx->varnames;

	if(ctx->title) free(ctx->title);

	/* Avoid dereferencing null pointers. */
	if (varnames == NULL) return;

        /* free up variables list */
        jrb_traverse(node, varnames)
		{
                if(node->val2.v) free(node->val2.v);
                free(node->key.s);
                }

        jrb_traverse(node, varnames)
                {
                struct jrb_chain *jvc = node->jval_chain;
                while(jvc)
                        {
                        struct jrb_chain *jvcn = jvc->next;
                        free(jvc);
                        jvc = jvcn;
                        }
                }

        jrb_free_tree(varnames);

	free(ctx);
	}
}


static gint destroy_via_closebutton_release(GtkWidget *widget, GdkEventButton *event)
{
GtkAllocation allocation;
gtk_widget_get_allocation(widget, &allocation);

if((event->x<0)||(event->x>=allocation.width)||(event->y<0)||(event->y>=allocation.height))
	{
        /* let gtk take focus from us with focus out event */
        }
	else
	{
	destroy_callback(widget, NULL);
	}

return(TRUE);
}

void bwlogbox(char *title, int width, ds_Tree *t, int display_mode)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox, *button1;
    GtkWidget *label, *separator;
    GtkWidget *ctext;
    GtkWidget *text;
    GtkWidget *close_button = NULL;
    gint pagenum = 0;
    FILE *handle;
    struct logfile_context_t *ctx;
    char *default_text = t->filename;

    handle = fopen(default_text, "rb");
    if(!handle)
	{
	if(strcmp(default_text, "(VerilatorTop)"))
		{
		fprintf(stderr, "Could not open sourcefile '%s'\n", default_text);
		}
	return;
	}
    fclose(handle);

/* nothing */

    /* create a new nonmodal window */
    if(!notebook)
	{
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    	if(fontname_logfile)
		{
    		set_winsize(window, width*1.8, 640);
		}
		else
		{
    		set_winsize(window, width, 640);
		}
    	gtk_window_set_title(GTK_WINDOW (window), title);
	}

	else
	{
	GtkWidget *tbox;
	GtkWidget *l1;
	GtkWidget *image;
#if !GTK_CHECK_VERSION(3,0,0)
	GtkRcStyle *rcstyle;
#endif

#if GTK_CHECK_VERSION(3,0,0)
	window = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	tbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_set_homogeneous (GTK_BOX(tbox), FALSE);
#else
	window = gtk_hpaned_new();
	tbox = gtk_hbox_new(FALSE, 0);
#endif
	l1 = gtk_label_new(title);

	/* code from gedit... */
        /* setup close button */
        close_button = gtk_button_new ();
        gtk_button_set_relief (GTK_BUTTON (close_button),
                               GTK_RELIEF_NONE);
        /* don't allow focus on the close button */
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_focus_on_click (GTK_WIDGET (close_button), FALSE);
#else
        gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);
#endif

        /* make it as small as possible */
#if !GTK_CHECK_VERSION(3,0,0)
        rcstyle = gtk_rc_style_new ();
        rcstyle->xthickness = rcstyle->ythickness = 0;
        gtk_widget_modify_style (close_button, rcstyle);
        g_object_unref(rcstyle); /* was gtk_rc_style_unref (rcstyle) */
#endif

#if GTK_CHECK_VERSION(3,0,0)
        image = gtk_image_new_from_icon_name (XXX_GTK_STOCK_CLOSE, 
                                          GTK_ICON_SIZE_MENU);
#else
        image = gtk_image_new_from_stock (XXX_GTK_STOCK_CLOSE, 
                                          GTK_ICON_SIZE_MENU);
#endif
        gtk_container_add (GTK_CONTAINER (close_button), image);
	/* ...code from gedit */

	gtk_widget_show(image);
        gtk_widget_show(close_button);

	gtk_box_pack_start(GTK_BOX (tbox), l1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (tbox), close_button, FALSE, FALSE, 0);

	gtk_widget_show(l1);
	gtk_widget_show(tbox);

        pagenum =
		gtk_notebook_append_page_menu  (GTK_NOTEBOOK(notebook), window, tbox, gtk_label_new(title));

	g_signal_connect (XXX_GTK_OBJECT (close_button), "button_release_event",
	                        G_CALLBACK (destroy_via_closebutton_release), NULL); /* this will destroy the tab by destroying the parent container */
	}

#if GTK_CHECK_VERSION(3,0,0)
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
    gtk_box_set_homogeneous (GTK_BOX(vbox), FALSE);
#else
    vbox = gtk_vbox_new (FALSE, 1);
#endif
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    label=gtk_label_new(default_text);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

#if GTK_CHECK_VERSION(3,0,0)
    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
#else
    separator = gtk_hseparator_new ();
#endif
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    ctext=create_log_text(&text);
    gtk_box_pack_start (GTK_BOX (vbox), ctext, TRUE, TRUE, 0);
    gtk_widget_show (ctext);

    g_signal_connect(XXX_GTK_OBJECT(text), "button_press_event",G_CALLBACK(button_press_event), NULL);

#if GTK_CHECK_VERSION(3,0,0)
    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
#else
    separator = gtk_hseparator_new ();
#endif
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

#if GTK_CHECK_VERSION(3,0,0)
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_box_set_homogeneous (GTK_BOX(hbox), FALSE);
#else
    hbox = gtk_hbox_new (FALSE, 1);
#endif

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    ctx = (struct logfile_context_t *)calloc(1, sizeof(struct logfile_context_t));
    ctx->which = t;
    ctx->display_mode = display_mode;
    ctx->width = width;
    ctx->title = strdup(title);

#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(XXX_GTK_OBJECT(window), "draw", G_CALLBACK(draw_event), NULL);
#else
    g_signal_connect(XXX_GTK_OBJECT(window), "expose_event",G_CALLBACK(expose_event_local), NULL);
#endif
    button1 = gtk_button_new_with_label (display_mode ? "View Design Unit Only": "View Full File");
    gtk_widget_set_size_request(button1, 100, -1);
    g_signal_connect(XXX_GTK_OBJECT (button1), "clicked",
                               G_CALLBACK(ok_callback),
                               ctx);
    gtk_widget_show (button1);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_box_pack_start(GTK_BOX(hbox), button1, TRUE, TRUE, 0);
#else
    gtk_container_add (GTK_CONTAINER (hbox), button1);
#endif
    gtk_widget_set_can_default (button1, TRUE);
    g_signal_connect_swapped (XXX_GTK_OBJECT (button1),
                                "realize",
                             (GCallback) gtk_widget_grab_default,
                             XXX_GTK_OBJECT (button1));


    gtk_widget_show(window);

    bwlogbox_2(ctx, window, close_button, text);

    if(text)
	{
	GtkWidget *src = text;
        GtkTargetEntry target_entry[3];
        /* Set up the list of data format types that our DND
         * callbacks will accept.
         */
        target_entry[0].target = WAVE_DRAG_TAR_NAME_0;
        target_entry[0].flags = 0;
        target_entry[0].info = WAVE_DRAG_TAR_INFO_0;
        target_entry[1].target = WAVE_DRAG_TAR_NAME_1;
        target_entry[1].flags = 0;
        target_entry[1].info = WAVE_DRAG_TAR_INFO_1;
        target_entry[2].target = WAVE_DRAG_TAR_NAME_2;
        target_entry[2].flags = 0;
        target_entry[2].info = WAVE_DRAG_TAR_INFO_2;

        gtk_drag_source_set(
        	src,
                GDK_BUTTON2_MASK,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
                GDK_ACTION_COPY | GDK_ACTION_MOVE
                );
        g_signal_connect(XXX_GTK_OBJECT(src), "drag_begin", G_CALLBACK(DNDBeginCB), ctx);
        g_signal_connect(XXX_GTK_OBJECT(src), "drag_end", G_CALLBACK(DNDEndCB), ctx);
        g_signal_connect(XXX_GTK_OBJECT(src), "drag_data_get", G_CALLBACK(DNDDataRequestCB), ctx);
        g_signal_connect(XXX_GTK_OBJECT(src), "drag_data_delete", G_CALLBACK(DNDDataDeleteCB), ctx);
        }

    if(notebook)
	{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), pagenum);
	}
}


void bwlogbox_2(struct logfile_context_t *ctx, GtkWidget *window, GtkWidget *button, GtkWidget *text)
{
    ds_Tree *t = ctx->which;
    int display_mode = ctx->display_mode;
    char *title = ctx->title;

    FILE *handle;
    int lx;
    int lx_module_line = 0;
    int lx_module_line_locked = 0;
    int lx_endmodule_line_locked = 0;

    struct wave_logfile_lines_t *wlog_head=NULL, *wlog_curr=NULL;
    int wlog_size = 0;
    int line_no;
    int s_line_find = -1, e_line_find = -1;
    struct text_find_t *text_curr;

    char *default_text = t->filename;
    char *design_unit = t->item;
    int s_line = t->s_line;
    int e_line = t->e_line;


    handle = fopen(default_text, "rb");
    if(!handle)
	{
	if(strcmp(default_text, "(VerilatorTop)"))
		{
		fprintf(stderr, "Could not open sourcefile '%s'\n", default_text);
		}
	return;
	}
    fclose(handle);

    v_preproc_name = default_text;
    while((lx = yylex()))
	{
	char *pnt = yytext;
	struct wave_logfile_lines_t *w;
	line_no = my_yylineno;

        if(!verilog_2005)
		{
                if(lx == V_KW_2005) lx = V_ID;
                }

	if(!lx_module_line_locked)
		{
		if(lx==V_MODULE)
			{
			lx_module_line = line_no;
			}
		else
		if((lx==V_ID)&&(lx_module_line))
			{
			if(!strcmp(pnt, design_unit))
				{
				lx_module_line_locked = 1;
				s_line_find = lx_module_line;
				}
			}
		else
		if((lx != V_WS)&&(lx_module_line))
			{
			lx_module_line = 0;
			}
		}
	else
		{
		if((lx==V_ENDMODULE)&&(!lx_endmodule_line_locked))
			{
			e_line_find = line_no;
			lx_endmodule_line_locked = 1;
			}
		}

	w = calloc(1, sizeof(struct wave_logfile_lines_t));
	w->line_no = line_no;
	w->tok = lx;
	wlog_size += yyleng;
	w->text = malloc(yyleng + 1);
	strcpy(w->text, pnt);
	if(!wlog_curr) { wlog_head = wlog_curr = w; } else { wlog_curr->next = w; wlog_curr = w; }
	}

    log_text(text, NULL, "Design unit ");
    log_text_bold(text, NULL, design_unit);
	{
	char buf[8192];

	s_line = s_line_find > 0 ? s_line_find : s_line;
	e_line = e_line_find > 0 ? e_line_find : e_line;

	sprintf(buf, " occupies lines %d - %d.\n", s_line, e_line);
	log_text(text, NULL, buf);
	if(anno_ctx)
		{
		sprintf(buf, "Marker time for '%s' is %s.\n", anno_ctx->aet_name,
			anno_ctx->marker_set ? anno_ctx->time_string: "not set");
		log_text(text, NULL, buf);
		}

	log_text(text, NULL, "\n");
	}

    if(wlog_curr)
	{
	struct wave_logfile_lines_t *w = wlog_head;
	struct wave_logfile_lines_t *wt;
	char *pnt = malloc(wlog_size + 1);
	char *pnt2 = pnt;
	JRB varnames = NULL;
	JRB node;
	int numvars = 0;

	/* build up list of potential variables in this module */
	if(!display_mode && !ctx->varnames)
		{
		varnames = make_jrb();
		while(w)
			{
			if(w->tok == V_ID)
				{
				if((w->line_no >= s_line) && (w->line_no <= e_line))
					{
					if(strcmp(w->text, design_unit)) /* filter out design unit name */
						{
						node = jrb_find_str(varnames, w->text);

						if(!node)
							{
							Jval dummy;
							dummy.v = NULL;
							jrb_insert_str(varnames, strdup(w->text), dummy);
							numvars++;
							}
						}
					}
				}
			w = w->next;
			}
		}

	if((anno_ctx)&&(anno_ctx->marker_set)&&(!display_mode)) /* !display_mode somehow was removed earlier causing full view crashes */
		{
		int resolved = 0;

/*************************/
		if(fst)
			{
			int numfacs=fstReaderGetVarCount(fst);
			int i;
			const char *scp_nam = NULL;
			fstHandle fh = 0;
			int new_scope_encountered = 1;
			int good_scope = 0;

			if(ctx->varnames) goto skip_resolved_fst;

			jrb_traverse(node, varnames) { node->val.i = -1; }
			fstReaderIterateHierRewind(fst);
			fstReaderResetScope(fst);

			for(i=0;i<numfacs;i++)
				{
				struct fstHier *h;

				new_scope_encountered = 0;
				while((h = fstReaderIterateHier(fst)))
				        {
					int do_brk = 0;
				        switch(h->htyp)
				                {
				                case FST_HT_SCOPE:
				                        scp_nam = fstReaderPushScope(fst, h->u.scope.name, NULL);
							new_scope_encountered = 1;
				                        break;
				                case FST_HT_UPSCOPE:
				                        scp_nam = fstReaderPopScope(fst);
							new_scope_encountered = 1;
				                        break;
				                case FST_HT_VAR:
				                        scp_nam = fstReaderGetCurrentFlatScope(fst);
							if(!h->u.var.is_alias) fh++;
							do_brk = 1;
							break;
						default:
							break;
						}
					if(do_brk) break;
					}
				if(!h) break;

				if(!new_scope_encountered)
					{
					if(!good_scope)
						{
						continue;
						}
					}
					else
					{
					good_scope = !strcmp(scp_nam, title);
					}

				if(!good_scope)
					{
					if(resolved == numvars)
						{
						break;
						}
					}
					else
					{
					jrb_traverse(node, varnames)
						{
						if(node->val.i < 0)
							{
							if(!fst_alpha_strcmpeq(h->u.var.name, node->key.s))
								{
								resolved++;
								if(h->u.var.is_alias)
									{
									node->val.i = h->u.var.handle;
									}
									else
									{
									node->val.i = fh;
									}
								}
							}
							else /* bitblasted */
							{
							if(!fst_alpha_strcmpeq(h->u.var.name, node->key.s))
								{
								struct jrb_chain *jvc = node->jval_chain;
								if(jvc) {
									while(jvc->next) jvc = jvc->next;
									jvc->next = calloc(1, sizeof(struct jrb_chain));
									jvc = jvc->next;
									}
									else
									{
									jvc = calloc(1, sizeof(struct jrb_chain));
									node->jval_chain = jvc;
									}

								if(h->u.var.is_alias)
									{
									jvc->val.i = h->u.var.is_alias;
									}
									else
									{
									jvc->val.i = fh;
									}
								}
							}
						}
					}
				}
/* resolved_fst: */
			ctx->varnames = varnames;
			ctx->resolved = resolved;
skip_resolved_fst:
			varnames = ctx->varnames;
			resolved = ctx->resolved;

			jrb_traverse(node, varnames)
				{
				if(node->val.i >= 0)
					{
					char rcb[65537];
					char *rc;
					struct jrb_chain *jvc = node->jval_chain;
					char first_char;

					rc = fstReaderGetValueFromHandleAtTime(fst, anno_ctx->marker, node->val.i, rcb);
					first_char = rc ? rc[0] : '?';

					if(!jvc)
						{
						if(rc)
							{
							node->val2.v = hexify(strdup(rc));
							}
							else
							{
							node->val2.v = NULL;
							}
						}
						else
						{
						char *rc2;
						int len = rc ? strlen(rc) : 0;
						int iter = 1;

						while(jvc)
							{
							fstReaderGetValueFromHandleAtTime(fst, anno_ctx->marker, jvc->val.i, rc);
							len+= (rc ? strlen(rc) : 0); /* scan-build : possible null pointer */
							iter++;
							jvc = jvc->next;
							}

						if(iter==len)
							{
							int pos = 1;
							jvc = node->jval_chain;
							rc2 = calloc(1, len+1);
							rc2[0] = first_char;

							while(jvc)
								{
								char rcv[65537];
								fstReaderGetValueFromHandleAtTime(fst, anno_ctx->marker, jvc->val.i, rcv);
								rc2[pos++] = *rcv;
								jvc = jvc->next;
								}

							node->val2.v = hexify(strdup(rc2));
							free(rc2);
							}
							else
							{
							node->val2.v = NULL;
							}
						}
					}
					else
					{
					node->val2.v = NULL;
					}
				}
			}
/*************************/
		else if(vzt)
			{
			int numfacs=vzt_rd_get_num_facs(vzt);
			int i;
			int tlen;
			char *pfx = NULL;

			if(ctx->varnames) goto skip_resolved_vzt;

			pfx = malloc((tlen=strlen(title))+1+1);
			strcpy(pfx, title);
			strcat(pfx+tlen, ".");
			tlen++;

			jrb_traverse(node, varnames) { node->val.i = -1; }

			for(i=0;i<numfacs;i++)
				{
				char *fnam = vzt_rd_get_facname(vzt, i);

				if(!strncmp(fnam, pfx, tlen))
					{
					if(!strchr(fnam+tlen, '.'))
						{
						jrb_traverse(node, varnames)
							{
							int mat = 0;

							if(node->val.i < 0)
								{
								if(!strcmp(fnam+tlen, node->key.s))
									{
									mat = 1;
									if(vzt->flags[i] & VZT_RD_SYM_F_ALIAS)
										{
										node->val.i = vzt_rd_get_alias_root(vzt, i);
										}
										else
										{
										node->val.i = i;
										}
									}
								}
								else /* bitblasted */
								{
								if(!strcmp(fnam+tlen, node->key.s))
									{
									struct jrb_chain *jvc = node->jval_chain;

									mat = 1;

									if(jvc) {
										while(jvc->next) jvc = jvc->next;
										jvc->next = calloc(1, sizeof(struct jrb_chain));
										jvc = jvc->next;
										}
										else
										{
										jvc = calloc(1, sizeof(struct jrb_chain));
										node->jval_chain = jvc;
										}

									if(vzt->flags[i] & VZT_RD_SYM_F_ALIAS)
										{
										jvc->val.i = vzt_rd_get_alias_root(vzt, i);
										}
										else
										{
										jvc->val.i = i;
										}
									}
								}

							if(mat)
								{
								if(i==(numfacs-1))
									{
									resolved++;
									}
									else
									{
									char *fnam2 = vzt_rd_get_facname(vzt, i+1);
									if(strcmp(fnam, fnam2))
										{
										resolved++;
										if(resolved == numvars) goto resolved_vzt;
										}
									}
								}

							}

						}
					}
				}
resolved_vzt:		free(pfx);
			ctx->varnames = varnames;
			ctx->resolved = resolved;
skip_resolved_vzt:
			varnames = ctx->varnames;
			resolved = ctx->resolved;

			jrb_traverse(node, varnames)
				{
				if(node->val.i >= 0)
					{
					char *rc = vzt_rd_value(vzt, anno_ctx->marker, node->val.i);
					struct jrb_chain *jvc = node->jval_chain;
					char first_char = rc ? rc[0] : '?';

					if(!jvc)
						{
						if(rc)
							{
							node->val2.v = hexify(strdup(rc));
							}
							else
							{
							node->val2.v = NULL;
							}
						}
						else
						{
						char *rc2;
						int len = rc ? strlen(rc) : 0;
						int iter = 1;

						while(jvc)
							{
							rc = vzt_rd_value(vzt, anno_ctx->marker, jvc->val.i);
							len+=(rc ? strlen(rc) : 0);
							iter++;
							jvc = jvc->next;
							}

						if(iter==len)
							{
							int pos = 1;
							jvc = node->jval_chain;
							rc2 = calloc(1, len+1);
							rc2[0] = first_char;

							while(jvc)
								{
								char *rcv = vzt_rd_value(vzt, anno_ctx->marker, jvc->val.i);
								rc2[pos++] = *rcv;
								jvc = jvc->next;
								}

							node->val2.v = hexify(strdup(rc2));
							free(rc2);
							}
							else
							{
							node->val2.v = NULL;
							}
						}
					}
					else
					{
					node->val2.v = NULL;
					}
				}
			}
/******/
		else if(lx2)
			{
			int numfacs=lxt2_rd_get_num_facs(lx2);
			int i;
			int tlen;
			char *pfx = NULL;

			if(ctx->varnames) goto skip_resolved_lxt2;

			pfx = malloc((tlen=strlen(title))+1+1);
			strcpy(pfx, title);
			strcat(pfx+tlen, ".");
			tlen++;

			lxt2_rd_clr_fac_process_mask_all(lx2);
			jrb_traverse(node, varnames) { node->val.i = -1; }

			for(i=0;i<numfacs;i++)
				{
				char *fnam = lxt2_rd_get_facname(lx2, i);

				if(!strncmp(fnam, pfx, tlen))
					{
					if(!strchr(fnam+tlen, '.'))
						{
						jrb_traverse(node, varnames)
							{
							int mat = 0;

							if(node->val.i < 0)
								{
								if(!strcmp(fnam+tlen, node->key.s))
									{
									mat = 1;

									if(lx2->flags[i] & LXT2_RD_SYM_F_ALIAS)
										{
										node->val.i = lxt2_rd_get_alias_root(lx2, i);
										}
										else
										{
										node->val.i = i;
										}

									lxt2_rd_set_fac_process_mask(lx2, node->val.i);
									}
								}
								else /* bitblasted */
								{
                                                                if(!strcmp(fnam+tlen, node->key.s))
                                                                        {
                                                                        struct jrb_chain *jvc = node->jval_chain;

                                                                        mat = 1;

                                                                        if(jvc) {
                                                                                while(jvc->next) jvc = jvc->next;
                                                                                jvc->next = calloc(1, sizeof(struct jrb_chain));
                                                                                jvc = jvc->next;
                                                                                }
                                                                                else
                                                                                {
                                                                                jvc = calloc(1, sizeof(struct jrb_chain));
                                                                                node->jval_chain = jvc;
                                                                                }

									if(lx2->flags[i] & LXT2_RD_SYM_F_ALIAS)
										{
										jvc->val.i = lxt2_rd_get_alias_root(lx2, i);
                                                                                }
                                                                                else
                                                                                {
                                                                                jvc->val.i = i;
                                                                                }

									lxt2_rd_set_fac_process_mask(lx2, jvc->val.i);
                                                                        }
								}

                                                       if(mat)
                                                                {
                                                                if(i==(numfacs-1))
                                                                        {
                                                                        resolved++;
                                                                        }
                                                                        else
                                                                        {
                                                                        char *fnam2 = lxt2_rd_get_facname(lx2, i+1);
                                                                        if(strcmp(fnam, fnam2))
                                                                                {
                                                                                resolved++;
                                                                                if(resolved == numvars) goto resolved_lxt2;
                                                                                }
                                                                        }
								}
							}
						}
					}
				}
resolved_lxt2:		free(pfx);
			ctx->varnames = varnames;
			ctx->resolved = resolved;
skip_resolved_lxt2:
			varnames = ctx->varnames;
			resolved = ctx->resolved;

			lx2vals = make_jrb();
			lxt2_rd_unlimit_time_range(lx2);
			lxt2_rd_limit_time_range(lx2, anno_ctx->marker, anno_ctx->marker);
			lxt2_rd_iter_blocks(lx2, lx2_iter_fn, NULL);

			jrb_traverse(node, varnames)
				{
				struct jrb_chain *jvc = node->jval_chain;

				if(node->val.i >= 0)
					{
					JRB srch = jrb_find_int(lx2vals, node->val.i);
					char *rc = srch ? srch->val.s : NULL;
					char first_char = rc ? rc[0] : '?';

					if(!jvc)
						{
						if(rc)
							{
							node->val2.v = hexify(strdup(rc));
							}
							else
							{
							node->val2.v = NULL;
							}
						}
						else
						{
                                                char *rc2;
                                                int len = rc ? strlen(rc) : 0;
                                                int iter = 1;

                                                while(jvc)
                                                        {
							srch = jrb_find_int(lx2vals, jvc->val.i);
							rc = srch ? srch->val.s : NULL;
                                                        len+=(rc ? strlen(rc) : 0);
                                                        iter++;
                                                        jvc = jvc->next;
                                                        }

                                                if(iter==len)
                                                        {
                                                        int pos = 1;
                                                        jvc = node->jval_chain;
                                                        rc2 = calloc(1, len+1);
                                                        rc2[0] = first_char;

                                                        while(jvc)
                                                                {
								srch = jrb_find_int(lx2vals, jvc->val.i);
								rc = srch->val.s;
                                                                rc2[pos++] = *rc;
                                                                jvc = jvc->next;
                                                                }

                                                        node->val2.v = hexify(strdup(rc2));
                                                        free(rc2);
                                                        }
                                                        else
                                                        {
                                                        node->val2.v = NULL;
							}
						}
					}
					else
					{
					node->val2.v = NULL;
					}
				}

			jrb_traverse(node, lx2vals)
				{
				if(node->val.s) free(node->val.s);
				}
			jrb_free_tree(lx2vals);
			lx2vals = NULL;
			}
/******/
#ifdef AET2_IS_PRESENT
		else if(ae2)
			{
			int numfacs=ae2_read_num_symbols(ae2);
			int i;
			int tlen;
			char *pfx = NULL;
			char *tstr;
			int attempt = 0;

			if(ctx->varnames) goto skip_resolved_ae2;

			pfx = malloc((tlen=strlen(title))+1+1);
			strcpy(pfx, title);
			strcat(pfx+tlen, ".");
			tlen++;

			jrb_traverse(node, varnames) { node->val.i = -1; }

retry_ae2:		for(i=0;i<numfacs;i++)
				{
				char bf[65537];
				char *fnam = bf;

				ae2_read_symbol_name(ae2, i, bf);

				if(!strncmp(fnam, pfx, tlen))
					{
					if(!strchr(fnam+tlen, '.'))
						{
						jrb_traverse(node, varnames)
							{
							if(node->val.i < 0)
								{
								/* note, ae2 is never bitblasted */
								if(!strcmp(fnam+tlen, node->key.s))
									{
									node->val.i = i;
									resolved++;
									if(resolved == numvars) goto resolved_ae2;
									}
								}
							}

						}
					}
				}

			free(pfx);

			/* prune off one level of hierarchy... */
			tstr = strchr(title, '.');
			if(tstr)
				{
				if((!attempt)&&(!resolved))
					{
					pfx = malloc((tlen=strlen(tstr+1))+1+1);
					strcpy(pfx, tstr+1);
					strcat(pfx+tlen, ".");
					tlen++;
					attempt++;
					goto retry_ae2;
					}
					else
					{
					pfx = malloc(1); /* dummy */
					}
				}
				else	/* try name as top level sig */
				{
				pfx = malloc(1); /* dummy */

				if(!resolved)
				for(i=0;i<numfacs;i++)
					{
					char bf[65537];
					char *fnam = bf;

					ae2_read_symbol_name(ae2, i, bf);
					jrb_traverse(node, varnames)
						{
						if(node->val.i < 0)
							{
							if(!strcmp(fnam, node->key.s))
								{
								node->val.i = i;
								resolved++;
								if(resolved == numvars) goto resolved_ae2;
								}
							}
						}
					}
				}

resolved_ae2:		free(pfx);
			ctx->varnames = varnames;
			ctx->resolved = resolved;
skip_resolved_ae2:
			varnames = ctx->varnames;
			resolved = ctx->resolved;

			jrb_traverse(node, varnames)
				{
				if(node->val.i >= 0)
					{
					char rc[AE2_MAXFACLEN+1];
					AE2_FACREF fr;

					fr.s = node->val.i;
					fr.row = 0;
					fr.row_high = 0;
					fr.offset = 0;
					fr.length = ae2_read_symbol_length(ae2, fr.s);
					ae2_read_value(ae2, &fr, anno_ctx->marker, rc);

					node->val2.v = rc[0] ? hexify(strdup(rc)) : NULL;
					}
					else
					{
					node->val2.v = NULL;
					}
				}
			}
#endif
/******/

		if(resolved > 0)
			{
			w = wlog_head;
			while(w)
				{
				if((w->line_no >= s_line) && (w->line_no <= e_line))
					{
					if(w->tok == V_ID)
						{
						if(strcmp(w->text, design_unit)) /* filter out design unit name */
							{
							node = jrb_find_str(varnames, w->text);
							if((node)&&(node->val2.v))
								{
								log_text(text, fontx, w->text);
								log_text_bold(text, fontx, "[");
								log_text_bold(text, fontx, node->val2.v);
								log_text_bold(text, fontx, "]");
								goto iter_free;
								}
							}
						}

					switch(w->tok)
						{
						case V_CMT:	log_text_active(text, fontx, w->text); break;

						case V_STRING:
						case V_PREPROC:
						case V_PREPROC_WS:
						case V_MACRO:	log_text_prelight(text, fontx, w->text); break;

						default:	log_text(text, fontx, w->text); break;
						}
					}

iter_free: 			free(w->text);
				wt = w;
				w = w->next;
				free(wt);
				}
			free(pnt);
			/* wlog_head = */ wlog_curr = NULL;
			goto free_vars;
			}

		}


	w = wlog_head;
	while(w)
		{
		if((display_mode)||((w->line_no >= s_line) && (w->line_no <= e_line)))
			{
			int len = strlen(w->text);
			memcpy(pnt2, w->text, len);
			pnt2 += len;
			}

		free(w->text);
		wt = w;
		w = w->next;
		free(wt);
		}
	/* wlog_head = */ wlog_curr = NULL;
	*pnt2 = 0;
	log_text(text, fontx, pnt);
	free(pnt);

free_vars:
#if 0
	/* free up variables list */
	if(!display_mode)
		{
		jrb_traverse(node, varnames)
			{
			if(node->val2.v) free(node->val2.v);
			free(node->key.s);
			}
		}

	jrb_traverse(node, varnames)
		{
		struct jrb_chain *jvc = node->jval_chain;
		while(jvc)
			{
			struct jrb_chain *jvcn = jvc->next;
			free(jvc);
			jvc = jvcn;
			}
		}

	jrb_free_tree(varnames);
	varnames = NULL;
#endif

	/* insert context for destroy */
	if(window)
		{
	        text_curr = (struct text_find_t *)calloc(1, sizeof(struct text_find_t));
		text_curr->window = window;
		text_curr->button = button;
		text_curr->text = text;
		text_curr->ctx = ctx;
		text_curr->next = text_root;
		text_root = text_curr;

		text_curr->bold_tag = bold_tag;
		text_curr->dgray_tag = dgray_tag;
		text_curr->lgray_tag = lgray_tag;
		text_curr->blue_tag = blue_tag;
		text_curr->fwht_tag = fwht_tag;
		text_curr->mono_tag = mono_tag;
		text_curr->size_tag = size_tag;
		}
	}
}

