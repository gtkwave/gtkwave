#!/usr/bin/env sh

set -e

cd $(dirname "$0")

bits="$(echo "$MSYSTEM" | tail -c 3)"

if [ -d gtkwave_gtk3 ]; then
  rm -rf gtkwave_gtk3
fi

mkdir gtkwave_gtk3

mkdir gtkwave_gtk3/bin
for item in $(cat exe.inc) $(cat dll.inc) $(cat "$bits"dll.inc); do
  cp /mingw"$bits"/bin/"$item" gtkwave_gtk3/bin/
done

mkdir gtkwave_gtk3/lib
for item in $(cat lib.inc); do
  cp -vr /mingw"$bits"/lib/"$item" gtkwave_gtk3/lib/
done

mkdir gtkwave_gtk3/share
for item in $(cat share.inc); do
  cp -vr /mingw"$bits"/share/"$item" gtkwave_gtk3/share/
done

tar czf ../../../gtkwave_gtk3_mingw"$bits"_standalone.tgz -C gtkwave_gtk3 .
