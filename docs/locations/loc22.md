# Loc22 (automatische Übersicht)

Funktionen `0x00425CF0`–`0x00426A1F` (9 Stück), Ereignis-Handler `0x00426510`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S81_To1081` | `0x00426270` |
| 2 | `S81_To79` | `0x00426270` |
| 3 | `S1081_To81` | `0x00426270` |
| 4 | `*(void **)(*piVar4 + 0x1c0)` | `0x00426270` |
| 4 | `S1081_BigStone (Ebene)` | `0x00426270` |
| 17 | `Gegenstand 17` | `0x004269A0` |
| 18 | `Gegenstand 18` | `0x004269A0` |
| 19 | `Gegenstand 19` | `0x004269A0` |
| 20 | `Gegenstand 20` | `0x004269A0` |
| 21 | `Gegenstand 21` | `0x004269A0` |
| 22 | `Gegenstand 22` | `0x004269A0` |
| 30 | `Gegenstand 16` | `0x00426270` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00426510.c FUN_00426510 @ 00426510
  Anfang: ruft FUN_00426510
  case 1: ruft CActor_WalkTo FUN_004269f0
  == 0: → 22/1081 21/79 | ruft CSzene_SetModal CAnimLayer_PlayEndEvent
  case 2: ruft CActor_WalkTo FUN_004269f0
  case 3: → 22/81
  case 4: ruft CGame_Random
  == 1: ruft CSzene_DisableInput FUN_004403e0 CAnimLayer_PlayEndEvent CSzene_EnableInput
  == 2: ruft FUN_004403e0
  case 15: ruft FUN_00426790
  case 22: ruft FUN_00426840
  case 30: Inv Remove(16) Add(17) Add(18) Add(19) Add(20) Add(22) | ruft CSzene_DisableInput FUN_004403e0 CGame_Delay FUN_004269a0 CSzene_EnableInput
```
