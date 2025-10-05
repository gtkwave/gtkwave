# GTKWave Accessibility Testing

This directory contains scripts for automated testing of GTKWave's accessibility features using pyatspi.

## Prerequisites

### System Packages
```bash
# On Ubuntu/Debian:
sudo apt-get install -y \
    python3-pyatspi \
    python3-gi \
    xvfb \
    wmctrl \
    xdotool \
    at-spi2-core \
    dbus-x11
```

### Python Packages
```bash
# Install pyvcd for the SYSTEM Python (NOT the 'vcd' package which is for video)
# Use the system Python (/usr/bin/python3) to ensure pyatspi compatibility
sudo /usr/bin/python3 -m pip install pyvcd python-xlib

# Or if you want to install for another Python, make sure it also has pyatspi
pip install pyvcd python-xlib
```

**Important:** The test scripts use `/usr/bin/python3` explicitly because pyatspi 
is only available through the system package manager (python3-pyatspi), not PyPI.

## Running Tests

### Method 1: Using the Setup Script (Recommended)

The `setup_test_env.sh` script handles all the AT-SPI and D-Bus configuration automatically:

```bash
# Start Xvfb (if not already running)
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99

# Set up PATH for gtkwave binaries
export PATH="$(pwd)/builddir/src/helpers:$(pwd)/builddir/src:$PATH"

# Run tests with proper environment
./scripts/setup_test_env.sh bash -c '
    /usr/bin/python3 scripts/awesome.py &
    AWESOME_PID=$!
    sleep 2
    /usr/bin/python3 scripts/stream_to_gtkwave.py
    kill $AWESOME_PID 2>/dev/null || true
'
```

### Method 2: Using Meson DevEnv

```bash
# Compile first
meson compile -C builddir

# Start Xvfb
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99

# Run with setup script
./scripts/setup_test_env.sh bash -c '
    # Set up meson environment
    source <(meson devenv -C builddir --dump)
    
    # Run tests (using /usr/bin/python3 for pyatspi compatibility)
    /usr/bin/python3 scripts/awesome.py &
    AWESOME_PID=$!
    sleep 2
    /usr/bin/python3 scripts/stream_to_gtkwave.py
    kill $AWESOME_PID 2>/dev/null || true
'
```

## Script Descriptions

### setup_test_env.sh
Sets up the complete environment needed for accessibility testing:
- Creates a D-Bus session for the current user
- Starts AT-SPI bus launcher and registry
- Handles cleanup of spawned processes

This is necessary because:
1. pyatspi requires a working AT-SPI infrastructure
2. AT-SPI requires a D-Bus session that the current user can access
3. In headless environments (Xvfb), this infrastructure isn't automatically available

### awesome.py
Automated UI testing script that:
- Connects to GTKWave via pyatspi
- Clicks on the 'mysim' hierarchy node to expand it
- Double-clicks on the 'sine_wave' signal to add it to the waveform view
- Clicks "Highlight All" menu item
- Clicks "Toggle Group Open|Close" menu item

### stream_to_gtkwave.py
Streams VCD waveform data to GTKWave in real-time:
- Launches shmidcat and gtkwave processes
- Uses pyvcd to generate a sine wave signal
- Streams data through the pipeline
- Verifies the GTKWave window appears

## Common Issues

### "Couldn't connect to accessibility bus"
This means AT-SPI is not properly set up. Use `setup_test_env.sh` to handle the setup automatically.

### "Permission denied" on D-Bus socket
This means the D-Bus session belongs to a different user. The setup script will detect this and create a new session for the current user.

### "WARNING: Cannot expand vector"
This is a known limitation when trying to expand vectors that use vlist storage (streaming data). It's a warning, not an error - the tests should continue to run.

## Troubleshooting

Enable debug output:
```bash
export G_MESSAGES_DEBUG=all
./scripts/setup_test_env.sh python3 scripts/awesome.py
```

Check AT-SPI is working:
```bash
./scripts/setup_test_env.sh /usr/bin/python3 -c "
import pyatspi
desktop = pyatspi.Registry.getDesktop(0)
print(f'Desktop: {desktop}')
print(f'Child count: {desktop.childCount}')
"
```
