#!/usr/bin/env python3
"""Extract the generated main.cpp + CMakeLists.txt from a `claude -p /vsg` transcript.
Picks blocks by content (not position), so extra prose/insight blocks are ignored.
Usage: extract_blocks.py <gen.log> <out-dir>"""
import re, sys

log = open(sys.argv[1], errors='replace').read()
outdir = sys.argv[2]
blocks = re.findall(r'```[a-zA-Z+]*\n(.*?)```', log, re.DOTALL)

cpp = next((b for b in blocks if '#include' in b or 'int main' in b), None)
cml = next((b for b in blocks if 'find_package' in b or 'cmake_minimum' in b), None)
if not cpp or not cml:
    sys.stderr.write(f"could not find both code blocks (cpp={cpp is not None}, cmake={cml is not None})\n")
    sys.exit(1)

open(outdir + '/main.cpp', 'w').write(cpp)
open(outdir + '/CMakeLists.txt', 'w').write(cml)
print(f"extracted main.cpp ({len(cpp)}b), CMakeLists.txt ({len(cml)}b)")
