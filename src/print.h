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

void print_mif_image(FILE *wave, gdouble px, gdouble py);
void print_ps_image(FILE *wave, gdouble px, gdouble py);

#endif
