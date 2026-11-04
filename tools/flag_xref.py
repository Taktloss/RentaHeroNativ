# SPDX-License-Identifier: GPL-3.0-or-later
"""Querverweis der Zustandsflags (CGameState-Offsets) über den portierten Spielcode.

Erkennt `constexpr std::size_t kName = 0xNNNN;` je Datei sowie direkte `flag(0xNNNN)` /
`S(0xNNNN)`-Zugriffe und listet pro Offset: Datei, Konstantenname, Schreibzugriffe (mit Wert)
und Anzahl Lesezugriffe.

Aufruf: python3 -I tools/flag_xref.py [0xNNNN ...]
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

CONST = re.compile(r"constexpr\s+std::size_t\s+(k\w+)\s*=\s*(0x[0-9A-Fa-f]+)")


def scan(src: Path):
    res = defaultdict(lambda: defaultdict(lambda: {"names": set(), "writes": [], "reads": 0}))
    for f in sorted(src.glob("*.cpp")):
        text = f.read_text()
        consts = {m.group(1): int(m.group(2), 16) for m in CONST.finditer(text)}
        accessors = [(n, off) for n, off in consts.items() if 0x3E80 <= off < 0x4110]
        for n, off in accessors:
            res[off][f.name]["names"].add(n)
        pats = [(re.compile(r"(?:flag|S)\(\s*" + re.escape(n) + r"\s*\)"), off) for n, off in accessors]
        pats.append((re.compile(r"(?:flag|S)\(\s*(0x[0-9A-Fa-f]{4})\s*\)"), None))
        for line in text.splitlines():
            code = line.split("//")[0]
            for pat, off in pats:
                for m in pat.finditer(code):
                    o = off if off is not None else int(m.group(1), 16)
                    if not 0x3E80 <= o < 0x4110:
                        continue
                    rest = code[m.end():]
                    w = re.match(r"\s*(=(?!=)|\+=|-=|\+\+|--)\s*([^;]*)", rest)
                    entry = res[o][f.name]
                    if w:
                        entry["writes"].append((w.group(1) + " " + w.group(2)).strip())
                    else:
                        entry["reads"] += 1
    return res


def main():
    res = scan(Path("src/game"))
    want = [int(a, 16) for a in sys.argv[1:]] or sorted(res)
    for off in want:
        print(f"0x{off:04X}")
        for fname, e in sorted(res.get(off, {}).items()):
            ws = sorted(set(e["writes"]))
            print(f"  {fname:12s} {','.join(sorted(e['names'])) or '-':22s} "
                  f"schreibt [{'; '.join(ws)}] liest {e['reads']}×")


if __name__ == "__main__":
    main()
