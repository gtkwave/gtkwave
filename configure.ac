#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gtkwave-gtk3],[3.4.0],[bybell@rocketmail.com])
AC_CONFIG_SRCDIR([src/vcd.c])
AM_INIT_AUTOMAKE([1.14 subdir-objects])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

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

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([pthread], [main])

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
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <lzma.h>]], [[lzma_end(NULL);]])],[],[have_liblzma=yes])
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
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <Judy.h>]], [[ JudySLIns (NULL, NULL, NULL);]])],[],[have_libjudy=yes])
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

AC_PATH_PROG(PKG_CONFIG, [pkg-config], [notfound])
	PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.24.0)
	GTK_VER=`$PKG_CONFIG gtk+-3.0 --modversion`

	# gdk-3.0 may have multiple targets e.g. "broadway quartz"
	_gdk_tgts=`$PKG_CONFIG --variable=targets gdk-3.0`
	for _gdk_tgt in $_gdk_tgts;
	do
	if test "x$_gdk_tgt" = xquartz; then
		PKG_CHECK_MODULES(GTK_MAC, gtk-mac-integration >= 3.0.0)
		AC_SUBST(GTK_MAC_LIBS)
		AC_SUBST(GTK_MAC_CFLAGS)

		COCOA_GTK_CFLAGS="-xobjective-c -DWAVE_COCOA_GTK"
		AC_SUBST(COCOA_GTK_CFLAGS)

		COCOA_GTK_LDADD="-lobjc"
		AC_SUBST(COCOA_GTK_LDADD)

		COCOA_GTK_LDFLAGS="-framework Cocoa -framework ApplicationServices"
		AC_SUBST(COCOA_GTK_LDFLAGS)
	fi
	done

	if test x$with_gconf = xyes; then
		PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.0)
		GCONF_CFLAGS="-DWAVE_HAVE_GCONF $GCONF_CFLAGS"
		AC_SUBST(GCONF_CFLAGS)
		AC_SUBST(GCONF_LIBS)
	fi

	PKG_CHECK_MODULES(GTK_UNIX_PRINT,gtk+-unix-print-3.0, GUP_F="yes", GUP_F="no")
	if test "x$GUP_F" = xyes; then
			GTK_UNIX_PRINT_CFLAGS="${GTK_UNIX_PRINT_CFLAGS} -DWAVE_GTK_UNIX_PRINT"
	fi
AC_SUBST(GTK_UNIX_PRINT_LIBS)
	AC_SUBST(GTK_UNIX_PRINT_CFLAGS)


# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

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

  gtk3                  : $GTK3
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

