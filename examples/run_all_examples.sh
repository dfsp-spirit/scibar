#!/usr/bin/env bash
#
# run_all_examples.sh — Build and run all scibar example applications.
#
# Usage: ./run_all_examples.sh
#
# Each example is built in its own build/ subdirectory, then executed.
# Exit code 0 = success, non-zero = failure.
# A summary of pass/fail counts is printed at the end.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ---------------------------------------------------------------------------
# Example definitions:  name  dir                       executable
# ---------------------------------------------------------------------------
declare -A EXAMPLES=(
    ["linear_horizontal_viridis"]="linear_horizontal_viridis:viridis_horizontal_example"
    ["linear_vertical_viridis"]="linear_vertical_viridis:viridis_example"
    ["diverging_horizontal_vik"]="diverging_horizontal_vik:diverging_horizontal_example"
    ["diverging_vertical_vik"]="diverging_vertical_vik:diverging_vertical_example"
    ["logarithmic_vertical_viridis"]="logarithmic_vertical_viridis:logarithmic_vertical_example"
    ["logarithmic_horizontal_viridis"]="logarithmic_horizontal_viridis:logarithmic_horizontal_example"
    ["categorical_vertical"]="categorical_vertical:categorical_vertical_example"
    ["categorical_horizontal"]="categorical_horizontal:categorical_horizontal_example"
    # Reversed variants
    ["linear_horizontal_viridis_reversed"]="linear_horizontal_viridis_reversed:viridis_horizontal_reversed_example"
    ["linear_vertical_viridis_reversed"]="linear_vertical_viridis_reversed:viridis_vertical_reversed_example"
    ["diverging_horizontal_vik_reversed"]="diverging_horizontal_vik_reversed:vik_horizontal_reversed_example"
    ["diverging_vertical_vik_reversed"]="diverging_vertical_vik_reversed:vik_vertical_reversed_example"
    ["logarithmic_vertical_viridis_reversed"]="logarithmic_vertical_viridis_reversed:log_vertical_reversed_example"
    ["logarithmic_horizontal_viridis_reversed"]="logarithmic_horizontal_viridis_reversed:log_horizontal_reversed_example"
    ["categorical_vertical_reversed"]="categorical_vertical_reversed:categorical_vertical_reversed_example"
    ["categorical_horizontal_reversed"]="categorical_horizontal_reversed:categorical_horizontal_reversed_example"
)

SUCCESSES=0
FAILURES=0
FAILED_LIST=()

echo "=============================================="
echo " Building and running all scibar examples"
echo "=============================================="
echo ""

# ---------------------------------------------------------------------------
# Build and run each example
# ---------------------------------------------------------------------------
for NAME in "${!EXAMPLES[@]}"; do
    IFS=":" read -r DIR EXE <<< "${EXAMPLES[$NAME]}"
    BUILD_DIR="${SCRIPT_DIR}/${DIR}/build"

    echo "--- ${NAME} ---"

    # --- Build ---
    echo "  Configuring..."
    mkdir -p "$BUILD_DIR"
    if ! cmake -S "${SCRIPT_DIR}/${DIR}" -B "$BUILD_DIR" > "${BUILD_DIR}/cmake.log" 2>&1; then
        echo "  [FAIL] cmake configure failed (see ${BUILD_DIR}/cmake.log)"
        FAILURES=$((FAILURES + 1))
        FAILED_LIST+=("${NAME} (cmake)")
        echo ""
        continue
    fi

    echo "  Building..."
    if ! cmake --build "$BUILD_DIR" > "${BUILD_DIR}/build.log" 2>&1; then
        echo "  [FAIL] build failed (see ${BUILD_DIR}/build.log)"
        FAILURES=$((FAILURES + 1))
        FAILED_LIST+=("${NAME} (build)")
        echo ""
        continue
    fi

    # --- Run ---
    echo "  Running..."
    if ( cd "$BUILD_DIR" && ./"$EXE" ) > "${BUILD_DIR}/run.log" 2>&1; then
        echo "  [PASS]"
        SUCCESSES=$((SUCCESSES + 1))
    else
        echo "  [FAIL] runtime error (see ${BUILD_DIR}/run.log)"
        FAILURES=$((FAILURES + 1))
        FAILED_LIST+=("${NAME} (run)")
    fi
    echo ""

done

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "=============================================="
echo " Summary"
echo "=============================================="
echo "  Passed : ${SUCCESSES}"
echo "  Failed : ${FAILURES}"
echo "  Total  : $((SUCCESSES + FAILURES))"
echo ""

if [ ${#FAILED_LIST[@]} -gt 0 ]; then
    echo "Failures:"
    for ITEM in "${FAILED_LIST[@]}"; do
        echo "  - ${ITEM}"
    done
    echo ""
fi

if [ "$FAILURES" -eq 0 ]; then
    echo "All examples passed."
    exit 0
else
    echo "Some examples failed."
    exit 1
fi
