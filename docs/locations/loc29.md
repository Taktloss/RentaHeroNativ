# Loc29 – Kneipe in Endavin, Szene 33

Funktionen `0x0042EE50`–`0x0042F74F` (5 Stück), Ereignis-Handler `0x0042F290`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S33_To32` | `0x0042F0E0` |
| 2 | `S33_Pirate1 (Ebene)` | `0x0042F0E0` |
| 2 | `S33_Pirate2 (Ebene)` | `0x0042F0E0` |
| 2 | `S33_Pirate3 (Ebene)` | `0x0042F0E0` |
| 3 | `S33_Krug1 (Ebene)` | `0x0042F0E0` `0x0042F290` |
| 4 | `Gegenstand 1` | `0x0042F0E0` |
| 4 | `Gegenstand 14` | `0x0042F0E0` |
| 4 | `Gegenstand 3` | `0x0042F0E0` |
| 4 | `Gegenstand 4` | `0x0042F0E0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0042f290.c FUN_0042f290 @ 0042f290
  Anfang: ruft FUN_0042f290
  case 1: → 28/32 | ruft CSzene_DisableInput CLayer_Disable CLayer_Enable CAnimLayer_PlayRange
  case 2: S? 40F8
  == 1: Sprache 033_ROD_01 033_PI2_01 033_PI3_01 | S= 40F8=2 | ruft CGame_Speak CAnimLayer_PlayRange
  == 2: Sprache 033_ROD_02 033_PI2_02 033_PI3_02 033_PI3_03 | S= 40F4=1 40F8=3 | ruft CSzene_DisableInput CGame_Speak CAnimLayer_PlayRange CLayer_Enable CAnimLayer_ShowFrame CArea_SetEvent CSzene_EnableInput
  == 3: Sprache 033_ROD_04 033_PI2_04 | ruft CGame_Speak
  case 3: Sprache 033_PI2_05 033_PI3_05 | → 30/34 | ruft CSzene_DisableInput CLayer_Disable CSzene_SetModal CAnimLayer_PlayRange FUN_004403e0 CGame_Delay
  case 4: ruft FUN_004403e0
```

## Einzelanalyse (2026-11-04, portiert: `src/game/loc29.cpp`)

### Flags
| Offset | Start | Bedeutung |
|---|---:|---|
| `0x40F4` | 0 | Krug steht auf dem Tisch (nach Gesprächsstufe 2); dann keine Eintritts-Animation mehr |
| `0x40F8` | 1 | Gespräch mit den Piraten: 1 → 2 → 3 (bei 2 wird der Krug hingestellt) |

### Ablauf
- Ctor `0x0042EE50`: Szene immer 33 (`+0x2C = 0x21`), Töne vor `CSzene_LoadAll("S33")`,
  `S33_Enter`/`S33_RodKipptAndLook` als eigene Animationen, Musik `Loc29_PubInEndavin`, Hall 4.
- Start `0x0042F0E0`: Eintritts-Animation nur frisch und ohne Krug. Piraten-Bereiche → 2,
  Krug → 3, Ausgang → 1, Verkleidungsteile 1/3/4/14 → 4 (`GEN_ROD_01`).
- Ereignisse `0x0042F290`: 1 `S33_RodGeht` → 28/32; 2 Gespräch je `0x40F8`; 3 Krug trinken
  (`S33_RodKipptAndLook` modal, Töne Trinken/Klavier/Aufschlag zwischen den Bildbereichen,
  `033_PI2_05`, 4 s, `033_PI3_05`) → 30/34.
- Probelauf 2026-11-04: zweimal ansprechen, Krug erscheint, trinken → Kerker.
