# Loc08 (automatische Übersicht)

Funktionen `0x00411640`–`0x004134AF` (9 Stück), Ereignis-Handler `0x004124B0`, `0x004125A0`, `0x00412FD0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `*(void **)(iVar1 + 0x1c0)` | `0x00412150` |
| 1 | `*(void **)(iVar2 + 0x1c0)` | `0x00411CD0` |
| 3 | `*(void **)(param_1 + 0xcc)` | `0x00411CD0` |
| 5 | `*(void **)(param_1 + 0xd0)` | `0x00411CD0` |
| 7 | `*(void **)(param_1 + 0xd4)` | `0x00411CD0` |
| 9 | `*(void **)(param_1 + 0xd8)` | `0x00411CD0` |
| 11 | `*(void **)(param_1 + 0xdc)` | `0x00411CD0` |
| 11 | `099_Gang` | `0x004125A0` |
| 11 | `pvVar3` | `0x00411CD0` |
| 13 | `*(void **)(*(int *)(param_1 + ` | `0x00411CD0` |
| 13 | `SabStand (Ebene)` | `0x004125A0` |
| 13 | `pvVar3` | `0x00411CD0` |
| 100 | `*(void **)(*(int *)(param_1 + ` | `0x00412150` |
| 100 | `BootFahne (Ebene)` | `0x00412FD0` |
| 100 | `this` | `0x00412150` |
| 101 | `*(void **)(param_1 + 0xe0)` | `0x00412150` |
| 102 | `Gegenstand 2` | `0x00412150` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 004124b0.c FUN_004124b0 @ 004124b0
  Anfang: ruft FUN_004124b0 FUN_004125a0 FUN_00412fd0 CGame_Random
  case 6: ruft CGame_Random CAudioBuffer_FadeFreq
  case 11: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
# 004125a0.c FUN_004125a0 @ 004125a0
  Anfang: ruft FUN_004125a0
  case 1: Sprache 099_ROD_10 099_ROD_01 099_SAB_02 | ruft CGame_WalkAndComment CGame_HeroSayWait CGame_Speak
  case 3: Sprache 099_ROD_11 099_ROD_03 099_ROD_04 099_SAB_04 | S= 3F10=4 | ruft CGame_WalkAndComment CSzene_DisableInput CGame_HeroSayWait CActor_WalkTo CGame_Speak CSzene_EnableInput
  case 5: Sprache 099_ROD_09 099_ROD_06 099_SAB_06 | S= 3F18=2 | ruft CGame_WalkAndComment CGame_HeroSayWait CSzene_DisableInput CGame_Speak CLayer_Disable FUN_004474c0 FUN_00442f90 FUN_004475b0 CAnimLayer_PlayRange CAnimLayer_Start CLayer_Enable CArea_SetEvent CSzene_EnableInput
  case 7: S? 3F0C
  == 1: Sprache 099_SAB_01 | S= 3F0C=2 3F18=1 | ruft CActor_WalkTo2 CSzene_DisableInput FUN_004475b0 CAnimLayer_PlayRange CActor_Hide FUN_00442f90 CAnimLayer_ShowFrame FUN_004474c0 CLayer_Enable CLayer_Disable FUN_00435b80 CActor_Show CGame_Speak CSzene_EnableInput CArea_SetEvent
  == 2: Sprache 099_ROD_08 099_ROD_07 099_SAB_07 | → 8/100 | ruft CGame_WalkAndComment CLayer_Disable FUN_004474c0 FUN_00442f90 FUN_004475b0 CAnimLayer_PlayEndEvent CActor_WalkTo2 CGame_HeroSayWait CGame_Speak
  case 9: Sprache 099_SAB_08 | → 10/39 16/58 | S? 3F18 3F10 | ruft CGame_Speak CActor_WalkTo
  case 11: S? 3F18 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent
  case 13: S? 3F18
  case 2: Sprache 099_ROD_02 099_SAB_03 | ruft CGame_HeroSayWait CGame_Speak
  case 4: Sprache 099_ROD_05 099_SAB_05 | S= 3F10=5 | ruft CSzene_DisableInput CGame_HeroSayWait CGame_Speak CLayer_Disable CAnimLayer_PlayEndEvent CLayer_Enable CSzene_EnableInput
  == 3: Sprache 099_ROD_07 | ruft CActor_WalkTo2 CGame_HeroSayWait
  case 70: ruft CEvent_SetTimerHandler
  case 71: ruft CAnimLayer_Start
# 00412fd0.c FUN_00412fd0 @ 00412fd0
  Anfang: ruft FUN_00412fd0
  case 1: Sprache 100_ROD_02 | ruft CGame_WalkAndComment
  case 13: Sprache 100_ROD_03 100_SAB_02 | ruft CActor_WalkTo FUN_00440400 CGame_Speak
  case 100: Sprache 100_ROD_01 100_SAB_01 | → 23/103 | S= 3F18=4 | S? 4088 3F14 | ruft FUN_004409c0 CGame_Speak CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayRange FUN_00442f90 FUN_004475b0
  case 101: → 8/99 | ruft CActor_WalkTo
  case 102: Sprache 100_ROD_04 100_SAB_03 | S= 3F14=1 | Inv Remove(2) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CAnimLayer_Start CArea_SetEvent CActor_Show FUN_00440400 CGame_Speak CSzene_EnableInput
```
