/*
 * Copyright (c) Tony Bybell 2008-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "globals.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if WAVE_USE_GTK2
#include <glib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "gtk12compat.h"
#include "analyzer.h"
#include "tree.h"
#include "symbol.h"
#include "vcd.h"
#include "lx2.h"
#include "busy.h"
#include "debug.h"
#include "hierpack.h"
#include "menu.h"
#include "tcl_helper.h"
#include "tcl_support_commands.h"

#if !defined __MINGW32__ && !defined _MSC_VER
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(HAVE_LIBTCL)
#include <tcl.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* XXX functions for embedding TCL interpreter XXX */
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

#if defined(HAVE_LIBTCL)

static int gtkwavetcl_badNumArgs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int expected)
{
(void)clientData;

Tcl_Obj *aobj;
char reportString[1024];

sprintf(reportString, "* wrong number of arguments for '%s': %d expected, %d encountered", Tcl_GetString(objv[0]), expected, objc-1);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);
return(TCL_ERROR);
}

static int gtkwavetcl_nop(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
(void)clientData;
(void)interp;
(void)objc;
(void)objv;

/* nothing, this is simply to call gtk's main loop */
gtkwave_main_iteration();
return(TCL_OK);
}

static int gtkwavetcl_printInteger(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int intVal)
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;
char reportString[33];

sprintf(reportString, "%d", intVal);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printTimeType(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], TimeType ttVal)
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;
char reportString[65];

sprintf(reportString, TTFormat, ttVal);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printTraceFlagsType(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], TraceFlagsType ttVal)
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;
char reportString[65];

sprintf(reportString, "%"TRACEFLAGSPRIuFMT, ttVal);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printDouble(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], double dVal)
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;
char reportString[65];

sprintf(reportString, "%e", dVal);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_printString(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], const char *reportString)
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static char *extractFullTraceName(Trptr t)
{
char *name = NULL;

 if(HasWave(t))
   {
     if (HasAlias(t))
       {
	 name = strdup_2(t->name_full);
       }
     else if (t->vector)
       {
	 name = strdup_2(t->n.vec->bvname);
       }
     else
       {
	 int flagged = HIER_DEPACK_ALLOC;

	 name = hier_decompress_flagged(t->n.nd->nname, &flagged);
	 if(!flagged)
	   {
	     name = strdup_2(name);
	   }
       }
   }
 return(name);
}


/* tcl interface functions */

char *get_Tcl_string(Tcl_Obj *obj) {
  char *s = Tcl_GetString(obj) ;
  if (*s == '{') {		/* braced string */
    char *p = strrchr(s, '}') ;
    if(p) {
      if(GLOBALS->previous_braced_tcl_string)
		{
		free_2(GLOBALS->previous_braced_tcl_string);
		}

      GLOBALS->previous_braced_tcl_string = strdup_2(s);
      GLOBALS->previous_braced_tcl_string[p-s] = 0;
      s = GLOBALS->previous_braced_tcl_string + 1;
    }
  }
  return s ;
}

static int gtkwavetcl_getNumFacs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->numfacs;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getLongestName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->longestname;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getFacName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;

if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->numfacs))
		{
		int was_packed = HIER_DEPACK_ALLOC;
        	char *hfacname = NULL;

        	hfacname = hier_decompress_flagged(GLOBALS->facs[which]->name, &was_packed);

		aobj = Tcl_NewStringObj(hfacname, -1);
		Tcl_SetObjResult(interp, aobj);
		if(was_packed) free_2(hfacname);
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getFacDir(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;

if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->numfacs))
		{
		WAVE_NODEVARDIR_STR
		int vardir = GLOBALS->facs[which]->n->vardir; /* two bit already chops down to 0..3, but this doesn't hurt */
	        if((vardir < 0) || (vardir > ND_DIR_MAX))
	                {
	                vardir = 0;
	                }

		aobj = Tcl_NewStringObj(vardir_strings[vardir], -1);
		Tcl_SetObjResult(interp, aobj);
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


#ifndef WAVE_USE_GTK2
/* truncate VHDL types to string directly after final '.' */
char *varxt_fix(char *s)
{
char *pnt = strrchr(s, '.');
return(pnt ? (pnt+1) : s);
}
#endif

static int gtkwavetcl_getFacVtype(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;

if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->numfacs))
		{
		WAVE_NODEVARTYPE_STR
		WAVE_NODEVARDATATYPE_STR
	        unsigned int varxt;
	        char *varxt_pnt;
	        int vartype;
	        int vardt;

	        varxt = GLOBALS->facs[which]->n->varxt;
	        varxt_pnt = varxt ? varxt_fix(GLOBALS->subvar_pnt[varxt]) : NULL;

	        vartype = GLOBALS->facs[which]->n->vartype;
	        if((vartype < 0) || (vartype > ND_VARTYPE_MAX))
	                {
	                vartype = 0;
	                }
	
	        vardt = GLOBALS->facs[which]->n->vardt;
	        if((vardt < 0) || (vardt > ND_VDT_MAX))
	                {
	                vardt = 0;
	                }

                aobj = Tcl_NewStringObj( (((GLOBALS->supplemental_datatypes_encountered) && (!GLOBALS->supplemental_vartypes_encountered)) ?
                                                        (varxt ? varxt_pnt : vardatatype_strings[vardt]) : vartype_strings[vartype]), -1);
		Tcl_SetObjResult(interp, aobj);
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getFacDtype(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
Tcl_Obj *aobj;

if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->numfacs))
		{
		WAVE_NODEVARTYPE_STR
		WAVE_NODEVARDATATYPE_STR
	        unsigned int varxt;
	        char *varxt_pnt;
	        int vardt;

	        varxt = GLOBALS->facs[which]->n->varxt;
	        varxt_pnt = varxt ? varxt_fix(GLOBALS->subvar_pnt[varxt]) : NULL;

	        vardt = GLOBALS->facs[which]->n->vardt;
	        if((vardt < 0) || (vardt > ND_VDT_MAX))
	                {
	                vardt = 0;
	                }

                aobj = Tcl_NewStringObj( varxt ? varxt_pnt : vardatatype_strings[vardt], -1);
		Tcl_SetObjResult(interp, aobj);
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getMinTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->min_time;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getMaxTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->max_time;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTimeZero(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->global_time_offset;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTimeDimension(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;
char reportString[2];

reportString[0] = GLOBALS->time_dimension;
reportString[1] = 0;

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_getArgv(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
(void)clientData;
(void)objc;
(void)objv;

if(GLOBALS->argvlist)
	{
	Tcl_Obj *aobj = Tcl_NewStringObj(GLOBALS->argvlist, -1);
	Tcl_SetObjResult(interp, aobj);
	}

return(TCL_OK);
}

static int gtkwavetcl_getBaselineMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.baseline;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.marker;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getWindowStartTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.start;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getWindowEndTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
TimeType value = GLOBALS->tims.end;
return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getDumpType(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
(void)clientData;
(void)objc;
(void)objv;

Tcl_Obj *aobj;
char *reportString = "UNKNOWN";

if(GLOBALS->is_vcd)
        {
        if(GLOBALS->partial_vcd)
                {
                reportString = "PVCD";
                }
                else
                {
                reportString = "VCD";
                }
        }
else
if(GLOBALS->is_lxt)
        {
	reportString = "LXT";
        }
else
if(GLOBALS->is_ghw)
        {
	reportString = "GHW";
        }
else
if(GLOBALS->is_lx2)
        {
        switch(GLOBALS->is_lx2)
                {
                case LXT2_IS_LXT2: reportString = "LXT2"; break;
                case LXT2_IS_AET2: reportString = "AET2"; break;
                case LXT2_IS_VZT:  reportString = "VZT"; break;
                case LXT2_IS_VLIST:reportString = "VCD"; break;
                case LXT2_IS_FST:  reportString = "FST"; break;
                case LXT2_IS_FSDB:  reportString = "FSDB"; break;
                }
        }

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}


static int gtkwavetcl_getNamedMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = -1;

	if((s[0]>='A')&&(s[0]<='Z'))
		{
		which = bijective_marker_id_string_hash(s);
		}
	else
	if((s[0]>='a')&&(s[0]<='z'))
		{
		which = bijective_marker_id_string_hash(s);
		}
	else
		{
		which = atoi(s); 
		}

	if((which >= 0) && (which < WAVE_NUM_NAMED_MARKERS))
		{
		TimeType value = GLOBALS->named_markers[which];
		return(gtkwavetcl_printTimeType(clientData, interp, objc, objv, value));
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getWaveHeight(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->waveheight;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getWaveWidth(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->wavewidth;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getPixelsUnitTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
double value = GLOBALS->pxns;
return(gtkwavetcl_printDouble(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getUnitTimePixels(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
double value = GLOBALS->nspx;
return(gtkwavetcl_printDouble(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getZoomFactor(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
double value = GLOBALS->tims.zoom;
return(gtkwavetcl_printDouble(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getDumpFileName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char *value = GLOBALS->loaded_file_name;
return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getVisibleNumTraces(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->traces.visible;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTotalNumTraces(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->traces.total;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getTraceNameFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				if(t->name)
					{
					return(gtkwavetcl_printString(clientData, interp, objc, objv, t->name));
					}
					else
					{
					break;
					}
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
        }

return(TCL_OK);
}

static int gtkwavetcl_getTraceFlagsFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				return(gtkwavetcl_printTraceFlagsType(clientData, interp, objc, objv, t->flags));
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getTraceValueAtMarkerFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				if(t->asciivalue)
					{
					char *pnt = t->asciivalue;
					if(*pnt == '=') pnt++;

					return(gtkwavetcl_printString(clientData, interp, objc, objv, pnt));
					}
					else
					{
					break;
					}
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getTraceValueAtMarkerFromName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	Trptr t = GLOBALS->traces.first;

	while(t)
		{
		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			char *name = extractFullTraceName(t);
			if(!strcmp(name, s))
				{
				free_2(name);
				break;
				}
			free_2(name);
			}
		t = t-> t_next;
		}

	if(t)
		{
		if(t->asciivalue)
			{
			char *pnt = t->asciivalue;
			if(*pnt == '=') pnt++;
			return(gtkwavetcl_printString(clientData, interp, objc, objv, pnt));
			}
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_getTraceValueAtNamedMarkerFromName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
	{
	char *sv = get_Tcl_string(objv[1]);
	int which = -1;
	TimeType oldmarker = GLOBALS->tims.marker;
	TimeType value = LLDescriptor(-1);

	if((sv[0]>='A')&&(sv[0]<='Z'))
		{
		which = bijective_marker_id_string_hash(sv);
		}
	else
	if((sv[0]>='a')&&(sv[0]<='z'))
		{
		which = bijective_marker_id_string_hash(sv);
		}
	else
		{
		which = atoi(sv);
		}

	if((which >= 0) && (which < WAVE_NUM_NAMED_MARKERS))
		{
		char *s = get_Tcl_string(objv[2]);
		Trptr t = GLOBALS->traces.first;

		value = GLOBALS->named_markers[which];

		while(t)
			{
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(!strcmp(name, s))
					{
					free_2(name);
					break;
					}
				free_2(name);
				}
			t = t-> t_next;
			}

		if(t && (value >= LLDescriptor(0)))
			{
			GLOBALS->tims.marker = value;
		        GLOBALS->signalwindow_width_dirty=1;
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();

			if(t->asciivalue)
				{
				Tcl_Obj *aobj;
				char *pnt = t->asciivalue;
				if(*pnt == '=') pnt++;

				aobj = Tcl_NewStringObj(pnt, -1);
				Tcl_SetObjResult(interp, aobj);

				GLOBALS->tims.marker = oldmarker;
				update_markertime(GLOBALS->tims.marker);
			        GLOBALS->signalwindow_width_dirty=1;
			        MaxSignalLength();
			        signalarea_configure_event(GLOBALS->signalarea, NULL);
			        wavearea_configure_event(GLOBALS->wavearea, NULL);
				gtkwave_main_iteration();

				return(TCL_OK);
				}

			GLOBALS->tims.marker = oldmarker;
			update_markertime(GLOBALS->tims.marker);
		        GLOBALS->signalwindow_width_dirty=1;
		        MaxSignalLength();
		        signalarea_configure_event(GLOBALS->signalarea, NULL);
		        wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
			}
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_getHierMaxLevel(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->hier_max_level;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getFontHeight(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->fontheight;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getLeftJustifySigs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = (GLOBALS->left_justify_sigs != 0);
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}

static int gtkwavetcl_getSaveFileName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char *value = GLOBALS->filesel_writesave;
if(value)
	{
	return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
	}

return(TCL_OK);
}

static int gtkwavetcl_getStemsFileName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char *value = GLOBALS->stems_name;
if(value)
	{
	return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
	}

return(TCL_OK);
}

static int gtkwavetcl_getTraceScrollbarRowValue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider);
int value = (int)wadj->value;

return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}



static int gtkwavetcl_setMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        TimeType mrk = unformat_time(s, GLOBALS->time_dimension);

	if((mrk >= GLOBALS->min_time) && (mrk <= GLOBALS->max_time))
		{
		GLOBALS->tims.marker = mrk;
		}
		else
		{
		GLOBALS->tims.marker = LLDescriptor(-1);
		}

        update_markertime(GLOBALS->tims.marker);
        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_setBaselineMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        TimeType mrk = unformat_time(s, GLOBALS->time_dimension);

	if((mrk >= GLOBALS->min_time) && (mrk <= GLOBALS->max_time))
		{
		GLOBALS->tims.baseline = mrk;
		}
		else
		{
		GLOBALS->tims.baseline = LLDescriptor(-1);
		}

        update_markertime(GLOBALS->tims.marker);
        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_setWindowStartTime(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);

	if(s)
	        {
	        TimeType gt;
	        char timval[40];
	        GtkAdjustment *hadj;
	        TimeType pageinc;

	        gt=unformat_time(s, GLOBALS->time_dimension);

	        if(gt<GLOBALS->tims.first) gt=GLOBALS->tims.first;
	        else if(gt>GLOBALS->tims.last) gt=GLOBALS->tims.last;

	        hadj=GTK_ADJUSTMENT(GLOBALS->wave_hslider);
	        hadj->value=gt;

	        pageinc=(TimeType)(((gdouble)GLOBALS->wavewidth)*GLOBALS->nspx);
	        if(gt<(GLOBALS->tims.last-pageinc+1))
	                GLOBALS->tims.timecache=gt;
	                else
	                {
	                GLOBALS->tims.timecache=GLOBALS->tims.last-pageinc+1;
	                if(GLOBALS->tims.timecache<GLOBALS->tims.first) GLOBALS->tims.timecache=GLOBALS->tims.first;
	                }

	        reformat_time(timval,GLOBALS->tims.timecache,GLOBALS->time_dimension);

	        time_update();
	        }

        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);
	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setZoomFactor(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        float f;

        sscanf(s, "%f", &f);
        if(f>0.0)
                {
                f=0.0; /* in case they try to go out of range */
                }
        else
        if(f<-62.0)
                {
                f=-62.0; /* in case they try to go out of range */
                }

        GLOBALS->tims.prevzoom=GLOBALS->tims.zoom;
        GLOBALS->tims.zoom=(gdouble)f;
        calczoom(GLOBALS->tims.zoom);
        fix_wavehadj();

        gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "changed");
        gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(GLOBALS->wave_hslider)), "value_changed");

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setZoomRangeTimes(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
        {
        char *s, *t;
	TimeType time1, time2;
	TimeType oldmarker = GLOBALS->tims.marker;

	s = get_Tcl_string(objv[1]);
	time1 = unformat_time(s, GLOBALS->time_dimension);
        t = get_Tcl_string(objv[2]);
	time2 = unformat_time(t, GLOBALS->time_dimension);

	if(time1 < GLOBALS->tims.first) { time1 = GLOBALS->tims.first; }
	if(time1 > GLOBALS->tims.last)  { time1 = GLOBALS->tims.last; }
	if(time2 < GLOBALS->tims.first) { time2 = GLOBALS->tims.first; }
	if(time2 > GLOBALS->tims.last)  { time2 = GLOBALS->tims.last; }

	service_dragzoom(time1, time2);
	GLOBALS->tims.marker = oldmarker;

        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setLeftJustifySigs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        TimeType val = atoi_64(s);
	GLOBALS->left_justify_sigs = (val != LLDescriptor(0)) ? ~0 : 0;

        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_setNamedMarker(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if((objc == 3)||(objc == 4))
        {
        char *s = get_Tcl_string(objv[1]);
	int which = -1;

        if((s[0]>='A')&&(s[0]<='Z'))
                {
		which = bijective_marker_id_string_hash(s);
                }
        else
        if((s[0]>='a')&&(s[0]<='z'))
                {
		which = bijective_marker_id_string_hash(s);
                }
	else
		{
	        which = atoi(s);
		}

        if((which >= 0) && (which < WAVE_NUM_NAMED_MARKERS))
                {
	        char *t = get_Tcl_string(objv[2]);
		TimeType gt=unformat_time(t, GLOBALS->time_dimension);

                GLOBALS->named_markers[which] = gt;

		if(GLOBALS->marker_names[which])
			{
			free_2(GLOBALS->marker_names[which]);
			GLOBALS->marker_names[which] = NULL;
			}

		if(objc == 4)
			{
			char *u = get_Tcl_string(objv[3]);

			GLOBALS->marker_names[which] = strdup_2(u);
			}

	        wavearea_configure_event(GLOBALS->wavearea, NULL);
		gtkwave_main_iteration();
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
        }

return(TCL_OK);
}


static int gtkwavetcl_setTraceScrollbarRowValue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        int target = atoi(s);

 	SetTraceScrollbarRowValue(target, 0);

/*         GtkAdjustment *wadj=GTK_ADJUSTMENT(GLOBALS->wave_vslider); */

/*         int num_traces_displayable=(GLOBALS->signalarea->allocation.height)/(GLOBALS->fontheight); */
/*         num_traces_displayable--;   /\* for the time trace that is always there *\/ */

/* 	if(target > GLOBALS->traces.visible - num_traces_displayable) target = GLOBALS->traces.visible - num_traces_displayable; */

/* 	if(target < 0) target = 0; */

/* 	wadj->value = target; */

/*         gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed"); /\* force bar update *\/ */
/*         gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /\* force text update *\/ */
/*	gtkwave_main_iteration(); */
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_addCommentTracesFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
        {
	char *s = Tcl_GetString(objv[1]);
        char** elem = NULL;
        int i, l = 0;

        elem = zSplitTclList(s, &l);
        if(elem)
                {
                for(i=0;i<l;i++)
                        {
                        InsertBlankTrace(elem[i], 0);
                        }
                free_2(elem);
                }

        GLOBALS->signalwindow_width_dirty=1;
        MaxSignalLength();
        signalarea_configure_event(GLOBALS->signalarea, NULL);
        wavearea_configure_event(GLOBALS->wavearea, NULL);

        sprintf(reportString, "%d", l);

        aobj = Tcl_NewStringObj(reportString, -1);
        Tcl_SetObjResult(interp, aobj);
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_addSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
char *one_entry = NULL, *mult_entry = NULL;
unsigned int mult_len = 0;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]); /* do not want to remove braces! */
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);

	if(elem)
        	{
		for(i=0;i<l;i++)
			{
			one_entry = make_single_tcl_list_name(elem[i], NULL, 0, 1); /* keep range */
			WAVE_OE_ME
			}
                free_2(elem);
                elem = NULL;
		if(mult_entry)
			{
			num_found = process_tcl_list(mult_entry, FALSE);
			free_2(mult_entry);
			}
		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
        		}
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_processTclList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]); /* do not want to remove braces! */
	if(s)
        	{
		num_found = process_tcl_list(s, FALSE);
		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
        		}
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_deleteSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]); /* do not want to remove braces */
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);

	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			t->cached_flags = t->flags;
			t->flags &= (~TR_HIGHLIGHT);
			t = t->t_next;
			}

		for(i=0;i<l;i++)
			{
			t = GLOBALS->traces.first;
			while(t)
				{
				if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH|TR_HIGHLIGHT)))
					{
					char *name = extractFullTraceName(t);
					if(name)
						{
						int len_name = strlen(name);
						int len_elem = strlen(elem[i]);
						int brackmatch = (len_name > len_elem) && (name[len_elem] == '[');

						if(((len_name == len_elem) && (!strcmp(name, elem[i])))
							|| (brackmatch && !strncmp(name, elem[i], len_elem)))
							{
							t->flags |= TR_HIGHLIGHT;
							num_found++;
							break;
							}
						free_2(name);
						}
					}
				t = t->t_next;
				}
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
			CutBuffer();
			}

		t = GLOBALS->traces.first;
		while(t)
			{
			t->flags = t->cached_flags;
			t->cached_flags = 0;
			t = t-> t_next;
			}

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
        		}
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_deleteSignalsFromListIncludingDuplicates(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]); /* do not want to remove braces */
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);

	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			t->cached_flags = t->flags;
			t->flags &= (~TR_HIGHLIGHT);

			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(name)
					{
					for(i=0;i<l;i++)
						{
						int len_name = strlen(name);
						int len_elem = strlen(elem[i]);
						int brackmatch = (len_name > len_elem) && (name[len_elem] == '[');

						if(((len_name == len_elem) && (!strcmp(name, elem[i])))
							|| (brackmatch && !strncmp(name, elem[i], len_elem)))
							{
							t->flags |= TR_HIGHLIGHT;
							num_found++;
							break;
							}
						}
					free_2(name);
					}
				}

			t = t-> t_next;
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
			CutBuffer();
			}

		t = GLOBALS->traces.first;
		while(t)
			{
			t->flags = t->cached_flags;
			t->cached_flags = 0;
			t = t-> t_next;
			}

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
        		}
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_highlightSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]); /* do not want to remove braces */
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);

	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(name)
					{
					for(i=0;i<l;i++)
						{
						if(!strcmp(name, elem[i]))
							{
							t->flags |= TR_HIGHLIGHT;
							num_found++;
							break;
							}
						}
					free_2(name);
					}
				}

			t = t-> t_next;
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
        		}
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}

static int gtkwavetcl_unhighlightSignalsFromList(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int i;
int num_found = 0;
char reportString[33];
Tcl_Obj *aobj;

if(objc==2)
	{
        char *s = Tcl_GetString(objv[1]); /* do not want to remove braces */
	char** elem = NULL;
	int l = 0;

	elem = zSplitTclList(s, &l);

	if(elem)
        	{
		Trptr t = GLOBALS->traces.first;
		while(t)
			{
			if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
				{
				char *name = extractFullTraceName(t);
				if(name)
					{
					for(i=0;i<l;i++)
						{
						if(!strcmp(name, elem[i]))
							{
							t->flags &= (~TR_HIGHLIGHT);
							num_found++;
							break;
							}
						}
					free_2(name);
					}
				}

			t = t-> t_next;
			}

                free_2(elem);
                elem = NULL;

		if(num_found)
        		{
        		MaxSignalLength();
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
        		wavearea_configure_event(GLOBALS->wavearea, NULL);
			gtkwave_main_iteration();
        		}
                }
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

sprintf(reportString, "%d", num_found);

aobj = Tcl_NewStringObj(reportString, -1);
Tcl_SetObjResult(interp, aobj);

return(TCL_OK);
}


static int gtkwavetcl_setTraceHighlightFromIndex(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);
	char *ts = get_Tcl_string(objv[2]);
	int onoff = atoi_64(ts);

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(i == which)
				{
				if(onoff)
					{
					t->flags |= TR_HIGHLIGHT;
					}
					else
					{
					t->flags &= (~TR_HIGHLIGHT);
					}
	        		signalarea_configure_event(GLOBALS->signalarea, NULL);
				gtkwave_main_iteration();
				break;
				}

			i++;
			t = t->t_next;
			}
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
        }

return(TCL_OK);
}

static int gtkwavetcl_setTraceHighlightFromNameMatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 3)
	{
	char *s = get_Tcl_string(objv[1]);
	int which = atoi(s);
	char *ts = get_Tcl_string(objv[2]);
	int onoff = atoi_64(ts);
	int mat = 0;

	if((which >= 0) && (which < GLOBALS->traces.total))
		{
		Trptr t = GLOBALS->traces.first;
		int i = 0;
		while(t)
			{
			if(t->name && !strcmp(t->name, s))
				{
				if(onoff)
					{
					t->flags |= TR_HIGHLIGHT;
					}
					else
					{
					t->flags &= (~TR_HIGHLIGHT);
					}
				mat++;
				}

			i++;
			t = t->t_next;
			}

		if(mat)
			{
        		signalarea_configure_event(GLOBALS->signalarea, NULL);
			gtkwave_main_iteration();
			}

		return(gtkwavetcl_printInteger(clientData, interp, objc, objv, mat));
		}
	}
	else
	{
	return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
	}

return(TCL_OK);
}


static int gtkwavetcl_signalChangeList(ClientData clientData, Tcl_Interp *interp,
				       int objc, Tcl_Obj *CONST objv[]) {
(void) clientData;

    int dir = STRACE_FORWARD ;
    TimeType start_time = 0 ;
    TimeType end_time = MAX_HISTENT_TIME ;
    int max_elements = 0x7fffffff ;
    char *sig_name = NULL ;
    int i ;
    char *str_p, *str1_p ;
    int error = 0 ;
    llist_p *l_head ;
    Tcl_Obj *l_obj ;
    Tcl_Obj *obj ;
    llist_p *p, *p1 ;

    for(i=1; i<objc; i++) {
      str_p = Tcl_GetStringFromObj(objv[i], NULL) ;
      if (*str_p != '-') {
	sig_name = str_p ;
      } else {
        if(i == (objc-1)) { /* loop overflow on i check */
	  error++;
	  break;
	  }
	str1_p = Tcl_GetStringFromObj(objv[++i], NULL) ;
	switch(str_p[1]) {
	case 's': /* start time */
	  if(!strstr("-start_time", str_p))
	    error++ ;
	  else {
	    if((start_time = atoi_64(str1_p)) < 0)
	      start_time = 0 ;
	    dir = (start_time > end_time) ? STRACE_BACKWARD : STRACE_FORWARD ;
	  }
	  break ;
	case 'e': /* end time */
	  if(!strstr("-end_time", str_p))
	    error++ ;
	  else {
	    end_time = atoi_64(str1_p) ;
	    dir = (start_time > end_time) ? STRACE_BACKWARD : STRACE_FORWARD ;
	  }
	  break ;
	case 'm': /* max */
	  if(!strstr("-max", str_p))
	    error++ ;
	  else {
	    max_elements = atoi(str1_p) ;
	  }
	  break ;
	case 'd': /* dir */
	  if(!strstr("-dir", str_p))
	    error++ ;
	  else {
	    if(strstr("forward", str1_p))
	      dir = STRACE_FORWARD ;
	    else
	      if(strstr("backward", str1_p))
		dir = STRACE_BACKWARD ;
	      else
		error++ ;
	  }
	  break ;
	default:
	  error++ ;
	}
      }
    }
    /* consistancy check */
    if(dir == STRACE_FORWARD) {
      if(start_time > end_time) error++ ;
    } else {
      if(start_time < end_time) {
	if(end_time == MAX_HISTENT_TIME)
	  end_time = 0 ;
	else
	  error ++ ;
      }
    }
    if(error) {
      Tcl_SetObjResult
	(interp,
	 Tcl_NewStringObj("Usage: signal_change_list ?name? ?-start time? ?-end time? ?-max size? ?-dir forward|backward?", -1)) ;
      return TCL_ERROR;
    }
    l_head = signal_change_list(sig_name, dir, start_time, end_time, max_elements) ;
    l_obj = Tcl_NewListObj(0, NULL) ;
    p = l_head;
    while(p) {
      obj = Tcl_NewWideIntObj((Tcl_WideInt )p->u.tt) ;
      p1= p->next ;
      free_2(p) ;
      Tcl_ListObjAppendElement(interp, l_obj, obj) ;
      obj = Tcl_NewStringObj(p1->u.str,-1) ;
      Tcl_ListObjAppendElement(interp, l_obj, obj) ;
      p = p1->next ;
      free_2(p1->u.str) ;
      free_2(p1) ;
    }
    Tcl_SetObjResult(interp, l_obj) ;
    return TCL_OK ;
}

static int gtkwavetcl_findNextEdge(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
edge_search(STRACE_FORWARD);
gtkwave_main_iteration();
return(gtkwavetcl_getMarker(clientData, interp, objc, objv));
}


static int gtkwavetcl_findPrevEdge(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
edge_search(STRACE_BACKWARD);
gtkwave_main_iteration();
return(gtkwavetcl_getMarker(clientData, interp, objc, objv));
}


int SST_open_node(char *name) ;
static int gtkwavetcl_forceOpenTreeNode(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  int rv = -100;		/* Tree does not exist */
  char *s = NULL ;
  if(objc == 2)
    s = get_Tcl_string(objv[1]);

  if(s && (strlen(s) > 1)) {	/* exclude empty strings */
    int len = strlen(s);
    if(s[len-1]!=GLOBALS->hier_delimeter)
      {
#ifdef WAVE_USE_GTK2
	rv = SST_open_node(s);
#endif
      }
    else {
#ifdef WAVE_USE_GTK2
      rv = SST_open_node(s);
#endif
    }
  } else {
    if (GLOBALS->selected_hierarchy_name) {
      rv = SST_NODE_CURRENT ;
    }
    gtkwave_main_iteration(); /* check if this is needed */
  }
  if (rv == -100) {
    return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
  }
  Tcl_SetObjResult(GLOBALS->interp, (rv == SST_NODE_CURRENT) ?
		   Tcl_NewStringObj(GLOBALS->selected_hierarchy_name,
				    strlen(GLOBALS->selected_hierarchy_name)) :
		   Tcl_NewIntObj(rv)) ;

  return(TCL_OK);
}

static int gtkwavetcl_setFromEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);

	if(s)
		{
		gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),s);
		from_entry_callback(NULL, GLOBALS->from_entry);
		}

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_setToEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);

	if(s)
		{
		gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),s);
		to_entry_callback(NULL, GLOBALS->to_entry);
		}

	gtkwave_main_iteration();
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}


static int gtkwavetcl_getFromEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
const char *value = gtk_entry_get_text(GTK_ENTRY(GLOBALS->from_entry));
if(value)
        {
        return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
        }

return(TCL_OK);
}


static int gtkwavetcl_getToEntry(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
const char *value = gtk_entry_get_text(GTK_ENTRY(GLOBALS->to_entry));
if(value)
        {
        return(gtkwavetcl_printString(clientData, interp, objc, objv, value));
        }

return(TCL_OK);
}


static int gtkwavetcl_getDisplayedSignals(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 1)
        {
	char *rv = add_traces_from_signal_window(TRUE);
	int rc = gtkwavetcl_printString(clientData, interp, objc, objv, rv);

	free_2(rv);
	return(rc);
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }
}


static int gtkwavetcl_getTraceFlagsFromName(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
	{
	char *s = get_Tcl_string(objv[1]);
	Trptr t = GLOBALS->traces.first;

	while(t)
		{
		if(!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
			{
			char *name = extractFullTraceName(t);
			if(!strcmp(name, s))
				{
				free_2(name);
				break;
				}
			free_2(name);
			}
		t = t-> t_next;
		}

	if(t)
		{
		return(gtkwavetcl_printTraceFlagsType(clientData, interp, objc, objv, t->flags));
		}
	}
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

return(TCL_OK);
}

static int gtkwavetcl_loadFile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if(objc == 2)
    {
      char *s = get_Tcl_string(objv[1]);

      if(!GLOBALS->in_tcl_callback)
        {
	/* wave_gconf_client_set_string("/current/savefile", s); */
        /*	read_save_helper(s, NULL, NULL, NULL, NULL); */
        process_url_file(s);
        /*	process_url_list(s); */
        /*	gtkwave_main_iteration(); */
        }
	else
	{
	gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"gtkwave::loadFile prohibited in callback",WAVE_TCLCB_ERROR_FLAGS);
	}
    }
  else
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
    }

  return(TCL_OK);
}

static int gtkwavetcl_reLoadFile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{

  if(objc == 1)
    {
      if(!GLOBALS->in_tcl_callback)
	{
      	reload_into_new_context();
	}
	else
	{
	gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"gtkwave::reLoadFile prohibited in callback",WAVE_TCLCB_ERROR_FLAGS);
	}
    }
  else
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 0));
    }
  return(TCL_OK);
}

static int gtkwavetcl_presentWindow(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{

  if(objc == 1)
    {
#ifdef WAVE_USE_GTK2
      gtk_window_present(GTK_WINDOW(GLOBALS->mainwindow));
#else
      gdk_window_raise(GTK_WIDGET(GLOBALS->mainwindow)->window);
#endif
    }
  else
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 0));
    }
  return(TCL_OK);
}

static int gtkwavetcl_showSignal(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
  if(objc == 3)
    {
      char *s0 = get_Tcl_string(objv[1]);
      char *s1;
      int row;
      unsigned location;

      sscanf(s0, "%d", &row);
      if (row < 0) { row = 0; };

      s1 = get_Tcl_string(objv[2]);
      sscanf(s1, "%u", &location);

      SetTraceScrollbarRowValue(row, location);
    }
  else
    {
      return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 2));
    }

  return(TCL_OK);
}


/*
 * swap to a given context based on tab number (from Tcl)
 */
static gint switch_to_tab_number(unsigned int i)
{
if(i < GLOBALS->num_notebook_pages)
        {
        struct Global *g_old = GLOBALS;
        /* printf("Switching to: %d\n", i); */

        set_GLOBALS((*GLOBALS->contexts)[i]);

        GLOBALS->lxt_clock_compress_to_z = g_old->lxt_clock_compress_to_z;
        GLOBALS->autoname_bundles = g_old->autoname_bundles;
        GLOBALS->autocoalesce_reversal = g_old->autocoalesce_reversal;
        GLOBALS->autocoalesce = g_old->autocoalesce;
        GLOBALS->hier_grouping = g_old->hier_grouping;
        GLOBALS->wave_scrolling = g_old->wave_scrolling;
        GLOBALS->constant_marker_update = g_old->constant_marker_update;
        GLOBALS->do_zoom_center = g_old->do_zoom_center;
        GLOBALS->use_roundcaps = g_old->use_roundcaps;
        GLOBALS->do_resize_signals = g_old->do_resize_signals;
        GLOBALS->alt_wheel_mode = g_old->alt_wheel_mode;
	GLOBALS->initial_signal_window_width = g_old->initial_signal_window_width;
        GLOBALS->use_full_precision = g_old->use_full_precision;
        GLOBALS->show_base = g_old->show_base;
        GLOBALS->display_grid = g_old->display_grid;
        GLOBALS->highlight_wavewindow = g_old->highlight_wavewindow;
        GLOBALS->disable_mouseover = g_old->disable_mouseover;
        GLOBALS->zoom_pow10_snap = g_old->zoom_pow10_snap;

        GLOBALS->scale_to_time_dimension = g_old->scale_to_time_dimension;
        GLOBALS->zoom_dyn = g_old->zoom_dyn;
        GLOBALS->zoom_dyne = g_old->zoom_dyne;

        gtk_notebook_set_current_page(GTK_NOTEBOOK(GLOBALS->notebook), GLOBALS->this_context_page);
        return(TRUE);
        }

return(FALSE);
}


static int gtkwavetcl_setTabActive(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
	gint rc;

	if(!GLOBALS->in_tcl_callback)
		{
        	char *s = get_Tcl_string(objv[1]);
        	unsigned int tabnum = atoi(s);
		rc = switch_to_tab_number(tabnum);

        	MaxSignalLength();
        	signalarea_configure_event(GLOBALS->signalarea, NULL);

        	gtkwave_main_iteration();
		}
		else
		{
		gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"gtkwave::setTabActive prohibited in callback",WAVE_TCLCB_ERROR_FLAGS);
		rc = -1;
		}

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_getNumTabs(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
int value = GLOBALS->num_notebook_pages;
return(gtkwavetcl_printInteger(clientData, interp, objc, objv, value));
}


static int gtkwavetcl_installFileFilter(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        unsigned int which = atoi(s);
	gint rc = install_file_filter(which);

        gtkwave_main_iteration();
	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_setCurrentTranslateFile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_file(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_file));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_setCurrentTranslateEnums(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_enums(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_file));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_installProcFilter(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        unsigned int which = atoi(s);
	gint rc = install_proc_filter(which);

        gtkwave_main_iteration();
	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_setCurrentTranslateProc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_proc(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_proc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_installTransFilter(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
        unsigned int which = atoi(s);
	gint rc = install_ttrans_filter(which);

        gtkwave_main_iteration();
	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, rc));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


static int gtkwavetcl_setCurrentTranslateTransProc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
if(objc == 2)
        {
        char *s = get_Tcl_string(objv[1]);
	set_current_translate_ttrans(s);

	return(gtkwavetcl_printInteger(clientData, interp, objc, objv, GLOBALS->current_translate_ttrans));
        }
        else
        {
        return(gtkwavetcl_badNumArgs(clientData, interp, objc, objv, 1));
        }

}


tcl_cmdstruct gtkwave_commands[] =
	{
	{"addCommentTracesFromList",		gtkwavetcl_addCommentTracesFromList},
	{"addSignalsFromList",			gtkwavetcl_addSignalsFromList},
	{"deleteSignalsFromList",		gtkwavetcl_deleteSignalsFromList},
	{"deleteSignalsFromListIncludingDuplicates", gtkwavetcl_deleteSignalsFromListIncludingDuplicates},
	{"findNextEdge",			gtkwavetcl_findNextEdge},
	{"findPrevEdge",			gtkwavetcl_findPrevEdge},
	{"forceOpenTreeNode",			gtkwavetcl_forceOpenTreeNode},
	{"getArgv",				gtkwavetcl_getArgv},
	{"getBaselineMarker",			gtkwavetcl_getBaselineMarker},
	{"getDisplayedSignals",			gtkwavetcl_getDisplayedSignals},
	{"getDumpFileName",			gtkwavetcl_getDumpFileName},
	{"getDumpType", 			gtkwavetcl_getDumpType},
	{"getFacDir", 				gtkwavetcl_getFacDir},
	{"getFacDtype", 			gtkwavetcl_getFacDtype},
	{"getFacName", 				gtkwavetcl_getFacName},
	{"getFacVtype", 			gtkwavetcl_getFacVtype},
	{"getFontHeight",			gtkwavetcl_getFontHeight},
	{"getFromEntry",			gtkwavetcl_getFromEntry},
	{"getHierMaxLevel",			gtkwavetcl_getHierMaxLevel},
	{"getLeftJustifySigs",			gtkwavetcl_getLeftJustifySigs},
	{"getLongestName", 			gtkwavetcl_getLongestName},
	{"getMarker",				gtkwavetcl_getMarker},
	{"getMaxTime", 				gtkwavetcl_getMaxTime},
	{"getMinTime", 				gtkwavetcl_getMinTime},
	{"getNamedMarker", 			gtkwavetcl_getNamedMarker},
	{"getNumFacs", 				gtkwavetcl_getNumFacs},
	{"getNumTabs",				gtkwavetcl_getNumTabs},
	{"getPixelsUnitTime", 			gtkwavetcl_getPixelsUnitTime},
	{"getSaveFileName",			gtkwavetcl_getSaveFileName},
	{"getStemsFileName",			gtkwavetcl_getStemsFileName},
	{"getTimeDimension", 			gtkwavetcl_getTimeDimension},
	{"getTimeZero", 			gtkwavetcl_getTimeZero},
	{"getToEntry",				gtkwavetcl_getToEntry},
	{"getTotalNumTraces",  			gtkwavetcl_getTotalNumTraces},
	{"getTraceFlagsFromIndex", 		gtkwavetcl_getTraceFlagsFromIndex},
	{"getTraceFlagsFromName",		gtkwavetcl_getTraceFlagsFromName},
	{"getTraceNameFromIndex", 		gtkwavetcl_getTraceNameFromIndex},
	{"getTraceScrollbarRowValue", 		gtkwavetcl_getTraceScrollbarRowValue},
	{"getTraceValueAtMarkerFromIndex", 	gtkwavetcl_getTraceValueAtMarkerFromIndex},
	{"getTraceValueAtMarkerFromName",	gtkwavetcl_getTraceValueAtMarkerFromName},
	{"getTraceValueAtNamedMarkerFromName",	gtkwavetcl_getTraceValueAtNamedMarkerFromName},
	{"getUnitTimePixels", 			gtkwavetcl_getUnitTimePixels},
	{"getVisibleNumTraces", 		gtkwavetcl_getVisibleNumTraces},
	{"getWaveHeight", 			gtkwavetcl_getWaveHeight},
	{"getWaveWidth", 			gtkwavetcl_getWaveWidth},
	{"getWindowEndTime", 			gtkwavetcl_getWindowEndTime},
	{"getWindowStartTime", 			gtkwavetcl_getWindowStartTime},
	{"getZoomFactor",			gtkwavetcl_getZoomFactor},
	{"highlightSignalsFromList",		gtkwavetcl_highlightSignalsFromList},
	{"installFileFilter",			gtkwavetcl_installFileFilter},
	{"installProcFilter",			gtkwavetcl_installProcFilter},
	{"installTransFilter",			gtkwavetcl_installTransFilter},
	{"loadFile",			        gtkwavetcl_loadFile},
   	{"nop", 				gtkwavetcl_nop},
	{"presentWindow",			gtkwavetcl_presentWindow},
	{"processTclList",			gtkwavetcl_processTclList}, /* not for general-purpose use */
	{"reLoadFile",			        gtkwavetcl_reLoadFile},
	{"setBaselineMarker",			gtkwavetcl_setBaselineMarker},
	{"setCurrentTranslateEnums",		gtkwavetcl_setCurrentTranslateEnums},
	{"setCurrentTranslateFile",		gtkwavetcl_setCurrentTranslateFile},
	{"setCurrentTranslateProc",		gtkwavetcl_setCurrentTranslateProc},
	{"setCurrentTranslateTransProc",	gtkwavetcl_setCurrentTranslateTransProc},
	{"setFromEntry",			gtkwavetcl_setFromEntry},
	{"setLeftJustifySigs",			gtkwavetcl_setLeftJustifySigs},
	{"setMarker",				gtkwavetcl_setMarker},
	{"setNamedMarker",			gtkwavetcl_setNamedMarker},
   	{"setTabActive",			gtkwavetcl_setTabActive},
	{"setToEntry",				gtkwavetcl_setToEntry},
	{"setTraceHighlightFromIndex",		gtkwavetcl_setTraceHighlightFromIndex},
	{"setTraceHighlightFromNameMatch",	gtkwavetcl_setTraceHighlightFromNameMatch},
	{"setTraceScrollbarRowValue", 		gtkwavetcl_setTraceScrollbarRowValue},
	{"setWindowStartTime",			gtkwavetcl_setWindowStartTime},
	{"setZoomFactor",			gtkwavetcl_setZoomFactor},
	{"setZoomRangeTimes",			gtkwavetcl_setZoomRangeTimes},
	{"showSignal",         			gtkwavetcl_showSignal},
	{"signalChangeList",                    gtkwavetcl_signalChangeList},	/* changed from signal_change_list for consistency! */
	{"unhighlightSignalsFromList",		gtkwavetcl_unhighlightSignalsFromList},
   	{"", 					NULL} /* sentinel */
	};

#else

static void dummy_function(void)
{
/* nothing */
}

#endif


