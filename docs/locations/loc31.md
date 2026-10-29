# Loc31 – Flugkarte des Gleiters (Szene 113)

Funktionen `0x00430360`–`0x00430A1F` (6 Stück), Ereignis-Handler `0x00430770`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S113_To8` | `0x00430670` |
| 2 | `S113_To37` | `0x00430670` |
| 3 | `S113_To57` | `0x00430670` |
| 4 | `S113_To30` | `0x00430670` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00430770.c FUN_00430770 @ 00430770
  Anfang: ruft FUN_00430770
  case 1: → 1/8 | ruft FUN_00430990
  case 2: → 10/37 | ruft FUN_00430990
  case 3: → 15/57 | ruft FUN_00430990
  case 4: → 28/31 | ruft FUN_00430990
  case 200: ruft CGame_Random
  == 1: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
```

## Analyse (2026-10-29, Port `src/game/loc31.cpp`)
Konstruktor `0x00430360` setzt die Szene fest auf 113 (`+0x2C = 0x71`), lädt `S113` komplett,
die Abflug-Animationen `S113_To30/37/57/8` (+0x7C…+0x88), die Schleife `S113_BackLoop` (+0x8C),
vier Bereiche, Klänge (Möwe, Wind, Meer 40 %, Gleiter, Tür) und den Timer +0xB4. Musik
`Loc01_Smashville`, Hall (0x10, 0.111, 2.767, 0.224), Anteil 0.1. Abbau `0x00430550` gibt
alles frei.

Start `0x00430670`: Schleife `start(13)`, Meer in Schleife (vtable +0x10), Ziele:

| Ereignis | Bereich | Bedingung | Ziel (Szene, Schauplatz) |
|---:|---|---|---|
| 1 | `S113_To8` | immer | 8, Loc01 (Smashville) |
| 2 | `S113_To37` | immer | 37, Loc10 |
| 3 | `S113_To57` | `0x3F98 != 0` | 57, Loc15 |
| 4 | `S113_To30` | `0x3E88 != 0` | 31, Loc28 |

Handler `0x00430770`: Türklang, `0x00430990(anim)` (Eingabe aus, Schleife mit Endmodus beenden
und auf Ende warten, Gleiterklang Frequenz 25 → 100 % in 2 s, Abflug-Animation modal bis zum
Ende), dann Szenenwechsel – ohne Prüfung des Wartergebnisses. Ereignis 200: Möwe (Lautstärke
50–100) oder Wind zufällig im Raum, neu alle 3–5 s.

Der Abflug-Animation übergibt der Handler als Stack-Argument (Ghidra zeigt `unaff_retaddr`):
To8 bei 0x00430815, To37 0x0043083D, To57 0x00430865, To30 0x0043088D.
