#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gtkwave, 3.3.104, bybell@rocketmail.com)
AC_CONFIG_SRCDIR([src/vcd.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

# ------------- GCONF -------------------

AC_ARG_WITH([gconf],
    [AS_HELP_STRING([--with-gconf],
        [Use GConf to store preferences])],
        [],
        [with_gconf=check])

# ------------- GSETTINGS -------------------

AC_ARG_WITH([gsettings],
    [AS_HELP_STRING([--with-gsettings],
        [Use GSettings to store preferences (precludes GConf)])],
        [],
	[with_gsettings=check])


# ------------ TI-RPC ------------------

AC_ARG_WITH([tirpc],
      [AS_HELP_STRING([--with-tirpc], [Use TI-RPC as RPC implementation (instead of SunRPC)])])

# ------------- XDG -------------------
AC_ARG_WITH(xdgdatadir, [  --with-xdgdatadir=path  Change where the theme icons and mime registrations are installed [[DATADIR]]], [opt_xdgdatadir=$withval])

if test x$opt_xdgdatadir = x; then
        # path was not specified with --with-xdgdatadir
        XDGDATADIR='${datadir}'
 else
        # path WAS specified with --with-xdgdatadir
        XDGDATADIR="$opt_xdgdatadir"
 fi
AC_SUBST(XDGDATADIR)



AC_MSG_CHECKING([if MIME and desktop updates should be disabled])

EMIM=yes
DMIM=no
AC_ARG_ENABLE([mime_update],
[  --disable-mime-update   Disables MIME type and desktop/icon updating],
[
if test "X$enable_mime_update" = "Xno" ; then
        EMIM=no
        DMIM=yes
else
    	EMIM=yes
        DMIM=no
fi
],
[
EMIM=yes
])
AC_MSG_RESULT([$DMIM])


AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)
AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database, no)
if test "X$DMIM" = "Xyes" ; then
	UPDATE_MIME_DATABASE=:
	UPDATE_DESKTOP_DATABASE=:
fi


AM_CONDITIONAL(FDO_MIME, test x$UPDATE_MIME_DATABASE != xno -a x$UPDATE_DESKTOP_DATABASE != xno)

# ------------- Set simarama base -------------------
if test "X$SIMARAMA_BASE" = "X" ; then
	SIMARAMA_BASE="/afs/awd/projects/simarama/releases/latest"
fi

# ------------- Set fsdbreader base -------------------
if test "X$FSDBREADER_HDRS" = "X" ; then
	FSDBREADER_HDRS="/pub/FsdbReader"
fi
if test "X$FSDBREADER_LIBS" = "X" ; then
	FSDBREADER_LIBS="/pub/FsdbReader"
fi


# Checks for build options
# ------------- Structure Packing -------------------

AC_MSG_CHECKING([if structure packing should be enabled])

ESTP=no
STRUCT_PACK=
AC_ARG_ENABLE([struct_pack],
[  --enable-struct-pack    Enables C language structure packing pragmas
                          intended for use on architectures which support
                          misaligned loads and stores (x86/x86_64/AVR32/
                          M68K/PowerPC/S390).  This can reduce memory on 
                          traces with very many signals.],
[
if test "X$enable_struct_pack" = "Xyes" ; then
	ESTP=yes
	STRUCT_PACK=-DWAVE_USE_STRUCT_PACKING
fi
])
AC_MSG_RESULT([$ESTP])
AC_SUBST(STRUCT_PACK)

# ------------- Tcl -------------------

AC_MSG_CHECKING([if Tcl usage should be disabled])

ETCL=yes
DTCL=no
AC_ARG_ENABLE([tcl],
[  --disable-tcl           Disables Tcl if found],
[
if test "X$enable_tcl" = "Xno" ; then
	ETCL=no
	DTCL=yes
else
	ETCL=yes
	DTCL=no
fi
],
[
ETCL=yes
])
AC_MSG_RESULT([$DTCL])


if test "X$ETCL" = "Xyes" ; then
	SC_PATH_TCLCONFIG
	if test "X$TCL_BIN_DIR" = "X# no Tcl configs found" ; then
		AC_MSG_WARN([Tcl not found, skipping.])
		ETCL=no
	        DTCL=yes
	else
		SC_LOAD_TCLCONFIG
		AC_SUBST(TCL_INCLUDE_SPEC)
		AC_SUBST(TCL_LIB_SPEC)

		AC_SUBST(TCL_MAJOR_VERSION)
		AC_SUBST(TCL_MINOR_VERSION)

		if test "$TCL_MAJOR_VERSION" -lt "8" ; then
			 AC_MSG_ERROR([Upgrade to at least Tcl version 8.4.])
		else
			if test "$TCL_MAJOR_VERSION" -eq "8" ; then
				if test "$TCL_MINOR_VERSION" -lt "4" ; then
					 AC_MSG_ERROR([Upgrade to at least Tcl version 8.4.])
				fi
			fi
		fi
	fi
fi

if test "X$ETCL" = "Xyes" ; then
	SC_PATH_TKCONFIG
	if test "X$TK_BIN_DIR" = "X# no Tk configs found" ; then
		AC_MSG_WARN([Tk not found, skipping.])
		ETCL=no
	        DTCL=yes
	else
		SC_LOAD_TKCONFIG
		AC_SUBST(TK_INCLUDE_SPEC)
		AC_SUBST(TK_LIB_SPEC)
	fi
fi

AC_MSG_CHECKING([if Tcl/Tk (if present) should be stubified])

STUBIFY=no
AC_ARG_ENABLE([stubify],
[  --enable-stubify        Causes the Tcl/Tk libraries to be dynamically loaded.],
[
if test "X$enable_stubify" = "Xno" ; then
	STUBIFY=no
else
	STUBIFY=yes
fi
],
[
STUBIFY=no
])

AC_MSG_RESULT([$STUBIFY])


AC_MSG_CHECKING([if gtk1 or gtk2 should be used])

GTK1=no
AC_ARG_ENABLE([gtk1],
[  --enable-gtk1           Causes the GTK+ frontend to be built with gtk1 
                          instead of gtk2.],
[
if test "X$enable_gtk1" = "Xno" ; then
	GTK1=no
else
	GTK1=yes
fi
],
[
GTK1=no
])

if test "X$GTK1" = "Xyes" ; then
	AC_MSG_RESULT([gtk1])
	CPPFLAGS="$CPPFLAGS"
else
	AC_MSG_RESULT([gtk2])
	CPPFLAGS="-DWAVE_USE_GTK2 $CPPFLAGS"
fi


# check for "fisher price" (simvision rendering style) mode
AC_MSG_CHECKING([if fatlines should be used])

FLN=no
AC_ARG_ENABLE([fatlines],
[  --enable-fatlines       Renders lines as double width in gtkwave.],
[
if test "X$enable_fatlines" = "Xno" ; then
	FLN=no
else
	FLN=yes
fi
],
[
FLN=no
])

if test "X$FLN" = "Xno" ; then
	AC_MSG_RESULT([no])
else
	AC_MSG_RESULT([yes])
	CPPFLAGS="-DWAVE_DOUBLE_LINE_WIDTH_MODE $CPPFLAGS"
fi


# check for massive amounts of named markers enabled
AC_MSG_CHECKING([if manymarkers should be used])

MMC=no
AC_ARG_ENABLE([manymarkers],
[  --enable-manymarkers    Expands named marker count from 26 to 702.],
[
if test "X$enable_manymarkers" = "Xno" ; then
	MMC=no
else
	MMC=yes
fi
],
[
MMC=no
])

if test "X$MMC" = "Xno" ; then
	AC_MSG_RESULT([no])
else
	AC_MSG_RESULT([yes])
	CPPFLAGS="-DWAVE_MANYMARKERS_MODE $CPPFLAGS"
fi


AC_MSG_CHECKING([if ae2 support should be used])

AE2_CFLAGS=
AET2_LDADD=
AE2=no
AC_ARG_ENABLE([ae2],
[  --enable-ae2            Compiles in AE2 from env var SIMARAMA_BASE location.],
[
if test "X$enable_ae2" = "Xno" ; then
	AE2=no
else
	AE2=yes
fi
],
[
AE2=no
])

if test "X$AE2" = "Xyes" ; then
	AC_CHECK_FILE("$SIMARAMA_BASE/libae2rw.so", AET2_SO=yes, AET2_SO=no)

	if test "X$AET2_SO" = "Xyes" ; then
		AET2_CFLAGS="-DAET2_IS_PRESENT -I$SIMARAMA_BASE"
		AET2_LDADD="$SIMARAMA_BASE/libae2rw.so"
	else

		AC_CHECK_FILE("$SIMARAMA_BASE/libae2rw.a", AET2_A=yes, AET2_A=no)
		if test "X$AET2_A" = "Xyes" ; then
			AET2_CFLAGS="-DAET2_IS_PRESENT -I$SIMARAMA_BASE"
			AET2_LDADD="$SIMARAMA_BASE/libae2rw.a"
		else

		AC_MSG_WARN([AET2 library not found, skipping.])

		fi
	fi

	AC_CHECK_FILE("$SIMARAMA_BASE/libaliasdb.so", AAET2_SO=yes, AAET2_SO=no)

	if test "X$AAET2_SO" = "Xyes" ; then
		AC_CHECK_FILE("$SIMARAMA_BASE/aliasdb.h", AAET2_SOY=yes, AAET2_SOY=no)
		if test "X$AAET2_SOY" = "Xyes" ; then
			AET2_CFLAGS="-DAET2_ALIASDB_IS_PRESENT $AET2_CFLAGS"
			AET2_LDADD="$SIMARAMA_BASE/libaliasdb.so $AET2_LDADD"
		else
			AC_MSG_WARN([AET2 alias header not found, skipping.])
		fi
	else

		AC_CHECK_FILE("$SIMARAMA_BASE/libaliasdb.a", AET2_A=yes, AET2_A=no)
		if test "X$AAET2_A" = "Xyes" ; then
			AC_CHECK_FILE("$SIMARAMA_BASE/aliasdb.h", AAET2_SOY=yes, AAET2_SOY=no)
			if test "X$AAET2_SOY" = "Xyes" ; then
				AET2_CFLAGS="-DAET2_ALIASDB_IS_PRESENT $AET2_CFLAGS"
				AET2_LDADD="$SIMARAMA_BASE/libaliasdb.a $AET2_LDADD"
			else
				AC_MSG_WARN([AET2 alias header not found, skipping.])
			fi
		else

		AC_MSG_WARN([AET2 alias library not found, skipping.])

		fi
	fi

	AC_CHECK_FILE("$SIMARAMA_BASE/ae2rw.h", AET2_SOY=yes, AET2_SOY=no)
	if test "X$AET2_SOY" = "Xno" ; then
		AET2_CFLAGS=
		AET2_LDADD=
		AC_MSG_WARN([AET2 header not found, skipping.])
	fi

else
	AC_MSG_RESULT([no])
fi

AC_SUBST(AET2_CFLAGS)
AC_SUBST(AET2_LDADD)


AC_MSG_CHECKING([if FsdbReader support should be used])

FFR_CFLAGS=
FSDB_LDADD=
FFR=no
AC_ARG_ENABLE([fsdb],
[  --enable-fsdb           Compiles in FsdbReader from env vars FSDBREADER_HDRS
                          header and FSDBREADER_LIBS library directories.],
[
if test "X$enable_fsdb" = "Xno" ; then
	FFR=no
else
	FFR=yes
fi
],
[
FFR=no
])

EXTLOAD_CFLAGS=

if test "X$FFR" = "Xyes" ; then
	AC_CHECK_FILE("$FSDBREADER_LIBS/libnffr.so", FSDB_SO=yes, FSDB_SO=no)

	if test "X$FSDB_SO" = "Xyes" ; then
		FSDB_CFLAGS="-DFSDB_IS_PRESENT -I$FSDBREADER_HDRS"
		FSDB_LDADD="$FSDBREADER_LIBS/libnffr.so"
		EXTLOAD_CFLAGS="-DEXTLOAD_SUFFIX=\\\"fsdb\\\""
	else

		AC_CHECK_FILE("$FSDBREADER_LIBS/libnffr.a", FSDB_A=yes, FSDB_A=no)
		if test "X$FSDB_A" = "Xyes" ; then
			FSDB_CFLAGS="-DFSDB_IS_PRESENT -I$FSDBREADER_HDRS"
			FSDB_LDADD="$FSDBREADER_LIBS/libnffr.a"
			EXTLOAD_CFLAGS="-DEXTLOAD_SUFFIX=\\\"fsdb\\\""
		else

		AC_MSG_WARN([FSDB reader library not found, skipping.])

		fi
	fi

	AC_CHECK_FILE("$FSDBREADER_LIBS/libnsys.so", AFSDB_SO=yes, AFSDB_SO=no)

	if test "X$AFSDB_SO" = "Xyes" ; then
		FSDB_CFLAGS="-DFSDB_NSYS_IS_PRESENT $FSDB_CFLAGS"
		FSDB_LDADD="$FSDBREADER_LIBS/libnsys.so $FSDB_LDADD"
		EXTLOAD_CFLAGS="-DEXTLOAD_SUFFIX=\\\"fsdb\\\""
	else

		AC_CHECK_FILE("$FSDBREADER_LIBS/libnsys.a", AFSDB_A=yes, AFSDB_A=no)
		if test "X$AFSDB_A" = "Xyes" ; then
			FSDB_CFLAGS="-DFSDB_NSYS_IS_PRESENT $FSDB_CFLAGS"
			FSDB_LDADD="$FSDB_LDADD $FSDBREADER_LIBS/libnsys.a"
			EXTLOAD_CFLAGS="-DEXTLOAD_SUFFIX=\\\"fsdb\\\""
		else

		FSDB_CFLAGS=
		FSDB_LDADD=
		EXTLOAD_CFLAGS=
		AC_MSG_WARN([FSDB system library not found, skipping.])

		fi
	fi

	AC_CHECK_FILE("$FSDBREADER_HDRS/ffrAPI.h", FSDB_SOY=yes, FSDB_SOY=no)
	if test "X$FSDB_SOY" = "Xno" ; then
		FSDB_CFLAGS=
		FSDB_LDADD=
		EXTLOAD_CFLAGS=
		AC_MSG_WARN([FSDB header not found, skipping.])
	fi

else
	AC_MSG_RESULT([no])
fi

AC_SUBST(FSDB_CFLAGS)
AC_SUBST(FSDB_LDADD)


# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LEX
AC_PROG_RANLIB
AM_PROG_CC_C_O

#
# for "Open Hierarchy Source"
#
AC_PATH_PROG(GEDITTEST, [gedit], [notfound])
if test "$GEDITTEST" = "notfound" ; then
	GEDIT_CFLAGS=	
else
	GEDIT_CFLAGS="-DGEDIT_PATH=\\\"$GEDITTEST\\\""
fi

AC_SUBST(GEDIT_CFLAGS)

#
# only needed if user wishes to process various files with an external reader
#
AC_PATH_PROG(EXTDEBUG, [fsdbdebug], [notfound])
if test "$EXTDEBUG" = "notfound" ; then
	EXTLOAD_CFLAGS="$EXTLOAD_CFLAGS"
else
	EXTLOAD_CFLAGS="-DEXTLOAD_PATH=\\\"$EXTDEBUG\\\" -DEXTLOAD_SUFFIX=\\\"fsdb\\\""
fi

# enables .fsdb as input filetype in vcd2fst, and also gtkwave -o

AC_PATH_PROG(EXTDEBUG2, [fsdb2vcd], [notfound])
if test "$EXTDEBUG2" != "notfound" ; then
	EXTLOAD_CFLAGS="$EXTLOAD_CFLAGS -DEXTCONV_PATH=\\\"$EXTDEBUG2\\\""
fi

# enables .vpd as input filetype in vcd2fst, and also gtkwave -o

AC_PATH_PROG(EXTDEBUG3, [vpd2vcd], [notfound])
if test "$EXTDEBUG3" != "notfound" ; then
	EXTLOAD_CFLAGS="$EXTLOAD_CFLAGS -DEXT2CONV_PATH=\\\"$EXTDEBUG3\\\" -DEXT2LOAD_SUFFIX=\\\"vpd\\\""
fi

# enables .wlf as input filetype in vcd2fst, and also gtkwave -o

AC_PATH_PROG(EXTDEBUG4, [wlf2vcd], [notfound])
if test "$EXTDEBUG4" != "notfound" ; then
	EXTLOAD_CFLAGS="$EXTLOAD_CFLAGS -DEXT3CONV_PATH=\\\"$EXTDEBUG4\\\" -DEXT3LOAD_SUFFIX=\\\"wlf\\\""
fi

AC_SUBST(EXTLOAD_CFLAGS)

#
# gperf only needed if the user updates the gperf data
# files which only developers will be doing...
#
AC_PATH_PROG(GPERF, [gperf], [notfound])
if test "$GPERF" = "notfound" ; then
	AC_MSG_ERROR([Couldn't find a usable gperf program.
Please install gperf which is available from
ftp://ftp.gnu.org/pub/gnu/gperf/
])
fi

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([pthread], [main])

# included or system libz and libbz2.  By default, we search for the
# system libz and libbz2.  If not found, build the included ones.  
# The --enable-local-libz and --enable-local_libbz2 arguments can be
# used to force the use of the included libs.  --disable-local-libz
# and --disable-local-libbz2 can be used to force the use of system
# libs
AC_ARG_ENABLE([local-libz],
[  --enable-local-libz     Use the bundled libz instead of any libz which may
                          already be installed on your system.  Default is to
                          first look for an installed libz and fall back to
                          using the bundled one.],
[
if test "X$enable_local_libz" = "Xno" ; then
	force_system_libz=yes
else
	force_bundled_libz=yes
fi
], , )

need_libz=no
LIBZ_LDADD=
LIBZ_CFLAGS=
if test "X$force_bundled_libz" != "Xyes" ; then
        AC_TRY_COMPILE([#include <zlib.h>], [gzdopen(0,"rb");],, [need_libz=yes])
	if test "$need_libz" = "yes" -a "X$force_system_libz" = "Xyes" ; then
		AC_MSG_ERROR([
You have disabled building the bundled libz but no system libz headers 
could be found.  Either allow building the bundled libz (by not using
--disable-local-libz) or make sure your system has a libz installed])
	fi
	AC_CHECK_LIB([z], [gzread], [LIBZ_LDADD=-lz] , [need_libz=yes])
	if test "$need_libz" = "yes" -a "X$force_system_libz" = "Xyes" ; then
		AC_MSG_ERROR([
You have disabled building the bundled libz but no system libz could
be found.  Either allow building the bundled libz (by not using
--disable-local-libz) or make sure your system has a libz installed])
	fi
else
	need_libz=yes
fi

AC_ARG_ENABLE([local-libbz2],
[  --enable-local-libbz2   Use the bundled libbz2 instead of any libbz2 which may
                          already be installed on your system.  Default is to
                          first look for an installed libbz2 and fall back to
                          using the bundled one.],
[
if test "X$enable_local_libbz2" = "Xno" ; then
	force_system_libbz2=yes
else
	force_bundled_libbz2=yes
fi
], , )

need_libbz2=no
LIBBZ2_LDADD=
LIBBZ2_CFLAGS=
if test "X$force_bundled_libbz2" != "Xyes" ; then
        AC_TRY_COMPILE([#include <bzlib.h>], [BZ2_bzdopen(0,"rb");],, [need_libbz2=yes])
	if test "$need_libbz2" = "yes" -a "X$force_system_libbz2" = "Xyes" ; then
		AC_MSG_ERROR([
You have disabled building the bundled libbz2 but no system libbz2 headers
could be found.  Either allow building the bundled libbz2 (by not using
--disable-local-libbz2) or make sure your system has a libbz2 installed])
	fi
	AC_CHECK_LIB([bz2], [BZ2_bzread], [LIBBZ2_LDADD=-lbz2], [need_libbz2=yes])
	if test "$need_libbz2" = "yes" -a "X$force_system_libbz2" = "Xyes" ; then
		AC_MSG_ERROR([
You have disabled building the bundled libbz2 but no system libbz2 could
be found.  Either allow building the bundled libbz2 (by not using
--disable-local-libbz2) or make sure your system has a libbz2 installed])
	fi
else
	need_libbz2=yes
fi

LIBZ_DIR=
if test "$need_libz" = "yes" ; then
	LIBZ_CFLAGS='-I$(top_srcdir)/src/libz'
	LIBZ_LDADD='$(top_builddir)/src/libz/libz.a'
	LIBZ_DIR=libz
fi
AC_SUBST(LIBZ_CFLAGS)
AC_SUBST(LIBZ_LDADD)
AC_SUBST(LIBZ_DIR)

LIBBZ2_DIR=
if test "$need_libbz2" = "yes" ; then
	LIBBZ2_CFLAGS='-I$(top_srcdir)/src/libbz2'
	LIBBZ2_LDADD='$(top_builddir)/src/libbz2/libbz2.a'
	LIBBZ2_DIR=libbz2
fi
AC_SUBST(LIBBZ2_CFLAGS)
AC_SUBST(LIBBZ2_LDADD)
AC_SUBST(LIBBZ2_DIR)

# ------------- LZMA / XZ -------------------

AC_MSG_CHECKING([if XZ should be enabled])

EXZ=yes
AC_ARG_ENABLE([xz],
[  --disable-xz            Disables LZMA support for VZT],
[
if test "X$enable_xz" = "Xno" ; then
	EXZ=no
fi
])
AC_MSG_RESULT([$EXZ])

LIBXZ_LDADD=
LIBXZ_CFLAGS=
if test "X$EXZ" == "Xyes"; then
	have_liblzma=no
	AC_TRY_COMPILE([#include <lzma.h>], [lzma_end(NULL);],, [have_liblzma=yes])
	AC_CHECK_LIB([lzma], [lzma_end], [LIBLZMA_LDADD=-llzma] , [have_liblzma=yes])
	if test "$have_liblzma" = "yes" ; then
		AC_MSG_ERROR([LZMA support for VZT is enabled, but xz could not be found.
Please install the xz-devel package, see the http://tukaani.org/xz website, or use the --disable-xz flag.])
	fi
	LIBXZ_LDADD='-llzma'
	LIBXZ_CFLAGS='-D_WAVE_HAVE_XZ'
fi
AC_SUBST(LIBXZ_LDADD)
AC_SUBST(LIBXZ_CFLAGS)

# ------------- Fast Tree -------------------

AC_MSG_CHECKING([if Fast SST Tree should be enabled])

FASTTREE=yes
AC_ARG_ENABLE([fasttree],
[  --disable-fasttree      Disables experimental Fast SST Tree widget code],
[
if test "X$enable_fasttree" = "Xno" ; then
	FASTTREE=no
fi
])
AC_MSG_RESULT([$FASTTREE])

FASTTREE_CFLAGS=
if test "X$FASTTREE" == "Xno"; then
	FASTTREE_CFLAGS='-DWAVE_DISABLE_FAST_TREE'
fi
AC_SUBST(FASTTREE_CFLAGS)

# ------------- Judy -------------------

AC_MSG_CHECKING([if Judy array support should be enabled])

EJUDY=no
AC_ARG_ENABLE([judy],
[  --enable-judy           Enables Judy array support],
[
if test "X$enable_judy" = "Xno" ; then
	EJUDY=no
else
	EJUDY=yes
fi
])
AC_MSG_RESULT([$EJUDY])

LIBJUDY_LDADD=
LIBJUDY_CFLAGS=
if test "X$EJUDY" == "Xyes"; then
	have_libjudy=no
	AC_TRY_COMPILE([#include <Judy.h>], [ JudySLIns (NULL, NULL, NULL);],, [have_libjudy=yes])
	AC_CHECK_LIB([Judy], [JudySLIns], [LIBJUDY_LDADD=-lJudy] , [have_libjudy=yes])
	if test "$have_libjudy" = "yes" ; then
		AC_MSG_ERROR([Judy support is enabled, but could not be found.
Please install Judy, see the http://judy.sourceforge.net website or use the --disable-judy flag.])
	fi
	LIBJUDY_LDADD='-lJudy'
	LIBJUDY_CFLAGS='-D_WAVE_HAVE_JUDY'
fi
AC_SUBST(LIBJUDY_LDADD)
AC_SUBST(LIBJUDY_CFLAGS)

# ------------- GTK -------------------
if test "X$GTK1" = "Xyes" ; then
	AC_PATH_PROG(GTK_CONFIG, [gtk-config], [notfound])
	if test "$GTK_CONFIG" = "notfound"; then
		AC_MSG_ERROR([You must have gtk installed on your system])
	else
		AC_MSG_CHECKING([gtk1 version])
		GTK_VER=`$GTK_CONFIG --version`
		AC_MSG_RESULT([$GTK_VER])

		case $GTK_VER in
			1.2.* )
				AC_MSG_CHECKING([for gtk1 CFLAGS])
				GTK_CFLAGS=`$GTK_CONFIG --cflags`
				AC_MSG_RESULT([$GTK_CFLAGS])
				AC_MSG_CHECKING([for gtk1 libs])
				GTK_LIBS=`$GTK_CONFIG --libs`
				AC_MSG_RESULT([$GTK_LIBS])
				;;

			* )
				AC_MSG_ERROR([when building with gtk1, you need version 1.2.*])
				;;
		esac
	fi
else
	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.2.0)
	GTK_VER=`$PKG_CONFIG gtk+-2.0 --modversion`

	_gdk_tgt=`$PKG_CONFIG --variable=target gdk-2.0`
# removed...causes problems with gtk-1.2...
# AM_CONDITIONAL([GDK_TARGET_QUARTZ], [test x$_gdk_tgt = xquartz])
	if test "x$_gdk_tgt" = xquartz; then
	   PKG_CHECK_MODULES(GTK_MAC, gtk-mac-integration >= 2.0.0)
	   AC_SUBST(GTK_MAC_LIBS)
	   AC_SUBST(GTK_MAC_CFLAGS)

	   COCOA_GTK_CFLAGS="-xobjective-c -DWAVE_COCOA_GTK"
	   AC_SUBST(COCOA_GTK_CFLAGS)

	   COCOA_GTK_LDADD="-lobjc"
	   AC_SUBST(COCOA_GTK_LDADD)

	   COCOA_GTK_LDFLAGS="-framework Cocoa -framework ApplicationServices"
	   AC_SUBST(COCOA_GTK_LDFLAGS)
	fi

	if test x$with_gconf = xyes; then
          PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.0)
	  GCONF_CFLAGS="-DWAVE_HAVE_GCONF $GCONF_CFLAGS"
  	  AC_SUBST(GCONF_CFLAGS)
	  AC_SUBST(GCONF_LIBS)
	fi

	if test x$with_gsettings = xyes; then
	  PKG_CHECK_MODULES(GIO, gio-2.0 >= 2.0)
          GLIB_GSETTINGS
          GSETTINGS_CFLAGS="-DWAVE_HAVE_GSETTINGS $GSETTINGS_CFLAGS"
          AC_SUBST(GSETTINGS_CFLAGS)
	fi

	PKG_CHECK_MODULES(GTK_UNIX_PRINT,gtk+-unix-print-2.0, GUP_F="yes", GUP_F="no")
	if test "x$GUP_F" = xyes; then
		GTK_UNIX_PRINT_CFLAGS="${GTK_UNIX_PRINT_CFLAGS} -DWAVE_GTK_UNIX_PRINT"
	fi
        AC_SUBST(GTK_UNIX_PRINT_LIBS)
        AC_SUBST(GTK_UNIX_PRINT_CFLAGS)

fi 

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h inttypes.h libintl.h limits.h malloc.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h wchar.h wctype.h getopt.h])

# TI-RPC needed after glibc 2.26, where SunRPC will go away
AS_IF([test "x$with_tirpc" = xyes],
     [PKG_CHECK_MODULES([TIRPC],
                         [libtirpc],
                         [RPC_CFLAGS=$TIRPC_CFLAGS; RPC_LDADD=$TIRPC_LIBS;],
                         [AC_MSG_ERROR([libtirpc requested, but library not found.])]
                       )],
     [RPC_CFLAGS=""; RPC_LDADD="";
      # rpc workaround for cygwin
      AC_CHECK_HEADERS([rpc/types.h rpc/xdr.h], [], [],
           [[#ifdef HAVE_RPC_TYPES_H
             # include <rpc/types.h>
             #endif
           ]])
      AC_CHECK_LIB(nsl,xdrmem_create)
      AC_CHECK_LIB(rpc,xdrmem_create)
     ]
)
AC_SUBST(RPC_CFLAGS)
AC_SUBST(RPC_LDADD)


# Checks for operand sizes.
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([ptrdiff_t])
AC_SYS_LARGEFILE

# Checks for library functions. (malloc/realloc removed to get rid of rpl_xxx substitutions)
# AC_FUNC_MALLOC
# AC_FUNC_REALLOC
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit btowc bzero dup2 memmove memset munmap pow putenv re_comp realpath regcomp select setenv strcasecmp strchr strdup strerror strncasecmp strrchr strstr getopt_long setenv unsetenv])

# some AIX adds (linker flag for 32 bit compiles)
if test "X$OSTYPE" = "Xaix" ; then
if test "X$MACHTYPE" = "Xrs6000" ; then
	CFLAGS="${CFLAGS} -D_WAVE_BE32"
	if test "X$CC" = "Xxlc" ; then
	LDFLAGS="${LDFLAGS} -bmaxdata:0xd0000000/dsa"
	else
	LDFLAGS="${LDFLAGS} -Wl,-bmaxdata:0xd0000000/dsa"
	fi
fi
fi

CFLAGS="${CFLAGS} ${STRUCT_PACK}"

if test "X$OSTYPE" = "Xmsys" ; then
# add library for common dialog found in file.c
	MINGW_LDADD="-lcomdlg32"
	else
	# there are conflicts with struct timespec in pthreads vs mingw
	CFLAGS="${CFLAGS} -DFST_WRITER_PARALLEL"
	fi

if test "X$ETCL" = "Xyes" ; then
if test "X$OSTYPE" = "Xcygwin" ; then
# skip Tcl_CreateInterp check on cygwin...
	if test "X$STUBIFY" = "Xyes" ; then
	TCL_DEFADD="-DHAVE_LIBTCL -DWAVE_TCL_STUBIFY -DUSE_TCL_STUBS -DUSE_TK_STUBS"
	TCL_LDADD="${TCL_STUB_LIB_SPEC}"
	TK_LDADD="${TK_STUB_LIB_SPEC}"
	else
	TCL_DEFADD="-DHAVE_LIBTCL"
	TCL_LDADD="${TCL_LIB_SPEC}"
	TK_LDADD="${TK_LIB_SPEC}"
        # cygwin tkConfig.sh has issues...
        if test "X$TK_LDADD" = "X" ; then
                TK_LDADD="${TK_BUILD_LIB_SPEC}"
        fi
	fi
else
if test "X$OSTYPE" = "Xdarwin" ; then
# skip Tcl_CreateInterp check on darwin (until we figure out)...
	if test "X$STUBIFY" = "Xyes" ; then
	TCL_DEFADD="-DHAVE_LIBTCL -DWAVE_TCL_STUBIFY -DUSE_TCL_STUBS -DUSE_TK_STUBS"
	TCL_LDADD="${TCL_STUB_LIB_SPEC}"
	TK_LDADD="${TK_STUB_LIB_SPEC}"
	else
	TCL_DEFADD="-DHAVE_LIBTCL"
	TCL_LDADD="${TCL_LIB_SPEC}"
	TK_LDADD="${TK_LIB_SPEC}"
	fi
else
if test "X$OSTYPE" = "Xmsys" ; then
# skip Tcl_CreateInterp check on mingw (until we figure out)...
	if test "X$STUBIFY" = "Xyes" ; then
	TCL_DEFADD="-DHAVE_LIBTCL -DWAVE_TCL_STUBIFY -DUSE_TCL_STUBS -DUSE_TK_STUBS"
	TCL_LDADD="${TCL_STUB_LIB_SPEC}"
	TK_LDADD="${TK_STUB_LIB_SPEC}"
	else
	TCL_DEFADD="-DHAVE_LIBTCL"
	TCL_LDADD="${TCL_LIB_SPEC}"
	TK_LDADD="${TK_LIB_SPEC}"
	fi
else
OLD_LDFLAGS="${LDFLAGS}"
TCLSPEC_LHS="${TCL_LIB_SPEC% *}"
TKLSPEC_LHS="${TK_LIB_SPEC% *}"
LDFLAGS="${LDFLAGS} ${TCLSPEC_LHS} ${TKLSPEC_LHS}"
AC_CHECK_LIB(tcl${TCL_VERSION},Tcl_CreateInterp, [TCL_LDADD=${TCL_LIB_SPEC}], [need_tcl=yes])
if test "$need_tcl" = "yes" ; then
                AC_MSG_WARN([
Tcl support not enabled.])
else
	if test "X$STUBIFY" = "Xyes" ; then
	TCL_DEFADD="-DHAVE_LIBTCL -DWAVE_TCL_STUBIFY -DUSE_TCL_STUBS -DUSE_TK_STUBS"
	TCL_LDADD="${TCL_STUB_LIB_SPEC}"
	TK_LDADD="${TK_STUB_LIB_SPEC}"
	else
	TCL_DEFADD="-DHAVE_LIBTCL"
	TCL_LDADD="${TCL_LIB_SPEC}"
	TK_LDADD="${TK_LIB_SPEC}"
	fi
	fi
LDFLAGS="${OLD_LDFLAGS}"
fi
fi
fi
fi

AC_SUBST(MINGW_LDADD)
AC_SUBST(TCL_LDADD)
AC_SUBST(TCL_DEFADD)
AC_SUBST(TK_LDADD)

AC_CONFIG_FILES([Makefile
		doc/Makefile
		contrib/Makefile
		contrib/rtlbrowse/Makefile
		contrib/xml2stems/Makefile
		contrib/bundle_for_osx/Makefile
		contrib/fst_jni/Makefile
		contrib/wlf2vcd/Makefile
		contrib/fsdb2vcd/Makefile
		examples/Makefile
		man/Makefile
		src/Makefile
		src/cocoa/Makefile
		src/helpers/Makefile
		src/liblzma/Makefile
		src/libbz2/Makefile
		src/libz/Makefile
		src/helpers/fst/Makefile
		share/Makefile
		share/mime/Makefile
		share/mime/packages/Makefile
		share/icons/Makefile
		share/icons/gnome/Makefile
		share/icons/gnome/16x16/Makefile
		share/icons/gnome/16x16/mimetypes/Makefile
		share/icons/gnome/32x32/Makefile
		share/icons/gnome/32x32/mimetypes/Makefile
		share/icons/gnome/48x48/Makefile
		share/icons/gnome/48x48/mimetypes/Makefile
		share/icons/hicolor/Makefile
		share/icons/hicolor/scalable/Makefile
		share/icons/hicolor/scalable/apps/Makefile
		share/applications/Makefile
		share/appdata/Makefile
])


AC_MSG_NOTICE([         
--------------------------------------------
  Configuration summary:

  FASTTREE_CFLAGS       : $FASTTREE_CFLAGS

  gtk1                  : $GTK1
  gtk version           : $GTK_VER
  GTK_CFLAGS            : $GTK_CFLAGS
  GTK_LIBS              : $GTK_LIBS

  LIBZ_CFLAGS           : $LIBZ_CFLAGS
  LIBZ_LDADD            : $LIBZ_LDADD

  LIBBZ2_CFLAGS         : $LIBBZ2_CFLAGS
  LIBBZ2_LDADD          : $LIBBZ2_LDADD

  LIBXZ_CFLAGS          : $LIBXZ_CFLAGS
  LIBXZ_LDADD           : $LIBXZ_LDADD

  LIBJUDY_CFLAGS        : $LIBJUDY_CFLAGS
  LIBJUDY_LDADD         : $LIBJUDY_LDADD

  AET2_CFLAGS           : $AET2_CFLAGS
  AET2_LDADD            : $AET2_LDADD

  FSDB_CFLAGS           : $FSDB_CFLAGS
  FSDB_LDADD            : $FSDB_LDADD

  EXTLOAD_CFLAGS        : $EXTLOAD_CFLAGS

  TCL_INCLUDE_SPEC      : $TCL_INCLUDE_SPEC
  TCL_LIB_SPEC          : $TCL_LIB_SPEC
  TCL_LDADD             : $TCL_LDADD
  TCL_DEFADD            : $TCL_DEFADD  

  TK_INCLUDE_SPEC       : $TK_INCLUDE_SPEC
  TK_LIB_SPEC           : $TK_LIB_SPEC
  TK_LDADD              : $TK_LDADD

  RPC_CFLAGS            : $RPC_CFLAGS
  RPC_LDADD             : $RPC_LDADD

  CPPFLAGS              : $CPPFLAGS
  CFLAGS                : $CFLAGS
  LDFLAGS               : $LDFLAGS
  LIBS                  : $LIBS

  MINGW_LDADD           : $MINGW_LDADD
  GTK_MAC_CFLAGS        : $GTK_MAC_CFLAGS
  GTK_MAC_LIBS          : $GTK_MAC_LIBS
  COCOA_GTK_CFLAGS      : $COCOA_GTK_CFLAGS
  COCOA_GTK_LDADD       : $COCOA_GTK_LDADD
  COCOA_GTK_LDFLAGS     : $COCOA_GTK_LDFLAGS

  GCONF_CFLAGS          : $GCONF_CFLAGS
  GCONF_LIBS            : $GCONF_LIBS
  GSETTINGS_CFLAGS      : $GSETTINGS_CFLAGS

  GTK_UNIX_PRINT_CFLAGS : $GTK_UNIX_PRINT_CFLAGS
  GTK_UNIX_PRINT_LIBS   : $GTK_UNIX_PRINT_LIBS

  GEDIT_CFLAGS          : $GEDIT_CFLAGS
--------------------------------------------
])

AC_OUTPUT

