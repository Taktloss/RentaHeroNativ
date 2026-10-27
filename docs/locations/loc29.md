# Loc29 (automatische Übersicht)

Funktionen `0x0042EE50`–`0x0042F74F` (5 Stück), Ereignis-Handler `0x0042F290`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S33_To32` | `0x0042F0E0` |
| 2 | `S33_Pirate1 (Ebene)` | `0x0042F0E0` |
| 2 | `S33_Pirate2 (Ebene)` | `0x0042F0E0` |
| 2 | `S33_Pirate3 (Ebene)` | `0x0042F0E0` |
| 3 | `S33_Krug1 (Ebene)` | `0x0042F0E0` `0x0042F290` |
| 4 | `Gegenstand 1` | `0x0042F0E0` |
| 4 | `Gegenstand 14` | `0x0042F0E0` |
| 4 | `Gegenstand 3` | `0x0042F0E0` |
| 4 | `Gegenstand 4` | `0x0042F0E0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0042f290.c FUN_0042f290 @ 0042f290
  Anfang: ruft FUN_0042f290
  case 1: → 28/32 | ruft CSzene_DisableInput CLayer_Disable CLayer_Enable CAnimLayer_PlayRange
  case 2: S? 40F8
  == 1: Sprache 033_ROD_01 033_PI2_01 033_PI3_01 | S= 40F8=2 | ruft CGame_Speak CAnimLayer_PlayRange
  == 2: Sprache 033_ROD_02 033_PI2_02 033_PI3_02 033_PI3_03 | S= 40F4=1 40F8=3 | ruft CSzene_DisableInput CGame_Speak CAnimLayer_PlayRange CLayer_Enable CAnimLayer_ShowFrame CArea_SetEvent CSzene_EnableInput
  == 3: Sprache 033_ROD_04 033_PI2_04 | ruft CGame_Speak
  case 3: Sprache 033_PI2_05 033_PI3_05 | → 30/34 | ruft CSzene_DisableInput CLayer_Disable CSzene_SetModal CAnimLayer_PlayRange FUN_004403e0 CGame_Delay
  case 4: ruft FUN_004403e0
```
