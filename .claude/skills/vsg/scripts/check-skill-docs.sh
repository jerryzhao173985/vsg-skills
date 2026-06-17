#!/usr/bin/env bash
# Post-emit consistency check for the `vsg` skill — the audit gate.
# Adapts the meta-skill's check-skill-docs.sh to a native C++ library:
#   1 ROUTING        every references/ file named in SKILL.md exists
#   2 HEADINGS       every component/foundation file ships its required sections
#   3 CITATIONS      every (path:line) cite resolves to a real repo source line
#   4 VERIFY         tally of [VERIFY] markers left in references/
#   5 PROBE          (with --probe) the compile-probe builds against vsg::vsg
#
# Usage: scripts/check-skill-docs.sh [--probe]
#   VSG_SRC=/path/to/VulkanSceneGraph   override the VSG source tree the citations resolve against
set -uo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
skill="$(cd "$here/.." && pwd)"

# Resolve the VSG source tree (authoritative headers/impl the file:line citations reference):
#   1) $VSG_SRC if set; 2) nested checkout (skill lives inside a VSG repo); 3) sibling VulkanSceneGraph checkout.
if [ -n "${VSG_SRC:-}" ]; then repo="$VSG_SRC"
elif [ -d "$here/../../../../include/vsg" ]; then repo="$(cd "$here/../../../.." && pwd)"
elif [ -d "$here/../../../../../VulkanSceneGraph/include/vsg" ]; then repo="$(cd "$here/../../../../../VulkanSceneGraph" && pwd)"
else echo "ERROR: VSG source not found. Set VSG_SRC=/path/to/VulkanSceneGraph (the headers the citations reference)."; exit 2; fi
refs="$skill/references"
fail=0

echo "== 1 ROUTING: SKILL.md rows resolve to real files =="
# pull references/...(.md) tokens out of the routing table / slate
grep -oE 'references/[A-Za-z0-9_./-]+\.md' "$skill/SKILL.md" | sort -u | while read -r rel; do
    if [ -f "$skill/$rel" ]; then echo "  ok   $rel"; else echo "  MISS $rel"; fi
done
if grep -oE 'references/[A-Za-z0-9_./-]+\.md' "$skill/SKILL.md" | sort -u | while read -r rel; do [ -f "$skill/$rel" ] || echo x; done | grep -q x; then
    echo "  ROUTING: FAIL"; fail=1
else echo "  ROUTING: PASS"; fi

echo ""
echo "== 2 HEADINGS: component + foundation files ship required sections =="
comp_req=("## Public includes" "## Key classes" "## Idioms" "## Common mistakes" "## Things to never invent" "## Source references")
found_req=("## The model" "## Key types & calls" "## Idiomatic wiring" "## Rules" "## Common mistakes" "## Source references")
for f in "$refs"/components/*.md; do
    for h in "${comp_req[@]}"; do
        grep -qF "$h" "$f" || { echo "  MISS [$(basename "$f")] '$h'"; fail=1; }
    done
done
for f in "$refs"/foundations/*.md; do
    for h in "${found_req[@]}"; do
        grep -qF "$h" "$f" || { echo "  MISS [$(basename "$f")] '$h'"; fail=1; }
    done
done
[ "$fail" -eq 0 ] && echo "  HEADINGS: PASS" || echo "  HEADINGS: FAIL"

echo ""
echo "== 3 CITATIONS: resolve every (path:line) against repo sources =="
if python3 "$here/check-citations.py" "$skill" "$repo"; then echo "  CITATIONS: PASS"; else echo "  CITATIONS: FAIL"; fail=1; fi

echo ""
echo "== 4 VERIFY: unresolved [VERIFY] markers in references/ =="
vcount=$(grep -rl '\[VERIFY\]' "$refs" 2>/dev/null | wc -l | tr -d ' ')
vtotal=$(grep -rohE '\[VERIFY\]' "$refs" 2>/dev/null | wc -l | tr -d ' ')
echo "  [VERIFY] markers: $vtotal (in $vcount files)"
grep -rn '\[VERIFY\]' "$refs" 2>/dev/null | sed 's/^/  /'
echo "  (markers are advisory, not a failure; resolve or accept each)"

echo ""
if [ "${1:-}" = "--probe" ]; then
    echo "== 5 PROBE: compile documented API surface against vsg::vsg =="
    if bash "$skill/probe/run-probe.sh" 2>&1 | tail -3; then echo "  PROBE: see PROBE_RESULT above"; else echo "  PROBE: FAIL"; fail=1; fi
    echo ""
    echo "== 6 EXAMPLES: compile + link the complete example programs =="
    if bash "$skill/examples/build-examples.sh" 2>&1 | tail -3; then echo "  EXAMPLES: see EXAMPLES_RESULT above"; else echo "  EXAMPLES: FAIL"; fail=1; fi
    echo ""
fi

if [ "$fail" -eq 0 ]; then echo "CHECK_SKILL_DOCS=PASS"; exit 0; else echo "CHECK_SKILL_DOCS=FAIL"; exit 1; fi
