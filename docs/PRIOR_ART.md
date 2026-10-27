# Vorarbeiten (Stand 2026-10-27)

## go-rent-a-hero (StarHack)
- https://github.com/StarHack/go-rent-a-hero – Go, SDL3, angelegt 2026-09-25,
  letzter Push 2026-10-15, 27 Commits, 0 Sterne.
- Laut README „fully playable remake“, durch Reverse Engineering der EXE entstanden.
- Inhalt (laut Dateibaum): Parser für A16, ACS, BMP, SZN, TCC, ZBF, WAV, INI, AVI;
  eigene Decoder für Cinepak und Indeo 5; Installer-Entpacker (InstallShield, ISO,
  BIN/CUE); Wegfindung (`navgrid`), Inventar, Sprache/Untertitel, Spielstände;
  Spiellogik als eine Go-Datei pro Schauplatz (`loc01.go` … `loc41.go`, `intro.go`).
  Offene Issues: Original-Hauptmenü (#9), Hotspot-Hilfe (#10).
- **Lizenz: keine.** Damit gilt „alle Rechte vorbehalten“; Code darf nicht übernommen
  werden. Bewertung: wertvoll als Bestätigung, dass die Spiellogik pro Schauplatz fest
  im Code steckt (deckt sich mit unserem Befund, siehe BINARY.md). Für Originaltreue
  nicht ausreichend belegt (keine Adressen/Doku sichtbar, `docs/` enthält nur eine
  Szenenliste). Wir arbeiten unabhängig vom Binary aus.
- Mögliche Option: Autor um eine Lizenz bitten (Entscheidung beim Nutzer).

## Demo „Rent a Hero Tryout-Version“ (1998)
- https://archive.org/details/kultcds_201 – ISO 431,6 MB, deutsch.
- Noch nicht untersucht. Interessant, falls die Demo-EXE einen anderen Build
  (evtl. mit Debuginfos oder anderem Funktionsumfang) enthält. Download und Analyse
  nur nach Absprache.

## ScummVM / andere Engines
- Keine Unterstützung in ScummVM gefunden. Keine öffentlichen Formatdokumentationen,
  Modding- oder Übersetzungsprojekte gefunden.

## Versionen
- Unsere Version: „Rent-A-Hero V4.0“ (README der CD), EXE-Linkzeitpunkt 1999-03-19,
  CD-Dateien vom 23.03.1999, Systemanforderungen nennen bereits Windows 2000 → später
  Nachdruck, nicht die Erstauflage vom November 1998.
- Englische Fassungen: THQ (UK, Juli 1999), SouthPeak (Nordamerika, 30.06.2000).
  Unterschiede: nicht recherchierbar; keine Quelle gefunden. Hinweis: Unsere Daten
  enthalten bereits `Common/DefInvent.eng` (englische Inventartexte) neben `.ger`,
  und `Rent-A-Hero.exe` enthält „Bitte legen Sie die DVD ein…“ → das Programm kennt
  eine DVD-Variante.
- Patches: keine gefunden.

## Middleware
- Keine (kein Smacker/Bink/Miles). Videos: Cinepak (`cvid`), Indeo 5 (`IV50`),
  Indeo 3 (`IV32`) in AVI, jeweils ohne Tonspur → FFmpeg (LGPL) kann alle decodieren
  (`cinepak`, `indeo5`, `indeo3`). Audio: PCM-WAV. Keine MIDI, kein CD-Audio.

Quellen: Wikipedia „Rent-a-Hero (1998 video game)“, GitHub-API, archive.org.
