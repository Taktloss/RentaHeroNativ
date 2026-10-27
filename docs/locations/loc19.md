# Loc19 (automatische Übersicht)

Funktionen `0x0041F070`–`0x004223DF` (8 Stück), Ereignis-Handler `0x004202D0`, `0x00420500`, `0x00420D60`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S69_To71` | `0x0041F960` |
| 2 | `S69_To70` | `0x0041F960` |
| 3 | `S69_To68` | `0x0041F960` |
| 4 | `S69_Zwerg1 (Ebene)` | `0x0041F960` |
| 5 | `S69_Wache (Ebene)` | `0x0041F960` |
| 7 | `S69_JasWalkKette (Ebene)` | `0x0041F960` |
| 7 | `this+0x58 (Ebene)` | `0x0041F960` |
| 8 | `S69_RamBackTalk (Ebene)` | `0x0041F960` |
| 10 | `S68_To67` | `0x0041F960` |
| 11 | `S101_To69` | `0x0041F960` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 004202d0.c FUN_004202d0 @ 004202d0
  Anfang: ruft FUN_004202d0
  == 10: Sprache 068_JAS_01 | → 10/38 | S? 3F28 | ruft CActor_WalkTo2 FUN_004403e0 CActor_WalkTo
  == 11: → 19/69 | S? 3F28 | ruft CActor_WalkTo CActor_WalkToClick CSzene_DisableInput CGame_Random
  case 1: ruft CGame_Random CAudioBuffer_SetVolume
  case 2: ruft CGame_Random CAudioBuffer_SetVolume
  case 3: ruft CGame_Random CAudioBuffer_SetVolume
  case 4: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
# 00420500.c FUN_00420500 @ 00420500
  Anfang: ruft FUN_00420500
  == 10: → 10/38 | ruft CActor_WalkTo
  == 11: Sprache 101_ROD_02 | ruft CGame_WalkAndComment CGame_Random
  == 1: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
# 00420d60.c FUN_00420d60 @ 00420d60
  Anfang: ruft FUN_00420d60
  case 1: Sprache 069_RAM_23 069_JAS_23 069_JAS_20 069_ROD_24 069_ROD_17 069_JAS_17 | → 21/71 | S= 401C=1 3FFC=1 4004=2 4008=2 | S? 3F48 4004 | ruft CActor_WalkTo CSzene_DisableInput CActor_WalkTo2 CLayer_Enable FUN_004474c0 FUN_00442f90 FUN_004475b0 FUN_00443310 CActor_Hide CLayer_Disable FUN_00420600 CAnimLayer_PlayRange CGame_Delay CActor_Show CActor_BlockRect CSzene_EnableInput CAnimLayer_PlayEndEvent FUN_004403e0 CAnimLayer_ShowFrame CGame_Random CAudioBuffer_SetVolume
  case 2: Sprache 069_ROD_18 069_JAS_18 069_ROD_11 069_RAM_11 069_JAS_11 | → 20/70 | S= 4008=3 4018=1 4014=0 400C=3 | S? 3FF0 | ruft CActor_WalkTo2 CSzene_DisableInput CAnimLayer_PlayEndEvent FUN_004403e0 FUN_00420600 CSzene_EnableInput CActor_WalkTo CGame_Speak CAnimLayer_PlayRange CLayer_Disable CActor_Show CActor_SetPos CActor_UnblockRect CActor_BlockRect CActor_Hide CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 069_JAS_03 069_ROD_04 069_THA_04 069_ZW2_04 069_ROD_05 069_THA_05 069_ROD_19 069_JAS_19 069_ROD_12 069_RAM_12 | → 20/70 19/68 | S= 3FF0=1 3FF4=0 401C=0xffffffff 4008=4 400C=4 | S? 401C | ruft CActor_WalkTo2 CGame_Speak FUN_004403e0 CSzene_DisableInput CLayer_Enable FUN_004474c0 CAnimLayer_PlayEndEvent FUN_00420600 CLayer_Disable CAnimLayer_PlayRange CActor_UnblockRect CSzene_EnableInput CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 4: Sprache 069_ROD_06 069_ZW1_06 069_RAM_06 069_RAM_07 069_ROD_08 069_ZW1_08 069_ROD_21 069_RAM_21 069_THA_21 069_JAS_21 069_RAM_22 069_ROD_22 069_ROD_16 069_JAS_16 069_ROD_13 069_RAM_13 069_ROD_09 069_RAM_09 069_ROD_10 | S= 401C=2 4010=2 3F48=1 4014=0 401C=0xffffffff 400C=4 400C=2 | S? 3FF4 4010 4018 | ruft CActor_WalkTo CSzene_DisableInput CGame_Speak CActor_WalkTo2 FUN_004474c0 CAnimLayer_Start FUN_004475b0 CLayer_Disable CActor_Hide CAnimLayer_PlayEndEvent CLayer_Enable FUN_004403e0 CGame_Delay CAnimLayer_PlayRange CActor_BlockRect CSzene_EnableInput FUN_00443310 FUN_00442f90 FUN_004409c0 CActor_UnblockRect FUN_00420600 CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
  case 5: Sprache 069_ROD_14 069_ZW2_14 | ruft CActor_WalkTo CGame_Speak
  case 7: S? 3FFC 4008 | ruft CActor_WalkTo
  case 8: S? 400C
```
