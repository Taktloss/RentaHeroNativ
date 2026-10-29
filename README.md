# RentaHeroNativ

Native, quelloffene Neuimplementierung (Source Port) des Adventures **„Rent-A-Hero“**
(neo Software, deutsche Version V4.0, 1999) für Linux und Windows (x86_64).
Ziel ist exakt originalgetreues Verhalten: Logik, Timing, Grafik, Ton und Spielstände.

> **Keine Spieldaten enthalten.** Zum Spielen brauchst du die beiden Original-CDs
> (oder deren Images). Dieses Repository enthält nur eigenen Code und eigene Dokumentation.

## Stand

**In Arbeit, noch nicht durchspielbar.** Engine (Szenen, Ebenen, Figuren, Wegsuche, Ton,
Videos inklusive Indeo-5-Transparenz, Inventar, Hilfe) steht. Portiert sind bisher das Intro
und die Schauplätze 1–6, 35, 37 und 40. Wege zu noch fehlenden Schauplätzen enden
mit „nicht verfügbar“. Spielstände können noch nicht geladen werden.

Details: [docs/PROGRESS.md](docs/PROGRESS.md)

## Bauen

Benötigt: CMake ≥ 3.24, ein C++20-Compiler (GCC 13+ oder Clang 17+), **SDL3**, **FFmpeg**
(libavformat, libavcodec, libswscale, libswresample, libavutil) und **FreeType**.

Fedora:

```bash
sudo dnf install cmake gcc-c++ ninja-build SDL3-devel ffmpeg-free-devel freetype-devel
```

Debian/Ubuntu (SDL3 ab Ubuntu 25.04 bzw. Debian 13):

```bash
sudo apt install cmake g++ ninja-build libsdl3-dev libavformat-dev libavcodec-dev \
  libswscale-dev libswresample-dev libavutil-dev libfreetype-dev
```

Dann:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/rah_tests          # Selbsttests
```

Ohne SDL3/FFmpeg/FreeType werden nur der Kern und die Tests gebaut.

## Spieldaten bereitstellen

Du brauchst zwei Dinge:

1. **Die CD-Inhalte.** Entweder die Images direkt (`.cue`/`.bin` im Format MODE2/2352 oder
   `.iso`, beide CDs angeben) oder einen Ordner, in den du den Inhalt beider CDs
   zusammenkopiert hast.
2. **Den Ordner `Common`** (Musik, Inventar, Rodrigo-Animationen …). Er steckt im
   Installationsarchiv auf CD 1. Das Entpacken ist im Port noch nicht eingebaut, deshalb
   vorerst mit [unshield](https://github.com/twogood/unshield):

   ```bash
   sudo dnf install unshield        # bzw. sudo apt install unshield
   unshield -d rah-setup x /pfad/zur/CD1/SETUP/DATA1.CAB
   # Ergebnis: rah-setup/Ausf_hrbare_Programmdateien/Common
   ```

   Alternativ geht der `Common`-Ordner einer vorhandenen Installation (auch unter Wine).

## Starten

```bash
COMMON="rah-setup/Ausf_hrbare_Programmdateien/Common"

# mit CD-Images
./build/rentahero --cd "Rent a Hero CD1.cue" --cd "Rent a Hero CD2.cue" --common "$COMMON" --game 40 114

# oder mit zusammenkopiertem CD-Ordner
./build/rentahero --cd RentAHero --common "$COMMON" --game 40 114
```

`--game 40 114` startet mit dem Intro. Einzelne Schauplätze lassen sich direkt starten mit
`--game <Schauplatz> <Szene> [<Herkunftsszene>]`. Die Herkunftsszene bestimmt, wo Rodrigo
auftaucht. Ohne sie bleibt er in vielen Schauplätzen unsichtbar, wie im Original.

| Befehl | Ort |
|---|---|
| `--game 40 114` | Intro |
| `--game 1 7` | Smashville, Balkon vor Rodrigos Haus |
| `--game 4 6 7` | Rodrigos Büro |
| `--game 5 10 9` | Louis Büro |
| `--game 3 12 8` | Brücke vor Smashville (Ankunft mit dem Gleiter) |
| `--game 6 26 8` | Ranama im Kloster |
| `--game 2 13 12` | Sanchos Kneipe, Eingang |

### Bedienung

Wie im Original: Linksklick geht hin bzw. benutzt, Doppelklick lässt Rodrigo rennen,
Inventar am unteren Bildrand, `Esc` beendet.

### Nützliche Optionen

| Option | Wirkung |
|---|---|
| `--subtitles` | Untertitel |
| `--mute`, `--music-vol <0-100>`, `--sfx-vol`, `--speech-vol` | Lautstärke |
| `--audio3d` | 3D-Ton (Abweichung vom Original, Standard aus) |
| `--debug` | Klicks, getroffene Bereiche und Ereignisse in der Konsole |

Alle Optionen: `./build/rentahero` ohne Argumente.

## Projektaufbau

| Ordner | Inhalt |
|---|---|
| `src/core`, `src/vfs` | Grundlagen, Dateisysteme (Ordner, ISO 9660, BIN/CUE) |
| `src/engine` | Szene, Ebenen, Figuren, Ereignisse, Ton |
| `src/video` | FFmpeg-Anbindung, eigener Indeo-5-Maskenleser |
| `src/game` | Spiellogik: ein Modul je Schauplatz (`locNN.cpp`) |
| `docs/` | Analyse des Originals, Dateiformate (`docs/formats/`), Schauplätze (`docs/locations/`) |
| `re/scripts`, `tools/` | Ghidra- und Python-Skripte für die Analyse |

Jede portierte Funktion verweist mit `// orig: Rent-A-Hero.exe 0x…` auf ihre Adresse im
Original-Binary.

## Rechtliches

Eigener Code und eigene Dokumentation stehen unter der **GPL-3.0-or-later** (`LICENSE`).
„Rent-A-Hero“ ist ein Werk von neo Software; alle Rechte an Spiel, Grafik, Ton und Texten
liegen bei den Rechteinhabern. Dieses Projekt enthält und verbreitet keine Originaldaten.
