#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""AVI → in.bin für iv5ref.exe: Video-strf (BITMAPINFOHEADER) und die ersten N Videobilder.
Aufruf: python3 -I avi2bin.py <datei.avi> <in.bin> [bpp=24] [bilder=8]"""
import struct
import sys


def chunks(d, a, b, out):
    while a + 8 <= b:
        tag = d[a:a + 4]
        size = struct.unpack_from('<I', d, a + 4)[0]
        if tag in (b'RIFF', b'LIST'):
            chunks(d, a + 12, a + 8 + size, out)
        else:
            out.append((tag, d[a + 8:a + 8 + size]))
        a += 8 + size + (size & 1)


def main():
    d = open(sys.argv[1], 'rb').read()
    bpp = int(sys.argv[3]) if len(sys.argv) > 3 else 24
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 8
    items = []
    chunks(d, 0, len(d), items)
    strf = next(c for t, c in items if t == b'strf' and len(c) >= 40 and c[16:20] != b'\0\0\0\0')
    frames = [c for t, c in items if t[2:4] in (b'dc', b'db')][:n]
    with open(sys.argv[2], 'wb') as o:
        o.write(struct.pack('<II', bpp, len(strf)) + strf + struct.pack('<I', len(frames)))
        for f in frames:
            o.write(struct.pack('<I', len(f)) + f)
    w, h = struct.unpack_from('<ii', strf, 4)
    print(f'{w}x{h}, {len(frames)} Bilder, fourcc {strf[16:20].decode(errors="replace")}')


main()
