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

public class fstAPI
{
static 	
{
System.loadLibrary("fstAPI");
}


private static String bitStringInt(int i, int len) 
{
len = Math.min(32, Math.max(len, 1));
char[] cs = new char[len];
for (int j = len - 1, b = 1; 0 <= j; --j, b <<= 1) 
	{
        cs[j] = ((i & b) == 0) ? '0' : '1';
    	}
return(new String(cs));
}


protected native long fstWriterCreate(String nam, boolean use_compressed_hier);
protected native void fstWriterClose(long ctx);
protected native boolean fstWriterGetFseekFailed(long ctx);
protected native boolean fstWriterGetDumpSizeLimitReached(long ctx);
protected native void fstWriterFlushContext(long ctx);
protected native void fstWriterSetUpscope(long ctx);
protected native void fstWriterSetAttrEnd(long ctx);
protected native void fstWriterSetPackType(long ctx, int typ);
protected native void fstWriterSetFileType(long ctx, int typ);
protected native void fstWriterSetRepackOnClose(long ctx, boolean enable);
protected native void fstWriterSetParallelMode(long ctx, boolean enable);
protected native void fstWriterSetTimescale(long ctx, int ts);
protected native void fstWriterSetTimezero(long ctx, long tim);
protected native void fstWriterSetDumpSizeLimit(long ctx, long numbytes);
protected native void fstWriterEmitDumpActive(long ctx, boolean enable);
protected native void fstWriterEmitTimeChange(long ctx, long tim);
protected native void fstWriterSetDate(long ctx, String dat);
protected native void fstWriterSetVersion(long ctx, String vers);
protected native void fstWriterSetComment(long ctx, String comm);
protected native void fstWriterSetEnvVar(long ctx, String envvar);
protected native void fstWriterSetTimescaleFromString(long ctx, String s);
protected native int fstWriterCreateVar(long ctx, int vt, int vd, int len, String nam, int aliasHandle);
protected native int fstWriterCreateVar2(long ctx, int vt, int vd, int len, String nam, int aliasHandle, String type, int svt, int sdt);
protected native void fstWriterSetSourceStem(long ctx, String path, int line, boolean use_realpath);
protected native void fstWriterSetSourceInstantiationStem(long ctx, String path, int line, boolean use_realpath);
protected native void fstWriterSetScope(long ctx, int scopetype, String scopename, String scopecomp);
protected native void fstWriterEmitVariableLengthValueChange(long ctx, int handle, String val, int len);
protected native void fstWriterSetAttrBegin(long ctx, int attrtype, int subtype, String attrname, long arg);
protected native void fstWriterEmitValueChange(long ctx, int handle, String val);
protected native void fstWriterEmitValueChange(long ctx, int handle, double val);
protected        void fstWriterEmitValueChange(long ctx, int handle, int val) { fstWriterEmitValueChange(ctx, handle, bitStringInt(val, 32)); }

protected native long fstReaderOpen(String nam);
protected native long fstReaderOpenForUtilitiesOnly();
protected native void fstReaderClose(long ctx);
protected native boolean fstReaderIterateHierRewind(long ctx);
protected native void fstReaderResetScope(long ctx);
protected native int fstReaderGetCurrentScopeLen(long ctx);
protected native int fstReaderGetFileType(long ctx);
protected native long fstReaderGetTimezero(long ctx);
protected native long fstReaderGetStartTime(long ctx);
protected native long fstReaderGetEndTime(long ctx);
protected native long fstReaderGetMemoryUsedByWriter(long ctx);
protected native long fstReaderGetScopeCount(long ctx);
protected native long fstReaderGetVarCount(long ctx);
protected native int fstReaderGetMaxHandle(long ctx);
protected native long fstReaderGetAliasCount(long ctx);
protected native long fstReaderGetValueChangeSectionCount(long ctx);
protected native boolean fstReaderGetFseekFailed(long ctx);
protected native void fstReaderSetUnlimitedTimeRange(long ctx);
protected native void fstReaderSetLimitTimeRange(long ctx, long start_time, long end_time);
protected native void fstReaderSetVcdExtensions(long ctx, boolean enable);
protected native int fstReaderGetNumberDumpActivityChanges(long ctx);
protected native long fstReaderGetDumpActivityChangeTime(long ctx, int idx);
protected native boolean fstReaderGetFacProcessMask(long ctx, int facidx); 
protected native void fstReaderSetFacProcessMask(long ctx, int facidx);
protected native void fstReaderClrFacProcessMask(long ctx, int facidx);
protected native void fstReaderSetFacProcessMaskAll(long ctx);
protected native void fstReaderClrFacProcessMaskAll(long ctx);
protected native String fstReaderGetVersionString(long ctx);
protected native String fstReaderGetDateString(long ctx);
protected native String fstReaderPopScope(long ctx);
protected native String fstReaderGetCurrentFlatScope(long ctx);
protected native String fstReaderGetCurrentScopeUserInfo(long ctx);
protected native String fstReaderPushScope(long ctx, String nam, long user_info);
protected native int fstReaderGetTimescale(long ctx);   
protected native boolean fstReaderGetDumpActivityChangeValue(long ctx, int idx);
protected native String fstReaderGetValueFromHandleAtTime(long ctx, long tim, int facidx);
protected native void fstReaderIterateHier(long ctx, fstHier fh);
protected native int fstReaderIterBlocks(long ctx, Object cbobj);

public native static String fstUtilityBinToEsc(byte []s, int len);
public native static byte[] fstUtilityEscToBin(String s);


//
// example do-nothing callback for fstReaderIterateHier()
//
public void fstReaderCallback(long tim, int facidx, String value)
{
}

}
