# SZN – Szenenbeschreibung

Textdatei im Windows-INI-Format (CRLF, cp1252), eine pro Szene (`GAME/LOCnn/<Szene>.SZN`,
77 Dateien). Gelesen von `FUN_00449380` (Methode des Szenen-/View-Objekts,
`__thiscall`, Parameter: Dateiname, `LoadMask`) ausschließlich über
`GetPrivateProfileStringA`/`GetPrivateProfileIntA`.

Konsequenzen für den Port:
- **Schlüssel und Sektionen sind case-insensitive** (Win32-INI-API). Die Dateien nutzen z. B.
  `ZPos`, der Code fragt `Zpos` ab.
- Fehlender String-Wert ⇔ Standardwert `"NULL"` (`0x004707D4`); der Code vergleicht mit
  `lstrcmpA(.., "NULL")`. Fehlender Integer ⇔ angegebener Standardwert (siehe Tabellen).
- Werte mit führenden Nullen (`fps=010`) sind dezimal (GetPrivateProfileInt).
- Dateinamen werden über `FUN_00448a70` (Pfadauflösung, noch zu analysieren) zu vollen
  Pfaden ergänzt.

## `[GENERAL]`

| Schlüssel | Bedeutung |
|---|---|
| `Background1` | Name der Hintergrund-Sektion. Nur `Background1` wird gelesen. |
| `Layer1` … `Layer40` | Namen der Ebenen-Sektionen. Schleife bricht beim ersten fehlenden ab (max. 40). |
| `Area1` … `Area60` | Namen der Klickbereich-Sektionen. Abbruch beim ersten fehlenden (max. 60). |
| `Character1`, `Character2` | Namen der Figuren-Sektionen (z. B. `RodrigoSmall`); beide optional. |

## Hintergrund-Sektion

| Schlüssel | Typ | Default | Verwendung |
|---|---|---|---|
| `Filename` | String | – | Hintergrundbild, geladen über `FUN_00449e70` |
| `zBuf` | String | NULL | Tiefenpuffer (`*_Depth.zbf`), `FUN_0044a1b0` |
| `wBuf` | String | NULL | Wegmaske (`*_Path.zbf`), `FUN_0044a260` |
| `fps` | Int | 0 | gespeichert bei Hintergrundobjekt `+0x54` |
Sektionsname wird bei `+0x15C` des Hintergrundobjekts abgelegt.

## Ebenen-Sektion (`LayerN`)

| Schlüssel | Typ | Default | Verwendung |
|---|---|---|---|
| `LoadCond` | Int (Bitmaske) | 0 | Ebene wird nur geladen, wenn `LoadCond == 0` oder `(LoadCond & LoadMask) == LoadCond`. In den Daten: 1, 2, 4. |
| `Filename` | String | – | `.bmp`/`.a16`/`.TCC`/`.avi`; erzeugt über `FUN_0044ab10(name, fps != 0)` |
| `X`, `Y` | Int | 0 | Position; Y wird um den Szenen-Offset `this+0x2F0` verschoben; Setzen über vtable `+0x14` des Ebenenobjekts |
| `fps` | Int | 0 | Animationsrate; `fps != 0` ⇒ animiert. Häufig: 10, 0, 20, 5 |
| `Z` | Int | **1** | Zeichenreihenfolge, als `double` bei Ebene `+0x20` |
| `ZBuffered` | Int | **1** | Ebene wird gegen Tiefenpuffer geprüft (`+0x40`); 29× auf 0 gesetzt |
| `DirtyAlpha` | Int | 0 | setzt Flag `+0x19C` und am Unterobjekt `+0x37C`, ruft vtable `+0x24` (Bedeutung offen) |
| `ZoomVal` | Int | 100 | Skalierung in Prozent; nur ≠100 wird angewendet (`FUN_00447030`) |
Zusätzlich führt der Lader Min/Max-Werte bei `this+0x2F8`/`+0x2FC` und einen Zähler pro
Wert in einem `short`-Array ab `this+0x300` (über `__ftol` aus double-Werten; welche
genau, ist UNVERIFIED – vermutlich Z).

## Klickbereich-Sektion (`AreaN`)

| Schlüssel | Typ | Default |
|---|---|---|
| `X1`, `X2`, `Y1`, `Y2` | Int | 0 → `RECT{left=X1, top=Y1, right=X2, bottom=Y2}` |
| `CursorType` | Int | 0 → Mauszeiger über dem Bereich; in den Daten 1–9 |
Registriert über `FUN_0044add0(name, rect, cursorType)`; diese Funktion liest
zusätzlich `AREA_TEXT` aus einer Sprachdatei `Loc%02d` (siehe Texte, noch offen).

## Figuren-Sektion (`Character1/2`)

| Schlüssel | Typ | Default | Verwendung |
|---|---|---|---|
| `Filename` | String | – | Figurenanimation (`.a16`), `FUN_0044a770` |
| `Type` | Int | 0 | Figurentyp; existiert bereits eine Figur mit anderem Typ (`+0x6BB*4`), wird sie zerstört und neu erzeugt. In den Daten: 1, 2, 12 |
| `SFXLeft`, `SFXRight`, `SFXAlt` | String | NULL | Schrittgeräusche; nur wenn `SFXLeft` gesetzt, `FUN_0044a8f0` |
| `PerCor` | Int | 100 | bei Figur `+0x16EC` (Perspektivkorrektur?, UNVERIFIED); kommt in keiner Datei vor |
| `RedAdd`, `GreenAdd`, `BlueAdd` | Int | 0 | Farbaddition (Tönung), `FUN_004473e0`; Werte auch negativ |
| `RedAddN`, `GreenAddN`, `BlueAddN`, `ColorAddXN`, `ColorAddYN`, `ColorAddRangeN` | Int | 0 | N = 1…10: ortsabhängige Tönung; `ColorAddRangeN ≠ 0` setzt Flag `+0x16D4` |
| `X`, `Y` | Int | 0 | Startposition, vtable `+0x14` |
| `Zpos` | Int | 0 | Tiefe (`+0x16D8`); daraus `+0x16E0` (double) über Perspektivparameter |
| `ZBuffered` | Int | 1 | `+0x40` |
| `ZoomVal` | Int | **0** | `+0x16E8` (hier Default 0, nicht 100!) |
| `Direction` | Int | 0 | Blickrichtung 0–7 (`+0x15CC`), `FUN_00435b80` |

Hinweis: Ghidra verwechselt in diesem Block Stack-Variablen (`local_5c4`,
`unaff_EBX`); dass alle Schlüssel aus der Figuren-Sektion gelesen werden, ist durch
die Dateien gedeckt, die exakten Sektionsargumente sind am Disassembly noch zu prüfen
(UNVERIFIED). Ebenso die Herkunft der Perspektivparameter für `+0x16E0`.

## `[COMPILER]`
`Loc`, `Szn`, `VideoN`, `SoundN` – wird vom Lader **nicht** gelesen, nur vom Editor-
Codegenerator (`FUN_00401a00`). Für das Spiel ohne Bedeutung.

## Abschluss
Nach dem Laden: Objekt `+0x00` = 1 (geladen), globales `DAT_00481f28` = 1;
falls `this+0x5B4` gesetzt, `FUN_00441f50` (noch offen).
