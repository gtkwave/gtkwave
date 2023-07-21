/*
 * Copyright (c) 2003-2005 Active State Corporation.
 * See the file LICENSE.TXT for information on usage and redistribution
 * and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef WAVE_TCLNP_H
#define WAVE_TCLNP_H

#include <config.h>

#ifdef HAVE_LIBTCL

#include <tcl.h>

/* ==== Np... Begin */

#  define NpPlatformMsg(s1, s2) printf("TCLINIT | Platform: %s\n\t%s\n", s1, s2)
#  define NpLog(x,y) printf("TCLINIT | " x, y)
#  define NpLog3(x,y,z) printf("TCLINIT | " x, y, z)
#  define NpPanic(x) fprintf(stderr, "TCLINIT | "x)

#include <tcl.h>

#if (TCL_MAJOR_VERSION < 8) \
	|| ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION < 4))
#error "Gtkwave requires Tcl 8.4+"
#endif

#ifndef TCL_TSD_INIT
#define TCL_TSD_INIT(keyPtr)	(ThreadSpecificData *)Tcl_GetThreadData((keyPtr), sizeof(ThreadSpecificData))
#endif

#define TCL_OBJ_CMD(cmd)	int (cmd)(ClientData clientData, \
		Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])

#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for getenv */
#endif

#ifdef WIN32

#  include <windows.h>

#  define dlclose(path)		((void *) FreeLibrary((HMODULE) path))
#  define DLSYM(handle, symbol, type, proc) \
	(proc = (type) GetProcAddress((HINSTANCE) handle, symbol))

#  define snprintf _snprintf

#  define HAVE_UNISTD_H 1

#  ifndef	F_OK
#    define	F_OK	0
#  endif

#  ifndef SHLIB_SUFFIX
#    define SHLIB_SUFFIX ".dll"
#  endif

#elif (defined(__MACH__) && defined(__APPLE__)) /* Mac OS X */

#  include <stdio.h>
/* #  include <Carbon/Carbon.h> */ /* Commented out: ajb 25sep11 */

#  if HAVE_UNISTD_H
#	include <sys/types.h>
#	include <unistd.h>
#  endif

#  ifndef SHLIB_SUFFIX
#    define SHLIB_SUFFIX ".dylib"
#  endif

#    include <dlfcn.h>
#  define HMODULE void *
#  define DLSYM(handle, symbol, type, proc) \
	(proc = (type) dlsym(handle, symbol))

#  define HIBYTE(i) (i >> 8)
#  define LOBYTE(i) (i & 0xff)

#else /* UNIX */

#ifdef __CYGWIN__
#    define SHLIB_SUFFIX ".dll"
#endif

#  include <stdio.h>

#  define HIBYTE(i) (i >> 8)
#  define LOBYTE(i) (i & 0xff)

#  if HAVE_UNISTD_H
#	include <sys/types.h>
#	include <unistd.h>
#  endif

#  if (!defined(HAVE_DLADDR) && defined(__hpux))

/* HPUX requires shl_* routines */
#    include <dl.h>
#    define HMODULE shl_t
#    define dlopen(libname, flags)	shl_load(libname, \
	BIND_DEFERRED|BIND_VERBOSE|DYNAMIC_PATH, 0L)
#    define dlclose(path)		shl_unload((shl_t) path)
#    define DLSYM(handle, symbol, type, proc) \
	if (shl_findsym(&handle, symbol, (short) TYPE_PROCEDURE, \
		(void *) &proc) != 0) { proc = NULL; }

#  ifndef SHLIB_SUFFIX
#    define SHLIB_SUFFIX ".sl"
#  endif

#  else

#    include <dlfcn.h>
#    define HMODULE void *
#    define DLSYM(handle, symbol, type, proc) \
	(proc = (type) dlsym(handle, symbol))

#  ifndef SHLIB_SUFFIX
#    define SHLIB_SUFFIX ".so"
#  endif

/*
 * FIX: For other non-dl systems, we need alternatives here
 */

#  endif

/*
 * Shared functions:
 */

#endif /* PLATFORM DEFS */

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

/*
 * Tcl Plugin version identifiers
 * (the 3 strings are computed from the 4 internal numbers)
 */
#define NPTCL_VERSION		PACKAGE_VERSION
#define NPTCL_PATCH_LEVEL	PACKAGE_PATCHLEVEL
#define NPTCL_INTERNAL_VERSION	PACKAGE_PATCHLEVEL

#define NPTCL_MAJOR_VERSION	3
#define NPTCL_MINOR_VERSION	0
#define NPTCL_RELEASE_LEVEL	0
#define NPTCL_RELEASE_SERIAL	1

#ifdef BUILD_nptcl
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_nptcl */

/*
 * Netscape APIs (needs system specific headers)
 * AIX predefines certain types that we must redefine.
 */

#ifdef _AIX
#define _PR_AIX_HAVE_BSD_INT_TYPES 1
#endif

/* #include "npapi.h" */

/*
 * The following constant is used to tell Netscape that the plugin will
 * accept whatever amount of input is available on a stream.
 */

#define MAXINPUTSIZE		0X0FFFFFFF

/*
 * Define the names of token tables used in the plugin:
 */

#define	NPTCL_INSTANCE		"npInstance"
#define NPTCL_STREAM		"npStream"

#ifndef NP_LOG
#define NP_LOG		((char *) NULL)
#endif

/*
 * Procedures shared between various modules in the plugin:
 */

/*
 * npinterp.c
 */
extern Tcl_Interp	*NpCreateMainInterp(char *me, int install_tk);
extern Tcl_Interp	*NpGetMainInterp(void);
extern void		NpDestroyMainInterp(void);
extern Tcl_Interp	*NpGetInstanceInterp(int install_tk);
extern void		NpDestroyInstanceInterp(Tcl_Interp *interp);

/* ==== Np... End */

#endif

#endif

