# SPDX-License-Identifier: GPL-3.0-or-later
"""Prüft, ob alle festen Objekt-/Bereichsnamen im Spielcode in den SZN-Dateien vorkommen.

Für src/game/locNN.cpp werden die String-Literale in getObject/getArea/obj/areaOf/
setAreaEvent-ähnlichen Aufrufen gesammelt und gegen die [Abschnitte] aller .SZN unter
<cd>/GAME/LOCNN (und GAME/COMMONCD) geprüft. Schauplatzordner-Abweichungen (z. B. Loc35 lädt
aus LOC35) ergeben sich aus dem Dateinamen.

Gilt auch als vorhanden: eine Datei mit diesem Namen (ohne Endung) im Schauplatzordner,
GAME/COMMONCD oder im Common-Ordner (Animationen per CGame_LoadAnim, Szenendateien,
Hintergrundvarianten).

Grenze: geprüft wird je Schauplatzordner, nicht je Szene oder Lademaske – ein Objekt kann
vorhanden, aber in der gerade geladenen Szene ausgeblendet sein.

Aufruf: python3 -I tools/name_check.py <cd-ordner> [<common-ordner>]
"""
import re
import sys
from pathlib import Path

CALL = re.compile(r"\b(?:getObject|getArea|obj|objA|area|anim|layer|setAreaEvent|areaEvent)\(\s*\"([^\"]+)\"")
TABLE = re.compile(r"\"((?:S\d+|0\d\d|\d{3,4})_[A-Za-z0-9_]+)\"")


def sections(dirs):
    out = set()
    for d in dirs:
        if not d.is_dir():
            continue
        for p in d.iterdir():
            if p.suffix.upper() == ".SZN":
                t = p.read_bytes().decode("latin-1")
                out.update(s.upper() for s in re.findall(r"^\s*\[([^\]]+)\]", t, re.M))
    return out


def basenames(dirs):
    out = set()
    for d in dirs:
        if d and d.is_dir():
            out.update(q.stem.upper() for q in d.iterdir() if q.is_file())
    return out


def find_dir(game: Path, name: str):
    for p in game.iterdir():
        if p.name.upper() == name.upper():
            return p
    return game / name


def main():
    cd = Path(sys.argv[1] if len(sys.argv) > 1 else "RentAHero")
    game = find_dir(cd, "GAME")
    common = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    allsec = sections([p for p in game.iterdir() if p.is_dir()])
    commonFiles = basenames([find_dir(game, "COMMONCD"), common])
    missing_total = 0
    for f in sorted(Path("src/game").glob("loc*.cpp")):
        m = re.match(r"loc(\d+)\.cpp", f.name)
        if not m:
            continue
        loc = f"LOC{int(m.group(1)):02d}"
        own = sections([find_dir(game, loc), find_dir(game, "COMMONCD")]) | basenames([find_dir(game, loc)]) | commonFiles
        text = f.read_text()
        names = set(CALL.findall(text)) | set(TABLE.findall(text))
        # Sprachschlüssel (NNN_XXX_NN) und Ton-/Musiknamen sind keine SZN-Abschnitte
        names = {n for n in names if not re.match(r"^\d{3,4}_[A-Z0-9]{2,3}_\d", n)
                 and not n.startswith(("Sfx_", "Loc", "Gen_", "GEN_"))}
        miss = sorted(n for n in names if n.upper() not in own)
        elsewhere = [n for n in miss if n.upper() in allsec]
        hard = [n for n in miss if n.upper() not in allsec]
        if miss:
            missing_total += len(hard)
            print(f"{f.name}: {len(names)} Namen; nicht im eigenen Ordner: {len(miss)}"
                  + (f" (anderswo: {', '.join(elsewhere)})" if elsewhere else "")
                  + (f" FEHLEN: {', '.join(hard)}" if hard else ""))
        else:
            print(f"{f.name}: {len(names)} Namen, alle vorhanden")
    print(f"{missing_total} Namen fehlen ganz")
    # Töne/Animationen/Musik: Dateiname (mit Endung wie makeFileName) in CD oder Common
    have = {}
    for q in list(cd.rglob("*")) + (list(common.iterdir()) if common and common.is_dir() else []):
        if q.is_file():
            have.setdefault(q.stem.upper(), set()).add(q.suffix.upper())
    ext = {"loadSfx": ".WAV", "loadSfxVol": ".WAV", "loadAnim": ".AVI", "playMusic": None}
    n, miss = 0, []
    for f in sorted(Path("src/game").glob("*.cpp")):
        for fn, name in re.findall(r'\b(loadSfx|loadSfxVol|loadAnim|playMusic)\(\s*"([^"]+)"', f.read_text()):
            n += 1
            stem, e = (name[:-4], {name[-4:].upper()}) if len(name) > 4 and name[-4] == "." else (name, None)
            got = have.get(stem.upper())
            if not got or (e and not (e & got)) or (not e and ext[fn] and ext[fn] not in got):
                miss.append(f"{f.name}:{name}")
    print(f"Ton/Animation/Musik: {n} Aufrufe, Datei fehlt: {', '.join(miss) or 'keine'}")


if __name__ == "__main__":
    main()
