#!/usr/bin/env python3
"""Read-only citation resolver for the `vsg` skill.

Scans every references/**/*.md for 'path:line' citation tokens and resolves each
against the repo sources: the file must exist and the line must be in range.
HARD FAIL on missing file or out-of-range line. Exit 0 iff all resolve.

Usage: check-citations.py <skill-dir> <repo-root>
Citations are '<module>/<File>.h:line' (relative to include/vsg) or
'src/vsg/<module>/<File>.cpp:line' (repo-relative).
"""
import os, re, sys, glob

SKILL = sys.argv[1]
REPO = os.path.abspath(sys.argv[2])
INC = os.path.join(REPO, 'include', 'vsg')

TOK = re.compile(r'(?<![\w/.])([A-Za-z_][\w./-]*\.(?:h|hpp|cpp)):(\d+)(?:-\d+)?')

def resolve(path):
    if path.startswith(('src/', 'include/')):
        return os.path.join(REPO, path)
    return os.path.join(INC, path)

cache = {}
def nlines(fp):
    if fp not in cache:
        try:
            cache[fp] = len(open(fp, errors='replace').read().splitlines())
        except FileNotFoundError:
            cache[fp] = None
    return cache[fp]

md_files = sorted(glob.glob(os.path.join(SKILL, 'references', '**', '*.md'), recursive=True))
total = ok = hard = 0
hard_list = []
for p in md_files:
    for m in TOK.finditer(open(p).read()):
        path, l1 = m.group(1), int(m.group(2))
        total += 1
        n = nlines(resolve(path))
        if n is None:
            hard += 1; hard_list.append((os.path.relpath(p, SKILL), m.group(0), 'file not found'))
        elif l1 < 1 or l1 > n:
            hard += 1; hard_list.append((os.path.relpath(p, SKILL), m.group(0), f'line {l1} > {n}'))
        else:
            ok += 1

print(f"CITATIONS: {ok}/{total} resolved, {hard} hard failures")
for f, c, why in hard_list:
    print(f"  FAIL [{f}] {c} -- {why}")
sys.exit(1 if hard else 0)
