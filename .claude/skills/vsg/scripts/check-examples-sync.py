#!/usr/bin/env python3
"""Assert example docs do not drift from their compile-verified source.

For each references/examples/<name>.md, every C++ line shown in its ```cpp blocks
(comments stripped, whitespace normalized) must appear in the twin examples/<name>.cpp
(the compile-verified source of truth). This lets the doc show an EXCERPT, but forbids
it from teaching code the built program does not contain.

Usage: check-examples-sync.py <skill-dir>
"""
import os, re, sys

SKILL = sys.argv[1]
EX_MD = os.path.join(SKILL, 'references', 'examples')
EX_CPP = os.path.join(SKILL, 'examples')

def norm(line):
    line = re.sub(r'//.*$', '', line)        # strip line comment
    line = re.sub(r'\s+', ' ', line).strip() # collapse whitespace
    return line

def cpp_lines(path):
    out = set()
    for ln in open(path, errors='replace').read().splitlines():
        n = norm(ln)
        if n:
            out.add(n)
    return out

def md_code_lines(path):
    text = open(path, errors='replace').read()
    blocks = re.findall(r'```cpp\n(.*?)```', text, re.DOTALL)
    lines = []
    for b in blocks:
        for ln in b.splitlines():
            n = norm(ln)
            if n and n not in ('{', '}', '});'):  # trivial punctuation matches anything; skip noise
                lines.append(n)
    return lines

total = misses = 0
miss_report = []
for md in sorted(f for f in os.listdir(EX_MD) if f.endswith('.md') and f != 'index.md'):
    name = md[:-3].replace('-', '_')
    cpp = os.path.join(EX_CPP, name + '.cpp')
    if not os.path.exists(cpp):
        miss_report.append((md, '(no twin .cpp: ' + name + '.cpp)'))
        misses += 1
        continue
    cset = cpp_lines(cpp)
    for line in md_code_lines(os.path.join(EX_MD, md)):
        total += 1
        if line not in cset:
            misses += 1
            miss_report.append((md, line))

print(f"EXAMPLE-SYNC: checked {total} doc code lines across example .md files; {misses} drift(s)")
for md, line in miss_report:
    print(f"  DRIFT [{md}] not in twin .cpp: {line}")
sys.exit(1 if misses else 0)
