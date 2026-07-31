#!/usr/bin/env bash
# Compose the README gallery image from the scibar_gallery example output.
# Requires: cmake, ImageMagick (montage).
# Usage: ./dev_tools/make_gallery.sh

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
GALLERY_DIR="$BUILD_DIR/examples/gallery"
OUTPUT="$PROJECT_DIR/web/gallery.png"

echo "==> Building scibar_gallery..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" > /dev/null
cmake --build "$BUILD_DIR" --target scibar_gallery -j "$(nproc)" 2>&1 | tail -1

echo "==> Running scibar_gallery..."
mkdir -p "$GALLERY_DIR"
cd "$GALLERY_DIR"
./scibar_gallery

echo "==> Composing 3×3 grid with ImageMagick montage..."
montage gallery_0{0,1,2,3,4,5,6,7,8}.ppm \
    -tile 3x3 \
    -geometry 600x120+6+6 \
    -background white \
    "$OUTPUT"

echo "==> Done: $OUTPUT"
