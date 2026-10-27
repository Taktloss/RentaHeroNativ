#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Markdown-Tabelle der Spielflags (docs/formats/gamestate.md) erzeugen.

Startwerte aus dem Dekompilat von CGameState_Reset (0x0044DA90), Zugriffe aus der
Ausgabe von tools/state_usage.py, Hinweis-Bedingungen aus docs/HINTS.md.

  python3 -I tools/gamestate_table.py <0044da90.c> <state.tsv> <HINTS.md>
"""
import re
import sys


def main(reset_c, state_tsv, hints_md):
    init = {}
    for line in open(reset_c):
        m = re.search(r'param_1 \+ (0x[0-9a-f]+|\d+)\) = (0x[0-9a-f]+|\d+);', line)
        if m:
            v = int(m.group(2), 0)
            init[int(m.group(1), 0)] = v - (1 << 32) if v >= 1 << 31 else v
    for o in range(0x4064, 0x407C, 4):      # Schleife im Reset: 6 x 0
        init[o] = 0
    use = {}
    for line in list(open(state_tsv))[1:]:
        o, r, w, vals, locs = line.rstrip('\n').split('\t')
        use[int(o, 16)] = (r, w, vals, locs)
    hints = {}
    for line in open(hints_md, encoding='utf-8'):
        m = re.match(r'\| (\d+(?:, \d+)*) \| (.*?) \| `(\w+)` \|', line)
        if m:
            for o in re.findall(r'S\[0x([0-9A-F]{4})\]', m.group(2)):
                hints.setdefault(int(o, 16), []).append(m.group(3))

    def fmt(vals):
        out = []
        for v in filter(None, vals.split(',')):
            val, _, n = v.rpartition('x')
            val = re.sub(r'\*\(int \*\)\(\*\(int \*\)\(\(int\)\w+ \+ 0x78\) \+ (0x[0-9a-f]+)\)',
                         lambda m: 'S[' + m.group(1) + ']', val)
            out.append(f'{re.sub(r"[ia]Var[0-9]+", "?", val)} ({n}×)')
        return ', '.join(out)

    print('| Offset | Nr. | Start | Lesen | Schreiben | geschriebene Werte | Schauplätze | Hinweis |')
    print('|---|---:|---|---:|---:|---|---|---|')
    for o in sorted(set(init) | set(use)):
        r, w, vals, locs = use.get(o, ('0', '0', '', ''))
        print(f'| `0x{o:04X}` | {(o - 0x3E80) // 4} | {init.get(o, "?")} | {r} | {w} | {fmt(vals)} '
              f'| {locs} | {", ".join(dict.fromkeys(hints.get(o, [])))} |')


if __name__ == '__main__':
    main(*sys.argv[1:4])
