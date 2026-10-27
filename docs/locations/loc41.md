# Loc41 (automatische Übersicht)

Funktionen `0x004332A0`–`0x0043389F` (5 Stück), Ereignis-Handler `0x004336E0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 004336e0.c FUN_004336e0 @ 004336e0
  Anfang: ruft FUN_004336e0
  == 130: ruft CGame_PlayMusic FUN_00437560 FUN_004375b0 CAnimLayer_SetFrameEvent
  == 160: Sprache 112_SPR_01 | ruft CGame_Say
  == 2300: ruft CGame_PlayMusic FUN_00437fd0
  == 1650: ruft CGame_PlayMusic
  == 1900: ruft CGame_PlayMusic FUN_00437fd0
  == 2880: ruft CAudioBuffer_FadeVolume CGame_PlayMusic FUN_00437fd0 CAnimLayer_SetFrameEvent
```
