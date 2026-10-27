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
| `zBuf` | String | NULL | Tiefenkarte (`*_Depth.zbf`) → `CSzene+0x2BC`, siehe zbf.md |
| `wBuf` | String | NULL | Wegkarte (`*_Path.zbf`) → `CSzene+0x2C0`, siehe zbf.md |
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
| `DirtyAlpha` | Int | 0 | setzt Flag `+0x19C` und am Bild `+0x37C`; Bild berechnet dann das Rechteck mit Alpha ≠ 0 (`CImage_CalcAlphaBounds`, tcc.md) |
| `ZoomVal` | Int | 100 | Skalierung in Prozent; nur ≠100 wird angewendet: `layer+0x28 = (double)ZoomVal`, `FUN_00447060` |
Zusätzlich führt der Lader Max/Min-Z bei `this+0x2F8`/`+0x2FC` und einen Zähler pro
Z-Wert im `short`-Array ab `this+0x300`: `fld [layer+0x20]` (Z als double) → `__ftol`
→ `++count[z]` (Disassembly `0x00449749`–`0x00449751`). Grundlage der Z-Reihenfolge in
`CSzene_HitTest` und `CSzene_Update`.

## Klickbereich-Sektion (`AreaN`)

| Schlüssel | Typ | Default |
|---|---|---|
| `X1`, `X2`, `Y1`, `Y2` | Int | 0 → `RECT{left=X1, top=Y1, right=X2, bottom=Y2}` |
| `CursorType` | Int | 0 → Mauszeiger über dem Bereich; in den Daten 1–9 |
Registriert über `CSzene_AddArea(name, rect, cursorType)`; diese Funktion liest
zusätzlich den Anzeigetext aus `[AREA_TEXT]` der `LocNN.GER` (ger.md).

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
| `Zpos` | Int | 0 | Tiefe (`+0x16D8`); daraus Skala `+0x16E0` (siehe unten) |
| `ZBuffered` | Int | 1 | `+0x40` |
| `ZoomVal` | Int | **0** | `+0x16E8` (hier Default 0, nicht 100!) |
| `Direction` | Int | 0 | Blickrichtung 0–7 (`+0x15CC`), `FUN_00435b80` |

Belegt an der Disassembly (`0x0044986E`–`0x00449CCC`): Der Wert von
`[GENERAL] Character1`/`Character2` ist der **Name der Sektion**, aus der alle obigen
Schlüssel gelesen werden (ein Puffer für alle Aufrufe). Ablauf je Figur:
1. `Filename`, `Type`; vorhandene Figur mit anderem `Type` (`+0x1AEC`) löschen.
2. Datei per `CSzene_FindPath` suchen, `FUN_0044a770` (Figur anlegen/laden).
3. `SFXLeft` gesetzt → `SFXRight`, `SFXAlt` lesen, alle drei suchen, `FUN_0044a8f0`.
4. `FUN_00433e30(Type)`, `PerCor` → `+0x16EC`, `RedAdd/GreenAdd/BlueAdd` → `FUN_004473e0(r, g, b)`.
5. N = 1…10 (`%1d`, also auch `RedAdd10`): `RedAddN` `+0x15E4+4(N−1)`, `GreenAddN` `+0x160C…`,
   `BlueAddN` `+0x1634…`, `ColorAddXN` `+0x165C…`, `ColorAddYN` `+0x1684…`,
   `ColorAddRangeN` `+0x16AC…` (≠ 0 → `+0x16D4 = 1`).
6. `X`, `Y`, `Zpos` → `+0x16D8`, `ZBuffered` → `+0x40`.
7. **Nur wenn eine Wegkarte geladen ist** (`CSzene+0x2C0`, zbf.md):
   `+0x16E0 = (double)(Zpos − vMin) * k + sMin` (Werte der Wegkarte, x87-Rechnung).
   Ohne Wegkarte bleibt `+0x16E0` unverändert.
8. `ZoomVal` (Vorgabe 0) → `+0x16E8`; Sektionsname nach `+0x15C` kopiert;
   `SetPos(X, Y)` (Slot `+0x14`), Slot `+0x24(0)`; `Direction` → `+0x15CC`, `FUN_00435b80`.

## `[COMPILER]`
`Loc`, `Szn`, `VideoN`, `SoundN` – wird vom Lader **nicht** gelesen, nur vom Editor-
Codegenerator (`FUN_00401a00`). Für das Spiel ohne Bedeutung.

## Abschluss
Nach dem Laden: Objekt `+0x00` = 1 (geladen), globales `DAT_00481f28` = 1;
falls `this+0x5B4` (Inventarleiste) gesetzt, `FUN_00441f50`: setzt dort `+0x408 = 1`
(Neuzeichnen anfordern).
