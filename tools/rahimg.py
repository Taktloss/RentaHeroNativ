#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decoder für die Bildformate von Rent-A-Hero (TCC, A16) – Analysewerkzeug.

Nachbildung von Rent-A-Hero.exe 0x0043B630 (CImageTcc_Load), 0x0043B740 (Decode),
0x004380B0 (RLE) und 0x00438800 (4-Bit-Expansion). Siehe docs/formats/tcc.md.

  python3 -I tools/rahimg.py info <datei>...
  python3 -I tools/rahimg.py png <datei> <ziel.png> [--order 012]
  python3 -I tools/rahimg.py a16 <datei> <ziel-präfix> [bild,...]
"""
import struct
import sys

HDR = struct.Struct('<7I')  # flags, size0..size3, width, height


def rle_decode(src, pos, size, dst, dpos=0):
    """orig: 0x004380B0. Liefert die Zielposition am Ende."""
    end = pos + size
    while pos < end:
        op = src[pos]
        n = src[pos + 1]
        if op & 1:                      # Füllen: n × Wert
            dst[dpos:dpos + n] = bytes([src[pos + 2]]) * n
            dpos += n
            pos += 3
        elif op & 2:                    # Überspringen (Ziel bleibt unverändert)
            dpos += n
            pos += 2
        else:                           # n Bytes literal
            dst[dpos:dpos + n] = src[pos + 2:pos + 2 + n]
            dpos += n
            pos += 2 + n
    return dpos


def expand4(dst, n):
    """orig: 0x00438800 – 4-Bit-Werte in-place auf 8 Bit; Paar (0,1) bleibt unverändert (Original)."""
    i3 = n
    while (i2 := i3 - 2) > 0:
        b = dst[i2 // 2]
        dst[i2] = b & 0xF0
        dst[i3 - 1] = (b << 4) & 0xFF
        i3 = i2


def load_tcc(data):
    flags, s0, s1, s2, s3, w, h = HDR.unpack_from(data, 0)
    body = data[HDR.size:HDR.size + s0 + s1 + s2 + s3]
    if s0 == 0:  # orig: Header erneut aus dem Puffer (0x0043B740)
        raise NotImplementedError('size0 == 0')
    n = w * h
    planes = []
    off = 0
    for s in (s0, s1, s2):
        p = bytearray(n + 0x10)
        rle_decode(body, off, s, p)
        if flags & 1:
            expand4(p, n)
        planes.append(p[:n])
        off += s
    alpha = None
    if s3:
        alpha = bytearray(n + 0x10)
        rle_decode(body, off, s3, alpha)
        alpha = alpha[:n]
    return dict(flags=flags, w=w, h=h, sizes=(s0, s1, s2, s3), planes=planes, alpha=alpha)


def decode_frame(data, off, state):
    """Ein Bild ab data[off] in die (persistenten) Ebenen von state dekodieren.

    orig: 0x0043B740. Übersprungene Pixel behalten den Wert des Vorgängerbilds.
    Gibt den Offset des nächsten Bilds zurück."""
    flags, s0, s1, s2, s3, w, h = HDR.unpack_from(data, off)
    n = w * h
    if state.get('dim') != (w, h):
        state.update(dim=(w, h), planes=[bytearray(n + 0x10) for _ in range(3)],
                     alpha=bytearray(n + 0x10) if s3 else None)
    pos = off + HDR.size
    for s, p in zip((s0, s1, s2), state['planes']):
        rle_decode(data, pos, s, p)
        if flags & 1:
            expand4(p, n)
        pos += s
    if s3:
        if state['alpha'] is None:  # orig: Puffer nur beim ersten Bild angelegt (0x0043B740)
            raise ValueError('Alpha erst in Folgebild')
        rle_decode(data, pos, s3, state['alpha'])
    state['flags'] = flags
    return pos + s3


def a16_frames(data):
    """orig: 0x0043BB60 – Bildtabelle (max. 300), Schlüsselbild = Bit 4 nicht gesetzt."""
    off, res = 0, []
    while off < len(data) and len(res) < 300:
        flags, s0, s1, s2, s3, w, h = HDR.unpack_from(data, off)
        res.append((off, not (flags & 0x10)))
        off += HDR.size + s0 + s1 + s2 + s3
    return res


def state_png(state, path):
    from PIL import Image
    w, h = state['dim']
    ch = [Image.frombytes('L', (w, h), bytes(p[:w * h])) for p in state['planes']]
    if state['alpha'] is not None:
        ch.append(Image.frombytes('L', (w, h), bytes(state['alpha'][:w * h])))
        Image.merge('RGBA', ch).save(path)
    else:
        Image.merge('RGB', ch).save(path)


def main(argv):
    if argv[1] == 'info':
        for p in argv[2:]:
            d = open(p, 'rb').read()
            flags, s0, s1, s2, s3, w, h = HDR.unpack_from(d, 0)
            extra = len(d) - HDR.size - (s0 + s1 + s2 + s3)
            print(f'{p}: flags={flags} {w}x{h} sizes={s0},{s1},{s2},{s3} rest={extra}')
    elif argv[1] == 'png':
        from PIL import Image
        img = load_tcc(open(argv[2], 'rb').read())
        order = argv[argv.index('--order') + 1] if '--order' in argv else '012'
        ch = [Image.frombytes('L', (img['w'], img['h']), bytes(img['planes'][int(c)])) for c in order]
        if img['alpha'] is not None:
            ch.append(Image.frombytes('L', (img['w'], img['h']), bytes(img['alpha'])))
            Image.merge('RGBA', ch).save(argv[3])
        else:
            Image.merge('RGB', ch).save(argv[3])
    elif argv[1] == 'a16':      # a16 <datei> <ziel-präfix> [bild,...]
        data = open(argv[2], 'rb').read()
        frames = a16_frames(data)
        want = {int(x) for x in argv[4].split(',')} if len(argv) > 4 else {0}
        print(f'{len(frames)} Bilder, Schlüsselbilder: {[i for i, f in enumerate(frames) if f[1]]}')
        state = {}
        for i, (off, key) in enumerate(frames):
            decode_frame(data, off, state)
            if i in want:
                state_png(state, f'{argv[3]}_{i:03d}.png')
            if i >= max(want):
                break


if __name__ == '__main__':
    main(sys.argv)
