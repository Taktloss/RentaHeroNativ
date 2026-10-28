# RentaHeroNativ – Projektregeln

Native, quelloffene Neuimplementierung (Source Port) von „Rent-A-Hero“ (neo Software,
deutsche Version V4.0, EXE-Build vom 19.03.1999) für Linux und Windows (x86_64).
Ziel: exakt originalgetreues Verhalten (Logik, Timing, Grafik, Ton, Spielstände).

## Festgelegte Entscheidungen
- Sprache: **C++20**. Plattform: **SDL3**, Videos über **FFmpeg** (LGPL-Build), Build: CMake.
- Lizenz des eigenen Codes: **GPL-3.0-or-later** (`LICENSE`). Neue Quelldateien beginnen mit
  `// SPDX-License-Identifier: GPL-3.0-or-later`.
- Timer (2026-10-27): Fälligkeit millisekundengenau wie das Original, aber Prüfung in jedem
  Hauptschleifendurchlauf im Hauptthread; normale Timer-Ereignisse wie im Original in
  `CSzene_DispatchEvents`; Sofort-Flag-Ereignisse beim nächsten Schleifendurchlauf.
  Schalter `timer_thread = original` (Standard aus) für echte Threads wie 1999 (docs/TIMING.md).
- Schrift (2026-10-27): System-Arial, sonst Liberation Sans; ohne Kantenglättung.
- Common-Dateien (2026-10-27): beides – Standard selbst aus `SETUP/DATA1.CAB` entpacken
  (libunshield, MIT), alternativ vorhandenen `Common`-Ordner angeben.
- Datenquellen (2026-10-27): Port akzeptiert (a) zusammengeführten CD-Ordner, (b) die
  originalen CD-Images direkt (BIN/CUE `MODE2/2352` und `.iso`; eigener ISO-9660-Leser),
  plus optional einen vorhandenen `Common`-Ordner.
- Indeo-5-Transparenz (2026-10-28): selbst dekodieren – Maskenformat aus `ir50_32.dll`
  verstehen (Ghidra) und in `docs/` beschreiben, Referenz per Wine-Messprogramm mit dem
  Original-Codec, eigener Maskenleser zusätzlich zu FFmpeg (VIDEO.md).

## Zu Beginn jeder Sitzung
1. `docs/PROGRESS.md` und diese Datei lesen.
2. Kurz zusammenfassen, wo wir stehen, und einen Plan für die Sitzung vorschlagen.
3. Arbeiten, testen, committen. Am Ende `docs/PROGRESS.md` aktualisieren.

## Rechtliches – strikt
- Niemals Originaldaten einchecken (Grafik, Ton, Video, Texte, EXE, Dumps, Disassembly).
  Erlaubt: eigener Code, eigene Doku, Prüfsummen, Adressen, Funktions-/Strukturnamen.
- Originaldaten liegen lokal und sind per `.gitignore` ausgeschlossen:
  - `RentAHero/` – zusammengeführter Inhalt von CD1+CD2
  - `wineprefix/` – Wine-Installation (Spiel unter `drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero/`)
  - `Rent a Hero CD1/CD2.bin/.cue` – CD-Images
  - `re/private/` – Ghidra-Projekt, Kopie der EXE, Strings, Logs, Exporte
- Kopierschutz/CD-Abfragen nur verstehen, nicht nachbilden, keine Umgehungswerkzeuge.
- go-rent-a-hero (StarHack) hat KEINE Lizenz → keinen Code übernehmen. Höchstens als
  Hinweis lesen; jede Aussage muss trotzdem am Original-Binary belegt werden.

## Arbeitsregeln
1. Niemals raten. Belege: Adresse im Binary, Ghidra-Referenz, Trace oder Testlauf.
   Unsicheres markieren: `// UNVERIFIED: ...`
2. Erst verstehen und in `docs/` beschreiben, dann portieren.
3. Jede portierte Funktion: `// orig: Rent-A-Hero.exe 0x0044F2C0 <OrigName falls bekannt>`
4. Kleine Commits mit aussagekräftigen Nachrichten.
5. Grundlegende Entscheidungen (Sprache, Architektur, Abweichungen) vorher mit dem Nutzer klären.
6. Original-Bugs originalgetreu nachbilden; Fixes nur optional schaltbar (Default aus).

## Token-sparsam arbeiten
- Keine rohe Disassembly / Hexdumps / langen Logs in den Kontext. Stattdessen Skripte
  (Ghidra headless, Python) schreiben, die kompakte Zusammenfassungen erzeugen.
- Lange Ausgaben zuerst mit grep/awk filtern. Pro Sitzung einen klar abgegrenzten Bereich.

## Werkzeuge (siehe docs/SETUP.md)
- Ghidra 12.2: `~/.local/opt/ghidra_12.2_PUBLIC`, JDK: `~/.local/opt/jdk-25.0.5+8`
- Headless: `JAVA_HOME=~/.local/opt/jdk-25.0.5+8 ~/.local/opt/ghidra_12.2_PUBLIC/support/analyzeHeadless re/private/ghidra RahGame -process Rent-A-Hero.exe -noanalysis -scriptPath re/scripts -postScript <Skript> <args>`
- Eigene Skripte: `re/scripts/` (Ghidra), `tools/` (Python, immer mit `python3 -I`)
- Wine-Prefix: `WINEPREFIX=$PWD/wineprefix`, Laufwerk `D:` = `RentAHero/` (als CD-ROM)

## Konventionen
- Dokumentation auf Deutsch, Bezeichner im Code auf Englisch.
- Adressen immer als `0x` + 8 Hex-Stellen (VA, ImageBase 0x00400000).
- Dateiformate: `docs/formats/<format>.md`.
