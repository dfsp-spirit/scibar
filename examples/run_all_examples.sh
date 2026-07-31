#!/usr/bin/env bash
#
# run_all_examples.sh — Build and run all scibar example applications.
#
# Executable names match directory names, so this script auto-discovers them.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

SUCCESSES=0
FAILURES=0
FAILED_LIST=()

echo "=============================================="
echo " Building and running all scibar examples"
echo "=============================================="
echo ""

for dir in */; do
    dir="${dir%/}"
    [ "$dir" = "build" ] && continue
    [ ! -f "$dir/CMakeLists.txt" ] && continue

    echo "--- $dir ---"

    mkdir -p "$dir/build"
    if ! cmake -B "$dir/build" -S "$dir" -DCMAKE_BUILD_TYPE=Release > "$dir/build/cmake.log" 2>&1; then
        echo "  FAIL: cmake configure (see $dir/build/cmake.log)"
        FAILURES=$((FAILURES + 1))
        FAILED_LIST+=("$dir (configure)")
        continue
    fi

    if ! cmake --build "$dir/build" --config Release -j"$(nproc)" > "$dir/build/build.log" 2>&1; then
        echo "  FAIL: build (see $dir/build/build.log)"
        FAILURES=$((FAILURES + 1))
        FAILED_LIST+=("$dir (build)")
        continue
    fi

    cd "$dir/build"
    if ./"$dir" > /dev/null 2>&1; then
        echo "  PASS: $dir"
        SUCCESSES=$((SUCCESSES + 1))
    else
        echo "  FAIL: $dir runtime"
        FAILURES=$((FAILURES + 1))
        FAILED_LIST+=("$dir (runtime)")
    fi
    cd "$SCRIPT_DIR"
    echo ""
done

echo "=============================================="
echo " Results: $SUCCESSES passed, $FAILURES failed"
echo "=============================================="

if [ "$FAILURES" -gt 0 ]; then
    echo ""
    echo "Failures:"
    for f in "${FAILED_LIST[@]}"; do
        echo "  - $f"
    done
    exit 1
fi
