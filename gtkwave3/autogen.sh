#! /bin/sh
#
# $Id: autogen.sh,v 1.2 2008/11/23 06:38:55 gtkwave Exp $
#
# Run the various GNU autotools to bootstrap the build
# system.  Should only need to be done once.

# for now avoid using bash as not everyone has that installed
CONFIG_SHELL=/bin/sh
export CONFIG_SHELL

echo "Running aclocal..."
aclocal $ACLOCAL_FLAGS || exit 1
echo "Done with aclocal"

echo "Running autoheader..."
autoheader || exit 1
echo "Done with autoheader"

echo "Running automake..."
automake -a -c --foreign || exit 1
echo "Done with automake"

echo "builtin(include,./tcl.m4)" >>aclocal.m4

echo "Running autoconf..."
autoconf || exit 1
echo "Done with autoconf"

echo "You must now run configure"

echo "All done with autogen.sh"

