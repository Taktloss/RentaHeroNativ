#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ghidra-Dekompilat verdichten, damit es mit wenig Platz lesbar ist.

Entfernt Casts, SEH-Verwaltung, Variablendeklarationen und Leerzeilen; schreibt
Feldzugriffe als  this[0x1C], Zustandsflags als  S[0x3E80], Strings ohne Adresse.
Liest lokale Dekompilate (nicht im Repo), Ausgabe auf stdout.

  python3 -I tools/compact_c.py <datei.c> [...]
"""
import re
import sys

TYPES = r'(?:undefined\d?|int|uint|short|ushort|char|byte|bool|float|double|void|LPCSTR|longlong|code)'
RULES = [
    (re.compile(r'\(' + TYPES + r' \*+\)'), ''),
    (re.compile(r'\(' + TYPES + r'\)'), ''),
    (re.compile(r'\*\(\*\(\((\w+) \+ 0x78\) \+ (0x[0-9a-f]+|\d+)\)'),
     lambda m: f'S[0x{int(m.group(2), 0):04X}]'),
    (re.compile(r'\*\(\((\w+) \+ (0x[0-9a-f]+|\d+)\)'),
     lambda m: f'{m.group(1)}[0x{int(m.group(2), 0):X}]'),
    (re.compile(r'\*\((\w+) \+ (0x[0-9a-f]+|\d+)\)'),
     lambda m: f'{m.group(1)}[0x{int(m.group(2), 0):X}]'),
    (re.compile(r'\bs_(\w+?)_00[0-9a-f]{6}\b'), r'"\1"'),
    (re.compile(r'\bparam_1\b'), 'this_'),
    (re.compile(r'CONCAT31\(extraout_var\w*,(\w+)\)'), r'\1'),
]
SKIP = re.compile(r'ExceptionList|local_c\b|puStack_|local_4 = |^\s*$|^\s*' + TYPES +
                  r'\d* \**\w+( \[\d+\])?;|^\s*undefined\d \w+;|^\s*//|^\s*\w+ \*\w+;')


def main(paths):
    for p in paths:
        print('##', p.rsplit('/', 1)[-1])
        for line in open(p, encoding='utf-8', errors='replace'):
            if SKIP.search(line):
                continue
            for rx, rep in RULES:
                line = rx.sub(rep, line)
            line = line.rstrip()
            print(line[:150])


if __name__ == '__main__':
    main(sys.argv[1:])
