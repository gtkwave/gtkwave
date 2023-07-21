## -*- makefile -*-
##
## To compile this, add AC_PROG_LIBTOOL to configure.ac
## also add contrib/vpi/Makefile to AC_CONFIG_FILES
## also run libtoolize to generate ltmain.sh.
##
## This is not built by default.  In the future this will be a normal part of
## the build process.
##
## Note that this explicitly uses the libz library found in this
## distro as this might be necessary if your simulator is not already linked
## against it.
##
## 20jan09ajb
##

lib_LTLIBRARIES=	libsysfst.la

libsysvzt_la_SOURCES= ../../src/libz/adler32.c ../../src/libz/compress.c ../../src/libz/crc32.c ../../src/libz/crc32.h \
	../../src/libz/deflate.c ../../src/libz/deflate.h ../../src/libz/gzio.c ../../src/libz/infback.c ../../src/libz/inffast.c \
	../../src/libz/inffast.h ../../src/libz/inffixed.h ../../src/libz/inflate.c ../../src/libz/inflate.h \
	../../src/libz/inftrees.c ../../src/libz/inftrees.h ../../src/libz/trees.c ../../src/libz/trees.h ../../src/libz/uncompr.c \
	../../src/libz/zconf.h ../../src/libz/zlib.h ../../src/libz/zutil.c ../../src/libz/zutil.h \
	../../src/helpers/fst/fstapi.c ../../src/helpers/fst/fstapi.h ../../src/helpers/fst/fastlz.c ../../src/helpers/fst/fastlz.h \
	sys_fst.c


libsysfst_la_LDFLAGS = -version-info 1:0:0
	
AM_CFLAGS= -I../../src/libz/ -I../../src/helpers/fst/ -I.
