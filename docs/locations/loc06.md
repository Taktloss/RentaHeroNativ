# Loc06 – Ranama im Kloster (Szenen 26, 27)

Funktionen `0x0040E730`–`0x0040F7AF` (9 Stück), Ereignis-Handler `0x0040F040`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S26_Exit` | `0x0040EBC0` |
| 2 | `S26_Bottle` | `0x0040EBC0` |
| 3 | `S26_Telescope` | `0x0040EBC0` |
| 4 | `S26_Background` | `0x0040EBC0` |
| 5 | `S26_RanTalk (Ebene)` | `0x0040EBC0` |
| 5 | `S26_Ranama` | `0x0040EBC0` |
| 10 | `S27_Exit` | `0x0040EBC0` |
| 11 | `S27_RanSchautRing (Ebene)` | `0x0040EBC0` |
| 12 | `S27_RanTalkRing (Ebene)` | `0x0040EBC0` |
| 13 | `S27_RanSchaut (Ebene)` | `0x0040EBC0` |
| 13 | `S27_RanTalk (Ebene)` | `0x0040EBC0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040f040.c FUN_0040f040 @ 0040f040
  Anfang: ruft FUN_0040f040
  case 1: ruft FUN_0040f6d0
  case 2: Sprache 026_ROD_07 026_ROD_06 | ruft CGame_WalkAndComment
  case 3: Sprache 026_ROD_08 | ruft CGame_WalkAndComment
  case 4: Sprache 026_ROD_09 | ruft CGame_WalkAndComment
  case 5: Sprache 026_ROD_05 | → 6/27 | S? 3EE8 | ruft FUN_004409c0 CActor_WalkTo
  case 11: Inv Add(14) | ruft CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayRange
  case 10: → 6/26
  case 13: Sprache 027_ROD_01 027_RAN_01 027_ROD_02 027_RAN_02 027_ROD_03 027_RAN_03 027_ROD_04 027_RAN_04 027_ROD_05 | S= 3EF0=2 | S? 3EF0 | ruft FUN_0040f640 FUN_0040f690
  case 20: Sprache 027_RAN_08 027_ROD_06 027_RAN_05 | S= 3EEC=3 | S? 3E8C 3EEC | ruft CSzene_DisableInput CLayer_Disable CAnimLayer_PlayRange CLayer_Enable FUN_0040f690 CSzene_EnableInput FUN_0040f640
  == 1: Sprache 027_ROD_07 027_RAN_06 | S= 3EEC=2 | ruft FUN_0040f640 FUN_0040f690
  == 2: Sprache 027_ROD_08 027_RAN_07 027_ROD_09 | S= 3EEC=3 | ruft FUN_0040f640 FUN_0040f690 CSzene_DisableInput CLayer_Disable CLayer_Enable CAnimLayer_PlayRange CSzene_EnableInput CEvent_SetTimerHandler
```

## Einzelanalyse und Port (`src/game/loc06.cpp`)
Belege: `Loc06_ctor` `0x0040E730`, Dtor `0x0040EAF0`, Start `0x0040EBC0`, Handler
`0x0040F040`, Hilfen `0x0040F4E0`…`0x0040F6D0`. Szenen-SZN „026“/„027“ (`0x0047210C`/`0x00472098`).
Musik `Loc06_Ranama`, Hall `(6, 0.403, 4.279, 0.5)`, Anteil 0.1.

Flags: `S[0x3EE8]` Besuche in Szene 26 (jeder Ctor mit Szene ≠ 27 zählt +1; 1 = erster Besuch:
Ranama schwebt, sonst Ranama mit Besen; Szene 27 setzt 2), `S[0x3E8C]` Ranama trägt den Ring
(SZN-Teil von 027), `S[0x3EEC]` Gesprächsstufe Ring (1→2→3, dann Griff zum Ohr), `S[0x3EF0]`
allgemeines Gespräch (1 → 2). Vorgängerszenen 1001/1002/1003/2001/2003 setzen Zustände vor
(Sprungziele): 1001 → Besuche 0, 1002 → 1, 1003 → 2, 2001 → 2 + Ring, 2003 → 2 + Ring +
Stufe 3.

- Erster Besuch: Ranama schwebt per `CLayer_MoveTo(0, 111/124, 0, 0, 13)`, Rückruf ist
  `0x0040F4F0` selbst (Stufe `+0xBC`: 0/1 auf und ab). Verlassen (`0x0040F6D0`) setzt die
  Stufe auf 3 bzw. 2 (wenn gerade aufwärts) → nach Ankunft unten fällt Ranama
  (`S26_RanFaellt`, Klopfen + Gong), 1 s, dann **Loc01 / Szene 7**.
- Ring (Szene 27, Ereignis 12, Stufe 3): Ranama schaut weg, 2 s Timer → Ereignis 20 (zurück);
  in der Zeit Klick aufs Ohr (Ereignis 11, `S27_OhrGreif`) → Gegenstand 14, zurück in Szene 26.
  Mit Gegenstand 14 beim Verlassen: `027_RAN_09/10` (Modus 0x0D, Bilder 46…52), Held läuft
  mit `+0x54 = 20` (doppelte Bildrate) hinaus.
- Inventar-Rückruf `0x0043FA60(0x0040F4E0)`: `CGame_DescribeItem(typ)`.
- **Original-Eigenheit**: Beide `CActor_SetPos`-Aufrufe in Start schieben nur x, y; die
  Richtung kommt von einem gemeinsamen `PUSH 6` vor der Verzweigung (`0x0040EC79`) → 6.
- Wartestellen-Ergebnisse werden in Loc06 nirgends geprüft.

Geprüft (Port): erster Besuch mit Originaldaten (Ranama schwebt, Gespräch); Kette
Loc01/S7 → Gleiter → S8 → „Kloster“ → Loc06/S26 (`--click-at`). Test
`loc06_visit_counter_and_entry_flags`.
