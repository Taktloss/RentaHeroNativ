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

- Editor ist im Release größtenteils funktionslos (nur 13 Befehle haben Code); dafür
  versteckte Tastenkürzel Ctrl+F1…F12 und WndProc dokumentiert → `docs/CONTROLS.md`.
- Klassen über vtables/Ctor-Muster und `new`-Größen rekonstruiert → `docs/STRUCTURES.md`:
  `CGame` + 33 `LocNN`-Klassen (alle Schauplatznummern belegt), `CSzene` (0xACC),
  `CActor` (0x1B00), `CGameState` (0x4110 = Spielstand-Block), Ebenen-Hierarchie
  (`CImage` + Bildformat-Klassen), Renderer-Schnittstelle. Namensliste `docs/symbols/names.tsv`
  (128 Namen, in Ghidra übertragen mit `ApplyNames.java`).

- Ebenen-/Bildklassen zugeordnet (CLayer/CAnimLayer, CImageBmp/Tcc/A16/Avi; Typ aus
  Dateiendung, case-sensitiv) → STRUCTURES.md.
- Ereignisfluss Mausklick → HitTest (Z-Priorität) → CEvent → OnGenericEvent → `docs/EVENTS.md`.
- Timing-Modell (Hauptschleife, 200-ms-Taktstufen, Animations-Uhr, MM-Timer, 5-s-Tick)
  → `docs/TIMING.md`; Ereignisse werden verzögert pro Frame ausgelöst.

### Phase 2 – nächste Schritte
- `CSzene`-Felder (`+0x2C0 … +0x5E8`) und `CActor`-Felder vollständig typisieren;
  Ghidra-Strukturen anlegen, damit Dekompilate lesbar werden.
- Audio-Klassen (Manager, Puffer, Musik) belegen.
- Offene Punkte aus EVENTS.md: Mehrfachauslösung bei Down/Up, `FUN_00434e80` (Laufen),
  Objekte `CSzene+0x5B4` (unterer Streifen) und `+0x9F8` (oberer Balken), `CArea`-Ctor.
- Offene SZN-Punkte: Pfadauflösung `FUN_00448a70`, Stack-Verwechslung im Figurenblock am
  Disassembly prüfen, Perspektivparameter (`this+0x2C0`).
- VC6-CRT besser erkennen (FID), damit Spielcode sauber getrennt ist.
- Timing im Testlauf gegenprüfen (Animationsraten messen), `CGame_Tick`-Zweck.
- Entscheidung Nutzer: MM-Timer im Hauptthread statt Timer-Thread (Sofort-Flag-Race).
