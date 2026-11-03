# Loc22 – Kristallhöhle: Szene 81 und Steinrätsel 1081

Funktionen `0x00425CF0`–`0x00426A1F` (9 Stück), Ereignis-Handler `0x00426510`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S81_To1081` | `0x00426270` |
| 2 | `S81_To79` | `0x00426270` |
| 3 | `S1081_To81` | `0x00426270` |
| 4 | `*(void **)(*piVar4 + 0x1c0)` | `0x00426270` |
| 4 | `S1081_BigStone (Ebene)` | `0x00426270` |
| 17 | `Gegenstand 17` | `0x004269A0` |
| 18 | `Gegenstand 18` | `0x004269A0` |
| 19 | `Gegenstand 19` | `0x004269A0` |
| 20 | `Gegenstand 20` | `0x004269A0` |
| 21 | `Gegenstand 21` | `0x004269A0` |
| 22 | `Gegenstand 22` | `0x004269A0` |
| 30 | `Gegenstand 16` | `0x00426270` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00426510.c FUN_00426510 @ 00426510
  Anfang: ruft FUN_00426510
  case 1: ruft CActor_WalkTo FUN_004269f0
  == 0: → 22/1081 21/79 | ruft CSzene_SetModal CAnimLayer_PlayEndEvent
  case 2: ruft CActor_WalkTo FUN_004269f0
  case 3: → 22/81
  case 4: ruft CGame_Random
  == 1: ruft CSzene_DisableInput FUN_004403e0 CAnimLayer_PlayEndEvent CSzene_EnableInput
  == 2: ruft FUN_004403e0
  case 15: ruft FUN_00426790
  case 22: ruft FUN_00426840
  case 30: Inv Remove(16) Add(17) Add(18) Add(19) Add(20) Add(22) | ruft CSzene_DisableInput FUN_004403e0 CGame_Delay FUN_004269a0 CSzene_EnableInput
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc22.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3FEC` | 0 | beim Betreten von 81 auf 1: Tharain ist fort (Loc20) |
| `0x4060` | 1 | erster Besuch: nach der Ankunft `081_ROD_01` |
| `0x4064`…`0x4078` | 0 | Plätze 12, 2, 4, 6, 8, 10 Uhr auf der Steinscheibe: Kristall-Nummer (17…22) oder 0 |
| Loc22 +0x128 | 1 | Kommentar `1081_ROD_02` beim nächsten Zurücknehmen (nach jedem Einsetzen wieder 1) |

### Ablauf
- Ctor `0x00425CF0`: Szene 81 (`S81`, Vollbild-Animationen Enter/Exit/To1081, sechs
  Kristallebenen) oder 1081 (`S1081`, Steinscheibe, sechs Hand-Animationen mit Tür-Cue Bild 27,
  Kristallebenen, `S1081_RodStreich`); Musik `Loc22_CrystalCave` mit Hall wie Loc20.
  Drehbereich der Scheibe je Kristall 17…22 (`+0xF8`/`+0x110`): 30–40, 10–20, 20–30, 40–50,
  0–10, 50–60.
- Start `0x00426270`: 81 – Ankunft aus 1081 bei (409,318) Richtung 5; sonst, solange kein
  Kristall liegt, Klettern (`S81_Enter` modal), Held bei (209,341) Richtung 7.
  1081 – Scheibe (Ereignis 4), Kristalle (10…15), Beutel (Gegenstand 16 → `0x1E`),
  Kristalle 17…22 (Ereignis = Nummer, `0x004269A0`), Rückweg (3).
- Ereignisse `0x00426510`: 1 → 1081 (`S81_To1081` nur ohne gelegte Kristalle), 2 → 21/79
  (`S81_Exit` ebenso), 4 zufällig Kommentar mit/ohne Streichen, 10…15 Kristall zurück
  (`0x00426790`), 17…22 Kristall auf den ersten freien Platz (`0x00426840`, Gegenstand aus dem
  zweiten Parameter), `0x1E` Beutel → Kristalle 17, 18, 19, 20, 22 (21 ist Tharains Stein).
- Lösung: 12 Uhr 21, 2 Uhr 22, 4 Uhr 17, 6 Uhr 18, 8 Uhr 20, 10 Uhr 19 →
  `Sfx_DangerStringsHigh`, Scheibe öffnet sich, `CGame_ChangeScene(89, 9)`.
- Probelauf 2026-11-03: 81 und 1081 aufgebaut; Beutel öffnen, Kristall einsetzen, Lösung
  mit dem letzten Kristall → 9/89. Offen: Zurücknehmen, Kommentar an der Scheibe.
