# SPDX-License-Identifier: GPL-3.0-or-later
"""Rauchtest: startet jedes changeScene-Ziel aus src/game/locNN.cpp einmal per --game.

Die Vorgänger-Szene ist im Quelltext nicht immer eindeutig; das Skript startet deshalb mit
Vorgänger 0 (Standardzweig). Geprüft wird nur: Prozess endet ohne Absturz und meldet den
erwarteten Schauplatz.

Aufruf: python3 -I tools/smoke_scenes.py <rentahero> <cd> <common> [frames]
"""
import os
import re
import subprocess
import sys
from pathlib import Path


def targets(src: Path):
    out = set()
    for f in sorted(src.glob("loc*.cpp")):
        m = re.match(r"loc(\d+)\.cpp", f.name)
        if not m:
            continue
        here = int(m.group(1))
        for a, b in re.findall(r"changeScene\(((?:0x)?[0-9A-Fa-f]+),\s*((?:0x)?[0-9A-Fa-f]+)\)", f.read_text()):
            out.add((int(b, 0), int(a, 0), here))
    return sorted(out)


# Schauplätze, die sich im Original mit anderer Nummer melden (STRUCTURES.md: Loc35 setzt
# CurLocation = 1 bei 0x00430A64)
REPORTED_AS = {35: 1}


def main():
    exe, cd, common = sys.argv[1:4]
    frames = sys.argv[4] if len(sys.argv) > 4 else "600"
    env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen", SDL_AUDIO_DRIVER="dummy")
    bad = 0
    for loc, scene, src in targets(Path("src/game")):
        cmd = [exe, "--cd", cd, "--common", common, "--game", str(loc), str(scene), "0",
               "--hidden", "--frame-ms", "1", "--frames", frames]
        try:
            r = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=300)
            err = r.stderr
            ok = r.returncode == 0 and f"Schauplatz {REPORTED_AS.get(loc, loc)} /" in err and "nicht verfügbar" not in err
            note = "" if ok else f"rc={r.returncode} " + " | ".join(
                l for l in err.splitlines() if "rror" in l or "nicht" in l or "Schauplatz" in l)[:200]
        except subprocess.TimeoutExpired:
            ok, note = False, "Zeitüberschreitung"
        if not ok:
            bad += 1
        print(f"{'OK  ' if ok else 'FEHL'} {loc:2d}/{scene:<4d} (aus Loc{src:02d}) {note}", flush=True)
    print(f"{bad} Fehler")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
