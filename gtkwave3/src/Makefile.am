## -*- makefile -*-
##

SUBDIRS=	$(LIBZ_DIR) $(LIBBZ2_DIR) liblzma helpers helpers/fst cocoa
DIST_SUBDIRS=	libz libbz2 liblzma helpers helpers/fst cocoa

LIBFST_CFLAGS  = -I$(srcdir)/helpers/fst
LIBFST_LDADD   = ./helpers/fst/libfst.a
LIBLZMA_CFLAGS = -I$(srcdir)/liblzma $(LIBXZ_CFLAGS)
LIBLZMA_LDADD  = ./liblzma/libgwlzma.a $(LIBXZ_LDADD)
LIBCOCOA_CFLAGS = -I$(srcdir)/cocoa
LIBCOCOA_LDADD = ./cocoa/libgtkwmacintegration.a $(COCOA_GTK_LDADD)
#DEBUGS = -DDEBUG_FACILITIES -DDEBUG_PRINTF -DDEBUG_MALLOC -DSTRICT_VCD_ONLY -DDEBUG_MALLOC_LINES

bin_PROGRAMS=	gtkwave twinwave

AM_CFLAGS=	-I$(srcdir)/.. -I$(srcdir)/helpers $(FASTTREE_CFLAGS) $(GTK_CFLAGS) $(LIBLZMA_CFLAGS) \
		$(LIBZ_CFLAGS) $(LIBBZ2_CFLAGS) $(LIBFST_CFLAGS) $(AET2_CFLAGS) $(FSDB_CFLAGS) $(TCL_INCLUDE_SPEC) \
		$(TCL_DEFADD) $(TK_INCLUDE_SPEC) $(EXTLOAD_CFLAGS) $(GEDIT_CFLAGS) $(LIBJUDY_CFLAGS) \
		$(GTK_MAC_CFLAGS) $(GCONF_CFLAGS) $(GSETTINGS_CFLAGS) $(LIBCOCOA_CFLAGS) $(GTK_UNIX_PRINT_CFLAGS)

AM_CXXFLAGS=	$(AM_CFLAGS)

./liblzma/libgwlzma.a: $(srcdir)/liblzma/LzmaLib.c $(srcdir)/liblzma/LzmaLib.h

./helpers/fst/libfst.a: $(srcdir)/helpers/fst/fastlz.c $(srcdir)/helpers/fst/fastlz.h $(srcdir)/helpers/fst/fstapi.c $(srcdir)/helpers/fst/fstapi.h

./cocoa/libgtkwmacintegration.a: $(srcdir)/cocoa/cocoa_misc.c $(srcdir)/cocoa/cocoa_misc.h

gtkwave_SOURCES= \
	fsdb_wrapper_api.cc fsdb_wrapper_api.h \
	baseconvert.h wavewindow.h zoombuttons.h fetchbuttons.h timeentry.h shiftbuttons.h pagebuttons.h edgebuttons.h \
	signalwindow.h entry.h file.h status.h search.h showchange.h treesearch.h hiersearch.h renderopt.h markerbox.h \
	simplereq.h help.h logfile.h vcd_partial.h mouseover.h mouseover_sigs.h interp.h \
	globals.c globals.h ae2.c ae2.h analyzer.c analyzer.h baseconvert.c bitvec.c \
	bsearch.c bsearch.h busy.c busy.h \
	clipping.c clipping.h color.c color.h currenttime.c currenttime.h \
	debug.c debug.h discardbuttons.c edgebuttons.c \
	entry.c extload.c extload.h fetchbuttons.c fgetdynamic.c fgetdynamic.h file.c fonts.c fonts.h fst.c fst.h \
	gconf.c gconf.h getopt.c \
	getopt1.c ghw.c ghw.h ghwlib.c ghwlib.h gnu-getopt.h gnu_regex.h gtk12compat.h \
	hierpack.c hierpack.h jrb.c jrb.h help.c helpers/lxt2_read.c \
	helpers/lxt_write.c helpers/vzt_read.c hiersearch.c interp.c \
	logfile.c lx2.c lx2.h lxt.c lxt.h main.c main.h markerbox.c menu.c menu.h mouseover.c \
	mouseover_sigs.c pagebuttons.c pipeio.c pipeio.h pixmaps.c pixmaps.h print.c print.h \
	ptranslate.c ptranslate.h rc.c rc.h \
	regex.c regex_wave.h renderopt.c rgb.c savefile.c savefile.h search.c shiftbuttons.c showchange.c \
	signalwindow.c simplereq.c splash.c status.c strace.c strace.h symbol.c symbol.h tcl_callbacks.h \
        tcl_commands.c tcl_helper.c tcl_helper.h tcl_np.c tcl_np.h tcl_support_commands.c tcl_support_commands.h \
	timeentry.c translate.c translate.h tree.c tree.h treesearch.c tree_component.c tree_component.h \
	ttranslate.c ttranslate.h vcd.c vcd.h vcd_keywords.c \
	vcd_partial.c vcd_recoder.c vcd_saver.c vcd_saver.h version.h vlist.c vlist.h vzt.c vzt.h wavealloca.h \
	wavewindow.c zoombuttons.c

gtkwave_LDADD= $(LIBCOCOA_LDADD) $(GTK_LIBS) $(LIBLZMA_LDADD) $(LIBZ_LDADD) $(LIBBZ2_LDADD) $(LIBFST_LDADD) $(AET2_LDADD) $(TCL_LDADD) $(TK_LDADD) $(FSDB_LDADD) $(LIBJUDY_LDADD) $(GTK_MAC_LIBS) $(GCONF_LIBS) $(GTK_UNIX_PRINT_LIBS) $(MINGW_LDADD)
gtkwave_LDFLAGS = $(COCOA_GTK_LDFLAGS)

twinwave_SOURCES= twinwave.c
twinwave_LDADD= $(GTK_LIBS) $(GTK_MAC_LIBS) $(GCONF_LIBS)

vcd_keywords.c: vcd_keywords.gperf
	printf "$(GPERF) -o -i 1 -C -k 1,\044 -L C -H keyword_hash -N check_identifier -tT --initializer-suffix=,0 $(srcdir)/vcd_keywords.gperf >vcd_keywords.c" | sh

gsettings_SCHEMAS = com.geda.gtkwave.gschema.xml
@GSETTINGS_RULES@

BUILT_SOURCES= vcd_keywords.c

# I'm listing treesearch_gtk2.c here instead of in SOURCES because we don't directly
# compile it.  Rather it is #include'd by treesearch.c.
EXTRA_DIST= treesearch_gtk1.c treesearch_gtk2.c vcd_keywords.gperf gnu_regex.c com.geda.gtkwave.gschema.xml
