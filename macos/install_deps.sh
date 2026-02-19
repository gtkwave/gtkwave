#!/bin/sh

CONTENTS="$MESON_INSTALL_PREFIX/Contents"
RESOURCES="$CONTENTS/Resources"
LIBS="$RESOURCES/lib"
BIN="$RESOURCES/bin"

mkdir -p "$LIBS"

LIBS_PAIRS="
libatk-1.0.0.dylib=at-spi2-core
libepoxy.0.dylib=libepoxy
libglib-2.0.0.dylib=glib
libgtk-3.0.dylib=gtk+3
libgdk-3.0.dylib=gtk+3
libpangocairo-1.0.0.dylib=pango
libpango-1.0.0.dylib=pango
libcairo.2.dylib=cairo
libgdk_pixbuf-2.0.0.dylib=gdk-pixbuf
libgio-2.0.0.dylib=glib
libgobject-2.0.0.dylib=glib
libgtkmacintegration-gtk3.4.dylib=gtk-mac-integration
libgtk-4.1.dylib=gtk4
libjson-glib-1.0.0.dylib=json-glib
libintl.8.dylib=gettext
libpcre2-8.0.dylib=pcre2
libgmodule-2.0.0.dylib=glib
librsvg-2.2.dylib=librsvg
"

# dylib list needs to be copied to lib folder
DYLIBS="
libatk-1.0.0.dylib
libepoxy.0.dylib
libglib-2.0.0.dylib
libgtk-3.0.dylib
libgdk-3.0.dylib
libpangocairo-1.0.0.dylib
libpango-1.0.0.dylib
libcairo.2.dylib
libgdk_pixbuf-2.0.0.dylib
libgio-2.0.0.dylib
libgobject-2.0.0.dylib
libgtkmacintegration-gtk3.4.dylib
libgtk-4.1.dylib
libjson-glib-1.0.0.dylib
libintl.8.dylib
libpcre2-8.0.dylib
libgmodule-2.0.0.dylib
librsvg-2.2.dylib
"

change_lib_path() {
    # $1: lib name, $2: pkg name, $3: binary path
    # Example: change_lib_path "libgtk-3.0.dylib" "gtk+3" "$BIN/gtkwave"
    local lib="$1"
    local pkg_path="$2"
    echo "Changing $lib path in $3"
    install_name_tool -change \
    "/opt/homebrew/opt/$pkg_path/lib/$lib" \
    "@executable_path/../lib/$lib" \
    "$3"
}

for lib in $DYLIBS; do
    # found pkg name for lib
    pkg_path=""
    for pair in $LIBS_PAIRS; do
        key="${pair%%=*}"
        val="${pair#*=}"
        if [ "$key" = "$lib" ]; then
            pkg_path="$val"
            break
        fi
    done

    # copy lib to lib folder, change lib path in binaries
    if [ -n "$pkg_path" ]; then
        echo "Copying $lib from /opt/homebrew/opt/$pkg_path/lib/$lib to $LIBS"
        cp "/opt/homebrew/opt/$pkg_path/lib/$lib" "$LIBS"

        echo "Changing $lib path in $BIN/gtkwave"
        change_lib_path "$lib" "$pkg_path" "$BIN/gtkwave"
    fi
done

install_name_tool -add_rpath "@executable_path/../lib" "$BIN/gtkwave"

# Only change lib paths in twinwave, avoid extra copy of dylibs
change_lib_path "libgtk-3.0.dylib" "gtk+3" "$BIN/twinwave"
change_lib_path "libgobject-2.0.0.dylib" "glib" "$BIN/twinwave"

install_name_tool -add_rpath "@executable_path/../lib" "$BIN/twinwave"

chmod +x "$BIN/gtkwave"
chmod +x "$BIN/twinwave"
chmod +x "$LIBS"/*.dylib
