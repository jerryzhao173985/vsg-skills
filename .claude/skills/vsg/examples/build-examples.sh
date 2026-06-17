#!/usr/bin/env bash
# Build all example programs against the installed vsg::vsg — the compile/link gate
# for references/examples/. A clean build proves every documented example is real.
#
# Output contract (greppable last line):
#   EXAMPLES_RESULT=PASS | FAIL
#
# Usage: examples/build-examples.sh [--clean]
set -uo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
build="$here/build"
prefix="${VSG_PREFIX:-/usr/local}"

[ "${1:-}" = "--clean" ] && rm -rf "$build"

echo "== configuring (CMAKE_PREFIX_PATH=$prefix) =="
if ! cmake -S "$here" -B "$build" -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_BUILD_TYPE=Release; then
    echo "EXAMPLES_RESULT=FAIL"; exit 1
fi

echo "== building hello_vsg, view_model, builder_primitives, custom_geometry =="
if ! cmake --build "$build" -j; then
    echo "EXAMPLES_RESULT=FAIL"; exit 1
fi

echo "EXAMPLES_RESULT=PASS"
