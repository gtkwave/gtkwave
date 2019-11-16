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

public class fstReader extends fstAPI 
{
private long ctx;

fstReader(String nam)
{
ctx = fstReaderOpen(nam);
}

protected void finalize() throws Throwable 
{
try	{
	fstReaderClose(ctx);
     	} finally 
	{
        super.finalize();
     	}
}


public void 	fstReaderClose() { fstReaderClose(ctx); ctx = 0; }
public void 	fstReaderClrFacProcessMaskAll() { fstReaderClrFacProcessMaskAll(ctx); }
public void 	fstReaderClrFacProcessMask(int facidx) { fstReaderClrFacProcessMask(ctx, facidx); }
public long 	fstReaderGetAliasCount() { return(fstReaderGetAliasCount(ctx)); }
public String 	fstReaderGetCurrentFlatScope() { return(fstReaderGetCurrentFlatScope(ctx)); }
public int 	fstReaderGetCurrentScopeLen() { return(fstReaderGetCurrentScopeLen(ctx)); }
public int 	fstReaderGetFileType() { return(fstReaderGetFileType(ctx)); }
public String 	fstReaderGetCurrentScopeUserInfo() { return(fstReaderGetCurrentScopeUserInfo(ctx)); }
public String 	fstReaderGetDateString() { return(fstReaderGetDateString(ctx)); }
public long 	fstReaderGetDumpActivityChangeTime(int idx) { return(fstReaderGetDumpActivityChangeTime(ctx, idx)); }
public boolean 	fstReaderGetDumpActivityChangeValue(int idx) { return(fstReaderGetDumpActivityChangeValue(ctx, idx)); }
public long 	fstReaderGetEndTime() { return(fstReaderGetEndTime(ctx)); }
public boolean 	fstReaderGetFacProcessMask(int facidx) { return(fstReaderGetFacProcessMask(ctx, facidx)); }
public boolean 	fstReaderGetFseekFailed() { return(fstReaderGetFseekFailed(ctx)); }
public int 	fstReaderGetMaxHandle() { return(fstReaderGetMaxHandle(ctx)); }
public long 	fstReaderGetMemoryUsedByWriter() { return(fstReaderGetMemoryUsedByWriter(ctx)); }
public int 	fstReaderGetNumberDumpActivityChanges() { return(fstReaderGetNumberDumpActivityChanges(ctx)); }
public long 	fstReaderGetScopeCount() { return(fstReaderGetScopeCount(ctx)); }
public long 	fstReaderGetStartTime() { return(fstReaderGetStartTime(ctx)); }
public int 	fstReaderGetTimescale() { return(fstReaderGetTimescale(ctx)); }
public long 	fstReaderGetTimezero() { return(fstReaderGetTimezero(ctx)); }
public long 	fstReaderGetValueChangeSectionCount() { return(fstReaderGetValueChangeSectionCount(ctx)); }
public String 	fstReaderGetValueFromHandleAtTime(long tim, int facidx) { return(fstReaderGetValueFromHandleAtTime(ctx, tim, facidx)); }
public long 	fstReaderGetVarCount() { return(fstReaderGetVarCount(ctx)); }
public String 	fstReaderGetVersionString() { return(fstReaderGetVersionString(ctx)); }
public void 	fstReaderIterateHier(fstHier fh) { fstReaderIterateHier(ctx, fh); }
public boolean 	fstReaderIterateHierRewind() { return(fstReaderIterateHierRewind(ctx)); }
public int 	fstReaderIterBlocks(Object cbobj) { return(fstReaderIterBlocks(ctx, cbobj)); }
public String 	fstReaderPopScope() { return(fstReaderPopScope(ctx)); }
public String 	fstReaderPushScope(String nam, long user_info) { return(fstReaderPushScope(ctx, nam, user_info)); }
public void 	fstReaderResetScope() { fstReaderResetScope(ctx); }
public void 	fstReaderSetFacProcessMaskAll() { fstReaderSetFacProcessMaskAll(ctx); }
public void 	fstReaderSetFacProcessMask(int facidx) { fstReaderSetFacProcessMask(ctx, facidx); }
public void     fstReaderSetVcdExtensions(boolean enable) { fstReaderSetVcdExtensions(ctx, enable); }
public void 	fstReaderSetLimitTimeRange(long start_time, long end_time) { fstReaderSetLimitTimeRange(ctx, start_time, end_time); }
public void 	fstReaderSetUnlimitedTimeRange() { fstReaderSetUnlimitedTimeRange(ctx); }


public String	fstReaderVcdID(int value)
{
char []s = new char[5]; // 94 ** 5
int vmod;
int i;

// zero is illegal for a value...it is assumed they start at one
for(i=0;;i++)
        {
	vmod = (value % 94);
        if(vmod != 0)
                {
		s[i] = (char) (vmod+32);
                }
                else
                {
		s[i] = '~';
                value -= 94;
                }
        value = value / 94;
        if(value == 0) { break; }
        }

return(new String(s, 0, i+1));
}


public String	fstReaderGetTimescaleString() 
{ 
int ts = fstReaderGetTimescale();
int time_scale = 1;
char time_dimension;
String s;

switch(ts)
	{       
        case  2:        time_scale = 100;               time_dimension = ' '; break;
        case  1:        time_scale = 10;
        case  0:                                        time_dimension = ' '; break;
         
        case -1:        time_scale = 100;               time_dimension = 'm'; break;
        case -2:        time_scale = 10;
        case -3:                                        time_dimension = 'm'; break;
                 
        case -4:        time_scale = 100;               time_dimension = 'u'; break;
        case -5:        time_scale = 10;
        case -6:                                        time_dimension = 'u'; break;

        case -10:       time_scale = 100;               time_dimension = 'p'; break;
        case -11:       time_scale = 10;
        case -12:                                       time_dimension = 'p'; break;
        
        case -13:       time_scale = 100;               time_dimension = 'f'; break;
        case -14:       time_scale = 10;
        case -15:                                       time_dimension = 'f'; break;
        
        case -16:       time_scale = 100;               time_dimension = 'a'; break;
        case -17:       time_scale = 10;
        case -18:                                       time_dimension = 'a'; break;
                
        case -19:       time_scale = 100;               time_dimension = 'z'; break;
        case -20:       time_scale = 10;
        case -21:                                       time_dimension = 'z'; break;

        case -7:        time_scale = 100;               time_dimension = 'n'; break;
        case -8:        time_scale = 10;
        case -9:
        default:                                        time_dimension = 'n'; break;
        }

s = "" + time_scale;
s = s + time_dimension;

return(s); 
}

}
