# Schauplatz-Basisklasse `CGame` und Schauplatzwechsel

Belege: `CGame_ctor` `0x0043F4D0`, `CGame_dtor` `0x0043F580`, `CGame_SetEventProc`
`0x0043FA30`, `CGame_GetViewObjects` `0x0043F6C0`, `CGame_ChangeScene` `0x00440A80`,
`RequestLocation` `0x00404B20`, `Game_SwitchLocation` `0x004036E0`, `Game_Idle` `0x004030C0`,
`CSzene_Reset` `0x00448930`, `CSzene_Clear` `0x004487A0`. API der Schauplätze: SCRIPTING.md.

## Felder (Auswahl)
| Off | Bedeutung |
|---|---|
| `+0x04` | 1 während eines geladenen Spielstands (Game_SwitchLocation) |
| `+0x08` | Musik |
| `+0x0C` | Hilfe-Objekt (`CSzene+0x9F8`) |
| `+0x10`/`+0x14`/`+0x18` | Hinweis-Countdown / – / Hinweisname (HINTS.md) |
| `+0x1C` | Held (`CSzene+0x5E0`) |
| `+0x20` | Zufalls-Startwert (`timeGetTime`) |
| `+0x24` | 1 (Ctor), 0 im Dtor |
| `+0x28` / `+0x2C` / `+0x30` | Schauplatz / Szene / vorherige Szene |
| `+0x34`…`+0x44`, `+0x4C` | Warteplätze, letzter Platz (SCRIPTING.md) |
| `+0x50` | Szene |
| `+0x70` | Sprachobjekt (SPEECH.md) |
| `+0x74` / `+0x78` | Inventarleiste (`CSzene+0x5B4`) / Spielzustand `CGameState` (`CSzene+0x5B8`, formats/gamestate.md; Port: `GameContext::state`) |

## Lebenszyklus
- Ctor: Felder 0, `SetEventProc(0)`, `+0x24 = 1`, Startwert. Der LocNN-Ctor setzt
  Schauplatz/Szene/vorige Szene/Szene, lädt die SZN (`CSzene_LoadAll`) und Objekte.
- `CGame_SetEventProc(h)`: `g_EventProc = h` (`0x0043F1C0`), zusätzlich an die Leiste
  (`0x00441E60(h, this)`).
- `CGame_GetViewObjects`: Figuren, Hilfe, Karten, Leiste aus der Szene übernehmen;
  `CSzene+0x5D4 = +0x2C`; Hinweis-Countdown aus der Tabelle `+0x78` (−1 → Symbol an).
- Dtor: belegte Warteplätze freigeben (je 3 Nachrichten verarbeiten), `+0x24 = 0`,
  `SetEventProc(0)`, Leisten-Handler 0, Leiste `0x00442280`/`0x00441E60(0, 0)`, Countdown in
  die Tabelle zurück, Ton-Objekte `+0x60…+0x6C` löschen, Sprache stoppen (`0x00446030`),
  Hilfe-Symbol aus, **`CSzene_Reset`**, `CSzene+0x5BC = 0`, `0x0047D130 = 0`.
- `CSzene_Reset`: `+0x00 = 0`; Ebenen und Bereiche löschen; Figuren löschen; Hintergrund
  `+0x5C0`, `+0x2B0`, Karten `+0x2BC/+0x2C0` und Vollbildobjekt `+0x5DC` nur auf 0 setzen
  (nicht löschen); Trefferraster leeren; `g_SceneChanged = 0`; Z-Grenzen 1/0xFF (Zählfeld
  `+0x300` bleibt!); beide Rechtecklisten leeren; Text-Umschalter aus; `+0x2C8 = 0`;
  `DisableInput`, `+0x510 = 1` (`0x0044B180`), `SetClickEvent(0)`.

## Wechsel
- `CGame_ChangeScene(szene, ort)`: belegte Warteplätze → 3 (abgebrochen), Abbruch-Flag =
  ob einer belegt war; `RequestLocation(ort, szene, +0x2C, abbruch)`.
- `RequestLocation`: `g_PrevLocation = g_NextLocation`, Ziel, vorige Szene, Abbruch,
  Flag `0x0047D750 = 1`.
- `Game_Idle` Schritt 7 (nur ohne Abbruch-Flag): Flag 0, alten `CGame` löschen,
  `Game_SwitchLocation`; scheitert es (Schauplatz unbekannt/Speicher), **mit vertauschten
  Werten** (`next = prev`, Szene ↔ vorige Szene) erneut; scheitert auch das,
  `CSzene_Clear` und Renderer `0x0043EC60`.
- `Game_SwitchLocation`: Szenenordner `menu\` bzw. `LocNN\` (Szene `+0x5E8`/`+0x6EC`),
  `+0x5D0/+0x5D4`; bei Schauplatz ≠ 0 CD-Prüfung `Config_ReadCdMap` (`0x00404360`, im Port
  entfällt sie – alle Daten sind eingebunden); Renderer `0x0043E960(0x0B)`; Schauplatz-Ctor
  über `switch` (Größe je Klasse); Renderer `(0)`; `CSzene+0x5BC = g_CurGame`;
  Spielstand-Position (`0x0047D764`); ausstehende Maus- und Tastennachrichten verwerfen;
  **Start** (vtable `+0x04`); `CHelp`-Setup mit Szene und Spiel; Statustext löschen;
  Leiste neu zeichnen (`0x00441F50`).

## Port (`src/game/`)
`Game` = `CGame`, `GameContext` hält, was im Original global ist (`g_CurScene`, `g_CurGame`,
Hilfe, Wechsel-Globals). Schauplätze sind Unterklassen (`Loc40`), `createLocation` ist der
`switch`. Bisher nur Loc40: jeder andere Wechsel scheitert und läuft in den Rücksprung mit
vertauschten Werten – nach dem Intro-Teil 1 beginnt das Intro deshalb neu. Musik
(`CSzene+0x1A8`) liegt in `GameContext::music` und überlebt Schauplatzwechsel (AUDIO.md).
Programm: `rentahero --cd … --common … --game 40 114 [--subtitles]`. Optional dritte Zahl =
Herkunftsszene (Standard 0); viele Schauplätze stellen Rodrigo nur danach auf, z. B.
`--game 3 12 8` (Brücke, mit dem Gleiter angekommen), `--game 4 6 7`, `--game 5 10 9`.
