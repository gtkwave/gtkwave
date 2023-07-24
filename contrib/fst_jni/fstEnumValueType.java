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

public class fstEnumValueType
{
private fstEnumValueType() { }

public static final String [] FST_VT_NAMESTRINGS =
	{ "integer", "bit", "logic", "int", "shortint", 
		"longint", "byte", "unsigned_integer", "unsigned_bit", "unsigned_logic", 
		"unsigned_int", "unsigned_shortint", "unsigned_longint", "unsigned_byte" };

public static final int FST_EV_MIN = 0;

public static final int FST_EV_SV_INTEGER = 0;
public static final int FST_EV_SV_BIT = 1;
public static final int FST_EV_SV_LOGIC = 2;
public static final int FST_EV_SV_INT = 3;
public static final int FST_EV_SV_SHORTINT = 4;
public static final int FST_EV_SV_LONGINT = 5;
public static final int FST_EV_SV_BYTE = 6;
public static final int FST_EV_SV_UNSIGNED_INTEGER = 7;
public static final int FST_EV_SV_UNSIGNED_BIT = 8;
public static final int FST_EV_SV_UNSIGNED_LOGIC = 9;
public static final int FST_EV_SV_UNSIGNED_INT = 10;
public static final int FST_EV_SV_UNSIGNED_SHORTINT = 11;
public static final int FST_EV_SV_UNSIGNED_LONGINT = 12;
public static final int FST_EV_SV_UNSIGNED_BYTE = 13;

public static final int FST_EV_MAX = 13;
};

