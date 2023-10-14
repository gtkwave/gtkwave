#pragma once

#include <glib.h>

typedef enum
{
    GW_VAR_TYPE_UNSPECIFIED_DEFAULT = 0,

    GW_VAR_TYPE_VCD_EVENT = 1,
    GW_VAR_TYPE_VCD_INTEGER = 2,
    GW_VAR_TYPE_VCD_PARAMETER = 3,
    GW_VAR_TYPE_VCD_REAL = 4,
    GW_VAR_TYPE_VCD_REAL_PARAMETER = 5,
    GW_VAR_TYPE_VCD_REG = 6,
    GW_VAR_TYPE_VCD_SUPPLY0 = 7,
    GW_VAR_TYPE_VCD_SUPPLY1 = 8,
    GW_VAR_TYPE_VCD_TIME = 9,
    GW_VAR_TYPE_VCD_TRI = 10,
    GW_VAR_TYPE_VCD_TRIAND = 11,
    GW_VAR_TYPE_VCD_TRIOR = 12,
    GW_VAR_TYPE_VCD_TRIREG = 13,
    GW_VAR_TYPE_VCD_TRI0 = 14,
    GW_VAR_TYPE_VCD_TRI1 = 15,
    GW_VAR_TYPE_VCD_WAND = 16,
    GW_VAR_TYPE_VCD_WIRE = 17,
    GW_VAR_TYPE_VCD_WOR = 18,
    GW_VAR_TYPE_VCD_ARRAY = 19, /* used to define the rownum (index) port on the array */
    GW_VAR_TYPE_VCD_REALTIME = 20,

    GW_VAR_TYPE_VCD_PORT = 21,
    GW_VAR_TYPE_GEN_STRING = 22, /* generic string type */

    GW_VAR_TYPE_SV_BIT = 23, /* SV types */
    GW_VAR_TYPE_SV_LOGIC = 24,
    GW_VAR_TYPE_SV_INT = 25,
    GW_VAR_TYPE_SV_SHORTINT = 26,
    GW_VAR_TYPE_SV_LONGINT = 27,
    GW_VAR_TYPE_SV_BYTE = 28,
    GW_VAR_TYPE_SV_ENUM = 29,
    GW_VAR_TYPE_SV_SHORTREAL = 30,

    GW_VAR_TYPE_VHDL_SIGNAL = 31,
    GW_VAR_TYPE_VHDL_VARIABLE = 32,
    GW_VAR_TYPE_VHDL_CONSTANT = 33,
    GW_VAR_TYPE_VHDL_FILE = 34,
    GW_VAR_TYPE_VHDL_MEMORY = 35,

    GW_VAR_TYPE_GEN_NET = 36, /* used for AE2 */
    GW_VAR_TYPE_GEN_ALIAS = 37,
    GW_VAR_TYPE_GEN_MISSING = 38,

    GW_VAR_TYPE_MAX = 38
    /* if this exceeds 63, need to update struct Node's "unsigned vartype : 6" declaration */
} GwVarType;

const gchar *gw_var_type_to_string(GwVarType self);

typedef enum
{
    GW_VAR_DATA_TYPE_NONE = 0,

    GW_VAR_DATA_TYPE_VHDL_BOOLEAN = 1,
    GW_VAR_DATA_TYPE_VHDL_BIT = 2,
    GW_VAR_DATA_TYPE_VHDL_BIT_VECTOR = 3,
    GW_VAR_DATA_TYPE_VHDL_STD_ULOGIC = 4,
    GW_VAR_DATA_TYPE_VHDL_STD_ULOGIC_VECTOR = 5,
    GW_VAR_DATA_TYPE_VHDL_STD_LOGIC = 6,
    GW_VAR_DATA_TYPE_VHDL_STD_LOGIC_VECTOR = 7,
    GW_VAR_DATA_TYPE_VHDL_UNSIGNED = 8,
    GW_VAR_DATA_TYPE_VHDL_SIGNED = 9,
    GW_VAR_DATA_TYPE_VHDL_INTEGER = 10,
    GW_VAR_DATA_TYPE_VHDL_REAL = 11,
    GW_VAR_DATA_TYPE_VHDL_NATURAL = 12,
    GW_VAR_DATA_TYPE_VHDL_POSITIVE = 13,
    GW_VAR_DATA_TYPE_VHDL_TIME = 14,
    GW_VAR_DATA_TYPE_VHDL_CHARACTER = 15,
    GW_VAR_DATA_TYPE_VHDL_STRING = 16,

    GW_VAR_DATA_TYPE_MAX = 16
    /* if this exceeds 63, need to update struct Node's "unsigned vardt : 6" declaration */
} GwVarDataType;

const gchar *gw_var_data_type_to_string(GwVarDataType self);

typedef enum
{
    GW_VAR_DIR_IMPLICIT = 0,
    GW_VAR_DIR_IN = 1,
    GW_VAR_DIR_OUT = 2,
    GW_VAR_DIR_INOUT = 3,
    GW_VAR_DIR_BUFFER = 4,
    GW_VAR_DIR_LINKAGE = 5,

    GW_VAR_DIR_MAX = 5,
    GW_VAR_DIR_UNSPECIFIED = 6
    /* if ND_DIR_MAX exceeds 7, need to update struct Node's "unsigned vardir : 3" declaration */
} GwVarDir;

const gchar *gw_var_dir_to_string(GwVarDir self);
