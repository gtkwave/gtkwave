## -*- makefile -*-
##

BIGFILES = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
AIXFLAGS = -bmaxdata:0xd0000000/dsa
LIBLZMA_CFLAGS = -I$(srcdir)/../liblzma $(LIBXZ_CFLAGS)
LIBLZMA_LDADD  = $(LIBXZ_LDADD)

AM_CFLAGS=	-I$(srcdir)/.. -I$(srcdir)/../.. $(LIBZ_CFLAGS) $(LIBBZ2_CFLAGS) $(LIBLZMA_CFLAGS) $(LIBJUDY_CFLAGS) $(EXTLOAD_CFLAGS) $(RPC_CFLAGS) -I$(srcdir)/fst -I$(srcdir)/../../contrib/rtlbrowse

bin_PROGRAMS= evcd2vcd fst2vcd vcd2fst fstminer ghwdump lxt2miner lxt2vcd \
	shmidcat vcd2lxt vcd2lxt2 vcd2vzt \
	vzt2vcd vztminer

vcd2fst_SOURCES= vcd2fst.c $(srcdir)/fst/lz4.c $(srcdir)/fst/lz4.h $(srcdir)/fst/fastlz.c $(srcdir)/fst/fastlz.h $(srcdir)/fst/fstapi.c $(srcdir)/fst/fstapi.h $(srcdir)/../../contrib/rtlbrowse/jrb.h $(srcdir)/../../contrib/rtlbrowse/jrb.c
vcd2fst_LDADD= $(LIBZ_LDADD) $(LIBJUDY_LDADD)

fst2vcd_SOURCES= fst2vcd.c $(srcdir)/fst/lz4.c $(srcdir)/fst/lz4.h $(srcdir)/fst/fastlz.c $(srcdir)/fst/fastlz.h $(srcdir)/fst/fstapi.c $(srcdir)/fst/fstapi.h
fst2vcd_LDADD= $(LIBZ_LDADD) $(LIBJUDY_LDADD)

fstminer_SOURCES= fstminer.c $(srcdir)/fst/lz4.c $(srcdir)/fst/lz4.h $(srcdir)/fst/fastlz.c $(srcdir)/fst/fastlz.h $(srcdir)/fst/fstapi.c $(srcdir)/fst/fstapi.h
fstminer_LDADD= $(LIBZ_LDADD) $(LIBJUDY_LDADD)

vcd2lxt_SOURCES= vcd2lxt.c lxt_write.c lxt_write.h v2l_analyzer.h v2l_debug.c v2l_debug.h
vcd2lxt_LDADD= $(LIBZ_LDADD) $(LIBBZ2_LDADD)

lxt2vcd_SOURCES= lxt2_read.c lxt2_read.h lxt2vcd.c scopenav.c
lxt2vcd_LDADD= $(LIBZ_LDADD)

vcd2lxt2_SOURCES= vcd2lxt2.c lxt2_write.c lxt2_write.h v2l_analyzer_lxt2.h v2l_debug_lxt2.c v2l_debug_lxt2.h
vcd2lxt2_LDADD= $(LIBZ_LDADD)

vzt2vcd_SOURCES= vzt_read.c vzt_read.h vzt2vcd.c scopenav.c $(srcdir)/../liblzma/LzmaLib.c $(srcdir)/../liblzma/LzmaLib.h
vzt2vcd_LDADD= $(LIBZ_LDADD) $(LIBBZ2_LDADD) $(LIBLZMA_LDADD) $(RPC_LDADD)

vcd2vzt_SOURCES= vcd2vzt.c vzt_write.c vzt_write.h v2l_analyzer_lxt2.h v2l_debug_lxt2.c v2l_debug_lxt2.h $(srcdir)/../liblzma/LzmaLib.c $(srcdir)/../liblzma/LzmaLib.h
vcd2vzt_LDADD= $(LIBZ_LDADD) $(LIBBZ2_LDADD) $(LIBLZMA_LDADD) $(RPC_LDADD)

vztminer_SOURCES= vztminer.c vzt_read.c vzt_read.h $(srcdir)/../liblzma/LzmaLib.c $(srcdir)/../liblzma/LzmaLib.h
vztminer_LDADD= $(LIBZ_LDADD) $(LIBBZ2_LDADD) $(LIBLZMA_LDADD) $(RPC_LDADD)

lxt2miner_SOURCES= lxt2miner.c lxt2_read.c lxt2_read.h
lxt2miner_LDADD= $(LIBZ_LDADD)

ghwdump_SOURCES= ghwdump.c $(srcdir)/../ghwlib.c

evcd2vcd_SOURCES= evcd2vcd.c $(srcdir)/../../contrib/rtlbrowse/jrb.h $(srcdir)/../../contrib/rtlbrowse/jrb.c
