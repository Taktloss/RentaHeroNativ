# Loc17 – Aasfresser-Lichtung (Szene 59) und Totenschädel (Szene 60)

Funktionen `0x0041CF00`–`0x0041E4EF` (5 Stück), Ereignis-Handler `0x0041D9F0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S59_To58` | `0x0041D480` |
| 2 | `S59_Skeleton` | `0x0041D480` |
| 3 | `S59_Wall` | `0x0041D480` |
| 4 | `S59_Head` | `0x0041D480` |
| 50 | `S60_To59` | `0x0041D480` |
| 51 | `S60_Skull` | `0x0041D480` |
| 52 | `S60_Mouth` | `0x0041D480` |
| 53 | `S60_Ear` | `0x0041D9F0` |
| 53 | `this` | `0x0041D480` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041d9f0.c FUN_0041d9f0 @ 0041d9f0
  Anfang: ruft FUN_0041d9f0
  case 1: → 16/58 | S? 3FC8 | ruft CActor_WalkTo CSzene_DisableInput CLayer_Enable FUN_00442f90 FUN_004475b0 CGame_Delay CSzene_EnableInput CGame_Random CAudioBuffer_SetVolume
  case 2: ruft CActor_WalkTo CGame_Random CGame_Comment CAudioBuffer_SetVolume
  case 3: Sprache 059_ROD_07 059_ROD_08 | S? 3FC8 | ruft CActor_WalkTo CGame_HeroSay CGame_Random CAudioBuffer_SetVolume
  case 4: → 17/60 | ruft CActor_WalkTo CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
  case 30: ruft CAnimLayer_PlayRange
  case 31: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 32: ruft CAnimLayer_PlayRange
  case 33: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 34: ruft CAnimLayer_PlayRange
  case 35: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 36: ruft CAnimLayer_PlayRange
  case 37: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 38: ruft CAnimLayer_PlayRange
  case 39: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 40: ruft CAnimLayer_PlayRange
  case 41: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 42: ruft CAnimLayer_PlayRange
  case 43: ruft FUN_00442f90 CGame_Random FUN_004475b0
  case 50: → 17/59 | ruft CActor_WalkTo
  case 51: Sprache 060_ROD_02 | ruft CActor_WalkTo CGame_Comment
  case 52: Sprache 060_ROD_09 060_ROD_03 | S? 3FC8 3FCC | ruft CActor_WalkTo CGame_Comment CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayEndEvent CGame_HeroSayWait CSzene_EnableInput
  case 53: Sprache 060_ROD_09 060_ROD_08 | → 18/61 | S= 3FD0=1 | S? 3FC8 3FD0 | ruft CActor_WalkTo CGame_Comment CGame_HeroSay
  case 54: S? 3FCC | ruft CActor_WalkTo
  == 1: Sprache 060_ROD_04 | S= 3FCC=2 | ruft CGame_Comment
  == 2: Sprache 060_ROD_05 | S= 3FCC=3 3FD8=2 | S? 3FD8 | ruft CGame_Comment
  == 3: Sprache 060_ROD_06 060_ROD_07 | S= 3FCC=4 | ruft CSzene_DisableInput CGame_HeroSay CActor_Hide CAnimLayer_PlayRange FUN_004475b0 CActor_Show CGame_Comment CArea_SetEvent CSzene_EnableInput
  case 60: ruft CLayer_Disable
  case 61: ruft CLayer_Disable
  case 62: ruft CLayer_Disable
  case 63: ruft CLayer_Disable
  case 64: ruft CLayer_Disable
  case 65: ruft CLayer_Disable
  case 66: ruft CLayer_Disable
  case 100: ruft CGame_Random
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc17.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3F44` | 0 | Pilz in Szene 39 liegt (Loc10); wird beim ersten Besuch hier 1 (`0x3FD4` → 0) |
| `0x3FA8` | 0 | gesetzt nach dem Fressen: ein Pirat folgt nach 58 (Loc16 verbrennt ihn) |
| `0x3FC8` | 0 | Aasfresser sind satt/weg; danach Mund/Ohr nur `060_ROD_09` |
| `0x3FCC` | 1 | Busch-Stufe: 1 → `060_ROD_04`; 2 → `060_ROD_05` (zweimal, über `0x3FD8`); 3 Busch wegziehen (`S60_BuschWeg`, Busch gleitet nach x 630); 4 Busch weg, Ohr (Ereignis `0x35`) frei |
| `0x3FD0` | 0 | `060_ROD_08` vor dem ersten Gang ins Ohr gesagt |
| `0x3FD8` | 1 | Hilfszähler der Busch-Stufe 2 |
| `0x3FE4` | 0 | Loc18 setzt 1 beim Öffnen des Kiefers (`0x0041EDFE`): Rodrigo kommt aus dem Schädel zurück (Ankunft aus 61) |

### Ablauf
- Ctor `0x0041CF00`: Szene 59 `S59` (Aasfresser1–7, Startpositionen per `__ftol` aus x/y +0x10/+0x18);
  Szene 60 `S60` mit `S60_RodRaus`, `S60_TryOpenSkull{Mit,Ohne}Busch`; aus 61 bei `0x3FC8 = 0` und
  `0x3FE4 ≠ 0` zusätzlich Töne und Cues (Spritz 5/9/12, RodRaus 44). Musik `Loc17_UglyAndDisgusting`.
- Start `0x0041D480`, Szene 59 aus 58 (301,355) Richtung 4: ohne `0x3FC8` fressen die Aasfresser
  (Bilder 0–2 Schleife) und fliegen nach je 10–20 Schritten (`0x00443530`) auf: Ereignis
  `0x1E+2i` Bilder 2–5, `0x1F+2i` Flug 6–17 nach x 485 in 20–30 Schritten, fps 20.
  Rückweg nach 58 (Ereignis 1): sie kehren von (490,255) an ihre Plätze zurück.
- Szene 60: aus 59 (8,296) Richtung 6 → (95,313); aus 61 mit `0x3FE4`: `S60_RodRaus`, die Aasfresser
  fliegen von (−92, 200–230) nach (350,256) Zoom 48 und verschwinden (Ereignisse `0x3C`–`0x42`),
  `S60_Spritz`, `0x3FC8 = 1`, `0x3FA8 = 1`, `060_ROD_01`. Mund (`0x34`): Schädel öffnen scheitert.
  Ohr (`0x35`) → 18/61.
- Original-Eigenheit: Die Ankunft aus 61 spielt die Töne ohne Prüfung; geladen sind sie nur bei
  `0x3FC8 = 0` (im Port mit Nullprüfung, `// UNVERIFIED`).
- Probelauf 2026-11-03: 59 aus 58 (Fressen, Auffliegen), Kopf → 60, Busch-Stufen bis zum Wegziehen,
  Ohr → 18/61, Rückweg 59 → 16/58. Ankunft aus 61 über Loc18 geprüft (Festmahl). Nicht geprüft: Mund.
