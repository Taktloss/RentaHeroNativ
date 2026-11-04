# Loc08 – Hafen mit Sabrina, Szenen 99 und 100

Funktionen `0x00411640`–`0x004134AF` (9 Stück), Ereignis-Handler `0x004124B0`, `0x004125A0`, `0x00412FD0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `*(void **)(iVar1 + 0x1c0)` | `0x00412150` |
| 1 | `*(void **)(iVar2 + 0x1c0)` | `0x00411CD0` |
| 3 | `*(void **)(param_1 + 0xcc)` | `0x00411CD0` |
| 5 | `*(void **)(param_1 + 0xd0)` | `0x00411CD0` |
| 7 | `*(void **)(param_1 + 0xd4)` | `0x00411CD0` |
| 9 | `*(void **)(param_1 + 0xd8)` | `0x00411CD0` |
| 11 | `*(void **)(param_1 + 0xdc)` | `0x00411CD0` |
| 11 | `099_Gang` | `0x004125A0` |
| 11 | `pvVar3` | `0x00411CD0` |
| 13 | `*(void **)(*(int *)(param_1 + ` | `0x00411CD0` |
| 13 | `SabStand (Ebene)` | `0x004125A0` |
| 13 | `pvVar3` | `0x00411CD0` |
| 100 | `*(void **)(*(int *)(param_1 + ` | `0x00412150` |
| 100 | `BootFahne (Ebene)` | `0x00412FD0` |
| 100 | `this` | `0x00412150` |
| 101 | `*(void **)(param_1 + 0xe0)` | `0x00412150` |
| 102 | `Gegenstand 2` | `0x00412150` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 004124b0.c FUN_004124b0 @ 004124b0
  Anfang: ruft FUN_004124b0 FUN_004125a0 FUN_00412fd0 CGame_Random
  case 6: ruft CGame_Random CAudioBuffer_FadeFreq
  case 11: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
# 004125a0.c FUN_004125a0 @ 004125a0
  Anfang: ruft FUN_004125a0
  case 1: Sprache 099_ROD_10 099_ROD_01 099_SAB_02 | ruft CGame_WalkAndComment CGame_HeroSayWait CGame_Speak
  case 3: Sprache 099_ROD_11 099_ROD_03 099_ROD_04 099_SAB_04 | S= 3F10=4 | ruft CGame_WalkAndComment CSzene_DisableInput CGame_HeroSayWait CActor_WalkTo CGame_Speak CSzene_EnableInput
  case 5: Sprache 099_ROD_09 099_ROD_06 099_SAB_06 | S= 3F18=2 | ruft CGame_WalkAndComment CGame_HeroSayWait CSzene_DisableInput CGame_Speak CLayer_Disable FUN_004474c0 FUN_00442f90 FUN_004475b0 CAnimLayer_PlayRange CAnimLayer_Start CLayer_Enable CArea_SetEvent CSzene_EnableInput
  case 7: S? 3F0C
  == 1: Sprache 099_SAB_01 | S= 3F0C=2 3F18=1 | ruft CActor_WalkTo2 CSzene_DisableInput FUN_004475b0 CAnimLayer_PlayRange CActor_Hide FUN_00442f90 CAnimLayer_ShowFrame FUN_004474c0 CLayer_Enable CLayer_Disable FUN_00435b80 CActor_Show CGame_Speak CSzene_EnableInput CArea_SetEvent
  == 2: Sprache 099_ROD_08 099_ROD_07 099_SAB_07 | → 8/100 | ruft CGame_WalkAndComment CLayer_Disable FUN_004474c0 FUN_00442f90 FUN_004475b0 CAnimLayer_PlayEndEvent CActor_WalkTo2 CGame_HeroSayWait CGame_Speak
  case 9: Sprache 099_SAB_08 | → 10/39 16/58 | S? 3F18 3F10 | ruft CGame_Speak CActor_WalkTo
  case 11: S? 3F18 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent
  case 13: S? 3F18
  case 2: Sprache 099_ROD_02 099_SAB_03 | ruft CGame_HeroSayWait CGame_Speak
  case 4: Sprache 099_ROD_05 099_SAB_05 | S= 3F10=5 | ruft CSzene_DisableInput CGame_HeroSayWait CGame_Speak CLayer_Disable CAnimLayer_PlayEndEvent CLayer_Enable CSzene_EnableInput
  == 3: Sprache 099_ROD_07 | ruft CActor_WalkTo2 CGame_HeroSayWait
  case 70: ruft CEvent_SetTimerHandler
  case 71: ruft CAnimLayer_Start
# 00412fd0.c FUN_00412fd0 @ 00412fd0
  Anfang: ruft FUN_00412fd0
  case 1: Sprache 100_ROD_02 | ruft CGame_WalkAndComment
  case 13: Sprache 100_ROD_03 100_SAB_02 | ruft CActor_WalkTo FUN_00440400 CGame_Speak
  case 100: Sprache 100_ROD_01 100_SAB_01 | → 23/103 | S= 3F18=4 | S? 4088 3F14 | ruft FUN_004409c0 CGame_Speak CActor_WalkTo CActor_Hide CLayer_Disable CAnimLayer_PlayRange FUN_00442f90 FUN_004475b0
  case 101: → 8/99 | ruft CActor_WalkTo
  case 102: Sprache 100_ROD_04 100_SAB_03 | S= 3F14=1 | Inv Remove(2) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CAnimLayer_Start CArea_SetEvent CActor_Show FUN_00440400 CGame_Speak CSzene_EnableInput
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc08.cpp`)

### Flags
| Offset | Bedeutung (belegt durch die Abläufe unten) |
|---|---|
| `0x3F0C` | Fass: 1 = Sabrina versteckt (Start), 2 = herausgesprungen |
| `0x3F10` | Gesprächsstufe Sabrina 1…5 |
| `0x3F14` | Fahne (Gegenstand 2) am Boot angebracht |
| `0x3F18` | Sabrina: 0 nicht da, 1 steht am Fass, 2 Klappe geöffnet, 3 aus dem Gang zurück, 4 Bootsfahrt erledigt |
| `0x3F1C` | Weg in den Wald einmal nach 58 statt 39 geführt |
| `0x3F20` | Boot zurückgekehrt (Szene 100, nur Ladebit) |
| `0x4088` | Rodrigo hat das Schiff mit Helm verlassen (Loc24 `0x00428A53`); ≠ 0 → Wald einmal nach 16/58, Boot verweigert (`100_ROD_01`) |

### Konstruktor `0x00411640`
- Ladebits: 1 wenn `0x3F18 < 2`, 2 wenn `< 4`; Szene 100 zusätzlich 4 wenn `0x3F20 = 0`.
- Szene 99 (`099`): `RodReinCam` nur bei `0x3F18 = 0`; Ebenen Busch, Klappe, Rod-/Sab-Animationen;
  Bereiche `099_AnlegePlatz/Stadt/Fass/Wald/Gang`; zwei Timer, `Sfx_Wind`.
- Szene 100 (`100`): Brandung, BoatAway/Returns/Back, Boot, BootFahne, RodFahne, SabWalk/Stand,
  `100_WayBack`.
- Beide: Möwe, Ocean (Vol. 30) u. a.; Musik `Loc08_Harbour`, Hall wie Loc01.

### Start `0x00411C20` → `0x00411CD0` (99) / `0x00412150` (100)
- Ocean in Schleife, einblenden; Umgebungs-Timer `0x48` alle 1–3 s (`Random(10,30)·100`):
  6/16 Frequenz 80–100 % in 3 s, 3/16 Möwe, je 1/16 Donner 1/2.
- 99: Bereiche 3/5/7/9, Piratenschiff (Ereignis 1) solange `0x3F18 < 4`. Aus 100: Klappe offen,
  Rodrigo klettert heraus (359,221), bei `0x3F18 ≤ 3` folgt Sabrina (Gehen 347→306) und steht
  dann (`0x3F18 = 3`). Sonst Sabrina je Stufe an (433,176), (368,171) oder (306,173), Busch-
  Animation, Wind, Ankunft (628,252) → (538,294); Stufe 0: Kamerafahrt `RodReinCam` (modal).
  Wind-Timer `0x46` alle 15 s periodisch → Wind, 500 ms später Busch (`0x47`).
- 100: Ocean Vol. 70, Brandung; Gegenstand 2 → Ereignis `0x66`. Aus 99: Stufe 4 nur Boot hinten,
  sonst Boot mit Ereignis 100 und Sabrina läuft von (600,151) zu (341,149). Sonst Rückkehr mit dem
  Boot (Paddeln), `0x3F18 = 4`, `0x3F20 = 1`. Weg zurück → `0x65`.

### Ereignisse
| Szene | ID | Ablauf |
|---|---:|---|
| 99 | 1/3/5 | Schiff, Anlegeplatz, Stadt: hingehen und Kommentar |
| 99 | 7 | Fass Stufe 1: Sabrina springt heraus, Rodrigo erschrickt, `099_SAB_01`, `0x3F18 = 1`; danach `099_ROD_08` |
| 99 | 9 | Wald (626,258): Sabrina ruft (`099_SAB_08`, Stufe 1, Gespräch < 4); → 10/39 bzw. einmal 16/58 |
| 99 | 11 | Klappe: Rodrigo steigt ein, Sabrina folgt (Stufe 2/3) → 8/100 |
| 99 | 13 | Sabrina: Gespräch 1–5; bei 4 zeigt sie, bei 5 öffnet sie die Klappe (`0x3F18 = 2`); Stufe 2/3 `099_ROD_07`/`099_SAB_07` |
| 99 | 70/71 | Wind / Busch |
| 100 | 1 | Schiff: Kommentar |
| 100 | 13 | Sabrina: `100_ROD_03`, `100_SAB_02` |
| 100 | 100 | Boot: `0x4088` → `100_ROD_01`; ohne Fahne `100_SAB_01`; sonst Abfahrt (Paddeln), `0x3F18 = 4` → 23/103 |
| 100 | 101 | (467,208) → 8/99 |
| 100 | 102 | Gegenstand 2 am Boot: Fahne, `0x3F14 = 1`, Gegenstand weg, `100_ROD_04`, `100_SAB_03` |
| 100 | sonst | `MessageBoxA("Unknown_Event____")` – Port: Meldung auf stderr |

- Eigenheiten: Gespräch Stufe 3 ruft nach `HeroSayWait` ein zweites `CGame_Wait` ohne Platz;
  Boot ohne Fahne belegt einen Warteplatz ohne zu warten. Beides nachgebildet.
- Probelauf 2026-11-03: Ankunft 99 (Kamerafahrt), Fass, fünf Gespräche bis zur offenen Klappe,
  Szene 100 aus 99 und nach der Bootsfahrt. Offen: Fahne anbringen, Bootsfahrt, Gang.
