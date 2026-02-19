#!/bin/sh

# GTKWave launcher script.

DIR="$(cd "$(dirname "$0")" && pwd)"
CONTENTS="$(dirname "$DIR")"
RESOURCES="$CONTENTS/Resources"

export DYLD_LIBRARY_PATH="$RESOURCES/lib:${DYLD_LIBRARY_PATH:-}"

# TODO: Find a cleaner solution to avoid the warnings.
# For now we filter out these warnings and empty lines that
# tend to appear between them.

exec "$RESOURCES/bin/gtkwave" "$@" 2>&1 | grep -v -E '^$|GdkPixbuf-WARNING.*Error loading XPM image loader'