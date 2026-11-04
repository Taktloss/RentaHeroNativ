# Hinweissystem (Hilfe-Symbol und HLP-Texte)

Belege: `CGame_Tick` `0x00440B80`, `Hints_Select` `0x004448D0`, `CGame_SetHint`
`0x00440BE0`, `CGame_ShowHint` `0x00440C40`, `CHelp_Show` `0x004414C0` (hlp.md),
Hilfe-Objekt `CSzene+0x9F8` (= `CGame+0x0C`), `CHelp_IconOn` `0x004414A0` /
`CHelp_IconOff` `0x004414B0`.

## Port
`Game::tick`/`setHint`/`showHint`/`selectHint` (`src/game/game.cpp`); `CGame_Tick` läuft in
`main.cpp` nach `CSzene_DispatchEvents`, nicht bei offenem Spielmenü. `CGame_ctor` (`0x0043F4D0`)
setzt `+0x10`/`+0x14`/`+0x18` auf 0. Tests `game_hint_*`; Probelauf Szene 6 (`04_006_A`, 1 s):
Symbol erscheint, Klick zeigt den Text.

## Ablauf
- `CGame_Tick` frühestens alle 5001 ms (TIMING.md): `Hints_Select`, dann – wenn ein
  Hinweis gesetzt ist (`CGame+0x18`) und der Countdown `CGame+0x10 > 0` –
  `Countdown −= vergangene ms`; unter 0 → `−1` und Hilfe-Symbol einblenden.
  Der erste Tick nach Schauplatzbeginn zieht 0 ab.
- `CGame_SetHint(name, ms)`: `name == NULL` → Symbol aus, Countdown 0, kein Hinweis.
  Countdown 0 → auf `ms` setzen. Anderer Name als bisher → Name und Countdown neu.
  Sonst läuft der Countdown weiter. (Name als Zeiger gespeichert.)
- Klick oben rechts (`y < 60`, `x ≥ 581`) bei sichtbarem Symbol → `CGame_ShowHint`:
  nur wenn Countdown `−1`, HLP-Datei `name` anzeigen. Klick bei offener Hilfe schließt sie.
- Countdown pro Szene im Spielstand: gamestate.md.

## Tabelle (`Hints_Select`, Schlüssel = aktuelle Szenennummer `CGame+0x2C`)
`S[x]` = Zustandsflag, `INV(n)` = Gegenstand n im Inventar. Zeiten in ms.

| Szene | Bedingung | Datei | Zeit |
|---:|---|---|---:|
| 6 | `S[0x3ED0] == 1` | `04_006_A` | 1 000 |
| 6 | sonst `S[0x3ED4] != 0` | `04_006_B` | 300 000 |
| 7, 46, 1007 | `S[0x3E9C] && S[0x3E94] && S[0x3EC4] && !INV(13)` | `01_046_A` | 300 000 |
| 27 | `S[0x3E8C] != 0` | `06_027_A` | 300 000 |
| 31 | nicht alle von `INV(1)`, `INV(3)`, `INV(4)`, `INV(14)` | `28_031_A` | 1 200 000 |
| 40 | `S[0x3F48] && S[0x3F54] ∈ {0, 1}` | `10_040_A` | 600 000 |
| 41 | `S[0x3F30] && S[0x3F5C] ∈ {1, 2}` | `10_041_A` | 300 000 |
| 52 | `S[0x3F30] != 0` | `07_052_A` | 300 000 |
| 61 | `S[0x3FE0] == 0` | `18_061_A` | 600 000 |
| 70 | `S[0x3FEC] != 0` | `20_070_A` | 600 000 |
| 73 | `S[0x4038] == 0` | `21_073_A` | 600 000 |
| 91 | `S[0x3F74] == 2` | `12_091_A` | 1 200 000 |
| 95 | `S[0x40B4] != 0` | `27_095_A` | 600 000 |
| 100 | `S[0x3F14] == 0` | `08_100_A` | 300 000 |
| 105 | `S[0x4090] == 1` | `25_105_A` | 300 000 |
| 1081 | immer | `22_081_A` | 1 200 000 |

Trifft nichts zu (auch: Bedingung der Szene nicht erfüllt) → `CGame_SetHint(NULL)`.
`37_102_A.HLP` liegt auf der CD, wird aber nirgends verwendet.
Szenennummern ≥ 1000 (1007, 1081) sind Sonderszenen; Zuordnung in Phase 4.

## Hilfe-Objekt `CHelp` (global `0x0047D150`, `CSzene+0x9F8`)
Belegt an `0x00440F70`–`0x004416F0`, WinMain `0x00401E00`, `Game_SwitchLocation`,
Renderer `0x0043EBC0`/`0x0043E1D0`.

| Off | Bedeutung |
|---|---|
| `+0x00` | Symbol sichtbar |
| `+0x04` | Hilfe offen |
| `+0x08` | `CLayer` Symbol (`HelpIcon.tcc`) |
| `+0x0C` / `+0x10` | `CLayer` Maske links (`Helpmask.tcc`, (0, 0)) / rechts (`Helpmaskr.tcc`, (320, 0)) |
| `+0x14` | aktuelle Maske |
| `+0x18` / `+0x1C` / `+0x20` / `+0x24` | Renderer / Szene / `CGame` / `HWND` |
| `+0x28` | Pfad des Symbols |
| `+0xF0` | Textrechteck |
| `+0x100` | Schrift `CreateFontA(15, …, 400, …, "Arial")` |

- Ctor `0x00440F70`: alles 0, Schrift, dann `0x00441080` (rechte Seite als Vorgabe).
- `0x004410C0(hwnd, renderer, szene, game)`: Werte ≠ 0 übernehmen; bei neuem Renderer oder
  fehlendem Symbol `0x00441110(NULL)` und `0x00441490`. WinMain ruft es mit allem, danach
  `0x00441110("HelpIcon.tcc")` und setzt `CSzene+0x9F8`; `Game_SwitchLocation` ohne Renderer.
- `0x00441110(name)`: `name ≠ NULL` → Pfad per `CSzene_FindPath`; sonst ohne gespeicherten
  Pfad nichts. Drei `CLayer` neu (alte löschen), je Z = 1.0, Slot 3, `0x00446C90`,
  `0x00446DE0(0)`, `SetPos`, `CLayer_Enable`, Name = Dateiname: Symbol (580, 0), Maske links
  (0, 0), rechts (320, 0). Scheitert eine Datei, werden die folgenden nicht geladen.
- Symbol an `0x004414A0` (nur wenn aus) / neu zeichnen `0x00441490` (nur wenn an) →
  `0x004413E0`: Symbol fehlt → `0x00441110(NULL)`; dann `SetPos(580 + Client-x, Client-y)`
  und `Draw(Ausgabeoberfläche Renderer+0x14, flag 0)` – **direkt auf den Bildschirm**
  (flag 0: Ziel vorher schwarz), `+0x00 = 1`.
- Symbol aus `0x004414B0` (nur wenn an) → `0x00441450`: Renderer-Slot 1 (`0x0043EBC0`):
  Bildschirmrechteck (580, 0, 640, 60) mit dem Farbschlüssel (Schwarz) füllen, `+0x00 = 0`.
- Klick auf das Symbol (`CSzene_OnClick`, `y < 60`, `x ≥ 581`, ohne Editor) →
  `0x004416E0` → `CGame_ShowHint(+0x20)` (oben) → `CHelp_Show(name)`.
  **Unabhängig davon, ob das Symbol sichtbar ist** – es zählt nur der Countdown −1.
- `CHelp_Show(name)` `0x004414C0`: `CSzene_FindFile(name, "hlp")`, ganze Datei + 100 Nullbytes,
  `Hlp_Decode`. Figur `CSzene+0x5E0` fehlt oder `x (+0x15D0) > 319` → links (`0x00441040`:
  Rechteck (20, 20, 300, 340), Maske links, `SetPos(0, 0)`), sonst rechts (`0x00441080`:
  (340, 20, 620, 340), Maske rechts, `SetPos(320, 0)`). Maske mit flag 1 in die
  **Szenenoberfläche** (Renderer+0x18), dann `GetDC` auf dieselbe und
  `DrawTextA(…, 0x15)` weiß, transparent (= `DT_CENTER | DT_WORDBREAK`; `DT_VCENTER` ohne
  `DT_SINGLELINE` wirkungslos → oben). Danach `0x0043E1D0` (Ausgabe (0, 0, 640, 420)),
  `+0x04 = 1`. Scheitert `GetDC` → `0x004416C0`.
- Schließen `0x004416C0`: `+0x04 = 0`, `CSzene_InvalidateAll` (Szene neu zusammensetzen).
  Aufrufer: `CSzene_OnClick` (Klick bei offener Hilfe, vor allen Sperren), Tastatur
  `0x0044CE50`.
- `CSzene_MouseMove` tut bei offener Hilfe nichts außer `+0x500/+0x504`.
