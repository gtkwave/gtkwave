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

public class fstSupplementalDataType
{
fstSupplementalDataType( ) { }

public static final int FST_SDT_MIN                    = 0;

public static final int FST_SDT_NONE                   = 0;

public static final int FST_SDT_VHDL_BOOLEAN           = 1;
public static final int FST_SDT_VHDL_BIT               = 2;
public static final int FST_SDT_VHDL_BIT_VECTOR        = 3;
public static final int FST_SDT_VHDL_STD_ULOGIC        = 4;
public static final int FST_SDT_VHDL_STD_ULOGIC_VECTOR = 5;
public static final int FST_SDT_VHDL_STD_LOGIC         = 6;
public static final int FST_SDT_VHDL_STD_LOGIC_VECTOR  = 7;
public static final int FST_SDT_VHDL_UNSIGNED          = 8;
public static final int FST_SDT_VHDL_SIGNED            = 9;
public static final int FST_SDT_VHDL_INTEGER           = 10;
public static final int FST_SDT_VHDL_REAL              = 11;
public static final int FST_SDT_VHDL_NATURAL           = 12;
public static final int FST_SDT_VHDL_POSITIVE          = 13;
public static final int FST_SDT_VHDL_TIME              = 14;
public static final int FST_SDT_VHDL_CHARACTER         = 15;
public static final int FST_SDT_VHDL_STRING            = 16;
    
public static final int FST_SDT_MAX                    = 16;
  
public static final int FST_SDT_SVT_SHIFT_COUNT        = 10; /* FST_SVT_* is ORed in to the left after shifting FST_SDT_SVT_SHIFT_COUNT */
public static final int FST_SDT_ABS_MAX                = (1<<(FST_SDT_SVT_SHIFT_COUNT));
};
