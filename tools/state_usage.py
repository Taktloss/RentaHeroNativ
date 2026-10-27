#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Zugriffe des Spielcodes auf den Zustandsblock (CGame+0x78 -> CGameState) zählen.

Liest lokale Ghidra-Dekompilate (re/private/decomp-game/*.c, nicht im Repo) und die
Namensliste docs/symbols/names.tsv; schreibt eine kompakte Übersicht als TSV.

  python3 -I tools/state_usage.py <decomp-dir> <names.tsv> <out.tsv>
"""
import collections
import glob
import os
import re
import sys

# *(T *)(*(int *)((int)this + 0x78) + 0x3ec8)  bzw. param_1/iVarN als Basis
NUM = r'(0x[0-9a-f]+|\d+)'
BASE = r'\*\(int \*\)\(\(int\)\w+ \+ 0x78\)'
DIRECT = re.compile(r'\*\((?:int|uint|undefined4) \*\)\((' + BASE + r'|\w+) \+ ' + NUM + r'\)'
                    r'(\s*=\s*([^;=][^;]*))?')
ALIAS = re.compile(r'(\w+) = ' + BASE + ';')
REASSIGN = re.compile(r'\s*(\w+) = ')
DYN = re.compile(BASE + r' \+ [a-zA-Z_]')
OTHER78 = re.compile(r'\+ 0x78\)')


def loc_of(addr, ctors):
    best = None
    for a, n in ctors:
        if a <= addr:
            best = n
    return best or '?'


def main(ddir, names, out):
    ctors = sorted((int(l.split('\t')[0], 16), l.split('\t')[1][:5])
                   for l in open(names) if '_ctor\t' in l and l.startswith('00') and 'Loc' in l)
    use = collections.defaultdict(lambda: {'r': 0, 'w': 0, 'vals': collections.Counter(), 'locs': set()})
    unmatched = 0
    dyn = []
    for p in sorted(glob.glob(os.path.join(ddir, '*.c'))):
        addr = int(os.path.basename(p)[:8], 16)
        loc = loc_of(addr, ctors)
        aliases = set()
        for line in open(p, encoding='utf-8', errors='replace'):
            a = ALIAS.search(line)
            if a:
                aliases.add(a.group(1))
                continue
            r = REASSIGN.match(line)
            if r:
                aliases.discard(r.group(1))
            if DYN.search(line):
                dyn.append((loc, os.path.basename(p)[:8], line.strip()[:100]))
            hits = [m for m in DIRECT.finditer(line)
                    if m.group(1).startswith('*') or m.group(1) in aliases]
            for m in hits:
                off = int(m.group(2), 0)
                u = use[off]
                u['locs'].add(loc)
                if m.group(3) and line.strip().startswith('*(') and line.strip().find(m.group(0)) == 0:
                    u['w'] += 1
                    u['vals'][m.group(4).strip()] += 1
                else:
                    u['r'] += 1
            if not hits and OTHER78.search(line) and not DYN.search(line):
                unmatched += 1
    with open(out, 'w') as f:
        f.write('offset\treads\twrites\tvalues\tlocs\n')
        for off in sorted(use):
            u = use[off]
            vals = ','.join(f'{v}x{n}' for v, n in u['vals'].most_common(4))
            f.write(f'0x{off:04X}\t{u["r"]}\t{u["w"]}\t{vals}\t{" ".join(sorted(u["locs"]))}\n')
    print(f'{len(use)} Offsets, {sum(u["r"] for u in use.values())} Lese-, '
          f'{sum(u["w"] for u in use.values())} Schreibzugriffe; '
          f'{unmatched} weitere Zeilen mit +0x78 (indirekt, nicht erfasst)')
    for d in dyn:
        print('dynamisch:', *d)
    lo = [o for o in use if o < 0x3E80]
    print(f'Offsets < 0x3E80: {len(lo)}' + (f' (z. B. {", ".join(hex(o) for o in sorted(lo)[:8])})' if lo else ''))


if __name__ == '__main__':
    main(*sys.argv[1:4])
