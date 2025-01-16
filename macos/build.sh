#!/bin/sh

DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="~/Applications"

meson setup "$DIR/build" \
    -Dbuild_macos_app=true \
    -Dset_rpath=enabled \
    -Dintrospection=false \
    --prefix=$APP_DIR/GTKWave.app \
    --datadir=$APP_DIR/GTKWave.app/Contents/Resources/share \
    --bindir=Contents/Resources/bin --libdir=Contents/Resources/lib \
    --includedir=Contents/Resources/include \
    --mandir=Contents/Resources/share/man

meson install -C "$DIR/build"
