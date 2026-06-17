#!/usr/bin/env bash
# Compile-probe runner — the validation gate for the `vsg` skill.
# Builds probe.cpp against the installed vsg::vsg. A clean build proves the API
# surface documented in references/ is real (signatures + linkage), not invented.
#
# Output contract (greppable last line):
#   PROBE_RESULT=PASS   build succeeded — documented API surface compiles & links
#   PROBE_RESULT=FAIL   build failed — see the compiler output above
#
# Usage: probe/run-probe.sh [--clean]
set -uo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
build="$here/build"
prefix="${VSG_PREFIX:-/usr/local}"   # where vsgConfig.cmake lives (override with VSG_PREFIX)

[ "${1:-}" = "--clean" ] && rm -rf "$build"

echo "== configuring (CMAKE_PREFIX_PATH=$prefix) =="
if ! cmake -S "$here" -B "$build" -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_BUILD_TYPE=Release; then
    echo "PROBE_RESULT=FAIL"
    exit 1
fi

echo "== building =="
if ! cmake --build "$build" -j; then
    echo "PROBE_RESULT=FAIL"
    exit 1
fi

echo "PROBE_RESULT=PASS"
