#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Durchspiel-Skripte nacheinander in EINEM Spiel abspielen (docs/WALKTHROUGH.md).
# Aufruf: tools/walkthrough.sh <CD-Ordner|.cue> <Common-Ordner> [skript…]
# Ohne Skripte: alle tests/walkthrough/*.txt in Namensreihenfolge.
set -eu
cd "$(dirname "$0")/.."
cd_src=$1; common=$2; shift 2
[ $# -gt 0 ] || set -- tests/walkthrough/*.txt
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cat "$@" > "$tmp/all.txt"
SDL_VIDEO_DRIVER=${SDL_VIDEO_DRIVER:-offscreen} ./build/rentahero --cd "$cd_src" --common "$common" \
  --play --skip-intro --no-auto-load --save-dir "$tmp/save" --hidden --virtual-clock 20 \
  --walkthrough "$tmp/all.txt" 2>&1 | grep -E '^\[walk|^Schauplatz'
