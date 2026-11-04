#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Vergleicht den Maskenleser (rah_iv5scan --compare) über ALLE Bilder jedes Indeo-5-Videos mit
# Transparenz gegen den Original-Codec unter Wine (docs/VIDEO.md).
# Aufruf: tools/iv5ref/compare_all.sh <arbeitsordner> <rah_iv5scan> <ordner mit AVIs> […]
# Voraussetzung: WINEPREFIX mit ir50_32.dll (32 Bit, syswow64), Arbeitsordner wird benutzt.
set -e
work=$1; scan=$2; shift 2
here=$(dirname "$0")
"$here/build.sh" "$work" >/dev/null
cp "$WINEPREFIX/drive_c/windows/syswow64/ir50_32.dll" "$work/"
fail=0
"$scan" "$@" | sed -n 's/^\(.*\.[Aa][Vv][Ii]\): .*Maske [1-9].*/\1/p' | while read -r name; do
  avi=$(find "$@" -name "$name" | head -1)
  python3 -I "$here/avi2bin.py" "$avi" "$work/in.bin" 24 999999 >/dev/null
  (cd "$work" && WINEDEBUG=-all wine iv5ref.exe >/dev/null 2>&1)
  "$scan" --compare "$avi" "$work/out.bin" || echo "ABWEICHUNG: $name"
  rm -f "$work/in.bin" "$work/out.bin"
done
