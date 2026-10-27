# Loc30 (automatische Übersicht)

Funktionen `0x0042F750`–`0x0043035F` (5 Stück), Ereignis-Handler `0x0042FC70`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S34_Door` | `0x0042FA00` |
| 2 | `S34_Ring1` | `0x0042FA00` |
| 3 | `S34_Ring2` | `0x0042FA00` |
| 4 | `S34_Plate` | `0x0042FA00` |
| 5 | `S34_Window` | `0x0042FA00` |
| 6 | `S34_JasStehTalk (Ebene)` | `0x0042FA00` |
| 7 | `S34_Straw (Ebene)` | `0x0042FA00` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0042fc70.c FUN_0042fc70 @ 0042fc70
  Anfang: ruft FUN_0042fc70
  case 1: Sprache 034_JAS_12 034_ROD_07 034_JAS_07 | ruft CActor_WalkTo CActor_Hide CGame_Speak CAnimLayer_PlayEndEvent CActor_Show CActor_WalkTo2 CAnimLayer_ShowFrame CGame_HeroSayWait FUN_00440400
  case 2: Sprache 034_JAS_16 034_ROD_08 034_JAS_08 | ruft CActor_WalkTo FUN_00440400 CGame_HeroSayWait
  case 3: Sprache 034_JAS_17 034_ROD_09 034_JAS_09 034_JAS_10 | S= 4108=4 | ruft CActor_WalkTo FUN_00440400 CSzene_DisableInput CGame_HeroSayWait CGame_Speak CSzene_EnableInput
  case 4: Sprache 034_JAS_15 034_ROD_11 034_JAS_11 | ruft CActor_WalkTo FUN_00440400 CGame_HeroSayWait
  case 5: → 28/35 | S= 40FC=1 | ruft CActor_WalkTo
  case 6: ruft CActor_WalkTo
  == 1: Sprache 034_ROD_02 034_JAS_02 034_ROD_04 034_JAS_04 034_JAS_01 034_JAS_06 | ruft CGame_HeroSayWait FUN_00440400
  == 2: Sprache 034_ROD_03 034_JAS_03 034_ROD_05 034_JAS_05 | ruft CGame_HeroSayWait FUN_00440400
  == 3: Sprache 034_ROD_06 | ruft CGame_Random CGame_HeroSayWait
  case 7: Sprache 034_JAS_13 034_JAS_14 034_ROD_12 | → 28/36 | S? 4100 | Inv Add(15) | ruft CActor_WalkTo CGame_Speak CActor_Hide CSzene_GetObject CAnimLayer_PlayEndEvent CActor_Show CAnimLayer_ShowFrame CSzene_DisableInput CLayer_Disable CAnimLayer_AddCue CAnimLayer_PlayRange FUN_004403e0 CSzene_EnableInput
  case 8: Sprache 034_JAS_01 | ruft CLayer_Disable FUN_00440400 CLayer_Enable
```
