#!/bin/sh
set -eu

PORT="${1:-8765}"
GTKWAVE_BIN="${2:-./build/src/gtkwave}"
WAVE_FILE="${3:-./examples/des.fst}"

if [ ! -x "$GTKWAVE_BIN" ]; then
    echo "error: gtkwave binary not found: $GTKWAVE_BIN" >&2
    exit 1
fi

if [ ! -f "$WAVE_FILE" ]; then
    echo "error: waveform file not found: $WAVE_FILE" >&2
    exit 1
fi

if ! command -v nc >/dev/null 2>&1; then
    echo "error: nc is required" >&2
    exit 1
fi

tmpdir="$(mktemp -d)"
cleanup() {
    if [ -n "${GTKWAVE_PID:-}" ]; then
        kill "$GTKWAVE_PID" >/dev/null 2>&1 || true
    fi
    rm -rf "$tmpdir"
}
trap cleanup EXIT

"$GTKWAVE_BIN" --wcp-port="$PORT" "$WAVE_FILE" >"$tmpdir/gtkwave.log" 2>&1 &
GTKWAVE_PID=$!

greeted=0
i=0
while [ "$i" -lt 50 ]; do
    if printf '' | nc -w 1 localhost "$PORT" >"$tmpdir/greet" 2>/dev/null; then
        if grep -q '"type":"greeting"' "$tmpdir/greet"; then
            greeted=1
            break
        fi
    fi
    i=$((i + 1))
    sleep 0.1
done
if [ "$greeted" -ne 1 ]; then
    echo "error: WCP server did not respond with greeting" >&2
    exit 1
fi

( printf '%s\n' \
    '{"type":"command","command":"add_items","items":["des"],"recursive":true}' \
    '{"type":"command","command":"get_item_list"}'; \
    sleep 1 ) | nc -N localhost "$PORT" >"$tmpdir/out1"

if ! grep -q '"command":"add_items"' "$tmpdir/out1"; then
    echo "error: add_items response missing" >&2
    exit 1
fi

item_id="$(grep '"command":"get_item_list"' "$tmpdir/out1" | tail -n 1 | \
    sed -n 's/.*"ids":\\[\\([0-9][0-9]*\\).*/\\1/p')"
if [ -z "$item_id" ]; then
    echo "error: get_item_list did not return any ids" >&2
    exit 1
fi

( printf '%s\n' \
    "{\"type\":\"command\",\"command\":\"get_item_info\",\"ids\":[${item_id}]}" \
    "{\"type\":\"command\",\"command\":\"set_item_color\",\"id\":${item_id},\"color\":\"red\"}"; \
    sleep 1 ) | nc -N localhost "$PORT" >"$tmpdir/out2"

if ! grep -q '"command":"get_item_info"' "$tmpdir/out2"; then
    echo "error: get_item_info response missing" >&2
    exit 1
fi
if ! grep -q '"command":"ack"' "$tmpdir/out2"; then
    echo "error: set_item_color ack missing" >&2
    exit 1
fi

( printf '%s\n' \
    '{"type":"command","command":"add_markers","markers":[{"time":100,"name":"A","move_focus":true}]}' \
    '{"type":"command","command":"set_viewport_to","timestamp":100}' \
    '{"type":"command","command":"set_viewport_range","start":0,"end":1000}' \
    '{"type":"command","command":"zoom_to_fit","viewport_idx":0}'; \
    sleep 1 ) | nc -N localhost "$PORT" >"$tmpdir/out3"

if ! grep -q '"command":"add_markers"' "$tmpdir/out3"; then
    echo "error: add_markers response missing" >&2
    exit 1
fi
if ! grep -q '"command":"ack"' "$tmpdir/out3"; then
    echo "error: viewport/zoom ack missing" >&2
    exit 1
fi

( printf '%s\n' \
    '{"type":"command","command":"reload"}'; \
    sleep 2 ) | nc -N localhost "$PORT" >"$tmpdir/out4"

if ! grep -q '"event":"waveforms_loaded"' "$tmpdir/out4"; then
    echo "error: waveforms_loaded event missing after reload" >&2
    exit 1
fi

printf '%s\n' '{"type":"command","command":"shutdown"}' | nc -N localhost "$PORT" >"$tmpdir/out5"

echo "WCP smoke test: OK"
