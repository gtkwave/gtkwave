#include "gw-var-enums.h"

#define WAVE_NODEVARTYPE_STR

const gchar *gw_var_type_to_string(GwVarType self)
{
    g_return_val_if_fail(self >= 0 && self < GW_VAR_TYPE_MAX, "");

    static const char *STRINGS[] = {
        "",        "event", "integer", "parm",   "real",     "real_parm", "reg",    "supply0",
        "supply1", "time",  "tri",     "triand", "trior",    "trireg",    "tri0",   "tri1",
        "wand",    "wire",  "wor",     "array",  "realtime", "port",      "string", "bit",
        "logic",   "int",   "s_int",   "l_int",  "byte",     "enum",      "s_real", "signal",
        "var",     "const", "file",    "memory", "net",      "alias",     "missing"};

    return STRINGS[self];
}

const gchar *gw_var_data_type_to_string(GwVarDataType self)
{
    g_return_val_if_fail(self >= 0 && self < GW_VAR_DATA_TYPE_MAX, "");

    static const gchar *STRINGS[] = {"",
                                     "boolean",
                                     "bit",
                                     "bit_vec",
                                     "ulogic",
                                     "ulogic_v",
                                     "logic",
                                     "logic_v",
                                     "unsigned",
                                     "signed",
                                     "integer",
                                     "real",
                                     "natural",
                                     "positive",
                                     "time",
                                     "char",
                                     "string"};

    return STRINGS[self];
}

const gchar *gw_var_dir_to_string(GwVarDir self)
{
    g_return_val_if_fail(self >= 0 && self < GW_VAR_DIR_MAX, "");

    static const gchar *STRINGS[] = {"", "I", "O", "IO", "B", "L"};

    return STRINGS[self];
}
