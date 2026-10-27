# Eingebauter Editor (Entwicklermodus)

Der Release-Build enthält den vollständigen Szenen-Editor der Entwickler. Er ist
nur durch fest verdrahtete Schalter abgeschaltet und **nicht** über `rah.ini`
erreichbar (die `ShowMenu`/`ShowAreas`-Schlüssel in `GAME/LOC21/RAH.INI` stammen von
einem älteren Build; der String `ShowMenu` kommt in dieser EXE nicht vor).

## Belege
- `FUN_00404db0` (INI lesen): liest `[MODES] TrueColor, FullScreen, SubTitles, RealTime,
  AutoLoadGame, MouseDelay` und `[SOUNDS] MusicVol, SfxVol, SpeechVol`. Danach werden
  überschrieben: `0x0047D12C` (RealTime) = 1, `0x0047D128` (FullScreen) = 1,
  `0x0047D134` (Editor) = 0, außerdem `0x0047D130/138/13C/140/144` = 0 (Bedeutung offen).
- `0x0047D134` wird nur bei `0x00404E70` geschrieben und an 6 Stellen gelesen
  (`FUN_00401e00`, `FUN_00402670`, `FUN_00402790`, `0x0040240E`).
- `FUN_00401e00` (WinMain): `if (editor) LoadMenuA(hInst, "AppMenu")`;
  `SetMenu` wenn `!FullScreen || editor`. Renderer-Objekt: `0x0047C870` bei Vollbild,
  sonst `0x0047C230`; Aufruf über vtable-Offset 8.

## Einschalten (nur lokale Analysekopie)
`re/scripts/patch_editor_mode.py <exe>` vertauscht die Zieladressen der Befehle bei
`0x00404E66`/`0x00404E70` (2 Bytes) → Editor = 1, FullScreen = 0. Angewendet nur im
Test-Prefix (`wineprefix-test`); der Haupt-Prefix bleibt original.
Start ohne virtuellen Desktop (Fenster mit Menü ist größer als 640×480):
```sh
WINEPREFIX=$PWD/wineprefix-test wine 'C:\Program Files (x86)\neoSoftware\Rent-A-Hero\Rent-A-Hero.exe'
```
Unter Wine geprüft: Fenster 640×480 mit Menüleiste erscheint.

## Menü `AppMenu` (Befehls-IDs aus der Ressource)

| Menü | ID | Eintrag |
|---|---:|---|
| Datei | 40044 | Neue Szene… |
| | 40030 | Öffnen Szene… |
| | 40042 / 40070 | Speichern Szene / Speichern Szene als… |
| | 40065 | Compile Szene (erzeugt C++-Gerüst, siehe BINARY.md) |
| | 40040 / 40061 | Öffnen Hintergrund / Flipping Hintergrund |
| | 40046 / 40035 / 40043 | Öffnen Video / Sprite / Sprite-Anim |
| | 40050 / 40056 | Lade Hero / Hero SFX |
| | 40073 / 40074 | Lade Lady / Lady SFX |
| | 40048 / 40049 | Öffnen Tiefeninfo / Wegmaske |
| | 40006 | Öffnen Vollbild-Anim |
| | 4 | Ende (Alt+F4) |
| Objekt | 40036 | Animieren |
| | 40064 | Actor – RGB-Add (F2) |
| | 40075 / 40076 | Add / Remove Inventoryitem (Ctrl+A / Ctrl+R) |
| Optionen | 40080 / 40033 / 40019 | Fullscreen / TrueColor (nur FS) / Realtime |
| | 40090 / 40091 | Subtitles ON / OFF (Ctrl+F11 / Ctrl+F12) |
| Sound | 40063 | Sound Control… |
| | 40047 / 40054 | Lade Music (autoplay) / Lade SFX |
| | 40051 / 40055 / 40052 / 40053 | Play / Loop / Stop Sound, Set Sound Volume |
| Areas | 40057 / 40058 / 40060 / 40059 | Set Area, Show Area Rects, Show Area Text, Delete Area |
| Skripts | 40068 | Start Skript (Ctrl+S) |
| | 40079 / 40092 | Start / Quit Game Menu |
| | 40071 / 40072 | Game Save… / Game Load… |
| Hilfe | 40045 | Tastenbelegung (F1) |
| | 40077 / 40078 | Hint On / Off |

Folgerungen für den Port: Die Begriffe „Hero“ und „Lady“ (zweite spielbare Figur?),
„Tiefeninfo“ (`*_DEPTH.ZBF`), „Wegmaske“ (vermutlich `*_PATH.ZBF`), „Flipping
Hintergrund“ und „RGB-Add“ benennen Engine-Funktionen, die in Phase 2/3 zu belegen sind.
Auf den CDs gibt es 59 `*_DEPTH.ZBF` und 60 `*_PATH.ZBF`; dass „Wegmaske“ die `_PATH.ZBF` lädt, ist naheliegend, aber noch UNVERIFIED.
