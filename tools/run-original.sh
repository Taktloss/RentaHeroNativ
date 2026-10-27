#!/usr/bin/env bash
# Startet das Original (Referenz) unter Wine.
#
# - virtueller Wine-Desktop 640x480: das Spiel bekommt Fokuswechsel (Alt+Tab)
#   nicht mit und ändert keine Bildschirmauflösung
# - gamescope skaliert ganzzahlig (nearest) auf die Bildschirmhöhe
#
# Aufruf: tools/run-original.sh [--no-scale] [prefix]
#   --no-scale  ohne gamescope (z. B. für Tests mit xdotool/Screenshots)
set -eu

scale=1
if [ "${1:-}" = "--no-scale" ]; then
    scale=0
    shift
fi
root=$(cd "$(dirname "$0")/.." && pwd)
export WINEPREFIX=$(realpath "${1:-$root/wineprefix}")
cd "$WINEPREFIX/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero"

cmd=(wine explorer /desktop=RentAHero,640x480 Rent-A-Hero.exe)
if [ $scale = 1 ] && command -v gamescope >/dev/null; then
    # Ausgabehöhe: größtes ganzzahliges Vielfaches von 480, das auf den Bildschirm passt
    screen_h=$(xrandr --current 2>/dev/null | awk '/\*/{split($1,a,"x"); print a[2]; exit}')
    out_h=$(( ${screen_h:-960} / 480 * 480 ))
    [ "$out_h" -ge 480 ] || out_h=480
    exec gamescope -w 640 -h 480 -H "$out_h" -S integer -F nearest -f -- "${cmd[@]}"
fi
exec "${cmd[@]}"
