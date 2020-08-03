#!/usr/bin/env sh

set -e

gtkver="$(basename $(realpath $(pwd)))"
tmpdir="gtkwave_${gtkver}"

bits="$(echo "$MSYSTEM" | tail -c 3)"

if [ -d "$tmpdir" ]; then
  rm -rf "$tmpdir"
fi

mkdir "$tmpdir"

mkdir "$tmpdir"/bin
for item in $(cat ../exe.inc) $(cat ../dll.inc) $(cat ../"$gtkver"dll.inc) $(cat ../"$bits"dll.inc); do
  cp /mingw"$bits"/bin/"$item" "$tmpdir"/bin/
done

mkdir "$tmpdir"/lib
for item in $(cat ../lib.inc); do
  cp -r /mingw"$bits"/lib/"$item" "$tmpdir"/lib/
done

sharedir="gtkwave"

if [ "$gtkver" = "gtk3" ]; then
  sharedir+="-gtk3"
fi

mkdir "$tmpdir"/share
for item in applications icons "$sharedir"; do
  cp -r /mingw"$bits"/share/"$item" "$tmpdir"/share/
done

tar czf ../../"$tmpdir"_mingw"$bits"_standalone.tgz -C "$tmpdir" .
