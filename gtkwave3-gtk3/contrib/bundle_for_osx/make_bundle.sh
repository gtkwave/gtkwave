#!/bin/sh

if [ -f ./gtkwave.pdf ]
then
echo Skipping download of gtkwave.pdf
else
echo Downloading gtkwave.pdf...
curl http://gtkwave.sourceforge.net/gtkwave.pdf >gtkwave.pdf
fi

gcc -o ../../examples/transaction ../../examples/transaction.c -DHAVE_INTTYPES_H

#
# this assumes that gtkwave --prefix is the
# same directory hierarchy that gtkwave via
# jhbuild is installed to
#
# make sure you are in jhbuild-shell and
# that jhbuild is in your path
#

# ~/.local/bin/ige-mac-bundler gtkwave.bundle
~/.local/bin/gtk-mac-bundler gtkwave.bundle

#
# this is a bit of a hack as it seems ige-mac-bundler
# should already do this...
#
gdk-pixbuf-query-loaders | \
sed 's#/Users/.*loaders/#@executable_path/../Resources/lib/gdk-pixbuf-2.0/loaders/#' \
> ~/Desktop/gtkwave.app/Contents/Resources/etc/gtk-2.0/gdk-pixbuf.loaders


#
# code that might need to be added to the installer
#

# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>vcd</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>evcd</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>lxt</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>lx2</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>vzt</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>ghw</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# defaults write com.apple.LaunchServices LSHandlers -array-add "<dict><key>LSHandlerContentTag</key> <string>fst</string><key>LSHandlerContentTagClass</key> <string>public.filename-extension</string><key>LSHandlerRoleAll</key> <string>com.geda.gtkwave</string></dict>"
# /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister -kill -domain local -domain system -domain user
