/*
 * Copyright (c) Tony Bybell 2006-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* AIX may need this for alloca to work */
#if defined _AIX
  #pragma alloca
#endif

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "currenttime.h"
#include "color.h"
#include "bsearch.h"
#include "hierpack.h"

WAVE_NODEVARTYPE_STR

/************************************************************************************************/

static char *get_fullname(Trptr t)
{
char *s = NULL;
if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
	{
	if(t->vector==TRUE)
		{
		s = strdup_2(t->n.vec->bvname);
		}
		else
		{
		if(!HasAlias(t))
			{
			int flagged = HIER_DEPACK_ALLOC;
	                s = hier_decompress_flagged(t->n.nd->nname, &flagged);
			if(!flagged) s = strdup_2(s);
			}
		}
	}

return(s);
}


static int determine_trace_flags(Trptr t, char *ch)
{
TraceFlagsType flags = t->flags;
int pos = 0;

/* [0] */
if((flags & TR_SIGNED) != 0)
	{
	ch[pos++] = '+';
	}

/* [1] */
if((flags & TR_HEX) != 0) { ch[pos++] = 'X'; }
else if ((flags & TR_ASCII) != 0) { ch[pos++] = 'A'; }
else if ((flags & TR_DEC) != 0) { ch[pos++] = 'D'; }
else if ((flags & TR_BIN) != 0) { ch[pos++] = 'B'; }
else if ((flags & TR_OCT) != 0) { ch[pos++] = 'O'; }

/* [2] */
if((flags & TR_RJUSTIFY) != 0) { ch[pos++] = 'J'; }

/* [3] */
if((flags & TR_INVERT) != 0) { ch[pos++] = '~'; }

/* [4] */
if((flags & TR_REVERSE) != 0) { ch[pos++] = 'V'; }

/* [5] */
if((flags & (TR_ANALOG_STEP|TR_ANALOG_INTERPOLATED)) == (TR_ANALOG_STEP|TR_ANALOG_INTERPOLATED)) { ch[pos++] = '*'; }
else if((flags & TR_ANALOG_STEP) != 0) { ch[pos++] = 'S'; }
else if((flags & TR_ANALOG_INTERPOLATED) != 0) { ch[pos++] = 'I'; }

/* [6] */
if((flags & TR_REAL) != 0) { ch[pos++] = 'R'; }

/* [7] */
if((flags & TR_REAL2BITS) != 0) { ch[pos++] = 'r'; }

/* [8] */
if((flags & TR_ZEROFILL) != 0) { ch[pos++] = '0'; }
else
if((flags & TR_ONEFILL) != 0) { ch[pos++] = '1'; }

/* [9] */
if((flags & TR_BINGRAY) != 0) { ch[pos++] = 'G'; }
/* [10] */
if((flags & TR_GRAYBIN) != 0) { ch[pos++] = 'g'; }

/* [11] */
if((flags & TR_FTRANSLATED) != 0) { ch[pos++] = 'F'; }

/* [12] */
if((flags & TR_PTRANSLATED) != 0) { ch[pos++] = 'P'; }

/* [13] */
if((flags & TR_TTRANSLATED) != 0) { ch[pos++] = 'T'; }

/* [14] */
if((flags & TR_FFO) != 0) { ch[pos++] = 'f'; }

/* [15] */
if((flags & TR_POPCNT) != 0) { ch[pos++] = 'p'; }

/* [16] */
if((flags & TR_FPDECSHIFT) != 0) 
	{ 
	int ln = sprintf(ch+pos, "s(%d)", t->t_fpdecshift);
	pos += ln;
	}

/* [17+] */
ch[pos] = 0;

if(!t->vector)
	{
	int vartype = t->n.nd->vartype;
	if((vartype < 0) || (vartype > ND_VARTYPE_MAX))
		{
		vartype = 0;
		}

	if(vartype)
		{
		ch[pos++] = ':';
		strcpy(ch + pos, vartype_strings[vartype]);
		pos += strlen(vartype_strings[vartype]);
		ch[++pos] = 0;
		}
	}

return(pos);
}

/************************************************************************************************/

static void local_trace_asciival(Trptr t, char *tname, TimeType tim, int *nmaxlen, int *vmaxlen, char **asciivalue)
{
int len=0;
int vlen=0;

if(tname)
	{
	len=font_engine_string_measure(GLOBALS->wavefont, tname);

	if((tim!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
                GLOBALS->shift_timebase=t->shift;

		if(t->vector)
			{
			char *str;
			vptr v;

                        v=bsearch_vector(t->n.vec,tim - t->shift);
                        str=convert_ascii(t,v);
			if(str)
				{
				vlen=font_engine_string_measure(GLOBALS->wavefont,str);
				*asciivalue=str;
				}
				else
				{
				vlen=0;
				*asciivalue=NULL;
				}

			}
			else
			{
			char *str;
			hptr h_ptr;
			if((h_ptr=bsearch_node(t->n.nd,tim - t->shift)))
				{
				if(!t->n.nd->extvals)
					{
					unsigned char h_val = h_ptr->v.h_val;
					if(t->n.nd->vartype == ND_VCD_EVENT)
						{
						h_val = (h_ptr->time >= GLOBALS->tims.first) && ((GLOBALS->tims.marker-GLOBALS->shift_timebase) == h_ptr->time) ? AN_1 : AN_0; /* generate impulse */
						}

					str=(char *)calloc_2(1,2*sizeof(char));
					if(t->flags&TR_INVERT)
						{
						str[0]=AN_STR_INV[h_val];
						}
						else
						{
						str[0]=AN_STR[h_val];
						}
					*asciivalue=str;
					vlen=font_engine_string_measure(GLOBALS->wavefont,str);
					}
					else
					{
					if(h_ptr->flags&HIST_REAL)
						{
						if(!(h_ptr->flags&HIST_STRING))
							{
#ifdef WAVE_HAS_H_DOUBLE
							str=convert_ascii_real(t, &h_ptr->v.h_double);
#else
							str=convert_ascii_real(t, (double *)h_ptr->v.h_vector);
#endif
							}
							else
							{
							str=convert_ascii_string((char *)h_ptr->v.h_vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,h_ptr->v.h_vector);
						}

					if(str)
						{
						vlen=font_engine_string_measure(GLOBALS->wavefont,str);
						*asciivalue=str;
						}
						else
						{
						vlen=0;
						*asciivalue=NULL;
						}
					}
				}
				else
				{
				vlen=0;
				*asciivalue=NULL;
				}
			}
		}
	}

*nmaxlen = len;
*vmaxlen = vlen;
}

/************************************************************************************************/

#if GTK_CHECK_VERSION(3,0,0)
static gint draw_event(GtkWidget *widget, cairo_t *cr, gpointer      user_data)
{
(void) widget;
(void) user_data;

cairo_set_source_surface(cr, GLOBALS->surface_mo_pixmap_mouseover_c_1, 0, 0);
cairo_paint (cr);

return(FALSE);
}
#else
static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(widget)), dummy);
gdk_cairo_region (cr, event->region);
cairo_clip (cr);

cairo_set_source_surface(cr, GLOBALS->surface_mo_pixmap_mouseover_c_1, 0, 0);
cairo_paint (cr);
cairo_destroy (cr);

return(FALSE);
}
#endif


static void create_mouseover_sigs(gint x, gint y, gint width, gint height)
{
GLOBALS->mo_width_mouseover_c_1 = width;
GLOBALS->mo_height_mouseover_c_1 = height;

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()))
        {
        GLOBALS->mo_area_mouseover_c_1=gtk_drawing_area_new();
        gtk_widget_show(GLOBALS->mo_area_mouseover_c_1);
        GLOBALS->mouseover_mouseover_c_1 = gtk_window_new (GTK_WINDOW_POPUP);
        gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW(GLOBALS->mainwindow)), GTK_WINDOW (GLOBALS->mouseover_mouseover_c_1));
        gtk_window_set_transient_for (GTK_WINDOW (GLOBALS->mouseover_mouseover_c_1), GTK_WINDOW(GLOBALS->mainwindow));
        gtk_container_add(GTK_CONTAINER(GLOBALS->mouseover_mouseover_c_1), GLOBALS->mo_area_mouseover_c_1);
        gtk_widget_realize (GLOBALS->mouseover_mouseover_c_1);
        gtk_window_set_type_hint (GTK_WINDOW (GLOBALS->mouseover_mouseover_c_1), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_move (GTK_WINDOW (GLOBALS->mouseover_mouseover_c_1), x, y);
        gtk_window_resize (GTK_WINDOW (GLOBALS->mouseover_mouseover_c_1), width, height);
        gtk_widget_show (GTK_WIDGET (GLOBALS->mouseover_mouseover_c_1));
        }
        else
#endif
      	{
	GLOBALS->mouseover_mouseover_c_1 = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_window_set_default_size(GTK_WINDOW (GLOBALS->mouseover_mouseover_c_1), width, height);
        gtk_window_set_type_hint(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
        gtk_window_move(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1), x, y);
        GLOBALS->mo_area_mouseover_c_1=gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(GLOBALS->mouseover_mouseover_c_1), GLOBALS->mo_area_mouseover_c_1);
        gtk_widget_show(GLOBALS->mo_area_mouseover_c_1);
        gtk_widget_show(GLOBALS->mouseover_mouseover_c_1);
        }

GtkAllocation allocation;
gtk_widget_get_allocation(GLOBALS->mouseover_mouseover_c_1, &allocation);

GLOBALS->surface_mo_pixmap_mouseover_c_1 = cairo_image_surface_create (CAIRO_FORMAT_RGB24, allocation.width, allocation.height);
GLOBALS->cr_mo_pixmap_mouseover_c_1 = cairo_create (GLOBALS->surface_mo_pixmap_mouseover_c_1);
cairo_set_line_width(GLOBALS->cr_mo_pixmap_mouseover_c_1, 1.0);

GLOBALS->rgb_mo_dk_gray_mouseover_c_1 = XXX_alloc_color(0x00cccccc);
GLOBALS->rgb_mo_black_mouseover_c_1   = XXX_alloc_color(0x00000000);

XXX_gdk_draw_rectangle(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->rgb_mo_dk_gray_mouseover_c_1,
                TRUE,
                0,0,
                GLOBALS->mo_width_mouseover_c_1, GLOBALS->mo_height_mouseover_c_1);

XXX_gdk_draw_rectangle(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->rgb_mo_dk_gray_mouseover_c_1,
                TRUE,
                1,1,
                GLOBALS->mo_width_mouseover_c_1-2, GLOBALS->mo_height_mouseover_c_1-2);

#if GTK_CHECK_VERSION(3,0,0)
g_signal_connect(XXX_GTK_OBJECT(GLOBALS->mo_area_mouseover_c_1), "draw",G_CALLBACK(draw_event), NULL);
#else
g_signal_connect(XXX_GTK_OBJECT(GLOBALS->mo_area_mouseover_c_1), "expose_event",G_CALLBACK(expose_event), NULL);
#endif
}

#define MOUSEOVER_BREAKSIZE (100)

void move_mouseover_sigs(Trptr t, gint xin, gint yin, TimeType tim)
{
gint xd = 0, yd = 0;
char *asciivalue = NULL;
int nmaxlen = 0, vmaxlen = 0;
int totalmax = 0;
int name_charlen = 0, value_charlen = 0;
int num_info_rows = 2;
char *flagged_name = NULL;
char *alternate_name = NULL;
int fh;
char flag_string[65];
char *tname = NULL;

if(GLOBALS->disable_mouseover)
	{
	if(GLOBALS->mouseover_mouseover_c_1)
		{
		gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1); GLOBALS->mouseover_mouseover_c_1 = NULL;

                if(GLOBALS->cr_mo_pixmap_mouseover_c_1) { cairo_destroy(GLOBALS->cr_mo_pixmap_mouseover_c_1); GLOBALS->cr_mo_pixmap_mouseover_c_1 = NULL; }
                if(GLOBALS->surface_mo_pixmap_mouseover_c_1) { cairo_surface_destroy(GLOBALS->surface_mo_pixmap_mouseover_c_1); GLOBALS->surface_mo_pixmap_mouseover_c_1 = NULL; }
		}
	goto bot;
	}

fh = GLOBALS->wavefont->ascent+GLOBALS->wavefont->descent;

if(t && (tname = get_fullname(t)))
	{
	local_trace_asciival(t, tname, tim, &nmaxlen, &vmaxlen, &asciivalue);

	value_charlen = asciivalue ? strlen(asciivalue) : 0;

	if(GLOBALS->clipboard_mouseover)
		{
                GdkDisplay *g = gdk_display_get_default();
                GtkClipboard *clip;

                if(t->name)
                        {
                        clip = gtk_clipboard_get_for_display (g, GDK_SELECTION_PRIMARY); /* middle mouse button	*/
                        gtk_clipboard_set_text (clip, t->name, strlen(t->name));
                        }

                clip = gtk_clipboard_get_for_display (g, GDK_SELECTION_CLIPBOARD); /* ctrl-c/ctrl-v */
                gtk_clipboard_set_text (clip, asciivalue ? asciivalue : "", value_charlen);
		}

	name_charlen = tname ? strlen(tname) : 0;
	if(name_charlen)
		{
		int len = determine_trace_flags(t, flag_string);
		flagged_name = malloc_2(name_charlen + 1 + len + 1);
		memcpy(flagged_name, tname, name_charlen);
		flagged_name[name_charlen] = ' ';
		strcpy(flagged_name+name_charlen+1, flag_string);
		name_charlen += (len + 1);
		}

	if(name_charlen > MOUSEOVER_BREAKSIZE)
		{
		alternate_name = malloc_2(MOUSEOVER_BREAKSIZE + 1);
		strcpy(alternate_name, "...");
		strcpy(alternate_name + 3, flagged_name + name_charlen - (MOUSEOVER_BREAKSIZE - 3));

		nmaxlen=font_engine_string_measure(GLOBALS->wavefont, alternate_name);
		}
		else
		{
		nmaxlen=font_engine_string_measure(GLOBALS->wavefont, flagged_name);
		}

	if(value_charlen > MOUSEOVER_BREAKSIZE)
		{
		char breakbuf[MOUSEOVER_BREAKSIZE+1];
		int i, localmax;

		num_info_rows = (value_charlen + (MOUSEOVER_BREAKSIZE-1)) / MOUSEOVER_BREAKSIZE;
		vmaxlen = 0;

		for(i=0;i<num_info_rows;i++)
			{
			memset(breakbuf, 0, MOUSEOVER_BREAKSIZE+1);
			strncpy(breakbuf, asciivalue + (i*MOUSEOVER_BREAKSIZE), MOUSEOVER_BREAKSIZE);
			localmax = font_engine_string_measure(GLOBALS->wavefont, breakbuf);
			vmaxlen = (localmax > vmaxlen) ? localmax : vmaxlen;
			}

		num_info_rows++;
		}

	totalmax = (nmaxlen > vmaxlen) ? nmaxlen : vmaxlen;
	totalmax += 8;
	totalmax = (totalmax + 1) & ~1; /* round up to next even pixel count */

	if((GLOBALS->mouseover_mouseover_c_1)&&((totalmax != GLOBALS->mo_width_mouseover_c_1)||((num_info_rows * fh + 7) != GLOBALS->mo_height_mouseover_c_1)))
		{
		gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1); GLOBALS->mouseover_mouseover_c_1 = NULL;

                if(GLOBALS->cr_mo_pixmap_mouseover_c_1) { cairo_destroy(GLOBALS->cr_mo_pixmap_mouseover_c_1); GLOBALS->cr_mo_pixmap_mouseover_c_1 = NULL; }
                if(GLOBALS->surface_mo_pixmap_mouseover_c_1) { cairo_surface_destroy(GLOBALS->surface_mo_pixmap_mouseover_c_1); GLOBALS->surface_mo_pixmap_mouseover_c_1 = NULL; }
		}
	}

if((!t)||(!tname)||(yin<0)||(yin>GLOBALS->waveheight))
	{
	if(GLOBALS->mouseover_mouseover_c_1)
		{
		gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1); GLOBALS->mouseover_mouseover_c_1 = NULL;

                if(GLOBALS->cr_mo_pixmap_mouseover_c_1) { cairo_destroy(GLOBALS->cr_mo_pixmap_mouseover_c_1); GLOBALS->cr_mo_pixmap_mouseover_c_1 = NULL; }
                if(GLOBALS->surface_mo_pixmap_mouseover_c_1) { cairo_surface_destroy(GLOBALS->surface_mo_pixmap_mouseover_c_1); GLOBALS->surface_mo_pixmap_mouseover_c_1 = NULL; }
		}
	goto bot;
	}

if(!GLOBALS->mouseover_mouseover_c_1)
	{
	gdk_window_get_origin(gtk_widget_get_window(GLOBALS->signalarea), &xd, &yd);
	create_mouseover_sigs(xin + xd + 8, yin + yd + 20, totalmax, num_info_rows * fh + 7);
	}
	else
	{
	gdk_window_get_origin(gtk_widget_get_window(GLOBALS->signalarea), &xd, &yd);
        gtk_window_move(GTK_WINDOW(GLOBALS->mouseover_mouseover_c_1), xin + xd + 8, yin + yd + 20);
	}

if(!GLOBALS->cr_mo_pixmap_mouseover_c_1) goto bot; /* harden against NULL pointer */


XXX_gdk_draw_rectangle(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->rgb_mo_dk_gray_mouseover_c_1, TRUE,
                0,0,
                GLOBALS->mo_width_mouseover_c_1, GLOBALS->mo_height_mouseover_c_1);

XXX_gdk_draw_rectangle(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->rgb_mo_black_mouseover_c_1,
                TRUE,
                1,1,
                GLOBALS->mo_width_mouseover_c_1-2, GLOBALS->mo_height_mouseover_c_1-2);

XXX_font_engine_draw_string(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->wavefont, &GLOBALS->rgb_mo_dk_gray_mouseover_c_1, 4+0.5, fh + 2+0.5, alternate_name ? alternate_name : flagged_name);

if(num_info_rows == 2)
	{
	if(asciivalue) XXX_font_engine_draw_string(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->wavefont, &GLOBALS->rgb_mo_dk_gray_mouseover_c_1, 4+0.5, 2*fh+2+0.5, asciivalue);
	}
	else
	{
	char breakbuf[MOUSEOVER_BREAKSIZE+1];
	int i;

	num_info_rows--;
	for(i=0;i<num_info_rows;i++)
		{
		memset(breakbuf, 0, MOUSEOVER_BREAKSIZE+1);
		strncpy(breakbuf, asciivalue + (i*MOUSEOVER_BREAKSIZE), MOUSEOVER_BREAKSIZE);
		XXX_font_engine_draw_string(GLOBALS->cr_mo_pixmap_mouseover_c_1, GLOBALS->wavefont, &GLOBALS->rgb_mo_dk_gray_mouseover_c_1, 4+0.5, ((2+i)*fh)+2+0.5, breakbuf);
		}
	}

gdk_window_raise(gtk_widget_get_window(GLOBALS->mouseover_mouseover_c_1));

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
GdkDrawingContext *gdc;
#endif

cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->mo_area_mouseover_c_1)), &gdc);
cairo_set_source_surface(cr, GLOBALS->surface_mo_pixmap_mouseover_c_1, 0, 0);
cairo_paint (cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->mo_area_mouseover_c_1), gdc);
#else
cairo_destroy (cr);
#endif

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) gtk_widget_queue_draw(GLOBALS->mo_area_mouseover_c_1);
#endif

bot:
if(asciivalue) { free_2(asciivalue); }
if(alternate_name) { free_2(alternate_name); }
if(flagged_name) { free_2(flagged_name); }
if(tname) { free_2(tname); }
}

