#!/bin/sh

DIR="$(cd "$(dirname "$0")" && pwd)"

meson setup "$DIR/build" \
    -Dbuild_macos_app=true \
    -Dset_rpath=enabled \
    -Dintrospection=false \
    --prefix=/Applications/GTKWave.app \
    --datadir=/Applications/GTKWave.app/Contents/Resources/share \
    --bindir=Contents/Resources/bin --libdir=Contents/Resources/lib

meson install -C "$DIR/build"