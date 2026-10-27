# Loc31 (automatische Übersicht)

Funktionen `0x00430360`–`0x00430A1F` (6 Stück), Ereignis-Handler `0x00430770`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S113_To8` | `0x00430670` |
| 2 | `S113_To37` | `0x00430670` |
| 3 | `S113_To57` | `0x00430670` |
| 4 | `S113_To30` | `0x00430670` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00430770.c FUN_00430770 @ 00430770
  Anfang: ruft FUN_00430770
  case 1: → 1/8 | ruft FUN_00430990
  case 2: → 10/37 | ruft FUN_00430990
  case 3: → 15/57 | ruft FUN_00430990
  case 4: → 28/31 | ruft FUN_00430990
  case 200: ruft CGame_Random
  == 1: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
```
