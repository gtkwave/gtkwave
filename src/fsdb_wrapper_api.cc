/*
 * Copyright (c) Tony Bybell 2013-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <fstapi.h>
#include "fsdb_wrapper_api.h"

#ifdef WAVE_FSDB_READER_IS_PRESENT

#ifdef NOVAS_FSDB
#undef NOVAS_FSDB
#endif

#include "ffrAPI.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#define FSDBR_FXT2U64(xt) (((uint64_t)(xt).hltag.H << 32) | ((uint64_t)(xt).hltag.L))

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

static bool_T __TreeCB(fsdbTreeCBType cb_type, void *client_data, void *tree_cb_data)
{
return(TRUE); /* currently unused along with var/scope traversal */
}

static bool_T __MyTreeCB(fsdbTreeCBType cb_type, void *client_data, void *tree_cb_data);


extern "C" void *fsdbReaderOpenFile(char *nam)
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


extern "C" void fsdbReaderReadScopeVarTree(void *ctx,void (*cb)(void *))
{
ffrObject *fsdb_obj = (ffrObject *)ctx;

fsdb_obj->ffrSetTreeCBFunc(__MyTreeCB, (void *) cb);
fsdb_obj->ffrReadScopeVarTree();
}


extern "C" int fsdbReaderGetMaxVarIdcode(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdbVarIdcode max_var_idcode = fsdb_obj->ffrGetMaxVarIdcode();
return(max_var_idcode);
}


extern "C" void fsdbReaderAddToSignalList(void *ctx, int i)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrAddToSignalList(i);
}


extern "C" void fsdbReaderResetSignalList(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrResetSignalList();
}


extern "C" void fsdbReaderLoadSignals(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrLoadSignals();
}


extern "C" void *fsdbReaderCreateVCTraverseHandle(void *ctx, int i)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl hdl = fsdb_obj->ffrCreateVCTraverseHandle(i);
return((void *)hdl);
}


extern "C" int fsdbReaderHasIncoreVC(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
return(fsdb_hdl->ffrHasIncoreVC() == TRUE);
}


extern "C" void fsdbReaderFree(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

fsdb_hdl->ffrFree();
}


extern "C" uint64_t fsdbReaderGetMinXTag(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

fsdb_hdl->ffrGetMinXTag((void*)&timetag);
uint64_t rv = (((uint64_t)timetag.H) << 32) | ((uint64_t)timetag.L);
return(rv);
}


extern "C" uint64_t fsdbReaderGetMaxXTag(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

fsdb_hdl->ffrGetMaxXTag((void*)&timetag);
uint64_t rv = (((uint64_t)timetag.H) << 32) | ((uint64_t)timetag.L);
return(rv);
}


extern "C" int fsdbReaderGotoXTag(void *ctx, void *hdl, uint64_t tim)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

timetag.H = (uint32_t)(tim >> 32);
timetag.L = (uint32_t)(tim & 0xFFFFFFFFUL);

return(fsdb_hdl->ffrGotoXTag((void*)&timetag) == FSDB_RC_SUCCESS);
}


extern "C" uint64_t fsdbReaderGetXTag(void *ctx, void *hdl, int *rc)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;
fsdbTag64 timetag;

*rc = (fsdb_hdl->ffrGetXTag((void*)&timetag) == FSDB_RC_SUCCESS);
uint64_t rv = (((uint64_t)timetag.H) << 32) | ((uint64_t)timetag.L);
return(rv);
}


extern "C" int fsdbReaderGetVC(void *ctx, void *hdl, void **val_ptr)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetVC((byte_T**)val_ptr) == FSDB_RC_SUCCESS);
}


extern "C" int fsdbReaderGotoNextVC(void *ctx, void *hdl)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGotoNextVC() == FSDB_RC_SUCCESS);
}


extern "C" void fsdbReaderUnloadSignals(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrUnloadSignals();
}


extern "C" void fsdbReaderClose(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdb_obj->ffrClose();
}


extern "C" int fsdbReaderGetBytesPerBit(void *hdl)
{
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetBytesPerBit());
}


extern "C" int fsdbReaderGetBitSize(void *hdl)
{
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetBitSize());
}


extern "C" int fsdbReaderGetVarType(void *hdl)
{
ffrVCTrvsHdl fsdb_hdl = (ffrVCTrvsHdl)hdl;

return(fsdb_hdl->ffrGetVarType());
}


extern "C" char *fsdbReaderTranslateVC(void *hdl, void *val_ptr)
{ 
ffrVCTrvsHdl vc_trvs_hdl = (ffrVCTrvsHdl)hdl;
byte_T *vc_ptr = (byte_T *)val_ptr;

static byte_T buffer[FSDB_MAX_BIT_SIZE+1];
uint_T i;
fsdbVarType   var_type; 
    
switch (vc_trvs_hdl->ffrGetBytesPerBit()) 
	{
	case FSDB_BYTES_PER_BIT_1B:
	        for (i = 0; i < vc_trvs_hdl->ffrGetBitSize(); i++) 
			{
		    	switch(vc_ptr[i]) 
				{
	 	    		case FSDB_BT_VCD_0:
		        		buffer[i] = '0';
		        		break;
	
		    		case FSDB_BT_VCD_1:
		        		buffer[i] = '1';
		        		break;
	
		    		case FSDB_BT_VCD_Z:
		        		buffer[i] = 'z';
		        		break;
	
		    		case FSDB_BT_VCD_X:
		    		default:
		        		buffer[i] = 'x';
					break;
		    		}
	        	}
	        buffer[i] = 0;
		break;

	case FSDB_BYTES_PER_BIT_2B:
	        for (i = 0; i < vc_trvs_hdl->ffrGetBitSize(); i++) 
			{
		    	switch(vc_ptr[i * 2]) 
				{
				case FSDB_BT_EVCD_D:
				case FSDB_BT_EVCD_d:
				case FSDB_BT_EVCD_L:
				case FSDB_BT_EVCD_l:
				case FSDB_BT_EVCD_0:
		        		buffer[i] = '0';
		        		break;
	
				case FSDB_BT_EVCD_U:
				case FSDB_BT_EVCD_u:
				case FSDB_BT_EVCD_H:
				case FSDB_BT_EVCD_h:
				case FSDB_BT_EVCD_1:
		        		buffer[i] = '1';
		        		break;
	
				case FSDB_BT_EVCD_Z:
				case FSDB_BT_EVCD_T:
				case FSDB_BT_EVCD_F:
				case FSDB_BT_EVCD_f:
		        		buffer[i] = 'z';
		        		break;
	
				case FSDB_BT_EVCD_N:
				case FSDB_BT_EVCD_X:
				case FSDB_BT_EVCD_QSTN:
				case FSDB_BT_EVCD_A:
				case FSDB_BT_EVCD_a:
				case FSDB_BT_EVCD_B:
				case FSDB_BT_EVCD_b:
				case FSDB_BT_EVCD_C:
				case FSDB_BT_EVCD_c:
		    		default:
		        		buffer[i] = 'x';
					break;
		    		}
	        	}
	        buffer[i] = 0;
		break;

	case FSDB_BYTES_PER_BIT_4B:
		var_type = vc_trvs_hdl->ffrGetVarType();
		switch(var_type)
			{
			case FSDB_VT_VCD_MEMORY_DEPTH:
			case FSDB_VT_VHDL_MEMORY_DEPTH:
				buffer[0] = 0;
				break;
	               
			default:    
				vc_trvs_hdl->ffrGetVC(&vc_ptr);
				sprintf((char *)buffer, "%f", *((float*)vc_ptr));
				break;
			}
		break;
	
	case FSDB_BYTES_PER_BIT_8B:
		sprintf((char *)buffer, "%.16g", *((double*)vc_ptr));
		break;

	default:
		buffer[0] = 0;
		break;
	}

return((char *)buffer);
}


extern "C" int fsdbReaderExtractScaleUnit(void *ctx, int *mult, char *scale)
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


extern "C" int fsdbReaderGetMinFsdbTag64(void *ctx, uint64_t *tim)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdbTag64 tag64;
fsdbRC rc = fsdb_obj->ffrGetMinFsdbTag64(&tag64);

if(rc == FSDB_RC_SUCCESS)
	{
	*tim = (((uint64_t)tag64.H) << 32) | ((uint64_t)tag64.L);
	}

return(rc == FSDB_RC_SUCCESS);
}


extern "C" int fsdbReaderGetMaxFsdbTag64(void *ctx, uint64_t *tim)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
fsdbTag64 tag64;
fsdbRC rc = fsdb_obj->ffrGetMaxFsdbTag64(&tag64);

if(rc == FSDB_RC_SUCCESS)
	{
	*tim = (((uint64_t)tag64.H) << 32) | ((uint64_t)tag64.L);
	}

return(rc == FSDB_RC_SUCCESS);
}


static bool_T __fsdbReaderGetStatisticsCB(fsdbTreeCBType cb_type, void *client_data, void *tree_cb_data)
{
struct fsdbReaderGetStatistics_t *gs = (struct fsdbReaderGetStatistics_t *)client_data;

switch (cb_type)
	{
	case FSDB_TREE_CBT_VAR:		gs->varCount++;
					break;

	case FSDB_TREE_CBT_STRUCT_BEGIN:
	case FSDB_TREE_CBT_SCOPE:	gs->scopeCount++;
					break;

	default:			break;
	}

return(TRUE);
}


extern "C" struct fsdbReaderGetStatistics_t *fsdbReaderGetStatistics(void *ctx)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
struct fsdbReaderGetStatistics_t *gs = (struct fsdbReaderGetStatistics_t *)calloc(1, sizeof(struct fsdbReaderGetStatistics_t));

fsdb_obj->ffrSetTreeCBFunc(__fsdbReaderGetStatisticsCB, gs);
fsdb_obj->ffrReadScopeVarTree();

return(gs);
}


static void 
__DumpScope(fsdbTreeCBDataScope* scope, void (*cb)(void *))
{
str_T type;
char bf[65537];

switch (scope->type) 
	{
    	case FSDB_ST_VCD_MODULE:
		type = (str_T) "vcd_module"; 
		break;

	case FSDB_ST_VCD_TASK:
		type = (str_T) "vcd_task"; 
		break;

	case FSDB_ST_VCD_FUNCTION:
		type = (str_T) "vcd_function"; 
		break;
	
	case FSDB_ST_VCD_BEGIN:
		type = (str_T) "vcd_begin"; 
		break;
	
	case FSDB_ST_VCD_FORK:
		type = (str_T) "vcd_fork"; 
		break;

	case FSDB_ST_VCD_GENERATE:
		type = (str_T) "vcd_generate"; 
		break;

	case FSDB_ST_SV_INTERFACE:
		type = (str_T) "sv_interface"; 
		break;

	case FSDB_ST_VHDL_ARCHITECTURE:
		type = (str_T) "vhdl_architecture";
		break;

	case FSDB_ST_VHDL_PROCEDURE:
		type = (str_T) "vhdl_procedure";
		break;

	case FSDB_ST_VHDL_FUNCTION:
		type = (str_T) "vhdl_function";
		break;

	case FSDB_ST_VHDL_RECORD:
		type = (str_T) "vhdl_record";
		break;

	case FSDB_ST_VHDL_PROCESS:
		type = (str_T) "vhdl_process";
		break;

	case FSDB_ST_VHDL_BLOCK:
		type = (str_T) "vhdl_block";
		break;

	case FSDB_ST_VHDL_FOR_GENERATE:
		type = (str_T) "vhdl_for_generate";
		break;

	case FSDB_ST_VHDL_IF_GENERATE:
		type = (str_T) "vhdl_if_generate";
		break;

	case FSDB_ST_VHDL_GENERATE:
		type = (str_T) "vhdl_generate";
		break;

	default:
		type = (str_T) "unknown_scope_type";
		break;
    	}

sprintf(bf, "Scope: %s %s %s", type, scope->name, scope->module ? scope->module : "NULL");
cb(bf);
}


static char* itoa_2(int value, char* result)
{
char* ptr = result, *ptr1 = result, tmp_char;
int tmp_value;

do {
        tmp_value = value;
        value /= 10; 
        *ptr++ = "9876543210123456789" [9 + (tmp_value - value * 10)];
} while ( value );

if (tmp_value < 0) *ptr++ = '-';
result = ptr;
*ptr-- = '\0';
while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
}
return(result);
}  


static void 
__DumpVar(fsdbTreeCBDataVar *var, void (*cb)(void *))
{
str_T type;
str_T bpb;
str_T direction;
char *pnt;
int len;
int typelen;
int dirlen;
char bf[65537];

switch(var->bytes_per_bit) 
	{
   	case FSDB_BYTES_PER_BIT_1B:
		bpb = (str_T) "1B";
		break;

    	case FSDB_BYTES_PER_BIT_2B:
		bpb = (str_T) "2B";
		break;

    	case FSDB_BYTES_PER_BIT_4B:
		bpb = (str_T) "4B";
		break;

    	case FSDB_BYTES_PER_BIT_8B:
		bpb = (str_T) "8B";
		break;

    	default:
		bpb = (str_T) "XB";
		break;
	}

switch (var->type) 
	{
    	case FSDB_VT_VCD_EVENT:
		type = (str_T) "vcd_event"; 
		typelen = 9;
  		break;

    	case FSDB_VT_VCD_INTEGER:
		type = (str_T) "vcd_integer"; 
		typelen = 11;
		break;

    	case FSDB_VT_VCD_PARAMETER:
		type = (str_T) "vcd_parameter"; 
		typelen = 13;
		break;

    	case FSDB_VT_VCD_REAL:
		type = (str_T) "vcd_real"; 
		typelen = 8;
		break;

    	case FSDB_VT_VCD_REG:
		type = (str_T) "vcd_reg"; 
		typelen = 7;
		break;

    	case FSDB_VT_VCD_SUPPLY0:
		type = (str_T) "vcd_supply0"; 
		typelen = 11;
		break;

    	case FSDB_VT_VCD_SUPPLY1:
		type = (str_T) "vcd_supply1"; 
		typelen = 11;
		break;

    	case FSDB_VT_VCD_TIME:
		type = (str_T) "vcd_time";
		typelen = 8;
		break;

    	case FSDB_VT_VCD_TRI:
		type = (str_T) "vcd_tri";
		typelen = 7;
		break;

    	case FSDB_VT_VCD_TRIAND:
		type = (str_T) "vcd_triand";
		typelen = 10;
		break;

    	case FSDB_VT_VCD_TRIOR:
		type = (str_T) "vcd_trior";
		typelen = 9;
		break;

    	case FSDB_VT_VCD_TRIREG:
		type = (str_T) "vcd_trireg";
		typelen = 10;
		break;

    	case FSDB_VT_VCD_TRI0:
		type = (str_T) "vcd_tri0";
		typelen = 8;
		break;

    	case FSDB_VT_VCD_TRI1:
		type = (str_T) "vcd_tri1";
		typelen = 8;
		break;

    	case FSDB_VT_VCD_WAND:
		type = (str_T) "vcd_wand";
		typelen = 8;
		break;

    	case FSDB_VT_VCD_WIRE:
		type = (str_T) "vcd_wire";
		typelen = 8;
		break;

    	case FSDB_VT_VCD_WOR:
		type = (str_T) "vcd_wor";
		typelen = 7;
		break;

    	case FSDB_VT_VCD_PORT:
		type = (str_T) "vcd_port";
		typelen = 8;
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
				type = (str_T) "vcd_real";
				typelen = 8;
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
					type = (str_T) "vcd_wire";
					typelen = 8;
					}
				else
					{
					type = (str_T) "vcd_reg";
					typelen = 7;
					}
				break;
			}
		break;

	case FSDB_VT_STREAM: /* these hold transactions: not yet supported */
		type = (str_T) "stream";
		typelen = 6;
		break;

    	default:
		type = (str_T) "vcd_wire";
		typelen = 8;
		break;
    	}

    switch(var->direction)
	{
	case FSDB_VD_INPUT:    	direction = (str_T) "input"; 	dirlen = 5; break;
	case FSDB_VD_OUTPUT:   	direction = (str_T) "output"; 	dirlen = 6; break;
	case FSDB_VD_INOUT:    	direction = (str_T) "inout"; 	dirlen = 5; break;
	case FSDB_VD_BUFFER:   	direction = (str_T) "buffer"; 	dirlen = 6; break;
	case FSDB_VD_LINKAGE:  	direction = (str_T) "linkage"; 	dirlen = 7; break;
	case FSDB_VD_IMPLICIT: 
	default:	       	direction = (str_T) "implicit"; dirlen = 8; break;
	}

/*
sprintf(bf, "Var: %s %s l:%d r:%d %s %d %s %d", type, var->name, var->lbitnum, var->rbitnum, 
	direction,
	var->u.idcode, bpb, var->dtidcode);
*/

memcpy(bf, "Var: ", 5);
pnt = bf+5;
len = typelen; /* strlen(type) */
memcpy(pnt, type, len);
pnt += len;
*(pnt++) = ' ';
len = strlen(var->name);
memcpy(pnt, var->name, len);
pnt += len;
memcpy(pnt, " l:", 3);
pnt += 3;
pnt = itoa_2(var->lbitnum, pnt);
memcpy(pnt, " r:", 3);
pnt += 3;
pnt = itoa_2(var->rbitnum, pnt);
*(pnt++) = ' ';
len = dirlen; /* strlen(direction) */
memcpy(pnt, direction, len);    
pnt += len;
*(pnt++) = ' ';
pnt = itoa_2(var->u.idcode, pnt);
*(pnt++) = ' ';
len = 2; /* strlen(bpb) */
memcpy(pnt, bpb, len);    
pnt += len;
*(pnt++) = ' ';
pnt = itoa_2(var->dtidcode, pnt);
*(pnt) = 0;

cb(bf);
}


static void 
__DumpStruct(fsdbTreeCBDataStructBegin* str, void (*cb)(void *))
{
char bf[65537];

/* printf("NAME: %s FIELDS: %d TYPE: %d is_partial_dumped: %d\n", str->name, (int)str->fieldCount, (int)str->type, (int)str->is_partial_dumped); */

sprintf(bf, "Scope: vcd_struct %s %s", str->name, "NULL");
cb(bf);
}


static void 
__DumpArray(fsdbTreeCBDataArrayBegin* arr, void (*cb)(void *))
{
/* printf("NAME: %s SIZE: %d is_partial_dumped: %d\n", arr->name, (int)arr->size, (int)arr->is_partial_dumped); */
}



static bool_T __MyTreeCB(fsdbTreeCBType cb_type, 
			 void *client_data, void *tree_cb_data)
{
void (*cb)(void *) = (void (*)(void *))client_data;
char bf[16];

switch (cb_type) 
	{
    	case FSDB_TREE_CBT_BEGIN_TREE:
		/* fprintf(stderr, "Begin Tree:\n"); */
		break;

    	case FSDB_TREE_CBT_SCOPE:
		__DumpScope((fsdbTreeCBDataScope*)tree_cb_data, cb);
		break;

    	case FSDB_TREE_CBT_VAR:
		__DumpVar((fsdbTreeCBDataVar*)tree_cb_data, cb);
		break;

    	case FSDB_TREE_CBT_UPSCOPE:
		strcpy(bf, "Upscope:");
		cb(bf);
		break;

    	case FSDB_TREE_CBT_END_TREE:
		strcpy(bf, "End Tree:");
		cb(bf);
		break;

	case FSDB_TREE_CBT_STRUCT_BEGIN:
		__DumpStruct((fsdbTreeCBDataStructBegin*)tree_cb_data, cb);
		break;

	case FSDB_TREE_CBT_STRUCT_END:
		strcpy(bf, "Upscope:");
		cb(bf);
		break;

	/* not yet supported */
    	case FSDB_TREE_CBT_ARRAY_BEGIN:
		__DumpArray((fsdbTreeCBDataArrayBegin*)tree_cb_data, cb);
		break;
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


/*
 * $dumpoff/$dumpon support
 */
extern "C" unsigned int fsdbReaderGetDumpOffRange(void *ctx, struct fsdbReaderBlackoutChain_t **r)   
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
 
if(fsdb_obj->ffrHasDumpOffRange())
        {
        uint_T count;
        fsdbDumpOffRange *fdr = NULL;

        if(FSDB_RC_SUCCESS == fsdb_obj->ffrGetDumpOffRange(count, fdr))
                {
                uint_T i;
                *r = (struct fsdbReaderBlackoutChain_t *)calloc(count * 2, sizeof(struct fsdbReaderBlackoutChain_t));

                for(i=0;i<count;i++)
                        {
                        (*r)[i].tim = FSDBR_FXT2U64(fdr[i*2  ].begin); (*r)[i*2  ].active = 0;
                        (*r)[i].tim = FSDBR_FXT2U64(fdr[i*2+1].begin); (*r)[i*2+1].active = 1;
                        }

                return(count*2);
                }
        }

if(*r) { *r = NULL; }
return(0);
}


/*
 * Transactions are currently not processed and are skipped
 */
extern "C" int fsdbReaderGetTransInfo(void *ctx, int idx, void **trans_info)
{
ffrObject *fsdb_obj = (ffrObject *)ctx;
ffrTransInfo *info = NULL;

fsdbRC rc = fsdb_obj->ffrGetTransInfo((fsdbTransId)idx, info);
*trans_info = info;

return(rc != FSDB_RC_FAILURE);
}


/*
 * Reformat FSDB hierarchy info into FST's format (for use with WAVE_USE_FSDB_FST_BRIDGE)
 */
static void 
__DumpScope2(fsdbTreeCBDataScope* scope, void (*cb)(void *))
{
unsigned char typ;
struct fstHier fh;

switch (scope->type) 
	{
    	case FSDB_ST_VCD_MODULE:
		typ = FST_ST_VCD_MODULE; 
		break;

	case FSDB_ST_VCD_TASK:
		typ = FST_ST_VCD_TASK; 
		break;

	case FSDB_ST_VCD_FUNCTION:
		typ = FST_ST_VCD_FUNCTION; 
		break;
	
	case FSDB_ST_VCD_BEGIN:
		typ = FST_ST_VCD_BEGIN; 
		break;
	
	case FSDB_ST_VCD_FORK:
		typ = FST_ST_VCD_FORK; 
		break;

	case FSDB_ST_VCD_GENERATE:
		typ = FST_ST_VCD_GENERATE; 
		break;

	case FSDB_ST_SV_INTERFACE:
		typ = FST_ST_VCD_INTERFACE; 
		break;

	case FSDB_ST_VHDL_ARCHITECTURE:
		typ = FST_ST_VHDL_ARCHITECTURE;
		break;

	case FSDB_ST_VHDL_PROCEDURE:
		typ = FST_ST_VHDL_PROCEDURE;
		break;

	case FSDB_ST_VHDL_FUNCTION:
		typ = FST_ST_VHDL_FUNCTION;
		break;

	case FSDB_ST_VHDL_RECORD:
		typ = FST_ST_VHDL_RECORD;
		break;

	case FSDB_ST_VHDL_PROCESS:
		typ = FST_ST_VHDL_PROCESS;
		break;

	case FSDB_ST_VHDL_BLOCK:
		typ = FST_ST_VHDL_BLOCK;
		break;

	case FSDB_ST_VHDL_FOR_GENERATE:
		typ = FST_ST_VHDL_FOR_GENERATE;
		break;

	case FSDB_ST_VHDL_IF_GENERATE:
		typ = FST_ST_VHDL_IF_GENERATE;
		break;

	case FSDB_ST_VHDL_GENERATE:
		typ = FST_ST_VHDL_GENERATE;
		break;

	default:
		typ = FST_ST_VCD_MODULE;
		break;
    	}

fh.htyp = FST_HT_SCOPE;
fh.u.scope.typ = typ;
fh.u.scope.name = scope->name;
fh.u.scope.name_length = strlen(fh.u.scope.name);
if(scope->module)
	{
	fh.u.scope.component = scope->module;
	fh.u.scope.component_length = strlen(fh.u.scope.component);
	}
	else
	{
	fh.u.scope.component = NULL;
	fh.u.scope.component_length = 0;
	}

cb(&fh);
}


static void 
__DumpVar2(fsdbTreeCBDataVar *var, void (*cb)(void *))
{
unsigned char typ;
unsigned char dir;
struct fstHier fh;

switch (var->type) 
	{
    	case FSDB_VT_VCD_EVENT:
		typ = FST_VT_VCD_EVENT; 
  		break;

    	case FSDB_VT_VCD_INTEGER:
		typ = FST_VT_VCD_INTEGER; 
		break;

    	case FSDB_VT_VCD_PARAMETER:
		typ = FST_VT_VCD_PARAMETER; 
		break;

    	case FSDB_VT_VCD_REAL:
		typ = FST_VT_VCD_REAL; 
		break;

    	case FSDB_VT_VCD_REG:
		typ = FST_VT_VCD_REG; 
		break;

    	case FSDB_VT_VCD_SUPPLY0:
		typ = FST_VT_VCD_SUPPLY0; 
		break;

    	case FSDB_VT_VCD_SUPPLY1:
		typ = FST_VT_VCD_SUPPLY1; 
		break;

    	case FSDB_VT_VCD_TIME:
		typ = FST_VT_VCD_TIME;
		break;

    	case FSDB_VT_VCD_TRI:
		typ = FST_VT_VCD_TRI;
		break;

    	case FSDB_VT_VCD_TRIAND:
		typ = FST_VT_VCD_TRIAND;
		break;

    	case FSDB_VT_VCD_TRIOR:
		typ = FST_VT_VCD_TRIOR;
		break;

    	case FSDB_VT_VCD_TRIREG:
		typ = FST_VT_VCD_TRIREG;
		break;

    	case FSDB_VT_VCD_TRI0:
		typ = FST_VT_VCD_TRI0;
		break;

    	case FSDB_VT_VCD_TRI1:
		typ = FST_VT_VCD_TRI1;
		break;

    	case FSDB_VT_VCD_WAND:
		typ = FST_VT_VCD_WAND;
		break;

    	case FSDB_VT_VCD_WIRE:
		typ = FST_VT_VCD_WIRE;
		break;

    	case FSDB_VT_VCD_WOR:
		typ = FST_VT_VCD_WOR;
		break;

    	case FSDB_VT_VCD_PORT:
		typ = FST_VT_VCD_PORT;
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
				typ = FST_VT_VCD_REAL;
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
					typ = FST_VT_VCD_WIRE;
					}
				else
					{
					typ = FST_VT_VCD_REG;
					}
				break;
			}
		break;

	case FSDB_VT_STREAM: /* these hold transactions: not yet supported */
		typ = FST_VT_GEN_STRING;
		break;

    	default:
		typ = FST_VT_VCD_WIRE;
		break;
    	}

    switch(var->direction)
	{
	case FSDB_VD_INPUT:    	dir = FST_VD_INPUT; break;
	case FSDB_VD_OUTPUT:   	dir = FST_VD_OUTPUT; break;
	case FSDB_VD_INOUT:    	dir = FST_VD_INOUT; break;
	case FSDB_VD_BUFFER:   	dir = FST_VD_BUFFER; break;
	case FSDB_VD_LINKAGE:  	dir = FST_VD_LINKAGE; break;
	case FSDB_VD_IMPLICIT: 
	default:	       	dir = FST_VD_IMPLICIT; break;
	}


fh.htyp = FST_HT_VAR;
fh.u.var.typ = typ;
fh.u.var.direction = dir;
fh.u.var.svt_workspace = 0;
fh.u.var.sdt_workspace = 0;
fh.u.var.sxt_workspace = 0;
fh.u.var.name = var->name;
fh.u.var.length = (var->lbitnum > var->rbitnum) ? (var->lbitnum - var->rbitnum + 1) : (var->rbitnum - var->lbitnum + 1);
fh.u.var.handle = var->u.idcode;
fh.u.var.name_length = strlen(fh.u.var.name);
fh.u.var.is_alias = 0; // for now

cb(&fh);
}


static void 
__DumpStruct2(fsdbTreeCBDataStructBegin* str, void (*cb)(void *))
{
struct fstHier fh;

fh.htyp = FST_HT_SCOPE;
fh.u.scope.typ = FST_ST_VCD_STRUCT;
fh.u.scope.name = str->name;
fh.u.scope.name_length = strlen(fh.u.scope.name);
fh.u.scope.component = NULL;
fh.u.scope.component_length = 0;

cb(&fh);
}


static void 
__DumpArray2(fsdbTreeCBDataArrayBegin* arr, void (*cb)(void *))
{
/* printf("NAME: %s SIZE: %d is_partial_dumped: %d\n", arr->name, (int)arr->size, (int)arr->is_partial_dumped); */
}



static bool_T __MyTreeCB2(fsdbTreeCBType cb_type, 
			 void *client_data, void *tree_cb_data)
{
void (*cb)(void *) = (void (*)(void *))client_data;
struct fstHier fh;



char bf[16];

switch (cb_type) 
	{
    	case FSDB_TREE_CBT_BEGIN_TREE:
		/* fprintf(stderr, "Begin Tree:\n"); */
		break;

    	case FSDB_TREE_CBT_SCOPE:
		__DumpScope2((fsdbTreeCBDataScope*)tree_cb_data, cb);
		break;

    	case FSDB_TREE_CBT_VAR:
		__DumpVar2((fsdbTreeCBDataVar*)tree_cb_data, cb);
		break;

    	case FSDB_TREE_CBT_UPSCOPE:
	case FSDB_TREE_CBT_STRUCT_END:
		fh.htyp = FST_HT_UPSCOPE;
		cb(&fh);
		break;

    	case FSDB_TREE_CBT_END_TREE:
		fh.htyp = FST_HT_TREEEND;
		cb(&fh);
		break;

	case FSDB_TREE_CBT_STRUCT_BEGIN:
		__DumpStruct2((fsdbTreeCBDataStructBegin*)tree_cb_data, cb);
		break;

	/* not yet supported */
    	case FSDB_TREE_CBT_ARRAY_BEGIN:
		__DumpArray2((fsdbTreeCBDataArrayBegin*)tree_cb_data, cb);
		break;
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

extern "C" void fsdbReaderReadScopeVarTree2(void *ctx,void (*cb)(void *))
{
ffrObject *fsdb_obj = (ffrObject *)ctx;

fsdb_obj->ffrSetTreeCBFunc(__MyTreeCB2, (void *) cb);
fsdb_obj->ffrReadScopeVarTree();
}

#else

static void dummy_compilation_unit(void)
{

}

#endif
