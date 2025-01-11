#!/bin/sh

DIR="$(cd "$(dirname "$0")" && pwd)"
CONTENTS="$(dirname "$DIR")"
RESOURCES="$CONTENTS/Resources"

exec "$RESOURCES/bin/gtkwave" "$@"