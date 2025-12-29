#!/bin/sh
set -e

if [ -z "${WAYLAND_DISPLAY:-}" ] && [ -z "${XDG_RUNTIME_DIR:-}" ]; then
  exit 77
fi

if [ -n "${XDG_RUNTIME_DIR:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
  if [ ! -S "${XDG_RUNTIME_DIR}/wayland-0" ]; then
    exit 77
  fi
fi

if ! command -v timeout >/dev/null 2>&1; then
  exit 77
fi

build_root="${MESON_BUILD_ROOT:-$(pwd)}"
bin="${build_root}/src/gtkwave"
if [ ! -x "$bin" ]; then
  echo "gtkwave binary not found at $bin" >&2
  exit 1
fi

out="$(GDK_BACKEND=wayland timeout 3s "$bin" -X 1 2>&1 || true)"

printf "%s\n" "$out" | grep -F "Ignoring -X; GtkPlug only works on X11." >/dev/null 2>&1
if printf "%s\n" "$out" | grep -F "GtkPlug only works under X11" >/dev/null 2>&1; then
  exit 1
fi

exit 0
