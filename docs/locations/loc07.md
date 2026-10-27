# Loc07 (automatische Übersicht)

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
