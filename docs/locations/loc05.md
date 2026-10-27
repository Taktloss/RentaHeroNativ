# Loc05 (automatische Übersicht)

Funktionen `0x0040D530`–`0x0040E72F` (10 Stück), Ereignis-Handler `0x0040DB30`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S10_Exit` | `0x0040D890` |
| 2 | `S10_Desk` | `0x0040D890` |
| 3 | `S10_Flowers1` | `0x0040D890` |
| 4 | `S10_Flowers2` | `0x0040D890` |
| 5 | `S10_Kitchen` | `0x0040D890` |
| 6 | `S10_Loui` | `0x0040D890` |
| 7 | `S10_Truhe` | `0x0040D890` |
| 8 | `S10_Krug` | `0x0040D890` |
| 9 | `S10_Ofen` | `0x0040D890` |
| 10 | `S10_KastenLinks` | `0x0040D890` |
| 11 | `S10_KastenRechts` | `0x0040D890` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040db30.c Loc05_OnGenericEvent @ 0040db30
  Anfang: ruft Loc05_OnGenericEvent
  case 1: Sprache 028_ROD_01 028_LOU_01 | → 1/9 | ruft CActor_WalkTo FUN_0040e200
  case 2: Sprache 010_ROD_04 028_ROD_02 028_LOU_02 | ruft CGame_WalkAndComment FUN_0040e200
  case 3: Sprache 028_ROD_03 028_LOU_03 | S= 3E84=1 3EA0=1 | S? 3EE0 | ruft FUN_0040e200
  == 1: Sprache 010_ROD_05 028_ROD_05 028_LOU_05 | S= 3EE0=2 | ruft CGame_WalkAndComment FUN_0040e200
  == 2: Sprache 010_ROD_06 028_ROD_06 028_LOU_06 | ruft CGame_WalkAndComment FUN_0040e200
  case 4: Sprache 028_ROD_04 028_LOU_04 | S? 3EE0 | ruft FUN_0040e200
  case 5: Sprache 010_ROD_07 028_ROD_08 028_LOU_08 | ruft CGame_WalkAndComment FUN_0040e200 CGame_Random
  case 6: Sprache 028_ROD_09 028_LOU_09 | ruft FUN_0040e180 FUN_0040e200
  == 3: Sprache 028_ROD_07 028_LOU_07 | ruft FUN_0040e200
  case 7: Sprache 028_ROD_10 028_LOU_10 010_ROD_08 | ruft FUN_0040e200 CGame_WalkAndComment
  case 8: Sprache 028_ROD_11 028_LOU_11 010_ROD_12 | S= 3EDC=9 | ruft FUN_0040e200 CGame_WalkAndComment
  case 9: Sprache 028_ROD_12 028_LOU_12 029_ROD_01 029_LOU_01 029_ROD_02 029_LOU_02 029_ROD_03 029_LOU_03 029_ROD_04 029_LOU_04 010_ROD_11 | S= 3EE4=1 3EDC=5 | ruft CSzene_DisableInput CSzene_GetArea FUN_0040e200 FUN_00440400 FUN_0040e2a0 CLayer_Disable CLayer_Enable CAnimLayer_PlayRange FUN_00435ba0 CGame_Speak FUN_0040e260 CAnimLayer_ShowFrame CSzene_EnableInput CGame_WalkAndComment
  case 10: Sprache 010_ROD_09 | ruft CGame_WalkAndComment
  case 11: Sprache 010_ROD_10 | ruft CGame_WalkAndComment
  case 20: ruft FUN_0040e2a0
```
