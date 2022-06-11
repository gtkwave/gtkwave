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

#include <jni.h>
#include <string.h>
#include "fstapi.h"

/*
 * fstWriter
 */

JNIEXPORT jlong JNICALL Java_fstAPI_fstWriterCreate
  (JNIEnv *env, jobject obj, jstring j_nam, jboolean use_compressed_hier)
{
void *ctx;
const char *nam = (*env)->GetStringUTFChars(env, j_nam, 0);

ctx = fstWriterCreate(nam, (int)use_compressed_hier);

(*env)->ReleaseStringUTFChars(env, j_nam, nam);

return((jlong)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterClose
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstWriterClose((void *)(long)ctx);
}


JNIEXPORT jboolean JNICALL Java_fstAPI_fstWriterGetFseekFailed
  (JNIEnv *env, jobject obj, jlong ctx)
{
int rc = fstWriterGetFseekFailed((void *)(long)ctx);

return(rc != 0);
}


JNIEXPORT jboolean JNICALL Java_fstAPI_fstWriterGetDumpSizeLimitReached
  (JNIEnv *env, jobject obj, jlong ctx)
{
int rc = fstWriterGetDumpSizeLimitReached((void *)(long)ctx);

return(rc != 0);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterFlushContext
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstWriterFlushContext((void *)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetUpscope
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstWriterSetUpscope((void *)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetAttrEnd
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstWriterSetAttrEnd((void *)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetPackType
  (JNIEnv *env, jobject obj, jlong ctx, jint typ)
{
fstWriterSetPackType((void *)(long)ctx, (int)typ);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetFileType
  (JNIEnv *env, jobject obj, jlong ctx, jint typ)
{
fstWriterSetFileType((void *)(long)ctx, (int)typ);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetRepackOnClose
  (JNIEnv *env, jobject obj, jlong ctx, jboolean enable)
{
fstWriterSetRepackOnClose((void *)(long)ctx, (int)enable);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetParallelMode
  (JNIEnv *env, jobject obj, jlong ctx, jboolean enable)
{
fstWriterSetParallelMode((void *)(long)ctx, (int)enable);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetTimescale
  (JNIEnv *env, jobject obj, jlong ctx, jint ts)
{
fstWriterSetParallelMode((void *)(long)ctx, (int)ts);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetTimezero
  (JNIEnv *env, jobject obj, jlong ctx, jlong tim)
{
fstWriterSetTimezero((void *)(long)ctx, (int64_t)tim);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetDumpSizeLimit
  (JNIEnv *env, jobject obj, jlong ctx, jlong numbytes)
{
fstWriterSetDumpSizeLimit((void *)(long)ctx, (uint64_t)numbytes);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterEmitDumpActive
  (JNIEnv *env, jobject obj, jlong ctx, jboolean enable)
{
fstWriterEmitDumpActive((void *)(long)ctx, (int)enable);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterEmitTimeChange
  (JNIEnv *env, jobject obj, jlong ctx, jlong tim)
{
fstWriterEmitTimeChange((void *)(long)ctx, (uint64_t)tim);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetDate
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_dat)
{
const char *dat = (*env)->GetStringUTFChars(env, j_dat, 0);

fstWriterSetDate((void *)(long)ctx, dat);

(*env)->ReleaseStringUTFChars(env, j_dat, dat);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetVersion
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_vers)
{
const char *vers = (*env)->GetStringUTFChars(env, j_vers, 0);

fstWriterSetVersion((void *)(long)ctx, vers);

(*env)->ReleaseStringUTFChars(env, j_vers, vers);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetComment
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_comm)
{
const char *comm = (*env)->GetStringUTFChars(env, j_comm, 0);

fstWriterSetComment((void *)(long)ctx, comm);

(*env)->ReleaseStringUTFChars(env, j_comm, comm);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetEnvVar
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_envvar)
{
const char *envvar = (*env)->GetStringUTFChars(env, j_envvar, 0);

fstWriterSetEnvVar((void *)(long)ctx, envvar);

(*env)->ReleaseStringUTFChars(env, j_envvar, envvar);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetTimescaleFromString
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_s)
{
const char *s = (*env)->GetStringUTFChars(env, j_s, 0);

fstWriterSetTimescaleFromString((void *)(long)ctx, s);

(*env)->ReleaseStringUTFChars(env, j_s, s);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterEmitValueChange__JILjava_lang_String_2
  (JNIEnv *env, jobject obj, jlong ctx, jint handle, jstring j_val)
{
const char *val = (*env)->GetStringUTFChars(env, j_val, 0);

fstWriterEmitValueChange((void *)(long)ctx, (fstHandle)handle, val);

(*env)->ReleaseStringUTFChars(env, j_val, val);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterEmitValueChange__JID
  (JNIEnv *env, jobject obj, jlong ctx, jint handle, jdouble d)
{
fstWriterEmitValueChange((void *)(long)ctx, (fstHandle)handle, (void *)&d);
}


JNIEXPORT jint JNICALL Java_fstAPI_fstWriterCreateVar
  (JNIEnv *env, jobject obj, jlong ctx, jint vt, jint vd, jint len, jstring j_nam, jint aliasHandle)
{
fstHandle handle;
const char *nam = (*env)->GetStringUTFChars(env, j_nam, 0);

handle = fstWriterCreateVar((void *)(long)ctx, (int)vt, (int)vd, (uint32_t)len, nam, (fstHandle)aliasHandle);

(*env)->ReleaseStringUTFChars(env, j_nam, nam);

return((jint)handle);
}


JNIEXPORT jint JNICALL Java_fstAPI_fstWriterCreateVar2
  (JNIEnv *env, jobject obj, jlong ctx, jint vt, jint vd, jint len, jstring j_nam, jint aliasHandle, jstring j_type, jint svt, jint sdt)
{
fstHandle handle;
const char *nam = (*env)->GetStringUTFChars(env, j_nam, 0);
const char *typ = (*env)->GetStringUTFChars(env, j_type, 0);

handle = fstWriterCreateVar2((void *)(long)ctx, (int)vt, (int)vd, (uint32_t)len, nam, (fstHandle)aliasHandle, typ, (int)svt, (int)sdt);

(*env)->ReleaseStringUTFChars(env, j_type, typ);
(*env)->ReleaseStringUTFChars(env, j_nam, nam);

return((jint)handle);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetSourceStem
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_path, jint line, jboolean use_realpath)
{
const char *path = (*env)->GetStringUTFChars(env, j_path, 0);

fstWriterSetSourceStem((void *)(long)ctx, path, line, use_realpath);

(*env)->ReleaseStringUTFChars(env, j_path, path);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetSourceInstantiationStem
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_path, jint line, jboolean use_realpath)
{
const char *path = (*env)->GetStringUTFChars(env, j_path, 0);

fstWriterSetSourceInstantiationStem((void *)(long)ctx, path, line, use_realpath);

(*env)->ReleaseStringUTFChars(env, j_path, path);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetScope
  (JNIEnv *env, jobject obj, jlong ctx, jint scopetype, jstring j_scopename, jstring j_scopecomp)
{
const char *scopename = (*env)->GetStringUTFChars(env, j_scopename, 0);
const char *scopecomp = (*env)->GetStringUTFChars(env, j_scopecomp, 0);

fstWriterSetScope((void *)(long)ctx, (int)scopetype, scopename, scopecomp);

(*env)->ReleaseStringUTFChars(env, j_scopecomp, scopecomp);
(*env)->ReleaseStringUTFChars(env, j_scopename, scopename);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterEmitVariableLengthValueChange
  (JNIEnv *env, jobject obj, jlong ctx, jint handle, jstring j_val, jint len)
{
const char *val = (*env)->GetStringUTFChars(env, j_val, 0);

fstWriterEmitVariableLengthValueChange((void *)(long)ctx, (fstHandle)handle, val, (uint32_t)len);

(*env)->ReleaseStringUTFChars(env, j_val, val);
}


JNIEXPORT void JNICALL Java_fstAPI_fstWriterSetAttrBegin
  (JNIEnv *env, jobject obj, jlong ctx, jint attrtype, jint subtype, jstring j_attrname, jlong arg)
{
const char *attrname = (*env)->GetStringUTFChars(env, j_attrname, 0);

fstWriterSetAttrBegin((void *)(long)ctx, (int)attrtype, (int)subtype, attrname, (uint64_t)arg);

(*env)->ReleaseStringUTFChars(env, j_attrname, attrname);
}


/*
 * fstReader
 */

JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderOpen
  (JNIEnv *env, jobject obj, jstring j_nam)
{
void *ctx;
const char *nam = (*env)->GetStringUTFChars(env, j_nam, 0);

ctx = fstReaderOpen(nam);

(*env)->ReleaseStringUTFChars(env, j_nam, nam);

return((jlong)(long)ctx);
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderOpenForUtilitiesOnly
  (JNIEnv *env, jobject obj)
{
void *ctx;

ctx = fstReaderOpenForUtilitiesOnly();

return((jlong)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderClose
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstReaderClose((void *)(long)ctx);
}


JNIEXPORT jboolean JNICALL Java_fstAPI_fstReaderIterateHierRewind
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jboolean)(fstReaderIterateHierRewind((void *)(long)ctx) != 0));
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderResetScope
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstReaderResetScope((void *)(long)ctx);
}


JNIEXPORT jint JNICALL Java_fstAPI_fstReaderGetCurrentScopeLen
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jint)fstReaderGetCurrentScopeLen((void *)(long)ctx));
}


JNIEXPORT jint JNICALL Java_fstAPI_fstReaderGetFileType
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jint)fstReaderGetFileType((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetTimezero
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetTimezero((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetStartTime
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetStartTime((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetEndTime
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetEndTime((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetMemoryUsedByWriter
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetMemoryUsedByWriter((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetScopeCount
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetScopeCount((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetVarCount
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetVarCount((void *)(long)ctx));
}


JNIEXPORT jint JNICALL Java_fstAPI_fstReaderGetMaxHandle
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jint)fstReaderGetMaxHandle((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetAliasCount
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetAliasCount((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetValueChangeSectionCount
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jlong)fstReaderGetValueChangeSectionCount((void *)(long)ctx));
}


JNIEXPORT jboolean JNICALL Java_fstAPI_fstReaderGetFseekFailed
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jboolean)(fstReaderGetFseekFailed((void *)(long)ctx) != 0));
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderSetUnlimitedTimeRange
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstReaderSetUnlimitedTimeRange((void *)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderSetLimitTimeRange
  (JNIEnv *env, jobject obj, jlong ctx, jlong start_time, jlong end_time)
{
fstReaderSetLimitTimeRange((void *)(long)ctx, (uint64_t)start_time, (uint64_t)end_time);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderSetVcdExtensions
  (JNIEnv *env, jobject obj, jlong ctx, jboolean enable)
{
fstReaderSetVcdExtensions((void *)(long)ctx, (int)enable);
}


JNIEXPORT jint JNICALL Java_fstAPI_fstReaderGetNumberDumpActivityChanges
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jint)fstReaderGetNumberDumpActivityChanges((void *)(long)ctx));
}


JNIEXPORT jlong JNICALL Java_fstAPI_fstReaderGetDumpActivityChangeTime
  (JNIEnv *env, jobject obj, jlong ctx, jint idx)
{
return((jlong)fstReaderGetDumpActivityChangeTime((void *)(long)ctx, (uint32_t)idx));
}


JNIEXPORT jboolean JNICALL Java_fstAPI_fstReaderGetFacProcessMask
  (JNIEnv *env, jobject obj, jlong ctx, jint facidx)
{
return((jboolean)(fstReaderGetFacProcessMask((void *)(long)ctx, (fstHandle)facidx) != 0));
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderSetFacProcessMask
  (JNIEnv *env, jobject obj, jlong ctx, jint facidx)
{
fstReaderSetFacProcessMask((void *)(long)ctx, (fstHandle)facidx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderClrFacProcessMask
  (JNIEnv *env, jobject obj, jlong ctx, jint facidx)
{
fstReaderClrFacProcessMask((void *)(long)ctx, (fstHandle)facidx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderSetFacProcessMaskAll
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstReaderSetFacProcessMaskAll((void *)(long)ctx);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderClrFacProcessMaskAll
  (JNIEnv *env, jobject obj, jlong ctx)
{
fstReaderClrFacProcessMaskAll((void *)(long)ctx);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderGetVersionString
  (JNIEnv *env, jobject obj, jlong ctx)
{
const char *s = fstReaderGetVersionString((void *)(long)ctx);
jstring j_s = (*env)->NewStringUTF(env, s);

return(j_s);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderGetDateString
  (JNIEnv *env, jobject obj, jlong ctx)
{
const char *s = fstReaderGetDateString((void *)(long)ctx);
jstring j_s = (*env)->NewStringUTF(env, s);

return(j_s);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderPopScope
  (JNIEnv *env, jobject obj, jlong ctx)
{
const char *s = fstReaderPopScope((void *)(long)ctx);
jstring j_s = (*env)->NewStringUTF(env, s);

return(j_s);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderGetCurrentFlatScope
  (JNIEnv *env, jobject obj, jlong ctx)
{
const char *s = fstReaderGetCurrentFlatScope((void *)(long)ctx);
jstring j_s = (*env)->NewStringUTF(env, s);

return(j_s);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderGetCurrentScopeUserInfo
  (JNIEnv *env, jobject obj, jlong ctx)
{
const char *s = fstReaderGetCurrentScopeUserInfo((void *)(long)ctx);
jstring j_s = (*env)->NewStringUTF(env, s);

return(j_s);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderPushScope
  (JNIEnv *env, jobject obj, jlong ctx, jstring j_nam, jlong user_info)
{
const char *nam = (*env)->GetStringUTFChars(env, j_nam, 0);

const char *s = fstReaderPushScope((void *)(long)ctx, nam, (void *)(long)user_info);
jstring j_s = (*env)->NewStringUTF(env, s);

(*env)->ReleaseStringUTFChars(env, j_nam, nam);

return(j_s);
}


JNIEXPORT jint JNICALL Java_fstAPI_fstReaderGetTimescale
  (JNIEnv *env, jobject obj, jlong ctx)
{
return((jint)fstReaderGetTimescale((void *)(long)ctx));
}


JNIEXPORT jboolean JNICALL Java_fstAPI_fstReaderGetDumpActivityChangeValue
  (JNIEnv *env, jobject obj, jlong ctx, jint idx)
{
return((jboolean)(fstReaderGetDumpActivityChangeValue((void *)(long)ctx, (uint32_t)idx) != 0));
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstReaderGetValueFromHandleAtTime
  (JNIEnv *env, jobject obj, jlong ctx, jlong tim, jint facidx)
{
char buf[65537];
const char *s = fstReaderGetValueFromHandleAtTime((void *)(long)ctx, (uint64_t)tim, (fstHandle)facidx, buf);
jstring j_s = (*env)->NewStringUTF(env, s);

return(j_s);
}


JNIEXPORT jstring JNICALL Java_fstAPI_fstUtilityBinToEsc
  (JNIEnv *env, jclass obj, jbyteArray b_s, jint len)
{
jbyte* s = (*env)->GetByteArrayElements(env, b_s,NULL);
char *d = malloc(len * 4);
jstring j_d;
int i;
int rlen;

d[0] = 0;
rlen = fstUtilityBinToEsc(d, s, (int)len);
d[rlen] = 0;
j_d = (*env)->NewStringUTF(env, d);

free(d);
(*env)->ReleaseByteArrayElements(env, b_s, s, 0); 

return(j_d);
}


JNIEXPORT jbyteArray JNICALL Java_fstAPI_fstUtilityEscToBin
  (JNIEnv *env, jclass obj, jstring j_s)
{
const char *s = (*env)->GetStringUTFChars(env, j_s, 0);
char *d = strdup(s);
int len = strlen(s);
jbyteArray result;

int rlen = fstUtilityEscToBin(d, d, len);

(*env)->ReleaseStringUTFChars(env, j_s, s);

result=(*env)->NewByteArray(env, rlen);

(*env)->SetByteArrayRegion(env, result, 0, rlen, d);
free(d);

return(result);
}


JNIEXPORT void JNICALL Java_fstAPI_fstReaderIterateHier
  (JNIEnv *env, jobject obj, jlong ctx, jobject obj2)
{
struct fstHier *fh = fstReaderIterateHier((void *)(long)ctx);

jclass javaDataClass = (*env)->FindClass(env, "fstHier");
jfieldID validField = (*env)->GetFieldID(env, javaDataClass, "valid", "Z");
jfieldID htypField = (*env)->GetFieldID(env, javaDataClass, "htyp", "I");
jfieldID typField = (*env)->GetFieldID(env, javaDataClass, "typ", "I");
jfieldID subtypeField;
jfieldID directionField;
jfieldID lengthField;
jfieldID handleField;
jfieldID name1Field;
jfieldID name2Field;
jfieldID argField;
jfieldID arg_from_nameField;
jfieldID isAliasField;
jstring j_name1;
jstring j_name2;

(*env)->SetBooleanField(env, obj2, validField, (fh != NULL));

if(fh)
	{
	(*env)->SetIntField(env, obj2, htypField, fh->htyp);
	
	switch(fh->htyp)
		{
		case FST_HT_SCOPE:
			(*env)->SetIntField(env, obj2, typField, fh->u.scope.typ);

			j_name1 = (*env)->NewStringUTF(env, fh->u.scope.name);
			name1Field = (*env)->GetFieldID(env, javaDataClass, "name1", "Ljava/lang/String;");
			(*env)->SetObjectField(env, obj2, name1Field, j_name1);

			j_name2 = (*env)->NewStringUTF(env, fh->u.scope.component);
			name2Field = (*env)->GetFieldID(env, javaDataClass, "name2", "Ljava/lang/String;");
			(*env)->SetObjectField(env, obj2, name2Field, j_name2);
			break;

		case FST_HT_UPSCOPE:
			(*env)->SetIntField(env, obj2, typField, FST_ST_VCD_UPSCOPE);
			break;

		case FST_HT_VAR:
			(*env)->SetIntField(env, obj2, typField, fh->u.var.typ);

			directionField = (*env)->GetFieldID(env, javaDataClass, "direction", "I");
			(*env)->SetIntField(env, obj2, directionField, (jint)fh->u.var.direction);

			j_name1 = (*env)->NewStringUTF(env, fh->u.var.name);
			name1Field = (*env)->GetFieldID(env, javaDataClass, "name1", "Ljava/lang/String;");
			(*env)->SetObjectField(env, obj2, name1Field, j_name1);

			lengthField = (*env)->GetFieldID(env, javaDataClass, "length", "I");
			(*env)->SetIntField(env, obj2, lengthField, (jint)fh->u.var.length);

			handleField = (*env)->GetFieldID(env, javaDataClass, "handle", "I");
			(*env)->SetIntField(env, obj2, handleField, (jint)fh->u.var.handle);

			isAliasField = (*env)->GetFieldID(env, javaDataClass, "is_alias", "Z");
			(*env)->SetBooleanField(env, obj2, isAliasField, (jboolean)(fh->u.var.is_alias));
			break;

		case FST_HT_ATTRBEGIN:
			(*env)->SetIntField(env, obj2, typField, fh->u.attr.typ);

			subtypeField = (*env)->GetFieldID(env, javaDataClass, "subtype", "I");
			(*env)->SetIntField(env, obj2, subtypeField, fh->u.attr.subtype);

			j_name1 = (*env)->NewStringUTF(env, fh->u.attr.name);
			name1Field = (*env)->GetFieldID(env, javaDataClass, "name1", "Ljava/lang/String;");
			(*env)->SetObjectField(env, obj2, name1Field, j_name1);

			argField = (*env)->GetFieldID(env, javaDataClass, "arg", "J");
			(*env)->SetLongField(env, obj2, argField, fh->u.attr.arg);

			arg_from_nameField = (*env)->GetFieldID(env, javaDataClass, "arg_from_name", "J");
			(*env)->SetLongField(env, obj2, arg_from_nameField, fh->u.attr.arg_from_name);
			break;

		case FST_HT_ATTREND:
			(*env)->SetIntField(env, obj2, typField, FST_ST_GEN_ATTREND);
			break;

		default:
			break;
		}
	}
}


struct jni_fstCB_t
{
JNIEnv *env;
jobject obj;
void *ctx;
jobject cbobj;
jclass cls;

jmethodID mid;
};


static void value_change_callback2(void *user_callback_data_pointer, uint64_t time, fstHandle facidx, const unsigned char *value, uint32_t len)
{
struct jni_fstCB_t *jni = (struct jni_fstCB_t *)user_callback_data_pointer;
jstring j_s = (*jni->env)->NewStringUTF(jni->env, value);
(*jni->env)->CallVoidMethod(jni->env, jni->cbobj, jni->mid, (jlong)time, (jint)facidx, j_s);
}

static void value_change_callback(void *user_callback_data_pointer, uint64_t time, fstHandle facidx, const unsigned char *value)
{
value_change_callback2(user_callback_data_pointer, time, facidx, value, 0);
}



JNIEXPORT jint JNICALL Java_fstAPI_fstReaderIterBlocks
  (JNIEnv *env, jobject obj, jlong ctx, jobject cbobj)
{
struct jni_fstCB_t cb;
jint rc;

jclass cls = (*env)->GetObjectClass(env, cbobj);
jmethodID mid = (*env)->GetMethodID(env, cls, "fstReaderCallback", "(JILjava/lang/String;)V");   

cb.env = env;
cb.obj = obj;
cb.ctx = (void *)(long)ctx;
cb.cbobj = cbobj;
cb.cls = cls;
cb.mid = mid;

rc = fstReaderIterBlocks2((void *)(long)ctx,   
        value_change_callback,
        value_change_callback2,
        &cb, NULL);

return(rc);
}
