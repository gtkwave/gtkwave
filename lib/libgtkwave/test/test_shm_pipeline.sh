#!/bin/bash

# Exit immediately if any command fails
set -e

# Meson provides these environment variables
# MESON_BUILD_ROOT is the path to the build directory (e.g., /path/to/gtkwave/builddir)
# MESON_SOURCE_ROOT is the path to the source directory (e.g., /path/to/gtkwave)

# Meson provides these environment variables
MESON_BUILD_ROOT="${MESON_BUILD_ROOT:-$(pwd)}"
MESON_SOURCE_ROOT="${MESON_SOURCE_ROOT:-$(pwd)/..}"
 
# Ensure test binaries can find the built dlls on Windows by adding the
# build tree library paths to PATH when running under MSYS/MinGW.
export PATH="${MESON_BUILD_ROOT}/lib/libgtkwave/src:${MESON_BUILD_ROOT}/subprojects/libfst/src:${MESON_BUILD_ROOT}/src/helpers:${PATH}"
 
PRODUCER="$MESON_BUILD_ROOT/src/helpers/shmidcat"
CONSUMER="$MESON_BUILD_ROOT/lib/libgtkwave/test/test-gw-shared-memory-consumer"
INPUT_FILE="$MESON_SOURCE_ROOT/lib/libgtkwave/test/files/basic.vcd"
OUTPUT_FILE="$MESON_BUILD_ROOT/lib/libgtkwave/test/consumed_output.vcd"

# Ensure executables exist
if [ ! -x "$PRODUCER" ]; then
    echo "Error: Producer '$PRODUCER' not found or not executable."
    exit 1
fi
if [ ! -x "$CONSUMER" ]; then
    echo "Error: Consumer '$CONSUMER' not found or not executable."
    exit 1
fi

echo "--- Running SHM Pipeline Test ---"

# The pipeline:
# 1. cat sends the input file to shmidcat's stdin.
# 2. shmidcat creates SHM, prints the ID to its stdout, and feeds the data into the SHM segment.
# 3. The consumer reads the SHM ID from its stdin, attaches, and consumes the data, printing to its stdout.
# 4. The consumer's stdout is redirected to our output file.
cat "$INPUT_FILE" | "$PRODUCER" | "$CONSUMER" > "$OUTPUT_FILE"

echo "--- Pipeline finished. Verifying output... ---"

# Verification: Compare the original input with the final output.
# If they are identical, diff will exit with 0 (success).
# If they differ, diff will exit with a non-zero status, and `set -e` will cause the script to fail.
if ! diff "$INPUT_FILE" "$OUTPUT_FILE"; then
    echo "ERROR: Output does not match input!"
    exit 1
fi

echo "--- Verification successful! ---"

# Cleanup
rm -f "$OUTPUT_FILE"

# Exit with success
exit 0
