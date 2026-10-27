# Loc03 (automatische Übersicht)

Funktionen `0x0040A4E0`–`0x0040B8BF` (7 Stück), Ereignis-Handler `0x0040AE30`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S12_Bridge` | `0x0040A9C0` |
| 2 | `S12_Schild` | `0x0040A9C0` |
| 3 | `S12_To13` | `0x0040A9C0` |
| 4 | `S12_Pflanze` | `0x0040A9C0` |
| 5 | `S12_RodGlider (Ebene)` | `0x0040A9C0` |
| 6 | `S12_SabSword (Ebene)` | `0x0040A9C0` |
| 7 | `S12_SabGlider (Ebene)` | `0x0040A9C0` |
| 8 | `S12_Rab1Talk (Ebene)` | `0x0040A9C0` |
| 9 | `S12_Rab2 (Ebene)` | `0x0040A9C0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040ae30.c FUN_0040ae30 @ 0040ae30
  Anfang: ruft FUN_0040ae30
  case 1: Sprache 012_ROD_03 012_RA1_01 | ruft FUN_0040ae10 CGame_WalkAndComment FUN_0040adc0 CGame_Random CAudioBuffer_SetVolume
  case 2: Sprache 012_ROD_01 012_RA2_01 | ruft FUN_0040ae10 CGame_WalkAndComment FUN_0040adc0 CGame_Speak FUN_00442f90 CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 012_RA1_02 | → 2/13 | ruft CActor_WalkTo FUN_0040ae10
  case 4: Sprache 012_ROD_02 012_RA2_02 | ruft FUN_0040ae10 CGame_WalkAndComment FUN_0040adc0 CGame_Speak CGame_Random CAudioBuffer_SetVolume
  case 5: → 1/7 | ruft FUN_0040ae10 CActor_WalkTo CSzene_DisableInput CLayer_Disable CSzene_SetModal CAnimLayer_PlayRange CAudioBuffer_SetFreq CAudioBuffer_FadeFreq
  case 9: Sprache 012_ROD_06 012_RA1_03 012_RA2_03 012_RA1_04 012_RA1_05 012_RA1_06 012_RA2_05 012_ROD_07 012_SAB_01 012_ROD_08 | S= 3ECC=1 | ruft FUN_0040ae10 CActor_WalkTo CSzene_DisableInput CActor_UnblockRect FUN_00440400 CGame_Speak CAnimLayer_PlayRange CAnimLayer_ShowFrame CLayer_Disable CAnimLayer_AddCue FUN_00442f90 FUN_004475b0 CActor_WalkTo2 CAnimLayer_ClearCues CLayer_Enable CSzene_EnableInput CActor_Turn CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
  case 7: ruft FUN_0040ae10 CGame_Random
  == 1: Sprache 012_ROD_04 012_ROD_05 | ruft CGame_WalkAndComment FUN_0040adc0
  case 10: ruft CGame_Random
  case 11: ruft FUN_0040adc0
  case 12: ruft CLayer_Disable CAnimLayer_ClearCues CLayer_Enable CAudioBuffer_SetFreq CAnimLayer_PlayRange
  case 13: ruft CAudioBuffer_FadeFreq CAnimLayer_PlayRange
  case 14: ruft CGame_Random
```
