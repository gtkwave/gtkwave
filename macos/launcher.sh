#!/bin/sh

DIR="$(cd "$(dirname "$0")" && pwd)"
CONTENTS="$(dirname "$DIR")"
RESOURCES="$CONTENTS/Resources"

DYLD_LIBRARY_PATH="$RESOURCES/lib:${DYLD_LIBRARY_PATH:-}" exec "$RESOURCES/bin/gtkwave" "$@"