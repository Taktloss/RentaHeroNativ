#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Durchspiel-Skripte nacheinander in EINEM Spiel abspielen (docs/WALKTHROUGH.md).
# Aufruf: tools/walkthrough.sh [--save-load] <CD-Ordner|.cue> <Common-Ordner> [skript…]
# Ohne Skripte: alle tests/walkthrough/*.txt in Namensreihenfolge.
# --save-load: nach jedem Kapitel speichern, sofort laden und mit dem geladenen Stand weiterspielen.
# --side: jeden Nebenweg tests/walkthrough/side/*.txt einzeln prüfen; die Kopfzeile `# nach: NN_name`
#         nennt das Hauptkapitel, bis zu dem vorher gespielt wird.
set -euo pipefail
cd "$(dirname "$0")/.."
saveload=0
side=0
while [ "${1:-}" = "--save-load" ] || [ "${1:-}" = "--side" ]; do
  [ "$1" = "--save-load" ] && saveload=1
  [ "$1" = "--side" ] && side=1
  shift
done
cd_src=$1; common=$2; shift 2
if [ $side = 1 ]; then
  [ $# -gt 0 ] || set -- tests/walkthrough/side/*.txt
  rc=0
  for s in "$@"; do
    after=$(sed -n 's/^# nach: *\([^ ]*\).*/\1/p' "$s" | head -1)
    chapters=()
    for f in tests/walkthrough/*.txt; do
      chapters+=("$f")
      [ "$(basename "$f" .txt)" = "$after" ] && break
    done
    echo "=== Nebenweg $(basename "$s" .txt) (nach $after)"
    if "$0" "$cd_src" "$common" "${chapters[@]}" "$s" | grep -E 'FEHLER|fertig|^\[walk.*L[0-9]+: (expect|note)' | tail -4; then :; fi
    [ "${PIPESTATUS[0]}" = 0 ] || rc=3
  done
  exit $rc
fi
[ $# -gt 0 ] || set -- tests/walkthrough/*.txt
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for f in "$@"; do
  cat "$f"
  echo
  # nicht nach dem letzten Kapitel (endet mit `end`)
  if [ $saveload = 1 ] && ! grep -qx 'end[[:space:]#]*.*' "$f"; then
    k=$(basename "$f" .txt)
    printf 'save %s\nload %s\n' "$k" "$k"
  fi
done > "$tmp/all.txt"
SDL_VIDEO_DRIVER=${SDL_VIDEO_DRIVER:-offscreen} ./build/rentahero --cd "$cd_src" --common "$common" \
  --play --skip-intro --no-auto-load --save-dir "$tmp/save" --hidden --virtual-clock 20 \
  --walkthrough "$tmp/all.txt" 2>&1 | grep -E '^\[walk|^Schauplatz'
