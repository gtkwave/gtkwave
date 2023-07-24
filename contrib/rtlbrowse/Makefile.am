## -*- makefile -*-
##

# FIXME -- how to figure out automatically when we need these?
# 
SUBDIRS= 

AET2LIB = 
AIXFLAGS = -bmaxdata:0xd0000000/dsa -D_WAVE_BE32
BIGFILES = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
LIBFST_CFLAGS  = -I$(srcdir)/../../src/helpers/fst
LIBLZMA_CFLAGS = -I$(srcdir)/../../src/liblzma $(LIBXZ_CFLAGS)
LIBLZMA_LDADD  = $(LIBXZ_LDADD)


bin_PROGRAMS= rtlbrowse

rtlbrowse_SOURCES = definehash.c fgetdynamic.c fgetdynamic.h \
	logfile.c splay.c tcl_helper.c tree_widget.c \
	$(srcdir)/../../src/helpers/fst/lz4.c \
	$(srcdir)/../../src/helpers/fst/lz4.h \
	$(srcdir)/../../src/helpers/fst/fastlz.c \
	$(srcdir)/../../src/helpers/fst/fastlz.h \
	$(srcdir)/../../src/helpers/fst/fstapi.c \
	$(srcdir)/../../src/helpers/fst/fstapi.h \
	stem_recurse.c jrb.c jrb.h $(srcdir)/../../src/helpers/vzt_read.c $(srcdir)/../../src/helpers/vzt_read.h \
	$(srcdir)/../../src/helpers/lxt2_read.c $(srcdir)/../../src/helpers/lxt2_read.h splay.h \
	vlex.l vlex.h wavelink.h $(srcdir)/../../src/liblzma/LzmaLib.c $(srcdir)/../../src/liblzma/LzmaLib.h

LDADD= $(GTK_LIBS) $(LIBZ_LDADD) $(LIBBZ2_LDADD) $(LIBLZMA_LDADD) $(AET2_LDADD) \
	$(LIBJUDY_LDADD) $(GTK_MAC_LIBS) $(GCONF_LIBS)
AM_CFLAGS= -I$(top_srcdir) -I$(top_srcdir)/src  -I$(top_srcdir)/src/helpers \
	$(GTK_CFLAGS) $(LIBZ_CFLAGS) $(LIBBZ2_CFLAGS) $(LIBLZMA_CFLAGS) \
	$(LIBFST_CFLAGS) $(AET2_CFLAGS) $(LIBJUDY_CFLAGS) \
	$(GTK_MAC_CFLAGS) $(GCONF_CFLAGS) -DYY_NO_INPUT

definehash.c: vpp_keyword.gperf
	$(GPERF) -C -I -N is_builtin_define $(srcdir)/vpp_keyword.gperf >definehash.c

BUILT_SOURCES= definehash.c

EXTRA_DIST= vpp_keyword.gperf

