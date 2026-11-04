# Loc04 – Rodrigos Büro (Szene 6)

Funktionen `0x0040B8C0`–`0x0040D52F` (12 Stück), Ereignis-Handler `0x0040C570`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `006_Desktop` | `0x0040C010` |
| 2 | `006_Eber` | `0x0040C010` |
| 3 | `006_Kiste` | `0x0040C010` |
| 4 | `006_Elefant` | `0x0040C010` |
| 5 | `006_Kasten` | `0x0040C010` |
| 6 | `006_Door` | `0x0040C010` |
| 7 | `006_DoorRamil` | `0x0040C010` |
| 8 | `006_NewsRamil` | `0x0040C010` |
| 9 | `006_FassRamil` | `0x0040C010` |
| 10 | `RamTalk (Ebene)` | `0x0040C010` |
| 11 | `006_Fass` | `0x0040C010` |
| 12 | `006_News` | `0x0040C010` |
| 13 | `006_Lade2` | `0x0040C010` |
| 14 | `006_Lade3` | `0x0040C010` |
| 15 | `006_Lade4` | `0x0040C010` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040c570.c FUN_0040c570 @ 0040c570
  Anfang: ruft FUN_0040c570
  case 1: Sprache 006_ROD_04 006_ROD_03 006_ROD_05 | S= 3ED4=0 | S? 3ED4 | Inv Add(7) | ruft FUN_0040d180 FUN_0040cf00 FUN_0040d300 CSzene_DisableInput CActor_WalkTo CSzene_EnableInput CActor_Hide CAnimLayer_PlayRange CActor_Show CActor_Turn FUN_00440400
  case 2: Sprache 006_ROD_07 006_ROD_06 006_ROD_16 006_RAM_06 | ruft FUN_0040d3b0 FUN_0040d400
  case 3: Sprache 006_ROD_10 006_ROD_17 006_RAM_07 006_ROD_18 | ruft FUN_0040d3b0 FUN_0040d400 FUN_0040d4e0 FUN_0040cf60
  case 4: Sprache 006_ROD_09 006_ROD_08 006_ROD_19 006_RAM_08 | ruft FUN_0040d3b0 FUN_0040d400
  case 5: Sprache 006_ROD_11 006_ROD_20 006_RAM_09 | ruft FUN_0040d300 FUN_0040d400
  case 6: Sprache 006_ROD_21 006_RAM_10 | → 1/7 | S= 3ED4=1 | ruft FUN_0040d300 CActor_WalkTo CSzene_DisableInput FUN_0044a6e0 FUN_0044aa00 FUN_004474c0 FUN_00442f90 CAnimLayer_SetFrameEvent FUN_004475b0 FUN_0044aa70 CSzene_SetModal CAnimLayer_PlayRange CSzene_EnableInput FUN_0040d400 FUN_004403e0 FUN_0040cf60
  case 7: Sprache 006_RAM_02 006_ROD_12 006_ROD_22 006_RAM_11 | ruft CSzene_DisableInput CLayer_Disable CLayer_Enable FUN_00442f90 FUN_0040d4e0 CAnimLayer_PlayRange FUN_0040d400 CSzene_EnableInput
  case 8: Sprache 006_ROD_27 006_ROD_23 006_RAM_12 | ruft CSzene_DisableInput FUN_0040d400 CSzene_EnableInput
  case 9: Sprache 006_ROD_26 006_ROD_24 006_ROD_25 006_ROD_13 006_RAM_03 006_ROD_14 006_RAM_04 006_ROD_15 006_RAM_05 | S= 3E84=2 3E88=1 | ruft CSzene_DisableInput FUN_0040d400 CSzene_EnableInput CActor_WalkTo CActor_Hide CSzene_SetModal CAnimLayer_PlayRange FUN_0040cf60 CSzene_SetClickEvent FUN_0040d4e0
  case 11: Sprache 006_ROD_26 | ruft FUN_0040cf00
  case 12: Sprache 006_ROD_27 | ruft FUN_0040cf00
  case 13: Sprache 006_ROD_28 | ruft FUN_0040d300
  case 14: Sprache 006_ROD_29 | ruft FUN_0040d300
  case 15: Sprache 006_ROD_30 | ruft FUN_0040d300
  case 19: ruft CAnimLayer_SetFrameEvent
  case 30: ruft FUN_0040d300 FUN_0040d3b0
```

## Analyse (2026-10-28, Port `src/game/loc04.cpp`)
Szene immer 6. Konstruktor `0x0040B8C0`: Sprungziele (Vorgänger-Szene) setzen Flags:

| Vorgänger | `0x3ED4` Sack in Kiste | `0x3E84` Ramil | `0x3ED0` Besuche |
|---|---|---|---|
| 1001 | 0 | 0 | 0 |
| 1002 | 0 | 0 | 1 |
| 1003 | 1 | – | 2 |
| 1004 | 0 | 1 | 3 |

Laden: bei `0x3E84 == 1` oder `0x3ED0 == 0` `006_rodtalk` (Nahaufnahme, `RodTalk`); bei
`0x3E84 == 1` zusätzlich `006_ramtalk` (`RamTalk`, `RodSitAct1/2`, Bereiche `*Ramil`,
Animationen `RamilRein/Raus/Geld`, Maske 3). `0x3ED4 != 0` → Maskenbit 2 (`RodTake`). Dann
Hintergrund und Tiefenkarte lösen und `006` laden. Jede SZN liefert eigenen Hintergrund
(`+0x11C/+0x120/+0x124`) und Tiefenkarte (`+0x12C/+0x128`). Musik `Loc04_RodrigosOffice`,
Hall (4, 0.208, 0.478, 0), Anteil 0.1. Inventar-Rückruf `0x0040CF50` = `CGame_DescribeItem`.

Klänge `+0xE8…+0x100`: Kramen 50, MoneyBag 70, Door_CreakShort 50, Tock 70, StepLeft/Right 75,
TockSoft.

### Start `0x0040C010`
`0x3ED0 += 1`. Ohne Spielstand: Tür (`RodRein` 0–17, 18–Ende), dann Held aus `+0x5E0` lösen,
als Ebene einfügen, `SetPosZ(442, 109, 5, 115)`, Bilder 99–101, Schritt-Bildereignisse 18/19
(Bilder 51/57), `MoveTo(223, 122, –, 72)` in 30 Schritten, zurück als Held, `SetPos(300, 350, 3)`.
- `0x3E84 == 1`: hinsetzen (Ramil-Ansicht), Ramil kommt (`RamilRein`), läuft nach (130, 292),
  `006_RAM_01`, Gesprächsstufe `+0x118 = 1`.
- erster Besuch (`0x3ED0 == 1`): Nahaufnahme, `006_ROD_01`, 1 s, Schreibtisch, Klick-Ereignis 30.
- sonst `WalkToClick(300, 350)`.

### Hilfsfunktionen
- `0x0040CF60` Ansicht wechseln (Hintergrund, Tiefenkarte, Bereiche; `+0x104` = am Tisch).
  Vergleicht mit `CGame+0x60`; Port: Hintergrund der Szene (immer gleich gesetzt).
- `0x0040D180` hinsetzen (0 Nahaufnahme, 1 Tisch, 2 Ramil), `0x0040D300` aufstehen
  (`RodSetzenLayer` 20→0, `SetPos(173, 247, 6)`).
- `0x0040CF00`/`0x0040D3B0` Kommentar am Tisch (`RodDeskTalk`) oder hingehen.
- `0x0040D400` Rodrigo spricht (0 Nahaufnahme, 1 Tisch, 2 Ramil-Ansicht), `0x0040D4E0` Ramil.

### Ereignisse (`0x0040C570`)
1 Schreibtisch (abwechselnd ROD_03/04, beim 2. Mal hinsetzen; mit Sack: Kiste leeren →
`ROD_05`, `0x3ED4 = 0`, Gegenstand 7) · 2/4 abwechselnde Kommentare · 6 Tür: Held als freie
Ebene `SetPosZ(223, 122, 18, 72)` → `MoveTo(465, 100, –, 125)`, `RodRaus`, → Szene 7 Eingang 1 ·
7–9 Ramil-Ansicht · 10 Gespräch mit Ramil in Stufen (Default/1: `0x3E88 = 1`; 6: Geldsack,
`0x3ED4 = 1`; 9: Ramil geht, `0x3E84 = 2`) · 16–19 Schrittklänge mit Folge-Bildereignis
(3/9/51/57) · 30 Klick ins Leere: aufstehen.

### Engine
`CSzene_Update` ruft in der Ebenenschleife bei `+0x1BC` direkt `CAnimLayer_Update`
(`0x0044B6A6`) statt vtable `+0x18`. Eine Figur in der Ebenenliste wird so nur animiert und
bewegt (Tween), nicht gelaufen. fps 0 ohne Tween: kein Update (Disassembly; Ghidras
Dekompilat zeigt das falsch).

- Probelauf 2026-11-04 (`--game 4 6 1004`): Ramil-Besuch komplett, danach Geldsack per Schreibtisch.
