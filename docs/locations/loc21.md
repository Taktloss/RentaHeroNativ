# Loc21 (automatische Übersicht)

Funktionen `0x00422D90`–`0x00425CEF` (5 Stück), Ereignis-Handler `0x00424600`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 10 | `S71_To69` | `0x00423880` |
| 11 | `S71_To74` | `0x00423880` |
| 12 | `S71_Wall` | `0x00423880` |
| 13 | `S71_Shovel (Ebene)` | `0x00423880` |
| 20 | `S72_To73` | `0x00423880` |
| 21 | `S72_To74` | `0x00423880` |
| 22 | `S72_Wall` | `0x00423880` |
| 30 | `S73_To72` | `0x00423880` |
| 31 | `S73_To75` | `0x00423880` |
| 32 | `S73_Wall` | `0x00423880` |
| 33 | `S73_Drill (Ebene)` | `0x00423880` |
| 34 | `S73_Hebel (Ebene)` | `0x00423880` |
| 35 | `Gegenstand 23` | `0x00423880` |
| 40 | `S74_To71` | `0x00423880` |
| 41 | `S74_To72` | `0x00423880` |
| 42 | `S74_To75` | `0x00423880` |
| 43 | `S74_To77` | `0x00423880` |
| 44 | `S74_Wall` | `0x00423880` |
| 45 | `S74_Hebel (Ebene)` | `0x00423880` |
| 46 | `Gegenstand 11` | `0x00423880` |
| 50 | `S75_To73` | `0x00423880` |
| 51 | `S75_To74` | `0x00423880` |
| 52 | `S75_To76` | `0x00423880` |
| 53 | `S75_Wall` | `0x00423880` |
| 60 | `S76_To75` | `0x00423880` |
| 61 | `S76_Wall` | `0x00423880` |
| 62 | `S76_DriveLever` | `0x00423880` |
| 63 | `S76_DrillLever` | `0x00423880` |
| 64 | `S76_Zunder (Ebene)` | `0x00423880` `0x00424600` |
| 70 | `S77_To74` | `0x00423880` |
| 71 | `S77_To78` | `0x00423880` |
| 72 | `S77_To80` | `0x00423880` |
| 73 | `S77_Wall` | `0x00423880` |
| 80 | `S78_To77` | `0x00423880` |
| 81 | `S78_To79` | `0x00423880` |
| 82 | `S78_Wall` | `0x00423880` |
| 90 | `S79_To78` | `0x00423880` |
| 91 | `S79_To81` | `0x00423880` `0x00424600` |
| 92 | `S79_Wall` | `0x00423880` |
| 93 | `S79_DriveLever` | `0x00423880` |
| 94 | `S79_DrillLever` | `0x00423880` |
| 100 | `S80_To77` | `0x00423880` |
| 101 | `S80_Wall` | `0x00423880` |
| 102 | `S80_PickAxe (Ebene)` | `0x00423880` |
| 103 | `S80_GloomOnFloor (Ebene)` | `0x00423880` `0x00424600` |
| 104 | `Gegenstand 9` | `0x00423880` `0x00424600` |
| 105 | `Gegenstand 11` | `0x00423880` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00424600.c FUN_00424600 @ 00424600
  Anfang: ruft FUN_00424600
  case 10: → 19/69 | ruft CActor_WalkTo
  case 12: Sprache 071_ROD_02 | ruft CGame_WalkAndComment
  case 13: Sprache 071_ROD_01 | Inv Add(11) | ruft CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 22: Sprache 072_ROD_01 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 32: Sprache 073_ROD_01 | ruft CGame_WalkAndComment
  case 33: Sprache 073_ROD_03 073_ROD_02 | S? 4038 | ruft CGame_WalkAndComment
  case 34: Sprache 073_ROD_05 076_ROD_04 | S= 4034=2 | S? 4038 403C | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0 CSzene_DisableInput CLayer_Disable CAnimLayer_Start CGame_Delay CSzene_EnableInput CAnimLayer_PlayRange CLayer_Enable
  case 35: Sprache 073_ROD_02 | S= 4038=1 | Inv Remove(23) | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CAnimLayer_Start CActor_Show FUN_004409c0
  case 40: → 21/71 | ruft CActor_WalkTo
  case 41: → 21/72 | ruft CActor_WalkTo
  case 44: Sprache 074_ROD_02 | ruft CGame_WalkAndComment
  case 45: Sprache 074_ROD_01 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 46: Sprache 074_ROD_03 074_ROD_04 | S= 403C=1 403C=0 4040=2 | S? 403C 4040 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CAnimLayer_Start FUN_00442f90 CActor_Show FUN_004409c0
  case 50: → 21/73 | ruft CActor_WalkTo
  case 52: → 21/76 | ruft CActor_WalkTo
  case 53: Sprache 075_ROD_01 | ruft CGame_WalkAndComment
  case 60: → 21/75 | ruft CActor_WalkTo
  case 61: Sprache 076_ROD_01 | ruft CGame_WalkAndComment
  case 62: S= 4034=1 4034=3 | S? 403C | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CSzene_GetArea CAnimLayer_Start CSzene_EnableInput CActor_WalkTo2
  case 63: Sprache 076_ROD_02 | S? 4044 | ruft CSzene_DisableInput FUN_00440400 CActor_WalkTo CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CAnimLayer_Start CActor_WalkTo2 CGame_Delay
  == 1: Sprache 079_ROD_03 080_ROD_03 | S= 4044=2 404C=2 4058=2 | ruft FUN_00442f90 CAnimLayer_PlayRange CGame_Delay FUN_004409c0 CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show
  == 2: Sprache 076_ROD_03 076_ROD_04 079_ROD_05 080_ROD_04 | S= 4048=1 4044=3 404C=3 | ruft FUN_00442f90 CAnimLayer_PlayRange CArea_SetEvent CLayer_Enable CGame_Delay FUN_004409c0 CSzene_EnableInput CSzene_GetArea CActor_UnblockRect CGame_WalkAndComment
  case 64: Sprache 069_ROD_26 | S= 4048=0 | Inv Add(26) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CSzene_EnableInput FUN_004409c0
  case 70: → 21/74 | ruft CActor_WalkTo
  case 71: → 21/78 | ruft CActor_WalkTo
  case 72: → 21/80 | ruft CActor_WalkTo
  case 73: Sprache 077_ROD_01 | ruft CGame_WalkAndComment
  case 81: → 21/79 | ruft CActor_WalkTo
  case 82: Sprache 078_ROD_01 | ruft CGame_WalkAndComment
  case 90: → 21/78 | ruft CActor_WalkTo
  case 91: → 22/81 | ruft CSzene_DisableInput CActor_WalkTo
  case 92: Sprache 079_ROD_01 | ruft CGame_WalkAndComment
  case 93: Sprache 079_ROD_06 | S= 4034=2 | S? 403C | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CAnimLayer_PlayRange CGame_Delay CSzene_EnableInput FUN_004409c0 CSzene_GetArea CAnimLayer_Start
  case 94: Sprache 079_ROD_02 | S? 404C | ruft CSzene_DisableInput FUN_00440400 CActor_WalkTo CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CAnimLayer_Start CGame_Delay
  == 3: Sprache 079_ROD_06 | ruft FUN_004409c0 CSzene_EnableInput
  case 100: → 21/77 | ruft CActor_WalkTo
  case 101: Sprache 080_ROD_02 080_ROD_01 | S= 405C=1 | S? 4054 | ruft CGame_WalkAndComment CActor_WalkTo CSzene_DisableInput CGame_Delay FUN_00435ba0 CSzene_EnableInput FUN_004409c0
  case 102: Sprache 080_ROD_07 | Inv Add(9) | ruft CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 103: S? 4058
  case 104: Sprache 080_ROD_06 | S= 4054=2 | S? 4054 405C | ruft CGame_DescribeItem CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CLayer_Enable CArea_SetEvent CActor_Show FUN_004409c0
  case 105: Sprache 080_ROD_05 | S= 4054=3 | S? 4054 | Inv Add(23) | ruft CGame_DescribeItem CActor_WalkTo CActor_Hide CSzene_DisableInput CAnimLayer_PlayRange CLayer_Disable CActor_Show FUN_004409c0 CSzene_EnableInput
  case 200: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
```
