## -*- makefile -*-
##

noinst_LIBRARIES=       libgtkwmacintegration.a

libgtkwmacintegration_a_CFLAGS = $(COCOA_GTK_CFLAGS) $(GTK_CFLAGS) $(GTK_MAC_CFLAGS)
libgtkwmacintegration_a_OBJCFLAGS = 

libgtkwmacintegration_a_SOURCES = cocoa_misc.c cocoa_misc.h

# I'm listing these here instead of in SOURCES because we don't directly
# compile them.  Rather it is #include'd by cocoa_misc.c

EXTRA_DIST= alert_sheet.m alert_sheet.h

