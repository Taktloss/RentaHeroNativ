# Loc19 – Esragoth: Waldweg (68, nachts 101) und Zwergentor (69)

Funktionen `0x0041F070`–`0x004223DF` (8 Stück), Ereignis-Handler `0x004202D0`, `0x00420500`, `0x00420D60`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S69_To71` | `0x0041F960` |
| 2 | `S69_To70` | `0x0041F960` |
| 3 | `S69_To68` | `0x0041F960` |
| 4 | `S69_Zwerg1 (Ebene)` | `0x0041F960` |
| 5 | `S69_Wache (Ebene)` | `0x0041F960` |
| 7 | `S69_JasWalkKette (Ebene)` | `0x0041F960` |
| 7 | `this+0x58 (Ebene)` | `0x0041F960` |
| 8 | `S69_RamBackTalk (Ebene)` | `0x0041F960` |
| 10 | `S68_To67` | `0x0041F960` |
| 11 | `S101_To69` | `0x0041F960` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 004202d0.c FUN_004202d0 @ 004202d0
  Anfang: ruft FUN_004202d0
  == 10: Sprache 068_JAS_01 | → 10/38 | S? 3F28 | ruft CActor_WalkTo2 FUN_004403e0 CActor_WalkTo
  == 11: → 19/69 | S? 3F28 | ruft CActor_WalkTo CActor_WalkToClick CSzene_DisableInput CGame_Random
  case 1: ruft CGame_Random CAudioBuffer_SetVolume
  case 2: ruft CGame_Random CAudioBuffer_SetVolume
  case 3: ruft CGame_Random CAudioBuffer_SetVolume
  case 4: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
# 00420500.c FUN_00420500 @ 00420500
  Anfang: ruft FUN_00420500
  == 10: → 10/38 | ruft CActor_WalkTo
  == 11: Sprache 101_ROD_02 | ruft CGame_WalkAndComment CGame_Random
  == 1: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
# 00420d60.c FUN_00420d60 @ 00420d60
  Anfang: ruft FUN_00420d60
  case 1: Sprache 069_RAM_23 069_JAS_23 069_JAS_20 069_ROD_24 069_ROD_17 069_JAS_17 | → 21/71 | S= 401C=1 3FFC=1 4004=2 4008=2 | S? 3F48 4004 | ruft CActor_WalkTo CSzene_DisableInput CActor_WalkTo2 CLayer_Enable FUN_004474c0 FUN_00442f90 FUN_004475b0 FUN_00443310 CActor_Hide CLayer_Disable FUN_00420600 CAnimLayer_PlayRange CGame_Delay CActor_Show CActor_BlockRect CSzene_EnableInput CAnimLayer_PlayEndEvent FUN_004403e0 CAnimLayer_ShowFrame CGame_Random CAudioBuffer_SetVolume
  case 2: Sprache 069_ROD_18 069_JAS_18 069_ROD_11 069_RAM_11 069_JAS_11 | → 20/70 | S= 4008=3 4018=1 4014=0 400C=3 | S? 3FF0 | ruft CActor_WalkTo2 CSzene_DisableInput CAnimLayer_PlayEndEvent FUN_004403e0 FUN_00420600 CSzene_EnableInput CActor_WalkTo CGame_Speak CAnimLayer_PlayRange CLayer_Disable CActor_Show CActor_SetPos CActor_UnblockRect CActor_BlockRect CActor_Hide CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 069_JAS_03 069_ROD_04 069_THA_04 069_ZW2_04 069_ROD_05 069_THA_05 069_ROD_19 069_JAS_19 069_ROD_12 069_RAM_12 | → 20/70 19/68 | S= 3FF0=1 3FF4=0 401C=0xffffffff 4008=4 400C=4 | S? 401C | ruft CActor_WalkTo2 CGame_Speak FUN_004403e0 CSzene_DisableInput CLayer_Enable FUN_004474c0 CAnimLayer_PlayEndEvent FUN_00420600 CLayer_Disable CAnimLayer_PlayRange CActor_UnblockRect CSzene_EnableInput CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 4: Sprache 069_ROD_06 069_ZW1_06 069_RAM_06 069_RAM_07 069_ROD_08 069_ZW1_08 069_ROD_21 069_RAM_21 069_THA_21 069_JAS_21 069_RAM_22 069_ROD_22 069_ROD_16 069_JAS_16 069_ROD_13 069_RAM_13 069_ROD_09 069_RAM_09 069_ROD_10 | S= 401C=2 4010=2 3F48=1 4014=0 401C=0xffffffff 400C=4 400C=2 | S? 3FF4 4010 4018 | ruft CActor_WalkTo CSzene_DisableInput CGame_Speak CActor_WalkTo2 FUN_004474c0 CAnimLayer_Start FUN_004475b0 CLayer_Disable CActor_Hide CAnimLayer_PlayEndEvent CLayer_Enable FUN_004403e0 CGame_Delay CAnimLayer_PlayRange CActor_BlockRect CSzene_EnableInput FUN_00443310 FUN_00442f90 FUN_004409c0 CActor_UnblockRect FUN_00420600 CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
  case 5: Sprache 069_ROD_14 069_ZW2_14 | ruft CActor_WalkTo CGame_Speak
  case 7: S? 3FFC 4008 | ruft CActor_WalkTo
  case 8: S? 400C
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc19.cpp`)

### Flags
| Offset | Start | Bedeutung (aus den Abläufen) |
|---|---:|---|
| `0x3F28` | 0 | Jasmin begleitet Rodrigo (aus Loc10); bei Ankunft in 69 → 0 und `0x4014 = 1` |
| `0x3F48` | 0 | Ende der Ramil-Abfolge (`069_ROD_22`): Jasmin sitzt danach in Szene 40 auf dem Baum (Loc10), Loc09 setzt 0; hier führt der Weg links dann nach 21/71 |
| `0x3FF0` | 0 | Tor zu Szene 70 offen (Thalion) |
| `0x3FF4` | 0 | Zwerg 1 steht vor dem Haus |
| `0x3FFC` | 0 | Jasmin an der Kette ansprechbar |
| `0x4004` | 1 | Ramil-mit-Kette-Auftritt: 1 bereit, 2 erledigt |
| `0x4008` | 1 | Gespräch mit Jasmin an der Kette 1…4 |
| `0x400C` | 1 | Gespräch mit Ramil 1…4 (bei 1 ist die Wache nicht anklickbar) |
| `0x4010` | 1 | Gespräch mit Zwerg 1 (1 → Ramil kommt, 2 danach) |
| `0x4014` | 0 | Jasmin ist in 69 dabei |
| `0x4018` | 0 | Jasmin ist (mit Ramil) gegangen |
| `0x401C` | −1 | Ebenen in 69: 1 Jasmin an der Kette + Ramil, 2 Jasmin/Ramil von hinten, −1 keine |
| `0x4028` | 0 | Weg nach 71 offen (gesetzt in Loc20) |

### Konstruktor `0x0041F070`
- Szene 68 (`S68`, Maske 0) und 101 (`S68` mit Hintergrund `S68_Back_nd`): Vögel, Donner,
  Bereiche `S68_To67` und `S68_To69` bzw. `S101_To69`; Musik `Loc19_Esragoth` bzw.
  `Loc10_ForestInDarkness`.
- Szene 69 (`S69`): `S69_WacheStaunt` als eigene Animation, 22 Ebenen (Jasmin, Ramil, Rodrigo,
  Thalion, Tür, Wache, Zwerg 1, Nahaufnahmen), Bereiche `S69_To71/70/68`, Schritte (Vol. 90)
  als Cues an 7 Ebenen, Türgeräusche. Hall wie Loc01.

### Start `0x0041F960`
- 68: aus 38 bei (4,331) Richtung 6 → (266,325), Jasmin (wenn dabei) (4,312) → (339,318); aus 69
  bei (554,273) Richtung 2. Andere Herkunft ohne Spielstand: Held bleibt unsichtbar (Original).
- 101: Weg rechts gesperrt (`BlockRect`), Ankunft (7,292) → (257,312), `101_ROD_01`.
- 69: Ebenen je `0x401C`. Ankunft aus 68 (631,244); mit Jasmin: Wache fragt, Jasmin antwortet,
  Zwerg 1 kommt heraus (`0x3FF4 = 1`). Aus 70: Tür auf, (332,312), Tür zu. Aus 71: (39,298) →
  (78,311). Wache nur aktiv bei `0x400C ≠ 1`, Tor bei `0x3FF0`, Weg 71 bei `0x4028`.
- Umgebungs-Timer 1000: 68/69 Vögel alle 2–10 s, 101 Donner alle 5–15 s (Lautstärke 50–100).

### Ereignisse Szene 69 `0x00420D60`
| ID | Ablauf |
|---:|---|
| 1 | `0x3F48` → (17,276) → 21/71. `0x4004 = 1`: Ramil bringt Jasmin an der Kette, zwei Nahaufnahmen, Rodrigo starrt; `0x401C = 1`, `0x3FFC = 1`, `0x4004 = 2`. Sonst `069_ROD_24` |
| 2 | Tor offen: (367,311), Tür auf → 20/70 |
| 3 | Jasmin noch da: `069_JAS_03`. Tor zu: Thalion öffnet, Wache staunt (Nahaufnahme), `0x3FF0 = 1` → 20/70. Sonst → 19/68 |
| 4 | Zwerg 1 (nur `0x3FF4`): Stufe 1 Ramil rennt herbei, umarmt Jasmin, `0x401C = 2`, `0x4010 = 2`; sonst `069_ROD_08`/`069_ZW1_08` |
| 5 | Wache: `069_ROD_14`/`069_ZW2_14` |
| 7 | Jasmin an der Kette: Stufen 1–3 Gespräch mit Nahaufnahme; 4 Ramil führt Jasmin ab, Thalion, `0x3F48 = 1`, `0x401C = −1`. Ohne Kette: `069_ROD_16`/`069_JAS_16` |
| 8 | Ramil von hinten: 1 → 2 (Wache wird aktiv), 2 → Jasmin geht (`0x4018 = 1`), 3/4 Nahaufnahme Ramil |

- `0x00420600` Nahaufnahme: Sichtbarkeit (+0x1A4) aller Ebenen, Figuren und des Hintergrunds
  sowie der Bereiche merken, ausblenden, sprechen (Ramil, Jasmin oder Wache nach `WacheStaunt`),
  wiederherstellen; optional `CSzene_InvalidateAll`.
- Tür rückwärts (10 → 0) endet auf Bild 1, weil Bild 0 rückwärts nie gezeigt wird (ANIM).
- Probelauf 2026-11-03: 68 aus 38 mit/ohne Jasmin, 101, 69 aus 68/70/71, Ankunft mit Jasmin,
  Zwerg 1 → Ramil, drei Ramil-Gespräche, Kette (Ereignis 1), vier Kettengespräche mit Finale.
  Spielstände mit gesetzten Flags über `--auto-load`. Offen: Thalion-Abfolge (Ereignis 3).
