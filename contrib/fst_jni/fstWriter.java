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

public class fstWriter extends fstAPI 
{
private long ctx;

fstWriter(String nam, boolean use_compressed_hier)
{
ctx = fstWriterCreate(nam, use_compressed_hier);
}

protected void finalize() throws Throwable 
{
try	{
	fstWriterClose(ctx);
     	} finally 
	{
        super.finalize();
     	}
}


public void 	fstWriterClose() { fstWriterClose(ctx); ctx = 0; }
public int 	fstWriterCreateVar(int vt, int vd, int len, String nam, int aliasHandle) { return(fstWriterCreateVar(ctx, vt, vd, len, nam, aliasHandle)); }
public void 	fstWriterEmitDumpActive(boolean enable) { fstWriterEmitDumpActive(ctx, enable); }
public void 	fstWriterEmitTimeChange(long tim) { fstWriterEmitTimeChange(ctx, tim); }
public void 	fstWriterEmitValueChange(int handle, double val) { fstWriterEmitValueChange(ctx, handle, val); }
public void 	fstWriterEmitValueChange(int handle, int val) { fstWriterEmitValueChange(ctx, handle, val); }
public void 	fstWriterEmitValueChange(int handle, String val) { fstWriterEmitValueChange(ctx, handle, val); }
public void 	fstWriterEmitVariableLengthValueChange(int handle, String val, int len) { fstWriterEmitVariableLengthValueChange(ctx, handle, val, len); }
public void 	fstWriterFlushContext() { fstWriterFlushContext(ctx); }
public boolean 	fstWriterGetDumpSizeLimitReached() { return(fstWriterGetDumpSizeLimitReached(ctx)); }
public boolean 	fstWriterGetFseekFailed() { return(fstWriterGetFseekFailed(ctx)); }
public void 	fstWriterSetAttrBegin(int attrtype, int subtype, String attrname, long arg) { fstWriterSetAttrBegin(ctx, attrtype, subtype, attrname, arg); }
public void 	fstWriterSetAttrEnd() { fstWriterSetAttrEnd(ctx); }
public void 	fstWriterSetComment(String comm) { fstWriterSetComment(ctx, comm); }
public void 	fstWriterSetDate(String dat) { fstWriterSetDate(ctx, dat); }
public void 	fstWriterSetDumpSizeLimit(long numbytes) { fstWriterSetDumpSizeLimit(ctx, numbytes); }
public void 	fstWriterSetEnvVar(String envvar) { fstWriterSetEnvVar(ctx, envvar); }
public void     fstWriterSetFileType(int typ) { fstWriterSetFileType(ctx, typ); }
public void 	fstWriterSetPackType(int typ) { fstWriterSetPackType(ctx, typ); }
public void 	fstWriterSetParallelMode(boolean enable) { fstWriterSetParallelMode(ctx, enable); }
public void 	fstWriterSetRepackOnClose(boolean enable) { fstWriterSetRepackOnClose(ctx, enable); }
public void 	fstWriterSetScope(int scopetype, String scopename, String scopecomp) { fstWriterSetScope(ctx, scopetype, scopename, scopecomp); }
public void 	fstWriterSetTimescaleFromString(String s) { fstWriterSetTimescaleFromString(ctx, s); }
public void 	fstWriterSetTimescale(int ts) { fstWriterSetTimescale(ctx, ts); }
public void 	fstWriterSetTimezero(long tim) { fstWriterSetTimezero(ctx, tim); }
public void 	fstWriterSetUpscope() { fstWriterSetUpscope(ctx); }
public void 	fstWriterSetVersion(String vers) { fstWriterSetVersion(ctx, vers); }
}
