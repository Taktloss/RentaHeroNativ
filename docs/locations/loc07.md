# Loc07 – Kanalisation unter Smashville, Szenen 47–56

Funktionen `0x0040F7B0`–`0x0041163F` (5 Stück), Ereignis-Handler `0x00410870`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 30 | `047_DoorTo50` | `0x0040FAD0` |
| 31 | `047_DoorTo51` | `0x0040FAD0` |
| 32 | `047_DoorTo49` | `0x0040FAD0` |
| 33 | `047_DeadEnd` | `0x0040FAD0` |
| 34 | `047_Ladder` | `0x0040FAD0` |
| 40 | `048_DoorTo50` | `0x0040FAD0` |
| 41 | `048_DeadEnd` | `0x0040FAD0` |
| 45 | `049_DoorTo52` | `0x0040FAD0` |
| 46 | `049_DoorTo47` | `0x0040FAD0` |
| 50 | `050_DoorTo54` | `0x0040FAD0` |
| 51 | `050_DoorTo48` | `0x0040FAD0` |
| 52 | `050_DeadEnd` | `0x0040FAD0` |
| 53 | `050_DoorTo47` | `0x0040FAD0` |
| 60 | `051_DoorTo54` | `0x0040FAD0` |
| 61 | `051_DoorTo47` | `0x0040FAD0` |
| 62 | `051_DoorTo52` | `0x0040FAD0` |
| 63 | `051_DoorTo55` | `0x0040FAD0` |
| 70 | `052_DoorTo56` | `0x0040FAD0` |
| 71 | `052_DoorTo51` | `0x0040FAD0` |
| 72 | `052_DoorTo49` | `0x0040FAD0` |
| 73 | `052_DoorTo53` | `0x0040FAD0` |
| 74 | `52_Kurbel (Ebene)` | `0x0040FAD0` |
| 75 | `52_Sack (Ebene)` | `0x0040FAD0` |
| 76 | `Gegenstand 10` | `0x0040FAD0` |
| 80 | `053_DoorTo52` | `0x0040FAD0` |
| 81 | `053_DoorTo57` | `0x0040FAD0` |
| 90 | `054_DoorTo50` | `0x0040FAD0` |
| 91 | `054_DoorTo51` | `0x0040FAD0` |
| 100 | `055_DoorTo51` | `0x0040FAD0` |
| 110 | `056_DoorTo52` | `0x0040FAD0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00410870.c FUN_00410870 @ 00410870
  Anfang: ruft FUN_00410870
  case 1: ruft CGame_Random CAudioBuffer_SetFreq CEvent_SetTimerHandler
  case 34: → 1/46 | ruft CActor_WalkTo2 CActor_Hide CSzene_GetObject CAnimLayer_PlayEndEvent CAnimLayer_Start FUN_004475b0
  case 40: → 7/50 | ruft CActor_WalkTo
  case 51: → 7/48 | ruft CActor_WalkTo
  case 52: Sprache 000_ROD_01 | ruft CActor_WalkTo CGame_HeroSayWait
  case 60: → 7/54 | ruft CActor_WalkTo
  case 61: → 7/47 | ruft CActor_WalkTo
  case 63: → 7/55 | ruft CActor_WalkTo
  case 70: → 7/56 | ruft CActor_WalkTo
  case 71: → 7/51 | ruft CActor_WalkTo
  case 72: → 7/49 | ruft CActor_WalkTo
  case 73: Sprache 052_ROD_05 | → 7/53 | S? 3F04 | ruft CActor_WalkTo CActor_Hide CSzene_GetObject CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait
  case 74: ruft CActor_WalkTo
  == 0: Sprache 052_ROD_02 052_ROD_01 052_ROD_06 | S= 3EFC=0 3F30=1 3F04=0 | ruft CActor_Hide CSzene_GetObject CAnimLayer_PlayEndEvent CActor_Show CActor_WalkTo CGame_HeroSayWait FUN_004475b0 CLayer_Disable CAnimLayer_Start CAnimLayer_PlayRange CActor_UnblockRect
  case 75: Sprache 052_ROD_03 | S= 3EF8=0 | S? 3EF8 | ruft CActor_WalkTo CActor_Hide CSzene_GetObject CAnimLayer_PlayRange FUN_004475b0 CActor_Show CGame_HeroSayWait
  case 76: Sprache 052_ROD_04 | S= 3F00=1 3EFC=1 3EF4=2 | Inv Remove(10) | ruft CActor_WalkTo CActor_Hide CSzene_GetObject CAnimLayer_PlayRange CLayer_Enable CActor_Show CGame_HeroSayWait
  case 80: Sprache 053_ROD_01 | S? 3F04 | ruft CGame_WalkAndComment
  case 81: → 15/57 | ruft CActor_WalkTo
  case 90: → 7/50 | ruft CActor_WalkTo
  case 100: → 7/51 | ruft CActor_WalkTo
  case 110: → 7/52 | ruft CActor_WalkTo
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc07.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3EF4` | 0 | Ankünfte über die Leiter (0 → `047_ROD_01`, 1 → `047_ROD_02`); nach dem Seil 2 |
| `0x3EF8` | 1 | Sack liegt in Szene 52 |
| `0x3EFC` | 1 | Seil hängt an der Kurbel (anfangs das alte) |
| `0x3F00` | 0 | neues Seil (Gegenstand 10) befestigt |
| `0x3F04` | 1 | Gitter in 52/53 unten (Loc15 setzt es beim Einstieg mit Gleiter/Nacht wieder) |
| `0x3F08` | 0 | Pirat in Szene 51 gesehen |
| `0x3F30` | 0 | altes Seil gerissen → Mädchen in Loc10 springt Seil |
| `0x3FC8` | 0 | UNVERIFIED (Loc16/17); ≠ 0 → kein Pirat in 51 |

### Konstruktor `0x0040F7B0`
- Szene 52: `CSzene_Load("52", Maske)` mit Bit 1 = Sack, Bit 2 = Gitter unten; Kurbel, Gitter,
  Seil, Sack sind statische `.TCC`-Ebenen, `52_SeilReisst` eine Animation.
- Andere Szenen: `wsprintf("%d")` + `CSzene_LoadAll`.
- Töne: Blubbern (Vol. 50), Kurbel, Gitter fällt, Gummiband, Seil festmachen, Streicher,
  Klavier; Musik `Loc07_Sewage`, Hall (0x15, 0.652, 2.886, 0.25).

### Start `0x0040FAD0`
- Je Szene `NN_BackAnim` (Modus 13) und Bereiche. Szene 49 startet `48_BackAnim` (Original).
- 47 aus 46: Rodrigo rutscht die Leiter herunter (`47_RodRunter` 80 px in 30 Schritten,
  `47_RodRunterStop`), Kommentar je `0x3EF4`.
- 51 aus 54: einmal schaut ein Pirat herein (`0x3F08`, nicht bei `0x3FC8`).
- 52: Kurbel nur bei Gitter unten anklickbar; ohne Seil Gegenstand 10 → Ereignis `0x4C`;
  Gitter oben bei (507,−40), sonst Sperrrechteck vor dem Bogen.
- 53: Gitter oben bei (0,−100), sonst Sperrrechteck.
- Blubbern alle 5–7 s, danach 5–10 s mit Frequenz 50–100 %.

### Ereignisse `0x00410870`
- Türen: hingehen, `CGame_ChangeScene(ziel, 7)`; Sackgassen `000_ROD_01`; Leiter (`0x22`) → 1/46;
  53 → 15/57 (Gitter offen), sonst `053_ROD_01`; 52 → 53 nur bei offenem Gitter, sonst rütteln.
- Kurbel (`0x4A`): ohne Seil dreht sie leer (`052_ROD_02`); altes Seil reißt (`0x3EFC = 0`,
  `0x3F30 = 1`, `052_ROD_01`); mit neuem Seil geht das Gitter hoch (`0x3F04 = 0`, `052_ROD_06`).
- Sack (`0x4B`): ins Wasser stoßen, `0x3EF8 = 0`, `052_ROD_03`.
- Gegenstand 10 (`0x4C`): Seil festmachen, `0x3F00 = 0x3EFC = 1`, `0x3EF4 = 2`, `052_ROD_04`.
- Unbekannt: `MessageBoxA("Unknown_Event____")` – Port: Meldung auf stderr.

- Probelauf 2026-11-03: 47 (Leiter), 49, 52, 53; Kurbel mit altem Seil (reißt) und neuem Seil
  (Gitter hoch), Weg 52 → 53 → 15/57. Offen: Seil festmachen, Sack, Pirat in 51.
