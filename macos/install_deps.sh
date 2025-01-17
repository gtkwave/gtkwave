#!/bin/sh

CONTENTS="$MESON_INSTALL_PREFIX/Contents"
RESOURCES="$CONTENTS/Resources"
LIBS="$RESOURCES/lib"
BIN="$RESOURCES/bin"

mkdir -p "$LIBS"

EXECUTABLES=("$BIN/gtkwave" "$BIN/twinwave")

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
"

for exe in $EXECUTABLES; do
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

        if [ -n "$pkg_path" ]; then
            echo "Copying $lib from /opt/homebrew/opt/$pkg_path/lib/$lib to $LIBS"
            cp "/opt/homebrew/opt/$pkg_path/lib/$lib" "$LIBS"

            echo "Changing $lib path in $exe"
            install_name_tool -change \
            "/opt/homebrew/opt/$pkg_path/lib/$lib" \
            "@executable_path/../lib/$lib" \
            "$exe"
        fi
    done

    install_name_tool -add_rpath "@executable_path/../lib" "$exe"
done

chmod +x "$BIN/gtkwave"
chmod +x "$BIN/twinwave"
chmod +x "$LIBS"/*.dylib