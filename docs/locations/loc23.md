# Loc23 (automatische Übersicht)

Funktionen `0x00426A20`–`0x00427C0F` (5 Stück), Ereignis-Handler `0x004275D0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S102_To100` | `0x00427100` |
| 2 | `S102_To103` | `0x00427100` |
| 3 | `S102_KisteLinks (Ebene)` | `0x00427100` |
| 3 | `S102_KisteRechts (Ebene)` | `0x00427100` |
| 4 | `SabWalk (Ebene)` | `0x00427100` |
| 5 | `S102_TuerZu (Ebene)` | `0x00427100` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 004275d0.c FUN_004275d0 @ 004275d0
  Anfang: ruft FUN_004275d0
  case 1: Sprache 102_SAB_05
  case 2: → 24/103 | S? 407C | ruft CActor_WalkTo
  case 3: Sprache 102_ROD_02 | S= 407C=2 407C=1 | S? 407C 4080 | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CLayer_Enable CActor_Show CActor_SetPos FUN_004409c0 CActor_UnblockRect CSzene_EnableInput CActor_WalkTo2 CActor_BlockRect
  case 4: Sprache 102_SAB_03 102_SAB_04 | S= 4080=1 | ruft CActor_WalkTo CSzene_DisableInput FUN_004403e0 CAnimLayer_PlayRange FUN_004475b0 CAnimLayer_ShowFrame CLayer_Disable CAnimLayer_PlayEndEvent CLayer_Enable CActor_BlockRect CSzene_EnableInput
  case 5: Sprache 102_ROD_01 | S? 407C 4080 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
```
