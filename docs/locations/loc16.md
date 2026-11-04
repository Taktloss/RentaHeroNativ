# Loc16 – Drachenhorst, Szene 58

Funktionen `0x0041B290`–`0x0041CEFF` (10 Stück), Ereignis-Handler `0x0041BDC0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S58_To57` | `0x0041B830` |
| 2 | `S58_To59` | `0x0041B830` |
| 3 | `S58_Dragon` | `0x0041B830` |
| 4 | `S58_Hut (Ebene)` | `0x0041B830` |
| 5 | `S58_Hand (Ebene)` | `0x0041B830` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041bdc0.c FUN_0041bdc0 @ 0041bdc0
  Anfang: ruft FUN_0041bdc0
  case 1: → 15/57 | ruft CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 2: Sprache 058_DRA_34 058_ROD_35 058_DRA_33 | → 17/59 | S? 3FC8 3FB8 | ruft FUN_0041c9a0 CGame_HeroSayWait FUN_0041ca50 CActor_WalkTo CGame_Speak CActor_WalkTo2 CGame_Random CAudioBuffer_SetVolume
  case 3: S? 3FB4 | ruft CActor_WalkTo FUN_0041c9f0 CGame_Random CAudioBuffer_SetVolume
  == 1: Sprache 058_ROD_07 058_DRA_07 058_ROD_19 058_DRA_19 | S= 3FAC=2 | ruft CSzene_DisableInput CGame_HeroSayWait CActor_Hide CAnimLayer_PlayRange FUN_0041c9a0 CActor_Show CSzene_EnableInput FUN_0041ca50 CGame_Speak
  == 9: Sprache 058_ROD_15 058_DRA_15 | → 17/59 | S= 3FB8=1 3FAC=iVar2 + 1 | S? 3FAC | ruft CSzene_DisableInput FUN_0041ca50 CGame_HeroSayWait CGame_Speak FUN_0041c860 CActor_WalkTo CSzene_EnableInput FUN_0041c9f0
  case 4: Sprache 058_ROD_28 058_DRA_28 058_ROD_29 | S= 3FC0=0 | ruft FUN_0041c9f0 CGame_HeroSayWait CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CLayer_Disable CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler CAudioBuffer_SetVolume
  case 5: Sprache 058_DRA_30 058_ROD_30 058_DRA_31 | S= 3FC4=0 | S? 3FB4 | ruft CSzene_DisableInput CActor_WalkTo FUN_0041c9a0 FUN_0041c9f0 FUN_0041c890 CSzene_EnableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show
  case 6: Sprache 058_ROD_32 058_DRA_32 058_ROD_18 058_DRA_18 | S? 3FB4 | ruft FUN_0041c890 FUN_0041c9f0 CActor_WalkTo FUN_0041ca50
  == 4: Sprache 058_ROD_22 058_DRA_22 | ruft CSzene_DisableInput CActor_Turn CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait CActor_WalkTo CAnimLayer_PlayRange FUN_0041c9a0 CSzene_EnableInput
  == 6: Sprache 058_ROD_24 058_DRA_24 | S= 3F74=2 3FB0=iVar2 + 1 | S? 3F74 3FB0 | ruft FUN_0041c9f0
  case 7: S? 3E80 | ruft CGame_Random
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc16.cpp`)

### Flags
| Offset | Bedeutung |
|---|---|
| `0x3E80` | Nacht: Szenenmaske 2, `S58_DragonTalk_nd`, Hintergrund `S58_Back_nd`, Held-Farbe (−52,−11,43), Eule/Donner statt Vögel |
| `0x3E90` | Megophias hat vom Helm erzählt (Loc27 `0x0042C078`); ≠ 0 und ohne Helm → bei der Landung Hinweis `058_*_05/06` |
| `0x3F40` | Helm-Hinweis gegeben |
| `0x3F74` | Loc12-Zustand: 1 → 2 nach Nacht-Gesprächsstufe 6 |
| `0x3FA4` | erster Besuch (Start 1): Begrüßung `058_DRA_01`, `058_ROD_01`/`058_DRA_02` |
| `0x3FA8` | ein Pirat folgt aus 59: Ankunft aus 59 zeigt `S58_PirateBurn`, danach liegen Hut und Hand da |
| `0x3FAC` | Tag-Gesprächsstufe 1…11: 1 Rodrigo greift an (`S58_RodAngriff`), 9 Weg nach 59 frei (`0x3FB8 = 1`, → 17/59), sonst `058_ROD/DRA_%02d` mit Stufe+6 |
| `0x3FB0` | Nacht-Gesprächsstufe 1…6: 4 Blick zum Himmel und hinsetzen, 6 setzt `0x3F74`, sonst Stufe+0x12 |
| `0x3FB4` | Gesprächsstufe vor dem Abflug: 1–3 Stufe+0x18, 4 `_28`/`ROD_29`, 5 Abflug, 6 Abflug nur am Startpunkt (427,286) ±10 |
| `0x3FB8` | Weg nach 59 frei (sonst Sperrrechteck (0,0)–(639,262) per `0x0041C860`) |
| `0x3FBC` | Drache fliegt Rodrigo: `S58_DragonTakeOff` → 26/93 (`0x0041C890`) |
| `0x3FC0`/`0x3FC4` | Hut (Gegenstand 4) / Hand (Gegenstand 3) liegen da |
| `0x3FC8` | Aasfresser haben den Piraten (Loc17, Rückkehr aus dem Schädel): Gespräch nur noch Abflug-Pfad, Weg 59 `058_DRA_34`/`ROD_35` |
| `0x409C` | Start 1, Loc26 setzt 0 bei der ersten Landung auf der Insel: ≠ 0 → auch nachts mit Flugerlaubnis zuerst zum Drachen laufen |

### Haltungssteuerung `0x0041CA50`
Wie Loui in Loc05: Modus 1/2 Übergang direkt, 3/4 zurück in Ruhe bzw. in Geste `arg`, 5 zufällige
Geste (Timer 5–7 s, läuft noch → 3–6 s), 6 Timer 1–2 s neu. Haltung +0x2B4 (0 Ruhe, 1–5), Übergang
+0x2B8; Bildbereiche: 1 85→90, 2 90→85, 3 129→132, 4 132→129, 5 48→73, 6 77→86, 7 86→110,
8 118→129, 9 129→139, 10 139→129. Gerade Modi warten auf das Ende (Klickzustand +0x514 bleibt).

### Ablauf
- Ankunft aus 57: (16,300) Richtung 6 → (192,293); aus 59: (500,218) Richtung 0, ggf. Piraten-
  Sequenz; aus 93 (Flug): `S58_DragonLanding`, dann (427,286) Richtung 4 → (285,320).
- Ereignisse: 1 → 15/57, 2 → 17/59 (bzw. Drache versperrt den Weg, `058_DRA_33`), 3 Gespräch,
  4/5 Hut/Hand aufheben (`S58_RodPickUp` an (248,246) bzw. (282,243)), 6 Haltung, 7 Umgebungston.
- Probelauf 2026-11-03: Begrüßung, Stufe 1 (Angriff), Weg 59 versperrt; Stufe 9 → 17/59;
  nachts Stufe 5 → Abflug → 26/93; Nachtbild geprüft. Piraten-Sequenz über 61 → 60 → 59 → 58 geprüft.
  Landung aus 93 geprüft (2026-11-04). Nicht geprüft: Hut/Hand aufheben.
  Port-Hinweis: Ankunft aus 93 bei Tag (nur per `--game`) wartet ewig, weil `S58_DragonLanding`
  nur nachts geladen wird (Original: Nullzeiger).
