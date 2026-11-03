#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Spielstand für Probeläufe abwandeln (Format: docs/formats/savegame.md).

Kopiert einen vorhandenen Spielstand, setzt Schauplatz/Szene/Herkunft/Position und
beliebige Flags im Zustandsblock. Mit `rentahero --play --auto-load --save-dir <ordner>`
startet der Port dann genau dort (Datei als SaveGameExit.rah ablegen).

  python3 -I tools/save_patch.py <quelle.rah> <ziel.rah> <loc> <szene> <herkunft-loc>
      <herkunft-szene> <x> <y> <richtung> [OFFSET=WERT ...]

OFFSET ist hexadezimal wie in formats/gamestate.md (z. B. 3F28=1, 401C=-1).
"""
import struct
import sys

HEAD = 4 + 30            # Version, Name
STATE = HEAD + 7 * 4 + 100 * 4 + 4  # Kopf, Inventar, Blockgröße → Zustandsblock


def main(argv):
    if len(argv) < 9:
        sys.exit(__doc__)
    src, dst = argv[0], argv[1]
    loc, scene, ploc, pscene, x, y, d = (int(v) for v in argv[2:9])
    b = bytearray(open(src, 'rb').read())
    if len(b) < STATE:
        sys.exit('Quelle ist kein Spielstand')
    struct.pack_into('<7i', b, HEAD, ploc, pscene, loc, scene, x, y, d)
    for kv in argv[9:]:
        k, v = kv.split('=')
        off = int(k, 16)
        if off % 4 or STATE + off + 4 > len(b):
            sys.exit(f'ungültiger Offset {k}')
        struct.pack_into('<i', b, STATE + off, int(v, 0))
    open(dst, 'wb').write(b)


if __name__ == '__main__':
    main(sys.argv[1:])
