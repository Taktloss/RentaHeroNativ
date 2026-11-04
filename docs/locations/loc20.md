# Loc20 – Bibliothek mit Tharain, Szene 70

Funktionen `0x004223E0`–`0x00422D8F` (6 Stück), Ereignis-Handler `0x00422930`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S70_To69` | `0x00422690` |
| 2 | `S70_Boden` | `0x00422690` |
| 10 | `S70_Book (Ebene)` | `0x00422690` |
| 10 | `S70_Pult (Ebene)` | `0x00422690` |
| 10 | `S70_Stone (Ebene)` | `0x00422690` |
| 10 | `S70_Tharain (Ebene)` | `0x00422690` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00422930.c FUN_00422930 @ 00422930
  Anfang: ruft FUN_00422930
  == 1: Sprache 070_THA_13 070_ROD_13 | → 19/69 | S? 402C | ruft CSzene_DisableInput CActor_WalkTo2 FUN_00422d50 CActor_WalkTo CActor_Turn CGame_HeroSayWait CSzene_EnableInput
  == 2: Sprache 070_ROD_14 | ruft CGame_WalkAndComment
  == 10: S? 4030 | ruft CActor_WalkTo
  case 1: Sprache 070_ROD_03 070_THA_03 | S= 4030=2 | ruft CGame_HeroSayWait FUN_00422d50
  case 2: Sprache 070_ROD_04 070_THA_04 | S= 4030=3 | ruft CGame_HeroSayWait FUN_00422d50
  case 3: Sprache 070_ROD_05 070_THA_05 070_ROD_06 | S= 4030=4 | ruft CGame_HeroSayWait FUN_00422d50
  case 4: Sprache 070_ROD_07 070_THA_07 | S= 4030=5 | ruft CGame_HeroSayWait FUN_00422d50
  case 5: Sprache 070_ROD_08 070_THA_08 | S= 4030=6 | ruft CGame_HeroSayWait FUN_00422d50 CSzene_DisableInput CLayer_Disable CAnimLayer_Start CAnimLayer_PlayRange CLayer_Enable CSzene_EnableInput
  case 6: Sprache 070_ROD_09 070_THA_09 070_THA_10 | S= 4030=7 | Inv Add(21) | ruft CGame_HeroSayWait FUN_00422d50 CSzene_DisableInput CGame_Speak CActor_WalkTo2 CAnimLayer_PlayRange CActor_Hide FUN_00442f90 CAnimLayer_PlayEndEvent CActor_Show CActor_WalkTo CSzene_EnableInput
  case 7: Sprache 070_ROD_11 070_THA_11 | S= 4030=8 | ruft CGame_HeroSayWait FUN_00422d50
  case 8: Sprache 070_ROD_12 070_THA_12 | S= 4024=1 4028=1 402C=1 | ruft CGame_HeroSayWait FUN_00422d50
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc20.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3FEC` | 0 | Tharain nicht mehr da (gesetzt in Loc22): Ladebit 1 fehlt, nur Boden-Kommentar |
| `0x4020` | 1 | erster Besuch: Begrüßung `070_THA_01`/`070_ROD_01`/`070_THA_02`; danach fehlt zufällig eine der sechs Farbebenen |
| `0x4024` | 0 | nach Gesprächsstufe 8 gesetzt (`0x00422C02`); wird im ganzen Binary nicht gelesen |
| `0x4028` | 0 | Weg nach 71 offen (liest Loc19) |
| `0x402C` | 0 | Gespräch beendet – vorher ruft Tharain Rodrigo zurück |
| `0x4030` | 1 | Gesprächsstufe 1…8 |

### Ablauf
- Ctor `0x004223E0`: Szene fest 70, `CSzene_Load("S70", 0x3FEC = 0)`; Pult, Buch, Stein und Farben
  sind statische Ebenen; `Sfx_MobileDown` als Cue Bild 0 am fliegenden Stein; Musik
  `Loc20_Library`, Hall (10, 0.5, 10.0, 0.3).
- Start `0x00422690`: Held (28,312) Richtung 6 → (253,345). Tharain, Buch, Pult und Stein (bis
  Stufe 5) → Ereignis 10.
- Ereignis 10 `0x00422930`: Stufen 1–4 Gespräch; 5 Tharain nimmt den Stein; 6 wirft ihn Rodrigo
  zu (Gegenstand 21); 7; 8 setzt `0x4024`, `0x4028`, `0x402C`. Tharain spricht über `0x00422D50`
  (Bilder 15–45).
- Ereignis 1: vor `0x402C` ruft Tharain zurück (`070_THA_13`, Drehungen), sonst (28,312) → 19/69
  (mit zweitem `CGame_Wait` ohne Platz wie im Original). Ereignis 2: Boden-Kommentar.
- Probelauf 2026-11-03: Begrüßung, Stufen 5 und 6 (Stein, Gegenstand 21), Rückweg vor/nach dem
  Gespräch. Offen: Variante ohne Tharain.
