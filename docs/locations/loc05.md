# Loc05 – Louis Büro (Szene 10)

Funktionen `0x0040D530`–`0x0040E72F` (10 Stück), Ereignis-Handler `0x0040DB30`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S10_Exit` | `0x0040D890` |
| 2 | `S10_Desk` | `0x0040D890` |
| 3 | `S10_Flowers1` | `0x0040D890` |
| 4 | `S10_Flowers2` | `0x0040D890` |
| 5 | `S10_Kitchen` | `0x0040D890` |
| 6 | `S10_Loui` | `0x0040D890` |
| 7 | `S10_Truhe` | `0x0040D890` |
| 8 | `S10_Krug` | `0x0040D890` |
| 9 | `S10_Ofen` | `0x0040D890` |
| 10 | `S10_KastenLinks` | `0x0040D890` |
| 11 | `S10_KastenRechts` | `0x0040D890` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040db30.c Loc05_OnGenericEvent @ 0040db30
  Anfang: ruft Loc05_OnGenericEvent
  case 1: Sprache 028_ROD_01 028_LOU_01 | → 1/9 | ruft CActor_WalkTo FUN_0040e200
  case 2: Sprache 010_ROD_04 028_ROD_02 028_LOU_02 | ruft CGame_WalkAndComment FUN_0040e200
  case 3: Sprache 028_ROD_03 028_LOU_03 | S= 3E84=1 3EA0=1 | S? 3EE0 | ruft FUN_0040e200
  == 1: Sprache 010_ROD_05 028_ROD_05 028_LOU_05 | S= 3EE0=2 | ruft CGame_WalkAndComment FUN_0040e200
  == 2: Sprache 010_ROD_06 028_ROD_06 028_LOU_06 | ruft CGame_WalkAndComment FUN_0040e200
  case 4: Sprache 028_ROD_04 028_LOU_04 | S? 3EE0 | ruft FUN_0040e200
  case 5: Sprache 010_ROD_07 028_ROD_08 028_LOU_08 | ruft CGame_WalkAndComment FUN_0040e200 CGame_Random
  case 6: Sprache 028_ROD_09 028_LOU_09 | ruft FUN_0040e180 FUN_0040e200
  == 3: Sprache 028_ROD_07 028_LOU_07 | ruft FUN_0040e200
  case 7: Sprache 028_ROD_10 028_LOU_10 010_ROD_08 | ruft FUN_0040e200 CGame_WalkAndComment
  case 8: Sprache 028_ROD_11 028_LOU_11 010_ROD_12 | S= 3EDC=9 | ruft FUN_0040e200 CGame_WalkAndComment
  case 9: Sprache 028_ROD_12 028_LOU_12 029_ROD_01 029_LOU_01 029_ROD_02 029_LOU_02 029_ROD_03 029_LOU_03 029_ROD_04 029_LOU_04 010_ROD_11 | S= 3EE4=1 3EDC=5 | ruft CSzene_DisableInput CSzene_GetArea FUN_0040e200 FUN_00440400 FUN_0040e2a0 CLayer_Disable CLayer_Enable CAnimLayer_PlayRange FUN_00435ba0 CGame_Speak FUN_0040e260 CAnimLayer_ShowFrame CSzene_EnableInput CGame_WalkAndComment
  case 10: Sprache 010_ROD_09 | ruft CGame_WalkAndComment
  case 11: Sprache 010_ROD_10 | ruft CGame_WalkAndComment
  case 20: ruft FUN_0040e2a0
```

## Analyse (2026-10-28, Port `src/game/loc05.cpp`)
Konstruktor `0x0040D530`: `S10` mit Ladebit `0x3EC8 != 0` (dann `S10_LouiWalk` mit zehn
Klang-Marken: Schritte bei 42/47/54/58/140/146/151, Wasser 93/112, Tock 129). Tür-Klang,
eigenes Ereignis `+0x88` (Haltungs-Timer), Musik `Loc05_LouisOffice`, Hall (4, 0.208, 0.478, 0),
Anteil 0.1.

Flags: `0x3ED8` Begrüßung beim Betreten (wird gelöscht), `0x3EDC` Gesprächsstufe, `0x3EE0`
Blumen (1 → ROD_05 und 2, 2 → ROD_06, sonst nichts), `0x3EE4` Gang durchs Büro gesehen.
Stufe 3 setzt `0x3E84 = 1` (Ramil besucht Rodrigo, Loc04) und `0x3EA0 = 1`.

### Start `0x0040D890`
Ohne Spielstand: Loui wendet sich zu, Tür (`S10_DoorOpen` 0–4, Knarren, 5–Ende), Held bei
(311, 218) Richtung 7; dann entweder `WalkTo2(419, 255, –, 1)` oder (bei `0x3ED8`) vor den
Tisch (283, 336) und Begrüßung `010_LOU_01`, `010_ROD_01/LOU_02`, `010_ROD_02/LOU_03`.
Danach Haltungs-Timer und Bereiche (Ereignisse 1–11).

### Haltung von Loui `0x0040E2A0`
`S10_LouiTalk +0x2B4` = Haltung (0, 1, 2), `+0x2B8` = letzter Übergang. Übergänge (von–bis Bild,
neue Haltung): 1 (1–15, 2), 2 (15–1, 0), 3 (1–3, 1), 4 (4–1, 0), 5 (15–3, 1), 6 (3–15, 2),
7 (69–122, 2 – Geste). Modus 1/2: Übergang direkt, 3/4: Zielhaltung aus der Tabelle
(0→1: 3, 0→2: 1, 1→0: 4, 1→2: 6, 2→0: 2, 2→1: 5); 2/4 warten (laufenden Übergang über
`0x004434A0` zu Ende, Klickzustand `CSzene+0x514` wird wiederhergestellt). Modus 6: Timer
1–2 s; Modus 5 (Ereignis 20): läuft noch → 3–6 s; sonst Zufallsübergang (Haltung 0: 1/4 → 1,
sonst 3; 1: 1/4 → 6, sonst 4; 2: je 1/3 → 2, 5, 7), Timer 5–7 s.

### Gespräch (Ereignis 6)
Stufen 1–4 fest, 5 zufällig (ROD/LOU_05…07) oder bei `0x3EC8` und nicht `0x3EE4` → 6;
6 → 7 → 8; 8: mit `0x3E84 == 2` ROD/LOU_11, dann 9: Loui geht mit Rodrigo durchs Büro
(`S10_LouiWalk` 1–43, 44–60, 61–96, Gespräch 029_*, 124–140, 141–150, 151–174, Rodrigo dreht
sich mit), danach `0x3EE4 = 1`, Stufe 5.

Abweichung: unbekannte Ereignisse zeigen im Original `MessageBoxA("Unknown_Event____")`; der
Port schreibt nach stderr.
