#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Klickbereiche/Ebenen eines Schauplatzes ihren Ereignis-IDs zuordnen.

Erkennt in lokalen Dekompilaten (mit angewendeten Namen):
  this[0xNN] = CSzene_GetArea|CSzene_GetObject(.., "Name")    (Feld -> Name)
  uVar = EventValue(n); p = CSzene_GetArea(.., "Name"); CArea_SetEvent(p, uVar)
  CArea_SetEvent(this[0xNN] | *(this[0xNN] + 0x1c0), EventValue(n))
  Inventory_SetEvent(inv, gegenstand, n)
Ausgabe: Markdown-Tabelle ID -> Bereich/Ebene (+ Funktion).

  python3 -I tools/loc_areas.py <datei.c> [...]
"""
import collections
import re
import sys

NUM = r'(0x[0-9a-f]+|\d+)'
FIELD = re.compile(r'\*\([^)]*\)\(\(int\)\w+ \+ ' + NUM + r'\) = (?:\w+ = )?(?:\w+;|CSzene_Get(?:Area|Object)\(.*?,s_(\w+?)_00[0-9a-f]{6}\))')
GETNAMED = re.compile(r'(\w+) = (?:\([^)]*\))?CSzene_Get(?:Area|Object)\(.*?,s_(\w+?)_00[0-9a-f]{6}\)')
ASSIGN_FIELD = re.compile(r'\*\([^)]*\)\(\(int\)\w+ \+ ' + NUM + r'\) = (\w+);')
EVVAL = re.compile(r'(\w+) = EventValue\(' + NUM + r'\)')
SETEV = re.compile(r'CArea_SetEvent\((.+),(\w+|EventValue\(' + NUM + r'\))\);')
INVEV = re.compile(r'Inventory_SetEvent\([^,]+,' + NUM + ',' + NUM + r'\)')
FIELDREF = re.compile(r'\(\(int\)\w+ \+ ' + NUM + r'\)')


def main(paths):
    fields = {}
    rows = collections.defaultdict(set)
    texts = {p: open(p, encoding='utf-8', errors='replace').read().split('\n') for p in paths}
    for p, lines in texts.items():
        local = {}
        for line in lines:
            m = GETNAMED.search(line)
            if m:
                local[m.group(1)] = m.group(2)
            m = ASSIGN_FIELD.search(line)
            if m and m.group(2) in local:
                fields[int(m.group(1), 0)] = local[m.group(2)]
    for p, lines in texts.items():
        fn = p.rsplit('/', 1)[-1][:8]
        evs, local = {}, {}
        for line in lines:
            m = EVVAL.search(line)
            if m:
                evs[m.group(1)] = int(m.group(2), 0)
            m = GETNAMED.search(line)
            if m:
                local[m.group(1)] = m.group(2)
            m = SETEV.search(line)
            if m:
                target, val = m.group(1), m.group(2)
                ev = int(m.group(3), 0) if m.group(3) else evs.get(val)
                if ev is None:
                    continue
                name = local.get(target.strip())
                if name is None:
                    refs = FIELDREF.findall(target)
                    if refs:
                        off = int(refs[0], 0)
                        name = fields.get(off, f'this+0x{off:X}') + (' (Ebene)' if '0x1c0' in target else '')
                rows[ev].add((name or target.strip()[:30], fn))
            m = INVEV.search(line)
            if m:
                rows[int(m.group(2), 0)].add((f'Gegenstand {int(m.group(1), 0)}', fn))
    print('| ID | Bereich / Auslöser | gesetzt in |')
    print('|---:|---|---|')
    for ev in sorted(rows):
        names = collections.defaultdict(list)
        for n, f in sorted(rows[ev]):
            names[n].append(f)
        for n, fs in names.items():
            print(f'| {ev} | `{n}` | {" ".join("`0x" + f.upper() + "`" for f in fs)} |')


if __name__ == '__main__':
    main(sys.argv[1:])
