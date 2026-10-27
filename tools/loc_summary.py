#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Übersicht pro Schauplatz aus lokalen Ghidra-Dekompilaten (nicht im Repo).

Je LocNN (Adressbereich ab Konstruktor): Ereignis-Handler, Ereignis-IDs im Switch,
Szenenwechsel (CGame_ChangeScene), gelesene/geschriebene Zustandsflags, Musik.
Gibt Markdown aus.

  python3 -I tools/loc_summary.py <names.tsv> <decomp-dir> [<decomp-dir> ...]
"""
import collections
import glob
import os
import re
import sys

NUM = r'(0x[0-9a-f]+|\d+)'
BASE = r'\*\(int \*\)\(\(int\)\w+ \+ 0x78\)'
FLAG = re.compile(r'\*\((?:int|uint|undefined4) \*\)\(' + BASE + r' \+ ' + NUM + r'\)(\s*=[^=])?')
ALIAS = re.compile(r'(\w+) = ' + BASE + ';')
ALIASED = r'\*\((?:int|uint|undefined4) \*\)\({} \+ ' + NUM + r'\)(\s*=[^=])?'
CHANGE = re.compile(r'CGame_ChangeScene\(\w+,' + NUM + ',' + NUM + r'\)')
SETPROC = re.compile(r'CGame_SetEventProc\(\w+,(0x[0-9a-f]+)\)')
EVVAL = re.compile(r'(?:EventValue|FUN_0043fa80)\(' + NUM + r'\)')
CASE = re.compile(r'^\s*case ' + NUM + ':')
MUSIC = re.compile(r'CGame_PlayMusic\(\w+,s_(\w+)_00[0-9a-f]{6}\)')
PARAM = r'\*\((?:undefined4|int|uint) \*\)\(param_\d \+ 0x20\)'
PALIAS = re.compile(r'(\w+) = ' + PARAM + ';')
REASSIGN = re.compile(r'\s*(\w+) = ')


def main(names, dirs):
    ctors = sorted((int(l.split('\t')[0], 16), l.split('\t')[1][:5])
                   for l in open(names) if re.search(r'\tLoc\d+_ctor\t', l))
    files = {}
    for d in dirs:
        for p in glob.glob(os.path.join(d, '*.c')):
            files.setdefault(int(os.path.basename(p)[:8], 16), p)

    def loc_of(a):
        best = None
        for c, n in ctors:
            if c <= a:
                best = n
        return best

    info = collections.defaultdict(lambda: {'procs': set(), 'ids': set(), 'chg': collections.Counter(),
                                            'reg': set(), 'r': set(), 'w': set(), 'music': set(), 'funcs': 0})
    procs = set()
    for a, p in files.items():
        for line in open(p, encoding='utf-8', errors='replace'):
            for m in SETPROC.finditer(line):
                procs.add(int(m.group(1), 16))
    for a, p in sorted(files.items()):
        loc = loc_of(a)
        if loc is None or a >= 0x435000:
            continue
        i = info[loc]
        i['funcs'] += 1
        if a in procs:
            i['procs'].add(a)
        aliases = set()
        depth = 0
        palias = set()
        sw_depth = []          # Klammertiefe innerhalb eines Switch über event->Param
        for line in open(p, encoding='utf-8', errors='replace'):
            m = ALIAS.search(line)
            if m:
                aliases.add(m.group(1))
                continue
            m = REASSIGN.match(line)
            if m:
                aliases.discard(m.group(1))
            m = PALIAS.search(line)
            if m:
                palias.add(m.group(1))
            elif m := REASSIGN.match(line):
                palias.discard(m.group(1))
            keys = [PARAM] + [re.escape(x) for x in palias]
            if re.search(r'switch\((?:' + '|'.join(keys) + r')\)', line):
                sw_depth.append(depth + 1)
            for m in re.finditer(r'(?:' + '|'.join(keys) + r') [!=]= ' + NUM + r'\b', line):
                i['ids'].add(int(m.group(1), 0))
            m = CASE.match(line)
            if m and sw_depth and depth == sw_depth[-1]:
                i['ids'].add(int(m.group(1), 0))
            depth += line.count('{') - line.count('}')
            while sw_depth and depth < sw_depth[-1]:
                sw_depth.pop()
            pats = [FLAG] + [re.compile(ALIASED.format(re.escape(x))) for x in aliases]
            for pat in pats:
                for m in pat.finditer(line):
                    off = int(m.group(1), 0)
                    (i['w'] if m.group(2) and line.lstrip().startswith(m.group(0)[:2]) and
                     line.lstrip().find(m.group(0)) == 0 else i['r']).add(off)
            for m in CHANGE.finditer(line):
                i['chg'][(int(m.group(2), 0), int(m.group(1), 0))] += 1
            for m in EVVAL.finditer(line):
                i['reg'].add(int(m.group(1), 0))
            for m in MUSIC.finditer(line):
                i['music'].add(m.group(1))

    print('| Schauplatz | Funktionen | Ereignis-Handler | IDs registriert | im Handler erkannt | nur registriert | Wechsel nach (Loc/Szene) | Flags schreibt | Flags liest nur | Musik |')
    print('|---|---:|---|---|---:|---|---|---|---|---|')
    for loc in sorted(info, key=lambda n: int(n[3:])):
        i = info[loc]
        reg = sorted(i['reg'])
        idtxt = (f'{len(reg)} ({reg[0]}–{reg[-1]})' if reg else '–') + f' | {len(i["ids"])} | '
        idtxt += ' '.join(str(x) for x in sorted(i['reg'] - i['ids']))
        chg = ', '.join(f'{l}/{s}' for l, s in sorted(i['chg']))
        w = ' '.join(f'{o:X}' for o in sorted(i['w']))
        r = ' '.join(f'{o:X}' for o in sorted(i['r'] - i['w']))
        procs_t = ' '.join(f'`{a:08X}`' for a in sorted(i['procs']))
        print(f'| {loc} | {i["funcs"]} | {procs_t} | {idtxt} | {chg} | {w} | {r} | {" ".join(sorted(i["music"]))} |')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2:])
