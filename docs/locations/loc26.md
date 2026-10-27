# Loc26 (automatische Übersicht)

Funktionen `0x0042A070`–`0x0042AE1F` (6 Stück), Ereignis-Handler `0x0042A8E0`, `0x0042AC00`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S93_Huette` | `0x0042A490` |
| 2 | `S93_Meer` | `0x0042A490` |
| 3 | `S93_Wald` | `0x0042A490` |
| 4 | `S93_To98` | `0x0042A490` |
| 5 | `S93_Dragon` | `0x0042A490` |
| 10 | `S98_To93` | `0x0042A490` |
| 11 | `S98_Grube` | `0x0042A490` |
| 12 | `S98_Fahne (Ebene)` | `0x0042A490` |
| 13 | `Gegenstand 11` | `0x0042A490` |
| 13 | `Gegenstand 9` | `0x0042A490` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0042a8e0.c FUN_0042a8e0 @ 0042a8e0
  Anfang: ruft FUN_0042a8e0
  case 1: → 27/95 | ruft CActor_WalkTo
  case 2: Sprache 093_ROD_03
  case 3: Sprache 093_ROD_04
  case 5: Sprache 093_ROD_02 093_DRA_02 | → 16/58 | ruft CActor_WalkTo CLayer_Disable CActor_Hide CAnimLayer_PlayRange CGame_Speak
  case 6: ruft CAnimLayer_PlayRange CGame_Random CEvent_SetTimerHandler
  == 1: ruft CGame_Random CAudioBuffer_SetVolume
  == 2: → 26/98 | S? 4098 | ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler CActor_WalkTo
# 0042ac00.c FUN_0042ac00 @ 0042ac00
  Anfang: ruft FUN_0042ac00
  == 12: Sprache 098_ROD_01 | S= 40A0=0 | Inv Add(2) | ruft CGame_WalkAndComment CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CSzene_EnableInput
  == 10: → 26/93 | ruft CActor_WalkTo
  == 11: Sprache 098_ROD_01b | S= 4094=1 | ruft CGame_WalkAndComment
  == 13: Sprache 098_ROD_02 | ruft CGame_Random
  == 2: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
```
