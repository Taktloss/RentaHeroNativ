# Loc09 (automatische Übersicht)

Funktionen `0x004134B0`–`0x00413FEF` (5 Stück), Ereignis-Handler `0x00413E70`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00413e70.c FUN_00413e70 @ 00413e70
  Anfang: ruft FUN_00413e70
  == 1: → 14/90 | S= 3E80=1
  case 261: Sprache 084_ROD_01 | ruft FUN_00437560 FUN_004375b0 CGame_Say
  case 291: Sprache 084_ROD_02 | ruft CGame_Say
  case 331: Sprache 084_ROD_03 | ruft CGame_Say
  case 338: ruft FUN_00437560 FUN_004375b0 CAnimLayer_SetFrameEvent
```
