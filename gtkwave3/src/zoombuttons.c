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
hadj->lower=GLOBALS->tims.first;
hadj->upper=GLOBALS->tims.last+2.0;

pageinc=(gfloat)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
hadj->page_size=hadj->page_increment=(pageinc>=1.0)?pageinc:1.0;

/* hadj->step_increment=(GLOBALS->nspx>=1.0)?GLOBALS->nspx:1.0; */

hadj->step_increment = pageinc / 10.0;
if(hadj->step_increment < 1.0) hadj->step_increment = 1.0;

if(hadj->page_size >= (hadj->upper-hadj->lower)) hadj->value=hadj->lower;
if(hadj->value+hadj->page_size>hadj->upper)
	{
	hadj->value=hadj->upper-hadj->page_size;
	if(hadj->value<hadj->lower)
		hadj->value=hadj->lower;
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
hadj->value=GLOBALS->tims.timecache=GLOBALS->tims.first;
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
hadj->value=GLOBALS->tims.timecache;
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
#ifdef WAVE_USE_GTK2
		" Alt + Scrollwheel Up also hits this button in non-alternative wheel mode."
#endif
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
	GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.timecache=GLOBALS->tims.start;
	}
	else
	{
	GLOBALS->tims.timecache=0;
	}

fix_wavehadj();

gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

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
#ifdef WAVE_USE_GTK2
		" Alt + Scrollwheel Down also hits this button in non-alternative wheel mode."
#endif
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
		GTK_ADJUSTMENT(GLOBALS->wave_hslider)->value=GLOBALS->tims.timecache=GLOBALS->tims.start;
		}
		else
		{
		GLOBALS->tims.timecache=0;
		}

	fix_wavehadj();

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

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

gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

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

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */
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

gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

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
        hadj->value=time1;

	calczoom(estimated);
	GLOBALS->tims.zoom=estimated;

	fix_wavehadj();

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed"); /* force zoom update */

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

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

pixmapzin=gtk_pixmap_new(GLOBALS->zoomin_pixmap, GLOBALS->zoomin_mask);
gtk_widget_show(pixmapzin);
pixmapzout=gtk_pixmap_new(GLOBALS->zoomout_pixmap, GLOBALS->zoomout_mask);
gtk_widget_show(pixmapzout);
pixmapzfit=gtk_pixmap_new(GLOBALS->zoomfit_pixmap, GLOBALS->zoomfit_mask);
gtk_widget_show(pixmapzfit);
pixmapzundo=gtk_pixmap_new(GLOBALS->zoomundo_pixmap, GLOBALS->zoomundo_mask);
gtk_widget_show(pixmapzundo);

pixmapzleft=gtk_pixmap_new(GLOBALS->zoom_larrow_pixmap, GLOBALS->zoom_larrow_mask);
gtk_widget_show(pixmapzleft);
pixmapzright=gtk_pixmap_new(GLOBALS->zoom_rarrow_pixmap, GLOBALS->zoom_rarrow_mask);
gtk_widget_show(pixmapzright);


/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 1, FALSE);

main_vbox = gtk_vbox_new (FALSE, 1);
gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Zoom ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = gtk_table_new (2, 3, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapzin);
gtk_table_attach (GTK_TABLE (table2), b1, 0, 1, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b1), "clicked", GTK_SIGNAL_FUNC(service_zoom_out), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b1, "Zoom Out", NULL);
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapzout);
gtk_table_attach (GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b2), "clicked", GTK_SIGNAL_FUNC(service_zoom_in), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b2, "Zoom In", NULL);
gtk_widget_show(b2);

b3 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b3), pixmapzfit);
gtk_table_attach (GTK_TABLE (table2), b3, 1, 2, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b3), "clicked", GTK_SIGNAL_FUNC(service_zoom_fit), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b3, "Zoom Best Fit", NULL);
gtk_widget_show(b3);

b4 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b4), pixmapzundo);
gtk_table_attach (GTK_TABLE (table2), b4, 1, 2, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b4), "clicked", GTK_SIGNAL_FUNC(service_zoom_undo), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b4, "Undo Last Zoom", NULL);
gtk_widget_show(b4);

b5 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b5), pixmapzleft);
gtk_table_attach (GTK_TABLE (table2), b5, 2, 3, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b5), "clicked", GTK_SIGNAL_FUNC(service_zoom_left), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b5, "Zoom To Start", NULL);
gtk_widget_show(b5);

b6 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b6), pixmapzright);
gtk_table_attach (GTK_TABLE (table2), b6, 2, 3, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b6), "clicked", GTK_SIGNAL_FUNC(service_zoom_right), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b6, "Zoom To End", NULL);
gtk_widget_show(b6);


gtk_container_add (GTK_CONTAINER (frame), table2);

gtk_widget_show(table2);

return table;
}

