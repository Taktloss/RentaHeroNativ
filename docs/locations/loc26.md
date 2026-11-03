# Loc26 – Insel des Megophias: Strand (Szene 93) und Grube (Szene 98)

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

## Einzelanalyse (2026-11-04, portiert: `src/game/loc26.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3F18` | 0 | Loc08-Zustand; ≠ 0 nötig, damit die Fahne in 98 steckt |
| `0x4094` | 0 | Grube in 98 kommentiert (`098_ROD_01b`; liest Loc27) |
| `0x4098` | 0 | Weg nach 98 frei (setzt Loc27); vorher nur der Wald (`093_ROD_04`) |
| `0x409C` | 1 | erste Ankunft: `093_ROD_01`, Drache `093_DRA_01` (auch Loc16: Abflug-Stufe 6) |
| `0x40A0` | 1 | Fahne steckt noch in 98 (Gegenstand 2) |
| `0x40A4` | 1 | Fahne: 1 erst Kommentar `098_ROD_01`, 2 aufheben (`Gen_RodPickUp` 0–7 / 8–Ende) |

### Ablauf
- Ctor `0x0042A070`: Szene 93 `S93` (Maske 0) mit Meer-Animationen, `S93_DragonTakeoff` (erweiterte
  Ton-Cues Flügel bei 66 und 87), `S93_DragonBreath`, `S93_DragonLanding`, Atem-Timer +0xC0;
  Szene 98 `S98` mit Fahne (A16, Zoom 40). Beide: Möwe/Wind, Meer in Schleife (vtable +0x10),
  Musik `Loc26_IslandOfMegophias` (ohne Hall).
- Start `0x0042A490`, Szene 93: Ankunft aus 58 `S93_DragonLanding` 1–Ende, Held (553,350) Richtung 3
  → (512,355); aus 95 (79,231) Richtung 0 → (93,260); aus 98 (386,160) → (388,170). Drache atmet
  alle 10–20 s (Bilder 1–10).
- Ereignisse 93 `0x0042A8E0`: Hütte → 27/95, Meer/Wald Kommentar an Ort und Stelle, Weg 98, Drache:
  steht Rodrigo schon am Drachen (±10 um (554,349)), steigt er auf → 16/58; sonst Gespräch
  `093_ROD_02`/`093_DRA_02`.
- Ereignisse 98 `0x0042AC00`: → 93, Grube, Fahne, Gegenstände 9/11 → `098_ROD_02`.
- Probelauf 2026-11-04: Nachts 58 (Stufe 5) → Abflug → 93 Landung → Drache zweimal → Abflug → 58
  Landung; aus 93 Hütte → 27/95 (noch nicht portiert); Szene 98 Fahne zweimal (aufgehoben), Grube,
  zurück nach 93. Nicht geprüft: erste Ankunft mit `0x409C`, Gegenstände 9/11.
