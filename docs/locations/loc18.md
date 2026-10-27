# Loc18 (automatische Übersicht)

Funktionen `0x0041E4F0`–`0x0041F06F` (9 Stück), Ereignis-Handler `0x0041EC80`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S61_To60` | `0x0041E960` |
| 2 | `S61_Brain` | `0x0041E960` |
| 3 | `S61_To60_2` | `0x0041E960` |
| 4 | `Gegenstand 6` | `0x0041E960` |
| 5 | `Gegenstand 8` | `0x0041E960` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041ec80.c FUN_0041ec80 @ 0041ec80
  Anfang: ruft FUN_0041ec80
  case 1: S? 3FE0 | ruft CActor_WalkTo CSzene_DisableInput FUN_0041ec40 FUN_0041ebc0
  == 1: Sprache 061_ROD_04 | ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D
  == 2: Sprache 061_ROD_05 061_ROD_06 | ruft CGame_WalkAndComment CGame_Random CAudioBuffer_SetVolume CEvent_SetTimerHandler
  case 3: Sprache 061_ROD_02 061_ROD_03 | → 17/60 | S= 3FE4=1 | S? 3FE0 | ruft CGame_WalkAndComment CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayEndEvent
  case 4: Sprache 061_ROD_08 061_ROD_09 | S= 3FE0=1 | Inv Remove(6) | ruft CSzene_DisableInput CGame_WalkAndComment CSzene_SetModal CAnimLayer_PlayEndEvent FUN_0041eb20 CGame_Delay FUN_00440400 CSzene_EnableInput
  case 5: Sprache 061_ROD_10 | ruft FUN_004409c0
  case 6: ruft CAnimLayer_PlayEndEvent FUN_0041ebc0
  == 0: ruft FUN_0041ec40
  case 14: ruft CGame_Random
```
