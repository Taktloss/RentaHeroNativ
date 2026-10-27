#!/usr/bin/env bash
# Reproduziert den Absturz beim Überspringen des Intros im Original unter Wine.
#
# Startet Rent-A-Hero.exe im angegebenen Prefix, wartet bis das Intro läuft,
# schickt eine Eingabe an das Spielfenster und prüft, ob das Spiel danach noch
# lebt und ob typische Fehlerzeilen im Log auftauchen.
#
# Aufruf: tools/wine-intro-skip-test.sh <prefix> <logdatei> [key|click] [wartezeit_s]
# Zusätzliche Wine-Einstellungen über die Umgebung, z. B. WINEDLLOVERRIDES=...
# Mit SHOTDIR=<verzeichnis> werden Screenshots des Fensters vor der Eingabe
# sowie 5 s und 15 s danach gespeichert (1-before.png, 2-after5.png, 3-after15.png).
set -u

shot() {
    [ -n "${SHOTDIR:-}" ] && import -window "$win" "$SHOTDIR/$1.png" 2>/dev/null
}

prefix=$(realpath "$1")
log=$2
input=${3:-key}
wait_intro=${4:-12}
game_dir="$prefix/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero"

export WINEPREFIX=$prefix
wineserver -k 2>/dev/null
cd "$game_dir" || exit 2
wine Rent-A-Hero.exe >"$log" 2>&1 &

sleep "$wait_intro"
win=$(xdotool search --name '^Rent-A-Hero' 2>/dev/null | head -1)
if [ -z "$win" ]; then
    echo "ERGEBNIS: kein Spielfenster gefunden"
    wineserver -k
    exit 3
fi
xdotool windowactivate --sync "$win" 2>/dev/null
shot 1-before
case $input in
    key) xdotool key --window "$win" Escape ;;
    click) xdotool mousemove --window "$win" 320 240 click 1 ;;
esac
echo "Eingabe '$input' an Fenster $win gesendet"

sleep 5
shot 2-after5
sleep 10
shot 3-after15
if pgrep -f 'Rent-A-Hero.exe' >/dev/null; then
    state="läuft noch"
else
    state="beendet"
fi
errors=$(grep -cE 'err:(seh|sync|quartz)' "$log")
echo "ERGEBNIS: Spiel $state, Fehlerzeilen (seh/sync/quartz): $errors"
grep -E 'err:(seh|sync|quartz)' "$log" | cut -c1-140 | sort | uniq -c | head -8

wineserver -k 2>/dev/null
