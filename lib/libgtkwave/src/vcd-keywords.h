#pragma once

int vcd_keyword_code(const char *s, unsigned int len);

enum VarTypes
{
    V_EVENT,
    V_PARAMETER,
    V_INTEGER,
    V_REAL,
    V_REAL_PARAMETER = V_REAL,
    V_REALTIME = V_REAL,
    V_SHORTREAL = V_REAL,
    V_REG,
    V_SUPPLY0,
    V_SUPPLY1,
    V_TIME,
    V_TRI,
    V_TRIAND,
    V_TRIOR,
    V_TRIREG,
    V_TRI0,
    V_TRI1,
    V_WAND,
    V_WIRE,
    V_WOR,
    V_PORT,
    V_IN = V_PORT,
    V_OUT = V_PORT,
    V_INOUT = V_PORT,
    V_BIT,
    V_LOGIC,
    V_INT,
    V_SHORTINT,
    V_LONGINT,
    V_BYTE,
    V_ENUM,
    V_STRINGTYPE,
    V_END,
    V_LB,
    V_COLON,
    V_RB,
    V_STRING
};
