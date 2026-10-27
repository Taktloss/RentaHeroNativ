# Arbeitsprotokoll

## Stand: 2026-10-27 – Phase 2 weitgehend, Phase 3 begonnen

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
- Offene SZN-Punkte: Stack-Verwechslung im Figurenblock am
  Disassembly prüfen. (`this+0x2C0` ist die Wegkarte, siehe zbf.md.)
- VC6-CRT besser erkennen (FID), damit Spielcode sauber getrennt ist.
- Timing im Testlauf gegenprüfen (Animationsraten messen), `CGame_Tick`-Zweck.
- Entscheidung Nutzer: MM-Timer im Hauptthread statt Timer-Thread (Sofort-Flag-Race).

### Phase 3 – erledigt
- `docs/formats/tcc.md`: TCC/A16 (RLE, Alpha, Delta-Bilder, Seek); Decoder
  `tools/rahimg.py` an allen Dateien geprüft (`S26_RING.TCC` ist ein ungültiger Rest).
- `docs/formats/savegame.md`: am Binary belegt, Aufträge Laden/Speichern/Neues Spiel
  (Start Loc 40/Szene 114)/Autosave; Namenspuffer-Reste für byte-identische Dateien.
- `docs/formats/zbf.md`: Tiefen-/Wegkarte 640×360 mit Skalenkopf.
- `docs/formats/hlp.md` (XOR 0x80 nur wenn erstes Byte ≥ 0x80, Layout links/rechts),
  `docs/formats/media.md` (BMP 24 Bit, WAV, AVI über DirectShow-Streaming; 124 AVIs, 10 fps,
  cvid/IV50/IV32).
- `docs/DATAPATHS.md`: rah.ini-Pfade, Suchreihenfolge der vier Ordner, Namensbildung.
- `docs/formats/ger.md`: LocNN.GER (`[AREA_TEXT]`, `[SPEECH]`), Sprachausgabe, ACS-Lippensync.

- Offene Phase-3-Punkte geschlossen: AVI-Zeit (Bildnummer bei 10 fps), Audio-`GetPos`
  in ms, SZN-Figurenblock am Disassembly belegt (Skala aus der Wegkarte), LUT-Kumulation
  am Code belegt, `docs/RENDERER.md` (16 Bit RGB565 als Referenz, Blit-Formeln,
  Tiefentest `depth > Z`, Zoom 7-Bit-Festkomma, GDI-Schriften), DATA1.CAB = IS5.

### Phase 3 – Rest / Laufzeitprüfungen
- Im Testlauf prüfen: LUT-Kumulation bei getönten Figuren sichtbar? Klick Down/Up doppelt?
  Animationsraten messen.
- Zoom+Tiefe-Blitter und `PerCor` geklärt (RENDERER.md, szn.md); Laufmodus 3 geklärt (ACTOR.md).

### Entscheidungen (2026-10-27, Phase 3)
1. Timer: Mix – Original-Zeitpunkte, Ausführung im Hauptthread, Schalter `timer_thread = original`.
2. Schrift: System-Arial, sonst Liberation Sans, ohne Glättung.
3. Common-Dateien: aus DATA1.CAB entpacken (libunshield) oder Common-Ordner angeben.
4. Datenquellen: CD-Ordner oder direkt die CD-Images (BIN/CUE, ISO).
   Siehe CLAUDE.md „Festgelegte Entscheidungen“.

### Phase 4 – begonnen
- `docs/SCRIPTING.md`: blockierende Wartestellen mit verschachtelter Nachrichtenschleife
  (neue Aktion bricht äußere ab, WM_COMMAND wird verworfen), Spiel-API (25 Funktionen
  benannt), Zufallsgenerator, Doppelklick = Rennen.
- `docs/formats/gamestate.md`: Aufteilung (Hinweis-Countdowns pro Szene + 164 Flags),
  Flag-Tabelle mit Startwerten und Zugriffen (`tools/state_usage.py`).
- `docs/HINTS.md`: Hinweissystem komplett (Tabelle Szene → Bedingung → HLP, Zeiten).
- `docs/LOCATIONS.md`: Übersicht aller 33 Schauplätze (40 Ereignis-Handler, registrierte
  IDs, Szenenwechsel, Flags, Musik) per `tools/loc_summary.py`; Aufbau eines Schauplatzes.
- `docs/PATHFINDING.md`: Wegsuche komplett (globales 160×90-Raster, A* vom Ziel zum Start
  mit Gleichstand-Regel, Wegpunkt-Glättung, mehrere Original-Fehler).
- `docs/ACTOR.md`: Fußpunkt, `SetPosition` (Zoom aus Wegkarte, ortsabhängige Tönung als
  Quadrat-Zonen), Richtungswahl (Laufmodus 1/2/3), Figurentypen-Tabelle.
- `docs/AUDIO.md`: Lautstärke/Frequenz/3D, Blenden im 100-ms-Timer, Animations-Cues
  (Intro Loc40). `tools/compact_c.py`: Dekompilate verdichtet lesen.
- `docs/locations/loc40.md`: Intro (3 Szenen, Bild-Ereignisketten, Überspringen per Klick).
- `docs/locations/loc35-37.md`: spielbare Intro-Abschnitte; SCRIPTING.md: Animations-Modi
  (0/10 ausblenden, 1/11 stehen, 12 zurück, 13 Schleife), Inventar-Funktionen.
- Flag-Statistik korrigiert: jetzt inkl. Ereignis-Handler (159 Flags mit Zugriff).
- `docs/locations/loc01.md`: Szenen, Flags, Bereich→ID-Tabelle und Fall-Zusammenfassung,
  erzeugt mit `tools/loc_areas.py` und `tools/case_summary.py` (Dekompilate mit Namen in
  `re/private/decomp-loc/`). `CGame+0x04` = Position aus Spielstand (SCRIPTING.md).
- `docs/locations/locNN.md` für alle 29 übrigen Schauplätze automatisch erzeugt
  (`tools/gen_loc_docs.py`).
- Nächste Schritte: Flag-Bedeutungen und Abläufe je Schauplatz von Hand ergänzen, oder
  mit dem Port-Gerüst (Phase 5) beginnen;  offene Punkte: `CAnimLayer_Play`-Parameter, Laufmodus 3,
  Sofort-Flag bei Timern, Rolle von `0x0047D130`/`0x0047D74C`.

### Phase 5 – begonnen (Entscheidung Nutzer: Gerüst jetzt, Schauplätze beim Portieren)
- CMake-Gerüst (`rah_core`, `rah_tests`, `rentahero` nur mit SDL3+FFmpeg), eigenes
  Testgerüst. `docs/ARCHITECTURE.md`.
- `src/vfs/`: Ordner (ohne Groß-/Kleinschreibung), ISO 9660/Joliet aus `.iso` und BIN/CUE,
  Vereinigung – alle 2792 Dateien byte-gleich geprüft.
- `src/formats/tcc.*`: TCC/A16 inkl. Original-Eigenheiten (4-Bit-Paar, kumulierende LUT,
  Next/Seek) – alle Originalbilder dekodiert.
- Wartet auf Nutzer: `sudo dnf install SDL3-devel ffmpeg-free-devel unshield-devel`.
- `src/formats/ini.*` (GetPrivateProfile-Semantik; Doppelte in den Daten sind identisch),
  `src/formats/zbf.*` (119 Karten), `src/render/` (RGB565-Puffer, Blitter deckend/Alpha/
  Tiefe/Zoom mit Original-Rundung). RENDERER.md: Tiefen-Adressierung korrigiert.
- Offen: Koordinatensystem des Zielrechtecks beim Tiefentest (Bildschirm oder eigene
  Szenenoberfläche) – geklärt: Szenenkoordinaten, sceneTop = 0 (COMPOSITOR.md).
- `src/engine/paths` (Suchreihenfolge des Originals), `src/formats/szn` (Leseregeln von
  CSzene_LoadSzn; alle 890 Dateiverweise lösen sich auf).
- Compositor analysiert und dokumentiert (`docs/COMPOSITOR.md`): Rechtecklisten immer im
  Vereinigungsmodus, Dirty-Ausbreitung, Z-Stufen von hinten nach vorn, Z=255-Sonderfall,
  Farbschlüssel Schwarz, deckender TCC-Blitter ohne Clipping, DirtyAlpha-Ausschnitt mit
  schrumpfendem unterem Rand, Szenenoberfläche in Szenenkoordinaten (Ausgabe y+60).
  STRUCTURES.md: vtables der beiden Renderer waren vertauscht (korrigiert).
- Port: `core/rect` (Win32-Semantik), `engine/rectlist`, `formats/bmp`, `render/tccsprite`
  (CImageTcc_Draw), `engine/layer` (CLayer), `engine/scene` (Laden, GetObject, Compose).
  Test: LOC01/7A wird korrekt zusammengesetzt (Hintergrund + statische TCC-Ebenen).
- Ebenen und Hintergrund starten inaktiv; die Schauplätze aktivieren sie (Hintergrund über
  `CGame+0x60` aus `CGame_GetViewObjects`, 22 Schauplätze).
- 2026-10-28: CAnimLayer analysiert (`docs/ANIMATION.md`) und portiert
  (`engine/animlayer`): Start-Modi aus Sprungtabelle `0x00443214`, Update-Takt,
  Nachhol-Eigenheit (A16: Cues/Bild-Ereignis doppelt ohne Bildwechsel), OnEnd, Bereiche
  vorwärts/rückwärts, ShowFrame, Stop. `A16Anim` als exaktes Zustandsmodell von
  CImageA16 neu (Halte-Flag, Zähler, Seek-Eigenheiten, tcc.md). `engine/event` (CEvent,
  CEvent_Fire), `engine/globals`. Szene legt A16-Ebenen jetzt an; AVI bleibt offen.
  40 Tests grün.
- 2026-10-28: `Scene::update` (CSzene_Update: Takt-Stufen, Z-Zählerpflege, Abbruch beim
  ersten leeren Platz; Eigenheit: erstes Fenster nach dem Ctor tickt doppelt) und
  `Scene::dispatchEvents` (Ebenen ausblenden vor dem Ende-Ereignis). SDL3/FFmpeg/unshield
  installiert. `rentahero` ist ein Szenen-Betrachter mit dem Frame-Ablauf von Game_Idle:
  `rentahero --cd RentAHero --common <Common> --loc 1 --scene 7A` (Kopflos:
  `SDL_VIDEO_DRIVER=offscreen … --frames N --dump x.ppm --hidden`). LOC01/7A läuft mit allen
  A16-Animationen. 44 Tests grün.
- Nächste Schritte: Bereiche (CArea, AddArea, HitTest) + Maus, CLayer_Update-Interpolation,
  Timer (CEvent_FireTimers), AVI-Ebenen (FFmpeg), Inventarleiste, Figuren, Loc40.
