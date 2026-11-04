# Loc21 – Zwergenbergwerk, Szenen 71–80

Funktionen `0x00422D90`–`0x00425CEF` (5 Stück), Ereignis-Handler `0x00424600`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 10 | `S71_To69` | `0x00423880` |
| 11 | `S71_To74` | `0x00423880` |
| 12 | `S71_Wall` | `0x00423880` |
| 13 | `S71_Shovel (Ebene)` | `0x00423880` |
| 20 | `S72_To73` | `0x00423880` |
| 21 | `S72_To74` | `0x00423880` |
| 22 | `S72_Wall` | `0x00423880` |
| 30 | `S73_To72` | `0x00423880` |
| 31 | `S73_To75` | `0x00423880` |
| 32 | `S73_Wall` | `0x00423880` |
| 33 | `S73_Drill (Ebene)` | `0x00423880` |
| 34 | `S73_Hebel (Ebene)` | `0x00423880` |
| 35 | `Gegenstand 23` | `0x00423880` |
| 40 | `S74_To71` | `0x00423880` |
| 41 | `S74_To72` | `0x00423880` |
| 42 | `S74_To75` | `0x00423880` |
| 43 | `S74_To77` | `0x00423880` |
| 44 | `S74_Wall` | `0x00423880` |
| 45 | `S74_Hebel (Ebene)` | `0x00423880` |
| 46 | `Gegenstand 11` | `0x00423880` |
| 50 | `S75_To73` | `0x00423880` |
| 51 | `S75_To74` | `0x00423880` |
| 52 | `S75_To76` | `0x00423880` |
| 53 | `S75_Wall` | `0x00423880` |
| 60 | `S76_To75` | `0x00423880` |
| 61 | `S76_Wall` | `0x00423880` |
| 62 | `S76_DriveLever` | `0x00423880` |
| 63 | `S76_DrillLever` | `0x00423880` |
| 64 | `S76_Zunder (Ebene)` | `0x00423880` `0x00424600` |
| 70 | `S77_To74` | `0x00423880` |
| 71 | `S77_To78` | `0x00423880` |
| 72 | `S77_To80` | `0x00423880` |
| 73 | `S77_Wall` | `0x00423880` |
| 80 | `S78_To77` | `0x00423880` |
| 81 | `S78_To79` | `0x00423880` |
| 82 | `S78_Wall` | `0x00423880` |
| 90 | `S79_To78` | `0x00423880` |
| 91 | `S79_To81` | `0x00423880` `0x00424600` |
| 92 | `S79_Wall` | `0x00423880` |
| 93 | `S79_DriveLever` | `0x00423880` |
| 94 | `S79_DrillLever` | `0x00423880` |
| 100 | `S80_To77` | `0x00423880` |
| 101 | `S80_Wall` | `0x00423880` |
| 102 | `S80_PickAxe (Ebene)` | `0x00423880` |
| 103 | `S80_GloomOnFloor (Ebene)` | `0x00423880` `0x00424600` |
| 104 | `Gegenstand 9` | `0x00423880` `0x00424600` |
| 105 | `Gegenstand 11` | `0x00423880` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00424600.c FUN_00424600 @ 00424600
  Anfang: ruft FUN_00424600
  case 10: → 19/69 | ruft CActor_WalkTo
  case 12: Sprache 071_ROD_02 | ruft CGame_WalkAndComment
  case 13: Sprache 071_ROD_01 | Inv Add(11) | ruft CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 22: Sprache 072_ROD_01 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 32: Sprache 073_ROD_01 | ruft CGame_WalkAndComment
  case 33: Sprache 073_ROD_03 073_ROD_02 | S? 4038 | ruft CGame_WalkAndComment
  case 34: Sprache 073_ROD_05 076_ROD_04 | S= 4034=2 | S? 4038 403C | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0 CSzene_DisableInput CLayer_Disable CAnimLayer_Start CGame_Delay CSzene_EnableInput CAnimLayer_PlayRange CLayer_Enable
  case 35: Sprache 073_ROD_02 | S= 4038=1 | Inv Remove(23) | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CAnimLayer_Start CActor_Show FUN_004409c0
  case 40: → 21/71 | ruft CActor_WalkTo
  case 41: → 21/72 | ruft CActor_WalkTo
  case 44: Sprache 074_ROD_02 | ruft CGame_WalkAndComment
  case 45: Sprache 074_ROD_01 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 46: Sprache 074_ROD_03 074_ROD_04 | S= 403C=1 403C=0 4040=2 | S? 403C 4040 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CAnimLayer_Start FUN_00442f90 CActor_Show FUN_004409c0
  case 50: → 21/73 | ruft CActor_WalkTo
  case 52: → 21/76 | ruft CActor_WalkTo
  case 53: Sprache 075_ROD_01 | ruft CGame_WalkAndComment
  case 60: → 21/75 | ruft CActor_WalkTo
  case 61: Sprache 076_ROD_01 | ruft CGame_WalkAndComment
  case 62: S= 4034=1 4034=3 | S? 403C | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CSzene_GetArea CAnimLayer_Start CSzene_EnableInput CActor_WalkTo2
  case 63: Sprache 076_ROD_02 | S? 4044 | ruft CSzene_DisableInput FUN_00440400 CActor_WalkTo CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CAnimLayer_Start CActor_WalkTo2 CGame_Delay
  == 1: Sprache 079_ROD_03 080_ROD_03 | S= 4044=2 404C=2 4058=2 | ruft FUN_00442f90 CAnimLayer_PlayRange CGame_Delay FUN_004409c0 CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show
  == 2: Sprache 076_ROD_03 076_ROD_04 079_ROD_05 080_ROD_04 | S= 4048=1 4044=3 404C=3 | ruft FUN_00442f90 CAnimLayer_PlayRange CArea_SetEvent CLayer_Enable CGame_Delay FUN_004409c0 CSzene_EnableInput CSzene_GetArea CActor_UnblockRect CGame_WalkAndComment
  case 64: Sprache 069_ROD_26 | S= 4048=0 | Inv Add(26) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CSzene_EnableInput FUN_004409c0
  case 70: → 21/74 | ruft CActor_WalkTo
  case 71: → 21/78 | ruft CActor_WalkTo
  case 72: → 21/80 | ruft CActor_WalkTo
  case 73: Sprache 077_ROD_01 | ruft CGame_WalkAndComment
  case 81: → 21/79 | ruft CActor_WalkTo
  case 82: Sprache 078_ROD_01 | ruft CGame_WalkAndComment
  case 90: → 21/78 | ruft CActor_WalkTo
  case 91: → 22/81 | ruft CSzene_DisableInput CActor_WalkTo
  case 92: Sprache 079_ROD_01 | ruft CGame_WalkAndComment
  case 93: Sprache 079_ROD_06 | S= 4034=2 | S? 403C | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CAnimLayer_PlayRange CGame_Delay CSzene_EnableInput FUN_004409c0 CSzene_GetArea CAnimLayer_Start
  case 94: Sprache 079_ROD_02 | S? 404C | ruft CSzene_DisableInput FUN_00440400 CActor_WalkTo CActor_Hide FUN_004474c0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Disable CAnimLayer_Start CGame_Delay
  == 3: Sprache 079_ROD_06 | ruft FUN_004409c0 CSzene_EnableInput
  case 100: → 21/77 | ruft CActor_WalkTo
  case 101: Sprache 080_ROD_02 080_ROD_01 | S= 405C=1 | S? 4054 | ruft CGame_WalkAndComment CActor_WalkTo CSzene_DisableInput CGame_Delay FUN_00435ba0 CSzene_EnableInput FUN_004409c0
  case 102: Sprache 080_ROD_07 | Inv Add(9) | ruft CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CActor_Show FUN_004409c0
  case 103: S? 4058
  case 104: Sprache 080_ROD_06 | S= 4054=2 | S? 4054 405C | ruft CGame_DescribeItem CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CLayer_Enable CArea_SetEvent CActor_Show FUN_004409c0
  case 105: Sprache 080_ROD_05 | S= 4054=3 | S? 4054 | Inv Add(23) | ruft CGame_DescribeItem CActor_WalkTo CActor_Hide CSzene_DisableInput CAnimLayer_PlayRange CLayer_Disable CActor_Show FUN_004409c0 CSzene_EnableInput
  case 200: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc21.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x4034` | 1 | Bohrer steht in 73 (1), 76 (2) oder 79 (3) |
| `0x4038` | 0 | Gegenstand 23 (Stein aus 80) im Kessel des Bohrers in 73 – erst dann fährt er |
| `0x403C` | 0 | Weiche in 74 umgestellt (Schaufel, Gegenstand 11): 73 → 76 nur bei 0, 76 → 79 nur bei 1 |
| `0x4040` | 1 | Kommentar zur Weiche (`074_ROD_03` einmal, dann `074_ROD_04`) |
| `0x4044` | 1 | Schutt in 76: 1 ganz, 2 halb, 3 weg (legt Zunder frei) |
| `0x4048` | 0 | Zunder (Gegenstand 26) liegt in 76 |
| `0x404C` | 1 | Schutt in 79; bei 3 Ausgang nach 22/81, vorher Sperrrechteck (414,200)–(639,359) |
| `0x4054` | 1 | Stein in 80: 1 in der Wand, 2 am Boden (heiß), 3 mit der Schaufel genommen (Gegenstand 23) |
| `0x4058` | 1 | Stein am Boden angefasst (`080_ROD_03`, danach `080_ROD_04`) |
| `0x405C` | 0 | Wand in 80 zweimal abgetastet – erst dann hackt die Spitzhacke (Gegenstand 9) |
| Loc21 +0xD8 | 0 | Wand in 80 einmal abgetastet (nur solange die Szene lebt) |

### Ablauf
- Ctor `0x00422D90`: `wsprintf("S%d")` + `CSzene_LoadAll`; Töne Kurbel, Bohrer, Tür, Flügel,
  Explosion, Tropfen; je Szene Ebenen und Ton-Cues (Bohrer entfernt sich: 8 erweiterte Cues
  mit Lautstärke 90…30 und z 0.1…0.4; Schutt zweimal Bild 30 wie im Original). Musik
  `Loc21_DwarvesMine`, Hall wie Loc07, kein `0x004375B0`.
- Start `0x00423880`: Bereiche je Szene, Ankünfte mit Richtungen aus dem Disasm; Held wird
  nur nach dem Laden eines Spielstands per `CActor_Show` gezeigt.
- Ereignisse `0x00424600`: Wege (`CGame_ChangeScene(ziel, 21)`, 71 → 19/69, 79 → 22/81),
  Wandkommentare, Schaufel, Wand abtasten (72, 80), Weiche, Kessel, Hebel in 73, Fahr- und
  Bohrhebel in 76/79 (`Gen_RodGreif` per `0x004474C0` an (336,156,18,106) bzw. (370,158,9,119)),
  Zunder, Spitzhacke, Stein; Tropfen alle 5–15 s (Ereignis 200).
- Original-Eigenheiten (nachgebildet): Bohrhebel ruft `CGame_Wait` auch ohne vorheriges
  `BeginWait`; in 79 wartet er nicht auf den Weg zum Platz (Platz bleibt belegt).
- Probelauf 2026-11-03: alle zehn Szenen aufgebaut; Hebel in 73 (Bohrer fährt los), Bohrhebel
  76 (Schutt 2 → 3), Zunder, Bohrhebel 79 (Schutt 2 → 3) und Weg nach 22/81.
  Offen: Weiche, Wand/Spitzhacke/Stein in 80, Kessel.

## `Sfx_Drip` fehlt in den Daten (2026-11-04)

Das Original lädt `Sfx_Drip` (String in der EXE), aber weder CD noch Common enthalten eine
solche WAV-Datei (`tools/name_check.py`). Der Puffer bleibt leer, das Tropfen ist also auch
im Original stumm. Der Port verhält sich genauso.
