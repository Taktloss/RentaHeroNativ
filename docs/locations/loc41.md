# Loc41 – Schlusssequenz und Abspann, Szene 112

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

## Einzelanalyse (2026-11-04, portiert: `src/game/loc41.cpp`)
- Ctor `0x004332A0`: `CSzene_LoadAll("S112")`; `S112_EndeSequenz` mit Ton-Cues (Donner,
  Explosion, Vögel mit Aktion 0x32 per `CAnimLayer_AddCueAction` `0x00442C10`), Bildereignis 0x14
  bei 130; `S112_Credits` Bildereignis 0x15 bei 1650. Musik `Loc41_Outro`.
- Start `0x00433610`: Musik `play` (kein FadeIn), EndeSequenz bis zum Ende, Meer in Schleife,
  Abspann bis zum Ende, Meer bis zum Ende, `S112_MeerFadeOut`, dann `g_SaveRequest = 5` (Beenden).
- Ereignisse `0x004336E0`: Bild 130 → `Loc41_InALand` (Hall 15), nächstes Bildereignis 160 →
  `112_SPR_01`. Abspann-Musik nach Bild: 1650 `Loc35_RodrigoOnGlider`, 1900 `Loc05_LouisOffice`,
  2100 `Loc06_Ranama`, 2300 `Loc26_IslandOfMegophias`, 2500 `Loc14_Cynthia` (je FadeIn 100/1 s
  per `0x00437FD0`, außer Glider: `play`), 2880 Ausblenden in 5 s.
- Probelauf 2026-11-04: ganze Sequenz, alle sechs Musikwechsel, Spiel beendet sich nach ~334 s.
