# Loc27 – Hütte (Szene 95) und Megophias' Geheimraum (Szene 96)

Funktionen `0x0042AE20`–`0x0042C6BF` (6 Stück), Ereignis-Handler `0x0042BB70`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S95_WindowW` | `0x0042B570` |
| 2 | `S95_WindowN` | `0x0042B570` |
| 3 | `S95_WindowE` | `0x0042B570` |
| 4 | `S95_Chair` | `0x0042B570` |
| 5 | `S95_Floor` | `0x0042B570` |
| 6 | `S95_ExitTo93` | `0x0042B570` |
| 10 | `S96_ExitTo95` | `0x0042B570` |
| 11 | `S96_Megophias` | `0x0042B570` |
| 12 | `S96_Steckkontakt` | `0x0042B570` |
| 13 | `S96_Anlagen` | `0x0042B570` |
| 20 | `S96_Eye (Ebene)` | `0x0042B570` |
| 21 | `S96_Helm (Ebene)` | `0x0042B570` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0042bb70.c FUN_0042bb70 @ 0042bb70
  Anfang: ruft FUN_0042bb70
  case 1: Sprache 095_ROD_01 096_ROD_02 096_MEG_02 | S= 40C0=2 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0042c690
  case 2: Sprache 095_ROD_01 095_ROD_02 096_ROD_03 096_MEG_03 | → 27/96 | S= 40AC=2 40C0=3 | S? 40AC 40A8 40B4 | ruft CGame_WalkAndComment CActor_WalkTo2 CSzene_DisableInput CGame_HeroSay FUN_004406e0 CGame_HeroSayWait FUN_0042c690
  case 3: Sprache 095_ROD_01 096_ROD_04 096_MEG_04 | S= 40C0=4 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0042c690
  case 4: Sprache 095_ROD_03 096_ROD_05 096_MEG_05 | S= 40C0=5 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0042c690
  case 5: Sprache 095_ROD_04 095_ROD_05 096_ROD_06 096_MEG_06 | S= 40B0=2 40A8=1 40C0=6 | S? 40B0 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0042c690
  case 6: Sprache 096_ROD_07 096_MEG_07 | → 26/93 | S= 40C0=7 | ruft CActor_WalkTo CGame_HeroSayWait FUN_0042c690
  case 10: Sprache 096_MEG_26 096_ROD_26 096_ROD_12 096_MEG_12 | → 27/95 | S= 40C0=0xb | S? 40BC 4098 | ruft CActor_WalkToClick CGame_Delay FUN_00435ba0 FUN_0042c690 CActor_WalkTo2 CActor_WalkTo CActor_Hide CActor_Turn CGame_HeroSayWait
  case 11: Sprache 096_ROD_13 096_MEG_13 | S= 40C0=0xc | ruft CActor_WalkTo2 CGame_HeroSayWait FUN_0042c690
  case 7: Sprache 096_ROD_08 096_MEG_08 | S= 40C0=8 | ruft CGame_HeroSayWait FUN_0042c690
  case 8: Sprache 096_ROD_09 096_MEG_09 | S= 40C0=9 | ruft CGame_HeroSayWait FUN_0042c690
  case 9: Sprache 096_ROD_10 096_MEG_10 096_ROD_11 096_MEG_11 | S= 40C0=10 | ruft CGame_HeroSayWait FUN_0042c690
  case 12: Sprache 096_ROD_14 096_MEG_14 096_ROD_25 096_MEG_25 | S= 40C0=0xd | ruft CGame_HeroSayWait FUN_0042c690 CActor_WalkTo
  case 13: Sprache 096_ROD_15 096_MEG_15 096_MEG_23 | S= 40C0=0xe | ruft CGame_HeroSayWait FUN_0042c690 CActor_WalkTo
  case 14: Sprache 096_ROD_16 096_MEG_16 | S= 40C0=0xf | ruft CGame_HeroSayWait FUN_0042c690
  case 15: Sprache 096_ROD_17 096_MEG_17 | S= 40B8=1 40C0=0x10 | ruft CGame_HeroSayWait FUN_0042c690 CActor_Hide CLayer_Disable CAnimLayer_PlayEndEvent CActor_SetPos CActor_Show CLayer_Enable FUN_00435ba0
  case 16: Sprache 096_ROD_18 096_MEG_18 096_ROD_19 096_MEG_19 | S= 4098=1 | ruft CGame_HeroSayWait FUN_0042c690
  == 1: Sprache 096_ROD_20 096_MEG_20 | ruft CGame_HeroSayWait FUN_0042c690 FUN_00442f90
  == 2: Sprache 096_ROD_21 096_MEG_21 | ruft CGame_HeroSayWait FUN_0042c690 FUN_00442f90
  == 3: Sprache 096_ROD_22 096_MEG_22 096_ROD_23 | S= 3E90=1 | ruft CGame_HeroSayWait FUN_0042c690 FUN_004403e0 FUN_00442f90 CGame_Random CEvent_SetTimerHandler
  case 20: Sprache 096_ROD_24 096_MEG_24 | ruft CActor_WalkTo CActor_Hide CLayer_Disable FUN_004403e0 CAnimLayer_PlayEndEvent CActor_Show CLayer_Enable FUN_0042c690
  case 21: → 25/104 | ruft CActor_WalkTo FUN_004406e0
  case 30: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D
```

## Einzelanalyse (2026-11-04, portiert: `src/game/loc27.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3E90` | 0 | Megophias hat vom Helm erzählt (`096_ROD_23`; liest Loc16) |
| `0x4094` | 0 | Grube gesehen (Loc26) → zweite Gesprächsreihe (`096_*_20…22`); Herkunft 1001 setzt es (UNVERIFIED) |
| `0x4098` | 0 | nach `096_MEG_19`: Weg nach 98 frei, Ausgang 96 → 95 erlaubt |
| `0x40A8` | 0 | Boden zweimal untersucht: Fenster N öffnet den Geheimgang (`S95_EnterSecretRoom`) |
| `0x40AC` | 1 | 1: Boden gesperrt, erst Fenster N (`095_ROD_01`), dann 2 |
| `0x40B0` | 1 | Boden: 1 `095_ROD_04`, danach `095_ROD_05` |
| `0x40B4` | 1 | erster Besuch im Geheimraum (`096_ROD_01`, `096_MEG_01`, `095_ROD_02`) |
| `0x40B8` | 0 | Steckkontakt anklickbar (nach Stufe 15) |
| `0x40BC` | 0 | Helm in die Anlage gelegt (Benutzen-Handler `0x0042C590`); Szenenmaske 2, `S96_HelmAuf` → 25/104 |
| `0x40C0` | 1 | Gesprächsstufe 1…16 (`096_ROD/MEG_02…19`); bei `0x4094` und Stufe > 3 zurück auf 1 |

### Ablauf
- Ctor `0x0042AE20`: Szene 96 `S96` (Maske 2 mit Helm), sonst Szene 95 `S95` (+0x2C = 95).
  `this+0x24 = 1` setzt schon `CGame_ctor` (`0x0043F522`, Dtor `0x0043F5F5` → 0); kein Leser
  in CGame gefunden.
- 95: Ankunft aus 93 `S95_EnterHut`, Rodrigo als freie Ebene (103,64,Z 1,Zoom 125, Farbe
  −90/−70/−50) blendet in 15 Schritten nach (242,90,Zoom 84) auf (1,1,1), dann (332,357)
  Richtung 5 und Klick-Weg (379,321).
- 96: `S96_RodEntersRoom`, Rodrigo als freie Ebene (−96,78 → 164,100, Zoom 115 → 78),
  `S96_RodAndEye`, (235,357) Richtung 5; der Weg nach (250,350) wird wie im Original ohne
  `CGame_Wait` gestartet (Warteplatz bleibt bis zum Ende belegt).
- Ausgang 96 ohne `0x4098`: Rodrigo läuft los, bleibt stehen, `096_MEG_26`, zurück.
- Probelauf 2026-11-04: 95 Ankunft, Fenster N, Boden ×2, Geheimgang → 96 (Ankunft, Gespräch ×2,
  Ausgang verweigert); Helm aus dem Inventar abgelegt; Helm angeklickt → 25/104.
