#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FRAMEWORK_DIR="$SCRIPT_DIR/icgv-volumetric-ray-tracer"
TIMESTAMP=$(date +"%Y%m%d-%H%M%S")
OUTPUT="$SCRIPT_DIR/icgv-submission-$TIMESTAMP.zip"

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

(
    cd "$FRAMEWORK_DIR"
    zip -r -X "$OUTPUT" CMakeLists.txt src \
        -x "*.DS_Store" \
        -x "*/.DS_Store" \
        -x "._*" \
        -x "*/._*"
)

echo "Created $OUTPUT"
echo "Archive contents:"
unzip -l "$OUTPUT"
