# Loc14 – bei Cynthia, Szene 90

Funktionen `0x00419650`–`0x0041A32F` (6 Stück), Ereignis-Handler `0x00419C90`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S90_Krankenbett` | `0x004199C0` |
| 2 | `S90_Doppelbett` | `0x004199C0` |
| 3 | `S90_Blumen` | `0x004199C0` |
| 4 | `S90_Exit` | `0x004199C0` |
| 5 | `S90_Trog` | `0x004199C0` |
| 6 | `S90_Regale` | `0x004199C0` |
| 10 | `S90_KaminArea` | `0x004199C0` |
| 11 | `S90_CynTalk (Ebene)` | `0x004199C0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00419c90.c FUN_00419c90 @ 00419c90
  Anfang: ruft FUN_00419c90
  case 1: Sprache 090_ROD_11 090_ROD_02 090_CYN_02 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0041a300
  case 2: Sprache 090_ROD_12 090_CYN_12 090_ROD_16 090_ROD_13 090_CYN_13 090_ROD_14 090_CYN_14 090_ROD_15 090_CYN_15 090_ROD_03 090_CYN_03 | S= 3F8C=2 3F8C=3 | ruft CActor_WalkTo CGame_HeroSayWait FUN_00435ba0 FUN_0041a300 CActor_Turn CSzene_DisableInput CGame_PlayMusic FUN_00437fd0 FUN_00437560 FUN_004375b0 FUN_004406e0 CMusic_FadeIn CSzene_EnableInput
  case 3: Sprache 090_ROD_17 090_CYN_17 090_ROD_04 090_CYN_04 | S= 3F80=1 3F88=4 | ruft CActor_WalkTo CGame_HeroSayWait FUN_00435ba0 FUN_0041a300 CSzene_DisableInput CActor_Turn CGame_Delay CSzene_EnableInput
  case 4: Sprache 090_ROD_21 090_CYN_21 090_ROD_05 090_CYN_05 090_ROD_06 090_ROD_07 | → 12/91 | S= 3F88=5 3F7C=1 | ruft CActor_WalkTo CGame_HeroSayWait FUN_00440400 CSzene_DisableInput FUN_0041a300 CActor_Turn CSzene_EnableInput
  case 5: Sprache 090_ROD_22 090_ROD_08 090_CYN_08 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0041a300
  case 6: Sprache 090_ROD_18 | ruft CGame_WalkAndComment
  case 10: Sprache 090_ROD_19 090_CYN_19 090_ROD_20 090_CYN_20 | S= 3F84=1 3F88=1 | S? 3F80 3F88 | ruft CActor_WalkTo CActor_Turn CGame_HeroSayWait FUN_0041a300
  case 11: Sprache 090_ROD_09 090_CYN_09 090_ROD_10 090_CYN_10 | S= 3F88=2 | ruft CActor_WalkTo CGame_HeroSayWait FUN_0041a300
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc14.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3F7C` | 0 | Kamin anklickbar (nach Gesprächsstufe 4 mit Cynthia) |
| `0x3F80` | 0 | Blumen angesehen – danach anderes Kamingespräch |
| `0x3F84` | 0 | über den Kamin gesprochen (`090_ROD_20`) |
| `0x3F88` | 1 | Gesprächsstufe mit Cynthia (1…5); Kamingespräch setzt Werte > 2 auf 1 zurück |
| `0x3F8C` | 1 | Doppelbett: 2 angesprochen, 3 Klappbett vorgeführt (dann wird `S90_Bett` nicht mehr geladen) |

### Ablauf
- Ctor `0x00419650`: Szene fest 90, `CSzene_Load("090", Herkunft == 89)`; aus 89 zusätzlich
  Aufwach-Animationen; `Sfx_Fire` (Vol. 0); bei `0x3F8C < 3` Klappbett-Animation mit drei Cues.
  Musik `Loc14_Cynthia`, Hall (4, 0.208, 0.478, 0).
- Start `0x004199C0`: Feuer in Schleife, auf 25 in 2 s; Bereiche 1–6, Kamin 10 (aktiv erst bei
  `0x3F7C`), Cynthia 11. Aus 89: Rodrigo wacht auf (Vollbild), setzt sich auf, `090_ROD_01`,
  steht auf, (459,295) Richtung 1 → (407,325); `090_CYN_01`. Sonst Ankunft (10,350) Richtung 6
  mit `CActor_WalkToClick` nach (143,336).
- Ereignisse `0x00419C90`: Krankenbett, Blumen, Trog, Regale (Kommentare); Doppelbett –
  bei Stufe 2, Kamingespräch und Gesprächsstufe 2 führt Cynthia das Klappbett vor (Musik
  `Loc14_LoveIsBreakingTheNight`, Vollbild `S90_Bett`); Kamin; Cynthia-Gesprächsstufen;
  Ausgang → 12/91 (Cynthia spricht ohne Warten weiter).
- Probelauf 2026-11-03: Aufwachen aus 89, Klappbett, Ausgang nach 12/91.
