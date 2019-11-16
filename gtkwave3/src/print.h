/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

/*
 * This module has been re-implemented by Udi Finkelstein.
 * Since it is no longer a PostScript-only module, it had been
 * renamed "print.c".
 *
 * Much of the code has been "C++"-ized in style, yet written in C.
 * We use classes, virtual functions, class members, and "this" pointers
 * written in C.
 */

#ifndef WAVE_PRINT_H
#define WAVE_PRINT_H

/*************************************************************************
 * Print Context                                                         *
 *                                                                       *
 * This structure contains everything needed for the generic print code  *
 *************************************************************************/
struct _gtk_print_device;
struct _pr_context {
    struct _gtk_print_device *gpd; /* Pointer to print device class */
    FILE *handle; /* Pointer to output file */
    gdouble PageX; /* Legal page width */
    gdouble PageY; /* Legal page height */
    gdouble LM; /* Left Margin (inch) */
    gdouble RM; /* Right Margin (inch) */
    gdouble BM; /* Bottom Margin (inch) */
    gdouble TM; /* Top Margin (inch) */
    gdouble xscale, yscale, xtotal;
    gdouble tr_x, tr_y;
    gdouble gray;
    int MinX, MinY, MaxX, MaxY;  /* These will be initialized by a routine */
    char fullpage;
};
typedef struct _pr_context pr_context;

/*************************************************************************
 * GTKWave Print Device                                                  *
 *                                                                       *
 * This structure contains pointers to device specific operations        *
 *************************************************************************/
struct _gtk_print_device {
    void (*gpd_header)(pr_context *prc);
    void (*gpd_trailer)(pr_context *prc);
    void (*gpd_signal_init)(pr_context *prc);
    void (*gpd_setgray)(pr_context *prc, gdouble gray);
    void (*gpd_draw_line)(pr_context *prc, gdouble x1, gdouble y1, gdouble x2, gdouble y2);
    void (*gpd_draw_box)(pr_context *prc, gdouble x1, gdouble y1, gdouble x2, gdouble y2);
    void (*gpd_draw_string)(pr_context *prc, int x, int y, char *str, int xsize, int ysize);
};
typedef struct _gtk_print_device gtk_print_device;


void print_image(pr_context *prc);
void print_mif_image(FILE *wave, gdouble px, gdouble py);
void print_ps_image(FILE *wave, gdouble px, gdouble py);

void ps_header(pr_context * prc);
void ps_trailer(pr_context * prc);
void ps_signal_init(pr_context * prc);
void ps_setgray(pr_context * prc, gdouble gray);
void ps_draw_line(pr_context * prc, gdouble x1, gdouble y1,
	     gdouble x2, gdouble y2);
void ps_draw_box(pr_context * prc, gdouble x1, gdouble y1, gdouble x2,
	    gdouble y2);
void ps_draw_string(pr_context * prc, int x, int y, char *str,
	       int xsize, int ysize);

void mif_header(pr_context * prc);
void mif_trailer(pr_context * prc);
void mif_signal_init(pr_context * prc);
void mif_setgray(pr_context * prc, gdouble gray);
void mif_translate(pr_context * prc, gdouble x, gdouble y);
void mif_draw_line(pr_context * prc, gdouble x1, gdouble y1,
	      gdouble x2, gdouble y2);
void mif_draw_box(pr_context * prc, gdouble x1, gdouble y1,
	     gdouble x2, gdouble y2);
void mif_draw_string(pr_context * prc, int x, int y, char *str,
		int xsize, int ysize);
#endif

