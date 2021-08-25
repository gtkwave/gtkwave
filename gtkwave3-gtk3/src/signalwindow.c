/*
 * Copyright (c) Tony Bybell 1999-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "gtk23compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "debug.h"

/* GDK_KEY_equal defined from gtk2 2.22 onwards. */
#ifndef GDK_KEY_equal
#define GDK_KEY_equal GDK_equal
#endif
#ifndef GDK_KEY_Up
#define GDK_KEY_Up GDK_Up
#endif
#ifndef GDK_KEY_KP_Up
#define GDK_KEY_KP_Up GDK_KP_Up
#endif
#ifndef GDK_KEY_Down
#define GDK_KEY_Down GDK_Down
#endif
#ifndef GDK_KEY_KP_Down
#define GDK_KEY_KP_Down GDK_KP_Down
#endif

#undef FOCUS_DEBUG_MSGS

/*
 * complain about certain ops conflict with dnd...
 */
void dnd_error(void)
{
status_text("Can't perform that operation when waveform drag and drop is in progress!\n");
}

void signalwindow_paint(cairo_t *cr) {
	int scale_factor = XXX_gtk_widget_get_scale_factor(GLOBALS->signalarea);
	cairo_matrix_t prev_matrix;
	cairo_get_matrix(cr, &prev_matrix);
	cairo_scale(cr, 1.0/scale_factor, 1.0/scale_factor);
	cairo_set_source_surface(cr, GLOBALS->surface_signalpixmap, -(gint)(gtk_adjustment_get_value(GTK_ADJUSTMENT(GLOBALS->signal_hslider))), 0);
	cairo_paint (cr);
	cairo_set_matrix(cr, &prev_matrix);
}

static void
service_hslider(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

GtkAdjustment *hadj;
gint xsrc;

if(GLOBALS->cr_signalpixmap)
	{
	hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
	xsrc=(gint)gtk_adjustment_get_value(hadj);
	DEBUG(printf("Signal HSlider Moved to %d\n",xsrc));
	GLOBALS->right_align_active = 0;

	if(!GLOBALS->use_dark)
		{
	        cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc.gc_mdgray.r, GLOBALS->rgb_gc.gc_mdgray.g, GLOBALS->rgb_gc.gc_mdgray.b, GLOBALS->rgb_gc.gc_mdgray.a);
	        cairo_rectangle (GLOBALS->cr_signalpixmap, 0.5, -1+0.5, GLOBALS->signal_fill_width, GLOBALS->fontheight);
	        cairo_fill (GLOBALS->cr_signalpixmap);
		}

	cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_white.r, GLOBALS->rgb_gc_white.g, GLOBALS->rgb_gc_white.b, GLOBALS->rgb_gc_white.a);
	cairo_move_to (GLOBALS->cr_signalpixmap, 0.5, GLOBALS->fontheight-1+0.5);
	cairo_line_to (GLOBALS->cr_signalpixmap, GLOBALS->signal_fill_width-1+0.5, GLOBALS->fontheight-1+0.5);
	cairo_stroke (GLOBALS->cr_signalpixmap);

	if(!GLOBALS->use_dark)
		{
		XXX_font_engine_draw_string(GLOBALS->cr_signalpixmap, GLOBALS->signalfont, &(GLOBALS->rgb_gc_black),
	        	3+xsrc+0.5, GLOBALS->fontheight-4+0.5, "Time");
		}

	if(GLOBALS->signalarea_has_focus)
		{
                GtkAllocation allocation;
                gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
		GdkDrawingContext *gdc;
#endif
                cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
                cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
                cairo_clip (cr);

				signalwindow_paint(cr);

                draw_signalarea_focus(cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
		gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
		cairo_destroy (cr);
#endif
		}
		else
		{
                GtkAllocation allocation;
                gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
		GdkDrawingContext *gdc;
#endif
                cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
                cairo_rectangle (cr, 0.0, 0.0, allocation.width, allocation.height);
                cairo_clip (cr);

				signalwindow_paint(cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
                gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
                cairo_destroy (cr);
#endif
		}
	}
}


void draw_signalarea_focus(cairo_t *cr)
{
if(GLOBALS->signalarea_has_focus)
        {
        GtkAllocation allocation;
        gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

	if(cr)
		{
                cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
                cairo_clip (cr);

		cairo_set_source_rgba (cr, GLOBALS->rgb_gc_black.r, GLOBALS->rgb_gc_black.g, GLOBALS->rgb_gc_black.b, GLOBALS->rgb_gc_black.a);
		cairo_rectangle (cr, 0.5, 0.5, allocation.width-1, allocation.height-1);
		cairo_stroke(cr);
		}
	}
}

/**************************************************************************/
/***  standard click routines turned on with "use_standard_clicking"=1  ***/

/*
 *      DND "drag_begin" handler, this is called whenever a drag starts.
 */
static void DNDBeginCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

#ifdef GDK_WINDOWING_WAYLAND
if(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) 
	{
	GdkDevice *device = gdk_drag_context_get_device(dc);
	const gchar *gn = gdk_device_get_name(device);
	if(!strcmp(gn, "Wayland Touch Master Pointer"))
		{
		fprintf(stderr, "GTKWAVE | Touch DnD not yet supported on Wayland.\n");
		return;
		}
	}
#endif

GLOBALS->dnd_state = 1;
}

/*
 *      DND "drag_failed" handler, this is called when a drag and drop has
 *      failed (e.g., by pressing ESC).
 */
static gboolean DNDFailedCB(
	GtkWidget *widget, GdkDragContext *context, GtkDragResult result)
{
(void)widget;
(void)context;
(void)result;

GLOBALS->dnd_cursor_timer = 0;
GLOBALS->dnd_state = 0;
GLOBALS->standard_trace_dnd_degate = 1;

MaxSignalLength();
signalarea_configure_event(GLOBALS->signalarea, NULL);
wavearea_configure_event(GLOBALS->wavearea, NULL);

return(FALSE);
}

/*
 *      DND "drag_end" handler, this is called when a drag and drop has
 *      completed. So this function is the last one to be called in
 *      any given DND operation.
 */
static void DNDEndCB(
        GtkWidget *widget, GdkDragContext *dc, gpointer data
)
{
(void)widget;
(void)dc;
(void)data;

GtkWidget *ddest;
int which;
gdouble x,y;
GdkModifierType state;
Trptr t;
int trwhich, trtarget;
int must_update_screen = 0;

gint xi, yi;

if(!GLOBALS->dnd_state) goto bot;

if(GLOBALS->std_dnd_tgt_on_signalarea || GLOBALS->std_dnd_tgt_on_wavearea)
	{
	GtkAdjustment *wadj;
        wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);

	WAVE_GDK_GET_POINTER(GLOBALS->std_dnd_tgt_on_signalarea ? gtk_widget_get_window(GLOBALS->signalarea) : gtk_widget_get_window(GLOBALS->wavearea), &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

        which=(int)(y);
        which=(which/GLOBALS->fontheight)-2;
	if(which < -1) which = -1;

	trtarget=((int)gtk_adjustment_get_value(wadj))+which;

	ddest = (GLOBALS->std_dnd_tgt_on_signalarea) ? GTK_WIDGET(GLOBALS->signalarea) : GTK_WIDGET(GLOBALS->wavearea);
        GtkAllocation allocation;
        gtk_widget_get_allocation(ddest, &allocation);
	if((x<0)||(x>=allocation.width)||(y<0)||(y>=allocation.height))
		{
		goto bot;
		}

	GLOBALS->cachedtrace=t=GLOBALS->traces.first;
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

	while(t && t->t_next && IsGroupEnd(t->t_next) && IsCollapsed(t->t_next)) { /* added missing "t &&" because of possible while termination above */
	  t = t->t_next;
	}

        GLOBALS->cachedtrace=t;
        if(GLOBALS->cachedtrace)
		{
		while(t)
			{
			if(!(t->flags&TR_HIGHLIGHT))
				{
				GLOBALS->cachedtrace = t;
			        if(CutBuffer())
			        	{
			                /* char buf[32];
			                sprintf(buf,"Dragging %d trace%s.\n",GLOBALS->traces.buffercount,GLOBALS->traces.buffercount!=1?"s":"");
			                status_text(buf); */
					must_update_screen = 1;
			                }

		                GLOBALS->cachedtrace->flags|=TR_HIGHLIGHT;
				goto success;
				}

			t=GivePrevTrace(t);
			}
		goto bot;
                }

success:
	if( ((which<0) && (GLOBALS->topmost_trace==GLOBALS->traces.first) && PrependBuffer()) || (PasteBuffer()) ) /* short circuit on special which<0 case */
        	{
                /* status_text("Drop completed.\n"); */

	        if(GLOBALS->cachedtrace)
	        	{
	                GLOBALS->cachedtrace->flags&=~TR_HIGHLIGHT;
	                }

		GLOBALS->signalwindow_width_dirty=1;
                MaxSignalLength();
                signalarea_configure_event(GLOBALS->signalarea, NULL);
                wavearea_configure_event(GLOBALS->wavearea, NULL);
		must_update_screen = 0;
                }
        }

bot:

if(must_update_screen)
	{
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}

GLOBALS->dnd_cursor_timer = 0;
GLOBALS->dnd_state = 0;
GLOBALS->standard_trace_dnd_degate = 1;
}

/*
 *	DND "drag_motion" handler, this is called whenever the
 *	pointer is dragging over the target widget.
 */
static gboolean DNDDragMotionCB(
        GtkWidget *widget, GdkDragContext *dc,
        gint xx, gint yy, guint tt,
        gpointer data
)
{
(void)xx;
(void)yy;
(void)data;
	gboolean same_widget;
	GdkDragAction suggested_action;
	GtkWidget *src_widget, *tar_widget;

        if((widget == NULL) || (dc == NULL))
                return(FALSE);


	/* Get source widget and target widget. */
	src_widget = gtk_drag_get_source_widget(dc);
	tar_widget = widget;

	/* Note if source widget is the same as the target. */
	same_widget = (src_widget == tar_widget) ? TRUE : FALSE;
	if(same_widget)
		{
		/* nothing */
		}

	GLOBALS->std_dnd_tgt_on_signalarea = (tar_widget == GLOBALS->signalarea);
	GLOBALS->std_dnd_tgt_on_wavearea = (tar_widget == GLOBALS->wavearea);

	/* If this is the same widget, our suggested action should be
	 * move.  For all other case we assume copy.
	 */
	suggested_action = GDK_ACTION_MOVE;

	/* Respond with default drag action (status). First we check
	 * the dc's list of actions. If the list only contains
	 * move, copy, or link then we select just that, otherwise we
	 * return with our default suggested action.
	 * If no valid actions are listed then we respond with 0.
	 */

        /* Only move? */
        if(gdk_drag_context_get_actions(dc) == GDK_ACTION_MOVE)
            gdk_drag_status(dc, GDK_ACTION_MOVE, tt);
        /* Only copy? */
        else if(gdk_drag_context_get_actions(dc) == GDK_ACTION_COPY)
            gdk_drag_status(dc, GDK_ACTION_COPY, tt);
        /* Only link? */
        else if(gdk_drag_context_get_actions(dc) == GDK_ACTION_LINK)
            gdk_drag_status(dc, GDK_ACTION_LINK, tt);
        /* Other action, check if listed in our actions list? */
        else if(gdk_drag_context_get_actions(dc) & suggested_action)
            gdk_drag_status(dc, suggested_action, tt);
        /* All else respond with 0. */
        else
            gdk_drag_status(dc, 0, tt);

if(GLOBALS->std_dnd_tgt_on_signalarea || GLOBALS->std_dnd_tgt_on_wavearea)
	{
	GtkAdjustment *wadj;
	GtkWidget *ddest;
	int which;
	gdouble x,y;
	GdkModifierType state;
	Trptr t;
	int trwhich, trtarget;

	gint xi, yi;

        wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);

	WAVE_GDK_GET_POINTER(GLOBALS->std_dnd_tgt_on_signalarea ? gtk_widget_get_window(GLOBALS->signalarea) : gtk_widget_get_window(GLOBALS->wavearea), &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

        which=(int)(y);
        which=(which/GLOBALS->fontheight)-2;
	if(which < -1) which = -1;

	trtarget=((int)gtk_adjustment_get_value(wadj))+which;

	ddest = (GLOBALS->std_dnd_tgt_on_signalarea) ? GTK_WIDGET(GLOBALS->signalarea) : GTK_WIDGET(GLOBALS->wavearea);
        GtkAllocation allocation;
        gtk_widget_get_allocation(ddest, &allocation);
	if((x<0)||(x>=allocation.width)||(y<0)||(y>=allocation.height))
		{
		goto bot;
		}

	t=GLOBALS->traces.first;
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

	while(t && t->t_next && IsGroupEnd(t->t_next) && IsCollapsed(t->t_next)) {
	  t = t->t_next;
	}

/* 	if(t) */
/* 		{ */
/* 		while(t) */
/* 			{ */
/* 			if(t->flags & TR_HIGHLIGHT) */
/* 				{ */
/* 				t=GivePrevTrace(t); */
/* 				which--; */
/* 				} */
/* 				else */
/* 				{ */
/* 				break; */
/* 				} */
/* 			} */
/* 		} */

	if(1)
		{
	        GtkAdjustment *hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
	        GtkAdjustment *sadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	        int rsig_trtarget=(int)(gtk_adjustment_get_value(sadj));
	        gint xsrc=(gint)gtk_adjustment_get_value(hadj);
		gint ylin;

                gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);

	        cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc.gc_ltgray.r, GLOBALS->rgb_gc.gc_ltgray.g, GLOBALS->rgb_gc.gc_ltgray.b, GLOBALS->rgb_gc.gc_ltgray.a);
	        cairo_rectangle (GLOBALS->cr_signalpixmap, 0.5, 0.5,
                        GLOBALS->signal_fill_width, allocation.height);
        	cairo_fill (GLOBALS->cr_signalpixmap);

		RenderSigs(rsig_trtarget, 0);

		GLOBALS->dnd_cursor_timer = 1;
		if((t)&&(which >= -1))
			{
			if(which >= GLOBALS->traces.total) { which = GLOBALS->traces.total-1; }
			ylin = ((which + 2) * GLOBALS->fontheight) - 2;

			cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_black.r, GLOBALS->rgb_gc_black.g, GLOBALS->rgb_gc_black.b, GLOBALS->rgb_gc_black.a);
		        cairo_move_to (GLOBALS->cr_signalpixmap, 0.5, ylin+0.5);
		        cairo_line_to (GLOBALS->cr_signalpixmap, GLOBALS->signal_fill_width-1+0.5, ylin+0.5);
		        cairo_stroke (GLOBALS->cr_signalpixmap);
			}
			else
			{
			int i;

			which = -1;
			ylin = ((which + 2) * GLOBALS->fontheight) - 2;

			for(i=0;i<GLOBALS->signal_fill_width-1; i+=16)
				{
				cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_black.r, GLOBALS->rgb_gc_black.g, GLOBALS->rgb_gc_black.b, GLOBALS->rgb_gc_black.a);
			        cairo_move_to (GLOBALS->cr_signalpixmap, i+0.5, ylin+0.5);
			        cairo_line_to (GLOBALS->cr_signalpixmap, i+7+0.5, ylin+0.5);
			        cairo_stroke (GLOBALS->cr_signalpixmap);

				cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc_white.r, GLOBALS->rgb_gc_white.g, GLOBALS->rgb_gc_white.b, GLOBALS->rgb_gc_white.a);
			        cairo_move_to (GLOBALS->cr_signalpixmap, i+8+0.5, ylin+0.5);
			        cairo_line_to (GLOBALS->cr_signalpixmap, i+15+0.5, ylin+0.5);
			        cairo_stroke (GLOBALS->cr_signalpixmap);
				}
			}

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
		GdkDrawingContext *gdc;
#endif
                cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(GLOBALS->signalarea)), &gdc);
                cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
                cairo_clip (cr);

		signalwindow_paint(cr);

		draw_signalarea_focus(cr);
#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
		gdk_window_end_draw_frame(gtk_widget_get_window(GLOBALS->signalarea), gdc);
#else
		cairo_destroy (cr);
#endif

		/* printf("drop to %d of %d: '%s'\n", which, GLOBALS->traces.total, t ? t->name : "undef"); */
		}
	bot: return(FALSE);
	}

	return(FALSE);
}

static gboolean ignoreAccelerators(GdkEventKey *event)
{
if(!GLOBALS || !GLOBALS->filter_entry || !event)
	{
	return(FALSE);
	}
	else
	{
#ifdef MAC_INTEGRATION
	return (gtk_widget_has_focus(GLOBALS->filter_entry));
#else
	return (gtk_widget_has_focus(GLOBALS->filter_entry) &&
	  !(event->state & GDK_CONTROL_MASK) &&
	  !(event->state & GDK_MOD1_MASK));
#endif
	}
}

/*
 * keypress processing, return TRUE to block the event from gtk
 */
static gint keypress_local(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
(void)widget;
(void)data;

GtkAdjustment *wadj;
int num_traces_displayable;
int target;
int which;
gint rc = FALSE;
int yscroll;
int ud_kill = 0;

#ifdef FOCUS_DEBUG_MSGS
printf("focus: %d %08x %08x %08x\n", gtk_widget_has_focus(GLOBALS->signalarea_event_box),
	GLOBALS->signalarea_event_box, widget, data);
#endif

if	(         (event->keyval == GDK_KEY_equal) &&
#ifdef MAC_INTEGRATION
                  (event->state & GDK_META_MASK)
#else
                  (event->state & GDK_CONTROL_MASK)
#endif
      	)
	{
                      service_zoom_in(NULL, NULL);
                      rc = TRUE;
        }
else
#ifdef WAVE_ALLOW_GTK3_HEADER_BAR
if (event->keyval == GDK_KEY_F10)
	{
	do_popup_main_menu(NULL, NULL);
	}
else
#endif
if(gtk_widget_has_focus(GLOBALS->signalarea_event_box))
	{
	switch(event->keyval)
		{
#ifdef MAC_INTEGRATION
		/* need to do this, otherwise if a menu accelerator it steals the key from gtk */
		case GDK_KEY_a:
		  if(event->state & GDK_MOD2_MASK)
		    {
		      menu_dataformat_highlight_all(NULL, 0, NULL);
		      rc = TRUE;
		    }
		  break;

		case GDK_KEY_A:
		  if(event->state & GDK_MOD2_MASK)
		    {
		      menu_dataformat_unhighlight_all(NULL, 0, NULL);
		      rc = TRUE;
		    }
		  break;

		case GDK_KEY_x:
		  if(event->state & GDK_MOD2_MASK)
		    {
		      menu_cut_traces(NULL, 0, NULL);
		      rc = TRUE;
		    }
		  break;

		case GDK_KEY_c:
		  if(event->state & GDK_MOD2_MASK)
		    {
		      menu_copy_traces(NULL, 0, NULL);
		      rc = TRUE;
		    }
		  break;

		case GDK_KEY_v:
		  if(event->state & GDK_MOD2_MASK)
		    {
		      menu_paste_traces(NULL, 0, NULL);
		      rc = TRUE;
		    }
		  break;
#endif
		case GDK_KEY_Page_Up:
		case GDK_KEY_KP_Page_Up:
		case GDK_KEY_Page_Down:
		case GDK_KEY_KP_Page_Down:
		case GDK_KEY_Up:
		case GDK_KEY_KP_Up:
		case GDK_KEY_Down:
		case GDK_KEY_KP_Down:
			wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
   		      	GtkAllocation allocation;
      			gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);
			num_traces_displayable=(allocation.height)/(GLOBALS->fontheight);
			num_traces_displayable--;   /* for the time trace that is always there */

			if(event->state & GDK_SHIFT_MASK)
				{
				Trptr t = NULL, t2 = NULL;
				Trptr tc = NULL, th = NULL;
				int num_high = 0;

				if((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_KP_Up))					
					{
					ud_kill = 1;
					for(t=GLOBALS->traces.first;t;t=t->t_next)
	        				{	
	        				if(t->flags&TR_HIGHLIGHT)
							{
							if(!th) th = t;
							num_high++;
							}

						if(t->is_cursor)
							{
							tc = t;
							}
	        				}

					if(num_high <= 1)
						{
						t = th ? GivePrevTrace(th) : GLOBALS->topmost_trace;
						}
						else
						{
						t = tc ? GivePrevTrace(tc) : GLOBALS->topmost_trace;
						}

					MarkTraceCursor(t);
					}
				else
				if((event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_KP_Down))					
					{
					ud_kill = 1;
					for(t=GLOBALS->traces.first;t;t=t->t_next)
	        				{	
	        				if(t->flags&TR_HIGHLIGHT) 
							{
							th = t;
							num_high++;
							}
						if(t->is_cursor)
							{
							tc = t;
							}
	        				}

					if(num_high <= 1)
						{
						t = th ? GiveNextTrace(th) : GLOBALS->topmost_trace;
						}
						else
						{
						if(tc)
							{
							t = GiveNextTrace(tc);
							}
						else
							{
							t = th ? GiveNextTrace(th) : GLOBALS->topmost_trace;
							}
						}

					MarkTraceCursor(t);
					}

				if(t)
					{
					int top_target = 0;
					target = 0;
					which=num_traces_displayable-1;

					ClearTraces();
					t->flags |= TR_HIGHLIGHT;
					t2 = t;

					for(t=GLOBALS->traces.first;t;t=GiveNextTrace(t))
						{
						if(t == GLOBALS->topmost_trace) break;
						top_target++;
						}

					for(t=GLOBALS->traces.first;t;t=GiveNextTrace(t))
						{
						if(t2 == t) break;
						target++;
						}

					if((target >= top_target) && (target <= top_target+which))
						{
						/* nothing */
						}
						else
						{
						if((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_KP_Up))
							{
							if(target<0) target=0;
							}
						else
						if((event->keyval == GDK_KEY_Down) || (event->keyval == GDK_KEY_KP_Down))
							{
							target = target - which;
							if(target+which>=(GLOBALS->traces.visible-1)) target=GLOBALS->traces.visible-which-1;
							}

	                       			gtk_adjustment_set_value(wadj,target);
						}

                       			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=which-1; /* force update */

                       			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed"); /* force bar update */
                       			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
					}
				}

			if((num_traces_displayable<GLOBALS->traces.visible) && (!ud_kill))
				{
				switch(event->keyval)
					{
					case GDK_KEY_Down:
					case GDK_KEY_KP_Down:
					case GDK_KEY_Page_Down:
					case GDK_KEY_KP_Page_Down:
						yscroll = ((event->keyval == GDK_KEY_Page_Down) || (event->keyval == GDK_KEY_KP_Page_Down)) ? num_traces_displayable : 1;
			                        target=((int)gtk_adjustment_get_value(wadj))+yscroll;
			                        which=num_traces_displayable-1;

			                        if(target+which>=(GLOBALS->traces.visible-1)) target=GLOBALS->traces.visible-which-1;
                        			gtk_adjustment_set_value(wadj,target);

                        			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=which-1; /* force update */

                        			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed"); /* force bar update */
                        			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
						break;

					case GDK_KEY_Up:
					case GDK_KEY_KP_Up:
					case GDK_KEY_Page_Up:
					case GDK_KEY_KP_Page_Up:
						yscroll = ((event->keyval == GDK_KEY_Page_Up) || (event->keyval == GDK_KEY_KP_Page_Up)) ? num_traces_displayable : 1;
                        			target=((int)gtk_adjustment_get_value(wadj))-yscroll;
                        			if(target<0) target=0;
                        			gtk_adjustment_set_value(wadj,target);

						which=0;
                        			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=-1; /* force update */

                        			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed"); /* force bar update */
                        			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
						break;
					}
				}
			rc = TRUE;
			break;

		case GDK_KEY_Left:
		case GDK_KEY_KP_Left:

			service_left_edge(NULL, 0);
			/*
			hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);

			if(gtk_adjustment_get_value(hadj) < gtk_adjustment_get_page_increment(hadj))
			        {
			        gtk_adjustment_set_value(hadj, (gfloat)0.0);
			        }
				else
				{
				gtk_adjustment_set_value(hadj, gtk_adjustment_get_value(hadj) - gtk_adjustment_get_page_increment(hadj));
				}

			g_signal_emit_by_name (XXX_GTK_OBJECT (hadj), "changed");
			g_signal_emit_by_name (XXX_GTK_OBJECT (hadj), "value_changed");
			signalarea_configure_event(GLOBALS->signalarea, NULL);
			*/

			rc = TRUE;
			break;

		case GDK_KEY_Right:
		case GDK_KEY_KP_Right:

			service_right_edge(NULL, 0);
			/*
			hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);

			if( ((int) gtk_adjustment_get_value(hadj) + gtk_adjustment_get_page_increment(hadj)) >= gtk_adjustment_get_upper(hadj))
			        {
			        gtk_adjustment_set_value(hadj, (gfloat)(gtk_adjustment_get_upper(hadj))-gtk_adjustment_get_page_increment(hadj));
			        }
				else
				{
				gtk_adjustment_set_value(hadj, gtk_adjustment_get_value(hadj) + gtk_adjustment_get_page_increment(hadj));
				}

			g_signal_emit_by_name (XXX_GTK_OBJECT (hadj), "changed");
			g_signal_emit_by_name (XXX_GTK_OBJECT (hadj), "value_changed");
			signalarea_configure_event(GLOBALS->signalarea, NULL);
			*/

			rc = TRUE;
			break;

		default:
#ifdef FOCUS_DEBUG_MSGS
			printf("key %x, widget: %08x\n", event->keyval, widget);
#endif
			break;
		}
	}
else
if(GLOBALS->dnd_sigview)
	{
	  if(gtk_widget_has_focus(GLOBALS->dnd_sigview) || gtk_widget_has_focus(GLOBALS->filter_entry))
	    {
	      switch(event->keyval)
		{
		case GDK_KEY_a:
#ifdef MAC_INTEGRATION
		  if(event->state & GDK_META_MASK)
#else
		  if(event->state & GDK_CONTROL_MASK)
#endif
		    {
		      treeview_select_all_callback();
		      rc = TRUE;
		    }
		  break;

		case GDK_KEY_A:
#ifdef MAC_INTEGRATION
		  if(event->state & GDK_META_MASK)
#else
		  if(event->state & GDK_CONTROL_MASK)
#endif
		    {
		      treeview_unselect_all_callback();
		      rc = TRUE;
		    }
		default:
		  break;
		}
	    }
	else
	  if(gtk_widget_has_focus(GLOBALS->treeview_main))
	    {
	      switch(event->keyval)
		{
		case GDK_KEY_a:
#ifdef MAC_INTEGRATION
		  if(event->state & GDK_META_MASK)
#else
		  if(event->state & GDK_CONTROL_MASK)
#endif
		    {
		      /* eat keystroke */
		      rc = TRUE;
		    }
		  break;

		case GDK_KEY_A:
#ifdef MAC_INTEGRATION
		  if(event->state & GDK_META_MASK)
#else
		  if(event->state & GDK_CONTROL_MASK)
#endif
		    {
		      /* eat keystroke */
		      rc = TRUE;
		    }
		default:
		  break;
		}
	    }
	}
 if (ignoreAccelerators(event)) {
   gtk_widget_event(GLOBALS->filter_entry, (GdkEvent *)event);
   /* eat keystroke */
   rc = TRUE;
 }

return(rc);
}

static        gint
scroll_event( GtkWidget * widget, GdkEventScroll * event )
{
  GdkEventKey ev_fake;

  DEBUG(printf("Mouse Scroll Event\n"));
  switch ( event->direction )
  {
    case GDK_SCROLL_UP:
      ev_fake.keyval = GDK_KEY_Up;
      keypress_local(widget, &ev_fake, GLOBALS->signalarea_event_box);
      break;
    case GDK_SCROLL_DOWN:
      ev_fake.keyval = GDK_KEY_Down;
      keypress_local(widget, &ev_fake, GLOBALS->signalarea_event_box);

    default:
      break;
  }
  return(TRUE);
}


#if defined(WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND) || defined(WAVE_ALLOW_GTK3_VSLIDER_WORKAROUND)
static gboolean osx_timer(gpointer dummy)
{
(void) dummy;

if(GLOBALS)
	{
	if(GLOBALS->force_hide_show == 2)
		{
		if((GLOBALS->signalarea)&&(GLOBALS->wavearea))
			{
			gtk_widget_hide(GLOBALS->signalarea);
			gtk_widget_show(GLOBALS->signalarea);
			gtk_widget_hide(GLOBALS->wavearea);
			gtk_widget_show(GLOBALS->wavearea);
			}
		}

	if(GLOBALS->force_hide_show)
		{
		GLOBALS->force_hide_show--;
		}
	}

return(TRUE);
}
#endif


static gboolean mouseover_timer(gpointer dummy)
{
(void)dummy;

static gboolean run_once = FALSE;
gdouble x,y;
GdkModifierType state;
TraceEnt t_trans;

gint xi, yi;

if(GLOBALS->button2_debounce_flag)
	{
	GLOBALS->button2_debounce_flag = 0;
	}

if((GLOBALS->dnd_state)||(GLOBALS->tree_dnd_begin)) /* drag scroll on DnD */
	{
	GtkAdjustment *wadj;
	int num_traces_displayable;
	int target;
	int which;
	int yscroll;

	WAVE_GDK_GET_POINTER(gtk_widget_get_window(GLOBALS->signalarea), &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

      	GtkAllocation allocation;
	gtk_widget_get_allocation(GLOBALS->signalarea, &allocation);
	if(y > allocation.height)
		{
                wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
                num_traces_displayable=(allocation.height)/(GLOBALS->fontheight);
                num_traces_displayable--;   /* for the time trace that is always there */

               	if(num_traces_displayable<GLOBALS->traces.visible)
			{
			yscroll = 1;
                        target=((int)gtk_adjustment_get_value(wadj))+yscroll;
                        which=num_traces_displayable-1;

                        if(target+which>=(GLOBALS->traces.visible-1)) target=GLOBALS->traces.visible-which-1;
       			gtk_adjustment_set_value(wadj,target);

       			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=which-1; /* force update */

       			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed"); /* force bar update */
       			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
			}
		}
	else
	if(y < 0)
		{
                wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
                num_traces_displayable=(allocation.height)/(GLOBALS->fontheight);
                num_traces_displayable--;   /* for the time trace that is always there */

               	if(num_traces_displayable<GLOBALS->traces.visible)
			{
			yscroll = 1;
       			target=((int)gtk_adjustment_get_value(wadj))-yscroll;
       			if(target<0) target=0;
       			gtk_adjustment_set_value(wadj,target);

			which=0;
       			if(GLOBALS->cachedwhich_signalwindow_c_1==which) GLOBALS->cachedwhich_signalwindow_c_1=-1; /* force update */

       			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed"); /* force bar update */
       			g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */
			}
		}
	}

if(in_main_iteration()) return(TRUE);

if(GLOBALS->splash_is_loading)
	{
	return(TRUE);
        }

if(GLOBALS->splash_fix_win_title)
	{
	GLOBALS->splash_fix_win_title = 0;
	wave_gtk_window_set_title(GTK_WINDOW(GLOBALS->mainwindow), GLOBALS->winname, GLOBALS->dumpfile_is_modified ? WAVE_SET_TITLE_MODIFIED: WAVE_SET_TITLE_NONE, 0);
	}

#ifdef MAC_INTEGRATION
if(GLOBALS->dnd_helper_quartz)
        {
        char *dhq = g_malloc(strlen(GLOBALS->dnd_helper_quartz)+1);
        strcpy(dhq, GLOBALS->dnd_helper_quartz);
        free_2(GLOBALS->dnd_helper_quartz);
        GLOBALS->dnd_helper_quartz = NULL;
        DND_helper_quartz(dhq);
        g_free(dhq);
        }
#endif

if(process_finder_names_queued())
	{
	if(GLOBALS->pFileChoose)
		{
		if(!GLOBALS->window_simplereq_c_9)
			{
			char *qn = process_finder_extract_queued_name();
			if(qn)
				{
				int qn_len = strlen(qn);
				const int mlen = 30;
				if(qn_len < mlen)
					{
					simplereqbox("File queued for loading",300,qn,"OK", NULL, NULL, 1);
					}
					else
					{
					char *qn_2 = wave_alloca(mlen + 4);
					strcpy(qn_2, "...");
					strcat(qn_2, qn + qn_len - mlen);
					simplereqbox("File queued for loading",300,qn_2,"OK", NULL, NULL, 1);
					}
				return(TRUE);
				}
			}
		}
		else
		{
		if(process_finder_name_integration())
			{
			return(TRUE);
			}
		}
	}

if(GLOBALS->loaded_file_type == MISSING_FILE)
	{
	return(TRUE);
	}

if(run_once == FALSE) /* avoid any race conditions with the toolkit for uninitialized data */
	{
	run_once = TRUE;
	return(TRUE);
	}

if((!GLOBALS->signalarea) || (!gtk_widget_get_window(GLOBALS->signalarea)))
	{
	return(TRUE);
	}

if(GLOBALS->dnd_cursor_timer)
	{
	GLOBALS->dnd_cursor_timer++;
	if(GLOBALS->dnd_cursor_timer == 50)
		{
		GLOBALS->dnd_cursor_timer = 0;
	        signalarea_configure_event(GLOBALS->signalarea, NULL);
		}
	}

if(GLOBALS->mouseover_counter < 0) return(TRUE); /* mouseover is up in wave window so don't bother */

WAVE_GDK_GET_POINTER(gtk_widget_get_window(GLOBALS->signalarea), &x, &y, &xi, &yi, &state);
WAVE_GDK_GET_POINTER_COPY;

GLOBALS->mouseover_counter++;

GtkAllocation s_allocation;
gtk_widget_get_allocation(GLOBALS->signalarea, &s_allocation);

if(!((x>=0)&&(x<s_allocation.width)&&(y>=0)&&(y<s_allocation.height)))
	{
	move_mouseover_sigs(NULL, 0, 0, LLDescriptor(0));
	}
else
if(GLOBALS->mouseover_counter == 10)
	{
      	GtkAllocation allocation;
	gtk_widget_get_allocation(GLOBALS->wavearea, &allocation);
	int num_traces_displayable=allocation.height/(GLOBALS->fontheight);
	int yr = GLOBALS->cached_mouseover_y;
	int i;
	Trptr t=NULL;

	num_traces_displayable--;   /* for the time trace that is always there */

	yr-=GLOBALS->fontheight;
	if(yr<0) goto bot;
	yr/=GLOBALS->fontheight;             /* y now indicates the trace in question */
	if(yr>num_traces_displayable) goto bot;

	t=GLOBALS->topmost_trace;

	for(i=0;i<yr;i++)
	        {
	        if(!t) goto bot;
	        t=GiveNextTrace(t);
	        }

	if(!t) goto bot;
	if((t->flags&(/*TR_BLANK|*/TR_EXCLUDE))) /* TR_BLANK removed because of transaction handling below... */
	        {
	        t = NULL;
	        goto bot;
	        }

if(t->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH))  /* seek to real xact trace if present... */
        {
        Trptr tscan = t;
        int bcnt = 0;
        while((tscan) && (tscan = GivePrevTrace(tscan)))
                {
                if(!(tscan->flags & (TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
                        {
                        if(tscan->flags & TR_TTRANSLATED)
                                {
                                break; /* found it */
                                }
                                else
                                {
                                tscan = NULL;
                                }
                        }
                        else
                        {
                        bcnt++; /* bcnt is number of blank traces */
                        }
                }

        if((tscan)&&(tscan->vector))
                {
                bvptr bv = tscan->n.vec;
                do
                        {
                        bv = bv->transaction_chain; /* correlate to blank trace */
                        } while(bv && (bcnt--));
                if(bv)
                        {
                        memcpy(&t_trans, tscan, sizeof(TraceEnt)); /* substitute into a synthetic trace */
                        t_trans.n.vec = bv;
			t_trans.vector = 1;

                        t_trans.name = bv->bvname;
                        if(GLOBALS->hier_max_level)
                                t_trans.name = hier_extract(t_trans.name, GLOBALS->hier_max_level);

                        t = &t_trans;
                        goto bot; /* is goto process_trace; in wavewindow.c */
                        }
                }
        }

if((t->flags&TR_BLANK))
        {
        t = NULL;
        goto bot;
        }

	if(t->flags & TR_ANALOG_BLANK_STRETCH)  /* seek to real analog trace is present... */
	        {
	        while((t) && (t = t->t_prev))
	                {
	                if(!(t->flags & TR_ANALOG_BLANK_STRETCH))
	                        {
	                        if(t->flags & TR_ANALOGMASK)
	                                {
	                                break; /* found it */
	                                }
	                                else
	                                {
	                                t = NULL;
	                                }
	                        }
	                }
	        }

bot:
	if(t)
		{
		move_mouseover_sigs(t, GLOBALS->cached_mouseover_x, GLOBALS->cached_mouseover_y, GLOBALS->tims.marker);
		}
		else
		{
		move_mouseover_sigs(NULL, 0, 0, LLDescriptor(0));
		}
	}

return(TRUE);
}

static gint motion_notify_event_std(GtkWidget *widget, GdkEventMotion *event)
{
(void)widget;

gdouble x,y;
GdkModifierType state;

gint xi, yi;

if(event->is_hint)
	{
	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;
	}
	else
	{
	x = event->x;
	y = event->y;
	state = event->state;
	}

GLOBALS->cached_mouseover_x = x;
GLOBALS->cached_mouseover_y = y;
GLOBALS->mouseover_counter = 0;

move_mouseover_sigs(NULL, 0, 0, LLDescriptor(0));

return(TRUE);
}


static gint button_release_event_std(GtkWidget *widget, GdkEventButton *event)
{
(void)widget;
(void)event;

if(GLOBALS->std_collapse_pressed)
	{
	GLOBALS->std_collapse_pressed = 0;
	}

return(TRUE);
}


static gint button_press_event_std(GtkWidget *widget, GdkEventButton *event)
{
int num_traces_displayable;
int which;
int trwhich, trtarget;
GtkAdjustment *wadj;
Trptr t, t2;

GtkAllocation allocation;
gtk_widget_get_allocation(widget, &allocation);

if(GLOBALS->signalarea_event_box)
	{

	  /* Don't mess with highlights with button 2 (save for dnd) */
	  if((event->button == 2) && (event->type == GDK_BUTTON_PRESS))
	    {
	      return(TRUE);
	    }

	  /* Don't mess with highlights with button 3 (save for menu_check) */
	  if((event->button == 3) && (event->type == GDK_BUTTON_PRESS))
	    {
	      goto menu_chk;
	    }

	if((event->x<0)||(event->x>=allocation.width)||(event->y<0)||(event->y>=allocation.height))
		{
		/* let gtk take focus from us with focus out event */
		}
		else
		{
		if(!GLOBALS->signalarea_has_focus)
			{
			GLOBALS->signalarea_has_focus = TRUE;
			gtk_widget_grab_focus(GTK_WIDGET(GLOBALS->signalarea_event_box));
			}
		}
	}

if((GLOBALS->traces.visible)&&(GLOBALS->cr_signalpixmap))
	{
	num_traces_displayable=allocation.height/(GLOBALS->fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	which=(int)(event->y);
	which=(which/GLOBALS->fontheight)-1;

	if(which>=GLOBALS->traces.visible)
		{
#ifdef MAC_INTEGRATION
		if((event->state&(GDK_MOD2_MASK|GDK_SHIFT_MASK)) == (GDK_SHIFT_MASK))
#else
		if((event->state&(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) == (GDK_SHIFT_MASK))
#endif
			{
			/* ok for plain-vanilla shift click only */
			which = GLOBALS->traces.visible-1;
			}
			else
			{
			  ClearTraces();
			  goto redraw; /* off in no man's land */
			}
		}

	if((which>=num_traces_displayable)||(which<0))
		{
		  ClearTraces();
		  goto redraw; /* off in no man's land */
		}

	wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
	trtarget=((int)gtk_adjustment_get_value(wadj))+which;

	t=GLOBALS->traces.first;
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


#ifdef MAC_INTEGRATION
	if(event->state & GDK_MOD2_MASK)
#else
	if(event->state&GDK_CONTROL_MASK)
#endif
		{
		if(t) /* scan-build */
		  {
		  if(IsGroupBegin(t) && IsSelected(t))
		    {
		      ClearGroupTraces(t);
		    }
		  else if(IsGroupEnd(t) && IsSelected(t))
		    {
		      ClearGroupTraces(t->t_match);
		    }
		  else
		    {
		      t->flags ^= TR_HIGHLIGHT; MarkTraceCursor(t);
		    }
		  }
		}
	else
	if((event->state&GDK_SHIFT_MASK)&&(GLOBALS->starting_unshifted_trace))
		{
		int src = -1, dst = -1, dsto = -1;
		int cnt = 0;

		t2=GLOBALS->traces.first;
		while(t2)
			{
			if(t2 == t) { dst = cnt; }
			if(t2 == GLOBALS->starting_unshifted_trace) { src = cnt; }

			cnt++;

			/*			t2->flags &= ~TR_HIGHLIGHT; */
			t2 = t2->t_next;
			}

		if(src != -1)
			{
			  cnt = 0;
			  t2=GLOBALS->traces.first;
			  while(t2)
				{
				  if ((cnt == src) && (cnt == dst) && IsSelected(t2))
				    {
				      GLOBALS->starting_unshifted_trace = NULL;
				    }
				  t2->flags &= ~TR_HIGHLIGHT;
				  t2=t2->t_next;
				  cnt++;
				}

			dsto = dst;
			if(src > dst) { int cpy; cpy = src; src = dst; dst = cpy; }
			cnt = 0;
			t2=GLOBALS->traces.first;
			while(t2 && GLOBALS->starting_unshifted_trace)
				{
				if((cnt >= src) && (cnt <= dst))
					{
					  t2->flags |= TR_HIGHLIGHT;
					}

				if(cnt == dsto) { MarkTraceCursor(t2); }

				cnt++;
				t2=t2->t_next;
				}
			}
			else
			{
			GLOBALS->starting_unshifted_trace = t;
			if(t) { t->flags |= TR_HIGHLIGHT; }  /* scan-build */
			}
		}
	/*	else if(!(t->flags & TR_HIGHLIGHT)) Ben Sferrazza suggested fix rather than a regular "else" 11aug08 */
	/*	changed to add use_standard_trace_select below to make this selectable, Sophana K request 08oct12 */
else if( (!GLOBALS->use_standard_trace_select) || (GLOBALS->standard_trace_dnd_degate) || ((t)&&(!(t->flags & TR_HIGHLIGHT))) )
		{
		GLOBALS->starting_unshifted_trace = t;

		t2=GLOBALS->traces.first;
		while(t2)
			{
			t2->flags &= ~TR_HIGHLIGHT;
			t2 = t2->t_next;
			}

		if(t) { t->flags |= TR_HIGHLIGHT; MarkTraceCursor(t); } /* scan-build */
		}

	GLOBALS->standard_trace_dnd_degate = 0;

	if(event->type == GDK_2BUTTON_PRESS)
	  {
	    menu_toggle_group(NULL, 0, widget);
	    goto menu_chk;
	  }

	redraw:

	GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	}


menu_chk:
if((event->button == 3) && (event->type == GDK_BUTTON_PRESS))
	{
      	do_popup_menu (widget, event);
    	}

return(TRUE);
}

/***  standard click routines turned on with "use_standard_clicking"=1  ***/
/**************************************************************************/


/**************************************************************************/
/***  standard click routines turned on with "use_standard_clicking"=0  ***/
/***                                                                    ***/
/***                        no longer supported                         ***/
/***                                                                    ***/
/***  gtkwave click routines turned on with "use_standard_clicking"=0   ***/
/**************************************************************************/


gint signalarea_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
(void)event;

GtkAdjustment *wadj, *hadj;
int num_traces_displayable;
int width;
int scale_factor;

if((!widget)||(!gtk_widget_get_window(widget))) return(TRUE);
scale_factor = XXX_gtk_widget_get_scale_factor(widget);

#if defined(WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND) || defined(WAVE_ALLOW_GTK3_VSLIDER_WORKAROUND)
if(!GLOBALS->force_hide_show)
	{
	GLOBALS->force_hide_show = 2;
	}
#endif

make_sigarea_gcs(widget);
UpdateTracesVisible();

GtkAllocation allocation;
gtk_widget_get_allocation(widget, &allocation);

num_traces_displayable=allocation.height/(GLOBALS->fontheight);
num_traces_displayable--;   /* for the time trace that is always there */


DEBUG(printf("SigWin Configure Event h: %d, w: %d\n",
		allocation.height,
		allocation.width));

GLOBALS->old_signal_fill_width=GLOBALS->signal_fill_width;
GLOBALS->signal_fill_width = ((width=allocation.width) > GLOBALS->signal_pixmap_width)
        ? allocation.width : GLOBALS->signal_pixmap_width;

if(GLOBALS->cr_signalpixmap)
	{
	if((GLOBALS->old_signal_fill_width!=GLOBALS->signal_fill_width)||(GLOBALS->old_signal_fill_height!=allocation.height))
		{
	        cairo_destroy (GLOBALS->cr_signalpixmap);
	        cairo_surface_destroy (GLOBALS->surface_signalpixmap);

		GLOBALS->surface_signalpixmap = cairo_image_surface_create (CAIRO_FORMAT_RGB24, GLOBALS->signal_fill_width*scale_factor, allocation.height*scale_factor);
		GLOBALS->cr_signalpixmap = cairo_create (GLOBALS->surface_signalpixmap);
		cairo_scale(GLOBALS->cr_signalpixmap, scale_factor, scale_factor);
		cairo_set_line_width(GLOBALS->cr_signalpixmap, 1.0);
		}
	}
	else
	{
	GLOBALS->surface_signalpixmap = cairo_image_surface_create (CAIRO_FORMAT_RGB24, GLOBALS->signal_fill_width*scale_factor, allocation.height*scale_factor);
	GLOBALS->cr_signalpixmap = cairo_create (GLOBALS->surface_signalpixmap);
	cairo_scale(GLOBALS->cr_signalpixmap, scale_factor, scale_factor);
	cairo_set_line_width(GLOBALS->cr_signalpixmap, 1.0);
	}

 if (!GLOBALS->left_justify_sigs && !GLOBALS->do_resize_signals)
   {
   if (width < GLOBALS->max_signal_name_pixel_width+15)
     {
       int delta = GLOBALS->max_signal_name_pixel_width+15 - width;

	 if(GLOBALS->cr_signalpixmap)
	   {

	     hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);

	     /* int pos = GLOBALS->max_signal_name_pixel_width+15 - (gint)gtk_adjustment_get_value(hadj); */

	     if ((gint) gtk_adjustment_get_value(hadj) > delta)
	       {
		 GLOBALS->right_align_active = 1;
		 delta = (gint)gtk_adjustment_get_value(hadj);
	       }
	     if (GLOBALS->right_align_active)
	       gtk_adjustment_set_value(hadj, (gint)delta);
	   }
       } else {
	 GLOBALS->right_align_active = 1;
       }
   }

GLOBALS->old_signal_fill_height= allocation.height;

cairo_set_source_rgba (GLOBALS->cr_signalpixmap, GLOBALS->rgb_gc.gc_ltgray.r, GLOBALS->rgb_gc.gc_ltgray.g, GLOBALS->rgb_gc.gc_ltgray.b, GLOBALS->rgb_gc.gc_ltgray.a);
cairo_rectangle (GLOBALS->cr_signalpixmap, 0.5, 0.5,
	GLOBALS->signal_fill_width, allocation.height);
cairo_fill (GLOBALS->cr_signalpixmap);

hadj=GTK_ADJUSTMENT(GLOBALS->signal_hslider);
gtk_adjustment_set_page_increment(hadj, (gfloat)width);
gtk_adjustment_set_page_size(hadj,gtk_adjustment_get_page_increment(hadj));
gtk_adjustment_set_page_increment(hadj,(gfloat)10.0);  /* approx 1ch at a time */
gtk_adjustment_set_lower(hadj,(gfloat)0.0);
gtk_adjustment_set_upper(hadj,(gfloat)GLOBALS->signal_pixmap_width);

if( ((int)gtk_adjustment_get_value(hadj))+width > GLOBALS->signal_fill_width)
	{
	gtk_adjustment_set_value(hadj, (gfloat)(GLOBALS->signal_fill_width-width));
	}

/* in gtk3 there is a race condition in how updating GLOBALS->wave_vslider causes resizing during configure phase */
#ifndef WAVE_GTK3_SIZE_ALLOCATE_WORKAROUND_WAVE_VSLIDER
wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
gtk_adjustment_set_page_size(wadj,(gfloat) num_traces_displayable);
gtk_adjustment_set_page_increment(wadj,(gfloat) num_traces_displayable);
gtk_adjustment_set_step_increment(wadj,(gfloat)1.0);
gtk_adjustment_set_lower(wadj,(gfloat)0.0);
gtk_adjustment_set_upper(wadj,(gfloat)(GLOBALS->traces.visible ? GLOBALS->traces.visible : 1));

if(GLOBALS->traces.scroll_bottom)
	{
	Trptr t = GLOBALS->traces.first;
	int which = 0;
	int scroll_top = -1, scroll_bottom = -1;
	int cur_top = gtk_adjustment_get_value(wadj);
	int cur_bottom = cur_top + num_traces_displayable - 1;

	while(t)
		{
		if(t == GLOBALS->traces.scroll_top)
			{
			scroll_top = which;
			}

		if(t == GLOBALS->traces.scroll_bottom)
			{
			scroll_bottom = which;
			break;
			}

		t = GiveNextTrace(t);
		which++;
		}

	GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = NULL;

	if((scroll_top >= 0) && (scroll_bottom >= 0))
		{
		if((scroll_top > cur_top) && (scroll_bottom <= cur_bottom))
			{
			/* nothing */
			}
			else
			{
			if((scroll_bottom - scroll_top + 1) >= num_traces_displayable)
				{
				gtk_adjustment_set_value(wadj,(gfloat)(scroll_bottom - num_traces_displayable + 1));
				}
				else
				{
				int midpoint = (cur_top + cur_bottom) / 2;

				if(scroll_top <= cur_top)
					{
					gtk_adjustment_set_value(wadj,(gfloat)scroll_top-1);
					}
				else if(scroll_top >= cur_bottom)
					{
					gtk_adjustment_set_value(wadj,(gfloat)(scroll_bottom - num_traces_displayable + 1));
					}
				else
				if(scroll_top < midpoint)
					{
					gtk_adjustment_set_value(wadj,(gfloat)scroll_top-1);
					}
				else
					{
					gtk_adjustment_set_value(wadj,(gfloat)(scroll_bottom - num_traces_displayable + 1));
					}
				}

			if(gtk_adjustment_get_value(wadj) < 0.0) gtk_adjustment_set_value(wadj, 0.0);
			}
		}
	}

if(num_traces_displayable>GLOBALS->traces.visible)
	{
	gtk_adjustment_set_value(wadj, (gfloat)(GLOBALS->trtarget_signalwindow_c_1=0));
	}
	else
	if (gtk_adjustment_get_value(wadj) + num_traces_displayable > GLOBALS->traces.visible)
	{
	gtk_adjustment_set_value(wadj,(gfloat)(GLOBALS->trtarget_signalwindow_c_1=GLOBALS->traces.visible-num_traces_displayable));
	}

g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "changed");	/* force bar update */
g_signal_emit_by_name (XXX_GTK_OBJECT (wadj), "value_changed"); /* force text update */

g_signal_emit_by_name (XXX_GTK_OBJECT (hadj), "changed");	/* force bar update */

#else

wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
GLOBALS->wave_vslider_page_size = (gfloat) num_traces_displayable;
GLOBALS->wave_vslider_page_increment = (gfloat) num_traces_displayable;
GLOBALS->wave_vslider_step_increment = (gfloat)1.0;
GLOBALS->wave_vslider_lower = (gfloat)0.0;
GLOBALS->wave_vslider_upper = (gfloat)(GLOBALS->traces.visible ? GLOBALS->traces.visible : 1);
GLOBALS->wave_vslider_value = gtk_adjustment_get_value(wadj);

if(GLOBALS->traces.scroll_bottom)
	{
	Trptr t = GLOBALS->traces.first;
	int which = 0;
	int scroll_top = -1, scroll_bottom = -1;
	int cur_top = gtk_adjustment_get_value(wadj);
	int cur_bottom = cur_top + num_traces_displayable - 1;

	while(t)
		{
		if(t == GLOBALS->traces.scroll_top)
			{
			scroll_top = which;
			}

		if(t == GLOBALS->traces.scroll_bottom)
			{
			scroll_bottom = which;
			break;
			}

		t = GiveNextTrace(t);
		which++;
		}

	GLOBALS->traces.scroll_top = GLOBALS->traces.scroll_bottom = NULL;

	if((scroll_top >= 0) && (scroll_bottom >= 0))
		{
		if((scroll_top > cur_top) && (scroll_bottom <= cur_bottom))
			{
			/* nothing */
			}
			else
			{
			if((scroll_bottom - scroll_top + 1) >= num_traces_displayable)
				{
				GLOBALS->wave_vslider_value = (gfloat)(scroll_bottom - num_traces_displayable + 1);
				}
				else
				{
				int midpoint = (cur_top + cur_bottom) / 2;

				if(scroll_top <= cur_top)
					{
					GLOBALS->wave_vslider_value = (gfloat)scroll_top-1;
					}
				else if(scroll_top >= cur_bottom)
					{
					GLOBALS->wave_vslider_value = (gfloat)(scroll_bottom - num_traces_displayable + 1);
					}
				else
				if(scroll_top < midpoint)
					{
					GLOBALS->wave_vslider_value = (gfloat)scroll_top-1;
					}
				else
					{
					GLOBALS->wave_vslider_value = (gfloat)(scroll_bottom - num_traces_displayable + 1);
					}
				}

			if(GLOBALS->wave_vslider_value < 0.0) GLOBALS->wave_vslider_value = 0.0;
			}
		}
	}

if(num_traces_displayable>GLOBALS->traces.visible)
	{
	GLOBALS->wave_vslider_value = (gfloat)(GLOBALS->trtarget_signalwindow_c_1=0);
	}
	else
	if (GLOBALS->wave_vslider_value + num_traces_displayable > GLOBALS->traces.visible)
	{
	GLOBALS->wave_vslider_value = (gfloat)(GLOBALS->trtarget_signalwindow_c_1=GLOBALS->traces.visible-num_traces_displayable);
	}

service_vslider(NULL, NULL); /* forces update of signals */
GLOBALS->wave_vslider_valid = 1;

#endif

return(TRUE);
}

static gint signalarea_configure_event_local(GtkWidget *widget, GdkEventConfigure *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
struct Global *g_old = GLOBALS;

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = signalarea_configure_event(widget, event);

set_GLOBALS(g_old);

return(rc);
}

#if GTK_CHECK_VERSION(3,0,0)
static gint draw_event(GtkWidget *widget, cairo_t *cr, gpointer      user_data)
{
(void) widget;
(void) user_data;

gint rc = FALSE;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
/* struct Global *g_old = GLOBALS; */

set_GLOBALS((*GLOBALS->contexts)[page_num]);

signalwindow_paint(cr);

draw_signalarea_focus(cr);

/* seems to cause a conflict flipping back so don't! */
/* set_GLOBALS(g_old); */

return(rc);
}
#else
static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
GdkDrawingContext *gdc;
#endif
cairo_t* cr = XXX_gdk_cairo_create (XXX_GDK_DRAWABLE (gtk_widget_get_window(widget)), &gdc);
gdk_cairo_region (cr, event->region);
cairo_clip (cr);

signalwindow_paint(cr);

draw_signalarea_focus(cr);

#ifdef WAVE_ALLOW_GTK3_CAIRO_CREATE_FIX
gdk_window_end_draw_frame(gtk_widget_get_window(widget), gdc);
#else
cairo_destroy (cr);
#endif

return(FALSE);
}

static gint expose_event_local(GtkWidget *widget, GdkEventExpose *event)
{
gint rc;
gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(GLOBALS->notebook));
/* struct Global *g_old = GLOBALS; */

set_GLOBALS((*GLOBALS->contexts)[page_num]);

rc = expose_event(widget, event);

/* seems to cause a conflict flipping back so don't! */
/* set_GLOBALS(g_old); */

return(rc);
}
#endif

static int focus_in_local(GtkWidget *widget, GdkEventFocus *event)
{
#ifdef FOCUS_DEBUG_MSGS
(void)event;

printf("Focus in: %08x %08x\n", widget, GLOBALS->signalarea_event_box);
#else
(void)widget;
(void)event;
#endif

GLOBALS->signalarea_has_focus = TRUE;

signalarea_configure_event(GLOBALS->signalarea, NULL);

return(FALSE);
}

static int focus_out_local(GtkWidget *widget, GdkEventFocus *event)
{
#ifdef FOCUS_DEBUG_MSGS
(void)event;

printf("Focus out: %08x\n", widget);
#else
(void)widget;
(void)event;
#endif

GLOBALS->signalarea_has_focus = FALSE;

signalarea_configure_event(GLOBALS->signalarea, NULL);

return(FALSE);
}

GtkWidget *
create_signalwindow(void)
{
GtkWidget *table;
GtkWidget *frame;
char do_focusing = 0;

table = XXX_gtk_table_new(10, 10, FALSE);

GLOBALS->signalarea=gtk_drawing_area_new();

gtk_widget_show(GLOBALS->signalarea);
MaxSignalLength();

gtk_widget_set_events(GLOBALS->signalarea,
		GDK_SCROLL_MASK |
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK |
		GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
		);

g_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "configure_event", G_CALLBACK(signalarea_configure_event_local), NULL);

#if GTK_CHECK_VERSION(3,0,0)
g_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "draw", G_CALLBACK(draw_event), NULL);
#else
g_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "expose_event",G_CALLBACK(expose_event_local), NULL);
#endif

sclick:
if(GLOBALS->use_standard_clicking)
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
        	GTK_WIDGET(GLOBALS->signalarea),
                GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                GTK_DEST_DEFAULT_DROP,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
		GDK_ACTION_MOVE
                );

        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "drag_motion", G_CALLBACK(DNDDragMotionCB), GTK_WIDGET(GLOBALS->signalarea));
        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "drag_begin", G_CALLBACK(DNDBeginCB), GTK_WIDGET(GLOBALS->signalarea));
        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "drag_end", G_CALLBACK(DNDEndCB), GTK_WIDGET(GLOBALS->signalarea));
        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "drag_failed", G_CALLBACK(DNDFailedCB), GTK_WIDGET(GLOBALS->signalarea));


        gtk_drag_dest_set(
        	GTK_WIDGET(GLOBALS->wavearea),
                GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT |
                GTK_DEST_DEFAULT_DROP,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
		GDK_ACTION_MOVE
                );

        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "drag_motion", G_CALLBACK(DNDDragMotionCB), GTK_WIDGET(GLOBALS->wavearea));
        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "drag_begin", G_CALLBACK(DNDBeginCB), GTK_WIDGET(GLOBALS->wavearea));
        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "drag_end", G_CALLBACK(DNDEndCB), GTK_WIDGET(GLOBALS->wavearea));
        gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->wavearea), "drag_failed", G_CALLBACK(DNDFailedCB), GTK_WIDGET(GLOBALS->wavearea));

	gtk_drag_source_set(GTK_WIDGET(GLOBALS->signalarea),
        	GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                target_entry,
                sizeof(target_entry) / sizeof(GtkTargetEntry),
                GDK_ACTION_PRIVATE);

	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "button_press_event",G_CALLBACK(button_press_event_std), NULL);
	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "button_release_event", G_CALLBACK(button_release_event_std), NULL);
	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "motion_notify_event",G_CALLBACK(motion_notify_event_std), NULL);
	g_timeout_add(100, mouseover_timer, NULL);
#if defined(WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND) || defined(WAVE_ALLOW_GTK3_VSLIDER_WORKAROUND)
	g_timeout_add(100, osx_timer, NULL);
#endif

	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea), "scroll_event",G_CALLBACK(scroll_event), NULL);
	do_focusing = 1;
	}
	else
	{
	fprintf(stderr, "GTKWAVE | \"use_standard_clicking off\" has been removed.\n");
	fprintf(stderr, "GTKWAVE | Please update your rc files accordingly.\n");
	GLOBALS->use_standard_clicking = 1;
	goto sclick;
	}

XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->signalarea, 0, 10, 0, 9,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 3, 2);

GLOBALS->signal_hslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signal_hslider), "value_changed",G_CALLBACK(service_hslider), NULL);
GLOBALS->hscroll_signalwindow_c_1=XXX_gtk_hscrollbar_new(GTK_ADJUSTMENT(GLOBALS->signal_hslider));
gtk_widget_show(GLOBALS->hscroll_signalwindow_c_1);

#ifdef WAVE_ALLOW_GTK3_GRID
XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->hscroll_signalwindow_c_1, 0, 10, 9, 10,
                        0,
                        GTK_SHRINK, 3, 4);
#else
XXX_gtk_table_attach (XXX_GTK_TABLE (table), GLOBALS->hscroll_signalwindow_c_1, 0, 10, 9, 10,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 4);
#endif
gtk_widget_show(table);

frame=gtk_frame_new("Signals");
gtk_container_set_border_width(GTK_CONTAINER(frame),2);

gtk_container_add(GTK_CONTAINER(frame),table);

if(do_focusing)
	{
	GLOBALS->signalarea_event_box = gtk_event_box_new();
	gtk_container_add (GTK_CONTAINER (GLOBALS->signalarea_event_box), frame);
	gtk_widget_show(frame);

	gtk_widget_set_can_default(GTK_WIDGET(GLOBALS->signalarea_event_box), TRUE);
	gtk_widget_set_can_focus(GTK_WIDGET(GLOBALS->signalarea_event_box), TRUE);
	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea_event_box), "focus_in_event", G_CALLBACK(focus_in_local), NULL);
	gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea_event_box), "focus_out_event", G_CALLBACK(focus_out_local), NULL);

	/* not necessary for now... */
	/* gtkwave_signal_connect(XXX_GTK_OBJECT(GLOBALS->signalarea_event_box), "popup_menu",G_CALLBACK(popup_event), NULL); */

	if(!GLOBALS->second_page_created)
		{
	        if(!GLOBALS->keypress_handler_id)
	                {
			GLOBALS->keypress_handler_id = install_keypress_handler();
	                }
		}

	return(GLOBALS->signalarea_event_box);
	}
	else
	{
	return(frame);
	}
}


gint install_keypress_handler(void)
{
gint rc =
	g_signal_connect(XXX_GTK_OBJECT(GLOBALS->mainwindow),
	"key_press_event",G_CALLBACK(keypress_local), NULL);

return(rc);
}


void remove_keypress_handler(gint id)
{
g_signal_handler_disconnect(XXX_GTK_OBJECT(GLOBALS->mainwindow), id);
}


