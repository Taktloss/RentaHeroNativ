#!/usr/bin/env python3
"""Erzeugt ein SHA-256-Manifest eines Verzeichnisses mit Spieldateien.

Ausgabe (stdout): eine Zeile pro Datei im Format
    <sha256>  <größe>  <relativer pfad>
sortiert nach Pfad (case-insensitive). Mit --summary statt dessen eine
Zusammenfassung nach Verzeichnis und Dateiendung (Anzahl, Gesamtgröße).

Der Inhalt der Dateien wird nicht ausgegeben, nur Prüfsummen und Metadaten.
"""
import argparse
import collections
import hashlib
import os
import sys


def walk(root):
    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            full = os.path.join(dirpath, name)
            yield os.path.relpath(full, root), full


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("root")
    ap.add_argument("--summary", action="store_true")
    args = ap.parse_args()

    files = sorted(walk(args.root), key=lambda t: t[0].lower())

    if args.summary:
        by_key = collections.defaultdict(lambda: [0, 0])
        for rel, full in files:
            d = os.path.dirname(rel) or "."
            ext = os.path.splitext(rel)[1].upper() or "(ohne)"
            entry = by_key[(d, ext)]
            entry[0] += 1
            entry[1] += os.path.getsize(full)
        print("| Verzeichnis | Endung | Anzahl | Größe (Bytes) |")
        print("|---|---|---:|---:|")
        for (d, ext), (n, size) in sorted(by_key.items(), key=lambda kv: (kv[0][0].lower(), kv[0][1])):
            print(f"| {d} | {ext} | {n} | {size} |")
        return

    for rel, full in files:
        print(f"{sha256(full)}  {os.path.getsize(full)}  {rel}")


if __name__ == "__main__":
    sys.exit(main())
