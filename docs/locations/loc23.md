# Loc23 – Piratenschiff, Szene 102

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

## Einzelanalyse (2026-11-03, portiert: `src/game/loc23.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x407C` | 1 | Kiste: 1 links (Sperrrechteck, Tür nicht anklickbar), 2 rechts (Luke frei) |
| `0x4080` | 0 | Sabrina ist aus der Kajüte (Tür offen, Luke nach 103 aktiv) |
| `0x4084` | 0 | nur `CGameState_Reset` schreibt (0), einziger Leser `0x00427919`: toter Zweig; ≠ 0 → Klick auf Sabrina ohne Wirkung |

### Ablauf
- Ctor `0x00426A20`: `CSzene_Load("S102", 0)`; Vollbild `S102_Escape`/`S102_EnterShip`; Kisten,
  Tür, Sabrina (`SabWalk`, Schritt-Cues), Rütteln; Töne; Musik `Loc23_PirateShip` mit Hall wie
  Loc09.
- Start `0x00427100`: Meeresrauschen in Schleife. Aus 100: Bereiche (1 zurück, 2 Luke, 3 Kisten,
  4 Sabrina, 5 Tür); Ankunft: `S102_EnterShip` modal, Sabrina an der Tür (358,151,1,238) Bild 52,
  Rodrigo als freie Ebene (Held aus der Szene genommen, `0x0044A6E0`/`0x0044AA00`) von (−89,167)
  nach (46,86) in 30 Schritten, dann wieder Held bei (131,339) Richtung 4. Die Schritt-Cues
  werden dem Helden bei jeder Ankunft erneut angehängt (Original). Aus 103: `S102_Escape` modal,
  dann 8/100.
- Ereignisse `0x004275D0`: 1 `102_SAB_05`; 2 Luke → 24/103 (nur Kiste rechts); 3 Kiste hin und
  her; 4 Sabrina – bei Kiste rechts kommt sie heraus (Wege per `0x004475B0`, Tür `S102_SabTuer`),
  sonst `102_SAB_04`; 5 Tür rütteln (`102_ROD_01`).
- Probelauf 2026-11-03: Ankunft aus 100, Kiste, Sabrina kommt heraus, Luke → 24/103; Flucht → 8/100.
