#!/usr/bin/env python3
"""Deeper verification of the vsg skill, beyond "citations resolve":

  A. SEMANTIC CITES  -- for every citation whose doc text clearly names a symbol
     (a backtick'd token or a vsg::X / X::Y just before the cite), confirm that
     symbol actually appears within a few lines of the cited line in the source.
     Catches "cited :67 but the method is at :72" and "cited a line that doesn't
     mention the claimed API". Reports a confirmation RATE + the suspicious cites.
  B. LINKS    -- every internal [text](./x.md) link resolves to a real file.
  C. ORPHANS  -- every references/ file is named somewhere in SKILL.md (so it is
     actually reachable via the routing table), else it is dead weight.

Usage: check-claims.py <skill-dir> <vsg-src-root>
Advisory by design: prints findings; exit 1 only on broken links or orphans
(hard structural defects), not on semantic 'unconfirmed' (which includes prose).
"""
import os, re, sys, glob

SKILL = os.path.abspath(sys.argv[1])
REPO = os.path.abspath(sys.argv[2])
INC = os.path.join(REPO, 'include', 'vsg')
WINDOW = 6
md_files = sorted(glob.glob(os.path.join(SKILL, 'references', '**', '*.md'), recursive=True))

def resolve(path):
    return os.path.join(REPO, path) if path.startswith(('src/', 'include/')) else os.path.join(INC, path)

_cache = {}
def lines_of(fp):
    if fp not in _cache:
        try: _cache[fp] = open(fp, errors='replace').read().splitlines()
        except FileNotFoundError: _cache[fp] = None
    return _cache[fp]

CITE = re.compile(r'([A-Za-z_][\w./-]*\.(?:h|hpp|cpp)):(\d+)(?:-(\d+))?')
STOP = {'the','and','for','with','when','that','this','from','into','each','via','its','not',
        'are','use','used','default','optional','public','field','method','returns','class','via',
        'one','all','any','set','get','add','new','data','list','type','node','base','call','see'}

def candidates(pre):
    """Identifiers the doc names right before a cite: nearest backtick span + vsg::X / X::Y."""
    ids = set()
    spans = re.findall(r'`([^`]+)`', pre)
    if spans:
        for tok in re.findall(r'[A-Za-z_]\w{2,}', spans[-1]):   # nearest backtick token only
            ids.add(tok)
    for m in re.findall(r'vsg::([A-Za-z_]\w+)', pre[-120:]): ids.add(m)
    for a, b in re.findall(r'([A-Za-z_]\w+)::([A-Za-z_]\w+)', pre[-120:]): ids.add(a); ids.add(b)
    return {i for i in ids if i.lower() not in STOP and len(i) >= 3}

# ---- A. semantic citations ----
checked = confirmed = skipped = negative = 0
suspicious = []
NEG = (' no ', ' not ', 'never ', 'without ', 'instead of', 'rather than', 'no `', 'not `', 'there is no')
for p in md_files:
    text = open(p).read()
    for m in CITE.finditer(text):
        path, l1 = m.group(1), int(m.group(2))
        l2 = int(m.group(3)) if m.group(3) else l1
        ls = lines_of(resolve(path))
        if ls is None or l1 > len(ls):
            continue  # resolution is the other gate's job
        line_start = text.rfind('\n', 0, m.start()) + 1
        pre = text[line_start:m.start()]
        cand = candidates(pre)
        if not cand:
            skipped += 1; continue
        checked += 1
        lo, hi = max(0, l1 - 1 - WINDOW), min(len(ls), l2 - 1 + WINDOW + 1)
        window = '\n'.join(ls[lo:hi])
        if any(c in window for c in cand):
            confirmed += 1
        elif any(neg in pre.lower()[-70:] for neg in NEG):
            negative += 1   # "no X" / "not X" -- the symbol is correctly ABSENT at the line
        else:
            suspicious.append((os.path.relpath(p, SKILL), m.group(0), sorted(cand)))

# ---- B. internal links ----
broken = []
LINK = re.compile(r'\]\((\.{0,2}/[^)]+\.md|[A-Za-z0-9_./-]+\.md)\)')
for p in md_files:
    for m in LINK.finditer(open(p).read()):
        target = m.group(1)
        if target.startswith('http'): continue
        full = os.path.normpath(os.path.join(os.path.dirname(p), target))
        if not os.path.exists(full):
            broken.append((os.path.relpath(p, SKILL), target))

# ---- C. orphaned reference files ----
skill_md = open(os.path.join(SKILL, 'SKILL.md')).read()
orphans = []
for p in md_files:
    rel = os.path.relpath(p, SKILL)              # e.g. references/components/core.md
    if rel not in skill_md:
        orphans.append(rel)

print("A. SEMANTIC CITATIONS")
rate = (100.0 * (confirmed + negative) / checked) if checked else 100.0
print(f"   {confirmed} confirmed at line + {negative} negative-claims (symbol correctly absent) = {rate:.1f}% of {checked} symbol-bearing cites accounted for; {len(suspicious)} need manual review; {skipped} prose cites skipped")
for f, c, cand in suspicious:
    print(f"   SUSPECT [{f}] {c}  expected one of {cand} near the line")
print(f"\nB. INTERNAL LINKS: {len(broken)} broken")
for f, t in broken: print(f"   BROKEN [{f}] -> {t}")
print(f"\nC. ORPHANED REFERENCE FILES (not named in SKILL.md): {len(orphans)}")
for o in orphans: print(f"   ORPHAN {o}")

sys.exit(1 if (broken or orphans) else 0)
