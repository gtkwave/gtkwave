/*
 * Copyright (c) Tony Bybell 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_BASECONVERT_H
#define WAVE_BASECONVERT_H

char *convert_ascii(GwTrace *t, GwVectorEnt *v);
char *convert_ascii_vec(GwTrace *t, char *vec);
char *convert_ascii_real(GwTrace *t, double *d);
char *convert_ascii_string(char *s);
char *convert_ascii_vec_2(GwTrace *t, char *vec);
double convert_real_vec(GwTrace *t, char *vec);
double convert_real(GwTrace *t, GwVectorEnt *v);
int vtype(GwTrace *t, char *vec);
int vtype2(GwTrace *t, GwVectorEnt *v);

#endif
