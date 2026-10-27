# Loc10 (automatische Übersicht)

Funktionen `0x00413FF0`–`0x004186FF` (7 Stück), Ereignis-Handler `0x00415E60`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S37_To38` | `0x00414A50` |
| 2 | `S37_To39` | `0x00414A50` |
| 3 | `S37_To40` | `0x00414A50` |
| 4 | `S37_To41` | `0x00414A50` |
| 5 | `S37_To42` | `0x00414A50` |
| 6 | `S37_To43` | `0x00414A50` |
| 7 | `S37_Pilz` | `0x00414A50` |
| 8 | `S37_Wald` | `0x00414A50` |
| 9 | `S37_Glider (Ebene)` | `0x00414A50` |
| 20 | `S38_To37` | `0x00414A50` |
| 21 | `S38_To39` | `0x00414A50` |
| 22 | `S38_To57` | `0x00414A50` |
| 23 | `S38_To68` | `0x00414A50` |
| 24 | `S38_Pilz` | `0x00414A50` |
| 25 | `S38_Wald` | `0x00414A50` |
| 26 | `S38_GirlJumps (Ebene)` | `0x00414A50` |
| 28 | `Gegenstand 12` | `0x00414A50` |
| 40 | `S39_To37` | `0x00414A50` |
| 41 | `S39_To38` | `0x00414A50` |
| 42 | `S39_To40` | `0x00414A50` |
| 43 | `S39_To99` | `0x00414A50` |
| 44 | `S39_Pilz1` | `0x00414A50` |
| 45 | `S39_Pilz2` | `0x00414A50` |
| 46 | `S39_Pilz3` | `0x00414A50` |
| 47 | `S39_Wald` | `0x00414A50` |
| 48 | `S39_Pilz (Ebene)` | `0x00414A50` |
| 60 | `S40_To37` | `0x00414A50` |
| 61 | `S40_To39` | `0x00414A50` |
| 62 | `S40_Pilze` | `0x00414A50` |
| 63 | `S40_Pilz` | `0x00414A50` |
| 64 | `S40_Wald` | `0x00414A50` |
| 65 | `S40_Root` | `0x00414A50` `0x00415E60` |
| 66 | `S40_Baum` | `0x00414A50` |
| 67 | `S40_Necklace (Ebene)` | `0x00414A50` `0x00415E60` |
| 68 | `S40_Straw (Ebene)` | `0x00414A50` `0x00415E60` |
| 69 | `S40_FireLoops (Ebene)` | `0x00414A50` `0x00415E60` |
| 76 | `Gegenstand 15` | `0x00414A50` `0x00415E60` |
| 77 | `Gegenstand 26` | `0x00414A50` |
| 80 | `S41_To37` | `0x00414A50` |
| 81 | `S41_To42` | `0x00414A50` |
| 82 | `S41_Pilz` | `0x00414A50` |
| 83 | `S41_Wald` | `0x00414A50` |
| 90 | `S42_To37` | `0x00414A50` |
| 91 | `S42_To41` | `0x00414A50` |
| 92 | `S42_To91` | `0x00414A50` |
| 93 | `S42_Wald` | `0x00414A50` |
| 100 | `S43_To37` | `0x00414A50` |
| 101 | `S43_To44` | `0x00414A50` |
| 102 | `S43_Wald` | `0x00414A50` |
| 103 | `S43_Pilz` | `0x00414A50` |
| 110 | `S44_To43` | `0x00414A50` |
| 111 | `S44_Wald` | `0x00414A50` |
| 112 | `S44_Busch` | `0x00414A50` |
| 113 | `S44_Pilz` | `0x00414A50` |
| 114 | `S44_Leaf (Ebene)` | `0x00414A50` `0x00415E60` |
| 115 | `Gegenstand 8` | `0x00414A50` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00415e60.c FUN_00415e60 @ 00415e60
  Anfang: ruft FUN_00415e60
  case 1: Sprache 038_ROD_02 040_ROD_02 040_JAS_02 040_ROD_05 040_JAS_05 040_ROD_18 | S= 3F34=2 3F54=1 | ruft CSzene_DisableInput CAnimLayer_PlayEndEvent CGame_HeroSayWait CLayer_Disable CGame_Speak CLayer_Enable CGame_Random CEvent_SetTimerHandler CSzene_EnableInput FUN_004403e0 CAudioBuffer_SetVolume CActor_Hide CAnimLayer_PlayRange CActor_Show CSzene_GetArea CArea_SetEvent FUN_004409c0
  case 2: Sprache 038_ROD_03 040_ROD_03 040_JAS_03 040_ROD_05 040_JAS_05 040_ROD_18 | S= 3F34=3 | ruft CSzene_DisableInput CAnimLayer_PlayEndEvent CGame_HeroSayWait CLayer_Disable CGame_Speak CLayer_Enable CGame_Random CEvent_SetTimerHandler CSzene_EnableInput FUN_004403e0 CAudioBuffer_SetVolume CActor_Hide CAnimLayer_PlayRange CActor_Show FUN_004409c0
  case 3: Sprache 038_ROD_04 040_ROD_04 040_JAS_04 040_ROD_07 040_JAS_07 | S= 3F34=4 | ruft CGame_HeroSayWait FUN_004403e0 CGame_Random CAudioBuffer_SetVolume
  case 4: Sprache 038_ROD_05 040_ROD_01 040_ROD_06 040_JAS_06 | S= 3F34=5 | ruft CSzene_DisableInput CAnimLayer_PlayEndEvent CGame_HeroSayWait CLayer_Disable CGame_Speak CLayer_Enable CGame_Random CEvent_SetTimerHandler CSzene_EnableInput CGame_WalkAndComment CGame_PlaySfxRandom3D CAudioBuffer_SetVolume FUN_004403e0
  case 5: Sprache 038_ROD_06 | S= 3F34=6 | ruft CSzene_DisableInput CAnimLayer_PlayEndEvent CGame_HeroSayWait CLayer_Disable CGame_Speak CLayer_Enable CGame_Random CEvent_SetTimerHandler CSzene_EnableInput CAudioBuffer_SetVolume CGame_PlaySfxRandom3D
  case 6: Sprache 038_ROD_07 038_ROD_11 | ruft FUN_00417eb0 CSzene_DisableInput CAnimLayer_PlayEndEvent CLayer_Disable CActor_Hide CAnimLayer_PlayRange FUN_004403e0 CLayer_Enable CActor_Show CEvent_Fire CActor_Turn CGame_HeroSayWait CSzene_EnableInput
  case 7: Sprache 037_ROD_03 | ruft CGame_WalkAndComment
  case 8: Sprache 037_ROD_01 037_ROD_02 | S? 3F2C | ruft CGame_WalkAndComment
  case 9: → 1/8 | S= 3F24=0 | ruft CActor_WalkTo CLayer_Disable CActor_Hide CAnimLayer_PlayRange CAudioBuffer_SetFreq CAudioBuffer_FadeFreq FUN_0044c1e0
  case 10: ruft FUN_004474c0 FUN_0044c1e0
  case 23: ruft FUN_00418380
  case 24: Sprache 038_ROD_08 | ruft CGame_WalkAndComment
  case 25: Sprache 038_ROD_01 | ruft CGame_WalkAndComment
  case 26: S? 3F34 | ruft CEvent_SetTimerHandler CActor_WalkTo
  case 27: ruft CAnimLayer_Start CAnimLayer_SetFrameEvent CGame_Random CEvent_SetTimerHandler
  case 28: Sprache 038_ROD_09 038_ROD_10 038_ROD_12 | S= 3F30=0 | S? 3F30 | Inv Add(10) Remove(12) | ruft CGame_DescribeItem CEvent_SetTimerHandler CActor_WalkTo CSzene_DisableInput CAnimLayer_PlayEndEvent CGame_HeroSayWait CActor_Hide CLayer_Disable CAnimLayer_PlayRange FUN_004403e0 CActor_Show CSzene_EnableInput FUN_004409c0
  case 29: ruft CAnimLayer_SetFrameEvent
  case 41: → 10/38 | ruft CActor_WalkTo
  case 42: → 10/40 | ruft CActor_WalkTo
  case 43: → 8/99 | ruft CActor_WalkTo
  case 44: Sprache 039_ROD_01
  case 45: Sprache 039_ROD_02
  case 46: Sprache 039_ROD_03 | ruft CGame_WalkAndComment
  case 47: ruft CGame_WalkAndComment
  case 48: Sprache 037_ROD_03 | S= 3F44=0 | Inv Add(8) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CSzene_EnableInput FUN_004409c0
  case 61: → 10/39 | ruft CActor_WalkTo
  case 62: Sprache 040_ROD_11 | ruft CGame_WalkAndComment
  case 63: Sprache 040_ROD_12 | ruft CGame_WalkAndComment
  case 64: Sprache 040_ROD_01 | ruft CGame_WalkAndComment
  case 65: Sprache 040_ROD_18 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 66: ruft CActor_WalkTo
  case 67: Sprache 040_ROD_10 | S= 3F58=0 | Inv Add(16) | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CGame_HeroSayWait
  case 68: Sprache 040_ROD_09 | S= 3F54=1 | Inv Add(15) | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CLayer_Disable CSzene_GetArea CArea_SetEvent CActor_Show FUN_004409c0
  case 69: S= 3F54=4 | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CActor_Show CLayer_Disable CLayer_Enable CSzene_EnableInput
  case 75: Sprache 040_JAS_01 | S? 3F4C | ruft CAnimLayer_Start FUN_004403e0 CGame_Random CEvent_SetTimerHandler
  case 76: Sprache 040_ROD_14 040_JAS_14 | S= 3F54=2 | S? 3F48 3F54 | Inv Remove(15) | ruft CGame_DescribeItem CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CLayer_Enable CArea_SetEvent CSzene_GetArea CActor_Show CGame_HeroSayWait FUN_004403e0
  case 77: Sprache 040_ROD_16 040_JAS_16 040_JAS_17 | S= 3F54=3 3F58=1 | S? 3F48 3F54 | ruft CGame_DescribeItem CActor_WalkTo CSzene_DisableInput CGame_HeroSayWait FUN_004403e0 CActor_Hide CAnimLayer_PlayRange CActor_Show CLayer_Disable CAnimLayer_Start CArea_SetEvent CLayer_Enable CActor_Turn CSzene_EnableInput
  case 81: → 10/42 | ruft CActor_WalkTo
  case 82: Sprache 040_ROD_11 | ruft CGame_WalkAndComment
  case 83: S? 3F5C
  == 1: Sprache 041_ROD_01 | S= 3F5C=2 | ruft CActor_WalkTo FUN_004409c0
  == 2: Sprache 041_ROD_02 | S= 3F5C=3 | Inv Add(12) | ruft CActor_WalkTo CSzene_DisableInput CGame_HeroSayWait CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CSzene_EnableInput
  == 3: Sprache 041_ROD_01 | ruft CGame_WalkAndComment
  case 91: → 10/41 | ruft CActor_WalkTo
  case 92: → 12/91 | ruft CActor_WalkTo
  case 93: Sprache 042_ROD_01 | ruft CGame_WalkAndComment
  case 100: → 10/37 | ruft CActor_WalkTo
  case 101: → 10/44 | ruft CActor_WalkTo
  case 102: Sprache 043_ROD_01 | ruft CGame_WalkAndComment
  case 103: Sprache 043_ROD_02 | ruft CGame_WalkAndComment
  case 110: → 10/43 | ruft CActor_WalkTo
  case 111: Sprache 044_ROD_01 | ruft CGame_WalkAndComment
  case 112: Sprache 044_ROD_02 | ruft CGame_WalkAndComment
  case 113: Sprache 044_ROD_04 | ruft CGame_WalkAndComment
  case 114: Sprache 044_ROD_05 | S= 3F68=0 3F6C=0 | Inv Add(6) | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show FUN_004409c0
  case 115: Sprache 044_ROD_03 | S= 3F64=3 3F6C=1 | Inv Remove(8) | ruft CActor_WalkTo CSzene_DisableInput CAnimLayer_PlayRange CActor_Hide CAudioBuffer_SetVolume FUN_00442f90 CArea_SetEvent CLayer_Disable CActor_Show FUN_004409c0 CSzene_EnableInput
  case 200: S? 3E80 | ruft CGame_Random
```
