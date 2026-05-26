#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FRAMEWORK_DIR="$SCRIPT_DIR/icgv-volumetric-ray-tracer"
OUTPUT="$SCRIPT_DIR/icgv-submission.tar.gz"

if [ ! -d "$FRAMEWORK_DIR" ]; then
    echo "Error: framework directory not found: $FRAMEWORK_DIR" >&2
    exit 1
fi

if [ ! -f "$FRAMEWORK_DIR/CMakeLists.txt" ]; then
    echo "Error: missing CMakeLists.txt in $FRAMEWORK_DIR" >&2
    exit 1
fi

if [ ! -d "$FRAMEWORK_DIR/src" ]; then
    echo "Error: missing src directory in $FRAMEWORK_DIR" >&2
    exit 1
fi

rm -f "$OUTPUT"

tar -czf "$OUTPUT" -C "$FRAMEWORK_DIR" CMakeLists.txt src

echo "Created $OUTPUT"
echo "Archive contents:"
tar -tzf "$OUTPUT"
