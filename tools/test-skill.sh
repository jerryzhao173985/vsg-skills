#!/usr/bin/env bash
# Self-test the vsg skill end-to-end against a real, separate Claude process.
#
# Spawns `claude -p "/vsg <prompt>"` with a READ-ONLY tool whitelist (it outputs code,
# writes no files, runs no commands), extracts the generated main.cpp + CMakeLists.txt,
# and compiles them against the installed vsg::vsg. A clean build proves the skill
# produces correct C++ for that prompt; a failure is a skill defect to investigate.
#
#   Usage: tools/test-skill.sh "render a spinning textured cube with a directional light"
#   Env:   VSG_PREFIX=/path   where vsgConfig.cmake lives (default /usr/local)
#
# Why it is safe: the Claude session is whitelisted to read-only tools (Read/Grep/Glob/
# Skill), so it cannot edit files or exec commands. The only build is the cmake step here.
# This is the "spawn a session to test + iterate" loop, kept non-autonomous on purpose.
set -uo pipefail

PROMPT="${1:?usage: tools/test-skill.sh \"<vsg app description>\"}"
here="$(cd "$(dirname "$0")" && pwd)"
skill="$here/../.claude/skills/vsg"
[ -d "$skill" ] || { echo "vsg skill not found at $skill" >&2; exit 2; }
command -v claude >/dev/null || { echo "claude CLI not on PATH" >&2; exit 2; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/.claude/skills"
cp -R "$skill" "$work/.claude/skills/vsg"
find "$work" -type d -name build -exec rm -rf {} + 2>/dev/null || true
cd "$work"

echo "== generating via: claude -p /vsg  (read-only session) =="
claude -p "/vsg ${PROMPT}. Output the COMPLETE main.cpp and CMakeLists.txt (use find_package(vsg) and link vsg::vsg) as TWO separate fenced code blocks labelled main.cpp and CMakeLists.txt. Do not write any files and do not run any commands." \
  --allowedTools "Read,Grep,Glob,Skill,SlashCommand,TodoWrite" > gen.log 2>&1 || true

if ! python3 "$here/extract_blocks.py" gen.log .; then
    echo "SKILL_TEST=FAIL — no usable code blocks generated"
    cp -f gen.log "$here/last-gen.log" 2>/dev/null || true
    echo "(transcript copied to $here/last-gen.log)"
    exit 1
fi

echo "== compiling the skill-generated app against vsg::vsg =="
if cmake -S . -B build -DCMAKE_PREFIX_PATH="${VSG_PREFIX:-/usr/local}" -DCMAKE_BUILD_TYPE=Release >build.log 2>&1 \
   && cmake --build build -j >>build.log 2>&1; then
    echo "SKILL_TEST=PASS — the skill-generated app compiled & linked"
else
    echo "SKILL_TEST=FAIL — compile/link errors (skill defect to investigate):"
    tail -n 30 build.log
    cp -f main.cpp CMakeLists.txt build.log "$here/" 2>/dev/null || true
    echo "(generated sources + build.log copied to $here/ for triage)"
    exit 1
fi
