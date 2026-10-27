# Arbeitsprotokoll

## Stand: 2026-10-27 – Phase 0 und Phase 1 abgeschlossen, warte auf OK

### Erledigt
- Projektstruktur (`src/ tools/ tests/ docs/ re/scripts/ re/private/`), Git-Repo, `.gitignore`.
- CD-Images entpackt und zusammengeführt (`RentAHero/`), Original unter Wine installiert
  und lauffähig (`wineprefix/`, `D:` = CD). Siehe `docs/SETUP.md`.
- Phase 0: Vorarbeiten recherchiert (`docs/PRIOR_ART.md`), Dateien inventarisiert mit
  SHA-256 (`docs/FILES.md`, `docs/manifests/`), Middleware bestimmt (keine; Cinepak/Indeo/PCM).
- Phase 1: PE-Analyse (`tools/peinfo.py`), Ghidra-Projekt + Export (`re/scripts/ExportSummary.java`),
  `docs/BINARY.md`, `docs/symbols/functions.tsv`, `docs/symbols/imports.txt`.

### Wichtigste Erkenntnisse
- VC++ 6.0, C++ ohne RTTI/MFC, Debug-CRT statisch, kein Packer/Schutz, PDB-Name `RahGame.pdb`.
- Keine Skriptsprache: Logik als C++-Klassen `LocNN : CGame` pro Schauplatz (Codegenerator-
  Vorlage im Binary belegt die Klassenstruktur). Szenenaufbau datengetrieben über `.SZN`.
- Spielcode ~214 KB in 714 Funktionen (227 Szenenlogik, 450 Engine, 37 Kern).

### Entscheidungen (2026-10-27)
- Sprache C++20 (SDL3, FFmpeg, CMake), Lizenz GPL-3.0-or-later.
- go-rent-a-hero getestet (lokal in `re/private/go-rent-a-hero`): spielbar, aber kein
  Menü, ein Speicherplatz (Taste `1`), Esc beendet sofort, vieles nur angenähert.
  Nutzer möchte die eigene Neuimplementierung. go-rent-a-hero dient nur als Hinweisquelle.

### Offene Fragen an den Nutzer
1. Demo von archive.org herunterladen und auf einen anderen Build prüfen?
2. go-rent-a-hero: Autor um Lizenz anfragen? (Bis dahin kein Code-Reuse.)

### Referenzumgebung
- Absturz beim Überspringen des Intros (Wines quartz/winegstreamer) gelöst durch native
  DirectShow- und Indeo-DLLs, Start über `tools/run-original.sh` (virtueller Desktop gegen
  Alt+Tab, gamescope). Details und Restpunkte: SETUP.md.
- Spielstand-Format (17 122 Bytes) mit echtem Original-Spielstand gegengeprüft;
  Struktur siehe SETUP.md. Für docs/formats/savegame.md noch am Binary belegen.

### Phase 2 – erledigt
- Ghidra-Hilfsskripte: `re/scripts/ghidra.sh` (Wrapper), `Decompile.java`, `XRefs.java`.
- Editor-Modus: nicht per INI, sondern fest abgeschaltet; per 2-Byte-Patch in der lokalen
  Kopie im Test-Prefix aktiviert, Menü dokumentiert → `docs/EDITOR.md`.
- SZN-Format vollständig aus `FUN_00449380` + Daten → `docs/formats/szn.md`.
- Startkonfiguration `FUN_00404db0`: Schlüssel und fest überschriebene Schalter.

### Phase 2 – nächste Schritte
- Editor im Test-Prefix gezielt nutzen (Areas anzeigen, Szene per „Start Skript“ wählen),
  um Analysehypothesen zu prüfen.
- Engine-Klassen rekonstruieren: Szenen-/View-Objekt (Felder `+0x2C0 … +0x5E8`), Ebene
  (vtable `+0x14` SetPos, `+0x24`), Figur (`+0x15CC` Direction, `+0x16D8` Zpos …);
  Konstruktoren über `operator_new`-Aufrufe und vtables finden, in Ghidra typisieren.
- Offene SZN-Punkte: Pfadauflösung `FUN_00448a70`, Stack-Verwechslung im Figurenblock am
  Disassembly prüfen, Perspektivparameter (`this+0x2C0`).
- VC6-CRT besser erkennen (FID), damit Spielcode sauber getrennt ist.
- Hauptschleife `0x004036E0` und Timer `0x0044D7D0` → Timing-Modell.
