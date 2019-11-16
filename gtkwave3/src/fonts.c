/*
 * Copyright (c) Tony Bybell 1999-2008
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "gtk12compat.h"
#include "currenttime.h"

#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)

#ifdef MAC_INTEGRATION
#define WAVE_MONOSPACE_12 "Monaco 16"
#define WAVE_MONOSPACE_10 "Monaco 14"
#define WAVE_MONOSPACE_8  "Monaco 12"
#define WAVE_MONOSPACE_6  "Monaco 10"
#define WAVE_SANS_12      "Sans 22"
#define WAVE_SANS_10      "Sans 16"
#else
#define WAVE_MONOSPACE_12 "Monospace 12"
#define WAVE_MONOSPACE_10 "Monospace 10"
#define WAVE_MONOSPACE_8  "Monospace 8"
#define WAVE_MONOSPACE_6  "Monospace 6"
#define WAVE_SANS_12      "Sans 12"
#define WAVE_SANS_10      "Sans 10"
#endif

static struct font_engine_font_t *do_font_load(const char *name)
{
struct font_engine_font_t *fef = NULL;
PangoFontDescription *desc;

if( (name) && (desc = pango_font_description_from_string(name)) )
	{
	fef = calloc_2(1, sizeof(struct font_engine_font_t));

	fef->desc = desc;
	fef->font = pango_font_map_load_font( pango_cairo_font_map_get_default(), GLOBALS->fonts_context,   fef->desc);
	fef->metrics=pango_font_get_metrics(fef->font, NULL /*pango_language_get_default()*/ );

	fef->ascent  = pango_font_metrics_get_ascent(fef->metrics) / 1000;
	fef->descent = pango_font_metrics_get_descent(fef->metrics) / 1000;

	fef->is_pango = 1;

	if(!strncmp(name, "Monospace", 9))
		{
		int i_width = font_engine_string_measure(fef, "i");
		fef->mono_width = font_engine_string_measure(fef, "O");
		fef->is_mono = (i_width == fef->mono_width);
		}
	}

return(fef);
}

static int setup_fonts(void)
{
  int width, height;
  GdkDrawable *drawable=GDK_DRAWABLE(gtk_widget_get_root_window(GTK_WIDGET(GLOBALS->mainwindow)));
  GdkScreen *fonts_screen = gdk_drawable_get_screen (drawable);
  GLOBALS->fonts_renderer = gdk_pango_renderer_get_default (fonts_screen);
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (GLOBALS->fonts_renderer), drawable);

  GLOBALS->fonts_gc = gdk_gc_new (drawable);
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (GLOBALS->fonts_renderer), GLOBALS->fonts_gc);

  gdk_drawable_get_size (drawable, &width, &height);

  GLOBALS->fonts_context = gdk_pango_context_get_for_screen (fonts_screen);
  GLOBALS->fonts_layout = pango_layout_new (GLOBALS->fonts_context);

  return 0;
}


static int my_font_height(struct font_engine_font_t *f)
{
return(f->ascent + f->descent);
}


static void pango_load_all_fonts(void)
{
  setup_fonts();
  GLOBALS->signalfont=do_font_load(GLOBALS->fontname_signals);

  if(!GLOBALS->signalfont)
    {
      if(GLOBALS->use_big_fonts)
	{
	  GLOBALS->signalfont=do_font_load(GLOBALS->use_nonprop_fonts ? WAVE_MONOSPACE_12 : WAVE_SANS_12);
	}
      else
	{
	  GLOBALS->signalfont=do_font_load(GLOBALS->use_nonprop_fonts ? WAVE_MONOSPACE_10 : WAVE_SANS_10);
	}
    }

  GLOBALS->fontheight= my_font_height(GLOBALS->signalfont)+4;

  GLOBALS->wavefont=GLOBALS->wavefont_smaller=do_font_load(GLOBALS->fontname_waves);
  if(!GLOBALS->wavefont)
    {
      if(GLOBALS->use_big_fonts)
	{
	  GLOBALS->wavefont=do_font_load(WAVE_MONOSPACE_12);
	  GLOBALS->wavefont_smaller=do_font_load(WAVE_MONOSPACE_10);
	}
      else
	{
	  GLOBALS->wavefont=do_font_load(WAVE_MONOSPACE_8);
	  GLOBALS->wavefont_smaller=do_font_load(WAVE_MONOSPACE_6);
	}
    }

  if( my_font_height(GLOBALS->signalfont) < my_font_height(GLOBALS->wavefont))
    {
      fprintf(stderr, "Signalfont is smaller than wavefont (%d vs %d).  Exiting!\n", my_font_height(GLOBALS->signalfont), my_font_height(GLOBALS->wavefont));
      exit(1);
    }

  if(my_font_height(GLOBALS->signalfont)>100)
    {
      fprintf(stderr, "Fonts are too big!  Try fonts with a smaller size.  Exiting!\n");
      exit(1);
    }

  GLOBALS->wavecrosspiece=GLOBALS->wavefont->ascent+1;
}
#endif

/***/

static struct font_engine_font_t *font_engine_gdk_font_load(const char *string)
{
GdkFont *f = gdk_font_load(string);

if(f)
	{
	struct font_engine_font_t *fef = calloc_2(1, sizeof(struct font_engine_font_t));
	fef->gdkfont = f;
	fef->ascent = f->ascent;
	fef->descent = f->descent;
	return(fef);
	}
	else
	{
	return(NULL);
	}
}

void font_engine_draw_string
                        (GdkDrawable                    *drawable,
                         struct font_engine_font_t      *font,
                         GdkGC                          *gc,
                         gint                           x,
                         gint                           y,
                         const gchar                    *string)
{
if(!font->is_pango)
	{
	gdk_draw_string(drawable, font->gdkfont, gc, x, y, string);
	}
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)
	else
	{
	/* PangoRectangle ink,logical; */

	pango_layout_set_text(GLOBALS->fonts_layout, string, -1);
	pango_layout_set_font_description(GLOBALS->fonts_layout, font->desc);
	/* pango_layout_get_extents(GLOBALS->fonts_layout,&ink,&logical); */
	gdk_draw_layout(drawable, gc, x, y-font->ascent, GLOBALS->fonts_layout);
	}
#endif
}

gint font_engine_string_measure
                        (struct font_engine_font_t      *font,
                         const gchar                    *string)
{
gint rc = 1; /* dummy value */

if(!font->is_pango)
	{
	rc = gdk_string_measure(font->gdkfont, string);
	}
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)
	else
	{
	if(font->is_mono)
		{
		rc = strlen(string) * font->mono_width;
		}
		else
		{
		PangoRectangle ink,logical;

		pango_layout_set_text(GLOBALS->fonts_layout, string, -1);
		pango_layout_set_font_description(GLOBALS->fonts_layout, font->desc);
		pango_layout_get_extents(GLOBALS->fonts_layout,&ink,&logical);
		rc = logical.width/1000;
		}
	}
#endif

return(rc);
}


void gdk_load_all_fonts(void)
{
if((GLOBALS->fontname_signals)&&(strlen(GLOBALS->fontname_signals)))
	{
        GLOBALS->signalfont=font_engine_gdk_font_load(GLOBALS->fontname_signals);
	}
	else
	{
	if(GLOBALS->use_big_fonts)
	        {
	        if(!GLOBALS->use_nonprop_fonts)
	                {
	                GLOBALS->signalfont=font_engine_gdk_font_load("-*-times-*-r-*-*-15-*-*-*-*-*-*-*");
	                }
	                else
	                {
#ifdef __CYGWIN__
	                GLOBALS->signalfont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-15-*-*-*-*-*-*-*");
#else
	                GLOBALS->signalfont=font_engine_gdk_font_load("-*-courier-*-r-*-*-15-*-*-*-*-*-*-*");
			if(!GLOBALS->signalfont) GLOBALS->signalfont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-15-*-*-*-*-*-*-*");
#endif
	                }
	        }
	        else
	        {
	        if(GLOBALS->use_nonprop_fonts)
	                {
#ifdef __CYGWIN__
	                GLOBALS->signalfont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-14-*-*-*-*-*-*-*");
#else
	                GLOBALS->signalfont=font_engine_gdk_font_load("-*-courier-*-r-*-*-14-*-*-*-*-*-*-*");
			if(!GLOBALS->signalfont) GLOBALS->signalfont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-14-*-*-*-*-*-*-*");
#endif
	                }
	        }
	}

if(!GLOBALS->signalfont)
        {
#if WAVE_USE_GTK2
        GLOBALS->signalfont=font_engine_gdk_font_load("-*-courier-*-r-*-*-14-*-*-*-*-*-*-*");
	if(!GLOBALS->signalfont) GLOBALS->signalfont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-14-*-*-*-*-*-*-*");
	if(!GLOBALS->signalfont) { fprintf(stderr, "Could not load signalfont courier 14 or misc-fixed 14, exiting!\n"); exit(255); }
#else
	GLOBALS->signalfont= calloc_2(1, sizeof(struct font_engine_font_t));
	GLOBALS->signalfont->gdkfont = GLOBALS->wavearea->style->font;
	GLOBALS->signalfont->ascent = GLOBALS->wavearea->style->font->ascent;
	GLOBALS->signalfont->descent = GLOBALS->wavearea->style->font->descent;
#endif
        }

GLOBALS->fontheight=(GLOBALS->signalfont->ascent+GLOBALS->signalfont->descent)+4;

if((GLOBALS->fontname_waves)&&(strlen(GLOBALS->fontname_waves)))
	{
        GLOBALS->wavefont=GLOBALS->wavefont_smaller=font_engine_gdk_font_load(GLOBALS->fontname_waves);
	}
	else
	{
#ifndef __CYGWIN__
	if(GLOBALS->use_big_fonts)
	        {
	        GLOBALS->wavefont=font_engine_gdk_font_load("-*-courier-*-r-*-*-14-*-*-*-*-*-*-*");
		if(!GLOBALS->wavefont) GLOBALS->wavefont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-14-*-*-*-*-*-*-*");

	        GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-*-courier-*-r-*-*-10-*-*-*-*-*-*-*");
		if(!GLOBALS->wavefont_smaller) GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-10-*-*-*-*-*-*-*");
	        }
	        else
	        {
	        GLOBALS->wavefont=GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-*-courier-*-r-*-*-10-*-*-*-*-*-*-*");
		if(!GLOBALS->wavefont) GLOBALS->wavefont=GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-10-*-*-*-*-*-*-*");
	        }
#else
	if(GLOBALS->use_big_fonts)
	        {
                GLOBALS->wavefont=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-14-*-*-*-*-*-*-*");
                GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-10-*-*-*-*-*-*-*");
	        }
	        else
	        {
                GLOBALS->wavefont=GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-10-*-*-*-*-*-*-*");
	        }
#endif
	}

if(!GLOBALS->wavefont)
        {
#if WAVE_USE_GTK2
        GLOBALS->wavefont=GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-*-courier-*-r-*-*-10-*-*-*-*-*-*-*");
	if(!GLOBALS->wavefont) GLOBALS->wavefont=GLOBALS->wavefont_smaller=font_engine_gdk_font_load("-misc-fixed-*-*-*-*-10-*-*-*-*-*-*-*");
	if(!GLOBALS->wavefont) { fprintf(stderr, "Could not load wavefont courier 10 or misc-fixed 10, exiting!\n"); exit(255); }
#else
	GLOBALS->wavefont = calloc_2(1, sizeof(struct font_engine_font_t));
        GLOBALS->wavefont->gdkfont = GLOBALS->wavearea->style->font;
	GLOBALS->wavefont->ascent = GLOBALS->wavearea->style->font->ascent;
	GLOBALS->wavefont->descent = GLOBALS->wavearea->style->font->descent;

	GLOBALS->wavefont_smaller = calloc_2(1, sizeof(struct font_engine_font_t));
        GLOBALS->wavefont_smaller->gdkfont = GLOBALS->wavearea->style->font;
	GLOBALS->wavefont_smaller->ascent = GLOBALS->wavearea->style->font->ascent;
	GLOBALS->wavefont_smaller->descent = GLOBALS->wavearea->style->font->descent;
#endif
        }


if(GLOBALS->signalfont->ascent<GLOBALS->wavefont->ascent)
	{
	fprintf(stderr, "Signalfont is smaller than wavefont.  Exiting!\n");
	exit(1);
	}

if(GLOBALS->signalfont->ascent>100)
	{
	fprintf(stderr, "Fonts are too big!  Try fonts with a smaller size.  Exiting!\n");
	exit(1);
	}

GLOBALS->wavecrosspiece=GLOBALS->wavefont->ascent+1;
}


void load_all_fonts(void)
{
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)
if(GLOBALS->use_pango_fonts)
	{
	pango_load_all_fonts();
	}
	else
#endif
	{
	gdk_load_all_fonts();
	}
}


