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
  A16-Animationen. 43 Tests grün.
- 2026-10-28: Klickbereiche (`docs/AREAS.md`, `engine/area`): CArea, globales 8×8-Treffer-
  Raster (Markier-Eigenheit am rechten/unteren Rand), AddArea mit Texten aus `LocNN.GER`,
  Ebenenbereiche in derselben Liste (Name = Dateiname), HitTest (Z aufsteigend, dann Z=255,
  dann freie Bereiche ohne Maske), Transparenzprüfung, OnClick (Typ 0x0B / 0x16),
  Bereichsereignisse in DispatchEvents. Eigenheit: letzte Spalte/Zeile von TCC-Ebenen nicht
  anklickbar. Betrachter: Mausklicks lösen Ereignisse aus (Ausgabe auf stderr). 47 Tests grün.
- 2026-10-28: Maus (`docs/MOUSE.md`): `Scene::onMouseMove` (CSzene_MouseMove), Umschalt-
  Ereignis `0x0044C8D0` (250 ms bis Text, 2000 ms sichtbar), Compose-Schritte 5/12.
  `render/font` (FreeType, einfarbig, CP1252; `rah_font`), `CArea_DrawText` (Ausrichtung,
  schwarzer Rand). `formats/pe` (Ressourcen, RT_GROUP_CURSOR → Schwarzweiß-Zeiger; alle 11
  Zeiger der EXE geprüft). Betrachter: Timer je Durchlauf (`pollTimers`), Bereichstext,
  Originalzeiger (`--exe`, `--font`, `--mouse x y`). Neue Abhängigkeit `freetype-devel`.
  54 Tests grün.
- 2026-10-28: AVI-Ebenen (`docs/VIDEO.md`): CImageAvi analysiert (DirectShow ohne Uhr,
  Zähler `+0x3CC` je Draw, Sample-Eigenheiten, Ende/Seek/Rewind), CLayer_Draw-Typ-3-Weg,
  `0x00446C90` nach jedem Update. `layer+0x30` ist kein „FromSzene“: nur CGame_LoadAnim und
  Vollbildobjekt setzen es (Direktweg auf den Bildschirm, noch offen). Port: `engine/aviimage`
  (Zustandsmodell), `video/ffvideo` (`rah_video`, FFmpeg aus dem Speicher), AVI-Hintergrund als
  CAnimLayer. Alle 124 AVIs: Bildanzahl = Länge, Seek konsistent. Betrachter zeigt LOC05/S10,
  LOC25/S105. Neue Ghidra-Skripte `CallArgs.java`, `FieldWrites.java`. 58 Tests grün.
- 2026-10-28: Ebenen-Interpolation (`docs/ANIMATION.md`): `0x004475F0`/`CLayer_MoveTo`
  `0x004475B0` und `CLayer_Update` portiert (`Layer::tweenTo/moveTo/update`), Kein-Ziel-Wert
  0.0, Farb-Eigenheit am Ende, Ereignis Typ 2. `CLayer_SetColor` mit Neu-Dekodieren
  (TCC: Dateiinhalt bleibt in der Ebene; A16: `A16Anim::setColor` vom Lesezeiger).
  Neues Ghidra-Skript `FpuOps.java` (ftol-Operanden zuordnen). 61 Tests grün.
- 2026-10-28: Inventarleiste (`docs/INVENTORY.md`, `engine/inventory`): Einträge, Add/Remove/
  Clear/Has, Gegenstands- und Benutzungs-Ereignisse, Takt, Zeichnen (dunkel −128 außerhalb,
  jedes Mal neu getönt), Blättern am Rand, Hover-Schleife mit Text aus `DefInvent.GER`,
  Klick → Ereignis oder `CGame_DescribeItem`-Haken. Statustext des Renderers
  (`0x0043E600/0x0043E7C0/0x0043E830`) über Plattform-Haken; `Font::ascent`,
  `drawTextCentered`. Szene: HitTest-Streifen, OnClick/MouseMove/Update/DispatchEvents.
  Betrachter: `--items 1,2,3`. 64 Tests grün.
- 2026-10-28: Hilfe-Objekt `CHelp` (`docs/HINTS.md`, `engine/help`): Symbol direkt auf den
  Bildschirm (an/aus/neu), Laden von `HelpIcon.tcc`/`Helpmask(r).tcc`, `CHelp_Show` (HLP
  entschlüsseln, Maske + Text in die Szenenoberfläche, Ausgabe 640×420), Schließen per Klick.
  Szene: Klick oben rechts (`x ≥ 581`, auch ohne sichtbares Symbol), MouseMove bei offener
  Hilfe. Korrektur: `DrawTextA`-Flag 0x15 = `DT_CENTER|DT_VCENTER|DT_WORDBREAK` – VCENTER
  wirkt ohne SINGLELINE nicht (Statustext und Hilfe oben, umgebrochen): `drawTextWordBreak`.
  TCC-Ebenen ohne Alpha füllen jetzt ihre Ebenenoberfläche (Bild-Slot 3). Betrachter:
  `--hint <HLP>`, `--click x y`. 68 Tests grün.
  Tastatur `0x0044CE50` inzwischen portiert (`Scene::onKeyDown`; Editor-Zweig `0x0044CF00` entfällt).
- 2026-10-28: Intro (Loc40). Vollbildobjekt `CSzene+0x5DC` (`SetModal`, `CGame_LoadAnim`,
  AVI direkt auf den Bildschirm, VIDEO.md); Sprache/Untertitel (`docs/SPEECH.md`,
  `engine/speech`: ohne Ton Timer-Weg des Originals, Dauer nach Textlänge); `CGame`-Gerüst
  und Schauplatzwechsel (`docs/GAME.md`, `src/game/`: `RequestLocation`, `Game_Idle`
  Schritt 7 mit Rücksprung, `Game_SwitchLocation`, `CSzene_Reset`); `Loc40` mit allen drei
  Szenen, Bild-Ereignis-Ketten, Klick-Überspringen. Programm: `--game 40 114 [--subtitles]`.
  71 Tests grün. Ton nur als Protokoll (`g.audioTrace`).
  Offen: Wartestellen (`CGame_BeginWait/Wait`, verschachtelte Schleife), Figurparameter
  der Sprache (`0x00446660`), Renderer `0x0043E960`, `CSzene_Clear` exakt.
- 2026-10-28: Ton. `CAudio`/`CAudioBuffer`/`CAudioStream` vollständig belegt (AUDIO.md:
  Lautstärke linear in dB −33,33…0, Blend-Takt, Ende-Ereignisse, Musik-Wiederholung nach
  30/40/…/300 s, Ausblenden-und-Löschen, 3D nur mit Hardware-3D). Port: WAV-Leser,
  Software-Mischer (22 050 Hz/16 Bit/Stereo), `engine/audio`, SDL3-Ausgabe; Sprache mit
  echten Puffern; Loc40 mit Musik, Effekten und allen 101 Cues (per
  `re/scripts/CallArgsResolved.java` aus dem Binary). `--audio-dump` zur Prüfung. 79 Tests.
- 2026-10-28: 3D-Ton als Option `--audio3d` (Entscheidung Nutzer; wie Hardware-3D + EAX 1.0:
  Position, Dämpfung, Panorama, Hall FOREST/CITY – Algorithmen UNVERIFIED), EAX-Aufrufe in
  Loc40 korrekt (Hallanteil 0.1f statt 0xCD). Lippensync (`CAnimLayer_SetLipSync`, ACS-Leser,
  Auswertung in `NextFrame`, Rücksetzen in `Stop(0)`), Sprecher-Parameter `0x00446660` und
  Sprechende (Modus 10/11/12), `GetPos` (Puffer: Abspielzeiger, Stream: Zeit). 84 Tests.
  Offen: Figur-Variante der Sprache (CActor).
- 2026-10-28: Figuren und Wegsuche. `CActor` vollständig (ACTOR.md: Konstruktor, vtable,
  Typen, Fußpunkt/Zoom/Tönung, Richtungen, Drehen, Laufen inkl. exakter Schrittrechnung aus der
  Disassembly, Schrittgeräusche, Begleiter, Sprechkopf `…Talk.a16` mit Alpha-Zeilen-Löschung);
  `CPathFinder` wörtlich (A* mit Original-Listen, Wegpunkte, Sichtlinie, West-Fehler).
  Szene: Laden aus der SZN, Update, Laufereignisse 0x29/0x2A, Compositor mit Figuren, Klick →
  Laufen; Sprache mit Figur-Variante; Hilfe-Seite nach Fuß-x. Testlauf LOC04/006 mit
  `--walk`: Rodrigo läuft mit Weg und Schrittbildern. Gefunden: bei `Zpos 0` teilt das
  Laufen durch 0 (Original). 93 Tests.
- 2026-10-28: Wartestellen. `CGame_BeginWait/Wait/WaitDone_i`, `CGame_Delay`,
  `WalkAndComment`/`Comment`/`HeroSay` (SCRIPTING.md). Hauptschleife in `pumpMessage`/`idle`
  zerlegt und re-entrant; `Game_Idle`-Verschachtelungstiefe `0x0047D754` mit Abbruch-Flag nur
  auf oberster Ebene; `CGame_dtor` mit 3 Nachrichten je belegtem Platz. Funktions-Handler
  (Wert ≥ 0xFFF) in `fireEvent`. 98 Tests.
- 2026-10-28: Loc35 (Gleiter, Szene 1) wörtlich portiert, `CSzene_DisableAll`,
  `CEvent_SetTimerHandler` als `Game::setTimerHandler`, Destruktor-Reihenfolge LocNN → CGame
  (`destroyLocal`). Probelauf bis zum Wechsel nach 40/115. Gefunden: Indeo-5-Transparenz, die FFmpeg
  ignoriert (graue Kästen; VIDEO.md – 21 Videos, frühere Zahl 80 war falsch gezählt). 99 Tests.
- 2026-10-28: Indeo-5-Transparenz – Entscheidung: selbst dekodieren (CLAUDE.md). Messprogramm
  `tools/iv5ref` (Win32 ohne CRT, VfW-DriverProc direkt) unter Wine: Original-Codec füllt
  transparente Pixel mit Schwarz (= Farbschlüssel). VIDEO.md.
- 2026-10-28: `ir50_32.dll` in Ghidra (`re/private/iv5/ghidra`, `ghidra.sh` per
  `RAH_GHIDRA_*` umschaltbar). Maskenformat weitgehend belegt (formats/indeo5-transparenz.md):
  1 Bit/Pixel, Lauflängen mit Indeo-Huffman-Codes, Zeilen-XOR, Kacheln, Füllfarbe.
- 2026-10-28: Maske vollständig für die Loc35-Videos: Lage (GOP-/Bildkopf, Bandlängen),
  Kachelkopf, Huffman, Lauflängen, XOR, Polarität (0 = transparent). Prototyp
  `tools/iv5ref/iv5mask.py` stimmt pixelgenau mit dem Original-Codec (5 Videos, alle Bilder).
- 2026-10-28: Indeo-5-Transparenz im Port: vollständiger GOP-Kopf belegt, alle 21
  transparenten Videos pixelgenau gegen den Original-Codec; `src/video/iv5mask` (C++-Leser),
  Einbau in `FfmpegVideo` (Maske je Zeitstempel → Schwarz = Farbschlüssel). Loc35 zeigt
  Rodrigo freigestellt. 102 Tests (`RAH_TEST_COMMON` für den Originaldaten-Test).
- 2026-10-28: Loc37 (Landung vor der Höhle) portiert: `CGame_Random`, `CGame_PlaySfxRandom3D`,
  Spielzustand `CGameState` (`src/game/gamestate`, Startwerte der 164 Flags), Mahn-Flag
  `0x410C` mit Original-Durchfall in den Höhlengang. Fehler im Startpfad behoben: `--game`
  führte den ersten Wechsel vor dem Aufbau der Nachrichtenschleife aus (Wartestellen in Start
  brachen ab). Probelauf: Landung → Mahnungen → Loc40/S116. 104 Tests.
- 2026-10-28: Loc01 (Smashville, Szenen 7/1007/8/9/46) vollständig portiert (15 Funktionen):
  Kamerafahrten mit Schrittgeräuschen, Lift, Hebel, Abflugmenü, Kanalwesen-Gespräch, Sack und
  Gegenstand 13. Neu: `CGame_SayActor`, `CGame_FaceAndSay`, `CGame_PlayAnimWait`,
  `CGame_DescribeItem`, `Scene::replaceBackground`. Korrektur: `CSzene_LoadAll` in Loc37 =
  Maske 0xFFFF; Startwert `S[0x3E98]` = 1. Probelauf: Intro S116 → Loc01/S7 (Gleiter landet,
  Gleiter schwebt). 106 Tests.
- 2026-10-28: Loc06 (Ranama im Kloster, Szenen 26/27) portiert; schwebende Ranama per
  MoveTo-Rückruf, Ring-Gespräch mit Zeitfenster, Gegenstand 14. Prüfoption
  `--click-at <bild> <x> <y>` (mehrfach; feuert je einmal, auch in verschachteltem Game_Idle).
  Kette Loc01/S7 → Abflug → Kloster → Loc06/S26 geprüft. 107 Tests.
  Korrektur: Die Haustür in Loc01 führt nach Loc04/Szene 6, nicht Loc06.
- 2026-10-28: Engine: Szene trennt Listen und Besitz (Ebenenliste `+0x04`, Figuren `+0x5E0`,
  Hintergrund `+0x5C0`, Tiefen-/Wegekarte als Zeiger; Besitz bis `reset()`). Neu
  `setBackground` (0x00449D10), `setDepth` (0x0044A140), `setHero` (0x0044A6E0),
  `insertLayer`/`removeLayer` (0x0044AA00/0x0044AA70), `Layer::setPosZ` (0x004474C0).
  Ebenenschleife ruft für `+0x1BC` `CAnimLayer_Update` direkt (0x0044B6A6) – Figur als freie Ebene.
- 2026-10-28: Loc04 (Rodrigos Büro, Szene 6) portiert: drei Ansichten (Raum, Nahaufnahme,
  Ramil-Gespräch) mit Tausch von Hintergrund/Tiefenkarte, Hinsetzen/Aufstehen, Ramil-Besuch
  (Geldsack → Kiste → Gegenstand 7), Ein-/Ausgang mit Held als freier Ebene. Probelauf: erster
  Besuch (Eintreten, Begrüßung) und Tür → Loc01/S7. 109 Tests.
- 2026-10-28: Loc05 (Louis Büro, Szene 10) portiert: Haltungsautomat für Loui mit Timer,
  Gesprächsstufen, Gang durchs Büro mit Klang-Marken. Engine: `AnimLayer::setEndEvent`
  (0x004434A0), Felder `+0x2B4/+0x2B8`. Probelauf: Eintreten, Gespräch. 110 Tests.
- 2026-10-29: Loc03 (Brücke vor Smashville, Szene 12) portiert: Wachen mit Zufallssprüchen,
  Säbelfrau-Auftritt, Piratenangriff (von Szene 15), Ab-/Anflug. Probelauf: Loc01/S7 → Gleiter
  → Kneipe → Loc03 → Wachen anklicken → ganze Folge. 111 Tests.
- 2026-10-29: Fehler behoben: TCC-Standbilder (`S12_RodGlider`, `S12_SabGlider`,
  `S27_RanTalkRing`, `S27_RanSchautRing`) wurden als `AnimLayer` gesucht → `nullptr`; in Loc03
  fehlte dadurch der Gleiter samt Klick. Prüfskript `tools/check_layer_types.py`. `--game` nimmt
  optional die Herkunftsszene (`--game 3 12 8`), sonst bleibt der Held in vielen Schauplätzen
  unsichtbar (wie im Original bei unbekannter Herkunft).
- 2026-10-29: Maus wie das Original (WndProc 0x00402350, Disassembly): nur Tastendruck löst
  `CSzene_OnClick` aus, Loslassen nicht (vorher: jedes Ereignis doppelt, zweites brach die
  Wartestelle des ersten ab – im Fenster „passiert nichts“). Linker Doppelklick =
  `CActor_SetRun` (Held rennt), sonst `CActor_ClearRun` + Klick. Option `--debug`.
- 2026-10-29: Loc02 (Sanchos Kneipe, Szenen 13/1013/2013/14/15) portiert. Engine:
  `AnimLayer::setStepCount` (0x00443300), `setCountEvent` (0x00443530). Probelauf: Eingang →
  Schankraum → Theke (Sancho-Gespräch), Tisch des Fremden. 112 Tests.
- 2026-10-29: Spielmodus verlangt `--common` (ohne fehlte Rodrigo stillschweigend). Loc31
  (Flugkarte, Szene 113) portiert; Probelauf Abflug nach Smashville → Loc01 Szene 8/7. 113 Tests.
- 2026-10-29: Loc10 (Pilzwald, Szenen 37–44 und 66) portiert; Dateisuche findet Latin-1-Namen
  (`038_MÄD_xx`). Probelauf: Flug 113 → 37, 37 → 41 (Teddy) → 37, Nacht (1001), Absturz (66).
- 2026-10-29: Loc12 (Brunnen vor Cynthias Haus, Szene 91) portiert.
- 2026-11-03: Werkzeug REA (MCP, `rea-agents` 6.8.0) eingerichtet; braucht Ghidra 12.1.x
  (parallel installiert) und Node ≥ 22.23.4 bzw. 24.22.1 (SETUP.md). Für Funktionen ohne
  Ghidra-Funktionsgrenzen und mit unseren Namen bleibt `re/scripts/` der Hauptweg.
- 2026-11-03: Spielmenü und Spielstände (MENU.md). Menü-Szene/-Klasse `0x00403520`/`0x004436B0`,
  Öffnen/Schließen mit eingefrorenem Spielbild, einfahrende Rahmen (in den Hintergrund
  gebacken), Optionen (Untertitel, Maus-Verzögerung, drei Lautstärken), Spielstand-Seite (neun
  Plätze, Namen, oranger Rahmen), Laden/Speichern/Neues Spiel/Beenden mit Autosave
  (`Save_Read`/`Save_Write`/`Save_Process`), Figur-Position beim Laden, Tasten Esc/F1–F12 und
  Ctrl-Kürzel, Befehle `0x00402870`. Dialogtexte zur Laufzeit aus der EXE. Optionen `--play`,
  `--auto-load`, `--save-dir`, `--key-at`. Korrekturen: `CGame_ctor` setzt `SetEventVariant`,
  Wartestellen-Rückrufe gehen an die aktuelle Klasse; Hinweis-Countdown `CGame+0x10` (nur
  Szene ≠ 0). Original-Spielstand „Save2Draussen“ lädt korrekt. 124 Tests.
- 2026-11-03: Ziele aus dem Pilzwald portiert: Loc15 (Kanal-Schlucht, Szene 57: Tag/Nacht,
  Gleiter landet und startet, Pirat, Geldsack), Loc08 (Hafen mit Sabrina, Szenen 99/100: Fass,
  Gespräch, Klappe, Boot, Fahne, Bootsfahrt), Loc19 (Esragoth 68/101 und Zwergentor 69: Jasmin,
  Zwerg 1, Ramil, Kette, Thalion). Doku je Schauplatz mit Flags. `tools/save_patch.py` setzt
  Flags in Spielständen für Probeläufe (`--auto-load`). 127 Tests.
- 2026-11-03: Loc07 (Kanalisation, Szenen 47–56) portiert: Leiter von/nach 1/46, Kurbel mit
  altem/neuem Seil, Gitter, Sack, Pirat, Weg zur Schlucht 15/57. Korrektur Loc19: statische
  Ebenen (BMP/TCC) sind keine AnimLayer – Nahaufnahmen hatten keinen Hintergrund (LOCATIONS.md);
  alle übrigen Schauplätze per Skript gegen die SZN-Typen geprüft, keine weiteren Fälle.
- 2026-11-03: Loc20 (Bibliothek mit Tharain, Szene 70) portiert: Begrüßung, acht
  Gesprächsstufen, Stein (Gegenstand 21), öffnet den Weg nach 71.
- 2026-11-03: Loc21 (Zwergenbergwerk, Szenen 71–80) portiert: Schaufel, Weiche, Bohrer mit
  Fahr-/Bohrhebel, Schutt in 76/79, Zunder, Spitzhacke und Stein in 80; Weg nach 22/81.
- 2026-11-03: Inventar-Korrektur: Gegenstand-Ereignisse (`Inventory_SetEvent`) kamen nie an –
  `CGame_SetEventProc` gibt der Leiste die Ereignisprozedur als Benutzen-Handler, `CGame_dtor`
  löscht die Ereignisse (INVENTORY.md). Seil in Loc07 jetzt geprüft. `save_patch.py +ID/-ID`.
- 2026-11-03: Loc22 (Kristallhöhle, Szenen 81/1081) portiert: Beutel, sechs Kristalle auf der
  Steinscheibe, Lösung → 9/89. 131 Tests.
- 2026-11-03: Loc09 (Zwischensequenz Angriff auf Smashville, Szene 89) portiert → 14/90.
- 2026-11-03: Loc14 (bei Cynthia, Szene 90) portiert: Aufwachen, Gespräche, Klappbett, → 12/91.
- 2026-11-03: Loc23 (Piratenschiff, Szene 102) portiert: Kiste, Sabrina befreien, Luke → 24/103,
  Flucht → 8/100.
- 2026-11-03: Loc24 (Kapitänskajüte, Szene 103) portiert: Kampf Sabrina gegen den Kapitän,
  Helm, Flasche, zurück nach 23/102. Neu in der Engine: `Layer::moveBy` (0x00446EC0),
  `Layer::setZ` (0x00446FD0). 136 Tests.
- 2026-11-03: Loc16 (Drachenhorst, Szene 58) portiert: Tag/Nacht, drei Gesprächsreihen,
  Haltungssteuerung des Drachen, Piraten-Sequenz, Hut/Hand, Abflug → 26/93. 137 Tests.
- 2026-11-03: Loc17 (Szenen 59/60, Aasfresser und Totenschädel) portiert: Aasfresser fressen und
  fliegen auf, Busch-Stufen, Ohr → 18/61, Rückkehr aus 61 mit dem Piraten-Festmahl. 138 Tests.
- 2026-11-03: Loc18 (im Totenschädel, Szene 61) portiert: Gehirn, Säure, Rauch-Timer, Kiefer
  öffnen; Kette 61 → 60 → 59 → 58 mit Piraten-Sequenz geprüft. 139 Tests.
- 2026-11-04: Loc26 (Insel, Szenen 93/98) portiert: Landung und Abflug des Drachen, Fahne,
  Grube; Rundflug 58 → 93 → 58 geprüft. 140 Tests.
- 2026-11-04: Loc27 (Hütte 95, Geheimraum 96) portiert: Megophias-Gespräche, Geheimgang, Helm
  ablegen und aufsetzen → 25/104. 141 Tests.
- 2026-11-04: Loc25 (Cyberspace, Endkampf, Szene 105) portiert: Helfer in der richtigen
  Reihenfolge → 41/112. 142 Tests.
- 2026-11-04: Loc41 (Schlusssequenz/Abspann, Szene 112) und Loc28 (Endavin, Szenen 31/32/35)
  portiert. 144 Tests.
- 2026-11-04: Loc29 (Kneipe, Szene 33) und Loc30 (Kerker, Szene 34) portiert. Kette geprüft:
  28/32 → Kneipe → Krug → Kerker → Fenster 35 → zurück → Gitter → Flucht 28/31 → 10/66. 145 Tests.
  **Damit sind alle Schauplätze portiert.**
- 2026-11-04: Hinweissystem portiert: `CGame_Tick` (0x00440B80), `CGame_SetHint` (0x00440BE0),
  `CGame_ShowHint` (0x00440C40), `Hints_Select` (0x004448D0); Klick auf das Symbol zeigt den
  Hinweis des aktuellen Schauplatzes. 147 Tests.
- 2026-11-04: Loc04 Ramil-Pfad (Vorgänger 1004) im Probelauf geprüft: Ramil kommt, neun
  Gesprächsstufen, Ramil geht, Geldsack aus der Kiste per Schreibtisch.
- 2026-11-04: Rauchtest `tools/smoke_scenes.py`: startet jedes `changeScene`-Ziel (86) per
  `--game` mit Vorgänger 0; alle laufen ohne Absturz (Loc35 meldet sich wie im Original als 1).
- Nächste Schritte: Menü im Fenster prüfen (Regler ziehen, Namenseingabe); noch nicht portierte Schauplätze: keine.
