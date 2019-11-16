/*
 to compile: 
 g++ -o fsdb2vcd_fast -O2 fsdb2vcd_fast.cc -I /pub/FsdbReader/ /pub/FsdbReader/libnffr.a /pub/FsdbReader/libnsys.a -ldl -lpthread -lz

 Much faster version of fsdb2vcd as compared to one bundled with Verdi.
 Requires libs and headers for FsdbReader.
 */

#ifdef NOVAS_FSDB
#undef NOVAS_FSDB
#endif

#include "ffrAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <map>

#ifdef __GNUC__
/* non-portable trick that caches the ->second value of a RB tree entry for a given ->first key */
/* provides about 20% overall speedup */
#define BYPASS_RB_TREE_IF_POSSIBLE
#endif

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define WRITEX_FILE_BUFFER_SIZE (64 * 1024)

#define T2U64(t) (((uint64_t)(t).H << 32) | ((uint64_t)(t).L))
#define XT2U64(xt) (((uint64_t)(xt).t64.H << 32) | ((uint64_t)(xt).t64.L))
#define FXT2U64(xt) (((uint64_t)(xt).hltag.H << 32) | ((uint64_t)(xt).hltag.L))

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

enum VcdVcTypes  { VCD_VC_TYPE_BIT, VCD_VC_TYPE_BITVEC, VCD_VC_TYPE_REAL, VCD_VC_TYPE_PORT, VCD_VC_TYPE_IGNORE };

struct fsdbReaderBlackoutChain
{
uint64_t tim;
unsigned active : 1;
};


static void writex(int fd, char *s, int len)
{
const int mx = WRITEX_FILE_BUFFER_SIZE;
static char buf[mx];
static int pos = 0;

if(len)
	{
	if(len < mx)
		{
		if(pos + len >= mx)
			{
			writex(fd, NULL, 0);
			}

		memcpy(buf + pos, s, len);
		pos += len;
		}
		else
		{
		writex(fd, NULL, 0);
		write(fd, s, len);
		}
	}
	else
	{
	if(pos)
		{
		write(fd, buf, pos);
		pos = 0;
		}
	}
}


static char *makeVcdID(unsigned int value, int *idlen)
{
static char buf[16];
char *pnt = buf;

/* zero is illegal for a value...it is assumed they start at one */
while (value)
        {
        value--;
        *(pnt++) = (char)('!' + value % 94);
        value = value / 94;
        }

*pnt = 0;
*idlen = pnt - buf;
return(buf);
}


static bool_T __TreeCB(fsdbTreeCBType cb_type, void *client_data, void *tree_cb_data)
{
return(TRUE);
}


static bool_T __MyTreeCB(fsdbTreeCBType cb_type, void *client_data, void *tree_cb_data);


static void *fsdbReaderOpenFile(char *nam)
{
fsdbFileType ft;
uint_T blk_idx = 0;

if(!ffrObject::ffrIsFSDB(nam))
	{
	return(NULL);
	}

ffrFSDBInfo fsdb_info;
ffrObject::ffrGetFSDBInfo(nam, fsdb_info);
if((fsdb_info.file_type != FSDB_FT_VERILOG) && (fsdb_info.file_type != FSDB_FT_VERILOG_VHDL) && (fsdb_info.file_type != FSDB_FT_VHDL))
	{
	return(NULL);
	}

ffrObject *fsdb_obj = ffrObject::ffrOpen3(nam);
if(!fsdb_obj)
	{
	return(NULL);
	}

fsdb_obj->ffrSetTreeCBFunc(__TreeCB, NULL);
    
ft = fsdb_obj->ffrGetFileType();
if((ft != FSDB_FT_VERILOG) && (ft != FSDB_FT_VERILOG_VHDL) && (ft != FSDB_FT_VHDL))
	{
        fsdb_obj->ffrClose();
	return(NULL);
	}

fsdb_obj->ffrReadDataTypeDefByBlkIdx(blk_idx); /* necessary if FSDB file has transaction data ... we don't process this but it prevents possible crashes */

return((void *)fsdb_obj);
}


static void fsdbReaderReadScopeVarTree(void *ctx, FILE *cb)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;

fsdb_obj->ffrSetTreeCBFunc(__MyTreeCB, (void *) cb);
fsdb_obj->ffrReadScopeVarTree();
}


static int fsdbReaderGetMaxVarIdcode(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdbVarIdcode max_var_idcode = fsdb_obj->ffrGetMaxVarIdcode();
return(max_var_idcode);
}


static void fsdbReaderAddToSignalList(void *ctx, int i)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrAddToSignalList(i);
}


static void fsdbReaderLoadSignals(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrLoadSignals();
}


static void *fsdbReaderCreateVCTraverseHandle(void *ctx, int i)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl hdl = fsdb_obj->ffrCreateVCTraverseHandle(i);
return((void *)hdl);
}


static int fsdbReaderHasIncoreVC(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
return(fsdb_hdl->ffrHasIncoreVC() == TRUE);
}


static void fsdbReaderFree(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

fsdb_hdl->ffrFree();
}


static uint64_t fsdbReaderGetMinXTag(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

fsdb_hdl->ffrGetMinXTag((void*)&timetag);
uint64_t rv = T2U64(timetag);
return(rv);
}


static uint64_t fsdbReaderGetMaxXTag(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

fsdb_hdl->ffrGetMaxXTag((void*)&timetag);
uint64_t rv = T2U64(timetag);
return(rv);
}


static void fsdbReaderGotoXTag(void *ctx, void *hdl, uint64_t tim)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

timetag.H = (uint32_t)(tim >> 32);
timetag.L = (uint32_t)(tim & 0xFFFFFFFFUL);

fsdb_hdl->ffrGotoXTag((void*)&timetag);
}


static uint64_t fsdbReaderGetXTag(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

fsdb_hdl->ffrGetXTag((void*)&timetag);
uint64_t rv = T2U64(timetag);
return(rv);
}


static int fsdbReaderGetVC(void *ctx, void *hdl, void **val_ptr)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetVC((byte_T**)val_ptr) == FSDB_RC_SUCCESS);
}


static int fsdbReaderGotoNextVC(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGotoNextVC() == FSDB_RC_SUCCESS);
}


static void fsdbReaderUnloadSignals(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrUnloadSignals();
}


static void fsdbReaderClose(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrClose();
}


static int fsdbReaderGetBytesPerBit(void *hdl)
{
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetBytesPerBit());
}


static int fsdbReaderGetBitSize(void *hdl)
{
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetBitSize());
}


static int fsdbReaderGetVarType(void *hdl)
{
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetVarType());
}


static char *fsdbReaderTranslateVC(void *hdl, void *val_ptr, int *vtype, int *vlen)
{ 
ffrVCTrvsHdl vc_trvs_hdl = (ffrVCTrvsHdl)hdl;
byte_T *vc_ptr = (byte_T *)val_ptr;
byte_T *bufferp;
uint_T i;
fsdbVarType var_type; 
int bs;
static byte_T buffer[FSDB_MAX_BIT_SIZE+1+32];

bufferp = buffer + 1;
    
switch (vc_trvs_hdl->ffrGetBytesPerBit()) 
	{
	case FSDB_BYTES_PER_BIT_1B:
		*vlen = bs = vc_trvs_hdl->ffrGetBitSize();
	        for (i = 0; i < bs; i++) 
			{
		    	switch(vc_ptr[i]) 
				{
	 	    		case FSDB_BT_VCD_0:
		        		bufferp[i] = '0';
		        		break;
	
		    		case FSDB_BT_VCD_1:
		        		bufferp[i] = '1';
		        		break;

		    		case FSDB_BT_VCD_X:
		        		bufferp[i] = 'x';
		        		break;
	
		    		case FSDB_BT_VCD_Z:
		        		bufferp[i] = 'z';
					break;

		    		default:
		        		bufferp[i] = 'x';
		        		break;
		    		}
	        	}
		*vtype = (i>1) ? VCD_VC_TYPE_BITVEC : VCD_VC_TYPE_BIT;
		break;

	case FSDB_BYTES_PER_BIT_2B:
		{
		bs = vc_trvs_hdl->ffrGetBitSize();
		fsdbBitType *bt = (fsdbBitType *)bufferp;
		byte_T *buffers0 = bufferp + bs*1+1;
		byte_T *buffers1 = bufferp + bs*2+2;
		
		fsdbStrengthType *s0 = (fsdbStrengthType *)buffers0;
		fsdbStrengthType *s1 = (fsdbStrengthType *)buffers1;
		
		ffrObject::ffrGetEvcdPortStrength((byte_T *)val_ptr, bt, s0, s1);
		for (i = 0; i < bs; i++)
			{
			/* normally use ffrObject::ffrGetEvcdPortStrength() */
			bufferp[i] = ((byte_T *)val_ptr)[i*2];
			buffers0[i] = ((byte_T *)val_ptr)[i*2+1] & 15;
			buffers1[i] = (((byte_T *)val_ptr)[i*2+1] >> 4) & 15;

			if((bufferp[i] >= FSDB_BT_EVCD_L) && (bufferp[i] <= FSDB_BT_EVCD_f))
				{
				bufferp[i] = "LlHhXxTDdUuNnZ?01AaBbCcFf"[bufferp[i]];
				}
				else
				{
				bufferp[i] = '?';
				}

			if((buffers0[i] >= FSDB_ST_HIGHZ) && (buffers0[i] <= FSDB_ST_SUPPLY))
				{
				buffers0[i] += '0';
				}
				else
				{
				buffers0[i] = FSDB_ST_HIGHZ + '0';
				}

			if((buffers1[i] >= FSDB_ST_HIGHZ) && (buffers1[i] <= FSDB_ST_SUPPLY))
				{
				buffers1[i] += '0';
				}
				else
				{
				buffers1[i] = FSDB_ST_HIGHZ + '0';
				}
			}

		bufferp[bs] = ' ';
		bufferp[bs*2+1] = ' ';
		bufferp[*vlen = bs*3+2] = 0;
		*vtype = VCD_VC_TYPE_PORT;
		}
		break;

	case FSDB_BYTES_PER_BIT_4B:
		var_type = vc_trvs_hdl->ffrGetVarType();
		switch(var_type)
			{
			case FSDB_VT_VCD_MEMORY_DEPTH:
			case FSDB_VT_VHDL_MEMORY_DEPTH:
				break;
	               
			default:    
				vc_trvs_hdl->ffrGetVC(&vc_ptr);
				*vlen = sprintf((char *)bufferp, "%f", *((float*)vc_ptr));
				break;
			}
		*vtype = VCD_VC_TYPE_IGNORE;
		break;
	
	case FSDB_BYTES_PER_BIT_8B:
		var_type = vc_trvs_hdl->ffrGetVarType();
		switch(var_type)
			{
			case FSDB_VT_VCD_REAL:
				*vlen = sprintf((char *)bufferp, "%.16g", *((double*)vc_ptr));
				*vtype = VCD_VC_TYPE_REAL;
				break;

			case FSDB_VT_STREAM:
			default:
				*vlen = 0;
				*vtype = VCD_VC_TYPE_IGNORE;
				break;
			}
		break;

	default:
		*vlen = 0;
		*vtype = VCD_VC_TYPE_IGNORE;
		break;
	}

return((char *)bufferp);
}


static int fsdbReaderExtractScaleUnit(void *ctx, int *mult, char *scale)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
uint_T digit;
char *unit;

str_T su = fsdb_obj->ffrGetScaleUnit();
fsdbRC rc = fsdb_obj->ffrExtractScaleUnit(su, digit, unit);

if(rc == FSDB_RC_SUCCESS)
	{
	*mult = digit ? ((int)digit) : 1; /* in case digit is zero */
	*scale = unit[0];
	}

return(rc == FSDB_RC_SUCCESS);
}


static int fsdbReaderGetMinFsdbTag64(void *ctx, uint64_t *tim)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdbTag64 tag64;
fsdbRC rc = fsdb_obj->ffrGetMinFsdbTag64(&tag64);

if(rc == FSDB_RC_SUCCESS)
	{
	*tim = T2U64(tag64);
	}

return(rc == FSDB_RC_SUCCESS);
}


static int fsdbReaderGetMaxFsdbTag64(void *ctx, uint64_t *tim)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdbTag64 tag64;
fsdbRC rc = fsdb_obj->ffrGetMaxFsdbTag64(&tag64);

if(rc == FSDB_RC_SUCCESS)
	{
	*tim = T2U64(tag64);
	}

return(rc == FSDB_RC_SUCCESS);
}


static unsigned int fsdbReaderGetDumpOffRange(void *ctx, struct fsdbReaderBlackoutChain **r)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;

if(fsdb_obj->ffrHasDumpOffRange())
	{
	uint_T count;
	fsdbDumpOffRange *fdr = NULL;

	if(FSDB_RC_SUCCESS == fsdb_obj->ffrGetDumpOffRange(count, fdr))
		{
		uint_T i;
		*r = (struct fsdbReaderBlackoutChain *)calloc(count * 2, sizeof(struct fsdbReaderBlackoutChain));

		for(i=0;i<count;i++)
			{
			(*r)[i*2  ].tim = FXT2U64(fdr[i].begin); (*r)[i*2  ].active = 0;
			(*r)[i*2+1].tim = FXT2U64(fdr[i].end);   (*r)[i*2+1].active = 1;
			}

		uint64_t max_t;
		if(fsdbReaderGetMaxFsdbTag64(ctx, &max_t))
			{
			if((count == 1) && (max_t == (*r)[0].tim))
				{
				free(*r);
				*r = NULL;
				count = 0;
				}
			}

		return(count*2);
		}
	}

if(*r) { *r = NULL; }
return(0);
}


static void __DumpStruct(fsdbTreeCBDataStructBegin* struc, FILE *cb)
{
str_T type;

fprintf(cb, "$scope %s %s $end\n", "fork", struc->name);
/* sprintf(bf, "Scope: %s %s %s", type, scope->name, scope->module ? scope->module : "NULL"); */
}


static void __DumpScope(fsdbTreeCBDataScope* scope, FILE *cb)
{
str_T type;

switch (scope->type) 
	{
	case FSDB_ST_SV_INTERFACE:
    	case FSDB_ST_VCD_MODULE:
		type = (str_T) "module"; 
		break;

	case FSDB_ST_VCD_TASK:
		type = (str_T) "task"; 
		break;

	case FSDB_ST_VCD_FUNCTION:
		type = (str_T) "function"; 
		break;
	
	case FSDB_ST_VCD_BEGIN:
		type = (str_T) "begin"; 
		break;
	
	case FSDB_ST_VCD_FORK:
		type = (str_T) "fork"; 
		break;

	case FSDB_ST_VCD_GENERATE:
	case FSDB_ST_VHDL_ARCHITECTURE:
	case FSDB_ST_VHDL_PROCEDURE:
	case FSDB_ST_VHDL_FUNCTION:
	case FSDB_ST_VHDL_RECORD:
	case FSDB_ST_VHDL_PROCESS:
	case FSDB_ST_VHDL_BLOCK:
	case FSDB_ST_VHDL_FOR_GENERATE:
	case FSDB_ST_VHDL_IF_GENERATE:
	case FSDB_ST_VHDL_GENERATE:
	default:
		type = (str_T) "begin";
		break;
    	}

fprintf(cb, "$scope %s %s $end\n", type, scope->name);
/* sprintf(bf, "Scope: %s %s %s", type, scope->name, scope->module ? scope->module : "NULL"); */
}


static void __DumpVar(fsdbTreeCBDataVar *var, FILE *cb)
{
str_T type;
int vcdid_len;
int siz = 0;

switch (var->type) 
	{
    	case FSDB_VT_VCD_EVENT:
		type = (str_T) "event"; 
  		break;

    	case FSDB_VT_VCD_INTEGER:
		type = (str_T) "integer"; 
		break;

    	case FSDB_VT_VCD_PARAMETER:
		type = (str_T) "parameter"; 
		break;

    	case FSDB_VT_VCD_REAL:
		type = (str_T) "real"; 
		siz = 64;
		break;

    	case FSDB_VT_VCD_REG:
		type = (str_T) "reg"; 
		break;

    	case FSDB_VT_VCD_SUPPLY0:
		type = (str_T) "supply0"; 
		break;

    	case FSDB_VT_VCD_SUPPLY1:
		type = (str_T) "supply1"; 
		break;

    	case FSDB_VT_VCD_TIME:
		type = (str_T) "time";
		break;

    	case FSDB_VT_VCD_TRI:
		type = (str_T) "tri";
		break;

    	case FSDB_VT_VCD_TRIAND:
		type = (str_T) "triand";
		break;

    	case FSDB_VT_VCD_TRIOR:
		type = (str_T) "trior";
		break;

    	case FSDB_VT_VCD_TRIREG:
		type = (str_T) "trireg";
		break;

    	case FSDB_VT_VCD_TRI0:
		type = (str_T) "tri0";
		break;

    	case FSDB_VT_VCD_TRI1:
		type = (str_T) "tri1";
		break;

    	case FSDB_VT_VCD_WAND:
		type = (str_T) "wand";
		break;

    	case FSDB_VT_VCD_WIRE:
		type = (str_T) "wire";
		break;

    	case FSDB_VT_VCD_WOR:
		type = (str_T) "wor";
		break;

    	case FSDB_VT_VHDL_SIGNAL:
    	case FSDB_VT_VHDL_VARIABLE:
    	case FSDB_VT_VHDL_CONSTANT:
    	case FSDB_VT_VHDL_FILE:
    	case FSDB_VT_VCD_MEMORY:
    	case FSDB_VT_VHDL_MEMORY:
    	case FSDB_VT_VCD_MEMORY_DEPTH:
    	case FSDB_VT_VHDL_MEMORY_DEPTH:         
		switch(var->vc_dt)
			{
			case FSDB_VC_DT_FLOAT:
			case FSDB_VC_DT_DOUBLE:
				type = (str_T) "real";
				break;

			case FSDB_VC_DT_UNKNOWN:
			case FSDB_VC_DT_BYTE:
			case FSDB_VC_DT_SHORT:
			case FSDB_VC_DT_INT:
			case FSDB_VC_DT_LONG:
			case FSDB_VC_DT_HL_INT:
			case FSDB_VC_DT_PHYSICAL:
			default:
				if(var->type == FSDB_VT_VHDL_SIGNAL)
					{
					type = (str_T) "wire";
					}
				else
					{
					type = (str_T) "reg";
					}
				break;
			}
		break;

	case FSDB_VT_VCD_PORT:
		type = (str_T) "port";
		break;

	case FSDB_VT_STREAM: /* these hold transactions: not yet supported so do not emit */
		return;
		/* type = (str_T) "stream"; */
		break;

    	default:
		type = (str_T) "wire";
		break;
    	}

if(!siz)
	{
	if(var->lbitnum >= var->rbitnum)
		{
		siz = var->lbitnum - var->rbitnum + 1;
		}
		else
		{
		siz = var->rbitnum - var->lbitnum + 1;
		}
	}

char *lb = strchr(var->name, '[');

if(!lb || strchr(var->name, ' '))
	{
	fprintf(cb, "$var %s %d %s %s $end\n", type, siz, makeVcdID(var->u.idcode, &vcdid_len), var->name);
	}
	else
	{
	fprintf(cb, "$var %s %d %s ", type, siz, makeVcdID(var->u.idcode, &vcdid_len)); /* add space, VCS-style */
	fwrite(var->name, lb - var->name, 1, cb);
	fprintf(cb, " %s $end\n", lb);
	}
}


static bool_T __MyTreeCB(fsdbTreeCBType cb_type, 
			 void *client_data, void *tree_cb_data)
{
FILE *cb = (FILE *)client_data;

switch (cb_type) 
	{
    	case FSDB_TREE_CBT_BEGIN_TREE:
		/* fprintf(stderr, "Begin Tree:\n"); */
		break;

    	case FSDB_TREE_CBT_SCOPE:
		__DumpScope((fsdbTreeCBDataScope *)tree_cb_data, cb);
		break;

	case FSDB_TREE_CBT_STRUCT_BEGIN:
		__DumpStruct((fsdbTreeCBDataStructBegin *)tree_cb_data, cb);
		break;

    	case FSDB_TREE_CBT_VAR:
		__DumpVar((fsdbTreeCBDataVar*)tree_cb_data, cb);
		break;

    	case FSDB_TREE_CBT_UPSCOPE:
	case FSDB_TREE_CBT_STRUCT_END:
		fprintf(cb, "$upscope $end\n");
		break;

    	case FSDB_TREE_CBT_END_TREE:
		/* fprintf(stderr, "End Tree:\n"); */
		break;

    	case FSDB_TREE_CBT_ARRAY_BEGIN:
    	case FSDB_TREE_CBT_ARRAY_END:
		break;

    	case FSDB_TREE_CBT_FILE_TYPE:
    	case FSDB_TREE_CBT_SIMULATOR_VERSION:
    	case FSDB_TREE_CBT_SIMULATION_DATE:
    	case FSDB_TREE_CBT_X_AXIS_SCALE:
    	case FSDB_TREE_CBT_END_ALL_TREE:
    	case FSDB_TREE_CBT_RECORD_BEGIN:
    	case FSDB_TREE_CBT_RECORD_END:
        	break;
             
    	default:
		return(FALSE);
    	}

return(TRUE);
}


int main(int argc, char **argv)
{
void *ctx;
FILE *fh;
int i;
ffrVCTrvsHdl *hdl;
int mx_id;
uint64_t key = 0;
uint64_t max_tim;
ffrFSDBInfo fsdb_info;
int mult;
char scale[3];
std::map <uint64_t, fsdbVarIdcode> time_map;
fsdbVarIdcode *time_autosort;
char timestring[32];
FILE *stdout_cache = stdout;
unsigned int dumpoff_count = 0;
unsigned int dumpoff_idx = 0;
struct fsdbReaderBlackoutChain *dumpoff_ranges = NULL;
uint64_t prev_dumponoff_tim = 0;
int dumptime_emitted = 0;
uint64_t time_serial_number = 0;

stdout_cache = stdout;
stdout = tmpfile(); /* redirects useless log file messages that would mess up VCD output for piped execution */

if((argc != 2) && (argc != 3))
	{
	fprintf(stderr, "Usage:\n------\n%s filename.fsdb [filename.vcd]\n\n", argv[0]);
	exit(0);
	}


ctx = fsdbReaderOpenFile(argv[1]);
if(!ctx)
	{
	fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
	exit(255);
	}

if(argc == 3)
	{
	fh =  fopen(argv[2], "wb");
	if(!fh)
		{
		fprintf(stderr, "Could not open '%s', exiting.\n", argv[2]);
		perror("Why");
		exit(255);
		}
	}
	else
	{
	fh = stdout_cache;
	if(!fh)
		{
		fprintf(stderr, "stdin is NULL, exiting.\n");
		exit(255);
		}
	}

dumpoff_count = fsdbReaderGetDumpOffRange(ctx, &dumpoff_ranges);

if(FSDB_RC_SUCCESS == ((ffrObject *)ctx)->ffrGetFSDBInfo(argv[1], fsdb_info))
	{
	fprintf(fh, "$date\n\t%s\n$end\n", fsdb_info.simulation_date);
	fprintf(fh, "$version\n\t%s\n$end\n", fsdb_info.simulator_version);
	}

if(fsdbReaderExtractScaleUnit(ctx, &mult, &scale[0]))
	{
	scale[0] = tolower(scale[0]);
	switch(scale[0])
		{
		case 'm':
		case 'u':
		case 'n':
		case 'p':
		case 'f':
		case 'a':
		case 'z': scale[1] = 's'; scale[2] = 0; break;

		default	: scale[0] = 's'; scale[1] = 0; break;
		}
	fprintf(fh, "$timescale\n\t%d%s\n$end\n", mult, scale);
	}


fsdbReaderReadScopeVarTree(ctx, fh);
fprintf(fh, "$enddefinitions $end\n$dumpvars\n");

mx_id = fsdbReaderGetMaxVarIdcode(ctx);
for(i=1;i<=mx_id;i++)
	{
	fsdbReaderAddToSignalList(ctx, i);
	}

fsdbReaderLoadSignals(ctx);

hdl = (ffrVCTrvsHdl *)calloc(i+1, sizeof(ffrVCTrvsHdl));
for(i=1;i<=mx_id;i++)
	{
	hdl[i] = (ffrVCTrvsHdl)fsdbReaderCreateVCTraverseHandle(ctx, i);
	}

time_autosort = (fsdbVarIdcode *)calloc(mx_id + 1, sizeof(fsdbVarIdcode));

for(i=mx_id;i>0;i--)
	{
	ffrXTag xt;
	fsdbRC gf = hdl[i]->ffrGotoTheFirstVC();
	if(gf == FSDB_RC_SUCCESS)
		{
	    	fsdbRC gx = hdl[i]->ffrGetXTag(&xt);

		if(gx == FSDB_RC_SUCCESS)
			{
			uint64_t x64 = XT2U64(xt);
			fsdbVarIdcode t_prev = time_map[x64];
			time_autosort[i] = t_prev;
			time_map[x64] = i;
			}
		}
	}

fflush(fh);
setvbuf(fh, (char *) NULL, _IONBF, 0); /* even buffered IO is slow so disable it and use our own routines that don't need seeking */
int fd = fileno(fh);

while(!time_map.empty())
	{
	key =  time_map.begin()->first;

	while(dumpoff_idx < dumpoff_count)
		{
		uint64_t top_tim = dumpoff_ranges[dumpoff_idx].tim;

		if(key >= top_tim)
			{
			uint64_t active = dumpoff_ranges[dumpoff_idx].active;

			if(top_tim >= prev_dumponoff_tim)
				{
				if((top_tim != prev_dumponoff_tim) || (!dumptime_emitted))
					{
					if(time_serial_number++ == 1)
						{
						writex(fd, (char *)"$end\n", 5);
						}
					if(top_tim) { writex(fd, timestring, sprintf(timestring, "#%"PRIu64"\n", top_tim)); }
					prev_dumponoff_tim = top_tim;
					dumptime_emitted = 1;
					}

				if(active)
					{
					writex(fd, timestring, sprintf(timestring, "$dumpon\n"));
					}
					else
					{
					writex(fd, timestring, sprintf(timestring, "$dumpoff\n"));
					}
				}

			dumpoff_idx++;
			}
			else
			{
			break;
			}
		}

	if((!dumptime_emitted) || (key != prev_dumponoff_tim))
		{
		if(time_serial_number++ == 1)
			{
			writex(fd, (char *)"$end\n", 5);
			}
		if(key) { writex(fd, timestring, sprintf(timestring, "#%"PRIu64"\n", key)); }
		}

	fsdbVarIdcode idx = time_map.begin()->second;
#ifdef BYPASS_RB_TREE_IF_POSSIBLE
	fsdbVarIdcode *tms = NULL;
	uint64_t prev_x64 = key - 1;
#endif
	while(idx)
		{
		fsdbVarIdcode idxn = time_autosort[idx];	
		byte_T *ret_vc;
		fsdbRC gvc = hdl[idx]->ffrGetVC(&ret_vc);
		if(gvc == FSDB_RC_SUCCESS)
			{
			int vtype, vlen;
			char *vcdid;
			int vcdid_len;
			char *vdata = fsdbReaderTranslateVC(hdl[idx], ret_vc, &vtype, &vlen);

			vcdid = makeVcdID(idx, &vcdid_len);

			switch(vtype)
				{
				case VCD_VC_TYPE_BIT:	
							{
							writex(fd, vdata, 1);
							vcdid[vcdid_len++] = '\n';
							writex(fd, vcdid, vcdid_len);
							}
							break;

				case VCD_VC_TYPE_BITVEC:
				case VCD_VC_TYPE_REAL:
				case VCD_VC_TYPE_PORT:
							{
							vdata--; /* buffer was specially allocated in fsdbReaderTranslateVC() so we can do this */
							vdata[0] = (vtype == VCD_VC_TYPE_BITVEC) ? 'b' : ((vtype == VCD_VC_TYPE_REAL) ? 'r' : 'p'); vlen++;
							vdata[vlen] = ' '; vlen++;
							writex(fd, vdata, vlen);

							vcdid[vcdid_len++] = '\n';
							writex(fd, vcdid, vcdid_len);
							}
							break;

				case VCD_VC_TYPE_IGNORE:
				default:
					1;
				}

			if(FSDB_RC_SUCCESS == hdl[idx]->ffrGotoNextVC())
				{
				ffrXTag xt;
				fsdbRC gx = hdl[idx]->ffrGetXTag(&xt);
				if(gx == FSDB_RC_SUCCESS)
					{
					uint64_t x64 = XT2U64(xt);

#ifdef BYPASS_RB_TREE_IF_POSSIBLE
					if(x64 == prev_x64)
						{
						}
						else
						{
						tms = & time_map[prev_x64 = x64];
						}

					fsdbVarIdcode t_prev = *tms;
					time_autosort[idx] = t_prev;
					*tms = idx;
#else
					fsdbVarIdcode t_prev = time_map[x64];
					time_autosort[idx] = t_prev;
					time_map[x64] = idx;
#endif
					}
				}
			}
		idx = idxn;
		}

	time_map.erase(key);
	}

if(fsdbReaderGetMaxFsdbTag64(ctx, &max_tim))
	{
	if(key != max_tim)
		{
		if(time_serial_number++ == 1)
			{
			writex(fd, (char *)"$end\n", 5);
			}
		if(max_tim) { writex(fd, timestring, sprintf(timestring, "#%"PRIu64"\n", max_tim)); }
		}
	}

writex(fd, NULL, 0);

free(dumpoff_ranges);
free(time_autosort);
free(hdl);
if(fh != stdout_cache) fclose(fh);
fsdbReaderClose(ctx);
fclose(stdout);

return(0);
}
