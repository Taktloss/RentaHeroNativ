# Loc06 (automatische Übersicht)

Funktionen `0x0040E730`–`0x0040F7AF` (9 Stück), Ereignis-Handler `0x0040F040`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S26_Exit` | `0x0040EBC0` |
| 2 | `S26_Bottle` | `0x0040EBC0` |
| 3 | `S26_Telescope` | `0x0040EBC0` |
| 4 | `S26_Background` | `0x0040EBC0` |
| 5 | `S26_RanTalk (Ebene)` | `0x0040EBC0` |
| 5 | `S26_Ranama` | `0x0040EBC0` |
| 10 | `S27_Exit` | `0x0040EBC0` |
| 11 | `S27_RanSchautRing (Ebene)` | `0x0040EBC0` |
| 12 | `S27_RanTalkRing (Ebene)` | `0x0040EBC0` |
| 13 | `S27_RanSchaut (Ebene)` | `0x0040EBC0` |
| 13 | `S27_RanTalk (Ebene)` | `0x0040EBC0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040f040.c FUN_0040f040 @ 0040f040
  Anfang: ruft FUN_0040f040
  case 1: ruft FUN_0040f6d0
  case 2: Sprache 026_ROD_07 026_ROD_06 | ruft CGame_WalkAndComment
  case 3: Sprache 026_ROD_08 | ruft CGame_WalkAndComment
  case 4: Sprache 026_ROD_09 | ruft CGame_WalkAndComment
  case 5: Sprache 026_ROD_05 | → 6/27 | S? 3EE8 | ruft FUN_004409c0 CActor_WalkTo
  case 11: Inv Add(14) | ruft CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayRange
  case 10: → 6/26
  case 13: Sprache 027_ROD_01 027_RAN_01 027_ROD_02 027_RAN_02 027_ROD_03 027_RAN_03 027_ROD_04 027_RAN_04 027_ROD_05 | S= 3EF0=2 | S? 3EF0 | ruft FUN_0040f640 FUN_0040f690
  case 20: Sprache 027_RAN_08 027_ROD_06 027_RAN_05 | S= 3EEC=3 | S? 3E8C 3EEC | ruft CSzene_DisableInput CLayer_Disable CAnimLayer_PlayRange CLayer_Enable FUN_0040f690 CSzene_EnableInput FUN_0040f640
  == 1: Sprache 027_ROD_07 027_RAN_06 | S= 3EEC=2 | ruft FUN_0040f640 FUN_0040f690
  == 2: Sprache 027_ROD_08 027_RAN_07 027_ROD_09 | S= 3EEC=3 | ruft FUN_0040f640 FUN_0040f690 CSzene_DisableInput CLayer_Disable CLayer_Enable CAnimLayer_PlayRange CSzene_EnableInput CEvent_SetTimerHandler
```
