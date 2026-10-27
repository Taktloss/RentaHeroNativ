#!/usr/bin/env python3
"""Schaltet in einer *lokalen Analysekopie* von Rent-A-Hero.exe den eingebauten
Editor-/Entwicklermodus ein (nur für Reverse Engineering, nicht weitergeben).

Hintergrund (docs/BINARY.md, FUN_00404db0): Nach dem Lesen von rah.ini werden
Entwicklerschalter fest überschrieben:
    0x00404E66  c7 05 28 d1 47 00 01 00 00 00   mov [0x0047D128], 1    ; FullScreen = 1
    0x00404E70  89 3d 34 d1 47 00               mov [0x0047D134], edi  ; Editor = 0 (edi == 0)
Der Patch vertauscht die beiden Zieladressen -> Editor = 1, FullScreen = 0 (Fenster).
Nur für EXE mit SHA-256 a1ccd604…4927 (deutsche V4.0).

Aufruf: python3 -I re/scripts/patch_editor_mode.py <Rent-A-Hero.exe>
"""
import hashlib
import sys

ORIG_SHA256 = "a1ccd60497111f0ac2e50d4eedae0dcd7973a49da8947b6a80d7f6fccc7b4927"
IMAGE_BASE = 0x400000  # .text: VA 0x401000 liegt bei Dateioffset 0x1000
PATCHES = [  # (VA, alt, neu)
    (0x00404E68, 0x28, 0x34),
    (0x00404E72, 0x34, 0x28),
]


def main():
    path = sys.argv[1]
    data = bytearray(open(path, "rb").read())
    digest = hashlib.sha256(data).hexdigest()
    if digest != ORIG_SHA256:
        sys.exit(f"unerwartete Datei (SHA-256 {digest}), nichts geändert")
    for va, old, new in PATCHES:
        off = va - IMAGE_BASE
        if data[off] != old:
            sys.exit(f"Byte bei 0x{va:08X} ist 0x{data[off]:02X}, erwartet 0x{old:02X}")
        data[off] = new
    open(path, "wb").write(data)
    print(f"gepatcht: {path} (SHA-256 neu {hashlib.sha256(data).hexdigest()})")


if __name__ == "__main__":
    main()
