#!/usr/bin/env bash
#
# Demonstration script showing how to run the commands from the issue
# with proper accessibility support. This version uses combinedscript.py
# which integrates both VCD streaming and GUI automation in a single script.
#
# If `run_with_accessibility.sh` is not present, it falls back to the
# older `setup_test_env.sh` usage.
#

set -euo pipefail

cd "$(dirname "$0")/.."

echo "====================================================="
echo "GTKWave Accessibility Test - Wrapped Demo Workflow"
echo "====================================================="
echo ""

# Try to ensure a working X display is available. Do NOT install packages.
# If the current $DISPLAY is not reachable and Xvfb is available, start it on :99.
XVFB_PID=""
cleanup_xvfb() {
    if [ -n "${XVFB_PID:-}" ]; then
        echo "Shutting down Xvfb (PID: ${XVFB_PID})..."
        kill "${XVFB_PID}" 2>/dev/null || true
        wait "${XVFB_PID}" 2>/dev/null || true
    fi
}
trap cleanup_xvfb EXIT

start_xvfb_if_needed() {
    # If DISPLAY is set and xdpyinfo can talk to it, we're good.
    if [ -n "${DISPLAY:-}" ]; then
        if command -v xdpyinfo >/dev/null 2>&1; then
            if xdpyinfo >/dev/null 2>&1; then
                echo "Existing X display (${DISPLAY}) is reachable."
                return 0
            else
                echo "Existing X display (${DISPLAY}) not reachable."
            fi
        else
            echo "xdpyinfo not found; cannot verify existing DISPLAY (${DISPLAY})."
        fi
    else
        echo "No DISPLAY set."
    fi

    # Try to start Xvfb on :99 if available (do not attempt to install).
    if command -v Xvfb >/dev/null 2>&1; then
        echo "Attempting to start Xvfb on :99 (no installation will be performed)..."
        # Start Xvfb detached, log to a temporary file
        Xvfb :99 -screen 0 1024x768x24 >/tmp/demo_xvfb.log 2>&1 &
        XVFB_PID=$!
        export DISPLAY=:99
        # Give it a moment to come up
        sleep 0.5
        if command -v xdpyinfo >/dev/null 2>&1; then
            if xdpyinfo >/dev/null 2>&1; then
                echo "Xvfb started and is responding on ${DISPLAY} (PID: ${XVFB_PID})."
                return 0
            else
                echo "Warning: Xvfb started (PID: ${XVFB_PID}) but xdpyinfo cannot connect yet."
            fi
        else
            echo "Xvfb started (PID: ${XVFB_PID}); xdpyinfo not available to verify."
            return 0
        fi
    else
        echo "Xvfb not available on PATH; continuing without starting an X server."
    fi

    return 1
}

# Step 1: Compile
echo "Step 1: Compiling GTKWave..."
meson compile -C builddir
echo "✓ Compilation complete"
echo ""

# Helper to run the demo under a provided launcher (either run_with_accessibility.sh
# or setup_test_env.sh). The launcher is expected to accept a command to run.
_run_demo_via_launcher() {
    local launcher="$1"

    if [ ! -x "$launcher" ]; then
        echo "ERROR: Launcher not found or not executable: $launcher"
        return 2
    fi

    # Feed the demo steps to the launcher by piping a small shell script.
    # The launcher will run `bash -s` (read script from stdin) so the inner script
    # executes in the prepared accessibility environment.
    cat <<'INNER' | "$launcher" bash -s --
# Inside accessibility-prepared environment (DISPLAY, DBUS, AT-SPI should be available)
set -eo pipefail

echo "Loading meson devenv..."
# meson devenv prints shell assignments; use process substitution to source them.
# If meson devenv is not present, this will fail, which is acceptable here.
# Note: We don't use 'set -u' here because meson devenv output may reference
# variables that aren't set yet (e.g., appending to GI_TYPELIB_PATH).
if command -v meson >/dev/null 2>&1; then
    # shellcheck disable=SC1091
    source <(meson devenv -C builddir --dump)
else
    echo "Warning: meson not found in PATH; proceeding without meson devenv."
fi

echo "Environment ready:"
echo "  - DISPLAY=${DISPLAY:-<unset>}"
echo "  - DBUS_SESSION_BUS_ADDRESS=${DBUS_SESSION_BUS_ADDRESS:-<unset>}"
echo ""

# Run combinedscript.py which integrates both VCD streaming and GUI automation
echo "Running combinedscript.py (integrated VCD streaming and GUI automation)..."
export AUTOMATED_TEST=1
/usr/bin/python3 scripts/combinedscript.py
EXIT_CODE=$?

echo ""
if [ "${EXIT_CODE:-0}" -eq 0 ]; then
    echo "====================================================="
    echo "✅ All tests passed successfully!"
    echo "====================================================="
else
    echo "====================================================="
    echo "❌ Tests failed with exit code: ${EXIT_CODE}"
    echo "====================================================="
fi

exit "${EXIT_CODE}"
INNER
}

# Prefer using the older setup_test_env.sh which properly sets up AT-SPI
LAUNCHER="./scripts/setup_test_env.sh"
# Ensure we have an X server available if possible (but don't install anything)
start_xvfb_if_needed || true
if [ -x "${LAUNCHER}" ]; then
    echo "Using ${LAUNCHER} to provide AT-SPI infrastructure..."
    _run_demo_via_launcher "${LAUNCHER}"
    EXIT_CODE=$?
elif [ -x "./scripts/run_with_accessibility.sh" ]; then
    # Fallback to run_with_accessibility.sh if setup_test_env.sh is not available
    echo "Using ./scripts/run_with_accessibility.sh (note: AT-SPI may not work properly)..."
    _run_demo_via_launcher "./scripts/run_with_accessibility.sh"
    EXIT_CODE=$?
else
    echo "Notice: ${LAUNCHER} not found. Falling back to setup_test_env.sh (older behavior)."
    # Use the older setup script which performs its own DBus/AT-SPI setup.
    if [ -x "./scripts/setup_test_env.sh" ]; then
        ./scripts/setup_test_env.sh bash -c '
            echo "Loading meson devenv..."
            if command -v meson >/dev/null 2>&1; then
                source <(meson devenv -C builddir --dump)
            fi

            echo "Running combinedscript.py (integrated VCD streaming and GUI automation)..."
            export AUTOMATED_TEST=1
            /usr/bin/python3 scripts/combinedscript.py
            EXIT_CODE=$?

            exit $EXIT_CODE
        '
        EXIT_CODE=$?
    else
        echo "ERROR: No launcher available (neither ${LAUNCHER} nor ./scripts/setup_test_env.sh found)."
        EXIT_CODE=3
    fi
fi

echo ""
echo "Done! (exit code: ${EXIT_CODE})"
exit "${EXIT_CODE}"
