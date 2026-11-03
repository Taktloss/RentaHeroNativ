# Loc25 – Cyberspace: Endkampf gegen Megophias, Szene 105

Funktionen `0x00429020`–`0x0042A06F` (6 Stück), Ereignis-Handler `0x00429810`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S105_WizWalk (Ebene)` | `0x00429660` |
| 11 | `S105_DraHead (Ebene)` | `0x00429810` |
| 12 | `S105_MadHead (Ebene)` | `0x00429810` |
| 13 | `S105_SabHead (Ebene)` | `0x00429810` |
| 14 | `S105_JasHead (Ebene)` | `0x00429810` |
| 15 | `S105_LouHead (Ebene)` | `0x00429810` |
| 16 | `S105_RanHead (Ebene)` | `0x00429810` |
| 17 | `S105_CynHead (Ebene)` | `0x00429810` |
| 18 | `S105_SanHead (Ebene)` | `0x00429810` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00429810.c FUN_00429810 @ 00429810
  Anfang: ruft FUN_00429810
  case 1: Sprache 105_MEG_01 | S= 4090=1 | S? 4090 | ruft CSzene_DisableInput CLayer_Disable CAnimLayer_PlayRange CAnimLayer_Start FUN_00440400 CLayer_Enable CGame_Delay CArea_SetEvent CSzene_EnableInput
  case 11: ruft FUN_00429c80
  case 12: ruft FUN_00429c80
  case 13: ruft FUN_00429c80
  case 14: ruft FUN_00429c80
  case 15: ruft FUN_00429c80
  case 16: ruft FUN_00429c80
  case 17: ruft FUN_00429c80
  case 18: ruft FUN_00429c80
  case 200: ruft CGame_Random
  == 1: ruft CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler
```

## Einzelanalyse (2026-11-04, portiert: `src/game/loc25.cpp`)

### Flag
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x4090` | 0 | 0: Helfer-Köpfe noch nicht gerufen; 1…6: Nummer des nächsten richtigen Helfers; 7: gewonnen |

### Ablauf
- Ctor `0x00429020`: `CSzene_LoadAll("S105")`, +0x2C = 105; Köpfe (TCC), Angriffs-Animationen (A16),
  Hintergrund ist eine AVI-Ebene. Musik `Loc25_Cyberspace` mit Hall (10, …).
- Start `0x00429660`: Musik in Schleife mit Lautstärke 100 (kein FadeIn), `Sfx_EndTimeFeeling`
  Frequenz 20 in Schleife, Hintergrund Bilder 0–10 in Schleife; Gegenstände 5, 8, 9, 10, 11, 24,
  26 werden entfernt. Zauberer kommt von (370,−167) Zoom 80 nach (357,42) Zoom 100 in 20
  Schritten; `S105_RodMorph`. Donner alle 3–5 s.
- Zauberer anklicken (1): er greift an (`S105_WizAttack` 0–5, 6–Ende), `105_MEG_01`; beim ersten Mal
  erscheinen die acht Köpfe (Ereignisse `0x0B`–`0x12`), `0x4090 = 1`.
- Kopf anklicken `0x00429C80(nummer, ebene)`: Rodrigo verwandelt sich. Richtige Reihenfolge:
  Madonna (1), Sabrina (2), Cynthia (3), Loui (4), Sandy (5), Drache (6). Jasper und Rancher (0)
  treffen nie; ihre Ebenen sind statische TCC (das Original ruft dort kein PlayRange auf).
  Falsch → Gegenangriff, `0x4090 = 1`. Richtig → Angriff 0…n−1, Explosion, n…Ende;
  nach dem sechsten Treffer Gong, Köpfe verschwinden, → 41/112.
- Probelauf 2026-11-04: Zauberer, alle sechs Helfer in richtiger Reihenfolge → 41/112.
  Nicht geprüft: falsche Reihenfolge.
