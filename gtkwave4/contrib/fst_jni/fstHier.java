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

public class fstHier 
{
public boolean valid; 	   // all types
public int htyp;	   // FST_HT_SCOPE, FST_HT_UPSCOPE, FST_HT_VAR, FST_HT_ATTRBEGIN, FST_HT_ATTREND
public int typ;		   // appropriate FST_ST_* type for htyp, vartype, etc.
public int subtype;	   // FST_HT_ATTRBEGIN
public String name1;	   // FST_HT_SCOPE, FST_HT_VAR, FST_HT_ATTRBEGIN
public String name2;	   // FST_HT_SCOPE
public int direction;	   // FST_HT_VAR
public int handle;	   // FST_HT_VAR
public int length;	   // FST_HT_VAR
public boolean is_alias;   // FST_HT_VAR
public long arg;	   // FST_HT_ATTRBEGIN
public long arg_from_name; // FST_HT_ATTRBEGIN

public fstHier() 
	{
	valid = false;
	htyp = 0;
	typ = 0;
	subtype = 0;
	name1 = "";
	name2 = "";
	direction = 0;
	handle = 0;
	length = 0;
	is_alias = false;
	arg = 0;
	arg_from_name = 0;
	};
};
