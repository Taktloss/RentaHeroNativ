# SPDX-License-Identifier: GPL-3.0-or-later
"""Prüft Schauplatz-Code gegen die SZN-Dateien: Namen, die per obj(...) bzw.
dynamic_cast<AnimLayer*> gesucht werden, müssen Animationen sein (.a16/.avi). Ein TCC-/BMP-
Standbild ist ein reiner Layer, der Cast liefert dann nullptr (Fehler in Loc03/Loc06).
Aufruf: python3 -I tools/check_layer_types.py [CD-Ordner]  (Standard: RentAHero)"""
import glob
import re
import sys

root = sys.argv[1] if len(sys.argv) > 1 else 'RentAHero'
bad = 0
for cpp in sorted(glob.glob('src/game/loc*.cpp')):
    src = open(cpp, encoding='utf-8').read()
    names = set(re.findall(r'(?:obj|dynamic_cast<AnimLayer\*>\(scene_\.getObject)\("([^"]+)"', src))
    loc = re.search(r'loc(\d+)', cpp).group(1)
    files = {}
    for szn in glob.glob(f'{root}/GAME/LOC{loc}/*.[sS][zZ][nN]'):
        cur = None
        for line in open(szn, encoding='latin-1'):
            line = line.strip()
            m = re.match(r'\[(.+)\]', line)
            if m:
                cur = m.group(1).lower()
            elif cur and line.lower().startswith('filename='):
                files.setdefault(cur, line.split('=', 1)[1])
    for n in sorted(names):
        f = files.get(n.lower())
        if f and not f.lower().endswith(('.a16', '.avi')):
            print(f'{cpp}: {n} = {f} ist kein AnimLayer')
            bad += 1
sys.exit(1 if bad else 0)
