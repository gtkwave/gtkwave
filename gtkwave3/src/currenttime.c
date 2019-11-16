/*
 * Copyright (c) Tony Bybell 1999-2016.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "currenttime.h"
#include "symbol.h"

static char *time_prefix=WAVE_SI_UNITS;

static char *maxtime_label_text="Maximum Time";
static char *marker_label_text ="Marker Time";

static char *maxtime_label_text_hpos="Max";
static char *marker_label_text_hpos ="Marker";


void fractional_timescale_fix(char *s)
{
char buf[32], sfx[2];
int i, len;
int prefix_idx = 0;

if(*s != '0') return;

len = strlen(s);
for(i=0;i<len;i++)
	{
        if((s[i]!='0')&&(s[i]!='1')&&(s[i]!='.'))
        	{
		buf[i] = 0;
                prefix_idx=i;
                break;
                }
		else
		{
		buf[i] = s[i];
		}
	}

if(!strcmp(buf, "0.1"))
	{
	strcpy(buf, "100");
	}
else if(!strcmp(buf, "0.01"))
	{
	strcpy(buf, "10");
	}
else if(!strcmp(buf, "0.001"))
	{
	strcpy(buf, "1");
	}
else
	{
	return;
	}

len = strlen(WAVE_SI_UNITS);
for(i=0;i<len-1;i++)
	{
	if(s[prefix_idx] == WAVE_SI_UNITS[i]) break;
	}

sfx[0] = WAVE_SI_UNITS[i+1];
sfx[1] = 0;
strcat(buf, sfx);
strcat(buf, "s");
/* printf("old time: '%s', new time: '%s'\n", s, buf); */
strcpy(s, buf);
}


void update_maxmarker_labels(void)
{
if(GLOBALS->use_maxtime_display)
	{
	gtk_label_set(GTK_LABEL(GLOBALS->max_or_marker_label_currenttime_c_1),
		(!GLOBALS->use_toolbutton_interface) ? maxtime_label_text : maxtime_label_text_hpos);
	update_maxtime(GLOBALS->max_time);
	}
	else
	{
	gtk_label_set(GTK_LABEL(GLOBALS->max_or_marker_label_currenttime_c_1),
		(!GLOBALS->use_toolbutton_interface) ? marker_label_text : marker_label_text_hpos);
	update_markertime(GLOBALS->tims.marker);
	}
}

/* handles floating point values with units */
static TimeType unformat_time_complex(const char *s, char dim)
{
int i, delta, rc;
unsigned char ch = dim;
double d = 0.0;
const char *offs = NULL, *doffs = NULL;

rc = sscanf(s, "%lf %cs", &d, &ch);
if(rc == 2)
        {
        ch = tolower(ch);
        if(ch=='s') ch = ' ';
        offs=strchr(time_prefix, ch);
        if(offs)
                {
                doffs=strchr(time_prefix, (int)dim);
                if(!doffs) doffs = offs; /* should *never* happen */

                delta= (doffs-time_prefix) - (offs-time_prefix);

                if(delta<0)
                        {
                        for(i=delta;i<0;i++)
                                {
                                d=d/1000;
                                }
                        }
                        else
                        {
                        for(i=0;i<delta;i++)
                                {
                                d=d*1000;
                                }
                        }
                }
        }

return((TimeType)d);
}

/* handles integer values with units */
static TimeType unformat_time_simple(const char *buf, char dim)
{
TimeType rval;
const char *pnt;
char *offs=NULL, *doffs;
char ch;
int i, ich, delta;

rval=atoi_64(buf);
if((pnt=GLOBALS->atoi_cont_ptr))
	{
	while((ch=*(pnt++)))
		{
		if((ch==' ')||(ch=='\t')) continue;

		ich=tolower((int)ch);
		if(ich=='s') ich=' ';	/* as in plain vanilla seconds */

		offs=strchr(time_prefix, ich);
		break;
		}
	}

if(!offs) return(rval);
if((dim=='S')||(dim=='s'))
	{
	doffs = time_prefix;
	}
	else
	{
	doffs=strchr(time_prefix, (int)dim);
	if(!doffs) return(rval); /* should *never* happen */
	}

delta= (doffs-time_prefix) - (offs-time_prefix);

if(delta<0)
	{
	for(i=delta;i<0;i++)
		{
		rval=rval/1000;
		}
	}
	else
	{
	for(i=0;i<delta;i++)
		{
		rval=rval*1000;
		}
	}

return(rval);
}

TimeType unformat_time(const char *s, char dim)
{
const char *compar = ".+eE";
int compar_len = strlen(compar);
int i;
char *pnt;

if((*s == 'M')||(*s == 'm'))
	{
	if(bijective_marker_id_string_len(s+1))
		{
		unsigned int mkv = bijective_marker_id_string_hash(s+1);
		if(mkv < WAVE_NUM_NAMED_MARKERS)
			{
			TimeType mkvt = GLOBALS->named_markers[mkv];
			if(mkvt != -1)
				{
				return(mkvt);
				}
			}
		}
	}

for(i=0;i<compar_len;i++)
	{
	if((pnt = strchr(s, (int)compar[i])))
		{
		if((tolower((int)(unsigned char)pnt[0]) != 'e') && (tolower((int)(unsigned char)pnt[1]) != 'c'))
			{
			return(unformat_time_complex(s, dim));
			}
		}
	}

return(unformat_time_simple(s, dim));
}

void reformat_time_simple(char *buf, TimeType val, char dim)
{
char *pnt;
int i, offset;

if(val < LLDescriptor(0))
        {
        val = -val;
        buf[0] = '-';
        buf++;
        }

pnt=strchr(time_prefix, (int)dim);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

for(i=offset; i>0; i--)
	{
	if(val%1000) break;
	val=val/1000;
	}

if(i)
	{
	sprintf(buf, TTFormat" %cs", val, time_prefix[i]);
	}
	else
	{
	sprintf(buf, TTFormat" sec", val);
	}
}

void reformat_time(char *buf, TimeType val, char dim)
{
char *pnt;
int i, offset, offsetfix;

if(val < LLDescriptor(0))
        {
        val = -val;
        buf[0] = '-';
        buf++;
        }

pnt=strchr(time_prefix, (int)dim);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

for(i=offset; i>0; i--)
	{
	if(val%1000) break;
	val=val/1000;
	}

if(GLOBALS->scale_to_time_dimension)
	{
	if(GLOBALS->scale_to_time_dimension == 's')
		{
		pnt = time_prefix;
		}
		else
		{
		pnt=strchr(time_prefix, (int)GLOBALS->scale_to_time_dimension);
		}
	if(pnt)
		{
		offsetfix = pnt-time_prefix;
		if(offsetfix != i)
			{
			int j;
			int deltaexp = (offsetfix - i);
			gdouble gval = (gdouble)val;
			gdouble mypow = 1.0;

			if(deltaexp > 0)
				{
				for(j=0;j<deltaexp;j++)
					{
					mypow *= 1000.0;
					}
				}
			else
				{
				for(j=0;j>deltaexp;j--)
					{
					mypow /= 1000.0;
					}
				}

			gval *= mypow;

			if(GLOBALS->scale_to_time_dimension == 's')
				{
				sprintf(buf, "%.9g sec", gval);
				}
				else
				{
				sprintf(buf, "%.9g %cs", gval, GLOBALS->scale_to_time_dimension);
				}

			return;
			}
		}
	}

if((i)&&(time_prefix)) /* scan-build on time_prefix, should not be necessary however */
	{
	sprintf(buf, TTFormat" %cs", val, time_prefix[i]);
	}
	else
	{
	sprintf(buf, TTFormat" sec", val);
	}
}


void reformat_time_as_frequency(char *buf, TimeType val, char dim)
{
char *pnt;
int offset;
double k;

static const double negpow[] = { 1.0, 1.0e-3, 1.0e-6, 1.0e-9, 1.0e-12, 1.0e-15, 1.0e-18, 1.0e-21 };

pnt=strchr(time_prefix, (int)dim);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

if(val)
	{
	k = 1 / ((double)val * negpow[offset]);

	sprintf(buf, "%e Hz", k);
	}
	else
	{
	strcpy(buf, "-- Hz");
	}

}


void reformat_time_blackout(char *buf, TimeType val, char dim)
{
char *pnt;
int i, offset, offsetfix;
struct blackout_region_t *bt = GLOBALS->blackout_regions;
char blackout = ' ';

while(bt)
	{
	if((val>=bt->bstart)&&(val<bt->bend))
		{
		blackout = '*';
		break;
		}

	bt=bt->next;
	}

pnt=strchr(time_prefix, (int)dim);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

for(i=offset; i>0; i--)
	{
	if(val%1000) break;
	val=val/1000;
	}

if(GLOBALS->scale_to_time_dimension)
	{
	if(GLOBALS->scale_to_time_dimension == 's')
		{
		pnt = time_prefix;
		}
		else
		{
		pnt=strchr(time_prefix, (int)GLOBALS->scale_to_time_dimension);
		}
	if(pnt)
		{
		offsetfix = pnt-time_prefix;
		if(offsetfix != i)
			{
			int j;
			int deltaexp = (offsetfix - i);
			gdouble gval = (gdouble)val;
			gdouble mypow = 1.0;

			if(deltaexp > 0)
				{
				for(j=0;j<deltaexp;j++)
					{
					mypow *= 1000.0;
					}
				}
			else
				{
				for(j=0;j>deltaexp;j--)
					{
					mypow /= 1000.0;
					}
				}

			gval *= mypow;

			if(GLOBALS->scale_to_time_dimension == 's')
				{
				sprintf(buf, "%.9g%csec", gval, blackout);
				}
				else
				{
				sprintf(buf, "%.9g%c%cs", gval, blackout, GLOBALS->scale_to_time_dimension);
				}

			return;
			}
		}
	}

if((i)&&(time_prefix)) /* scan-build on time_prefix, should not be necessary however */
	{
	sprintf(buf, TTFormat"%c%cs", val, blackout, time_prefix[i]);
	}
	else
	{
	sprintf(buf, TTFormat"%csec", val, blackout);
	}
}


void update_markertime(TimeType val)
{
#if !defined _MSC_VER
if(GLOBALS->anno_ctx)
	{
	if(val >= 0)
		{
		GLOBALS->anno_ctx->marker_set = 0;	/* avoid race on update */

		if(!GLOBALS->ae2_time_xlate)
			{
			GLOBALS->anno_ctx->marker = val / GLOBALS->time_scale;
			}
			else
			{
			int rvs_xlate = bsearch_aetinfo_timechain(val);
			GLOBALS->anno_ctx->marker = ((TimeType)rvs_xlate) + GLOBALS->ae2_start_cyc;
			}

		reformat_time(GLOBALS->anno_ctx->time_string, val + GLOBALS->global_time_offset, GLOBALS->time_dimension);

		GLOBALS->anno_ctx->marker_set = 1;
		}
		else
		{
		GLOBALS->anno_ctx->marker_set = 0;
		}
	}
#endif

if(!GLOBALS->use_maxtime_display)
	{
	if(val>=0)
		{
		if(GLOBALS->tims.baseline>=0)
			{
			val-=GLOBALS->tims.baseline; /* do delta instead */
			*GLOBALS->maxtext_currenttime_c_1='B';
			if(val>=0)
				{
				*(GLOBALS->maxtext_currenttime_c_1+1)='+';
				if(GLOBALS->use_frequency_delta)
					{
					reformat_time_as_frequency(GLOBALS->maxtext_currenttime_c_1+2, val, GLOBALS->time_dimension);
					}
					else
					{
					reformat_time(GLOBALS->maxtext_currenttime_c_1+2, val, GLOBALS->time_dimension);
					}
				}
				else
				{
				if(GLOBALS->use_frequency_delta)
					{
					reformat_time_as_frequency(GLOBALS->maxtext_currenttime_c_1+1, val, GLOBALS->time_dimension);
					}
					else
					{
					reformat_time(GLOBALS->maxtext_currenttime_c_1+1, val, GLOBALS->time_dimension);
					}
				}
			}
		else if(GLOBALS->tims.lmbcache>=0)
			{
			val-=GLOBALS->tims.lmbcache; /* do delta instead */

			if(GLOBALS->use_frequency_delta)
				{
				reformat_time_as_frequency(GLOBALS->maxtext_currenttime_c_1, val, GLOBALS->time_dimension);
				}
			else
				{
				if(val>=0)
					{
					*GLOBALS->maxtext_currenttime_c_1='+';
					reformat_time(GLOBALS->maxtext_currenttime_c_1+1, val, GLOBALS->time_dimension);
					}
					else
					{
					reformat_time(GLOBALS->maxtext_currenttime_c_1, val, GLOBALS->time_dimension);
					}
				}
			}
		else
			{
			reformat_time(GLOBALS->maxtext_currenttime_c_1, val + GLOBALS->global_time_offset, GLOBALS->time_dimension);
			}
		}
		else
		{
		sprintf(GLOBALS->maxtext_currenttime_c_1, "--");
		}

	gtk_label_set(GTK_LABEL(GLOBALS->maxtimewid_currenttime_c_1), GLOBALS->maxtext_currenttime_c_1);
	}

if(GLOBALS->named_marker_lock_idx>-1)
	{
	if(GLOBALS->tims.marker >= 0)
		{
		int ent_idx = GLOBALS->named_marker_lock_idx;

		if(GLOBALS->named_markers[ent_idx] != GLOBALS->tims.marker)
			{
			GLOBALS->named_markers[ent_idx] = GLOBALS->tims.marker;
			wavearea_configure_event(GLOBALS->wavearea, NULL);
			}
		}
	}
}


void update_basetime(TimeType val)
{
if(val>=0)
	{
	gtk_label_set(GTK_LABEL(GLOBALS->base_or_curtime_label_currenttime_c_1), (!GLOBALS->use_toolbutton_interface) ? "Base Marker" : "Base");
	reformat_time(GLOBALS->curtext_currenttime_c_1, val + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	}
	else
	{
	gtk_label_set(GTK_LABEL(GLOBALS->base_or_curtime_label_currenttime_c_1), (!GLOBALS->use_toolbutton_interface) ? "Current Time" : "Cursor");
	reformat_time_blackout(GLOBALS->curtext_currenttime_c_1, GLOBALS->cached_currenttimeval_currenttime_c_1 + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	}

gtk_label_set(GTK_LABEL(GLOBALS->curtimewid_currenttime_c_1), GLOBALS->curtext_currenttime_c_1);
}


void update_maxtime(TimeType val)
{
GLOBALS->max_time=val;

if(GLOBALS->use_maxtime_display)
	{
	reformat_time(GLOBALS->maxtext_currenttime_c_1, val + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_label_set(GTK_LABEL(GLOBALS->maxtimewid_currenttime_c_1), GLOBALS->maxtext_currenttime_c_1);
	}
}


void update_currenttime(TimeType val)
{
GLOBALS->cached_currenttimeval_currenttime_c_1 = val;

if(GLOBALS->tims.baseline<0)
	{
	GLOBALS->currenttime=val;
	reformat_time_blackout(GLOBALS->curtext_currenttime_c_1, val + GLOBALS->global_time_offset, GLOBALS->time_dimension);
	gtk_label_set(GTK_LABEL(GLOBALS->curtimewid_currenttime_c_1), GLOBALS->curtext_currenttime_c_1);
	}
}


/* Create an entry box */
GtkWidget *
create_time_box(void)
{
GtkWidget *mainbox;
GtkWidget *eventbox;

GLOBALS->max_or_marker_label_currenttime_c_1=(GLOBALS->use_maxtime_display)
	? gtk_label_new((!GLOBALS->use_toolbutton_interface) ? maxtime_label_text : maxtime_label_text_hpos)
	: gtk_label_new((!GLOBALS->use_toolbutton_interface) ? marker_label_text : marker_label_text_hpos);

GLOBALS->maxtext_currenttime_c_1=(char *)malloc_2(40);
if(GLOBALS->use_maxtime_display)
	{
	reformat_time(GLOBALS->maxtext_currenttime_c_1, GLOBALS->max_time, GLOBALS->time_dimension);
	}
	else
	{
	sprintf(GLOBALS->maxtext_currenttime_c_1,"--");
	}

GLOBALS->maxtimewid_currenttime_c_1=gtk_label_new(GLOBALS->maxtext_currenttime_c_1);

GLOBALS->curtext_currenttime_c_1=(char *)malloc_2(40);
if(GLOBALS->tims.baseline<0)
	{
	GLOBALS->base_or_curtime_label_currenttime_c_1=gtk_label_new((!GLOBALS->use_toolbutton_interface) ? "Current Time" : "Cursor");
	reformat_time(GLOBALS->curtext_currenttime_c_1, (GLOBALS->currenttime=GLOBALS->min_time), GLOBALS->time_dimension);
	GLOBALS->curtimewid_currenttime_c_1=gtk_label_new(GLOBALS->curtext_currenttime_c_1);
	}
	else
	{
	GLOBALS->base_or_curtime_label_currenttime_c_1=gtk_label_new((!GLOBALS->use_toolbutton_interface) ? "Base Marker" : "Base");
	reformat_time(GLOBALS->curtext_currenttime_c_1, GLOBALS->tims.baseline, GLOBALS->time_dimension);
	GLOBALS->curtimewid_currenttime_c_1=gtk_label_new(GLOBALS->curtext_currenttime_c_1);
	}

if(!GLOBALS->use_toolbutton_interface)
	{
	mainbox=gtk_vbox_new(FALSE, 0);
	}
	else
	{
	mainbox=gtk_hbox_new(FALSE, 0);
	}

gtk_widget_show(mainbox);
eventbox=gtk_event_box_new();
gtk_container_add(GTK_CONTAINER(eventbox), mainbox);

if(!GLOBALS->use_toolbutton_interface)
	{
	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->max_or_marker_label_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->max_or_marker_label_currenttime_c_1);
	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->maxtimewid_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->maxtimewid_currenttime_c_1);

	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->base_or_curtime_label_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->base_or_curtime_label_currenttime_c_1);
	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->curtimewid_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->curtimewid_currenttime_c_1);
	}
	else
	{
	GtkWidget *dummy;

	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->max_or_marker_label_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->max_or_marker_label_currenttime_c_1);

        dummy=gtk_label_new(": ");
        gtk_widget_show (dummy);
	gtk_box_pack_start(GTK_BOX(mainbox), dummy, TRUE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->maxtimewid_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->maxtimewid_currenttime_c_1);

        dummy=gtk_label_new("  |  ");
        gtk_widget_show (dummy);
	gtk_box_pack_start(GTK_BOX(mainbox), dummy, TRUE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->base_or_curtime_label_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->base_or_curtime_label_currenttime_c_1);

        dummy=gtk_label_new(": ");
        gtk_widget_show (dummy);
	gtk_box_pack_start(GTK_BOX(mainbox), dummy, TRUE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), GLOBALS->curtimewid_currenttime_c_1, TRUE, FALSE, 0);
	gtk_widget_show(GLOBALS->curtimewid_currenttime_c_1);
	}

return(eventbox);
}




TimeType time_trunc(TimeType t)
{
if(!GLOBALS->use_full_precision)
if(GLOBALS->time_trunc_val_currenttime_c_1!=1)
	{
	t=t/GLOBALS->time_trunc_val_currenttime_c_1;
	t=t*GLOBALS->time_trunc_val_currenttime_c_1;
	if(t<GLOBALS->tims.first) t=GLOBALS->tims.first;
	}

return(t);
}

void time_trunc_set(void)
{
gdouble gcompar=1e15;
TimeType compar=LLDescriptor(1000000000000000);

for(;compar!=1;compar=compar/10,gcompar=gcompar/((gdouble)10.0))
	{
	if(GLOBALS->nspx>=gcompar)
		{
		GLOBALS->time_trunc_val_currenttime_c_1=compar;
		return;
		}
        }

GLOBALS->time_trunc_val_currenttime_c_1=1;
}


/*
 * called by lxt/lxt2/vzt reader inits
 */
void exponent_to_time_scale(signed char scale)
{
switch(scale)
        {
        case 2:        GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 's'; break;
        case 1:        GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case 0:                                        GLOBALS->time_dimension = 's'; break;

        case -1:        GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'm'; break;
        case -2:        GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -3:                                        GLOBALS->time_dimension = 'm'; break;

        case -4:        GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'u'; break;
        case -5:        GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -6:                                        GLOBALS->time_dimension = 'u'; break;

        case -10:       GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'p'; break;
        case -11:       GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -12:                                       GLOBALS->time_dimension = 'p'; break;

        case -13:       GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'f'; break;
        case -14:       GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -15:                                       GLOBALS->time_dimension = 'f'; break;

        case -16:       GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'a'; break;
        case -17:       GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -18:                                       GLOBALS->time_dimension = 'a'; break;

        case -19:       GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'z'; break;
        case -20:       GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -21:                                       GLOBALS->time_dimension = 'z'; break;

        case -7:        GLOBALS->time_scale = LLDescriptor(100); GLOBALS->time_dimension = 'n'; break;
        case -8:        GLOBALS->time_scale = LLDescriptor(10); /* fallthrough */
        case -9:
        default:                                        GLOBALS->time_dimension = 'n'; break;
        }
}

