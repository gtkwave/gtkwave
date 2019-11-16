/*
 to compile: 
 gcc -O2 -o wlf2vcd wlf2vcd.c -L ./lib -I ./include/ ./lib/libwlf.a ./lib/libtcl8.5.a -lm -lsqlite3 -lz ./lib/libucdb.a

 Much faster version of wlf2vcd as compared to one bundled with Questa, etc.
 Requires libs and headers from ModelSim.  Some libwlf.a versions don't need libucdb.a.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include "wlf_api.h"

/* skips using wlfValueToString() and determines string directly from value representation */
#define BYPASS_USING_VALUE_TO_STRING

/* definition of WlfVreg from wlf_api.h */
#define WLF2VCD_MVL4		"01zx"
#define WLF2VCD_MVL9		"ux01zwlh-"
static const char *wlf_mvl9[9] = {"'U'", "'X'", "'0'", "'1'", "'Z'", "'W'", "'L'", "'H'", "'-'"};

typedef struct WlfGlobalContext 
{
WlfPackId pack;
WlfTime64 old_time;
char *prev_hier;
int prev_hier_len;
unsigned int vcdid_added;

unsigned int num_scopes;
unsigned int num_symbols;
unsigned int max_bits;
unsigned int num_aliases;

unsigned is_vhdl : 1;

unsigned char *value_string;
int *archive_number;
WlfSymbolId *archive_sym;
} WlfGlobalContext;

WlfGlobalContext wgc;

static void AddSymbolToCB(
        WlfSymbolId sym,
        unsigned int vcdid,
                 int num_bits,
        unsigned int is_real,
        unsigned int is_vbit,
        unsigned int is_vhbit,
        unsigned int is_vreg
        );

/* structure to hold callback data */
typedef struct cbData 
{
#ifdef BYPASS_USING_VALUE_TO_STRING
void *pv;
#endif
WlfValueId val;
unsigned int vcdid;
unsigned int num_bits;
unsigned is_real : 1;
unsigned is_vbit : 1;
unsigned is_vhbit : 1;
unsigned is_vreg : 1;
} cbData;


char *genVcdID(char *buf, unsigned int value)
{
char *pnt = buf;
unsigned int vmod;

/* zero is illegal for a value...it is assumed they start at one */
for(;;)
        {
        if((vmod = (value % 94)))
                {
                *(pnt++) = (char)(vmod + 32);
                }
                else
                {
                *(pnt++) = '~'; value -= 94;
                }
        value = value / 94;
        if(!value) { break; }
        }

*pnt = 0;
return(buf);
}

      
/****************************************************************************** 
//  errorExit 
//  Prints error message to stderr and exits. 
******************************************************************************/  
static void errorExit(char *funcName)  
{  
int errorNum = wlfErrorNum();  
fprintf(stderr, "Error: %s - %d - %s\n", funcName, wlfErrorNum(), wlfErrorMsg());  
exit(errorNum);  
}  
      
      
/****************************************************************************** 
//  countSubElements 
//  Recursively enumerates the context tree information
******************************************************************************/  
static void countSubElements(WlfSymbolId top)
{  
WlfIterId iter;  
WlfSymbolId sym;  
int cnt;  
WlfDataType vtyp;

/* create an iterator to retrieve children of top */  
iter = wlfSymChildren64(top, wlfSelAll);  
if(iter == NULL) return;  
      
/* iterate through the children */  
while ((sym = wlfIterate(iter)) != NULL) 
	{  
     	cnt = wlfSymPropInt(sym, WLF_PROP_SUBELEMENT_COUNT);  
    	WlfSymbolSel64 typ = wlfSymPropSymbolSel64(sym, WLF_PROP_SYMBOL_TYPE);

	if(typ & wlfSelVhdlScopes) 
		{
		wgc.is_vhdl = 1;
		}

	if(typ & (wlfSelVhdlScopes | wlfSelVlogScopes))
		{
		wgc.num_scopes++;
		}
		else
            	{  
		WlfTypeId wid = wlfSymPropTypeId(sym, WLF_PROP_TYPE_ID);
		vtyp = wlfTypePropDataType(wid, WLF_TYPE_TYPE);

		if(vtyp == wlfTypeRecord)
			{
			// fprintf(stderr, "wlfTypeRecord %d\n", cnt);
			wgc.num_scopes++;
			}
		else
		if(vtyp != wlfTypeArray) /* still possibly have to recurse */
			{
			int rgh = wlfTypePropInt(wid, WLF_TYPE_ARRAY_RIGHT);
			int lft = wlfTypePropInt(wid, WLF_TYPE_ARRAY_LEFT);
			int len = wlfTypePropInt(wid, WLF_TYPE_ARRAY_LENGTH);

			if(vtyp == wlfTypeScalar)
				{
				lft = 31; rgh = 0; len = 32;
				}

			wgc.num_symbols++;
			if(len > wgc.max_bits) wgc.max_bits = len;
			}
    		} 
      
	/* recurse through the children, but block out bitblasted children (except vhdl) */
	if((cnt<=0) || (wgc.is_vhdl))
		{
            	countSubElements(sym);  
		}
        }  

wlfIteratorDestroy(iter);  
}  


/****************************************************************************** 
//  printSubElements 
//  Recursively prints the context tree information starting at the top and 
//  adds elements to the symbol callback iterator. 
******************************************************************************/  
static void printSubElements(WlfSymbolId top)
{  
WlfIterId iter;  
WlfSymbolId sym;  
int cnt;  
char *name;  
char vcdid_str[10];      

/* create an iterator to retrieve children of top */  
iter = wlfSymChildren64(top, wlfSelAll);  
if(iter == NULL) return;  
      
/* iterate through the children */  
while ((sym = wlfIterate(iter)) != NULL) 
	{  
//	printf("\n");
//	char *lib = wlfSymPropString(sym, WLF_PROP_LIBRARY_NAME);
//	printf("lib: '%s'\n", lib);
//	char *sor= wlfSymPropString(sym, WLF_PROP_SOURCE_NAME);
//	printf("sor: '%s'\n", sor);
//	char *pri = wlfSymPropString(sym, WLF_PROP_PRIMARY_NAME);	// component type
//	printf("pri: '%s'\n", pri);
//	char *sec = wlfSymPropString(sym, WLF_PROP_SECONDARY_NAME);	// secondary component type?  (VHDL?)
//	printf("sec: '%s'\n", sec);
//	char *exp= wlfSymPropString(sym, WLF_PROP_EXPRESSION);
//	printf("exp: '%s'\n", exp);
//	int ssor = wlfSymPropInt(sym, WLF_PROP_SYMBOL_SOURCE);
//	printf("sym source: '%d'\n", ssor);
//	char *apath = wlfSymPropString(sym, WLF_PROP_SYMBOL_ABSOLUTE_PATH);
//	printf("abs path: '%s'\n", apath);
//	char *dname = wlfSymPropString(sym, WLF_PROP_SYMBOL_DATASET_NAME);
//	printf("dataset name: '%s'\n", dname);
//	char *pat = wlfSymPropString(sym, WLF_PROP_SYMBOL_PATH);
//	printf("path: '%s'\n", pat);
      
     	cnt = wlfSymPropInt(sym, WLF_PROP_SUBELEMENT_COUNT);  
    	WlfSymbolSel64 typ = wlfSymPropSymbolSel64(sym, WLF_PROP_SYMBOL_TYPE);

	int this_arch;
	int arch = this_arch = wlfSymPropInt(sym, WLF_PROP_ARCHIVE_NUMBER);
	int alias_ok = 0;
	if(arch < 0)
		{
		if(cnt > 0)
			{
			WlfIterId iter2 = wlfSymChildren64(sym, wlfSelAll);
			if(iter2)
				{
				WlfSymbolId sym2 = wlfIterate(iter2);
				if(sym2)
					{
					arch = wlfSymPropInt(sym2, WLF_PROP_ARCHIVE_NUMBER);
					/* NOTE: alias comparisons should really look across ALL bits, not just the first */
					}
				wlfIteratorDestroy(iter2);
				}
			}
		}

	if(arch >= 0)
		{
		WlfSymbolId archive_sym = wgc.archive_sym[arch];
		if(archive_sym)
			{
			int archive_arch = wlfSymPropInt(archive_sym, WLF_PROP_ARCHIVE_NUMBER);
			if((this_arch >= 0) && (archive_arch >= 0))
				{
				alias_ok = (this_arch == archive_arch);
				}
			else
				{
				WlfIterId this_iter = wlfSymChildren64(sym, wlfSelAll);
				WlfIterId archive_iter = wlfSymChildren64(archive_sym, wlfSelAll);
				WlfSymbolId this_child;
				WlfSymbolId archive_child;
	
				alias_ok = 1;
				for(;;)
					{
					this_child = wlfIterate(this_iter);
					archive_child = wlfIterate(archive_iter);
	
					if(this_child && archive_child)
						{
						int this_child_arch    = wlfSymPropInt(this_child, WLF_PROP_ARCHIVE_NUMBER);
						int archive_child_arch = wlfSymPropInt(archive_child, WLF_PROP_ARCHIVE_NUMBER);
						if(this_child_arch != archive_child_arch)
							{
							alias_ok = 0;
							break;
							}
						}
					else
					if(!this_child && !archive_child)
						{
						break;
						}
					else
						{
						alias_ok = 0;
						break;
						}
					}
	
				wlfIteratorDestroy(archive_iter);
				wlfIteratorDestroy(this_iter);
				}
			}
		}

	unsigned int is_real = 0;
	unsigned int is_vbit = 0;
	unsigned int is_vhbit = 0;
	unsigned int num_bits = 0;
	unsigned int is_vreg = 0;
	char *vartype = NULL;
	char *scopetype = "module";

	switch(typ)
		{
		/* wlfSelVhdlScopes */
		case wlfSelArchitecture:	scopetype = "architecture"; break;
		case wlfSelBlock:		scopetype = "block"; break;
		case wlfSelGenerate:		scopetype = "generate"; break;
		case wlfSelPackage:		scopetype = "package"; break;
		case wlfSelSubprogram:		scopetype = "subprogram"; break;
		case wlfSelForeign:		scopetype = "foreign"; break;

		/* wlfSelVlogScopes */
		case wlfSelModule:		scopetype = "module"; break;
		case wlfSelTask:		scopetype = "task"; break;
		/* case wlfSelBlock: (same as VHDL's) */
		case wlfSelFunction:		scopetype = "function"; break;
		case wlfSelStatement:		scopetype = "statement"; break;
		case wlfSelSVCovergroup:	scopetype = "covergroup"; break;
		case wlfSelSVCoverpoint:	scopetype = "coverpoint"; break;
		case wlfSelSVCross:		scopetype = "cross"; break;
		case wlfSelSVClass:		scopetype = "class"; break;
		case wlfSelSVParamClass:	scopetype = "paramclass"; break;
		case wlfSelSVInterface:		scopetype = "interface"; break;
		case wlfSelVlPackage:		scopetype = "package"; break;
		case wlfSelVlGenerateBlock:	scopetype = "generate"; break;
		case wlfSelAssertionScope:	scopetype = "assertionscope"; break;
		case wlfSelClockingBlock:	scopetype = "clockingblock"; break;
		case wlfSelVlTypedef:		scopetype = "typedef"; break;

		/* wlfSelVlogVars */
		case wlfSelParameter:	vartype = "parameter"; is_vbit = 1; break;
		case wlfSelReg:		vartype = "reg"; is_vreg = 1; break;
		case wlfSelInteger:	vartype = "integer"; is_vbit = 1; break;
		case wlfSelTime:	vartype = "time"; is_vbit = 1; break;
		case wlfSelReal:	vartype = "real"; is_real = 1; break;

		case wlfSelSpecparam:
		case wlfSelMemory:	break;
		case wlfSelNamedEvent:	vartype = "event"; break;

		/* wlfSelHdlSignals */
		case wlfSelSignal:	vartype = "wire"; break;
					break;

		case wlfSelNet:		vartype = "wire"; break;

		/* wlfSelHdlVars */
		case wlfSelVariable:
		case wlfSelConstant:
		case wlfSelGeneric:
		case wlfSelAlias:

		default:	fprintf(stderr, "Type: %d\n", typ); exit(255);
				break;
		}

	WlfModeSel ptyp = wlfSymPropModeSel(sym, WLF_PROP_PORT_TYPE);

       	name = wlfSymPropString(sym, WLF_PROP_SYMBOL_PATH);  
	while(wgc.prev_hier_len && strncmp(name, wgc.prev_hier, wgc.prev_hier_len))
		{
		wgc.prev_hier[wgc.prev_hier_len - 1] = 0;
		char *pmod = strrchr(wgc.prev_hier, '/');
		char *dmod = strrchr(wgc.prev_hier, '.');
		if(dmod && (dmod - pmod > 0)) pmod = dmod;
		*(pmod + 1) = 0;
		wgc.prev_hier_len = pmod - wgc.prev_hier + 1;
		
		printf("$upscope $end\n");
		}

	WlfTypeId wid = wlfSymPropTypeId(sym, WLF_PROP_TYPE_ID);
	WlfDataType vtyp = wlfTypePropDataType(wid, WLF_TYPE_TYPE);

	if((typ & (wlfSelVhdlScopes | wlfSelVlogScopes)) || (vtyp == wlfTypeRecord))
		{
		char *ls = strrchr(name, '/');
		char *ds = strrchr(name, '.');
		if(ds && (ds - ls > 0)) ls = ds;

		char *sname = ls ? (ls+1) : name;
		printf("$scope %s %s $end\n", scopetype, sname);

		if(wgc.prev_hier)
			{
			free(wgc.prev_hier);
			}

		int hlen = strlen(name);
		wgc.prev_hier = malloc(hlen + 1 + 1);
		memcpy(wgc.prev_hier, name, hlen);
		wgc.prev_hier[hlen] = (vtyp == wlfTypeRecord) ? '.' : '/';
		wgc.prev_hier[(wgc.prev_hier_len = hlen + 1)] = 0;
		}
		else
            	{  
		if(vtyp != wlfTypeArray) /* still possibly have to recurse, depends on value of cnt below */
			{
			int rgh = wlfTypePropInt(wid, WLF_TYPE_ARRAY_RIGHT);
			int lft = wlfTypePropInt(wid, WLF_TYPE_ARRAY_LEFT);
			int len = wlfTypePropInt(wid, WLF_TYPE_ARRAY_LENGTH);

                        if(vtyp == wlfTypeScalar)
                                {
                                lft = 31; rgh = 0; len = 32;
				is_vbit = 1; is_vhbit = 0; is_real = 0; is_vreg = 0;
                                }

			if(vtyp == wlfTypeEnum)
				{
				char **enumLiterals;
				int i, count;
				wlfEnumLiterals(wid, &enumLiterals, &count);
				if(count == 9)
					{
					for(i=0;i<9;i++)
						{
						if(strcmp(enumLiterals[i], wlf_mvl9[i]))
							{
							break;
							}
						}
					if(i == 9)
						{
						is_vhbit = 1;
						is_vbit = 0;
						lft = rgh = 0;
						len = 1;
						}
					}
				}

			if((is_real) || (vtyp == wlfTypeReal) || (vtyp == wlfTypeVlogReal))
				{
				is_real = 1;
				len = lft = rgh = 0;
				vartype = "real";
				if(64 > wgc.max_bits) wgc.max_bits = 64;
				}

	            	name = wlfSymPropString(sym, WLF_PROP_SYMBOL_PATH);  

			/* add the symbol to the callback iterator if not an alias */  
			unsigned int vcdid = 0;

			if((arch >= 0) && (alias_ok))
				{
				vcdid = wgc.archive_number[arch];
				}

			if(!vcdid)
				{
		            	AddSymbolToCB(sym, ++wgc.vcdid_added, num_bits = len, is_real, is_vbit, is_vhbit, is_vreg);  
				vcdid = wgc.vcdid_added;
				if(arch >= 0)
					{
					wgc.archive_number[arch] = vcdid;
					wgc.archive_sym[arch] = sym;
					}
				}
				else
				{
				if(arch >= 0)
					{
					wgc.archive_number[arch] = vcdid;
					wgc.archive_sym[arch] = sym;
					}

				wgc.num_aliases++;
				}

			char *rsl = strrchr(name, '.');
			if(!rsl) rsl = strrchr(name, '/');

			char *lp = strrchr(rsl, '(');
			char *rp = strrchr(rsl, ')');
			char *lp2 = NULL, *rp2 = NULL;
			if(lp && rp)
				{
				*lp = '['; *rp = ']';

				lp2 = strrchr(rsl, '(');
				rp2 = strrchr(rsl, ')');
				if(lp2 && rp2)
					{
					*lp2 = '['; *rp2 = ']';
					}
					else
					{
					lp2 = rp2 = NULL;
					}
				}

			if((lft != rgh) && (!is_vbit) && (!is_real))
				{
				printf("$var %s %d %s %s [%d:%d] $end\n", vartype, len, genVcdID(vcdid_str, vcdid), rsl+1, lft, rgh);
				}
				else
				{
				if(cnt && (!is_vbit) && (!is_real))
					{
					printf("$var %s %d %s %s [%d] $end\n", vartype, len, genVcdID(vcdid_str, vcdid), rsl+1, lft);
					}
					else
					{
					printf("$var %s %d %s %s $end\n", vartype, len, genVcdID(vcdid_str, vcdid), rsl+1);
					}
				}

			if(lp && rp)
				{
				*lp = '('; *rp = ')';
				}
			if(lp2 && rp2)
				{
				*lp2 = '('; *rp2 = ')';
				}
			}
    		} 
      
        /* recurse through the children, but block out bitblasted children (except vhdl) */  
	if((cnt<=0) || (wgc.is_vhdl))
		{
            	printSubElements(sym);  
		}
        }  

/* status = */ wlfIteratorDestroy(iter);  
}  
      
      
/****************************************************************************** 
//  timeCb 
//  This function is called by the WLF reader when time advances. 
//  Unused in the program.
******************************************************************************/  
static WlfCallbackResponse timeCb(   
	void *cbData,   
        WlfTime64 oldTime,   
        int oldDelta)  
{  
return(WLF_CONTINUE_SCAN);  
}  

      
/****************************************************************************** 
//  sigCb 
//  This function prints the time, the name of the signal, and its value. 
//  This function is called by the WLF reader as events occur on  
//  registered signals.  This function was registered with each signal  
//  in AddSymbolToCB().
******************************************************************************/  
#ifdef BYPASS_USING_VALUE_TO_STRING

/* faster version not using wlfValueToString() */

static WlfCallbackResponse sigCb(   
        void *data,   
        WlfCallbackReason reason)  
{  
WlfValueId v = ((cbData*) data)->val;  
WlfTime64 time;  
int i;
char vbuf[16];
      
if(reason == WLF_ENDLOG)  
	{
	wlfValueDestroy(v);
	free(data); /* no longer need cbData struct as we're iterating through the end log */
      	return(WLF_CONTINUE_SCAN);  
	}      

wlfPackTime(wgc.pack, &time);  

if(time != wgc.old_time)
	{
	printf("#"LLDSTR"\n", time);
	wgc.old_time = time;
	}

if(!((cbData*) data)->is_real)
	{
	unsigned int nbits = ((cbData*) data)->num_bits;
	unsigned char *pv = ((cbData*) data)->pv;
	char *value = wgc.value_string;

	if(((cbData*) data)->is_vhbit)
		{
		for(i=0;i<nbits;i++)
			{
			value[i] = WLF2VCD_MVL9[*(pv++)];
			}
		value[i] = 0;
		}
	else if(((cbData*) data)->is_vbit)
		{
		WlfVbit *ip = (WlfVbit *)pv;
		int bitrvs = (nbits - 1);
		for(i=0;i<nbits;i++)
			{
			int word_l = (i / ((sizeof((WlfVbit){0}).val) * 8));
			int bit = i & (((sizeof((WlfVbit){0}).val) * 8) - 1);
			int vl = (ip[word_l].val >> bit) & 1;	// 01 plane
			
			value[bitrvs--] = '0' | vl;
			}
		value[i] = 0;
		}
	else if(((cbData*) data)->is_vreg)
		{
		WlfVreg *ip = (WlfVreg *)pv;
		int bitrvs = (nbits - 1);
		for(i=0;i<nbits;i++)
			{
			int word_l = (i / ((sizeof((WlfVreg){0}).val) * 8));
			int bit = i & (((sizeof((WlfVreg){0}).val) * 8) - 1);
			int vl = (ip[word_l].val >> bit) & 1;	// 01 plane
			int vh = (ip[word_l].unk >> bit) & 1;	// zx plane
			
			value[bitrvs--] = WLF2VCD_MVL4[(vh << 1) | vl];
			}
		value[i] = 0;
		}
	else
		{
		if(pv)
			{
			for(i=0;i<nbits;i++)
				{
				value[i] = WLF2VCD_MVL4[pv[i] & 3]; // strength info is in top-order bits
				}
			value[i] = 0;
			}
			else
			{
			value[0] = '1';
			value[1] = 0;
			}
		}

	if(nbits == 1)
		{
		putc(value[0], stdout);
		puts(genVcdID(vbuf, ((cbData*) data)->vcdid));
		}
		else
		{
		putc('b', stdout);
		fputs(value, stdout);
		putc(' ', stdout);
		puts(genVcdID(vbuf, ((cbData*) data)->vcdid));
		}
	}
	else
	{
	double *d = ((cbData*) data)->pv;
	double dv;

	if(d)
		{
		dv = *d;
		}
		else
		{
		dv = 0.0;	/* probably should be NaN */
		}

	putc('r', stdout);
	fprintf(stdout, "%lg", dv);
	putc(' ', stdout);
	puts(genVcdID(vbuf, ((cbData*) data)->vcdid));
	}

return(WLF_CONTINUE_SCAN);  
}  

#else

/* slower version using wlfValueToString() */

static WlfCallbackResponse sigCb(   
        void *data,   
        WlfCallbackReason reason)  
{  
WlfValueId v = ((cbData*) data)->val;  
WlfTime64 time;  
char *value;  
char vbuf[16];
      
if(reason == WLF_ENDLOG)  
	{
	free(data); /* no longer need cbData struct as we're iterating through the end log */
      	return(WLF_CONTINUE_SCAN);  
	}      

wlfPackTime(wgc.pack, &time);  

if(time != wgc.old_time)
	{
	printf("#"LLDSTR"\n", time);
	wgc.old_time = time;
	}
          
if(!((cbData*) data)->is_real)
	{
	if((value=wlfValueToString(v, WLF_RADIX_BINARY, 0))==NULL)   // wgc.max_bits instead of 0 to specify explicitly
		{
		/* for events */
		value = "1";
		}

	if(((cbData*) data)->num_bits == 1)
		{
		putc(tolower(value[0]), stdout);
		puts(genVcdID(vbuf, ((cbData*) data)->vcdid));
		}
		else
		{
		putc('b', stdout);
		fputs(value, stdout);
		putc(' ', stdout);
		puts(genVcdID(vbuf, ((cbData*) data)->vcdid));
		}
	}
	else
	{
	if((value=wlfValueToString(v, WLF_RADIX_SYMBOLIC, 0))==NULL)  
		{
		value = "0";
		}

	putc('r', stdout);
	fputs(value, stdout);
	putc(' ', stdout);
	puts(genVcdID(vbuf, ((cbData*) data)->vcdid));
	}

return(WLF_CONTINUE_SCAN);  
}  

#endif


/****************************************************************************** 
//  AddSymbolToCB 
//  Adds a symbol to the callback iterator
******************************************************************************/  
static void AddSymbolToCB(   
        WlfSymbolId sym,   
	unsigned int vcdid,
	int num_bits,
	unsigned int is_real,
	unsigned int is_vbit,
	unsigned int is_vhbit,
	unsigned int is_vreg
        )  
{  
int status;  
cbData *pdata;  
      
if(wlfSymIsSymbolSelect64(sym, wlfSelAllSignals)) 
	{  
        WlfValueId val = wlfValueCreate(sym);  
        pdata = (cbData *) malloc(sizeof(cbData));  
        pdata->val = val;  
#ifdef BYPASS_USING_VALUE_TO_STRING
        pdata->pv = wlfValueGetValue(val);
#endif
	pdata->vcdid = vcdid;
	pdata->num_bits = (num_bits < 0) ? 0 : num_bits; // ?? how to fix properly
	pdata->is_real = is_real;
	pdata->is_vbit = is_vbit;
	pdata->is_vhbit = is_vhbit;
	pdata->is_vreg = is_vreg;
        status = wlfAddSignalEventCB(wgc.pack, sym, val, WLF_REQUEST_POSTPONED, sigCb, pdata);  
        if(status != WLF_OK)
		{  
                errorExit("AddSymbolToCB");  
		}
        }  
}  
      
      
/****************************************************************************** 
//  main 
******************************************************************************/  
int main(int argc, char **argv)  
{  
int status;  
int resolution;  
WlfFileId  wlfFile;  
WlfSymbolId top;  
WlfValueId val = NULL;  
WlfFileInfo  fileInfo;  
      
wgc.old_time = -1ULL;
wgc.prev_hier = NULL;
wgc.prev_hier_len = 0;
wgc.vcdid_added = 0;
wgc.is_vhdl = 0;

if(argc < 2) 
	{  
        fprintf(stderr,"Usage: %s <WLF-file>\n", argv[0]);  
        exit(1);  
        }  
      
/* Initialize the WLF api */  
status = wlfInit();  
if(status != WLF_OK)  
	{
        errorExit("wlfInit");  
	}
      
/* Open the WLF File */  
wlfFile = wlfFileOpen(argv[1], "vsim_wlf");  
if(wlfFile == NULL)
	{  
        errorExit("wlfFileOpen");  
	}
      
/* Check the API version */  
status = wlfFileInfo(wlfFile, &fileInfo);  
if(status != WLF_OK)  
	{
        errorExit("wlfFileInfo");  
	}
fprintf(stderr, "max archive num: %d\n", fileInfo.signalCount);
wgc.archive_number = calloc(fileInfo.signalCount + 1, sizeof(int));
wgc.archive_sym = calloc(fileInfo.signalCount + 1, sizeof(WlfSymbolId));
        
time_t walltime = fileInfo.creationTime;
printf("$date\n\t%s\n$end\n", asctime(localtime(&walltime)));
printf("$version\n\t%s\n$end\n", fileInfo.productName);

/* Get the simulator resolution */  
status = wlfFileResolution(wlfFile, &resolution);  
if(status != WLF_OK)
	{  
        errorExit("wlfFileResolution");  
	}
      
/* Retrieve and print out the top level context for this wlf file */  
top = wlfFileGetTopContext(wlfFile);  
if(top == NULL)
	{  
        errorExit("wlfFileGetTopContext");  
	}
      
char *tscale = NULL;
switch(resolution)
	{
	case WLF_TIME_RES_1FS:	tscale = "1fs"; break;
	case WLF_TIME_RES_10FS:	tscale = "10fs"; break;
	case WLF_TIME_RES_100FS:tscale = "100fs"; break;
	case WLF_TIME_RES_1PS:	tscale = "1ps"; break;
	case WLF_TIME_RES_10PS:	tscale = "10ps"; break;
	case WLF_TIME_RES_100PS:tscale = "100ps"; break;
	case WLF_TIME_RES_1NS:	tscale = "1ns"; break;
	case WLF_TIME_RES_10NS:	tscale = "10ns"; break;
	case WLF_TIME_RES_100NS:tscale = "100ns"; break;
	case WLF_TIME_RES_1US:	tscale = "1us"; break;
	case WLF_TIME_RES_10US:	tscale = "10us"; break;
	case WLF_TIME_RES_100US:tscale = "100us"; break;
	case WLF_TIME_RES_1MS:	tscale = "1ms"; break;
	case WLF_TIME_RES_10MS:	tscale = "10ms"; break;
	case WLF_TIME_RES_100MS:tscale = "100ms"; break;
	case WLF_TIME_RES_1SEC:	tscale = "1s"; break;
	case WLF_TIME_RES_10SEC:tscale = "10s"; break;
	case WLF_TIME_RES_100SEC:tscale ="100s"; break;
	default: tscale = "1ns"; break;
	}
printf("$timescale\n\t%s\n$end\n", tscale);
       
/* Create a callback context  */  
wgc.pack = wlfPackCreate();  
if(wgc.pack == NULL)  
	{
	errorExit("wlfPackCreate");  
	}
          
/* gather symbol information */  
countSubElements(top);  

/* print out symbol information */  
printSubElements(top);  
fprintf(stderr, "num_scopes: %d\n", wgc.num_scopes);
fprintf(stderr, "num_symbols: %d\n", wgc.num_symbols);
fprintf(stderr, "max_bits: %d\n", wgc.max_bits);
wgc.value_string = malloc(wgc.max_bits+1);
fprintf(stderr, "num_signals %d\n", wgc.num_symbols - wgc.num_aliases);
fprintf(stderr, "num_aliases %d\n", wgc.num_aliases);

while(wgc.prev_hier_len && strncmp("/", wgc.prev_hier, wgc.prev_hier_len))
	{ 
        wgc.prev_hier[wgc.prev_hier_len - 1] = 0;
        char *pmod = strrchr(wgc.prev_hier, '/');
        *(pmod + 1) = 0;
        wgc.prev_hier_len = pmod - wgc.prev_hier + 1;
 
        printf("$upscope $end\n");
        }

printf("$enddefinitions $end\n");
printf("$dumpvars\n");

free(wgc.archive_sym); wgc.archive_sym = NULL;
free(wgc.archive_number); wgc.archive_number = NULL;
      
/* Scan the data starting at time 0 through the end of file */  
status = wlfReadDataOverRange(wgc.pack, fileInfo.startTime, fileInfo.startDelta, fileInfo.lastTime, fileInfo.lastDelta, NULL, NULL, NULL);  
if(status != WLF_OK)  
	{
        errorExit("wlfReadDataOverRange");  
	}
      
/* free resources */
wlfPackDestroy(wgc.pack);
free(wgc.value_string); wgc.value_string = NULL;

/* close the file and release resources */  
status  = wlfFileClose(wlfFile);  
if(status != WLF_OK)  
	{
        errorExit("wlfFileClose");  
	}
      
status = wlfCleanup();  
if(status != WLF_OK)  
	{
        errorExit("wlfCleanup");  
	}

return(status);  
}  
