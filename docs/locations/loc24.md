# Loc24 – Kapitänskajüte, Szene 103

Funktionen `0x00427C10`–`0x0042901F` (8 Stück), Ereignis-Handler `0x00428820`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S103_Ofen` | `0x004282D0` |
| 2 | `S103_Hebel` | `0x004282D0` |
| 3 | `S103_Instrumente` | `0x004282D0` |
| 4 | `S103_To102` | `0x004282D0` |
| 5 | `S103_Swords` | `0x004282D0` |
| 6 | `S103_Tisch (Ebene)` | `0x004282D0` |
| 7 | `S103_Flasche (Ebene)` | `0x004282D0` |
| 20 | `S103_CapAttack1 (Ebene)` | `0x004282D0` |
| 20 | `S103_CapAttack2 (Ebene)` | `0x004282D0` |
| 20 | `S103_CapAttack3 (Ebene)` | `0x004282D0` |
| 20 | `S103_CapDecktUnten (Ebene)` | `0x004282D0` |
| 21 | `S103_SabAttack2 (Ebene)` | `0x004282D0` |
| 21 | `S103_SabDeckt1 (Ebene)` | `0x004282D0` |
| 21 | `S103_SabDeckt2 (Ebene)` | `0x004282D0` |
| 22 | `S103_Helm (Ebene)` | `0x004282D0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00428820.c FUN_00428820 @ 00428820
  Anfang: ruft FUN_00428820
  case 1: Sprache 103_ROD_08 | ruft CGame_WalkAndComment
  case 2: Sprache 103_ROD_10 | ruft CGame_WalkAndComment
  case 3: Sprache 103_ROD_09 | ruft CGame_WalkAndComment
  case 4: Inv Has(5)
  == 0: Sprache 103_ROD_05 | → 23/102 | S= 4088=1 | ruft CGame_WalkAndComment CActor_WalkTo2
  case 5: Sprache 103_ROD_11
  case 6: Sprache 103_ROD_04 | ruft CGame_WalkAndComment CGame_Random CAudioBuffer_FadeFreq
  case 7: Inv Add(24) | ruft CActor_WalkTo2 CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show
  case 10: ruft CGame_Random FUN_00428db0
  case 11: ruft FUN_00428f90 CAnimLayer_PlayRange CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
  case 12: ruft FUN_00442f90
  case 13: ruft CAnimLayer_PlayRange
  case 15: ruft FUN_00428f90 CAnimLayer_PlayRange
  case 16: ruft FUN_00442f90
  case 17: ruft CAnimLayer_PlayRange
  case 18: S= 408C=1 | ruft FUN_00428db0 FUN_00446ec0
  case 20: Sprache 103_SAB_02 | ruft FUN_004403e0
  case 21: Sprache 103_ROD_03 | ruft CGame_WalkAndComment
  case 22: Inv Add(5) | ruft CActor_WalkTo2 CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CActor_Show
  case 30: Sprache 103_SAB_07 | ruft FUN_004403e0 CActor_UnblockRect
  case 100: ruft CGame_Random
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc24.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x4088` | 0 | Rodrigo ist mit dem Helm über die Treppe hinauf (liest Loc08) |
| `0x408C` | 0 | Kampf ist nach rechts gerückt (Sabrina-Ebenen +56, Kapitän-Ebenen +36 per `0x00446EC0`) |
| Loc24 +0x110 | 0 | Kampfrunde: 0, 1, 2 (nach jeder Runde zufällig neu), 3 sobald Rodrigo den Helm hat |

### Ablauf
- Ctor `0x00427C10`: `CSzene_LoadAll("S103")`; Tisch, Helm, Flasche statisch; Kapitän-, Sabrina-
  und Rodrigo-Animationen mit Ton-Cues; Umgebungs-Timer; Musik `Loc23_PirateShip`, Hall wie Loc14.
- Start `0x004282D0`: Dampf in Schleife, Umgebungston alle 1–3 s (Ereignis 100: Meeresrauschen
  Frequenz, Möwe, Donner), Flasche (Gegenstand 24 → 7), Helm (Gegenstand 5 → `0x16`).
  Erste Ankunft: Rodrigo (142,264) Richtung 6 → (287,340); Sabrina folgt als `SabWalk`
  (`0x004474C0`/`0x004475B0`), zieht ihr Schwert; der Kapitän kommt (`CapComes`, Z danach 115 per
  `0x00446FD0`), `103_CAP_01`, `103_SAB_01`, erster Angriff. Dann Kampfrunde (`0x00428DB0`),
  Bereiche, Sperrrechteck (114,253)–(300,310) solange ohne Helm.
- Kampf: Runden 0–2 spielen Angriffs-/Parade-Animationen mit Ende- bzw. Schrittzähler-Ereignissen
  (`0x00443300`/`0x00443530`: Schritt 8); Ereignis 10 beendet eine Runde. Runde 3 (mit Helm):
  `103_CAP_06` (Ende → `0x1E`: `103_SAB_07`, Sperre weg), Kapitän stolpert (`0x12`): Kampf
  rückt nach rechts, `0x408C = 1`, wieder Runde 0.
- Ereignisse `0x00428820`: Ofen/Hebel/Instrumente/Schwerter/Tisch kommentieren; Treppe (4) ohne
  Helm `103_ROD_05`, mit Helm `0x4088 = 1` → 23/102; Flasche; Helm (`round = 3`).
- Probelauf 2026-11-03: Ankunft und Kampf, Helm nehmen, Kapitän stolpert, Sperre weg, Treppe →
  23/102 → Flucht → 8/100. Flasche angeklickt (Weg), Aufnehmen nicht einzeln geprüft.
