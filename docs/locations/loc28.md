# Loc28 (automatische Übersicht)

Funktionen `0x0042C6C0`–`0x0042EE4F` (7 Stück), Ereignis-Handler `0x0042D940`, `0x0042E570`, `0x0042EA20`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S31_To30_113` | `0x0042CE30` |
| 2 | `S31_To32` | `0x0042CE30` |
| 3 | `S31_Wall` | `0x0042CE30` |
| 4 | `S31_Wache (Ebene)` | `0x0042CE30` |
| 5 | `Gegenstand 1` | `0x0042CE30` |
| 6 | `Gegenstand 4` | `0x0042CE30` |
| 7 | `Gegenstand 14` | `0x0042CE30` |
| 8 | `Gegenstand 3` | `0x0042CE30` |
| 11 | `S32_To31` | `0x0042CE30` |
| 12 | `S32_Wagen` | `0x0042CE30` |
| 13 | `S32_FensterLinks` | `0x0042CE30` |
| 14 | `S32_FensterRechts` | `0x0042CE30` |
| 15 | `S32_Kneipe` | `0x0042CE30` |
| 16 | `S32_Treppen` | `0x0042CE30` |
| 17 | `S32_SchuttLinks` | `0x0042CE30` |
| 18 | `S32_SchuttRechts` | `0x0042CE30` |
| 19 | `S32_FassLinks` | `0x0042CE30` |
| 20 | `S32_FassRechts` | `0x0042CE30` |
| 21 | `S32_Kerker` | `0x0042CE30` |
| 22 | `S32_Wache (Ebene)` | `0x0042CE30` |
| 23 | `Gegenstand 1` | `0x0042CE30` |
| 24 | `Gegenstand 4` | `0x0042CE30` |
| 25 | `Gegenstand 14` | `0x0042CE30` |
| 26 | `Gegenstand 3` | `0x0042CE30` |
| 31 | `S35_To34` | `0x0042CE30` |
| 32 | `S35_To34_2` | `0x0042CE30` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0042d940.c FUN_0042d940 @ 0042d940
  Anfang: ruft FUN_0042d940
  == 1000: ruft CGame_Random
  case 1: Sprache 031_ROD_04 | → 31/113 | S= 40E4=0 40F0=0 | S? 40F0 40E4 | ruft CGame_Random CAudioBuffer_SetVolume CGame_Speak CLayer_Disable CAnimLayer_PlayRange CLayer_Enable CAnimLayer_PlayEndEvent CActor_WalkTo CActor_Hide CGame_Delay CSzene_EnableInput
  case 2: ruft CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 031_ROD_10 | ruft CGame_Random CAudioBuffer_SetVolume
  case 4: S? 40F0 | ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler CActor_WalkTo
  == 1: Sprache 031_ROD_14 031_PW1_14 031_ROD_05 031_ROD_08 031_ROD_06 031_PW2_06 031_PW1_06 031_PW2_07 031_ROD_07 031_PW1_11 031_ROD_11 031_PW1_12b 031_PW1_12 031_ROD_23 031_ROD_24 | S= 40E8=2 40E4=1 40F0=1 40D0=0 | S? 40E4 40D0 | ruft CGame_Speak CSzene_DisableInput FUN_004403e0 CGame_Delay CAnimLayer_PlayRange CLayer_Disable CAnimLayer_PlayEndEvent CActor_Show CActor_SetPos CActor_WalkTo2 CActor_Hide CSzene_EnableInput
  == 2: Sprache 031_ROD_15 031_PW1_15 031_ROD_16 031_PW1_16 | → 28/32 | S? 40F0 | Inv Has(1) | ruft CGame_Speak CActor_BlockRect CActor_WalkTo CActor_Hide CSzene_SetModal CAnimLayer_PlayEndEvent
  == 0: Sprache 031_ROD_09 | Inv Has(3) Has(4) Has(14) Has(1)
  case 5: Sprache 031_ROD_25 | S? 40F0 | Inv Has(1) Has(3) Has(4) Has(14) | ruft FUN_004409c0 FUN_0042e420
  case 6: Sprache 031_ROD_26 | S? 40F0 | Inv Has(1) Has(3) Has(4) Has(14) | ruft FUN_004409c0 FUN_0042e420
  case 7: S? 40F0 | Inv Has(1) Has(3) Has(4) Has(14) | ruft FUN_004409c0 FUN_0042e420 CGame_Random
  case 8: Sprache 031_ROD_26b | S? 40F0 | Inv Has(1) Has(3) Has(4) Has(14) | ruft FUN_004409c0 FUN_0042e420 CGame_Speak
# 0042e570.c FUN_0042e570 @ 0042e570
  Anfang: ruft FUN_0042e570
  case 11: → 28/31 | ruft CActor_WalkTo
  case 12: Sprache 032_ROD_13 | ruft CGame_WalkAndComment
  case 13: Sprache 032_ROD_14 | ruft CGame_WalkAndComment
  case 14: Sprache 032_ROD_15 | ruft CGame_WalkAndComment
  case 15: → 29/33 | ruft CActor_WalkTo
  case 16: Sprache 032_ROD_16 | ruft CGame_WalkAndComment
  case 17: ruft CGame_Random
  == 1: Sprache 032_ROD_02 032_ROD_04 032_ROD_07 032_ROD_10 032_PW2_10 | S= 40D4=2 40DC=2 | ruft CGame_WalkAndComment FUN_00440400
  == 2: Sprache 032_ROD_03 032_ROD_05 032_ROD_08 032_ROD_09 032_ROD_11 032_PW2_11 | S= 40D4=3 40DC=3 | Inv Add(25) | ruft CGame_WalkAndComment CSzene_DisableInput CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CLayer_Disable CActor_Show CSzene_EnableInput FUN_00440400
  case 18: S? 40D4
  == 3: Sprache 032_ROD_06 032_ROD_12 032_PW2_12 | ruft CGame_WalkAndComment FUN_00440400
  case 19: Sprache 032_ROD_17 | ruft CGame_WalkAndComment
  case 20: Sprache 032_ROD_18 | ruft CGame_WalkAndComment
  case 22: S? 40DC | ruft CActor_WalkTo
  case 26: ruft FUN_004409c0
# 0042ea20.c FUN_0042ea20 @ 0042ea20
  Anfang: ruft FUN_0042ea20
  case 32: → 30/34
  case 33: S? 40C4
  case 1: Sprache 035_CAP_01 | S= 40C4=2 | ruft CGame_Speak
  case 2: Sprache 035_FRE_01 | S= 40C4=3 | ruft CGame_Speak
  case 3: Sprache 035_CAP_02 | S= 40C4=4 | ruft CGame_Speak
  case 4: Sprache 035_FRE_02 | S= 40C4=5 | ruft CGame_Speak
  case 5: Sprache 035_CAP_03 | S= 40C4=6 | ruft CGame_Speak
  case 6: Sprache 035_FRE_03 | S= 40C4=7 | ruft CGame_Speak
  case 7: Sprache 035_CAP_04 | S= 40C4=8 | ruft CGame_Speak
  case 8: Sprache 035_FRE_04 | S= 40C4=9 | ruft CGame_Speak
  case 9: Sprache 035_CAP_05 | S= 40C4=10 | ruft CGame_Speak
  case 10: Sprache 035_FRE_05 | S= 40C4=0xb | ruft CGame_Speak
  case 11: Sprache 035_CAP_06 035_FRE_06 035_FRE_07 | S= 40C4=1 | ruft CSzene_DisableInput CGame_Speak CAnimLayer_PlayRange CSzene_EnableInput CEvent_SetTimerHandler
  case 34: ruft CEvent_SetTimerHandler
```
