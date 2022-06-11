/*
 * Copyright (c) 2013 Tony Bybell.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

public class fstVarType
{
private fstVarType() { }

public static final String [] FST_VT_NAMESTRINGS =
	{ "event", "integer", "parameter", "real", "real_parameter",
        	"reg", "supply0", "supply1", "time", "tri",
        	"triand", "trior", "trireg", "tri0", "tri1",
        	"wand", "wire", "wor", "port", "sparray", 
		"realtime", "string", "bit", "logic", "int", 
		"shortint", "longint", "byte", "enum", "shortreal" };

public static final int FST_VT_VCD_MIN = 0;
public static final int FST_VT_VCD_EVENT = 0;
public static final int FST_VT_VCD_INTEGER = 1;
public static final int FST_VT_VCD_PARAMETER = 2;
public static final int FST_VT_VCD_REAL = 3;
public static final int FST_VT_VCD_REAL_PARAMETER = 4;
public static final int FST_VT_VCD_REG = 5;
public static final int FST_VT_VCD_SUPPLY0 = 6;
public static final int FST_VT_VCD_SUPPLY1 = 7;
public static final int FST_VT_VCD_TIME = 8; 
public static final int FST_VT_VCD_TRI = 9;    
public static final int FST_VT_VCD_TRIAND = 10;
public static final int FST_VT_VCD_TRIOR = 11; 
public static final int FST_VT_VCD_TRIREG = 12;
public static final int FST_VT_VCD_TRI0 = 13; 
public static final int FST_VT_VCD_TRI1 = 14;  
public static final int FST_VT_VCD_WAND = 15;
public static final int FST_VT_VCD_WIRE = 16;
public static final int FST_VT_VCD_WOR = 17;   
public static final int FST_VT_VCD_PORT = 18;
public static final int FST_VT_VCD_SPARRAY = 19;
public static final int FST_VT_VCD_REALTIME = 20;

public static final int FST_VT_GEN_STRING = 21;

public static final int FST_VT_SV_BIT = 22;
public static final int FST_VT_SV_LOGIC = 23;
public static final int FST_VT_SV_INT = 24; 
public static final int FST_VT_SV_SHORTINT = 25;
public static final int FST_VT_SV_LONGINT = 26; 
public static final int FST_VT_SV_BYTE = 27;  
public static final int FST_VT_SV_ENUM = 28;
public static final int FST_VT_SV_SHORTREAL = 29;

public static final int FST_VT_VCD_MAX = 29;
};

