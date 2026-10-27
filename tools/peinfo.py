#!/usr/bin/env python3
"""Kompakte Strukturübersicht einer PE32-Datei (für docs/BINARY.md).

Gibt aus: Header-Felder, Sektionen mit Entropie, Rich-Header (MSVC-Toolchain),
Importe nach DLL, Exporte, TLS, Relokationen, Ressourcen-Übersicht und
Kennzeichen-Strings für Compiler/Laufzeit/Packer. Es werden nie rohe Bytes
oder Disassembly ausgegeben.

Aufruf: python3 -I tools/peinfo.py <datei.exe> [--imports]
"""
import argparse
import collections
import datetime
import math
import re
import sys

import pefile

# Strings, die auf Compiler, Laufzeit, Frameworks oder Packer hinweisen.
MARKERS = [
    rb"Microsoft Visual C\+\+ Runtime Library", rb"MSVCRT", rb"MFC\d*", rb"Borland",
    rb"Delphi", rb"Watcom", rb"UPX!?", rb"ASPack", rb"PECompact", rb"\.?AV[A-Za-z_]\w*@@",
    rb"R6\d\d\d", rb"__GLOBAL_HEAP_SELECTED", rb"Copyright[^\0]{0,60}",
    rb"SafeDisc", rb"SecuROM", rb"LaserLock", rb"\\\\\.\\[A-Za-z]+",
]


def entropy(data):
    if not data:
        return 0.0
    counts = collections.Counter(data)
    n = len(data)
    return -sum(c / n * math.log2(c / n) for c in counts.values())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--imports", action="store_true", help="alle importierten Funktionen auflisten")
    args = ap.parse_args()

    pe = pefile.PE(args.path)
    raw = open(args.path, "rb").read()
    fh, oh = pe.FILE_HEADER, pe.OPTIONAL_HEADER

    print(f"Datei: {args.path} ({len(raw)} Bytes)")
    print(f"Machine=0x{fh.Machine:04x} Sektionen={fh.NumberOfSections} "
          f"TimeDateStamp=0x{fh.TimeDateStamp:08x} "
          f"({datetime.datetime.fromtimestamp(fh.TimeDateStamp, datetime.UTC):%Y-%m-%d %H:%M:%S} UTC)")
    print(f"Characteristics=0x{fh.Characteristics:04x} Magic=0x{oh.Magic:x} "
          f"Linker={oh.MajorLinkerVersion}.{oh.MinorLinkerVersion} "
          f"OS={oh.MajorOperatingSystemVersion}.{oh.MinorOperatingSystemVersion} "
          f"Subsystem={oh.Subsystem} ({oh.MajorSubsystemVersion}.{oh.MinorSubsystemVersion})")
    print(f"ImageBase=0x{oh.ImageBase:08x} EntryPoint=0x{oh.ImageBase + oh.AddressOfEntryPoint:08x} "
          f"SizeOfImage=0x{oh.SizeOfImage:x} SectionAlign=0x{oh.SectionAlignment:x} "
          f"FileAlign=0x{oh.FileAlignment:x} DllChar=0x{oh.DllCharacteristics:04x}")
    print(f"Stack reserve/commit=0x{oh.SizeOfStackReserve:x}/0x{oh.SizeOfStackCommit:x} "
          f"Heap reserve/commit=0x{oh.SizeOfHeapReserve:x}/0x{oh.SizeOfHeapCommit:x}")
    ep_sec = pe.get_section_by_rva(oh.AddressOfEntryPoint)
    print(f"EntryPoint liegt in Sektion: {ep_sec.Name.rstrip(b'\\0').decode() if ep_sec else '?'}")

    print("\nSektionen (Name, VA, VSize, RawOff, RawSize, Flags, Entropie):")
    for s in pe.sections:
        name = s.Name.rstrip(b"\0").decode("latin1")
        print(f"  {name:8} 0x{oh.ImageBase + s.VirtualAddress:08x} 0x{s.Misc_VirtualSize:07x} "
              f"0x{s.PointerToRawData:07x} 0x{s.SizeOfRawData:07x} 0x{s.Characteristics:08x} "
              f"{entropy(s.get_data()):.2f}")
    if pe.get_overlay_data_start_offset():
        ov = pe.get_overlay_data_start_offset()
        print(f"  Overlay ab 0x{ov:x}, {len(raw) - ov} Bytes, Entropie {entropy(raw[ov:]):.2f}")

    print("\nData Directories (nicht leer):")
    for d in oh.DATA_DIRECTORY:
        if d.Size:
            print(f"  {d.name:40} RVA=0x{d.VirtualAddress:06x} Size=0x{d.Size:x}")

    rich = pe.parse_rich_header()
    print("\nRich-Header:", "nicht vorhanden" if not rich else "")
    if rich:
        vals = rich["values"]
        for i in range(0, len(vals), 2):
            comp_id, count = vals[i], vals[i + 1]
            print(f"  prodid={comp_id >> 16:3} build={comp_id & 0xffff:5} count={count}")

    print("\nImporte:")
    for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        names = [(i.name.decode() if i.name else f"#{i.ordinal}") for i in entry.imports]
        print(f"  {entry.dll.decode()}: {len(names)} Funktionen")
        if args.imports:
            for i, n in zip(entry.imports, names):
                print(f"    0x{i.address:08x} {n}")

    if hasattr(pe, "DIRECTORY_ENTRY_EXPORT"):
        print(f"\nExporte: {len(pe.DIRECTORY_ENTRY_EXPORT.symbols)}")
        for s in pe.DIRECTORY_ENTRY_EXPORT.symbols:
            print(f"  0x{oh.ImageBase + s.address:08x} {s.name}")
    print(f"\nTLS: {'ja' if hasattr(pe, 'DIRECTORY_ENTRY_TLS') else 'nein'}")
    nrel = sum(len(b.entries) for b in getattr(pe, "DIRECTORY_ENTRY_BASERELOC", []))
    print(f"Relokationen: {nrel}")

    print("\nRessourcen (Typ: Anzahl):")
    res = getattr(pe, "DIRECTORY_ENTRY_RESOURCE", None)
    for t in res.entries if res else []:
        tname = pefile.RESOURCE_TYPE.get(t.id, str(t.name) if t.name else t.id)
        items = []
        for e in t.directory.entries:
            ename = str(e.name) if e.name else str(e.id)
            for lang in e.directory.entries:
                items.append(f"{ename}/lang={lang.id}({lang.data.struct.Size}B)")
        print(f"  {tname}: {len(items)} -> {', '.join(items[:12])}{' …' if len(items) > 12 else ''}")

    print("\nKennzeichen-Strings (Anzahl, Beispiele):")
    for pat in MARKERS:
        hits = re.findall(pat, raw)
        if hits:
            uniq = sorted(set(h.decode("latin1") for h in hits))
            print(f"  {pat.decode()}: {len(hits)} -> {uniq[:6]}")


if __name__ == "__main__":
    sys.exit(main())
