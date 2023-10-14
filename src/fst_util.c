#include <config.h>
#include <gtkwave.h>
#include "fst_util.h"

GwTreeKind fst_scope_type_to_gw_tree_kind(enum fstScopeType scope_type)
{
    switch (scope_type) {
        case FST_ST_VCD_MODULE:
            return GW_TREE_KIND_VCD_ST_MODULE;
        case FST_ST_VCD_TASK:
            return GW_TREE_KIND_VCD_ST_TASK;
        case FST_ST_VCD_FUNCTION:
            return GW_TREE_KIND_VCD_ST_FUNCTION;
        case FST_ST_VCD_BEGIN:
            return GW_TREE_KIND_VCD_ST_BEGIN;
        case FST_ST_VCD_FORK:
            return GW_TREE_KIND_VCD_ST_FORK;
        case FST_ST_VCD_GENERATE:
            return GW_TREE_KIND_VCD_ST_GENERATE;
        case FST_ST_VCD_STRUCT:
            return GW_TREE_KIND_VCD_ST_STRUCT;
        case FST_ST_VCD_UNION:
            return GW_TREE_KIND_VCD_ST_UNION;
        case FST_ST_VCD_CLASS:
            return GW_TREE_KIND_VCD_ST_CLASS;
        case FST_ST_VCD_INTERFACE:
            return GW_TREE_KIND_VCD_ST_INTERFACE;
        case FST_ST_VCD_PACKAGE:
            return GW_TREE_KIND_VCD_ST_PACKAGE;
        case FST_ST_VCD_PROGRAM:
            return GW_TREE_KIND_VCD_ST_PROGRAM;

        case FST_ST_VHDL_ARCHITECTURE:
            return GW_TREE_KIND_VHDL_ST_ARCHITECTURE;
        case FST_ST_VHDL_PROCEDURE:
            return GW_TREE_KIND_VHDL_ST_PROCEDURE;
        case FST_ST_VHDL_FUNCTION:
            return GW_TREE_KIND_VHDL_ST_FUNCTION;
        case FST_ST_VHDL_RECORD:
            return GW_TREE_KIND_VHDL_ST_RECORD;
        case FST_ST_VHDL_PROCESS:
            return GW_TREE_KIND_VHDL_ST_PROCESS;
        case FST_ST_VHDL_BLOCK:
            return GW_TREE_KIND_VHDL_ST_BLOCK;
        case FST_ST_VHDL_FOR_GENERATE:
            return GW_TREE_KIND_VHDL_ST_GENFOR;
        case FST_ST_VHDL_IF_GENERATE:
            return GW_TREE_KIND_VHDL_ST_GENIF;
        case FST_ST_VHDL_GENERATE:
            return GW_TREE_KIND_VHDL_ST_GENERATE;
        case FST_ST_VHDL_PACKAGE:
            return GW_TREE_KIND_VHDL_ST_PACKAGE;

        default:
            return GW_TREE_KIND_UNKNOWN;
    }
}

GwVarType fst_var_type_to_gw_var_type(enum fstVarType var_type)
{
    switch (var_type) {
        case FST_VT_VCD_EVENT:
            return GW_VAR_TYPE_VCD_EVENT;
        case FST_VT_VCD_INTEGER:
            return GW_VAR_TYPE_VCD_INTEGER;
        case FST_VT_VCD_PARAMETER:
            return GW_VAR_TYPE_VCD_PARAMETER;
        case FST_VT_VCD_REAL:
            return GW_VAR_TYPE_VCD_REAL;
        case FST_VT_VCD_REAL_PARAMETER:
            return GW_VAR_TYPE_VCD_REAL_PARAMETER;
        case FST_VT_VCD_REALTIME:
            return GW_VAR_TYPE_VCD_REALTIME;
        case FST_VT_VCD_REG:
            return GW_VAR_TYPE_VCD_REG;
        case FST_VT_VCD_SUPPLY0:
            return GW_VAR_TYPE_VCD_SUPPLY0;
        case FST_VT_VCD_SUPPLY1:
            return GW_VAR_TYPE_VCD_SUPPLY1;
        case FST_VT_VCD_TIME:
            return GW_VAR_TYPE_VCD_TIME;
        case FST_VT_VCD_TRI:
            return GW_VAR_TYPE_VCD_TRI;
        case FST_VT_VCD_TRIAND:
            return GW_VAR_TYPE_VCD_TRIAND;
        case FST_VT_VCD_TRIOR:
            return GW_VAR_TYPE_VCD_TRIOR;
        case FST_VT_VCD_TRIREG:
            return GW_VAR_TYPE_VCD_TRIREG;
        case FST_VT_VCD_TRI0:
            return GW_VAR_TYPE_VCD_TRI0;
        case FST_VT_VCD_TRI1:
            return GW_VAR_TYPE_VCD_TRI1;
        case FST_VT_VCD_WAND:
            return GW_VAR_TYPE_VCD_WAND;
        case FST_VT_VCD_WIRE:
            return GW_VAR_TYPE_VCD_WIRE;
        case FST_VT_VCD_WOR:
            return GW_VAR_TYPE_VCD_WOR;
        case FST_VT_VCD_PORT:
            return GW_VAR_TYPE_VCD_PORT;

        case FST_VT_GEN_STRING:
            return GW_VAR_TYPE_GEN_STRING;

        case FST_VT_SV_BIT:
            return GW_VAR_TYPE_SV_BIT;
        case FST_VT_SV_LOGIC:
            return GW_VAR_TYPE_SV_LOGIC;
        case FST_VT_SV_INT:
            return GW_VAR_TYPE_SV_INT;
        case FST_VT_SV_SHORTINT:
            return GW_VAR_TYPE_SV_SHORTINT;
        case FST_VT_SV_LONGINT:
            return GW_VAR_TYPE_SV_LONGINT;
        case FST_VT_SV_BYTE:
            return GW_VAR_TYPE_SV_BYTE;
        case FST_VT_SV_ENUM:
            return GW_VAR_TYPE_SV_ENUM;
        case FST_VT_SV_SHORTREAL:
            return GW_VAR_TYPE_SV_SHORTREAL;

        default:
            return GW_VAR_TYPE_UNSPECIFIED_DEFAULT;
    }
}

GwVarDir fst_var_dir_to_gw_var_dir(enum fstVarDir var_dir)
{
    switch (var_dir) {
        case FST_VD_INPUT:
            return GW_VAR_DIR_IN;
        case FST_VD_OUTPUT:
            return GW_VAR_DIR_OUT;
        case FST_VD_INOUT:
            return GW_VAR_DIR_INOUT;
        case FST_VD_BUFFER:
            return GW_VAR_DIR_BUFFER;
        case FST_VD_LINKAGE:
            return GW_VAR_DIR_LINKAGE;

        case FST_VD_IMPLICIT:
        default:
            return GW_VAR_DIR_IMPLICIT;
    }
}

GwVarDataType fst_supplemental_data_type_to_gw_var_data_type(
    enum fstSupplementalDataType supplemental_data_type)
{
    switch (supplemental_data_type) {
        case FST_SDT_VHDL_BOOLEAN:
            return GW_VAR_DATA_TYPE_VHDL_BOOLEAN;
        case FST_SDT_VHDL_BIT:
            return GW_VAR_DATA_TYPE_VHDL_BIT;
        case FST_SDT_VHDL_BIT_VECTOR:
            return GW_VAR_DATA_TYPE_VHDL_BIT_VECTOR;
        case FST_SDT_VHDL_STD_ULOGIC:
            return GW_VAR_DATA_TYPE_VHDL_STD_ULOGIC;
        case FST_SDT_VHDL_STD_ULOGIC_VECTOR:
            return GW_VAR_DATA_TYPE_VHDL_STD_ULOGIC_VECTOR;
        case FST_SDT_VHDL_STD_LOGIC:
            return GW_VAR_DATA_TYPE_VHDL_STD_LOGIC;
        case FST_SDT_VHDL_STD_LOGIC_VECTOR:
            return GW_VAR_DATA_TYPE_VHDL_STD_LOGIC_VECTOR;
        case FST_SDT_VHDL_UNSIGNED:
            return GW_VAR_DATA_TYPE_VHDL_UNSIGNED;
        case FST_SDT_VHDL_SIGNED:
            return GW_VAR_DATA_TYPE_VHDL_SIGNED;
        case FST_SDT_VHDL_INTEGER:
            return GW_VAR_DATA_TYPE_VHDL_INTEGER;
        case FST_SDT_VHDL_REAL:
            return GW_VAR_DATA_TYPE_VHDL_REAL;
        case FST_SDT_VHDL_NATURAL:
            return GW_VAR_DATA_TYPE_VHDL_NATURAL;
        case FST_SDT_VHDL_POSITIVE:
            return GW_VAR_DATA_TYPE_VHDL_POSITIVE;
        case FST_SDT_VHDL_TIME:
            return GW_VAR_DATA_TYPE_VHDL_TIME;
        case FST_SDT_VHDL_CHARACTER:
            return GW_VAR_DATA_TYPE_VHDL_CHARACTER;
        case FST_SDT_VHDL_STRING:
            return GW_VAR_DATA_TYPE_VHDL_STRING;
        default:
            return GW_VAR_DATA_TYPE_NONE;
    }
}
