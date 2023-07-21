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
#include "gtk23compat.h"
#include "currenttime.h"


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
  GdkScreen *fonts_screen = gdk_screen_get_default();

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

/***/

gint font_engine_string_measure
                        (struct font_engine_font_t      *font,
                         const gchar                    *string)
{
gint rc = 1; /* dummy value */

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

return(rc);
}


void load_all_fonts(void)
{
if(GLOBALS->use_pango_fonts)
	{
	pango_load_all_fonts();
	}
	else
	{
	printf("GDK X11 fonts are no longer supported, exiting.\n");
	exit(255);
	}
}



void XXX_font_engine_draw_string
                        (cairo_t                        *cr,
                         struct font_engine_font_t      *font,
                         wave_rgb_t                     *gc,
                         gint                           x,
                         gint                           y,
                         const gchar                    *string)
{
	/* PangoRectangle ink,logical; */

	pango_layout_set_text(GLOBALS->fonts_layout, string, -1);
	pango_layout_set_font_description(GLOBALS->fonts_layout, font->desc);
	/* pango_layout_get_extents(GLOBALS->fonts_layout,&ink,&logical); */

	cairo_set_source_rgba (cr, gc->r, gc->g, gc->b, gc->a);
	cairo_move_to (cr, x, y-font->ascent);
	pango_cairo_show_layout (cr, GLOBALS->fonts_layout);
}
