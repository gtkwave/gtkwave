/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Wayne A. Christopher, U. C. Berkeley CAD Group

Spice is covered now covered by the BSD Copyright:

Copyright (c) 1985-1991 The Regents of the University of California.
All rights reserved.

Permission is hereby granted, without written agreement and without license
or royalty fees, to use, copy, modify, and distribute this software and its
documentation for any purpose, provided that the above copyright notice and
the following two paragraphs appear in all copies of this software.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON AN
"AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
**********/

/*
 * Polynomial interpolation code.
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef HAVE_BZERO
#define bcopy(a,b,c) memcpy((b),(a),(c))
#define bzero(a,b) memset((a),0,(b))
#define bcmp(a,b,c) memcmp((a),(b),(c))
#endif


#if 0
static void
printmat (char *name, double *mat, int m, int n)
{
  int i, j;

  printf ("\n\r=== Matrix: %s ===\n\r", name);
  for (i = 0; i < m; i++)
    {
      printf (" | ");
      for (j = 0; j < n; j++)
	printf ("%G ", mat[i * n + j]);
      printf ("|\n\r");
    }
  printf ("===\n\r");
  return;
}
#endif

static double
ft_peval (double x, double *coeffs, int degree)
{
  double y;
  int i;

  if (!coeffs)
    return 0.0;			/* XXX Should not happen */

  y = coeffs[degree];		/* there are (degree+1) coeffs */

  for (i = degree - 1; i >= 0; i--)
    {
      y *= x;
      y += coeffs[i];
    }

  return y;
}


/* Returns the index of the last element that was calculated. oval is the
 * value of the old scale at the end of the interval that is being interpolated
 * from, and sign is 1 if the old scale was increasing, and -1 if it was
 * decreasing.
 */
static int
putinterval (double *poly, int degree, double *nvec, int last, double *nscale,
	     int nlen, double oval, int sign)
{
  int end, i;

  /* See how far we have to go. */
  for (end = last + 1; end < nlen; end++)
    if (nscale[end] * sign > oval * sign)
      break;
  end--;

  for (i = last + 1; i <= end; i++)
    nvec[i] = ft_peval (nscale[i], poly, degree);
  return (end);
}


/* Takes n = (degree+1) doubles, and fills in result with the n coefficients
 * of the polynomial that will fit them. It also takes a pointer to an
 * array of n ^ 2 + n doubles to use for scratch -- we want to make this
 * fast and avoid doing mallocs for each call.
 */
static int
ft_polyfit (double *xdata, double *ydata, double *result, int degree,
	    double *scratch)
{
  double *mat1 = scratch;
  int l, k, j, i;
  int n = degree + 1;
  double *mat2 = scratch + n * n;	/* XXX These guys are hacks! */
  double d;

  bzero ((char *) result, n * sizeof (double));
  bzero ((char *) mat1, n * n * sizeof (double));
  bcopy ((char *) ydata, (char *) mat2, n * sizeof (double));

  /* Fill in the matrix with x^k for 0 <= k <= degree for each point */
  l = 0;
  for (i = 0; i < n; i++)
    {
      d = 1.0;
      for (j = 0; j < n; j++)
	{
	  mat1[l] = d;
	  d *= xdata[i];
	  l += 1;
	}
    }

  /* Do Gauss-Jordan elimination on mat1. */
  for (i = 0; i < n; i++)
    {
      int lindex;
      double largest;
      /* choose largest pivot */
      for (j = i, largest = mat1[i * n + i], lindex = i; j < n; j++)
	{
	  if (fabs (mat1[j * n + i]) > largest)
	    {
	      largest = fabs (mat1[j * n + i]);
	      lindex = j;
	    }
	}
      if (lindex != i)
	{
	  /* swap rows i and lindex */
	  for (k = 0; k < n; k++)
	    {
	      d = mat1[i * n + k];
	      mat1[i * n + k] = mat1[lindex * n + k];
	      mat1[lindex * n + k] = d;
	    }
	  d = mat2[i];
	  mat2[i] = mat2[lindex];
	  mat2[lindex] = d;
	}

      /* Make sure we have a non-zero pivot. */
      if (mat1[i * n + i] == 0.0)
	{
	  /* this should be rotated. */
	  return (0);
	}
      for (j = i + 1; j < n; j++)
	{
	  d = mat1[j * n + i] / mat1[i * n + i];
	  for (k = 0; k < n; k++)
	    mat1[j * n + k] -= d * mat1[i * n + k];
	  mat2[j] -= d * mat2[i];
	}
    }

  for (i = n - 1; i > 0; i--)
    for (j = i - 1; j >= 0; j--)
      {
	d = mat1[j * n + i] / mat1[i * n + i];
	for (k = 0; k < n; k++)
	  mat1[j * n + k] -= d * mat1[i * n + k];
	mat2[j] -= d * mat2[i];
      }

  /* Now write the stuff into the result vector. */
  for (i = 0; i < n; i++)
    {
      result[i] = mat2[i] / mat1[i * n + i];
      /* printf(stderr, "result[%d] = %G\n", i, result[i]); */
    }

#define ABS_TOL 0.001
#define REL_TOL 0.001

  /* Let's check and make sure the coefficients are ok.  If they aren't,
   * just return false.  This is not the best way to do it.
   */
  for (i = 0; i < n; i++)
    {
      d = ft_peval (xdata[i], result, degree);
      if (fabs (d - ydata[i]) > ABS_TOL)
	{
	  /*
	     fprintf(stderr,
	     "Error: polyfit: x = %le, y = %le, int = %le\n",
	     xdata[i], ydata[i], d);
	     printmat("mat1", mat1, n, n);
	     printmat("mat2", mat2, n, 1);
	   */
	  return (0);
	}
      else if (fabs (d - ydata[i]) / (fabs (d) > ABS_TOL ? fabs (d) :
				      ABS_TOL) > REL_TOL)
	{
	  /*
	     fprintf(stderr,
	     "Error: polyfit: x = %le, y = %le, int = %le\n",
	     xdata[i], ydata[i], d);
	     printmat("mat1", mat1, n, n);
	     printmat("mat2", mat2, n, 1);
	   */
	  return (0);
	}
    }

  return (1);
}


/* Interpolate data from oscale to nscale. data is assumed to be olen long,
 * ndata will be nlen long. Returns false if the scales are too strange
 * to deal with.  Note that we are guaranteed that either both scales are
 * strictly increasing or both are strictly decreasing.
 *
 * Usage: return indicates success or failure
 *
 * data[] = old y
 * oscale[] = old x
 * olen = size of above array
 *
 * ndata[] = routine fills in with new y
 * nscale[] = user fills in with new x
 * nlen = user fills in with size of above array
 *
 * note that degree > 2 will result in bumpy curves if the derivatives
 * are not smooth
 */
int
ft_interpolate (double *data, double *ndata, double *oscale, int olen,
		double *nscale, int nlen, int degree)
{
  double *result, *scratch, *xdata, *ydata;
  int sign, lastone, i, l;
  int rc;

  if ((olen < 2) || (nlen < 2))
    {
      /* fprintf(stderr, "Error: lengths too small to interpolate.\n"); */
      return (0);
    }
  if ((degree < 1) || (degree > olen))
    {
      /* fprintf(stderr, "Error: degree is %d, can't interpolate.\n",
         degree); */
      return (0);
    }

  if (oscale[1] < oscale[0])
    sign = -1;
  else
    sign = 1;

  scratch = (double *) malloc ((degree + 1) * (degree + 2) * sizeof (double));
  result = (double *) malloc ((degree + 1) * sizeof (double));
  xdata = (double *) malloc ((degree + 1) * sizeof (double));
  ydata = (double *) malloc ((degree + 1) * sizeof (double));

  /* Deal with the first degree pieces. */
  bcopy ((char *) data, (char *) ydata, (degree + 1) * sizeof (double));
  bcopy ((char *) oscale, (char *) xdata, (degree + 1) * sizeof (double));

  while (!ft_polyfit (xdata, ydata, result, degree, scratch))
    {
      /* If it doesn't work this time, bump the interpolation
       * degree down by one.
       */

      if (--degree == 0)
	{
	  /* fprintf(stderr, "ft_interpolate: Internal Error.\n"); */
	  rc = 0;
	  goto bot;
	}

    }

  /* Add this part of the curve. What we do is evaluate the polynomial
   * at those points between the last one and the one that is greatest,
   * without being greater than the leftmost old scale point, or least
   * if the scale is decreasing at the end of the interval we are looking
   * at.
   */
  lastone = -1;
  for (i = 0; i < degree; i++)
    {
      lastone = putinterval (result, degree, ndata, lastone,
			     nscale, nlen, xdata[i], sign);
    }

  /* Now plot the rest, piece by piece. l is the
   * last element under consideration.
   */
  for (l = degree + 1; l < olen; l++)
    {

      /* Shift the old stuff by one and get another value. */
      for (i = 0; i < degree; i++)
	{
	  xdata[i] = xdata[i + 1];
	  ydata[i] = ydata[i + 1];
	}
      ydata[i] = data[l];
      xdata[i] = oscale[l];

      while (!ft_polyfit (xdata, ydata, result, degree, scratch))
	{
	  if (--degree == 0)
	    {
	      /* fprintf(stderr,
	         "interpolate: Internal Error.\n"); */
	      rc = 0;
	      goto bot;
	    }
	}
      lastone = putinterval (result, degree, ndata, lastone,
			     nscale, nlen, xdata[i], sign);
    }
  if (lastone < nlen - 1)	/* ??? */
    ndata[nlen - 1] = data[olen - 1];

  rc = 1;

bot:
  free (scratch);
  free (xdata);
  free (ydata);
  free (result);
  return (rc);
}

