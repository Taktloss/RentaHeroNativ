# Hinweissystem (Hilfe-Symbol und HLP-Texte)

Belege: `CGame_Tick` `0x00440B80`, `Hints_Select` `0x004448D0`, `CGame_SetHint`
`0x00440BE0`, `CGame_ShowHint` `0x00440C40`, `CHelp_Show` `0x004414C0` (hlp.md),
Hilfe-Objekt `CSzene+0x9F8` (= `CGame+0x0C`), `CHelp_IconOn` `0x004414A0` /
`CHelp_IconOff` `0x004414B0`.

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
