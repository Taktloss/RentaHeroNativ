# SPDX-License-Identifier: GPL-3.0-or-later
# Kenndaten der Cursor-Ressourcen einer PE-Datei ausgeben (keine Bilddaten).
import struct
import sys


def main(path):
    d = open(path, 'rb').read()
    e = struct.unpack_from('<I', d, 0x3C)[0]
    n = struct.unpack_from('<H', d, e + 6)[0]
    opt = e + 24
    magic = struct.unpack_from('<H', d, opt)[0]
    dd = opt + (96 if magic == 0x10B else 112)
    rva = struct.unpack_from('<I', d, dd + 2 * 8)[0]
    secs = []
    o = opt + struct.unpack_from('<H', d, e + 20)[0]
    for i in range(n):
        _, vs, va, rs, ra = struct.unpack_from('<8sIIII', d, o + 40 * i)
        secs.append((va, max(vs, rs), ra))

    def off(r):
        for va, sz, ra in secs:
            if va <= r < va + sz:
                return ra + r - va
        raise ValueError(hex(r))

    base = off(rva)

    def entries(o):
        nn, ni = struct.unpack_from('<HH', d, o + 12)
        for k in range(nn + ni):
            name, ptr = struct.unpack_from('<II', d, o + 16 + 8 * k)
            yield name, ptr

    def leaf(p):
        while p & 0x80000000:
            p = next(entries(base + (p & 0x7FFFFFFF)))[1]
        r, size = struct.unpack_from('<II', d, base + p)
        return off(r), size

    res = {t: p for t, p in entries(base)}
    cur = {i: leaf(p) for i, p in entries(base + (res[1] & 0x7FFFFFFF))}
    for gid, p in entries(base + (res[12] & 0x7FFFFFFF)):
        o, size = leaf(p)
        cnt = struct.unpack_from('<H', d, o + 4)[0]
        parts = []
        for k in range(cnt):
            w, h, planes, bits, nbytes, cid = struct.unpack_from('<HHHHIH', d, o + 6 + 14 * k)
            co, cs = cur[cid]
            hx, hy = struct.unpack_from('<HH', d, co)
            bw, bh, _, bb = struct.unpack_from('<iiHH', d, co + 8)[:4]
            parts.append(f'cur {cid}: {bw}x{bh // 2} {bb}bpp hotspot ({hx},{hy})')
        print(f'Gruppe 0x{gid:X}:', '; '.join(parts))


main(sys.argv[1])
