/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include "pixmaps.h"
#include "currenttime.h"
#include "debug.h"

void fix_wavehadj(void)
{
GtkAdjustment *hadj;
gfloat pageinc;

hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
gtk_adjustment_set_lower(hadj, GLOBALS->tims.first);
gtk_adjustment_set_upper(hadj, GLOBALS->tims.last+2.0);

pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
gtk_adjustment_set_page_increment(hadj, (pageinc>=1.0)?pageinc:1.0);
gtk_adjustment_set_page_size(hadj, gtk_adjustment_get_page_increment(hadj));

/* hadj->step_increment=(GLOBALS->nspx>=1.0)?GLOBALS->nspx:1.0; */

gtk_adjustment_set_step_increment (hadj, pageinc / 10.0);
if(gtk_adjustment_get_step_increment (hadj) < 1.0) gtk_adjustment_set_step_increment (hadj, 1.0);

#if GTK_CHECK_VERSION(3,0,0)
gtk_adjustment_set_value(hadj, GLOBALS->tims.start); /* work around GTK3 clamping code */
#endif

if(gtk_adjustment_get_page_size(hadj) >= (gtk_adjustment_get_upper(hadj)-gtk_adjustment_get_lower(hadj))) gtk_adjustment_set_value(hadj, gtk_adjustment_get_lower(hadj));
if(gtk_adjustment_get_value(hadj)+gtk_adjustment_get_page_size(hadj)>gtk_adjustment_get_upper(hadj))
	{
	gtk_adjustment_set_value(hadj, gtk_adjustment_get_upper(hadj)-gtk_adjustment_get_page_size(hadj));
	if(gtk_adjustment_get_value(hadj)<gtk_adjustment_get_lower(hadj))
		gtk_adjustment_set_value(hadj, gtk_adjustment_get_lower(hadj));
	}
}

void service_zoom_left(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

GtkAdjustment *hadj;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom To Start");
        help_text(
		" is used to jump scroll to the trace's beginning."
        );
        return;
        }

hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
gtk_adjustment_set_value(hadj, GLOBALS->tims.timecache=GLOBALS->tims.first);
time_update();
}

void service_zoom_right(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

GtkAdjustment *hadj;
TimeType ntinc;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom To End");
        help_text(
		" is used to jump scroll to the trace's end."
        );
        return;
        }

ntinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);

GLOBALS->tims.timecache=GLOBALS->tims.last-ntinc+1;
        if(GLOBALS->tims.timecache<GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.first;

hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
gtk_adjustment_set_value(hadj, GLOBALS->tims.timecache);
time_update();
}

void service_zoom_out(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

TimeType middle=0, width;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Out");
        help_text(
		" is used to decrease the zoom factor around the marker."
		" Alt + Scrollwheel Up also hits this button in non-alternative wheel mode."
        );
        return;
        }

if(GLOBALS->do_zoom_center)
	{
	if((GLOBALS->tims.marker<0)||(GLOBALS->tims.marker<GLOBALS->tims.first)||(GLOBALS->tims.marker>GLOBALS->tims.last))
		{
		if(GLOBALS->tims.end>GLOBALS->tims.last) GLOBALS->tims.end=GLOBALS->tims.last;
		middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
		if((GLOBALS->tims.start&1)&&(GLOBALS->tims.end&1)) middle++;
		}
		else
		{
		middle=GLOBALS->tims.marker;
		}
	}

GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;

GLOBALS->tims.zoom--;
calczoom(GLOBALS->tims.zoom);

if(GLOBALS->do_zoom_center)
	{
	width=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	GLOBALS->tims.start=time_trunc(middle-(width/2));
        if(GLOBALS->tims.start+width>GLOBALS->tims.last) GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-width);
	if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
	gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache=GLOBALS->tims.start);
	}
	else
	{
	GLOBALS->tims.timecache=0;
	}

fix_wavehadj();

g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

DEBUG(printf("Zoombuttons out\n"));
}

void service_zoom_in(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom In");
        help_text(
                " is used to increase the zoom factor around the marker."
		" Alt + Scrollwheel Down also hits this button in non-alternative wheel mode."
        );
        return;
        }

if(GLOBALS->tims.zoom<0)		/* otherwise it's ridiculous and can cause */
	{		/* overflow problems in the scope          */
	TimeType middle=0, width;

	if(GLOBALS->do_zoom_center)
		{
		if((GLOBALS->tims.marker<0)||(GLOBALS->tims.marker<GLOBALS->tims.first)||(GLOBALS->tims.marker>GLOBALS->tims.last))
			{
			if(GLOBALS->tims.end>GLOBALS->tims.last) GLOBALS->tims.end=GLOBALS->tims.last;
			middle=(GLOBALS->tims.start/2)+(GLOBALS->tims.end/2);
			if((GLOBALS->tims.start&1)&&(GLOBALS->tims.end&1)) middle++;
			}
			else
			{
			middle=GLOBALS->tims.marker;
			}
		}

	GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;

	GLOBALS->tims.zoom++;

	calczoom(GLOBALS->tims.zoom);

	if(GLOBALS->do_zoom_center)
		{
		width=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
		GLOBALS->tims.start=time_trunc(middle-(width/2));
	        if(GLOBALS->tims.start+width>GLOBALS->tims.last) GLOBALS->tims.start=time_trunc(GLOBALS->tims.last-width);
		if(GLOBALS->tims.start<GLOBALS->tims.first) GLOBALS->tims.start=GLOBALS->tims.first;
		gtk_adjustment_set_value(GTK_ADJUSTMENT(GLOBALS->wave_hslider), GLOBALS->tims.timecache=GLOBALS->tims.start);
		}
		else
		{
		GLOBALS->tims.timecache=0;
		}

	fix_wavehadj();

	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

	DEBUG(printf("Zoombuttons in\n"));
	}
}

void service_zoom_undo(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

gdouble temp;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Undo");
        help_text(
                " is used to revert to the previous zoom value used.  Undo"
		" only works one level deep."
        );
        return;
        }


temp=GLOBALS->tims.zoom;
GLOBALS->tims.zoom=GLOBALS->tims.prevzoom;
GLOBALS->tims.prevzoom=temp;
GLOBALS->tims.timecache=0;
calczoom(GLOBALS->tims.zoom);
fix_wavehadj();

g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

DEBUG(printf("Zoombuttons Undo\n"));
}

void service_zoom_fit(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

gdouble estimated;
int fixedwidth;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Best Fit");
        help_text(
		" attempts a \"best fit\" to get the whole trace onscreen."
		"  Note that the trace may be more or less than a whole screen since"
		" this isn't a \"perfect fit.\" Also, if the middle button baseline"
		" marker is nailed down, the zoom instead of getting the whole trace"
		" onscreen will use the part of the trace between the primary"
		" marker and the baseline marker."
        );
        return;
        }

if((GLOBALS->tims.baseline>=0)&&(GLOBALS->tims.marker>=0))
	{
	service_dragzoom(GLOBALS->tims.baseline, GLOBALS->tims.marker); /* new semantics added to zoom between the two */
	}
	else
	{
	if(GLOBALS->wavewidth>4) { fixedwidth=GLOBALS->wavewidth-4; } else { fixedwidth=GLOBALS->wavewidth; }
	estimated=-log(((gdouble)(GLOBALS->tims.last-GLOBALS->tims.first+1))/((gdouble)fixedwidth)*((gdouble)200.0))/log(GLOBALS->zoombase);
	if(estimated>((gdouble)(0.0))) estimated=((gdouble)(0.0));

	GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;
	GLOBALS->tims.timecache=0;

	calczoom(estimated);
	GLOBALS->tims.zoom=estimated;

	fix_wavehadj();

	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
	}

DEBUG(printf("Zoombuttons Fit\n"));
}

void service_zoom_full(GtkWidget *text, gpointer data)
{
(void)text;
(void)data;

gdouble estimated;
int fixedwidth;

if(GLOBALS->helpbox_is_active)
        {
        help_text_bold("\n\nZoom Full");
        help_text(
		" attempts a \"best fit\" to get the whole trace onscreen."
		"  Note that the trace may be more or less than a whole screen since"
		" this isn't a \"perfect fit.\""
        );
        return;
        }

if(GLOBALS->wavewidth>4) { fixedwidth=GLOBALS->wavewidth-4; } else { fixedwidth=GLOBALS->wavewidth; }
estimated=-log(((gdouble)(GLOBALS->tims.last-GLOBALS->tims.first+1))/((gdouble)fixedwidth)*((gdouble)200.0))/log(GLOBALS->zoombase);
if(estimated>((gdouble)(0.0))) estimated=((gdouble)(0.0));

GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;
GLOBALS->tims.timecache=0;

calczoom(estimated);
GLOBALS->tims.zoom=estimated;

fix_wavehadj();

g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

DEBUG(printf("Zoombuttons Full\n"));
}


void service_dragzoom(TimeType time1, TimeType time2)	/* the function you've been waiting for... */
{
gdouble estimated;
int fixedwidth;
TimeType temp;
GtkAdjustment *hadj;
Trptr t;
int dragzoom_ok = 1;

if(time2<time1)
	{
	temp=time1;
	time1=time2;
	time2=temp;
	}

if(GLOBALS->dragzoom_threshold)
	{
	TimeType tdelta = time2 - time1;
	gdouble x = tdelta * GLOBALS->pxns;
	if(x<GLOBALS->dragzoom_threshold)
		{
		dragzoom_ok = 0;
		}
	}

if((time2>time1)&&(dragzoom_ok))	/* ensure at least 1 tick and dragzoom_threshold if set */
	{
	if(GLOBALS->wavewidth>4) { fixedwidth=GLOBALS->wavewidth-4; } else { fixedwidth=GLOBALS->wavewidth; }
	estimated=-log(((gdouble)(time2-time1+1))/((gdouble)fixedwidth)*((gdouble)200.0))/log(GLOBALS->zoombase);
	if(estimated>((gdouble)(0.0))) estimated=((gdouble)(0.0));

	GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;
	GLOBALS->tims.timecache=GLOBALS->tims.laststart=GLOBALS->tims.start=time_trunc(time1);

        for(t=GLOBALS->traces.first;t;t=t->t_next) /* have to nuke string refs so printout is ok! */
                {
                if(t->asciivalue) { free_2(t->asciivalue); t->asciivalue=NULL; }
                }

        for(t=GLOBALS->traces.buffer;t;t=t->t_next)
                {
                if(t->asciivalue) { free_2(t->asciivalue); t->asciivalue=NULL; }
                }

	if(!((GLOBALS->tims.baseline>=0)&&(GLOBALS->tims.marker>=0)))
		{
	        update_markertime(GLOBALS->tims.marker=-1);
		}
        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();


        hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
        gtk_adjustment_set_value(hadj, time1);

	calczoom(estimated);
	GLOBALS->tims.zoom=estimated;

	fix_wavehadj();

	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	g_signal_emit_by_name (XXX_GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

	DEBUG(printf("Drag Zoom\n"));
	}
}

/* Create actual buttons */
GtkWidget *
create_zoom_buttons (void)
{
GtkWidget *table;
GtkWidget *table2;
GtkWidget *frame;
GtkWidget *main_vbox;
GtkWidget *b1;
GtkWidget *b2;
GtkWidget *b3;
GtkWidget *b4;
GtkWidget *b5;
GtkWidget *b6;
GtkWidget *pixmapzout, *pixmapzin, *pixmapzfit, *pixmapzundo;
GtkWidget *pixmapzleft, *pixmapzright;

pixmapzin=gtk_image_new_from_pixbuf(GLOBALS->zoomin_pixbuf);
gtk_widget_show(pixmapzin);
pixmapzout=gtk_image_new_from_pixbuf(GLOBALS->zoomout_pixbuf);
gtk_widget_show(pixmapzout);
pixmapzfit=gtk_image_new_from_pixbuf(GLOBALS->zoomfit_pixbuf);
gtk_widget_show(pixmapzfit);
pixmapzundo=gtk_image_new_from_pixbuf(GLOBALS->zoomundo_pixbuf);
gtk_widget_show(pixmapzundo);

pixmapzleft=gtk_image_new_from_pixbuf(GLOBALS->zoom_larrow_pixbuf);
gtk_widget_show(pixmapzleft);
pixmapzright=gtk_image_new_from_pixbuf(GLOBALS->zoom_rarrow_pixbuf);
gtk_widget_show(pixmapzright);


/* Create a table to hold the text widget and scrollbars */
table = XXX_gtk_table_new (1, 1, FALSE);

main_vbox = XXX_gtk_vbox_new (FALSE, 1);
gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Zoom ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = XXX_gtk_table_new (2, 3, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapzin);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b1, 0, 1, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b1), "clicked", G_CALLBACK(service_zoom_out), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b1, "Zoom Out");
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapzout);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b2), "clicked", G_CALLBACK(service_zoom_in), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b2, "Zoom In");
gtk_widget_show(b2);

b3 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b3), pixmapzfit);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b3, 1, 2, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b3), "clicked", G_CALLBACK(service_zoom_fit), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b3, "Zoom Best Fit");
gtk_widget_show(b3);

b4 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b4), pixmapzundo);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b4, 1, 2, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b4), "clicked", G_CALLBACK(service_zoom_undo), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b4, "Undo Last Zoom");
gtk_widget_show(b4);

b5 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b5), pixmapzleft);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b5, 2, 3, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b5), "clicked", G_CALLBACK(service_zoom_left), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b5, "Zoom To Start");
gtk_widget_show(b5);

b6 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b6), pixmapzright);
XXX_gtk_table_attach (XXX_GTK_TABLE (table2), b6, 2, 3, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
g_signal_connect_swapped (XXX_GTK_OBJECT (b6), "clicked", G_CALLBACK(service_zoom_right), XXX_GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(b6, "Zoom To End");
gtk_widget_show(b6);


gtk_container_add (GTK_CONTAINER (frame), table2);

gtk_widget_show(table2);

return table;
}

