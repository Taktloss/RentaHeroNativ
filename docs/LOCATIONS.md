# Schauplätze: Übersicht

Automatisch erzeugt mit `python3 -I tools/loc_summary.py docs/symbols/names.tsv
re/private/decomp-game re/private/decomp-ev` aus den lokalen Dekompilaten (Zuordnung
Funktion → Schauplatz über den Adressbereich ab `LocNN_ctor`). Grundlage für die
Einzeldokumentation; Bedeutung der Flags siehe formats/gamestate.md.

## Aufbau eines Schauplatzes (belegt)
- Konstruktor `LocNN_ctor` lädt Szene(n), legt Ebenen/Animationen/Bereiche an und
  registriert Ereignis-IDs: Klickbereiche über `CArea_SetEvent(area, EventValue(id))`,
  Timer über `CEvent_SetTimerHandler(event, ms, …, EventValue(id))` (Zufallsdauer mit
  `CGame_Random`, z. B. Loc20 `0x00422D90`: 5–15 s).
- `CGame_SetEventProc(this, handler)` setzt den Ereignis-Handler (`g_EventProc`,
  EVENTS.md). Der Handler verzweigt nach `event->Param` (= ID), teils zuerst nach der
  aktuellen Szene `CGame+0x2C` (z. B. Loc02 `0x00408D50`: Szenen 13, 14, 15, 1013, 2013
  je eigene Unterfunktion).
- Unbekannte ID → `MessageBoxA("Unknown Event …")` (z. B. Loc09 `0x00413E70`).
- Zeitgesteuerte Abläufe (Lippensync-Dialoge) über Animations-Cues: Ereignis mit
  Bildnummer (`CAnimLayer+0x2C0`) als Schalter, je Fall `CGame_Say(…)` und nächster Cue
  (Loc09 Ereignis 2: Bilder 0x105 → 0x123 → 0x14B …).
- Szenenwechsel `CGame_ChangeScene(this, szene, loc)` (SCRIPTING.md). Szenen ≥ 1000
  sind Varianten einer Szene (z. B. 1007 zu 7, 2013 zu 13).

## Tabelle
Spalten: „IDs registriert“ = Literale in `EventValue(n)`; „im Handler erkannt“ = Fälle
in Switch/If-Ketten über `event->Param` (Skript-Erkennung, Untergrenze, da Ghidra
Variablen wiederverwendet); „nur registriert“ = registriert, aber im Handler nicht
erkannt (UNVERIFIED: bei der Einzelanalyse prüfen). „Wechsel nach“ = `Loc/Szene`.
Flags = Offsets im Zustandsblock (hex).

| Schauplatz | Funktionen | Ereignis-Handler | IDs registriert | im Handler erkannt | nur registriert | Wechsel nach (Loc/Szene) | Flags schreibt | Flags liest nur | Musik |
|---|---:|---|---|---:|---|---|---|---|---|
| Loc01 | 16 | `004065C0` | 28 (1–200) | 30 |  | 1/7, 1/9, 1/46, 1/1007, 3/12, 5/10, 6/26, 7/47, 31/113 | 3E94 3E98 3E9C 3EC4 |  |  |
| Loc02 | 15 | `00408D50` | 21 (1–45) | 22 | 2 3 |  | 3EA0 3EA8 3EAC 3EB0 3EB4 3EB8 3EBC 3EC0 3ECC | 3EA4 3EC8 |  |
| Loc03 | 7 | `0040AE30` | 14 (1–14) | 14 |  | 1/7, 2/13 | 3EC8 3ECC |  |  |
| Loc04 | 12 | `0040C570` | 18 (1–30) | 20 |  | 1/7 | 3E84 3E88 3ED0 3ED4 |  |  |
| Loc05 | 10 | `0040DB30` | 12 (1–20) | 12 |  | 1/9 | 3E84 3EA0 3ED8 3EDC 3EE0 3EE4 | 3EC8 |  |
| Loc06 | 9 | `0040F040` | 10 (1–20) | 10 |  | 6/26, 6/27 | 3E8C 3EE8 3EEC 3EF0 |  |  |
| Loc07 | 5 | `00410870` | 30 (1–110) | 31 |  | 1/46, 7/47, 7/48, 7/49, 7/50, 7/51, 7/52, 7/53, 7/54, 7/55, 7/56, 15/57 | 3EF4 3EF8 3EFC 3F00 3F04 3F08 3F30 | 3FC8 |  |
| Loc08 | 9 | `004124B0` | 12 (1–101) | 13 |  |  | 3F0C 3F10 3F14 3F18 3F1C | 3F20 4088 |  |
| Loc09 | 5 | `00413E70` | 2 (1–2) | 2 |  | 14/90 | 3E80 3F04 3F48 3F54 |  |  |
| Loc10 | 7 | `00415E60` | 57 (1–200) | 61 |  | 1/8, 8/99, 10/37, 10/38, 10/39, 10/40, 10/41, 10/42, 10/43, 10/44, 12/91 | 3E80 3EFC 3F00 3F04 3F24 3F28 3F2C 3F30 3F34 3F3C 3F44 3F4C 3F50 3F54 3F58 3F5C 3F64 3F68 3F6C | 3F38 3F40 3F48 3F60 |  |
| Loc12 | 5 | `00418BF0` | 6 (1–6) | 7 |  | 10/42, 14/90 | 3F60 3F74 3F78 3FA0 3FBC | 409C |  |
| Loc14 | 6 | `00419C90` | 8 (1–11) | 8 |  | 12/91 | 3F7C 3F80 3F84 3F88 3F8C |  | Loc14_Cynthia Loc14_LoveIsBreakingTheNight |
| Loc15 | 6 | `0041ACF0` | 7 (1–11) | 7 |  | 7/53, 10/38, 16/58, 31/113 | 3E80 3EFC 3F00 3F04 3F38 3F90 3F94 3F98 3F9C 3FA0 | 3F24 |  |
| Loc16 | 10 | `0041BDC0` | 7 (1–7) | 7 |  | 15/57, 17/59 | 3F40 3F74 3FA4 3FA8 3FAC 3FB0 3FB4 3FB8 3FC0 3FC4 | 3E80 3E90 3FBC 3FC8 409C |  |
| Loc17 | 5 | `0041D9F0` | 17 (1–100) | 31 |  |  | 3F44 3FA8 3FC8 3FCC 3FD0 3FD4 3FD8 | 3FE4 |  |
| Loc18 | 9 | `0041EC80` | 7 (1–14) | 9 |  | 17/60 | 3FDC 3FE0 3FE4 3FE8 |  |  |
| Loc19 | 8 | `004202D0` `00420500` `00420D60` | 10 (1–1000) | 9 | 11 | 10/38, 19/68, 19/69, 20/70, 21/71 | 3F28 3F48 3FF0 3FF4 3FFC 4004 4008 400C 4010 4014 4018 401C | 4028 |  |
| Loc20 | 6 | `00422930` | 3 (1–10) | 1 | 2 10 | 19/69 | 4020 4024 4028 402C 4030 | 3FEC |  |
| Loc21 | 5 | `00424600` | 44 (10–200) | 48 |  | 19/69, 21/71, 21/72, 21/73, 21/74, 21/75, 21/76, 21/77, 21/78, 21/79, 21/80, 22/81 | 4034 4038 403C 4040 4044 4048 404C 4054 4058 405C |  |  |
| Loc22 | 9 | `00426510` | 4 (1–4) | 17 |  | 21/79, 22/81, 22/1081 | 3FEC 4060 | 4064 4068 406C 4070 4074 4078 |  |
| Loc23 | 5 | `004275D0` | 5 (1–5) | 5 |  | 24/103 | 407C 4080 | 4084 |  |
| Loc24 | 8 | `00428820` | 20 (1–100) | 20 |  | 23/102 | 4088 408C |  |  |
| Loc25 | 6 | `00429810` | 10 (1–200) | 10 |  |  | 4090 |  |  |
| Loc26 | 6 | `0042A8E0` `0042AC00` | 10 (1–1000) | 8 | 10 11 | 16/58, 26/93, 26/98, 27/95 | 4094 409C 40A0 40A4 | 3F18 4098 |  |
| Loc27 | 6 | `0042BB70` | 13 (1–30) | 13 |  | 25/104, 26/93, 27/95, 27/96 | 3E90 4094 4098 40A8 40AC 40B0 40B4 40B8 40C0 | 40BC |  |
| Loc28 | 7 | `0042D940` `0042E570` `0042EA20` | 21 (1–1000) | 21 | 1 2 3 4 | 28/31, 28/32, 29/33, 30/34, 31/113 | 3E8C 40C4 40C8 40CC 40D0 40D4 40D8 40DC 40E0 40E4 40E8 40F0 |  |  |
| Loc29 | 5 | `0042F290` | 3 (1–3) | 4 |  | 28/32, 30/34 | 40F4 40F8 |  |  |
| Loc30 | 5 | `0042FC70` | 8 (1–8) | 8 |  | 28/35, 28/36 | 40FC 4104 4108 | 4100 |  |
| Loc31 | 6 | `00430770` | 5 (1–200) | 5 |  | 1/8, 10/37, 15/57, 28/31 |  | 3E88 3F98 |  |
| Loc35 | 5 | `00430D70` | 2 (1–2) | 3 |  | 40/115 |  |  |  |
| Loc37 | 5 | `004316F0` | 4 (1–4) | 4 |  | 40/116 | 410C |  |  |
| Loc40 | 7 | `00432EF0` `004330D0` `00433100` | 3 (1–3) | 3 |  | 1/7, 35/1, 37/3 |  |  | Loc09_SmashvilleAttack Loc35_RodrigoOnGlider |
| Loc41 | 30 | `004336E0` | 2 (20–21) | 2 |  |  |  |  | Loc05_LouisOffice Loc06_Ranama Loc14_Cynthia Loc26_IslandOfMegophias Loc35_RodrigoOnGlider Loc41_InALand |
