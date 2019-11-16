/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/* This file extracted from the GTK tutorial. */

/* radiobuttons.c */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "analyzer.h"
#include "symbol.h"
#include "debug.h"


static void toggle_generic(GtkWidget *widget, TraceFlagsType msk)
{
if(GTK_TOGGLE_BUTTON(widget)->active)
	{
	GLOBALS->flags_showchange_c_1|=msk;
	}
	else
	{
	GLOBALS->flags_showchange_c_1&=(~msk);
	}
}

static void toggle1_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

toggle_generic(widget, TR_RJUSTIFY);
}
static void toggle2_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

toggle_generic(widget, TR_INVERT);
}
static void toggle3_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

toggle_generic(widget, TR_REVERSE);
}
static void toggle4_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

toggle_generic(widget, TR_EXCLUDE);
}
static void toggle5_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)nothing;

toggle_generic(widget, TR_POPCNT);
}

static void enter_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  GLOBALS->flags_showchange_c_1=GLOBALS->flags_showchange_c_1&(~(TR_HIGHLIGHT|TR_NUMMASK));

  if(GTK_TOGGLE_BUTTON(GLOBALS->button1_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_HEX;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button2_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_DEC;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button3_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_BIN;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button4_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_OCT;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button5_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_SIGNED;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button6_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_ASCII;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button7_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=TR_REAL;
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button8_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=(TR_TIME|TR_DEC);
	}
  else
  if(GTK_TOGGLE_BUTTON(GLOBALS->button9_showchange_c_1)->active)
	{
	GLOBALS->flags_showchange_c_1|=(TR_ENUM|TR_BIN);
	}

  GLOBALS->tcache_showchange_c_1->flags=GLOBALS->flags_showchange_c_1;
  GLOBALS->tcache_showchange_c_1->minmax_valid = 0; /* force analog traces to regenerate if necessary */

  wave_gtk_grab_remove(GLOBALS->window_showchange_c_8);
  gtk_widget_destroy(GLOBALS->window_showchange_c_8);
  GLOBALS->window_showchange_c_8 = NULL;

  GLOBALS->cleanup_showchange_c_6();
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
(void)widget;
(void)nothing;

  wave_gtk_grab_remove(GLOBALS->window_showchange_c_8);
  gtk_widget_destroy(GLOBALS->window_showchange_c_8);
  GLOBALS->window_showchange_c_8 = NULL;
}


void showchange(char *title, Trptr t, GtkSignalFunc func)
{
  GtkWidget *main_vbox;
  GtkWidget *ok_hbox;
  GtkWidget *hbox;
  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *separator;
  GSList *group;
  GtkWidget *frame1, *frame2;

  GLOBALS->cleanup_showchange_c_6=func;
  GLOBALS->tcache_showchange_c_1=t;
  GLOBALS->flags_showchange_c_1=t->flags;

  /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key occurs */
  if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

  GLOBALS->window_showchange_c_8 = gtk_window_new (GLOBALS->disable_window_manager ? GTK_WINDOW_POPUP : GTK_WINDOW_TOPLEVEL);
  install_focus_cb(GLOBALS->window_showchange_c_8, ((char *)&GLOBALS->window_showchange_c_8) - ((char *)GLOBALS));

  gtkwave_signal_connect (GTK_OBJECT (GLOBALS->window_showchange_c_8), "delete_event",GTK_SIGNAL_FUNC(destroy_callback),NULL);

  gtk_window_set_title (GTK_WINDOW (GLOBALS->window_showchange_c_8), title);
  gtk_container_border_width (GTK_CONTAINER (GLOBALS->window_showchange_c_8), 0);


  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_widget_show (main_vbox);

  label=gtk_label_new(t->name);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), separator, FALSE, TRUE, 0);
  gtk_widget_show (separator);


  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbox), 5);
  gtk_widget_show (hbox);

  box2 = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box2), 5);
  gtk_widget_show (box2);

  GLOBALS->button1_showchange_c_1 = gtk_radio_button_new_with_label (NULL, "Hex");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button1_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_HEX) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button1_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button1_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button1_showchange_c_1));

  GLOBALS->button2_showchange_c_1 = gtk_radio_button_new_with_label(group, "Decimal");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button2_showchange_c_1, TRUE, TRUE, 0);
  if((GLOBALS->flags_showchange_c_1&TR_DEC) && (!(GLOBALS->flags_showchange_c_1&TR_TIME))) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button2_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button2_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button2_showchange_c_1));

  GLOBALS->button5_showchange_c_1 = gtk_radio_button_new_with_label(group, "Signed Decimal");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button5_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_SIGNED) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button5_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button5_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button5_showchange_c_1));

  GLOBALS->button3_showchange_c_1 = gtk_radio_button_new_with_label(group, "Binary");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button3_showchange_c_1, TRUE, TRUE, 0);
  if((GLOBALS->flags_showchange_c_1&TR_BIN) && (!(GLOBALS->flags_showchange_c_1&TR_ENUM))) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button3_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button3_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button3_showchange_c_1));

  GLOBALS->button4_showchange_c_1 = gtk_radio_button_new_with_label(group, "Octal");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button4_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_OCT) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button4_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button4_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button4_showchange_c_1));

  GLOBALS->button6_showchange_c_1 = gtk_radio_button_new_with_label(group, "ASCII");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button6_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_ASCII) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button6_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button6_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button6_showchange_c_1));

  GLOBALS->button7_showchange_c_1 = gtk_radio_button_new_with_label(group, "Real");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button7_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_ASCII) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button7_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button7_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button7_showchange_c_1));

  GLOBALS->button8_showchange_c_1 = gtk_radio_button_new_with_label(group, "Time");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button8_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_ASCII) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button8_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button8_showchange_c_1);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (GLOBALS->button8_showchange_c_1));

  GLOBALS->button9_showchange_c_1 = gtk_radio_button_new_with_label(group, "Enum");
  gtk_box_pack_start (GTK_BOX (box2), GLOBALS->button9_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_ASCII) gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (GLOBALS->button9_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->button9_showchange_c_1);

  frame2 = gtk_frame_new ("Base");
  gtk_container_border_width (GTK_CONTAINER (frame2), 3);
  gtk_container_add (GTK_CONTAINER (frame2), box2);
  gtk_widget_show (frame2);
  gtk_box_pack_start(GTK_BOX (hbox), frame2, TRUE, TRUE, 0);

/****************************************************************************************************/

  box1 = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box1), 5);
  gtk_widget_show (box1);


  frame1 = gtk_frame_new ("Attributes");
  gtk_container_border_width (GTK_CONTAINER (frame1), 3);
  gtk_container_add (GTK_CONTAINER (frame1), box1);
  gtk_box_pack_start(GTK_BOX (hbox), frame1, TRUE, TRUE, 0);
  gtk_widget_show (frame1);

  GLOBALS->toggle1_showchange_c_1=gtk_check_button_new_with_label("Right Justify");
  gtk_box_pack_start (GTK_BOX (box1), GLOBALS->toggle1_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_RJUSTIFY)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(GLOBALS->toggle1_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->toggle1_showchange_c_1);
  gtkwave_signal_connect (GTK_OBJECT (GLOBALS->toggle1_showchange_c_1), "toggled", GTK_SIGNAL_FUNC(toggle1_callback), NULL);

  GLOBALS->toggle2_showchange_c_1=gtk_check_button_new_with_label("Invert");
  gtk_box_pack_start (GTK_BOX (box1), GLOBALS->toggle2_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_INVERT)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(GLOBALS->toggle2_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->toggle2_showchange_c_1);
  gtkwave_signal_connect (GTK_OBJECT (GLOBALS->toggle2_showchange_c_1), "toggled", GTK_SIGNAL_FUNC(toggle2_callback), NULL);

  GLOBALS->toggle3_showchange_c_1=gtk_check_button_new_with_label("Reverse");
  gtk_box_pack_start (GTK_BOX (box1), GLOBALS->toggle3_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_REVERSE)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(GLOBALS->toggle3_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->toggle3_showchange_c_1);
  gtkwave_signal_connect (GTK_OBJECT (GLOBALS->toggle3_showchange_c_1), "toggled", GTK_SIGNAL_FUNC(toggle3_callback), NULL);

  GLOBALS->toggle4_showchange_c_1=gtk_check_button_new_with_label("Exclude");
  gtk_box_pack_start (GTK_BOX (box1), GLOBALS->toggle4_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_EXCLUDE)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(GLOBALS->toggle4_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->toggle4_showchange_c_1);
  gtkwave_signal_connect (GTK_OBJECT (GLOBALS->toggle4_showchange_c_1), "toggled", GTK_SIGNAL_FUNC(toggle4_callback), NULL);

  GLOBALS->toggle5_showchange_c_1=gtk_check_button_new_with_label("Popcnt");
  gtk_box_pack_start (GTK_BOX (box1), GLOBALS->toggle5_showchange_c_1, TRUE, TRUE, 0);
  if(GLOBALS->flags_showchange_c_1&TR_POPCNT)gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(GLOBALS->toggle5_showchange_c_1), TRUE);
  gtk_widget_show (GLOBALS->toggle5_showchange_c_1);
  gtkwave_signal_connect (GTK_OBJECT (GLOBALS->toggle5_showchange_c_1), "toggled", GTK_SIGNAL_FUNC(toggle5_callback), NULL);

  gtk_container_add (GTK_CONTAINER (main_vbox), hbox);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), separator, FALSE, TRUE, 0);
  gtk_widget_show (separator);

/****************************************************************************************************/

  ok_hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (ok_hbox), 1);
  gtk_widget_show (ok_hbox);

  button = gtk_button_new_with_label ("Cancel");
  gtkwave_signal_connect_object (GTK_OBJECT (button), "clicked",GTK_SIGNAL_FUNC(destroy_callback),GTK_OBJECT (GLOBALS->window_showchange_c_8));
  gtk_box_pack_end (GTK_BOX (ok_hbox), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  gtk_container_add (GTK_CONTAINER (main_vbox), ok_hbox);

  button = gtk_button_new_with_label ("  OK  ");
  gtkwave_signal_connect_object (GTK_OBJECT (button), "clicked",GTK_SIGNAL_FUNC(enter_callback),GTK_OBJECT (GLOBALS->window_showchange_c_8));

  gtkwave_signal_connect_object (GTK_OBJECT (button), "realize", (GtkSignalFunc) gtk_widget_grab_default, GTK_OBJECT (button));

  gtk_box_pack_end (GTK_BOX (ok_hbox), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

/****************************************************************************************************/

  gtk_container_add (GTK_CONTAINER (GLOBALS->window_showchange_c_8), main_vbox);
  gtk_widget_show (GLOBALS->window_showchange_c_8);
  wave_gtk_grab_add(GLOBALS->window_showchange_c_8);
}

