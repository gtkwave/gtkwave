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

public class fstScopeType
{
private fstScopeType() { }

public static final String [] FST_ST_NAMESTRINGS =
	{ "module", "task", "function", "begin", "fork", 
		"generate", "struct", "union", "class", "interface", 
		"package", "program",

		"vhdl_architecture", "vhdl_procedure", "vhdl_function", "vhdl_record", "vhdl_process", 
		"vhdl_block", "vhdl_for_generate", "vhdl_if_generate", "vhdl_generate", "vhdl_package"
	};

public static final int FST_ST_MIN = 0;

public static final int FST_ST_VCD_MODULE = 0;
public static final int FST_ST_VCD_TASK = 1;
public static final int FST_ST_VCD_FUNCTION = 2;
public static final int FST_ST_VCD_BEGIN = 3;
public static final int FST_ST_VCD_FORK = 4;
public static final int FST_ST_VCD_GENERATE = 5;
public static final int FST_ST_VCD_STRUCT = 6;
public static final int FST_ST_VCD_UNION = 7;
public static final int FST_ST_VCD_CLASS = 8;
public static final int FST_ST_VCD_INTERFACE = 9;
public static final int FST_ST_VCD_PACKAGE = 10;
public static final int FST_ST_VCD_PROGRAM = 11;

public static final int FST_ST_VHDL_ARCHITECTURE = 12;
public static final int FST_ST_VHDL_PROCEDURE = 13;
public static final int FST_ST_VHDL_FUNCTION = 14;
public static final int FST_ST_VHDL_RECORD = 15;
public static final int FST_ST_VHDL_PROCESS = 16;
public static final int FST_ST_VHDL_BLOCK = 17;
public static final int FST_ST_VHDL_FOR_GENERATE = 18;
public static final int FST_ST_VHDL_IF_GENERATE = 19;
public static final int FST_ST_VHDL_GENERATE = 20;
public static final int FST_ST_VHDL_PACKAGE = 21;

public static final int FST_ST_MAX = 21;

public static final int FST_ST_GEN_ATTRBEGIN = 252;
public static final int FST_ST_GEN_ATTREND = 253;

public static final int FST_ST_VCD_SCOPE = 254;
public static final int FST_ST_VCD_UPSCOPE = 255;
};

