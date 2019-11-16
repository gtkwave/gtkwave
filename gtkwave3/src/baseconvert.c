/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <math.h>
#include <string.h>
#include "gtk12compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "symbol.h"
#include "bsearch.h"
#include "color.h"
#include "strace.h"
#include "debug.h"
#include "translate.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "pipeio.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

/*
 * convert binary <=> gray/popcnt in place
 */
#define cvt_gray(f,p,n) \
do { \
if((f)&(TR_GRAYMASK|TR_POPCNT)) \
	{ \
	if((f)&TR_BINGRAY) { convert_bingray((p),(n)); } \
	if((f)&TR_GRAYBIN) { convert_graybin((p),(n)); } \
	if((f)&TR_POPCNT)  { convert_popcnt((p),(n)); } \
	} \
} while(0)


static void convert_graybin(char *pnt, int nbits)
{
char kill_state = 0;
char pch = AN_0;
int i;

for(i=0;i<nbits;i++)
	{
	char ch = pnt[i];

	if(!kill_state)
		{
		switch(ch)
			{
			case AN_0:
			case AN_L:
					if((pch == AN_1) || (pch == AN_H))
						{
						pnt[i] = pch;
						}
					break;

			case AN_1:
			case AN_H:
					if(pch == AN_1)
						{
						pnt[i] = AN_0;
						}
					else if(pch == AN_H)
						{
						pnt[i] = AN_L;
						}
					break;

			default:
					kill_state = 1;
					break;
			}

		pch = pnt[i];	/* pch is xor accumulator */
		}
		else
		{
		pnt[i] = pch;
		}
	}
}

static void convert_bingray(char *pnt, int nbits)
{
char kill_state = 0;
char pch = AN_0;
int i;

for(i=0;i<nbits;i++)
	{
	char ch = pnt[i];

	if(!kill_state)
		{
		switch(ch)
			{
			case AN_0:
			case AN_L:
					if((pch == AN_1) || (pch == AN_H))
						{
						pnt[i] = pch;
						}
					break;

			case AN_1:
			case AN_H:
					if(pch == AN_1)
						{
						pnt[i] = AN_0;
						}
					else if(pch == AN_H)
						{
						pnt[i] = AN_L;
						}
					break;

			default:
					kill_state = 1;
					break;
			}

		pch = ch;	/* pch is previous character */
		}
		else
		{
		pnt[i] = pch;
		}
	}
}


static void convert_popcnt(char *pnt, int nbits)
{
int i;
unsigned int pop = 0;

for(i=0;i<nbits;i++)
	{
	char ch = pnt[i];

	switch(ch)
		{
		case AN_1:
		case AN_H:	pop++;
				break;

		default:
				break;
			}

	}

for(i=nbits-1;i>=0;i--) /* always requires less number of bits */
	{
	pnt[i] = (pop & 1) ? AN_1 : AN_0;
	pop >>= 1;
	}
}


static void dpr_e16(char *str, double d)
{
char *buf16;
char buf15[24];
int l16;
int l15;

buf16 = str;
buf16[23] = 0;
l16 = snprintf(buf16, 24, "%.16g", d);
if(l16 >= 18)
	{
	buf15[23] = 0;
	l15 = snprintf(buf15, 24, "%.15g", d);	
	if((l16-l15) > 3)
		{
		strcpy(str, buf15);
		}
	}
}


static void cvt_fpsdec(Trptr t, TimeType val, char *os, int len, int nbits)
{
(void)nbits; /* number of bits shouldn't be relevant here as we're going through a fraction */

int shamt = t->t_fpdecshift;
TimeType lpart = val >> shamt;
TimeType rmsk = (1 << shamt);
TimeType rbit = (val >= 0) ? (val & (rmsk-1)) : ((-val) & (rmsk-1));
double rfrac;
int negflag = 0;
char dbuf[32];
char bigbuf[64];


if(rmsk)
        {
        rfrac = (double)rbit / (double)rmsk;

        if(shamt)
                {
                if(lpart < 0)
                        {
                        if(rbit)
                                {
                                lpart++;
                                if(!lpart) negflag = 1;
                                }
                        }
                }

        }
        else
        {
        rfrac = 0.0;
        }

dpr_e16(dbuf, rfrac); /* sprintf(dbuf, "%.16g", rfrac); */
char *dot = strchr(dbuf, '.');

if(dot && (dbuf[0] == '0'))
        {
	sprintf(bigbuf, "%s"TTFormat".%s", negflag ? "-" : "", lpart, dot+1);
        strncpy(os, bigbuf, len);
        os[len-1] = 0;
        }
        else
        {
	sprintf(os, "%s"TTFormat, negflag ? "-" : "", lpart);
        }
}

static void cvt_fpsudec(Trptr t, TimeType val, char *os, int len)
{
int shamt = t->t_fpdecshift;
UTimeType lpart = ((UTimeType)val) >> shamt;
TimeType rmsk = (1 << shamt);
TimeType rbit = (val & (rmsk-1));
double rfrac;
char dbuf[32];
char bigbuf[64];

if(rmsk)
	{
	rfrac = (double)rbit / (double)rmsk;
	}
	else
	{
	rfrac = 0.0;
	}			

dpr_e16(dbuf, rfrac); /* sprintf(dbuf, "%.16g", rfrac);	*/
char *dot = strchr(dbuf, '.');
if(dot && (dbuf[0] == '0'))
	{
	sprintf(bigbuf, UTTFormat".%s", lpart, dot+1);
	strncpy(os, bigbuf, len);
	os[len-1] = 0;
	}
	else
	{
	sprintf(os, UTTFormat, lpart);
	}
}


/*
 * convert trptr+vptr into an ascii string
 */
static char *convert_ascii_2(Trptr t, vptr v)
{
TraceFlagsType flags;
int nbits;
unsigned char *bits;
char *os, *pnt, *newbuff;
int i, j, len;

static const char xfwd[AN_COUNT]= AN_NORMAL  ;
static const char xrev[AN_COUNT]= AN_INVERSE ;
const char *xtab;

flags=t->flags;
nbits=t->n.vec->nbits;
bits=v->v;

if(flags&TR_INVERT)
	{
	xtab = xrev;
	}
	else
	{
	xtab = xfwd;
	}

if(flags&(TR_ZEROFILL|TR_ONEFILL))
	{
	char whichfill = (flags&TR_ZEROFILL) ? AN_0 : AN_1;
	int msi = 0, lsi = 0, ok = 0;
	if((t->name)&&(nbits > 1))
		{
		char *lbrack = strrchr(t->name, '[');
		if(lbrack)
			{
			int rc = sscanf(lbrack+1, "%d:%d", &msi, &lsi);
			if(rc == 2)
				{
				if(((msi - lsi + 1) == nbits) || ((lsi - msi + 1) == nbits))
					{
					ok = 1;	/* to ensure sanity... */
					}
				}
			}
		}

	if(ok)
		{
		if(msi > lsi)
			{
			if(lsi > 0)
				{
				pnt=wave_alloca(msi + 1);

				memcpy(pnt, bits, nbits);

	        		for(i=nbits;i<msi+1;i++)
	                		{
	                		pnt[i]=whichfill;
	                		}

				bits = (unsigned char *)pnt;
				nbits = msi + 1;
				}
			}
			else
			{
			if(msi > 0)
				{
				pnt=wave_alloca(lsi + 1);

	        		for(i=0;i<msi;i++)
	                		{
	                		pnt[i]=whichfill;
	                		}

				memcpy(pnt+i, bits, nbits);

				bits = (unsigned char *)pnt;
				nbits = lsi + 1;
				}
			}
		}
	}


newbuff=(char *)malloc_2(nbits+6); /* for justify */
if(flags&TR_REVERSE)
	{
	char *fwdpnt, *revpnt;

	fwdpnt=(char *)bits;
	revpnt=newbuff+nbits+6;
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(--revpnt)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	}
	else
	{
	char *fwdpnt, *fwdpnt2;

	fwdpnt=(char *)bits;
	fwdpnt2=newbuff;
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	}


if(flags&TR_ASCII)
	{
	char *parse;
	int found=0;

	free_2(newbuff);
	newbuff=(char *)malloc_2(nbits+14); /* for justify */
	if(flags&TR_REVERSE)
		{
		char *fwdpnt, *revpnt;
	
		fwdpnt=(char *)bits;
		revpnt=newbuff+nbits+14;
		for(i=0;i<7;i++) *(--revpnt)=xfwd[0];
		for(i=0;i<nbits;i++)
			{
			*(--revpnt)=xtab[(int)(*(fwdpnt++))];
			}
		for(i=0;i<7;i++) *(--revpnt)=xfwd[0];
		}
		else
		{
		char *fwdpnt, *fwdpnt2;
	
		fwdpnt=(char *)bits;
		fwdpnt2=newbuff;
		for(i=0;i<7;i++) *(fwdpnt2++)=xfwd[0];
		for(i=0;i<nbits;i++)
			{
			*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
			}
		for(i=0;i<7;i++) *(fwdpnt2++)=xfwd[0];
		}

	len=(nbits/8)+2+2;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='"'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+7)&7)):(newbuff+7);
	cvt_gray(flags,parse,(flags&TR_RJUSTIFY)?((nbits+7)&~7):nbits);

	for(i=0;i<nbits;i+=8)
		{
		unsigned long val;

		val=0;
		for(j=0;j<8;j++)
			{
			val<<=1;

			if((parse[j]==AN_X)||(parse[j]==AN_Z)||(parse[j]==AN_W)||(parse[j]==AN_U)||(parse[j]==AN_DASH)) { val=1000; /* arbitrarily large */}
			if((parse[j]==AN_1)||(parse[j]==AN_H)) { val|=1; }
			}


		if (val) {
			if (val > 0x7f || !isprint(val)) *pnt++ = '.'; else *pnt++ = val;
			found=1;
		}

		parse+=8;
		}
	if (!found && !GLOBALS->show_base) {
		*(pnt++)='"';
		*(pnt++)='"';
	}

	if(GLOBALS->show_base) { *(pnt++)='"'; }
	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if((flags&TR_HEX)||((flags&(TR_DEC|TR_SIGNED))&&(nbits>64)&&(!(flags&TR_POPCNT))))
	{
	char *parse;

	len=(nbits/4)+2+1;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='$'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+3)&3)):(newbuff+3);
	cvt_gray(flags,parse,(flags&TR_RJUSTIFY)?((nbits+3)&~3):nbits);

	for(i=0;i<nbits;i+=4)
		{
		unsigned char val;

		val=0;
		for(j=0;j<4;j++)
			{
			val<<=1;

			if((parse[j]==AN_1)||(parse[j]==AN_H))
				{
				val|=1;
				}
			else
			if((parse[j]==AN_0)||(parse[j]==AN_L))
				{
				}
			else
			if(parse[j]==AN_X)
				{
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
				for(k=j+1;k<4;k++)
					{
					if(parse[k]!=AN_X)
						{
						char *thisbyt = parse + i + k;
						char *lastbyt = newbuff + 3 + nbits - 1;
						if((lastbyt - thisbyt) >= 0) match = 0;
						break;
						}
					}
				val = (match) ? 16 : 21; break;
				}
			else
			if(parse[j]==AN_Z)
				{
				int xover = 0;
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
				for(k=j+1;k<4;k++)
					{
					if(parse[k]!=AN_Z)
						{
						if(parse[k]==AN_X)
							{
							xover = 1;
							}
							else
							{
							char *thisbyt = parse + i + k;
							char *lastbyt = newbuff + 3 + nbits - 1;
							if((lastbyt - thisbyt) >= 0) match = 0;
							}
						break;
						}
					}

				if(xover) val = 21;
				else val = (match) ? 17 : 22;
				break;
				}
			else
			if(parse[j]==AN_W)
				{
				int xover = 0;
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
				for(k=j+1;k<4;k++)
					{
					if(parse[k]!=AN_W)
						{
						if(parse[k]==AN_X)
							{
							xover = 1;
							}
							else
							{
							char *thisbyt = parse + i + k;
							char *lastbyt = newbuff + 3 + nbits - 1;
							if((lastbyt - thisbyt) >= 0) match = 0;
							}
						break;
						}
					}

				if(xover) val = 21;
				else val = (match) ? 18 : 23;
				break;
				}
			else
			if(parse[j]==AN_U)
				{
				int xover = 0;
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
				for(k=j+1;k<4;k++)
					{
					if(parse[k]!=AN_U)
						{
						if(parse[k]==AN_X)
							{
							xover = 1;
							}
							else
							{
							char *thisbyt = parse + i + k;
							char *lastbyt = newbuff + 3 + nbits - 1;
							if((lastbyt - thisbyt) >= 0) match = 0;
							}
						break;
						}
					}

				if(xover) val = 21;
				else val = (match) ? 19 : 24;
				break;
				}
			else
			if(parse[j]==AN_DASH)
				{
				int xover = 0;
				int k;
				for(k=j+1;k<4;k++)
					{
					if(parse[k]!=AN_DASH)
						{
						if(parse[k]==AN_X)
							{
							xover = 1;
							}
						break;
						}
					}

				if(xover) val = 21;
				else val = 20;
				break;
				}
			}

		*(pnt++)=AN_HEX_STR[val];

		parse+=4;
		}

	*(pnt)=0x00;  /* scan build : remove dead increment */
	}
else if(flags&TR_OCT)
	{
	char *parse;

	len=(nbits/3)+2+1;		/* #xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='#'; }

	parse=(flags&TR_RJUSTIFY)
		?(newbuff+((nbits%3)?(nbits%3):3))
		:(newbuff+3);
	cvt_gray(flags,parse,(flags&TR_RJUSTIFY)?(((nbits+2)/3)*3):nbits);

	for(i=0;i<nbits;i+=3)
		{
		unsigned char val;

		val=0;
		for(j=0;j<3;j++)
			{
			val<<=1;

			if(parse[j]==AN_X) { val=8; break; }
			if(parse[j]==AN_Z) { val=9; break; }
			if(parse[j]==AN_W) { val=10; break; }
			if(parse[j]==AN_U) { val=11; break; }
			if(parse[j]==AN_DASH) { val=12; break; }

			if((parse[j]==AN_1)||(parse[j]==AN_H)) { val|=1; }
			}

		*(pnt++)=AN_OCT_STR[val];

		parse+=3;
		}

	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if(flags&TR_BIN)
	{
	char *parse;

	len=(nbits/1)+2+1;		/* %xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='%'; }

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	for(i=0;i<nbits;i++)
		{
		*(pnt++)=AN_STR[(int)(*(parse++))];
		}

	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if(flags&TR_SIGNED)
	{
	char *parse;
	TimeType val = 0;
	unsigned char fail=0;

	len=21;	/* len+1 of 0x8000000000000000 expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	if((parse[0]==AN_1)||(parse[0]==AN_H))
		{ val = LLDescriptor(-1); }
	else
	if((parse[0]==AN_0)||(parse[0]==AN_L))
		{ val = LLDescriptor(0); }
	else
		{ fail = 1; }

	if(!fail)
	for(i=1;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
		else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
		}

	if(!fail)
		{
		if((flags&TR_FPDECSHIFT)&&(t->t_fpdecshift))
			{
			cvt_fpsdec(t, val, os, len, nbits);
			}
			else
			{
			if(!(flags&TR_TIME))
				{
				sprintf(os, TTFormat, val);
				}
				else
				{
				free_2(os);
				os = calloc_2(1, 128);
				reformat_time(os, val, GLOBALS->time_dimension);
				}
			}
		}
		else
		{
		strcpy(os, "XXX");
		}
	}
else if(flags&TR_REAL)
	{
	char *parse;

	if((nbits==64) || (nbits==32))
		{
		UTimeType utt = LLDescriptor(0);
		double d;
		float f;
		uint32_t utt_32;

		parse=newbuff+3;
		cvt_gray(flags,parse,nbits);

		for(i=0;i<nbits;i++)
			{
			char ch = AN_STR[(int)(*(parse++))];
			if ((ch=='0')||(ch=='1'))
				{
				utt <<= 1;
				if(ch=='1')
					{
					utt |= LLDescriptor(1);
					}
				}
				else
				{
				goto rl_go_binary;
				}
			}

		os=/*pnt=*/(char *)calloc_2(1,64); /* scan-build */
		if(nbits==64)
			{
			memcpy(&d, &utt, sizeof(double));
			dpr_e16(os, d); /* sprintf(os, "%.16g", d); */
			}
			else
			{
			utt_32 = utt;
			memcpy(&f, &utt_32, sizeof(float));
			sprintf(os, "%f", f);
			}
		}
		else
		{
rl_go_binary:	len=(nbits/1)+2+1;		/* %xxxxx */
		os=pnt=(char *)calloc_2(1,len);
		if(GLOBALS->show_base) { *(pnt++)='%'; }

		parse=newbuff+3;
		cvt_gray(flags,parse,nbits);

		for(i=0;i<nbits;i++)
			{
			*(pnt++)=AN_STR[(int)(*(parse++))];
			}

		*(pnt)=0x00; /* scan build : remove dead increment */
		}
	}
else	/* decimal when all else fails */
	{
	char *parse;
	UTimeType val=0;
	unsigned char fail=0;

	len=21;	/* len+1 of 0xffffffffffffffff expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	for(i=0;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
		else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
		}

	if(!fail)
		{
		if((flags&TR_FPDECSHIFT)&&(t->t_fpdecshift))
			{
			cvt_fpsudec(t, val, os, len);
			}
			else
			{
                        if(!(flags&TR_TIME))
                                {
				sprintf(os, UTTFormat, val);
                                }
                                else
                                {
                                free_2(os);
                                os = calloc_2(1, 128);
                                reformat_time(os, val, GLOBALS->time_dimension);
                                }
			}
		}
		else
		{
		strcpy(os, "XXX");
		}
	}

free_2(newbuff);
return(os);
}


/*
 * convert trptr+hptr vectorstring into an ascii string
 */
char *convert_ascii_real(Trptr t, double *d)
{
char *rv;

if(t && (t->flags & TR_REAL2BITS) && d) /* "real2bits" also allows other filters such as "bits2real" on top of it */
        {
	struct TraceEnt t2;
	char vec[64];
	int i;
	guint64 swapmem;

	memcpy(&swapmem, d, sizeof(guint64));
	for(i=0;i<64;i++)
		{
		if(swapmem & (ULLDescriptor(1)<<(63-i)))
			{
			vec[i] = AN_1;
			}
			else
			{
			vec[i] = AN_0;
			}
		}

	memcpy(&t2, t, sizeof(struct TraceEnt));

	t2.n.nd->msi = 63;
	t2.n.nd->lsi = 0;
	t2.flags &= ~(TR_REAL2BITS); /* to avoid possible recursion in the future */

	rv = convert_ascii_vec_2(&t2, vec);
        }
	else
	{
	rv=malloc_2(24);	/* enough for .16e format */

	if(d)
		{
		dpr_e16(rv, *d); /* sprintf(rv,"%.16g",*d); */
		}
	else
		{
		strcpy(rv,"UNDEF");
		}
	}

return(rv);
}

char *convert_ascii_string(char *s)
{
char *rv;

if(s)
	{
	rv=(char *)malloc_2(strlen(s)+1);
	strcpy(rv, s);
	}
	else
	{
	rv=(char *)malloc_2(6);
	strcpy(rv, "UNDEF");
	}
return(rv);
}


static const unsigned char cvt_table[] = {
AN_0    /* . */, AN_X    /* . */, AN_Z    /* . */, AN_1    /* . */, AN_H    /* . */, AN_U    /* . */, AN_W    /* . */, AN_L    /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /*   */, AN_DASH /* ! */, AN_DASH /* " */, AN_DASH /* # */, AN_DASH /* $ */, AN_DASH /* % */, AN_DASH /* & */, AN_DASH /* ' */,
AN_DASH /* ( */, AN_DASH /* ) */, AN_DASH /* * */, AN_DASH /* + */, AN_DASH /* , */, AN_DASH /* - */, AN_DASH /* . */, AN_DASH /* / */,
AN_0    /* 0 */, AN_1    /* 1 */, AN_DASH /* 2 */, AN_DASH /* 3 */, AN_DASH /* 4 */, AN_DASH /* 5 */, AN_DASH /* 6 */, AN_DASH /* 7 */,
AN_DASH /* 8 */, AN_DASH /* 9 */, AN_DASH /* : */, AN_DASH /* ; */, AN_DASH /* < */, AN_DASH /* = */, AN_DASH /* > */, AN_DASH /* ? */,
AN_DASH /* @ */, AN_DASH /* A */, AN_DASH /* B */, AN_DASH /* C */, AN_DASH /* D */, AN_DASH /* E */, AN_DASH /* F */, AN_DASH /* G */,
AN_H    /* H */, AN_DASH /* I */, AN_DASH /* J */, AN_DASH /* K */, AN_L    /* L */, AN_DASH /* M */, AN_DASH /* N */, AN_DASH /* O */,
AN_DASH /* P */, AN_DASH /* Q */, AN_DASH /* R */, AN_DASH /* S */, AN_DASH /* T */, AN_U    /* U */, AN_DASH /* V */, AN_W    /* W */,
AN_X    /* X */, AN_DASH /* Y */, AN_Z    /* Z */, AN_DASH /* [ */, AN_DASH /* \ */, AN_DASH /* ] */, AN_DASH /* ^ */, AN_DASH /* _ */,
AN_DASH /* ` */, AN_DASH /* a */, AN_DASH /* b */, AN_DASH /* c */, AN_DASH /* d */, AN_DASH /* e */, AN_DASH /* f */, AN_DASH /* g */,
AN_H    /* h */, AN_DASH /* i */, AN_DASH /* j */, AN_DASH /* k */, AN_L    /* l */, AN_DASH /* m */, AN_DASH /* n */, AN_DASH /* o */,
AN_DASH /* p */, AN_DASH /* q */, AN_DASH /* r */, AN_DASH /* s */, AN_DASH /* t */, AN_U    /* u */, AN_DASH /* v */, AN_W    /* w */,
AN_X    /* x */, AN_DASH /* y */, AN_Z    /* z */, AN_DASH /* { */, AN_DASH /* | */, AN_DASH /* } */, AN_DASH /* ~ */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */,
AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */, AN_DASH /* . */
};

int vtype(Trptr t, char *vec)
{
int i, nbits;
char pch, ch;

if (vec == NULL)
	return(AN_X);

nbits=t->n.nd->msi-t->n.nd->lsi;
if(nbits<0)nbits=-nbits;
nbits++;
pch = ch = cvt_table[(unsigned char)vec[0]];
for (i = 1; i < nbits; i++)
	{
	ch = cvt_table[(unsigned char)vec[i]];
        if(ch != pch) goto miscompare;
	}

return(ch);

miscompare:
if((pch == AN_X) || (pch == AN_U)) return(pch);
if(pch == AN_Z) return(AN_X);
for (; i < nbits; i++)
        {
	ch = cvt_table[(unsigned char)vec[i]];
        if((ch == AN_X) || (ch == AN_U)) return(ch);
        if(ch == AN_Z) return(AN_X);
	}

return(AN_COUNT);
}

int vtype2(Trptr t, vptr v)
{
int i, nbits;
char pch, ch;
char *vec=(char *)v->v;

if(!t->t_filter_converted)
	{
	if (vec == NULL)
		return(AN_X);
	}
	else
	{
	return ( ((vec == NULL) || (vec[0] == 0)) ? AN_Z : AN_COUNT );
	}

nbits=t->n.vec->nbits;

pch = ch = cvt_table[(unsigned char)vec[0]];
for (i = 1; i < nbits; i++)
		{
		ch = cvt_table[(unsigned char)vec[i]];
	        if(ch != pch) goto miscompare;
		}

return(ch);

miscompare:
if((pch == AN_X) || (pch == AN_U)) return(pch);
if(pch == AN_Z) return(AN_X);
for (; i < nbits; i++)
        {
	ch = cvt_table[(unsigned char)vec[i]];
        if((ch == AN_X) || (ch == AN_U)) return(ch);
        if(ch == AN_Z) return(AN_X);
	}

return(AN_COUNT);
}


/*
 * convert trptr+hptr vectorstring into an ascii string
 */
char *convert_ascii_vec_2(Trptr t, char *vec)
{
TraceFlagsType flags;
int nbits;
char *bits;
char *os, *pnt, *newbuff;
int i, j, len;
const char *xtab;

static const char xfwd[AN_COUNT]= AN_NORMAL  ;
static const char xrev[AN_COUNT]= AN_INVERSE ;

flags=t->flags;

nbits=t->n.nd->msi-t->n.nd->lsi;
if(nbits<0)nbits=-nbits;
nbits++;

if(vec)
        {
        bits=vec;
        if(*vec>AN_MSK)              /* convert as needed */
        for(i=0;i<nbits;i++)
                {
		vec[i] = cvt_table[(unsigned char)vec[i]];
                }
        }
        else
        {
        pnt=bits=wave_alloca(nbits);
        for(i=0;i<nbits;i++)
                {
                *pnt++=AN_X;
                }
        }

if((flags&(TR_ZEROFILL|TR_ONEFILL))&&(nbits>1)&&(t->n.nd->msi)&&(t->n.nd->lsi))
	{
	char whichfill = (flags&TR_ZEROFILL) ? AN_0 : AN_1;

	if(t->n.nd->msi > t->n.nd->lsi)
		{
		if(t->n.nd->lsi > 0)
			{
			pnt=wave_alloca(t->n.nd->msi + 1);

			memcpy(pnt, bits, nbits);

        		for(i=nbits;i<t->n.nd->msi+1;i++)
                		{
                		pnt[i]=whichfill;
                		}

			bits = pnt;
			nbits = t->n.nd->msi + 1;
			}
		}
		else
		{
		if(t->n.nd->msi > 0)
			{
			pnt=wave_alloca(t->n.nd->lsi + 1);

        		for(i=0;i<t->n.nd->msi;i++)
                		{
                		pnt[i]=whichfill;
                		}

			memcpy(pnt+i, bits, nbits);

			bits = pnt;
			nbits = t->n.nd->lsi + 1;
			}
		}
	}

if(flags&TR_INVERT)
        {
        xtab = xrev;
        }
        else
        {
        xtab = xfwd;
        }

newbuff=(char *)malloc_2(nbits+6); /* for justify */
if(flags&TR_REVERSE)
	{
	char *fwdpnt, *revpnt;

	fwdpnt=bits;
	revpnt=newbuff+nbits+6;
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(--revpnt)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	}
	else
	{
	char *fwdpnt, *fwdpnt2;

	fwdpnt=bits;
	fwdpnt2=newbuff;
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	}

if(flags&TR_ASCII)
	{
	char *parse;
	int found=0;

	free_2(newbuff);
	newbuff=(char *)malloc_2(nbits+14); /* for justify */
	if(flags&TR_REVERSE)
		{
		char *fwdpnt, *revpnt;
	
		fwdpnt=(char *)bits;
		revpnt=newbuff+nbits+14;
		for(i=0;i<7;i++) *(--revpnt)=xfwd[0];
		for(i=0;i<nbits;i++)
			{
			*(--revpnt)=xtab[(int)(*(fwdpnt++))];
			}
		for(i=0;i<7;i++) *(--revpnt)=xfwd[0];
		}
		else
		{
		char *fwdpnt, *fwdpnt2;
	
		fwdpnt=(char *)bits;
		fwdpnt2=newbuff;
		for(i=0;i<7;i++) *(fwdpnt2++)=xfwd[0];
		for(i=0;i<nbits;i++)
			{
			*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
			}
		for(i=0;i<7;i++) *(fwdpnt2++)=xfwd[0];
		}

	len=(nbits/8)+2+2;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='"'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+7)&7)):(newbuff+7);
	cvt_gray(flags,parse,(flags&TR_RJUSTIFY)?((nbits+7)&~7):nbits);

	for(i=0;i<nbits;i+=8)
		{
		unsigned long val;

		val=0;
		for(j=0;j<8;j++)
			{
			val<<=1;

			if((parse[j]==AN_X)||(parse[j]==AN_Z)||(parse[j]==AN_W)||(parse[j]==AN_U)||(parse[j]==AN_DASH)) { val=1000; /* arbitrarily large */}
			if((parse[j]==AN_1)||(parse[j]==AN_H)) { val|=1; }
			}

		if (val) {
			if (val > 0x7f || !isprint(val)) *pnt++ = '.'; else *pnt++ = val;
			found=1;
		}

		parse+=8;
		}
	if (!found && !GLOBALS->show_base) {
		*(pnt++)='"';
		*(pnt++)='"';
	}

	if(GLOBALS->show_base) { *(pnt++)='"'; }
	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if((flags&TR_HEX)||((flags&(TR_DEC|TR_SIGNED))&&(nbits>64)&&(!(flags&TR_POPCNT))))
	{
	char *parse;

	len=(nbits/4)+2+1;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='$'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+3)&3)):(newbuff+3);
	cvt_gray(flags,parse,(flags&TR_RJUSTIFY)?((nbits+3)&~3):nbits);

	for(i=0;i<nbits;i+=4)
		{
		unsigned char val;

		val=0;
		for(j=0;j<4;j++)
			{
			val<<=1;

			if((parse[j]==AN_1)||(parse[j]==AN_H))
				{
				val|=1;
				}
			else
			if((parse[j]==AN_0)||(parse[j]==AN_L))
				{
				}
			else
			if(parse[j]==AN_X)
				{
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
				for(k=j+1;k<4;k++)
					{
                                        if(parse[k]!=AN_X)
                                                {
                                                char *thisbyt = parse + i + k;
                                                char *lastbyt = newbuff + 3 + nbits - 1;
                                                if((lastbyt - thisbyt) >= 0) match = 0;
                                                break;
                                                }
					}
				val = (match) ? 16 : 21; break;
				}
			else
			if(parse[j]==AN_Z)
				{
				int xover = 0;
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
                                for(k=j+1;k<4;k++)
                                        {
                                        if(parse[k]!=AN_Z)
                                                {
                                                if(parse[k]==AN_X)
                                                        {
                                                        xover = 1;
                                                        }
                                                        else
                                                        {
                                                        char *thisbyt = parse + i + k;
                                                        char *lastbyt = newbuff + 3 + nbits - 1;
                                                        if((lastbyt - thisbyt) >= 0) match = 0;
                                                        }
                                                break;
                                                }
                                        }

				if(xover) val = 21;
				else val = (match) ? 17 : 22;
				break;
				}
			else
			if(parse[j]==AN_W)
				{
				int xover = 0;
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
                                for(k=j+1;k<4;k++)
                                        {
                                        if(parse[k]!=AN_W)
                                                {
                                                if(parse[k]==AN_X)
                                                        {
                                                        xover = 1;
                                                        }
                                                        else
                                                        {
                                                        char *thisbyt = parse + i + k;
                                                        char *lastbyt = newbuff + 3 + nbits - 1;
                                                        if((lastbyt - thisbyt) >= 0) match = 0;
                                                        }
                                                break;
                                                }
                                        }

				if(xover) val = 21;
				else val = (match) ? 18 : 23;
				break;
				}
			else
			if(parse[j]==AN_U)
				{
				int xover = 0;
				int match = (j==0) || ((parse + i + j) == (newbuff + 3));
				int k;
                                for(k=j+1;k<4;k++)
                                        {
                                        if(parse[k]!=AN_U)
                                                {
                                                if(parse[k]==AN_X)
                                                        {
                                                        xover = 1;
                                                        }
                                                        else
                                                        {
                                                        char *thisbyt = parse + i + k;
                                                        char *lastbyt = newbuff + 3 + nbits - 1;
                                                        if((lastbyt - thisbyt) >= 0) match = 0;
                                                        }
                                                break;
                                                }
                                        }

				if(xover) val = 21;
				else val = (match) ? 19 : 24;
				break;
				}
			else
			if(parse[j]==AN_DASH)
				{
				int xover = 0;
				int k;
                                for(k=j+1;k<4;k++)
                                        {
                                        if(parse[k]!=AN_DASH)
                                                {
                                                if(parse[k]==AN_X)
                                                        {
                                                        xover = 1;
                                                        }
                                                break;
                                                }
                                        }

				if(xover) val = 21;
				else val = 20;
				break;
				}
			}

		*(pnt++)=AN_HEX_STR[val];

		parse+=4;
		}

	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if(flags&TR_OCT)
	{
	char *parse;

	len=(nbits/3)+2+1;		/* #xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='#'; }

	parse=(flags&TR_RJUSTIFY)
		?(newbuff+((nbits%3)?(nbits%3):3))
		:(newbuff+3);
	cvt_gray(flags,parse,(flags&TR_RJUSTIFY)?(((nbits+2)/3)*3):nbits);

	for(i=0;i<nbits;i+=3)
		{
		unsigned char val;

		val=0;
		for(j=0;j<3;j++)
			{
			val<<=1;

			if(parse[j]==AN_X) { val=8; break; }
			if(parse[j]==AN_Z) { val=9; break; }
			if(parse[j]==AN_W) { val=10; break; }
			if(parse[j]==AN_U) { val=11; break; }
			if(parse[j]==AN_DASH) { val=12; break; }

			if((parse[j]==AN_1)||(parse[j]==AN_H)) { val|=1; }
			}

		*(pnt++)=AN_OCT_STR[val];
		parse+=3;
		}

	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if(flags&TR_BIN)
	{
	char *parse;

	len=(nbits/1)+2+1;		/* %xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(GLOBALS->show_base) { *(pnt++)='%'; }

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	for(i=0;i<nbits;i++)
		{
		*(pnt++)=AN_STR[(int)(*(parse++))];
		}

	*(pnt)=0x00; /* scan build : remove dead increment */
	}
else if(flags&TR_SIGNED)
	{
	char *parse;
	TimeType val = 0;
	unsigned char fail=0;

	len=21;	/* len+1 of 0x8000000000000000 expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

        if((parse[0]==AN_1)||(parse[0]==AN_H))
                { val = LLDescriptor(-1); }
        else
        if((parse[0]==AN_0)||(parse[0]==AN_L))
                { val = LLDescriptor(0); }
        else
                { fail = 1; }

        if(!fail)
	for(i=1;i<nbits;i++)
		{
		val<<=1;

                if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
                else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
		}

	if(!fail)
		{
		if((flags&TR_FPDECSHIFT)&&(t->t_fpdecshift))
			{
			cvt_fpsdec(t, val, os, len, nbits);
			}
			else
			{
                        if(!(flags&TR_TIME))
                                {
                                sprintf(os, TTFormat, val);
                                }
                                else
                                {
                                free_2(os);
                                os = calloc_2(1, 128);
                                reformat_time(os, val, GLOBALS->time_dimension);
                                }
			}
		}
		else
		{
		strcpy(os, "XXX");
		}
	}
else if(flags&TR_REAL)
	{
	char *parse;

	if((nbits==64) || (nbits == 32))
		{
		UTimeType utt = LLDescriptor(0);
		double d;
		float f;
		uint32_t utt_32;

		parse=newbuff+3;
		cvt_gray(flags,parse,nbits);

		for(i=0;i<nbits;i++)
			{
			char ch = AN_STR[(int)(*(parse++))];
			if ((ch=='0')||(ch=='1'))
				{
				utt <<= 1;
				if(ch=='1')
					{
					utt |= LLDescriptor(1);
					}
				}
				else
				{
				goto rl_go_binary;
				}
			}

		os=/*pnt=*/(char *)calloc_2(1,64); /* scan-build */

		if(nbits==64)
			{
			memcpy(&d, &utt, sizeof(double));
			dpr_e16(os, d); /* sprintf(os, "%.16g", d); */
			}
			else
			{
			utt_32 = utt;
			memcpy(&f, &utt_32, sizeof(float));
			sprintf(os, "%f", f);
			}
		}
		else
		{
rl_go_binary:	len=(nbits/1)+2+1;		/* %xxxxx */
		os=pnt=(char *)calloc_2(1,len);
		if(GLOBALS->show_base) { *(pnt++)='%'; }

		parse=newbuff+3;
		cvt_gray(flags,parse,nbits);

		for(i=0;i<nbits;i++)
			{
			*(pnt++)=AN_STR[(int)(*(parse++))];
			}

		*(pnt)=0x00; /* scan build : remove dead increment */
		}
	}
else	/* decimal when all else fails */
	{
	char *parse;
	UTimeType val=0;
	unsigned char fail=0;

	len=21; /* len+1 of 0xffffffffffffffff expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	for(i=0;i<nbits;i++)
		{
		val<<=1;

                if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
                else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
		}

	if(!fail)
		{
		if((flags&TR_FPDECSHIFT)&&(t->t_fpdecshift))
			{
			cvt_fpsudec(t, val, os, len);
			}
			else
			{
                        if(!(flags&TR_TIME))
                                {
				sprintf(os, UTTFormat, val);
                                }
                                else
                                {
                                free_2(os);
                                os = calloc_2(1, 128);
                                reformat_time(os, val, GLOBALS->time_dimension);
                                }
			}
		}
		else
		{
		strcpy(os, "XXX");
		}
	}

free_2(newbuff);
return(os);
}


static char *dofilter(Trptr t, char *s)
{
GLOBALS->xl_file_filter[t->f_filter] = xl_splay(s, GLOBALS->xl_file_filter[t->f_filter]);

if(!strcasecmp(s, GLOBALS->xl_file_filter[t->f_filter]->item))
	{
	free_2(s);
	s = malloc_2(strlen(GLOBALS->xl_file_filter[t->f_filter]->trans) + 1);
	strcpy(s, GLOBALS->xl_file_filter[t->f_filter]->trans);
	}

if((*s == '?') && (!GLOBALS->color_active_in_filter))
	{
	char *s2a;
	char *s2 = strchr(s+1, '?');
	if(s2)
		{
		s2++;
		s2a = malloc_2(strlen(s2)+1);
		strcpy(s2a, s2);
		free_2(s);
		s = s2a;
		}
	}

return(s);
}

static char *edofilter(Trptr t, char *s)
{
if(t->flags & TR_ENUM)
	{
	int filt = t->e_filter - 1;

#ifdef _WAVE_HAVE_JUDY
	PPvoid_t pv = JudyHSGet(GLOBALS->xl_enum_filter[filt], s, strlen(s));
	if(pv)
		{
		free_2(s);
		s = malloc_2(strlen(*pv) + 1);
		strcpy(s, *pv);
		}
#else
	GLOBALS->xl_enum_filter[filt] = xl_splay(s, GLOBALS->xl_enum_filter[filt]);

	if(!strcasecmp(s, GLOBALS->xl_enum_filter[filt]->item))
		{
		free_2(s);
		s = malloc_2(strlen(GLOBALS->xl_enum_filter[filt]->trans) + 1);
		strcpy(s, GLOBALS->xl_enum_filter[filt]->trans);
		}
#endif
	else
		{
		char *zerofind = s;
		char *dst = s, *src;
		while(*zerofind == '0') zerofind++;
		if(zerofind != s)
			{
			src = (!*zerofind) ? (zerofind-1) : zerofind;
			while(*src)
				{
				*(dst++) = *(src++);
				}
			*dst = 0;
			}
		}
	}

return(s);
}

static char *pdofilter(Trptr t, char *s)
{
struct pipe_ctx *p = GLOBALS->proc_filter[t->p_filter];
char buf[1025];
int n;

if(p)
	{
#if !defined _MSC_VER && !defined __MINGW32__
	fputs(s, p->sout);
	fputc('\n', p->sout);
	fflush(p->sout);

	buf[0] = 0;

	n = fgets(buf, 1024, p->sin) ? strlen(buf) : 0;
	buf[n] = 0;
#else
	{
	BOOL bSuccess;
	DWORD dwWritten, dwRead;

	WriteFile(p->g_hChildStd_IN_Wr, s, strlen(s), &dwWritten, NULL);
	WriteFile(p->g_hChildStd_IN_Wr, "\n", 1, &dwWritten, NULL);

	for(n=0;n<1024;n++)
		{
		do 	{
			bSuccess = ReadFile(p->g_hChildStd_OUT_Rd, buf+n, 1, &dwRead, NULL);
			if((!bSuccess)||(buf[n]=='\n'))
				{
				goto ex;
				}

			} while(buf[n]=='\r');
		}
ex:	buf[n] = 0;
	}

#endif

	if(n)
		{
		if(buf[n-1] == '\n') { buf[n-1] = 0; n--; }
		}

	if(buf[0])
		{
		free_2(s);
		s = malloc_2(n + 1);
		strcpy(s, buf);
		}
	}

if((*s == '?') && (!GLOBALS->color_active_in_filter))
	{
	char *s2a;
	char *s2 = strchr(s+1, '?');
	if(s2)
		{
		s2++;
		s2a = malloc_2(strlen(s2)+1);
		strcpy(s2a, s2);
		free_2(s);
		s = s2a;
		}
	}
return(s);
}


char *convert_ascii_vec(Trptr t, char *vec)
{
char *s = convert_ascii_vec_2(t, vec);

if(!(t->f_filter|t->p_filter|t->e_filter))
	{
	}
	else
	{
	if(t->e_filter)
		{
		s = edofilter(t, s);
		}
	else
	if(t->f_filter)
		{
		s = dofilter(t, s);
		}
		else
		{
		s = pdofilter(t, s);
		}
	}

return(s);
}

char *convert_ascii(Trptr t, vptr v)
{
char *s;

if((!t->t_filter_converted) && (!(v->flags & HIST_STRING)))
	{
	s = convert_ascii_2(t, v);
	}
	else
	{
	s = strdup_2((char *)v->v);

	if((*s == '?') && (!GLOBALS->color_active_in_filter))
	        {
	        char *s2a;
	        char *s2 = strchr(s+1, '?');
	        if(s2)
	                {
	                s2++;
	                s2a = malloc_2(strlen(s2)+1);
	                strcpy(s2a, s2);
	                free_2(s);
	                s = s2a;
	                }
	        }
	}

if(!(t->f_filter|t->p_filter|t->e_filter))
	{
	}
	else
	{
	if(t->e_filter)
		{
		s = edofilter(t, s);
		}
	else
	if(t->f_filter)
		{
		s = dofilter(t, s);
		}
		else
		{
		s = pdofilter(t, s);
		}
	}

return(s);
}


/*
 * convert trptr+hptr vectorstring into a real
 */
double convert_real_vec(Trptr t, char *vec)
{
Ulong flags;
int nbits;
char *bits;
char *pnt, *newbuff;
int i;
const char *xtab;
double mynan = strtod("NaN", NULL);
double retval = mynan;

static const char xfwd[AN_COUNT]= AN_NORMAL  ;
static const char xrev[AN_COUNT]= AN_INVERSE ;

flags=t->flags;

nbits=t->n.nd->msi-t->n.nd->lsi;
if(nbits<0)nbits=-nbits;
nbits++;

if(vec)
        {
        bits=vec;
        if(*vec>AN_MSK)              /* convert as needed */
        for(i=0;i<nbits;i++)
                {
		vec[i] = cvt_table[(unsigned char)vec[i]];
                }
        }
        else
        {
        pnt=bits=wave_alloca(nbits);
        for(i=0;i<nbits;i++)
                {
                *pnt++=AN_X;
                }
        }


if(flags&TR_INVERT)
        {
        xtab = xrev;
        }
        else
        {
        xtab = xfwd;
        }

newbuff=(char *)malloc_2(nbits+6); /* for justify */
if(flags&TR_REVERSE)
	{
	char *fwdpnt, *revpnt;

	fwdpnt=bits;
	revpnt=newbuff+nbits+6;
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(--revpnt)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	}
	else
	{
	char *fwdpnt, *fwdpnt2;

	fwdpnt=bits;
	fwdpnt2=newbuff;
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	}

if(flags&TR_REAL)
	{
	if((nbits==64) || (nbits == 32)) /* fail (NaN) otherwise */
		{
		char *parse;
		UTimeType val=0;
		unsigned char fail=0;
		uint32_t val_32;

		parse=newbuff+3;
		for(i=0;i<nbits;i++)
			{
			val<<=1;

	        if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
	        else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
			}
		if(!fail)
			{
	                if(nbits==64)
        	                {
				memcpy(&retval, &val, sizeof(double)); /* otherwise strict-aliasing rules problem if retval = *(double *)&val; */
        	                }
        	                else
        	                {
				float f;
        	                val_32 = val;
				memcpy(&f, &val_32, sizeof(float)); /* otherwise strict-aliasing rules problem if retval = *(double *)&val; */
				retval = (double)f;
        	                }
			}
		}
	}
    else
    {
	if(flags&TR_SIGNED)
		{
		char *parse;
		TimeType val = 0;
		unsigned char fail=0;

		parse=newbuff+3;
		cvt_gray(flags,parse,nbits);

	        if((parse[0]==AN_1)||(parse[0]==AN_H))
	                { val = LLDescriptor(-1); }
	        else
	        if((parse[0]==AN_0)||(parse[0]==AN_L))
	                { val = LLDescriptor(0); }
	        else
	                { fail = 1; }

	        if(!fail)
		for(i=1;i<nbits;i++)
			{
			val<<=1;

	                if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
	                else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
			}
		if(!fail)
			{
			retval = val;
			}
		}
	else	/* decimal when all else fails */
		{
		char *parse;
		UTimeType val=0;
		unsigned char fail=0;

		parse=newbuff+3;
		cvt_gray(flags,parse,nbits);

		for(i=0;i<nbits;i++)
			{
			val<<=1;

	                if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
	                else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
			}
		if(!fail)
			{
			retval = val;
			}
		}
	}

free_2(newbuff);
return(retval);
}



/*
 * convert trptr+vptr into a real
 */
double convert_real(Trptr t, vptr v)
{
Ulong flags;
int nbits;
unsigned char *bits;
char *newbuff;
int i;
const char *xtab;
double mynan = strtod("NaN", NULL);
double retval = mynan;

static const char xfwd[AN_COUNT]= AN_NORMAL  ;
static const char xrev[AN_COUNT]= AN_INVERSE ;

flags=t->flags;
nbits=t->n.vec->nbits;
bits=v->v;

if(flags&TR_INVERT)
        {
        xtab = xrev;
        }
        else
        {
        xtab = xfwd;
        }

newbuff=(char *)malloc_2(nbits+6); /* for justify */
if(flags&TR_REVERSE)
	{
	char *fwdpnt, *revpnt;

	fwdpnt=(char *)bits;
	revpnt=newbuff+nbits+6;
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(--revpnt)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(--revpnt)=xtab[0]; */
	for(i=0;i<3;i++) *(--revpnt)=xfwd[0];
	}
	else
	{
	char *fwdpnt, *fwdpnt2;

	fwdpnt=(char *)bits;
	fwdpnt2=newbuff;
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	for(i=0;i<nbits;i++)
		{
		*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
		}
	/* for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0]; */
	for(i=0;i<3;i++) *(fwdpnt2++)=xfwd[0];
	}


if(flags&TR_SIGNED)
	{
	char *parse;
	TimeType val = 0;
	unsigned char fail=0;

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	if((parse[0]==AN_1)||(parse[0]==AN_H))
		{ val = LLDescriptor(-1); }
	else
	if((parse[0]==AN_0)||(parse[0]==AN_L))
		{ val = LLDescriptor(0); }
	else
		{ fail = 1; }

	if(!fail)
	for(i=1;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
		else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
		}

	if(!fail)
		{
		retval = val;
		}
	}
else	/* decimal when all else fails */
	{
	char *parse;
	UTimeType val=0;
	unsigned char fail=0;

	parse=newbuff+3;
	cvt_gray(flags,parse,nbits);

	for(i=0;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==AN_1)||(parse[i]==AN_H)) {  val|=LLDescriptor(1); }
		else if((parse[i]!=AN_0)&&(parse[i]!=AN_L)) { fail=1; break; }
		}

	if(!fail)
		{
		retval = val;
		}
	}

free_2(newbuff);
return(retval);
}

