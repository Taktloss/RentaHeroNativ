# Loc17 (automatische Übersicht)

Funktionen `0x0041CF00`–`0x0041E4EF` (5 Stück), Ereignis-Handler `0x0041D9F0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S59_To58` | `0x0041D480` |
| 2 | `S59_Skeleton` | `0x0041D480` |
| 3 | `S59_Wall` | `0x0041D480` |
| 4 | `S59_Head` | `0x0041D480` |
| 50 | `S60_To59` | `0x0041D480` |
| 51 | `S60_Skull` | `0x0041D480` |
| 52 | `S60_Mouth` | `0x0041D480` |
| 53 | `S60_Ear` | `0x0041D9F0` |
| 53 | `this` | `0x0041D480` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041d9f0.c FUN_0041d9f0 @ 0041d9f0
  Anfang: ruft FUN_0041d9f0
  case 1: → 16/58 | S? 3FC8 | ruft CActor_WalkTo CSzene_DisableInput CLayer_Enable FUN_00442f90 FUN_004475b0 CGame_Delay CSzene_EnableInput CGame_Random CAudioBuffer_SetVolume
  case 2: ruft CActor_WalkTo CGame_Random CGame_Comment CAudioBuffer_SetVolume
  case 3: Sprache 059_ROD_07 059_ROD_08 | S? 3FC8 | ruft CActor_WalkTo CGame_HeroSay CGame_Random CAudioBuffer_SetVolume
  case 4: → 17/60 | ruft CActor_WalkTo CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
  case 30: ruft CAnimLayer_PlayRange
  case 31: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 32: ruft CAnimLayer_PlayRange
  case 33: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 34: ruft CAnimLayer_PlayRange
  case 35: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 36: ruft CAnimLayer_PlayRange
  case 37: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 38: ruft CAnimLayer_PlayRange
  case 39: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 40: ruft CAnimLayer_PlayRange
  case 41: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 42: ruft CAnimLayer_PlayRange
  case 43: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 50: → 17/59 | ruft CActor_WalkTo
  case 51: Sprache 060_ROD_02 | ruft CActor_WalkTo CGame_Comment
  case 52: Sprache 060_ROD_09 060_ROD_03 | S? 3FC8 3FCC | ruft CActor_WalkTo CGame_Comment CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayEndEvent CGame_HeroSayWait CSzene_EnableInput
  case 53: Sprache 060_ROD_09 060_ROD_08 | → 18/61 | S= 3FD0=1 | S? 3FC8 3FD0 | ruft CActor_WalkTo CGame_Comment CGame_HeroSay
  case 54: S? 3FCC | ruft CActor_WalkTo
  == 1: Sprache 060_ROD_04 | S= 3FCC=2 | ruft CGame_Comment
  == 2: Sprache 060_ROD_05 | S= 3FCC=3 3FD8=2 | S? 3FD8 | ruft CGame_Comment
  == 3: Sprache 060_ROD_06 060_ROD_07 | S= 3FCC=4 | ruft CSzene_DisableInput CGame_HeroSay CActor_Hide CAnimLayer_PlayRange FUN_004475b0 CActor_Show CGame_Comment CArea_SetEvent CSzene_EnableInput
  case 60: ruft CLayer_Disable
  case 61: ruft CLayer_Disable
  case 62: ruft CLayer_Disable
  case 63: ruft CLayer_Disable
  case 64: ruft CLayer_Disable
  case 65: ruft CLayer_Disable
  case 66: ruft CLayer_Disable
  case 100: ruft CGame_Random
```
