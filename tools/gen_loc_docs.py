#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Automatische Schauplatz-Übersichten docs/locations/locNN.md erzeugen.

Je LocNN (Adressbereich ab LocNN_ctor): Klickbereiche -> Ereignis-IDs
(tools/loc_areas.py) und Fall-Zusammenfassung der Ereignis-Handler
(tools/case_summary.py). Vorhandene, von Hand geschriebene Dateien bleiben unberührt.

  python3 -I tools/gen_loc_docs.py <names.tsv> <decomp-loc-dir> <out-dir>
"""
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKIP = {'Loc01', 'Loc35', 'Loc37', 'Loc40'}   # von Hand dokumentiert


def run(script, files):
    return subprocess.run([sys.executable, '-I', os.path.join(HERE, script)] + files,
                          capture_output=True, text=True, check=True).stdout


def main(names, ddir, outdir):
    ctors = sorted((int(l.split('\t')[0], 16), l.split('\t')[1][:5])
                   for l in open(names) if re.search(r'\tLoc\d+_ctor\t', l))
    files = sorted(glob.glob(os.path.join(ddir, '*.c')))
    procs = set()
    for p in files:
        procs.update(int(a, 16) for a in re.findall(r'CGame_SetEventProc\(\w+,(0x[0-9a-f]+)\)',
                                                     open(p, errors='replace').read()))
    for i, (start, loc) in enumerate(ctors):
        end = ctors[i + 1][0] if i + 1 < len(ctors) else 0x004338A0   # danach CActor (Engine)
        mine = [p for p in files if start <= int(os.path.basename(p)[:8], 16) < end]
        out = os.path.join(outdir, loc.lower() + '.md')
        if loc in SKIP or os.path.exists(out):
            continue
        handlers = [p for p in mine if int(os.path.basename(p)[:8], 16) in procs]
        # Unter-Handler (Verzweigung nach Szene): Funktionen, die event->Param auswerten
        handlers += [p for p in mine if p not in handlers and
                     re.search(r'\(param_\d \+ 0x20\)', open(p, errors='replace').read())]
        with open(out, 'w', encoding='utf-8') as f:
            f.write(f'# {loc} (automatische Übersicht)\n\n'
                    f'Funktionen `0x{start:08X}`–`0x{end - 1:08X}` ({len(mine)} Stück), Ereignis-Handler '
                    + ', '.join(f'`0x{os.path.basename(p)[:8].upper()}`' for p in handlers) +
                    '.\nErzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt '
                    'bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein '
                    'Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.\n\n## Klickbereiche → Ereignis-IDs\n')
            f.write(run('loc_areas.py', mine))
            f.write('\n## Ereignis-Handler (Zusammenfassung je Fall)\n```\n')
            f.write(run('case_summary.py', handlers).replace('\n## ', '\n# ').replace('## ', '# ', 1))
            f.write('```\n')
        print(loc, len(mine), len(handlers))


if __name__ == '__main__':
    main(*sys.argv[1:4])
