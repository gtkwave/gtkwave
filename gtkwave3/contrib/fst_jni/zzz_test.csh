#!/bin/csh

setenv LD_LIBRARY_PATH `pwd`
javac \
	fstAPI.java \
	fstArrayType.java \
	fstAttrType.java \
	fstEnumValueType.java \
	fstFileType.java \
	fstHier.java \
	fstHierType.java \
	fstMiscType.java \
	fstPackType.java \
	fstScopeType.java \
	fstSupplementalDataType.java \
	fstSupplementalVarType.java \
	fstVarDir.java \
	fstVarType.java \
	fstWriter.java \
	fstWriterPackType.java \
	fstReader.java
			 
javac \
	fst2Vcd.java \
	Main.java

javah -jni fstAPI
gcc -o libfstAPI.so -fPIC -shared -Wl,-soname,fstAPI.so fstAPI.c ../../src/helpers/fst/fstapi.c ../../src/helpers/fst/fastlz.c \
	-I./ -I../../ -I../../src/helpers/fst/ -lz
java Main /tmp/des.fst

rm *.class
rm libfstAPI.so

