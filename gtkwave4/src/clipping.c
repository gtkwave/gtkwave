/*
 * Copyright (c) Tony Bybell 2005-2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#define x1 coords[0]
#define y1 coords[1]
#define x2 coords[2]
#define y2 coords[3]

#define rx1 rect[0]
#define ry1 rect[1]
#define rx2 rect[2]
#define ry2 rect[3]

int wave_lineclip(int *coords, int *rect)
{
int msk1, msk2;

/*
     these comparisons assume the bounding rectangle is set up as follows:

           rx1    rx2
     ry1   +--------+
           |        |
     ry2   +--------+
*/

msk1 = (x1<rx1);
msk1|= (x1>rx2)<<1;
msk1|= (y1<ry1)<<2;
msk1|= (y1>ry2)<<3;

msk2 = (x2<rx1);
msk2|= (x2>rx2)<<1;
msk2|= (y2<ry1)<<2;
msk2|= (y2>ry2)<<3;

if(!(msk1|msk2)) return(1); /* trivial accept, all points are inside rectangle */
if(msk1&msk2) return(0);    /* trivial reject, common x or y out of range */

if(y1==y2)
	{
	if(x1<rx1) x1 = rx1; else if(x1>rx2) x1 = rx2;
	if(x2<rx1) x2 = rx1; else if(x2>rx2) x2 = rx2;
	}
else
if(x1==x2)
	{
	if(y1<ry1) y1 = ry1; else if(y1>ry2) y1 = ry2;
	if(y2<ry1) y2 = ry1; else if(y2>ry2) y2 = ry2;
	}
else
	{
	double dx1 = x1, dy1 = y1, dx2 = x2, dy2 = y2;

	double m = (dy2-dy1)/(dx2-dx1);
	double b = dy1 - m*dx1;

	if((x1<rx1)&&(x2>=rx1)) { dx1 = rx1; dy1 = m*dx1 + b; }
	else if((x1>rx2)&&(x2<=rx2)) { dx1 = rx2; dy1 = m*dx1 + b; }

	if((y1<ry1)&&(y2>=ry1)) { dy1 = ry1; dx1 = (dy1 - b) / m; }
	else if((y1>ry2)&&(y2<=ry2)) { dy1 = ry2; dx1 = (dy1 - b) / m; }

	if((x2<rx1)&&(x1>=rx1)) { dx2 = rx1; dy2 = m*dx2 + b; }
	else if((x2>rx2)&&(x1<=rx2)) { dx2 = rx2; dy2 = m*dx2 + b; }

	if((y2<ry1)&&(y1>=ry1)) { dy2 = ry1; dx2 = (dy2 - b) / m; }
	else if((y2>ry2)&&(y1<=ry2)) { dy2 = ry2; dx2 = (dy2 - b) / m; }

	x1 = dx1; y1 = dy1;
	x2 = dx2; y2 = dy2;
	}

msk1 = (x1<rx1);
msk1|= (x1>rx2)<<1;
msk1|= (y1<ry1)<<2;
msk1|= (y1>ry2)<<3;

msk2 = (x2<rx1);
msk2|= (x2>rx2)<<1;
msk2|= (y2<ry1)<<2;
msk2|= (y2>ry2)<<3;

return(!msk1 && !msk2); /* see if points are really inside */
}

