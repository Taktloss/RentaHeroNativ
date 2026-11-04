# Loc12 – Brunnen vor Cynthias Haus (Szene 91)

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

## Analyse (2026-10-29, Port `src/game/loc12.cpp`)
Konstruktor `0x00418700` (Szene fest 91, `S91` komplett, Herkunft 90 → Hintergrund `S91_Back2`),
Abbau `0x00418900` gibt alles frei, Start `0x004189E0`, Handler `0x00418BF0`.

- Start: `0x409C == 0` → Brunnen-Stufe `0x3F74 = 4` (`0x409C`: Start 1, Loc26 setzt 0 bei der ersten Landung auf der Insel).
  Herkunft 42 → von links, sonst Kamerafahrt `S91_RodRausCam` aus Cynthias Haus.
- Ereignisse: 1/2 Türen rütteln, 3 → Cynthias Haus (Szene 90, Loc14; Kamerafahrt nur, wenn man
  nicht von dort kam), 4 Brunnen, 5 → Wald (Szene 42, setzt `0x3F60 = 1`), 6 Umgebung (Eule,
  Donner), 7 Geldsack (Gegenstand 7) in den Brunnen → Stufe 3.
- Brunnen nach `0x3F74`: 1 = zwei Sätze, 2 = Hinweis (ohne Gegenstand 7: `0x3FA0 = 1`),
  3 = Gespräch `0x3F78` 1→2→3→4 (Stufe 4 setzt `0x3FBC = 1` und zurück auf 3), 4 = ein Satz.
  Stufe 1 sperrt die Eingabe, die anderen nicht (wie im Original).
