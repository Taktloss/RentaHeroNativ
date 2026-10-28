# Architektur des Ports

Stand Phase 5 (Gerüst). Grundsatzentscheidungen: CLAUDE.md „Festgelegte Entscheidungen“.

## Bausteine
| Ordner | Inhalt | Abhängigkeiten |
|---|---|---|
| `src/core/` | Byte-/String-Hilfen (Windows-Pfadsemantik) | – |
| `src/vfs/` | Datenquellen: Ordner ohne Groß-/Kleinschreibung (`DirFileSystem`), ISO 9660 + Joliet aus `.iso` oder BIN/CUE (`IsoFileSystem`, `cue.cpp`), Vereinigung (`Vfs`) | – |
| `src/formats/` | Dateiformate (TCC/A16; folgen: SZN/INI, ZBF, HLP, GER/ACS, Spielstand) | – |
| `src/engine/` | Szene, Ebenen, Figuren, Wegsuche, Ereignisse, Timer, Inventar, Hilfe, Sprache | – |
| `src/audio/` | Software-Mischer (DirectSound-Ersatz, AUDIO.md) | – |
| `src/game/` | `CGame` und Schauplätze (GAME.md) | engine |
| `src/app/` | Programmstart, Fenster, Eingabe, Ton, Video | SDL3, FFmpeg |

`rah_core` (statische Bibliothek) enthält alles ohne Plattformbezug und wird von den
Tests (`rah_tests`) und vom Spiel (`rentahero`) benutzt. Geplant: Engine (Szene, Ebenen,
Ereignisse, Timer, Wartestellen) ebenfalls in `rah_core`, Plattform nur in `src/app/`.

## Bauen und testen
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
RAH_TEST_DATA=<Ordner mit "Rent a Hero CD1.cue"/CD2 und RentAHero/> ./build/rah_tests
```
Ohne `RAH_TEST_DATA` laufen nur die Tests ohne Originaldaten.
Abhängigkeiten (Fedora/Nobara): `SDL3-devel ffmpeg-free-devel unshield-devel freetype-devel`
(FreeType: `rah_font`, Schrift der Bereichstexte; FFmpeg: `rah_video`, Videoebenen; ohne sie
kein Spiel und keine zugehörigen Tests)
(ffmpeg-free enthält die nötigen Decoder Cinepak, Indeo 3, Indeo 5).

## Geprüft
- Alle 2792 Dateien beider CD-Images (BIN/CUE, MODE2/2352, Joliet) byte-gleich zum
  zusammengeführten Ordner.
- Alle 102 gültigen TCC und 219 A16 (7465 Bilder) dekodieren; `S26_RING.TCC` wird als
  ungültig abgewiesen.
