# Loc12 (automatische Übersicht)

Funktionen `0x00418700`–`0x0041964F` (5 Stück), Ereignis-Handler `0x00418BF0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S91_Door1` | `0x004189E0` |
| 2 | `S91_Door2` | `0x004189E0` |
| 3 | `S91_CynDoor` | `0x004189E0` |
| 4 | `S91_Brunnen (Ebene)` | `0x004189E0` |
| 5 | `S91_Wald` | `0x004189E0` |
| 7 | `Gegenstand 7` | `0x004189E0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00418bf0.c FUN_00418bf0 @ 00418bf0
  Anfang: ruft FUN_00418bf0
  case 1: Sprache 091_ROD_02 091_ROD_04 091_ROD_05 091_ROD_06 091_ROD_11 091_ROD_12 | S= 3F78=2 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait CSzene_DisableInput CLayer_Disable FUN_004403e0 CLayer_Enable CActor_WalkTo2 CSzene_EnableInput
  case 2: Sprache 091_ROD_03 091_ROD_07 091_ROD_13 091_ROD_14 | S= 3F78=3 | Inv Has(7) | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait CLayer_Disable FUN_004403e0
  case 3: Sprache 091_ROD_15 091_ROD_16 | → 14/90 | S= 3F78=4 | S? 3F78 | ruft CActor_WalkTo CSzene_SetModal CAnimLayer_PlayEndEvent CActor_Hide CLayer_Disable FUN_004403e0
  case 4: Sprache 091_ROD_17 091_ROD_18 091_ROD_20 091_ROD_04 091_ROD_19 | S= 3F78=3 3FBC=1 | S? 3F74 | ruft CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent FUN_004403e0 CActor_Show CLayer_Enable CActor_WalkTo2 CGame_HeroSayWait CGame_Delay CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
  == 0: S= 3FA0=1
  case 5: → 10/42 | S= 3F60=1 | ruft CActor_WalkTo
  case 6: ruft CGame_Random
  case 7: Sprache 091_ROD_08 091_ROD_09 091_ROD_10 | S= 3F74=3 | Inv Remove(7) | ruft CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CGame_Delay FUN_004403e0 CActor_Show CLayer_Enable CActor_WalkTo2 CGame_HeroSayWait
```
