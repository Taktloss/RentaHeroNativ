#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Prototyp des Indeo-5-Maskenlesers (docs/formats/indeo5-transparenz.md); Vergleich mit iv5ref:
#   python3 -I iv5mask.py <avi> <out.bin von iv5ref> <bilder>
# Bisher nur GOP mit Längenfeld, eine Kachel, Bild-Flags ohne 0x20.
import sys
# Prototyp: Indeo-5-Transparenzmaske nach ir50_32.dll (formats/indeo5-transparenz.md)
def frames_of(path):
    d=open(path,'rb').read(); out=[]
    def walk(a,b):
        b=min(b,len(d))
        while a+8<=b:
            tag=d[a:a+4]; size=int.from_bytes(d[a+4:a+8],'little')
            if tag in (b'RIFF',b'LIST'): walk(a+12,a+8+size)
            elif tag[2:4] in (b'dc',b'db') and size>0: out.append(d[a+8:a+8+size])
            a+=8+size+(size&1)
    walk(0,len(d)); return out

def build_table(desc):
    codes={}  # (länge, code) -> symbol
    sym=0; k=len(desc)
    for r in range(1,k+1):
        last = r==k
        pre = r-1 if last else r      # Präfixlänge
        prefix=(1<<(r-1))-1           # r-1 Einsen, dann 0 (letzte Zeile ohne 0)
        xb=desc[r-1]
        for j in range(1<<xb):
            if sym>255: break
            rev=int(format(j,f'0{xb}b')[::-1],2) if xb else 0
            codes[(pre+xb, prefix | (rev<<pre))]=sym; sym+=1
    return codes

class St: desc=None

def decode(fr, w, h, st, nbands=6):
    v=int.from_bytes(fr,'little')
    def g(p,n): return (v>>p)&((1<<n)-1)
    ft=g(5,3); p=16+(g(24,16)*8 if ft==0 else 0)
    fl=g(p,8); p+=8
    if fl&1: p+=24
    if fl&0x10: p+=16
    assert not fl&0x20
    if fl&0x40:
        i=g(p,3); p+=3
        if i==7: k=g(p,4); p+=4+4*k
    p+=3; b=(p+7)//8
    for n in range(nbands):
        b += 1 if fr[b]&1 else int.from_bytes(fr[b+1:b+4],'little')
    m=int.from_bytes(fr[b:],'little'); q=0
    def h_(n):
        nonlocal q
        r=(m>>q)&((1<<n)-1); q+=n; return r
    if fl&1: h_(24)
    nr=h_(3)
    for _ in range(nr): h_(64)
    delta=h_(1)
    if h_(1):
        k=h_(4); st.desc=[h_(4) for _ in range(k)]
    tb=b+(q+7)//8
    t=int.from_bytes(fr[tb:],'little'); tp=0
    b0=t&1; b1=(t>>1)&1
    W=(w+31)&~31
    rows=[[0]*W for _ in range(h)]
    if b0:
        val=(t>>2)&1
        return [[val]*W for _ in range(h)], 'füll'
    if b1:
        sz=(t>>2)&0xff; tp=10
        if sz==0xff: sz=(t>>10)&0xffffff; tp=34
        start=tb+(tp+7)//8
    else:
        start=tb+1
    codes=build_table(st.desc); maxl=max(l for l,_ in codes)
    data=int.from_bytes(fr[start:],'little'); bp=1
    val=data&1
    out=[]; total=W*h
    while len(out)<total:
        for L in range(1,maxl+1):
            c=(data>>bp)&((1<<L)-1)
            if (L,c) in codes: s=codes[(L,c)]; bp+=L; break
        else: raise ValueError('kein Code')
        if s==0: out.extend([val]*255)
        else: out.extend([val]*s); val^=1
    out=out[:total]
    rows=[out[y*W:(y+1)*W] for y in range(h)]
    if delta:
        for y in range(1,h): rows[y]=[a^c for a,c in zip(rows[y],rows[y-1])]
    return rows, f'rle ende bit {bp}'

def gop_info(fr):
    v=int.from_bytes(fr[:32],'little'); p=16
    def g(n):
        nonlocal p
        r=(v>>p)&((1<<n)-1); p+=n; return r
    gf=g(8)
    if gf&1: g(16)
    if gf&0x20: g(32)
    if gf&0x40: g(2)
    cfg=g(3); ps=g(4)
    nb=(cfg&3)*3+1+2*((cfg>>2)*3+1)
    if ps==15: x=g(26); return x>>13, x&0x1fff, nb, gf
    return None, None, nb, gf

if __name__=='__main__':
    import struct
    fr=frames_of(sys.argv[1]); ref=open(sys.argv[2],'rb').read(); n=int(sys.argv[3])
    w,h,nb,gf=gop_info(fr[0])
    st=St(); stride=(w*3+3)&~3; size=stride*h; bad=0; kinds={}
    for i in range(min(n,len(fr))):
        rows,info=decode(fr[i],w,h,st,nb)
        kinds[info.split()[0]]=kinds.get(info.split()[0],0)+1
        px=ref[i*(4+size)+4:i*(4+size)+4+size]
        for y in range(h):
            row=px[(h-1-y)*stride:(h-1-y)*stride+w*3]
            for x in range(w):
                if rows[y][x]==0 and row[x*3:x*3+3]!=b'\0\0\0': bad+=1
    print(f"{w}x{h} Bänder {nb} gop {gf:#x}: {min(n,len(fr))} Bilder, Arten {kinds}, Maske 0 aber nicht schwarz: {bad}")
