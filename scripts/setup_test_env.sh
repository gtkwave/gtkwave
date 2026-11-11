#!/bin/bash
#
# GTKWave Test Environment Setup
# 
# This script sets up the complete environment needed to run GTKWave tests
# that use accessibility features (pyatspi) in a headless Xvfb environment.
#

set -e

# Function to start a service and wait for it
start_service() {
    local service_name=$1
    local service_cmd=$2
    local max_wait=${3:-10}
    
    echo "Starting $service_name..."
    $service_cmd &
    local pid=$!
    
    # Wait for service to be responsive
    local count=0
    while [ $count -lt $max_wait ]; do
        if ps -p $pid > /dev/null 2>&1; then
            sleep 0.5
            count=$((count + 1))
        else
            echo "ERROR: $service_name failed to start"
            return 1
        fi
    done
    
    echo "$service_name started with PID $pid"
    echo $pid
}

# Ensure DISPLAY is set
export DISPLAY=${DISPLAY:-:99}
echo "Using DISPLAY: $DISPLAY"

# Check if Xvfb is running (skip if xdpyinfo not available)
if command -v xdpyinfo >/dev/null 2>&1; then
    if ! xdpyinfo -display $DISPLAY >/dev/null 2>&1; then
        echo "ERROR: X server not found on $DISPLAY"
        echo "Start Xvfb with: Xvfb $DISPLAY -screen 0 1024x768x24 &"
        exit 1
    fi
else
    echo "Note: xdpyinfo not available, skipping X server check"
fi

# Set up D-Bus session
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    echo "No D-Bus session found, starting one..."
    # Create a temporary directory for dbus
    DBUS_DIR=$(mktemp -d)
    dbus-daemon --session --fork --print-address=3 --print-pid=4 \
        3> "$DBUS_DIR/bus_address" 4> "$DBUS_DIR/bus_pid"
    
    export DBUS_SESSION_BUS_ADDRESS=$(cat "$DBUS_DIR/bus_address")
    DBUS_PID=$(cat "$DBUS_DIR/bus_pid")
    
    # Cleanup function
    cleanup_dbus() {
        if [ -n "$DBUS_PID" ]; then
            kill $DBUS_PID 2>/dev/null || true
        fi
        rm -rf "$DBUS_DIR" 2>/dev/null || true
    }
    trap cleanup_dbus EXIT
else
    echo "Existing D-Bus session found: $DBUS_SESSION_BUS_ADDRESS"
    # Check if it's accessible
    if ! dbus-send --session --print-reply --dest=org.freedesktop.DBus \
         /org/freedesktop/DBus org.freedesktop.DBus.ListNames >/dev/null 2>&1; then
        echo "WARNING: Cannot connect to existing D-Bus session, starting a new one..."
        unset DBUS_SESSION_BUS_ADDRESS
        
        # Create a temporary directory for dbus
        DBUS_DIR=$(mktemp -d)
        dbus-daemon --session --fork --print-address=3 --print-pid=4 \
            3> "$DBUS_DIR/bus_address" 4> "$DBUS_DIR/bus_pid"
        
        export DBUS_SESSION_BUS_ADDRESS=$(cat "$DBUS_DIR/bus_address")
        DBUS_PID=$(cat "$DBUS_DIR/bus_pid")
        
        # Cleanup function
        cleanup_dbus() {
            if [ -n "$DBUS_PID" ]; then
                kill $DBUS_PID 2>/dev/null || true
            fi
            rm -rf "$DBUS_DIR" 2>/dev/null || true
        }
        trap cleanup_dbus EXIT
    fi
fi

# Start AT-SPI bus launcher
echo "Starting AT-SPI bus launcher..."
/usr/libexec/at-spi-bus-launcher --launch-immediately &
ATSPI_LAUNCHER_PID=$!
sleep 2

# Verify AT-SPI launcher is running
if ! ps -p $ATSPI_LAUNCHER_PID > /dev/null 2>&1; then
    echo "WARNING: AT-SPI bus launcher failed to start or exited"
    echo "Accessibility features may not work properly"
fi

# Start AT-SPI registry daemon
echo "Starting AT-SPI registry..."
/usr/libexec/at-spi2-registryd &
ATSPI_REG_PID=$!
sleep 1

# Verify AT-SPI registry is running
if ps -p $ATSPI_REG_PID > /dev/null 2>&1; then
    echo "AT-SPI registry started with PID $ATSPI_REG_PID"
else
    echo "WARNING: AT-SPI registry failed to start or exited"
fi

# Print environment for debugging
echo ""
echo "Environment configured:"
echo "  DISPLAY=$DISPLAY"
echo "  DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS"
echo ""

# If command arguments provided, run them
if [ $# -gt 0 ]; then
    echo "Executing: $@"
    "$@"
    EXIT_CODE=$?
    
    # Cleanup
    kill $ATSPI_REG_PID 2>/dev/null || true
    kill $ATSPI_LAUNCHER_PID 2>/dev/null || true
    
    exit $EXIT_CODE
else
    echo "Environment ready. Run your commands now."
    echo "Press Ctrl+C to exit and clean up."
    
    # Wait for interrupt
    wait
fi
