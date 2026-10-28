#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prototyp des Indeo-5-Maskenlesers (docs/formats/indeo5-transparenz.md).
Vergleich mit dem Original-Codec (tools/iv5ref):
    python3 -I iv5mask.py <avi> <out.bin von iv5ref> <bilder>"""
import sys

PIC_W = [640, 320, 160, 704, 352, 352, 176, 240, 640, 704, 80, 88, 0, 0, 0]  # 0x10087C90
PIC_H = [480, 240, 120, 480, 240, 288, 144, 180, 240, 240, 60, 72, 0, 0, 0]  # 0x10087CD0


def frames_of(path):
    d = open(path, 'rb').read()
    out = []

    def walk(a, b):
        b = min(b, len(d))
        while a + 8 <= b:
            tag = d[a:a + 4]
            size = int.from_bytes(d[a + 4:a + 8], 'little')
            if tag in (b'RIFF', b'LIST'):
                walk(a + 12, a + 8 + size)
            elif tag[2:4] in (b'dc', b'db') and size > 0:
                out.append(d[a + 8:a + 8 + size])
            a += 8 + size + (size & 1)
    walk(0, len(d))
    return out


class Bits:
    def __init__(self, data, pos=0):
        self.v = int.from_bytes(data, 'little')
        self.p = pos

    def get(self, n):
        r = (self.v >> self.p) & ((1 << n) - 1)
        self.p += n
        return r

    def align(self):
        self.p = (self.p + 7) & ~7


class State:
    """Was der Codec über Bilder hinweg behält (GOP, Codebuch)."""
    def __init__(self):
        self.w = self.h = 0
        self.bands = 0
        self.tile = None
        self.transparent = False
        self.fill = None
        self.desc = None
        self.gop_flags = 0


def parse_gop(r, st):
    # orig: ir50_32.dll 0x100232D0 (GOP-Teil); r steht hinter Startcode/Typ/Nummer
    g = r.get(8)
    st.gop_flags = g
    hdr_end = None
    if g & 1:
        hdr_end = 16 + 8 * r.get(16)
    if g & 0x20:
        r.get(32)
    st.tile = 64 << r.get(2) if g & 0x40 else None
    cfg = r.get(3)
    assert cfg in (0, 1, 2, 6), cfg
    luma, chroma = (cfg & 3) * 3 + 1, (cfg >> 2) * 3 + 1
    ps = r.get(4)
    if ps == 15:
        v = r.get(26)
        st.w, st.h = v >> 13, v & 0x1FFF
    else:
        st.w, st.h = PIC_W[ps], PIC_H[ps]
    st.bands = luma + 2 * chroma
    for _ in range(luma + chroma):  # Beschreibungen: Luma-Bänder, dann Chroma (U und V gemeinsam)
        v = r.get(6)
        if v & 0x38:
            assert r.get(2) == 0
    st.transparent = bool(g & 8)
    if g & 8:
        v = r.get(4)
        assert v & 7 == 0
        if v & 8:
            c = r.get(24)
            st.fill = (c >> 16 & 0xFF, c >> 8 & 0xFF, c & 0xFF)
    r.align()
    v = r.get(24)
    if v & 0x800000:
        while r.get(16) & 0x8000:
            pass
    r.align()
    if hdr_end is not None:
        assert r.p == hdr_end, (r.p, hdr_end)


def build_codes(desc):
    codes = {}
    sym = 0
    for row in range(1, len(desc) + 1):
        pre = row - 1 if row == len(desc) else row
        prefix = (1 << (row - 1)) - 1
        xb = desc[row - 1]
        for j in range(1 << xb):
            if sym > 255:
                break
            rev = int(format(j, f'0{xb}b')[::-1], 2) if xb else 0
            codes[(pre + xb, prefix | (rev << pre))] = sym
            sym += 1
    return codes


def decode_runs(data, codes, count):
    # orig: ir50_32.dll 0x1004F3C0; liefert Bits und verbrauchte Bytes
    r = Bits(data)
    val = r.get(1)
    maxl = max(l for l, _ in codes)
    out = []
    while len(out) < count:
        for n in range(1, maxl + 1):
            c = (r.v >> r.p) & ((1 << n) - 1)
            if (n, c) in codes:
                s = codes[(n, c)]
                r.p += n
                break
        else:
            raise ValueError('kein Huffman-Code')
        if s == 0:
            out.extend([val] * 255)
        else:
            out.extend([val] * s)
            val ^= 1
    # 0x1004F3C0 liest in 16-Bit-Schritten; Rückgabe = (Bitpos + 7) / 8
    return out[:count], (r.p + 7) >> 3


def decode(fr, st, next_mode='data'):
    r = Bits(fr)
    assert r.get(5) == 0x1F
    ftype = r.get(3)
    r.get(8)
    if ftype == 0:
        parse_gop(r, st)
    flags = r.get(8)
    if flags & 1:
        r.get(24)
    if flags & 0x10:
        r.get(16)
    if flags & 0x20:
        while True:
            n = r.get(8)
            if n == 0:
                break
            r.get(8 * n)
    if flags & 0x40:
        if r.get(3) == 7:
            k = r.get(4)
            r.get(4 * k)
    r.get(3)
    r.align()
    b = r.p >> 3
    assert flags & 0x80, 'Bandlängen fehlen'
    for _ in range(st.bands):
        b += 1 if fr[b] & 1 else int.from_bytes(fr[b + 1:b + 4], 'little')
    if not st.transparent:
        return None, 'keine Transparenz'
    m = Bits(fr[b:])
    if flags & 1:
        m.get(24)
    for _ in range(m.get(3)):
        m.get(64)
    delta = m.get(1)
    if m.get(1):
        st.desc = [m.get(4) for _ in range(m.get(4))]
    m.align()
    pos = b + (m.p >> 3)
    w, h = st.w, st.h
    W = (w + 31) & ~31
    th = st.tile or h
    tw = st.tile or w
    rows = [[0] * W for _ in range(h)]
    kinds = []
    bit = 0
    for ty in range(0, h, th):
        for tx in range(0, w, tw):
            nh = th if ty + th <= h else h % th
            twp = (tw + 31) & ~31
            nw = twp if tx + twp <= W else W % twp
            t = Bits(fr[pos:], bit)
            b0 = t.get(1)
            b1 = t.get(1)
            if b0:
                for y in range(ty, ty + nh):
                    rows[y][tx:tx + nw] = [b1] * nw
                bit = t.p
                kinds.append('f')
                continue
            if b1:
                sz = t.get(8)
                if sz == 0xFF:
                    sz = t.get(24)
                start = pos + ((t.p + 7) >> 3)
                nxt = pos + sz
            else:
                start = pos + ((bit + 9) >> 3)
                nxt = None
            bits, used = decode_runs(fr[start:], build_codes(st.desc), nh * nw)
            for i, y in enumerate(range(ty, ty + nh)):
                row = bits[i * nw:(i + 1) * nw]
                if delta and i:
                    row = [a ^ c for a, c in zip(row, rows[y - 1][tx:tx + nw])]
                rows[y][tx:tx + nw] = row
            if nxt is None:
                nxt = start + used if next_mode == 'data' else pos + used
            pos, bit = nxt, 0
            kinds.append('r')
    return rows, ''.join(kinds)


if __name__ == '__main__':
    fr = frames_of(sys.argv[1])
    ref = open(sys.argv[2], 'rb').read()
    n = int(sys.argv[3])
    st = State()
    bad = 0
    kinds = set()
    for i in range(min(n, len(fr))):
        rows, info = decode(fr[i], st)
        kinds.add(info)
        w, h = st.w, st.h
        stride = (w * 3 + 3) & ~3
        size = stride * h
        px = ref[i * (4 + size) + 4:(i + 1) * (4 + size)]
        for y in range(h):
            line = px[(h - 1 - y) * stride:(h - 1 - y) * stride + w * 3]
            for x in range(w):
                black = line[x * 3:x * 3 + 3] == b'\0\0\0'
                if rows[y][x] == 0 and not black:
                    bad += 1
    print(f'{st.w}x{st.h} gop {st.gop_flags:#x} Kachel {st.tile} Füllfarbe {st.fill}: '
          f'{min(n, len(fr))} Bilder, Kachelarten {sorted(kinds)[:4]}, Maske 0 aber nicht schwarz: {bad}')
