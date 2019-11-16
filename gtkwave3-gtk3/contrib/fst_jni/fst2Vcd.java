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

public class fst2Vcd
{
fstReader fb;
byte [] facType;
long prevTime;
long startTime, endTime;


public void fstReaderCallback(long tim, int facidx, String value)
{
if(tim != prevTime)
	{
	System.out.println("#" + tim);
	prevTime = tim;
	}

switch(facType[facidx])
	{
	case fstVarType.FST_VT_VCD_REAL:
	case fstVarType.FST_VT_VCD_REAL_PARAMETER:
	case fstVarType.FST_VT_VCD_REALTIME:
	case fstVarType.FST_VT_SV_SHORTREAL:		
		System.out.println("r" + value + " " + fb.fstReaderVcdID(facidx));
		break;

	case fstVarType.FST_VT_GEN_STRING:
		System.out.println("s" + value + " " + fb.fstReaderVcdID(facidx));
		break;

	default:if(value.length() == 1)
			{
			System.out.println(value + fb.fstReaderVcdID(facidx));
			}
		else
			{
			System.out.println("b" + value + " " + fb.fstReaderVcdID(facidx));
			}
		break;
	}
}


fst2Vcd(String fnam)
	{
	fb = new fstReader(fnam);

	startTime = fb.fstReaderGetStartTime();
	endTime = fb.fstReaderGetEndTime();

	System.out.println("$date\n\t" + fb.fstReaderGetDateString() + "\n$end");
	System.out.println("$version\n\t" + fb.fstReaderGetVersionString() + "\n$end");
	System.out.println("$timescale\n\t" + fb.fstReaderGetTimescaleString() + "s\n$end");

	int maxHandle = fb.fstReaderGetMaxHandle();
	
	facType = new byte[maxHandle+1];

	fstHier fh = new fstHier();
	for(;;)
		{
		fb.fstReaderIterateHier(fh);
		if(!fh.valid) break;

		switch(fh.htyp)
			{
			case fstHierType.FST_HT_SCOPE:
				System.out.println("$scope " + fstScopeType.FST_ST_NAMESTRINGS[fh.typ] + " " + fh.name1 + " $end");
				break;

			case fstHierType.FST_HT_UPSCOPE:
				System.out.println("$upscope $end");
				break;

			case fstHierType.FST_HT_VAR:
				int modlen;
				facType[fh.handle] = (byte)fh.typ;
				switch(fh.typ)
					{
					case fstVarType.FST_VT_VCD_REAL:
					case fstVarType.FST_VT_VCD_REAL_PARAMETER:
					case fstVarType.FST_VT_VCD_REALTIME:		modlen = 64; break;
					case fstVarType.FST_VT_SV_SHORTREAL:		modlen = 32; break;
					case fstVarType.FST_VT_GEN_STRING:		modlen = 0;  break;
					default:					modlen = fh.length; break;
					}

				System.out.println("$var " + fstVarType.FST_VT_NAMESTRINGS[fh.typ] + " " + 
						modlen + " " +
						fb.fstReaderVcdID(fh.handle) + " " + fh.name1 + " $end");
				break;

			case fstHierType.FST_HT_ATTRBEGIN:

				if(fh.typ == fstAttrType.FST_AT_MISC)
					{
					switch(fh.subtype)
						{
						case fstMiscType.FST_MT_COMMENT:
							System.out.println("$comment\n\t" + fh.name1 + "\n$end");
							break;

						case fstMiscType.FST_MT_SOURCESTEM:
						case fstMiscType.FST_MT_SOURCEISTEM:
							System.out.println("$attrbegin " + fstAttrType.FST_AT_NAMESTRINGS[fh.typ] + " " + 
								Integer.toHexString(fh.subtype) + " " + fh.arg_from_name + " " + fh.arg + " $end");
							break;

						default:
							System.out.println("$attrbegin " + fstAttrType.FST_AT_NAMESTRINGS[fh.typ] + " " + 
								Integer.toHexString(fh.subtype) + " " + fh.name1 + " " + fh.arg + " $end");
							break;
						}
					}
				break;

			case fstHierType.FST_HT_ATTREND:
				System.out.println("$attrend $end");
				break;
			}
		}

	System.out.println("$enddefinitions $end");

	fb.fstReaderSetFacProcessMaskAll();

	prevTime = 0;
 	fb.fstReaderIterBlocks(this);

	if(prevTime != endTime)
		{
		System.out.println("#" + endTime);
		}

	fb.fstReaderClose();
	}
}
