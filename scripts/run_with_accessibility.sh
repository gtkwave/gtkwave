#!/usr/bin/env bash
#
# run_with_accessibility.sh
#
# Wrapper to run commands with accessibility support.
# - Detects whether a display is available.
# - If headless, starts Xvfb and sets DISPLAY.
# - Ensures a D-Bus session is available (dbus-launch).
# - Starts AT-SPI registry (at-spi2-registryd) when possible.
# - Runs the provided command and performs cleanup afterwards.
#
# Usage:
#   ./run_with_accessibility.sh <command> [args...]
#
# Notes:
# - This script tries to be conservative about killing things it started.
# - It will not attempt to kill processes it didn't create (except the AT-SPI
#   registry if it was started by this script).
#

set -euo pipefail

# Helpers
log()  { printf '%s\n' "$*"; }
warn() { printf 'Warning: %s\n' "$*" >&2; }
err()  { printf 'Error: %s\n' "$*" >&2; }

# Globals for cleanup
XVFB_PID=""
DBUS_STARTED=""
DBUS_SESSION_BUS_PID=""
ATSPI_PID=""

# Default display to use for Xvfb if needed
XVFB_DISPLAY="${DISPLAY:-:99}"

# Timeout when waiting for X server to become responsive (seconds)
XVFB_START_TIMEOUT=5

# Try to detect whether an X server is available on $DISPLAY.
# Returns 0 if available, non-zero otherwise.
_have_x_server() {
    # If DISPLAY is empty/unset, definitely no X server.
    if [ -z "${DISPLAY:-}" ]; then
        return 1
    fi

    # Prefer xdpyinfo if installed
    if command -v xdpyinfo >/dev/null 2>&1; then
        xdpyinfo >/dev/null 2>&1 && return 0 || return 1
    fi

    # Fallback to xset
    if command -v xset >/dev/null 2>&1; then
        xset -q >/dev/null 2>&1 && return 0 || return 1
    fi

    # If neither tool is available, be optimistic and assume DISPLAY works.
    # This is a conservative choice: if DISPLAY is set, and we can't probe it,
    # we avoid starting Xvfb unnecessarily. The caller still may see failures
    # and should use the setup script in the repo for robust environments.
    return 0
}

_start_xvfb() {
    local display="$1"
    # Try to start Xvfb in background. Use -nolisten tcp to avoid opening network socket.
    if ! command -v Xvfb >/dev/null 2>&1; then
        warn "Xvfb not found in PATH; cannot start headless X server. Accessibility tests may fail."
        return 1
    fi

    log "Starting Xvfb on display ${display}..."
    # Launch Xvfb and detach; redirect output to /dev/null.
    Xvfb "${display}" -screen 0 1024x768x24 -nolisten tcp >/dev/null 2>&1 &
    XVFB_PID=$!
    export DISPLAY="${display}"

    # Wait for X to become responsive
    local start_time now
    start_time=$(date +%s)
    while true; do
        if _have_x_server; then
            log "Xvfb appears to be ready on ${display} (PID ${XVFB_PID})."
            return 0
        fi
        now=$(date +%s)
        if [ "$((now - start_time))" -ge "${XVFB_START_TIMEOUT}" ]; then
            warn "Xvfb did not become ready within ${XVFB_START_TIMEOUT}s. Continuing anyway."
            return 0
        fi
        sleep 0.25
    done
}

_start_dbus_if_needed() {
    if [ -n "${DBUS_SESSION_BUS_ADDRESS:-}" ]; then
        log "Using existing D-Bus session: ${DBUS_SESSION_BUS_ADDRESS}"
        return 0
    fi

    if ! command -v dbus-launch >/dev/null 2>&1; then
        warn "dbus-launch not available; AT-SPI / pyatspi may not work without a session bus."
        return 1
    fi

    log "Starting D-Bus session..."
    # Evaluates DBUS_SESSION_BUS_ADDRESS and DBUS_SESSION_BUS_PID into this shell.
    # Use --sh-syntax to make parsing easy. --exit-with-session kept off so our child
    # processes can outlive this shell if necessary; we'll still attempt cleanup.
    eval "$(dbus-launch --sh-syntax)"
    DBUS_STARTED=1
    # dbus-launch usually sets DBUS_SESSION_BUS_PID in the environment; keep it.
    DBUS_SESSION_BUS_PID="${DBUS_SESSION_BUS_PID:-}"
    log "D-Bus session address: ${DBUS_SESSION_BUS_ADDRESS:-<unknown>}"
}

_start_atspi_if_needed() {
    # If pyatspi/AT-SPI is not present or launch fails, we warn but continue.
    # Try common locations/commands for the registry.
    if command -v at-spi2-registryd >/dev/null 2>&1; then
        log "Starting AT-SPI registry (at-spi2-registryd)..."
        at-spi2-registryd >/dev/null 2>&1 &
        ATSPI_PID=$!
        sleep 0.25
        if ! ps -p "${ATSPI_PID}" >/dev/null 2>&1; then
            warn "at-spi2-registryd failed to start (pid ${ATSPI_PID} not running)."
            ATSPI_PID=""
        else
            log "AT-SPI registry started (PID ${ATSPI_PID})."
        fi
        return 0
    fi

    # Try a few known library locations (common in some distros)
    for candidate in \
        "/usr/libexec/at-spi2-registryd" \
        "/usr/lib/at-spi2-core/at-spi2-registryd" \
        "/usr/lib64/at-spi2-core/at-spi2-registryd" \
        ; do
        if [ -x "$candidate" ]; then
            log "Starting AT-SPI registry (${candidate})..."
            "$candidate" >/dev/null 2>&1 &
            ATSPI_PID=$!
            sleep 0.25
            if ps -p "${ATSPI_PID}" >/dev/null 2>&1; then
                log "AT-SPI registry started (PID ${ATSPI_PID})."
                return 0
            else
                warn "AT-SPI registry at ${candidate} failed to start."
                ATSPI_PID=""
            fi
        fi
    done

    warn "Could not find at-spi2-registryd in PATH or common locations. Accessibility may not work."
    return 1
}

# Cleanup function to kill processes we started.
_cleanup() {
    local rc=$?
    log "Cleaning up accessibility environment..."

    if [ -n "${ATSPI_PID:-}" ]; then
        if ps -p "${ATSPI_PID}" >/dev/null 2>&1; then
            log "Killing AT-SPI registry (PID ${ATSPI_PID})..."
            kill "${ATSPI_PID}" 2>/dev/null || true
            sleep 0.1
            if ps -p "${ATSPI_PID}" >/dev/null 2>&1; then
                kill -9 "${ATSPI_PID}" 2>/dev/null || true
            fi
        fi
    fi

    if [ -n "${DBUS_STARTED:-}" ]; then
        if [ -n "${DBUS_SESSION_BUS_PID:-}" ] && ps -p "${DBUS_SESSION_BUS_PID}" >/dev/null 2>&1; then
            log "Killing D-Bus session (PID ${DBUS_SESSION_BUS_PID})..."
            kill "${DBUS_SESSION_BUS_PID}" 2>/dev/null || true
            sleep 0.1
            if ps -p "${DBUS_SESSION_BUS_PID}" >/dev/null 2>&1; then
                kill -9 "${DBUS_SESSION_BUS_PID}" 2>/dev/null || true
            fi
        else
            # Some dbus-launch variants do not set DBUS_SESSION_BUS_PID. Try to unset env.
            unset DBUS_SESSION_BUS_ADDRESS || true
        fi
    fi

    if [ -n "${XVFB_PID:-}" ]; then
        if ps -p "${XVFB_PID}" >/dev/null 2>&1; then
            log "Killing Xvfb (PID ${XVFB_PID})..."
            kill "${XVFB_PID}" 2>/dev/null || true
            sleep 0.1
            if ps -p "${XVFB_PID}" >/dev/null 2>&1; then
                kill -9 "${XVFB_PID}" 2>/dev/null || true
            fi
        fi
    fi

    log "Cleanup complete."
    return $rc
}

# Ensure cleanup runs on exit or signals
trap _cleanup EXIT
trap 'kill 0 2>/dev/null || true' INT TERM

# Main
if [ $# -lt 1 ]; then
    err "No command specified. Usage: $0 <command> [args...]"
    exit 2
fi

# Detect whether we need Xvfb
HEADLESS=0
if ! _have_x_server; then
    HEADLESS=1
fi

if [ "$HEADLESS" -eq 1 ]; then
    log "No usable X server detected on DISPLAY='${DISPLAY:-<unset>}'. Attempting to start Xvfb..."
    if ! _start_xvfb "${XVFB_DISPLAY}"; then
        warn "Failed to start Xvfb; continuing without a display. Accessibility tests are likely to fail."
    fi
else
    log "Found usable X server on DISPLAY='${DISPLAY:-<unset>}'. Running without Xvfb."
fi

# Ensure D-Bus session exists
_start_dbus_if_needed || warn "Proceeding without D-Bus session."

# Ensure AT-SPI registry is running
_start_atspi_if_needed || warn "Proceeding without AT-SPI registry."

# Execute the requested command
log "Running command: $*"
# Run command in the current shell so environment and signals behave naturally.
# We capture the exit code so we can return it after cleanup.
set +e
"$@"
EXIT_CODE=$?
set -e

log "Command exited with code: ${EXIT_CODE}"

# Explicitly exit with the command's exit code (trap will run cleanup)
exit "${EXIT_CODE}"
