#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ereignis-Handler fallweise zusammenfassen (lokale Ghidra-Dekompilate).

Je `case N:` eines Switch über event->Param (und je `Param == N`-Zweig): Sprache
(GER-Schlüssel), Szenenwechsel, Flag-Schreib-/Lesezugriffe, Inventar, aufgerufene
Hilfsfunktionen. Ausgabe kompakt auf stdout; nur Bezeichner/Adressen/Zahlen.

  python3 -I tools/case_summary.py <handler.c> [<helper.c> ...]
"""
import re
import sys

NUM = r'(0x[0-9a-f]+|\d+)'
STATE = re.compile(r'\*\(\w+ \*\)\(\*\(int \*\)\(\(int\)\w+ \+ 0x78\) \+ ' + NUM + r'\)(\s*=\s*([^;=][^;]*))?')
CASE = re.compile(r'^\s*case ' + NUM + ':')
IFP = re.compile(r'(?:param_\d \+ 0x20\)|\b[iu]Var\d+) == ' + NUM + r'\)')
SAY = re.compile(r'"?s_(\d{3}_[A-Z0-9]{2,4}_\d{2}\w*?)_00[0-9a-f]{6}')
CHG = re.compile(r'CGame_ChangeScene\(\w+,' + NUM + ',' + NUM + r'\)')
INV = re.compile(r'(Inventory_\w+)\([^,]+,' + NUM + r'\)')
CALL = re.compile(r'\b(FUN_[0-9a-f]{8}|C\w+_\w+|Loc\d+_\w+)\(')
IGN = {'CGame_BeginWait', 'CGame_Wait', 'CONCAT31', 'CEvent_ctor'}


def summarize(path):
    lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
    cur = 'Anfang'
    out = {}
    order = []

    def bucket():
        if cur not in out:
            out[cur] = {'say': [], 'chg': [], 'w': [], 'r': [], 'inv': [], 'call': []}
            order.append(cur)
        return out[cur]

    for line in lines:
        m = CASE.match(line)
        if m:
            cur = f'case {int(m.group(1), 0)}'
        else:
            m = IFP.search(line)
            if m and line.strip().startswith(('if', '} else if', 'else if')):
                cur = f'== {int(m.group(1), 0)}'
        b = bucket()
        for m in SAY.finditer(line):
            b['say'].append(m.group(1))
        for m in CHG.finditer(line):
            b['chg'].append(f'{int(m.group(2), 0)}/{int(m.group(1), 0)}')
        for m in STATE.finditer(line):
            off = f'{int(m.group(1), 0):X}'
            if m.group(2) and line.lstrip().startswith('*') and line.lstrip().find(m.group(0)[:10]) == 0:
                b['w'].append(f'{off}={m.group(3).strip()[:12]}')
            else:
                b['r'].append(off)
        for m in INV.finditer(line):
            b['inv'].append(f'{m.group(1)[10:]}({int(m.group(2), 0)})')
        for m in CALL.finditer(line):
            if m.group(1) not in IGN and not m.group(1).startswith(('Inventory_', 'CGame_ChangeScene')):
                b['call'].append(m.group(1))
    print('##', path.rsplit('/', 1)[-1], lines[0].replace('// ', '')[:60])
    for k in order:
        b = out[k]
        parts = []
        for key, lab in (('say', 'Sprache'), ('chg', '→'), ('w', 'S='), ('r', 'S?'), ('inv', 'Inv'), ('call', 'ruft')):
            vals = list(dict.fromkeys(b[key]))
            if vals:
                parts.append(f'{lab} ' + ' '.join(vals))
        if parts:
            print(f'  {k}: ' + ' | '.join(parts))


if __name__ == '__main__':
    for p in sys.argv[1:]:
        summarize(p)
