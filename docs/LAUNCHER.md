# Startmenü / Setup (Port)

Entscheidung 2026-11-04 (CLAUDE.md): eigenes Menü des Ports mit **Dear ImGui** im selben
Fenster wie das Spiel. Das Original kennt so etwas nicht: Es wird per InstallShield
eingerichtet (`rah.ini`, DATAPATHS.md) und startet direkt mit Auftrag 3 bzw. AutoLoad
(savegame.md). Das Spielmenü im Spiel (Esc, MENU.md) bleibt unverändert das des Originals.

## Wann es erscheint

- **Start ohne Argumente** (Doppelklick): Startmenü.
- **Mit Argumenten**: wie bisher direkt (Prüfläufe, Skripte). Das Menü ist dann aus.

## Aufbau (`src/app/launcher.cpp`)

| Reiter | Inhalt |
|---|---|
| Spielen | **Fortsetzen** (`SaveGameExit.rah`, mit Datum), **Neues Spiel** mit „Intro überspringen“, Liste der Spielstände `SaveGame00…08.rah` (Name aus dem Kopf wie im Spielmenü, Datei-Datum), Laden per Knopf oder Doppelklick, Beenden |
| Datenquelle | CD-Quellen (Ordner, `.iso`, `.cue`; beliebig viele, z. B. CD1 + CD2) über die nativen SDL-Dialoge oder eingetippt; Prüfung; Common: aus `SETUP/DATA1.CAB` entpacken (Standard) oder vorhandenen Ordner angeben |
| Optionen | Lautstärken Musik/Effekte/Sprache (0–100), Stumm, 3D-Ton (`--audio3d`), Untertitel, Intro überspringen, Speichern beim Schließen, Bild (Filter, CRT, Entbänderung, Skalierung, Vollbild; FILTERS.md), Spielstand-Ordner |

- **Prüfung** (`checkDataSources`, `launchersettings.cpp`): Die Quellen werden wie beim
  Spielstart eingebunden. Danach wird geprüft, ob alle 35 Ordner unter `GAME/` vorhanden sind
  (`COMMONCD`, `MENU`, 33 × `LOCnn` der zusammengeführten CDs) und ob es `SETUP/DATA1.CAB` gibt.
  Gestartet werden kann erst, wenn alles da ist.
- **Entpacken**: Ist noch nichts entpackt, packt der Start erst `DATA1.CAB` nach
  `<Benutzer-Datenordner>/Install` aus. Das läuft im Hintergrund, mit Fortschrittsbalken.
  Es ist derselbe Weg wie beim Start ohne `--common`.
- Ein Start wird in Spieloptionen übersetzt, wie es auch die Schalter tun: `--play`, kein AutoLoad,
  und bei Fortsetzen/Laden zusätzlich `--load <datei>`.

## Einstellungen

`<Benutzer-Datenordner>/launcher.ini` liegt unter Linux in `~/.local/share/RentaHeroNativ/`, unter
Windows in `%APPDATA%\RentaHeroNativ\`. Die Datei ist UTF-8 und hat den Abschnitt `[Launcher]` mit
den Schlüsseln `Source1…16`, `Common`, `SaveDir`, `SubTitles`, `Mute`, `Audio3D`,
`MusicVolume`, `SfxVolume`, `SpeechVolume`, `SaveOnClose`, `Fullscreen`, `SkipIntro`, `FixWalkJump`, `HotspotKey`, `Filter`, `Crt`,
`Deband` und `IntegerScale`.
Gespeichert wird bei jedem Start und beim Beenden. ImGui legt kein eigenes `imgui.ini` an.

Ändert man die Lautstärke im Spielmenü, wird das nicht zurückgeschrieben. Der Port speichert
die Lautstärke ohnehin nirgends dauerhaft.

## Neue Schalter (auch ohne Menü)

- `--load <datei.rah>`: Spielstand laden (Auftrag 1) statt Neues Spiel.
- `--skip-intro`: **Port-Option, Standard aus.** Ist sie gesetzt, springt „Neues Spiel“
  (Auftrag 3, `Save_Process` `0x004045C0`) nicht zu Loc 40/Szene 114, sondern gleich an das
  Ende des Intros: Loc40 S116 → `changeScene(7, 1)` mit vorheriger Szene 5, also Loc 1/Szene 7,
  Herkunft 5 (loc40.md). Den Rest erledigt das Spiel selbst, wie nach dem Intro: Ankunft
  mit dem Gleiter, `005_ROD_01`, Treppe → Loc 4/Szene 6 (`Loc01::arriveByGlider`).
  Das Zurücksetzen von Zustand und Inventar bleibt. Was das Intro sonst ändert:
  - `S[0x410C]` (Start 1; nur Loc37 liest es) bleibt 1 statt 2.
  - Den Gegenstand aus Loc35 nimmt das Intro selbst wieder weg.
  - Sonst schreiben Loc35/37/40 keinen Zustand (geprüft im Port-Code).

  Die Option gilt auch für „Neues Spiel“ im Spielmenü.
- `--show-hotspots`: Hotspots immer zeigen wie mit gedrückter Tab-Taste (HOTSPOTS.md, Prüfung).
- `--fix-walk-jump`: **Port-Korrektur, Standard aus.** Ein Klick ohne Weg setzt Rodrigo nicht ans
  Ziel (Original-Fehler, PATHFINDING.md).
- `--fullscreen`: Vollbild (Desktop-Auflösung, Seitenverhältnis bleibt über die logische
  Darstellung 640×480 erhalten).

## Dear ImGui

`third_party/imgui/` enthält v1.92.9b unverändert (MIT, `LICENSE.txt`, Prüfsumme in
`VERSION.txt`): den Kern, die Backends `imgui_impl_sdl3` und `imgui_impl_sdlrenderer3` sowie
`misc/cpp/imgui_stdlib`. Das CMake-Ziel ist `rah_imgui` und wird ohne Warnungen gebaut.
Als Schrift dient die System-Arial bzw. Liberation Sans (`Font::findSystemFont`) mit
Kantenglättung, weil das Menü nicht zum Original gehört. Die Größe richtet sich nach
`SDL_GetWindowDisplayScale`. Das Farbschema ist eigenes Holz/Messing.

## Prüfmodus

- `RAH_LAUNCHER_SHOT=<präfix>` speichert die drei Reiter als `<präfix>-0…2.bmp` und beendet.
- Mit `RAH_LAUNCHER_AUTO=new|continue|load:<0…8>` wird danach so gestartet, als wäre der Knopf
  gedrückt.

Geprüft (2026-11-04), unter Linux und als Windows-Build unter Wine:
- Reiter ohne und mit Daten (beide CUE-Abbilder: 35/35 gefunden, CAB gefunden).
- Entpacken aus dem Menü (129 Common-Dateien).
- Neu → 40/114, ohne Intro → 1/7 → 4/6, Fortsetzen und Laden → Szene aus dem Spielstand.
- Intro-Teil 1 dauert 129 s, so lang wie das Video (128,9 s).
- Tests: `launcher_settings_roundtrip`, `launcher_check_*` (mit `RAH_TEST_DATA` gegen die Originaldaten).

Nicht automatisch geprüft: Bedienung mit Maus/Tastatur und die nativen Dateidialoge
(Linux: xdg-desktop-portal bzw. zenity; Windows: Explorer-Dialog). Ist kein Dialog
verfügbar, gibt es einen Hinweis, und der Pfad lässt sich eintippen.
