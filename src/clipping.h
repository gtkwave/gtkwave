/*
 * Copyright (c) Tony Bybell 2005
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_CLIP_H
#define WAVE_CLIP_H

/* pack points into, out of an array */
#define CLIP_PACK(z, a, b, c, d) do{z[0]=a;z[1]=b;z[2]=c;z[3]=d;}while(0)
#define CLIP_UNPACK(z, a, b, c, d) do{a=z[0];b=z[1];c=z[2];d=z[3];}while(0)

/* returns true if line is visible in rectangle */
int wave_lineclip(int *coords, int *rect);

#endif

