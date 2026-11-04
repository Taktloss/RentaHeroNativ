# Hotspots anzeigen (Port)

Nutzerwunsch 2026-11-04. Das ist eine Port-Hilfe, das Original kennt sie nicht. **Solange Tab
gedrückt ist,** werden alle anklickbaren Stellen der aktuellen Szene markiert und mit ihrem Text
beschriftet. Das ist derselbe Text, den das Spiel unter dem Mauszeiger zeigt (`CArea +0x42`,
MOUSE.md).

- **Tab ist im Original frei:** `WM_KEYDOWN` geht an `0x0044CE50`. Dort sind nur Esc und F1–F12
  belegt, der Rest geht an `0x0044CF00`. Diese Funktion wertet nur die Leertaste und die Tasten
  ab 0x61 aus, und auch die nur mit gewählter Editor-Figur (`+0x5D8`/`+0x5DC`, am Disassembly
  geprüft). Der Port gibt Tab deshalb nicht an das Spiel weiter.
- **Was markiert wird:** Die Szene wird im 4×4-Raster mit derselben Trefferprüfung abgetastet, die
  auch Klicks auswerten: `CSzene_HitTest` `0x0044C3C0`, also mit Ebenen, Masken, `HitGrid` und
  nur aktiven Bereichen. Markiert wird genau, was im Augenblick anklickbar ist. Ausnahmen:
  - Die Figuren selbst (Rodrigo, zweite Figur) werden nicht markiert.
  - Die Inventarleiste wird nicht markiert.
  - Bei offenem Spielmenü wird nichts markiert.
- **Darstellung:**
  - Gezeichnet wird als ImGui-Zeichenliste über dem Fenster: halbtransparente Zellen, Rahmen und
    Beschriftung in der Mitte.
  - Das 640×480-Spielbild, `--dump` und die Spiellogik bleiben unverändert.
  - Mit CRT-Geom (gewölbtes Bild) liegen die Markierungen am Rand leicht daneben, weil die
    Wölbung nicht nachgerechnet wird.
- **Einstellung:**
  - Startmenü → Optionen → Spiel: „Hotspots anzeigen, solange Tab gedrückt ist“. In
    `launcher.ini` heißt sie `HotspotKey`, Standard 1, weil Tab im Original nichts tut.
  - `--show-hotspots` zeigt die Markierungen immer. Das dient der Prüfung mit `--shot`.
  - `--sdl-key-at <bild> tab` bzw. `up:tab` simuliert Drücken und Loslassen.

Geprüft (2026-11-04), Loc10/Szene 38:
- Wald, Pilz, beide Wege und das Mädchen werden markiert, das Mädchen in seiner Maskenform.
- Tab über die SDL-Ereignisschlange: Mit gedrückter Taste erscheinen die Markierungen, nach dem
  Loslassen sind sie weg.
- Tests: `launcher_settings_roundtrip` mit `HotspotKey`.
