# Loc02 (automatische Übersicht)

Funktionen `0x00407E50`–`0x0040A4DF` (15 Stück), Ereignis-Handler `0x00408D50`, `0x00408DD0`, `0x00408EB0`, `0x004093A0`, `0x00409420`, `0x00409AE0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S0013_Exit` | `0x00408620` |
| 2 | `S0013_To2013` | `0x00408620` |
| 3 | `S0013_To1013` | `0x00408620` |
| 10 | `S1013_San (Ebene)` | `0x00408620` |
| 11 | `S1013_Gast1 (Ebene)` | `0x00408620` |
| 11 | `S1013_Gast2 (Ebene)` | `0x00408620` |
| 12 | `S1013_To0013` | `0x00408620` |
| 20 | `S2013_Fremder` | `0x00408620` |
| 21 | `S2013_To0013` | `0x00408620` |
| 30 | `S0014_To1013` | `0x00408620` |
| 31 | `S0014_Sancho` | `0x00408620` |
| 32 | `S0014_SanchosOhr` | `0x00408620` |
| 33 | `Gegenstand 13` | `0x00408620` `0x00409420` |
| 40 | `S0015_To2013A` | `0x00408620` |
| 40 | `S0015_To2013B` | `0x00408620` |
| 41 | `S0015_Fremder` | `0x00408620` |
| 42 | `S0015_Fremder` | `0x00408620` |
| 43 | `S0015_Bier` | `0x00408620` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00408d50.c FUN_00408d50 @ 00408d50
  Anfang: ruft FUN_00408d50
  == 15: ruft FUN_00409ae0
  == 13: ruft FUN_00408dd0
  == 14: ruft FUN_00409420
  == 1013: ruft FUN_00408eb0
  == 2013: ruft FUN_004093a0
# 00408dd0.c FUN_00408dd0 @ 00408dd0
  Anfang: ruft FUN_00408dd0
  == 1: → 3/12 | ruft CActor_WalkTo
  == 2: → 2/2013 2/1013 | ruft CActor_WalkTo
# 00408eb0.c FUN_00408eb0 @ 00408eb0
  Anfang: ruft FUN_00408eb0
  case 10: → 2/14 | ruft CActor_WalkTo
  case 11: ruft CActor_WalkTo
  == 3: ruft CGame_Random
  case 1: Sprache 013_ROD_01 | ruft FUN_00440400
  case 2: Sprache 013_ROD_02 | ruft FUN_00440400
  case 3: Sprache 013_ROD_03 | ruft FUN_00440400
  case 4: Sprache 013_ROD_04 | ruft FUN_00440400
  == 1: Sprache 013_ROD_05 013_GA1_01 013_GA2_01 013_ROD_06 013_GA1_02 | ruft FUN_00440400 CGame_Speak
  == 2: Sprache 013_ROD_07 013_GA1_03 013_GA1_04 013_GA1_05 013_GA2_02 | ruft FUN_00440400 CGame_Speak CGame_Delay CAnimLayer_PlayRange CGame_Random CEvent_SetTimerHandler
  case 12: → 2/13 | ruft CActor_WalkTo
  case 13: ruft CAnimLayer_Start CGame_Random CEvent_SetTimerHandler
  case 14: ruft FUN_00442f90 FUN_00443300 FUN_00443530 CGame_Random CEvent_SetTimerHandler
  case 15: ruft FUN_00443530
# 004093a0.c FUN_004093a0 @ 004093a0
  Anfang: ruft FUN_004093a0
  == 20: → 2/15 | ruft CActor_WalkTo
  == 21: → 2/13 | ruft CActor_WalkTo
# 00409420.c FUN_00409420 @ 00409420
  Anfang: ruft FUN_00409420 CSzene_DisableInput
  case 30: → 2/1013 | ruft CSzene_EnableInput
  == 1: Sprache 014_ROD_01 014_SAN_02 | S= 3EAC=2 | ruft FUN_0040a040 FUN_0040a080 CSzene_EnableInput
  case 2: Sprache 014_ROD_02 014_SAN_03 | ruft FUN_0040a040 FUN_0040a080 CSzene_EnableInput
  case 3: Sprache 014_ROD_03 014_SAN_04 | ruft FUN_0040a040 FUN_0040a080 CSzene_EnableInput
  case 4: Sprache 014_ROD_04 014_SAN_05 | ruft FUN_0040a040 CAnimLayer_PlayRange CGame_Speak FUN_0044c1e0 CSzene_EnableInput
  case 5: Sprache 014_ROD_05 014_SAN_06 014_ROD_06 | ruft CAnimLayer_PlayRange CGame_Speak FUN_0040a040 CSzene_EnableInput
  case 7: Inv Has(13)
  == 0: Sprache 014_ROD_08 014_SAN_08 014_ROD_09 014_SAN_09 | S= 3EA8=8 | ruft FUN_0040a040 FUN_0040a080 CSzene_EnableInput
  case 8: Sprache 014_ROD_10 014_SAN_10 014_ROD_07 014_SAN_07 | Inv Add(13) | ruft FUN_0040a040 FUN_0040a080 CAnimLayer_PlayRange CSzene_EnableInput
  == 2: Sprache 014_ROD_11 014_SAN_11 014_ROD_12 014_SAN_12 | ruft FUN_0040a040 FUN_0040a080 CSzene_EnableInput
  == 3: Sprache 014_ROD_13 014_SAN_13 014_SAN_16 | ruft FUN_0040a040 FUN_0040a080 CSzene_EnableInput CAnimLayer_PlayRange CGame_Delay
  case 32: Sprache 014_ROD_14 014_SAN_14 014_ROD_15 | S= 3EB0=iVar2 + 1 | S? 3EB0 | ruft CAnimLayer_PlayRange FUN_0040a040 FUN_0040a080 CSzene_EnableInput
  case 33: Sprache 014_ROD_17 014_SAN_17 | Inv Remove(13) | ruft CAnimLayer_PlayRange FUN_0040a040 FUN_0040a080 CSzene_EnableInput
# 00409ae0.c FUN_00409ae0 @ 00409ae0
  Anfang: ruft FUN_00409ae0
  case 40: → 2/2013 | ruft FUN_0040a190
  case 1: Sprache 015_ROD_02 015_FRE_02 | ruft FUN_0040a100
  case 2: Sprache 015_ROD_03 015_FRE_03 | ruft FUN_0040a100
  case 3: Sprache 015_ROD_04 015_FRE_04 | ruft FUN_0040a100
  case 4: Sprache 015_ROD_05 015_FRE_05 015_ROD_06 015_FRE_06 015_ST1_01 | → 3/12 | S= 3EA0=0 3EB4=5 3ECC=1 | Inv Add(1) | ruft CSzene_DisableInput FUN_0040a190 FUN_0040a0c0 CSzene_SetModal FUN_004403e0 CAnimLayer_PlayRange CAnimLayer_PlayEndEvent
  case 42: S= 3EBC=iVar1 + 1 | S? 3EBC | ruft CSzene_DisableInput FUN_0040a190 CSzene_SetModal CAnimLayer_PlayRange CSzene_EnableInput
  case 43: ruft CSzene_DisableInput FUN_0040a190 CSzene_SetModal CAnimLayer_PlayRange CSzene_EnableInput CSzene_GetArea
  case 44: ruft FUN_0040a190
  case 46: ruft FUN_00443530
  case 47: S? 3EB8 | ruft CSzene_GetArea
```
