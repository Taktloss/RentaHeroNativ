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

### Offene Fragen an den Nutzer
1. Sprache des Ports (Empfehlung: C++20, siehe Zusammenfassung im Chat).
2. Lizenz für eigenen Code (Vorschlag: GPL-3.0-or-later; alternativ MIT).
3. Demo von archive.org herunterladen und auf einen anderen Build prüfen?
4. go-rent-a-hero: Autor um Lizenz anfragen? (Bis dahin kein Code-Reuse.)

### Nächste Schritte (Phase 2, nach OK)
- Ghidra: VC6-CRT besser erkennen (FID-Datenbank für VS6 bauen oder Signaturen aus
  LIBCD.LIB, falls verfügbar), damit Spielcode sauber getrennt ist.
- Engine-Klassen rekonstruieren: Konstruktoren über `operator_new`-Aufrufe und vtables
  finden, `CGame`/`CSzene`/`CLayer`/`CArea`/`CEvent`/`CAudioBuffer` typisieren.
- SZN-Lader `0x00449380` dekompilieren → alle SZN-Schlüssel (`LoadCond`, `DirtyAlpha`, …).
- Editor-Modus über `ShowMenu=1` unter Wine ausprobieren.
- Hauptschleife `0x004036E0` und Timer `0x0044D7D0` → Timing-Modell.
