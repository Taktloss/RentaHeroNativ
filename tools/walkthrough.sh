#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Durchspiel-Skripte nacheinander in EINEM Spiel abspielen (docs/WALKTHROUGH.md).
# Aufruf: tools/walkthrough.sh [--save-load] <CD-Ordner|.cue> <Common-Ordner> [skript…]
# Ohne Skripte: alle tests/walkthrough/*.txt in Namensreihenfolge.
# --save-load: nach jedem Kapitel speichern, sofort laden und mit dem geladenen Stand weiterspielen.
set -euo pipefail
cd "$(dirname "$0")/.."
saveload=0
if [ "${1:-}" = "--save-load" ]; then saveload=1; shift; fi
cd_src=$1; common=$2; shift 2
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
