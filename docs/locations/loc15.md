# Loc15 – Kanal-Schlucht im Wald, Szene 57

Funktionen `0x0041A330`–`0x0041B28F` (6 Stück), Ereignis-Handler `0x0041ACF0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S57_Kanal` | `0x0041A8B0` |
| 2 | `S57_To38` | `0x0041A8B0` |
| 3 | `S57_To58` | `0x0041A8B0` |
| 4 | `S57_Schlucht` | `0x0041A8B0` |
| 5 | `S57_Gleiter (Ebene)` | `0x0041A8B0` |
| 6 | `S57_Sack (Ebene)` | `0x0041A8B0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041acf0.c FUN_0041acf0 @ 0041acf0
  Anfang: ruft FUN_0041acf0
  case 1: → 7/53 | S= 3EFC=0 3F00=0 | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayEndEvent CSzene_EnableInput CGame_Random CAudioBuffer_SetVolume
  case 2: → 10/38 | S= 3F38=1 | ruft CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 057_ROD_02 | → 16/58 | S= 3F94=0 | S? 3F94 | ruft CGame_HeroSayWait CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 4: Sprache 057_ROD_01 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler CAudioBuffer_SetVolume
  case 5: → 31/113 | S= 3F9C=0 | ruft CActor_WalkTo CSzene_DisableInput CLayer_Disable CActor_Hide CAnimLayer_PlayRange CAudioBuffer_SetFreq CAudioBuffer_FadeFreq
  case 6: Sprache 057_ROD_03 | S= 3FA0=0 | Inv Add(7) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CSzene_EnableInput CGame_HeroSayWait
  case 11: S? 3E80 | ruft CGame_Random
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc15.cpp`)

### Konstruktor `0x0041A330`
- `+0x28 = 15`, Szene aus der Anfrage. Varianten: 571 (`0x23B`) → Nacht `0x3E80 = 1`,
  570 (`0x23A`) → Tag `0x3E80 = 0`, 572 (`0x23C`) → Pirat `0x3F90 = 1`; danach Szene fest 57.
- Tag (`0x3E80 = 0`): Ladebit 1, Vögel/Zirpen, `Sfx_DangerStringsLow`,
  `Sfx_Glider_PassingBy` (Vol. 70); Ankunft aus 113 (Flugkarte) → Gleiter steht `0x3F9C = 1`.
- Nacht: Ladebit 2, Eule/Donner, `0x3F9C = 0`; Ankunft aus 113 wird zu 58.
- Herkunft 53 → Ladebit 8, sonst `0x3F90 = 0`. Gleiter steht → Ladebit 4.
- Tag: Ebenen `S57_RodRein_vd`, `S57_RodRaus_vd`, `S57_RodSchaut`, `S57_GleiterWeg`,
  `S57_RodAbsteig`, `S57_Gleiter`, `S57_GleiterHin`, `S57_PiratGuck`; Held-Farbe (−10, −34, −52);
  Musik `Loc10_ForestBeforeDarkness`.
- Nacht: Hintergrund `S57_Back_nd`, `S57_RodRein_nd`/`_nd`, `Gen_RodPickUp`, `S57_Sack`;
  Farbe (−122, −84, −24) für Held und Aufheben; Musik `Loc10_ForestInDarkness`.
- `Sfx_Clothes_Scratched` (Vol. 70) als Cue Bild 9 (Rein) bzw. 3 (Raus).
- Gleiter steht → `CActor_BlockRect(115, 316, 280, 324)`. Hall wie Loc10/Loc12.

### Start `0x0041A8B0`
- Umgebungs-Timer ID 11 (5–15 s), Bereiche 1–4; Geldsack `0x3FA0` → Ebene an, Ereignis 6;
  Gleiter steht → Ereignis 5, Ebene an, Schaukeln `0x0041B200` (y 207 ↔ 209, je 2 Schritte,
  Funktionszeiger als Handler wie Loc01).
- Ankunft (nicht geladen): aus 38 bei (212,178) Richtung 0 → (273,331); aus 53 Kletter-Animation
  `RodRaus`, dann (40,322) Richtung 6 → (100,322); aus 113 Landung (`GleiterHin` fps 20 ab
  (585,128), Flug nach (227,198) in 50 und (127,208) in 40 Schritten, 500 ms, `RodAbsteig`,
  Held (205,310) Richtung 0); sonst (570,300) Richtung 2 → (453,319).
- `0x3F90` gesetzt → einmalig Pirat: `0x3F98 = 1`, `PiratGuck` an, Streicher, nach x −10 (10
  Schritte), Animation, 300 ms, nach x −30 (5 Schritte), aus. Sonst Schlucht-Bereich inaktiv
  (`+0xB8 = 0`) – UNVERIFIED: damit ist „in die Schlucht schauen“ (Ereignis 4) nur nach dem
  Piraten-Auftritt anklickbar; Probelauf im Original steht aus.

### Ereignisse `0x0041ACF0`
| ID | Ablauf |
|---:|---|
| 1 | Kanal: (44,323) Richtung 2, `RodRein`, Schaukeln aus; Gleiter (57 oder 37) oder Nacht → `0x3F04 = 1`, `0x3EFC = 0x3F00 = 0`; → 7/53 |
| 2 | (212,178) Richtung 4, `0x3F38 = 1` → 10/38 |
| 3 | einmalig `057_ROD_02` (`0x3F94`), (570,300) Richtung 6 → 16/58 |
| 4 | (109,306) Richtung 6, `RodSchaut`, `057_ROD_01` |
| 5 | (205,310) Richtung 0, Gleiter weg: Bilder 0–48, Ton (Frequenz 25 → 100 in 2 s), ab 49 → 31/113 |
| 6 | (20,356) Richtung 4, Aufheben Bilder 0–7, Sack aus, Gegenstand 7, `0x3FA0 = 0`, Rest, `057_ROD_03` |
| 11 | Umgebungston (Nacht: Eule 50 %, Donner 1/2; Tag: Vogel 1–4, Lautstärke 50–100), neu 5–10 s |

- Probelauf 2026-11-03: Ankunft aus 38, 53 (mit Pirat), 113 (Landung) und nachts aus 58;
  Klick auf den Gleiter → 31/113. Aufheben des Geldsacks (Ereignis 6) noch nicht im Probelauf.
