# Loc09 – Zwischensequenz „Angriff auf Smashville“, Szene 89

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

## Einzelanalyse (2026-11-03, portiert: `src/game/loc09.cpp`)

- Ctor `0x004134B0`: Szene fest 89, `CSzene_LoadAll("xxx")` (leere Szene), Vollbild-Animation
  `Attack`, 15 Töne (`+0x80`…`+0xB8`; Ocean und Seagull Vol. 70, EndTimeFeeling Vol. 0) mit
  Ton-Cues (einfache und erweiterte; Bild 252/261 doppelt, Bild 261 ein drittes Mal – Original).
  Musik `Loc09_SmashvilleAttack`, Hall (0x10, 0.111, 2.767, 0.224) + `0x004375B0`.
- Start `0x00413DC0`: Musik abspielen (vtable +0x08), Animation modal mit Ende-Ereignis 1,
  Bildereignis 2 bei Bild 261, Meeresrauschen in Schleife. Setzt `0x3E80 = 1` (Nacht),
  `0x3F04 = 1` (Gitter in Loc07 wieder unten), `0x3F48 = 0`, `0x3F54 = 4`.
- Ereignisse `0x00413E70`: 2 je Bild – 261: Hall (10, 0.5, 10, 0.3), `084_ROD_01`, weiter 291;
  291: `084_ROD_02`, weiter 331; 331: `084_ROD_03`, weiter 338; 338: Hall zurück.
  1: `0x3E80 = 1`, `CGame_ChangeScene(90, 14)`. Sonst `MessageBoxA("Unknown_Event____")`.
- Probelauf 2026-11-03: Sequenz mit vier Bildereignissen bis zum Wechsel nach 14/90.
