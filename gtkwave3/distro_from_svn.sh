#!/bin/sh
echo "Cleaning out SVN directories..."
find . | grep '\.svn' | tac | awk '{print "rm -rf "$0}' | sh

echo "Making distribution tarball from SVN directory..."
cd ../
cat ./gtkwave3/configure.ac | grep AC_INIT | awk '{print $2}' | sed 's/,//' | \
	awk '{print "mv gtkwave3 gtkwave-"$0" ; tar cvf gtkwave-"$0".tar gtkwave-"$0" ; gzip -9 gtkwave-"$0".tar"}' | sh

echo "Done!"
