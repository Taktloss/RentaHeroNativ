# Loc25 (automatische Übersicht)

Funktionen `0x00429020`–`0x0042A06F` (6 Stück), Ereignis-Handler `0x00429810`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S105_WizWalk (Ebene)` | `0x00429660` |
| 11 | `S105_DraHead (Ebene)` | `0x00429810` |
| 12 | `S105_MadHead (Ebene)` | `0x00429810` |
| 13 | `S105_SabHead (Ebene)` | `0x00429810` |
| 14 | `S105_JasHead (Ebene)` | `0x00429810` |
| 15 | `S105_LouHead (Ebene)` | `0x00429810` |
| 16 | `S105_RanHead (Ebene)` | `0x00429810` |
| 17 | `S105_CynHead (Ebene)` | `0x00429810` |
| 18 | `S105_SanHead (Ebene)` | `0x00429810` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00429810.c FUN_00429810 @ 00429810
  Anfang: ruft FUN_00429810
  case 1: Sprache 105_MEG_01 | S= 4090=1 | S? 4090 | ruft CSzene_DisableInput CLayer_Disable CAnimLayer_PlayRange CAnimLayer_Start FUN_00440400 CLayer_Enable CGame_Delay CArea_SetEvent CSzene_EnableInput
  case 11: ruft FUN_00429c80
  case 12: ruft FUN_00429c80
  case 13: ruft FUN_00429c80
  case 14: ruft FUN_00429c80
  case 15: ruft FUN_00429c80
  case 16: ruft FUN_00429c80
  case 17: ruft FUN_00429c80
  case 18: ruft FUN_00429c80
  case 200: ruft CGame_Random
  == 1: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
```
