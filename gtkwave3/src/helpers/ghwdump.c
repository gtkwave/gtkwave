/*  Display a GHDL Wavefile for debugging.
    Copyright (C) 2005-2008 Tristan Gingold

    GHDL is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2, or (at your option) any later
    version.

    GHDL is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with GCC; see the file COPYING.  If not, write to the Free
    Software Foundation, 51 Franklin Street - Suite 500, Boston, MA
    02110-1335, USA.
*/

#include <config.h>
#include <stdio.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ghwlib.h"

#include "wave_locale.h"

static const char *progname;
void
usage (void)
{
  printf ("usage: %s [OPTIONS] FILEs...\n", progname);
  printf ("Options are:\n"
	  " -t  display types\n"
	  " -h  display hierarchy\n"
	  " -T  display time\n"
	  " -s  display signals (and time)\n"
	  " -l  display list of sections\n"
	  " -v  verbose\n");
}

int
main (int argc, char **argv)
{
  int i;
  int flag_disp_types;
  int flag_disp_hierarchy;
  int flag_disp_time;
  int flag_disp_signals;
  int flag_list;
  int flag_verbose;
  int eof;
  enum ghw_sm_type sm;

  progname = argv[0];
  flag_disp_types = 0;
  flag_disp_hierarchy = 0;
  flag_disp_time = 0;
  flag_disp_signals = 0;
  flag_list = 0;
  flag_verbose = 0;

  WAVE_LOCALE_FIX

  while (1)
    {
      int c;

      c = getopt (argc, argv, "thTslv");
      if (c == -1)
	break;
      switch (c)
	{
	case 't':
	  flag_disp_types = 1;
	  break;
	case 'h':
	  flag_disp_hierarchy = 1;
	  break;
	case 'T':
	  flag_disp_time = 1;
	  break;
	case 's':
	  flag_disp_signals = 1;
	  flag_disp_time = 1;
	  break;
	case 'l':
	  flag_list = 1;
	  break;
	case 'v':
	  flag_verbose++;
	  break;
	default:
	  usage ();
	  exit (2);
	}
    }

  if (optind >= argc)
    {
      usage ();
      return 1;
    }

  for (i = optind; i < argc; i++)
    {
      struct ghw_handler h;
      struct ghw_handler *hp = &h;

      hp->flag_verbose = flag_verbose;

      if (ghw_open (hp, argv[i]) != 0)
	{
	  fprintf (stderr, "cannot open ghw file %s\n", argv[i]);
	  return 1;
	}
      if (flag_list)
	{
	  while (1)
	    {
	      int section;

	      section = ghw_read_section (hp);
	      if (section == -2)
		{
		  printf ("eof of file\n");
		  break;
		}
	      else if (section < 0)
		{
		  printf ("Error in file\n");
		  break;
		}
	      else if (section == 0)
		{
		  printf ("Unknown section\n");
		  break;
		}
	      printf ("Section %s\n", ghw_sections[section].name);
	      if ((*ghw_sections[section].handler)(hp) < 0)
		break;
	    }
	}
      else
	{
	  if (ghw_read_base (hp) < 0)
	    {
	      fprintf (stderr, "cannot read ghw file\n");
	      return 2;
	    }
	  if (0)
	    {
	      unsigned ix;
	      printf ("String table:\n");

	      for (ix = 1; ix < hp->nbr_str; ix++)
		printf (" %s\n", hp->str_table[ix]);
	    }
	  if (flag_disp_types)
	    ghw_disp_types (hp);
	  if (flag_disp_hierarchy)
	    ghw_disp_hie (hp, hp->hie);

#if 1
	  sm = ghw_sm_init;
	  eof = 0;
	  while (!eof)
	    {
	      switch (ghw_read_sm (hp, &sm))
		{
		case ghw_res_snapshot:
		case ghw_res_cycle:
		  if (flag_disp_time)
		    printf ("Time is "GHWPRI64" fs\n", hp->snap_time);
		  if (flag_disp_signals)
		    ghw_disp_values (hp);
		  break;
		case ghw_res_eof:
		  eof = 1;
		  break;
		default:
		  abort ();
		}
	    }

#else
	  if (ghw_read_dump (hp) < 0)
	    {
	      fprintf (stderr, "error in ghw dump\n");
	      return 3;
	    }
#endif
	}
      ghw_close (&h);
    }
  return 0;
}

