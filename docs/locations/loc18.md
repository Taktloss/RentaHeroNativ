# Loc18 – im Totenschädel, Szene 61

Funktionen `0x0041E4F0`–`0x0041F06F` (9 Stück), Ereignis-Handler `0x0041EC80`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S61_To60` | `0x0041E960` |
| 2 | `S61_Brain` | `0x0041E960` |
| 3 | `S61_To60_2` | `0x0041E960` |
| 4 | `Gegenstand 6` | `0x0041E960` |
| 5 | `Gegenstand 8` | `0x0041E960` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041ec80.c FUN_0041ec80 @ 0041ec80
  Anfang: ruft FUN_0041ec80
  case 1: S? 3FE0 | ruft CActor_WalkTo CSzene_DisableInput FUN_0041ec40 FUN_0041ebc0
  == 1: Sprache 061_ROD_04 | ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D
  == 2: Sprache 061_ROD_05 061_ROD_06 | ruft CGame_WalkAndComment CGame_Random CAudioBuffer_SetVolume CEvent_SetTimerHandler
  case 3: Sprache 061_ROD_02 061_ROD_03 | → 17/60 | S= 3FE4=1 | S? 3FE0 | ruft CGame_WalkAndComment CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayEndEvent
  case 4: Sprache 061_ROD_08 061_ROD_09 | S= 3FE0=1 | Inv Remove(6) | ruft CSzene_DisableInput CGame_WalkAndComment CSzene_SetModal CAnimLayer_PlayEndEvent FUN_0041eb20 CGame_Delay FUN_00440400 CSzene_EnableInput
  case 5: Sprache 061_ROD_10 | ruft FUN_004409c0
  case 6: ruft CAnimLayer_PlayEndEvent FUN_0041ebc0
  == 0: ruft FUN_0041ec40
  case 14: ruft CGame_Random
```

## Einzelanalyse (2026-11-03, portiert: `src/game/loc18.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x3FDC` | 1 | Kommentar-Reihe am Gehirn: `061_ROD_04` → `_05` → `_06` → wieder `_04` |
| `0x3FE0` | 0 | Gegenstand 6 aufs Gehirn gestreut (`S61_Streu`, `Sfx_Acid`): Gehirn raucht, Kiefer öffnen sich |
| `0x3FE4` | 0 | Kiefer geöffnet (`S61_OpenJaws`) → 17/60, dort Ankunft mit dem Festmahl der Aasfresser |
| `0x3FE8` | 1 | erster Besuch: `061_ROD_01` |

### Ablauf
- Ctor `0x0041E4F0`: `CSzene_Load("S61", 0)`; Vollbild-Animationen ClimbIn/ClimbOut/OpenJaws/Streu
  mit Ton-Cues; Musik `Loc17_UglyAndDisgusting` mit Hall (2, …).
- Start `0x0041E960`: Pilze an; Gegenstand 6 → Ereignis 4, Gegenstand 8 → Ereignis 5
  (`061_ROD_10`); Ankunft `S61_ClimbIn`, Held (139,173) Richtung 0 → (145,190); bei `0x3FE0` Rauch.
- Rauch: eigener Timer +0xA8 (`0x0041EB20` neu, 1 s; `0x0041EBC0` 4 s; `0x0041EBF0` löschen).
  Ereignis 6 startet abwechselnd `S61_Smoke` (Ende 7) und `S61_SmokeClone` (Ende 8); +0xD0/+0xD4
  laufen. Ausgang (1) hält den Timer an; läuft noch Rauch, wartet er (+0xD8) auf 7/8 und klettert
  dann hinaus (`0x0041EC40` → 17/60). Abgebrochener Weg setzt den Timer wieder (4 s).
- Fliegen-Geräusch alle 3–6 s (Ereignis `0x0E`).
- Probelauf 2026-11-03: Ankunft, Gehirn, Kiefer ohne Säure, Ausgang mit/ohne Rauch; mit `0x3FE0`
  Kiefer → 17/60 (Festmahl) → 59 → 16/58 (Pirat verbrannt, Hut und Hand liegen da).
  Nicht geprüft: Streuen selbst (Gegenstand 6 aus dem Inventar).
