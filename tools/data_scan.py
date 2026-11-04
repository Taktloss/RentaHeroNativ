# SPDX-License-Identifier: GPL-3.0-or-later
"""Wertebereiche in den Originaldaten prüfen (Belege für Randfälle im Port).

Liest alle .SZN/.INI unter <cd>/GAME und meldet: Filename-Werte unter 3 Zeichen, Z- und
fps-Bereich, Hex-Schreibweisen. Dazu die Z-Argumente fester Werte im portierten Spielcode
(setPosZ/moveTo/tweenTo/setZ und die Hilfsfunktionen moveWait/enableAt/sabStand).

Aufruf: python3 -I tools/data_scan.py <cd-ordner>
"""
import re
import sys
from pathlib import Path


def scan_data(cd: Path):
    short, zs, fps, hexv, files = [], [], [], [], 0
    for p in cd.joinpath("GAME").rglob("*"):
        if not re.search(r"\.(SZN|INI)$", p.name, re.I):
            continue
        files += 1
        t = p.read_bytes().decode("latin-1")
        for k, v in re.findall(r"^\s*(\w+)\s*=\s*(.*?)\s*$", t, re.M):
            kl = k.lower()
            if kl == "filename" and len(v.strip()) < 3:
                short.append((p.name, v))
            if re.match(r"^-?0[xX][0-9A-Fa-f]+$", v):
                hexv.append((p.name, k, v))
            if kl in ("z", "fps"):
                try:
                    (zs if kl == "z" else fps).append(int(v))
                except ValueError:
                    hexv.append((p.name, k, v))
    print(f"{files} SZN/INI-Dateien")
    print(f"Filename < 3 Zeichen: {short or 'keine'}")
    print(f"Z: {min(zs)}…{max(zs)} ({len(zs)} Werte); fps: {min(fps)}…{max(fps)}")
    print(f"Hex-/unlesbare Werte: {hexv or 'keine'}")


def scan_code(src: Path):
    out, n = [], 0
    for p in sorted(src.glob("*.cpp")):
        t = p.read_text()
        for m in re.finditer(r"\b(setPosZ|moveTo|tweenTo|moveWait|enableAt|sabStand|setZ)\(\s*([^;{]*?)\)\s*[;)&|]", t):
            args = m.group(2)
            if "double" in args or "int " in args:
                continue  # Deklaration
            parts = [a.strip() for a in re.split(r",(?![^(]*\))", args)]
            i = {"setZ": 0, "sabStand": 2, "setPosZ": 2, "moveTo": 2, "tweenTo": 2}.get(m.group(1), 3)
            if len(parts) <= i:
                continue
            n += 1
            try:
                v = float(int(parts[i], 0)) if parts[i].startswith("0x") else float(parts[i])
            except ValueError:
                if parts[i] != "z":
                    out.append((p.name, m.group(1), parts[i]))
                continue
            if not 0 <= v <= 255:
                out.append((p.name, m.group(1), parts[i]))
    print(f"Z-Argumente im Spielcode: {n} Aufrufe, außerhalb 0…255 oder nicht fest: {out or 'keine'}")


if __name__ == "__main__":
    cd = Path(sys.argv[1] if len(sys.argv) > 1 else "RentAHero")
    scan_data(cd)
    scan_code(Path("src/game"))
