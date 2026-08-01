#!/usr/bin/env bash
# Compose the README vertical gallery image from the scibar_gallery_vertical
# example output.
# Requires: cmake, ImageMagick (montage).
# Usage: ./dev_tools/make_gallery_vertical.sh

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
GALLERY_DIR="$BUILD_DIR/examples/gallery_vertical"
OUTPUT="$PROJECT_DIR/web/gallery_vertical.png"

echo "==> Building gallery_vertical..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" > /dev/null
cmake --build "$BUILD_DIR" --target gallery_vertical -j "$(nproc)" 2>&1 | tail -1

echo "==> Running gallery_vertical..."
mkdir -p "$GALLERY_DIR"
cd "$GALLERY_DIR"
./gallery_vertical

echo "==> Composing 1×9 strip with ImageMagick montage..."
montage gallery_v_0{0,1,2,3,4,5,6,7,8}.ppm \
    -tile 9x1 \
    -geometry 120x600+6+6 \
    -background white \
    "$OUTPUT"

echo "==> Done: $OUTPUT"
