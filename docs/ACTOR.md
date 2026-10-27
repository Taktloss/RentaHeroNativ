# Figur (`CActor`): Position, Richtung, Tönung, Typen

Belege: Disassembly/Dekompilate `0x00434720`, `0x004347D0`, `0x00434850`,
`0x00434AE0`, `0x00433E30`; Felder des SZN-Figurenblocks siehe formats/szn.md,
Laufbefehle SCRIPTING.md, Wegsuche PATHFINDING.md.

## Fußpunkt ↔ Ebenenposition
`+0x15D0/+0x15D4` = Fußpunkt (Szenenpixel), Ebenenposition `+0x10/+0x18` (double),
Zoom `+0x28` (double, Prozent). Versatz `d = (+0x16F4 · ftol(zoom)) / 100`,
`(+0x16F8 · ftol(zoom)) / 100` (int-Division), gespeichert in `+0x16FC/+0x1700`.
- `CActor_UpdateFoot` (`0x00434720`): `Fuß = ftol(Ebene) + d`.
- `CActor_FootToLayer` (`0x004347D0`): `Ebene = Fuß − d`.
`ftol` = `__ftol` (Abschneiden Richtung 0).

## `CActor_SetPosition(x, y, force)` (`0x00434AE0`)
1. Mit Wegkarte (`+0x15C8`) und `Zpos` (`+0x16D8 ≠ 0`): `x` auf 0…639, `y` auf 0…359
   begrenzt, `v = Wegkarte[y][x]`.
   - `v ≠ 0`: `+0x20 = v`; Zoom `+0x28 = Faktor · (+0x16E8)` mit
     `Faktor = Zpos / v`, wenn `sMin` der Karte (`+0x114`) gleich `0.0` ist
     (Konstante `0x0046B560`), sonst `+0x16E0 / ((v − vMin) · k + sMin)` (zbf.md; x87 double);
     danach `0x00447060` (Zoom übernehmen).
   - `v == 0` und `force == 0` → **Abbruch, Figur bewegt sich nicht**; mit `force`
     alter Zoom.
2. Position aus den **unbegrenzten** `x, y`: `CActor_FootToLayer`, `CLayer_SetPos`
   (`0x00446E70`), `CActor_UpdateFoot`.
3. Ortsabhängige Tönung (nur wenn `+0x16D4`): Grundfarbe = Ebenenfarbe `R +0x44`,
   `G +0x4C`, `B +0x48` (alle 0 → 0). Für jede Zone i = 0…9 mit Radius `r ≠ 0`
   (`+0x16AC[i]`): `ax = |Fuß.x − X[i]|`, `ay = |Fuß.y − Y[i]|`; nur wenn `ax ≤ r` und
   `ay ≤ r` (**Quadrat**, kein Kreis):
   `w = (r−ay)·100/r + (r−ax)·100/r − 100`, `w < 0 → 0`;
   `R += RAdd[i]·w/100`, `G += GAdd[i]·w/100`, `B += BAdd[i]·w/100` (int-Division,
   Rundung Richtung 0). Weicht das Ergebnis von der Bildfarbe ab (`img+0x340/+0x348/+0x344`),
   `img->SetColor(R, G, B, 0)` (vtable `+0x28`, tcc.md: LUT-Kumulation).

## Richtung wählen `CActor_FaceTowards(x0, y0, x1, y1)` (`0x00434850`)
`dx = x1−x0`, `dy = y1−y0`; `a = |dy|·90 / (|dy|+|dx|)` (bei 0/0 Nenner 1).
- `a < 23` oder `a > 67`: nur eine Achse – `|dy| < |dx|` → waagerecht (`dy := 0`),
  sonst senkrecht (`dx := 0`).
- Code: `dx > 0` → 1, `dx < 0` → 2; `dy > 0` → `|4`, `dy < 0` → `|8`.
- Code ≠ 0: **Laufmodus** `+0x1AD0` = 2 (senkrecht, `dx = 0`), 1 (waagerecht,
  `dy = 0`) oder **3 (diagonal)**; Richtung `+0x15CC = +0x1A90[Code]`.
  (Damit ist „Laufmodus 3“ aus szn.md geklärt.)

## Figurentypen `CActor_SetType(typ)` (`0x00433E30`)
Tabellen je Bildnummer n der Figuren-Animation (96 Einträge, Disassembly `0x00434144`):
- `StepX[n]` `+0x1720`, `StepY[n]` `+0x18A0`: Schritt je Animationsbild in Pixeln bei
  100 % Zoom. Erst alle 96 mit `+0x1704` (X) bzw. `+0x1708` (Y) gefüllt, dann Bilder
  36…47 (`+0x17B0`/`+0x1930`) und 84…95 (`+0x1870`/`+0x19F0`) einzeln überschrieben
  (nur Typ 1 und „sonst“).
- `+0x170C`, `+0x1710`: gesetzt, Leser nicht gefunden (UNVERIFIED: unbenutzt?).
- `+0x171C` = **Kopfhöhe in Zeilen**: Beim Sprechen (`+0x1ADC`, Kopf-Ebene `+0x1AF4`)
  löscht `CActor_Draw` (`0x00435AE0`) die obersten `+0x171C` Zeilen des Körperbilds
  (`CImage_ClearRows` `0x004387B0`: `pitch · n` Bytes auf 0) und zeichnet danach den Kopf.
- `+0x1A60[dir]` = Standbild je Richtung (`CActor_Turn`), `+0x1A20/+0x1A40[dir]` = erstes/
  letztes Laufbild je Richtung.

Verwendung in `CActor_Update` (`0x004354DA`ff.): Bildindex
`n = first[dir] + frame + 1`, bei `0 ≤ frame ≤ 13` und `frame+1 > last−first` → `n −= 12`
(Umlauf); `sx = StepX[n] · zoom · 0.01`, `sy = StepY[n] · (PerCor ≠ 100 ? PerCor · 0.01 : 1)
· zoom · 0.01` (Konstante `0x0046B580` = 0.01), Rundung siehe szn.md.

| Typ | Fuß `+0x16F4/+0x16F8` | StepX/StepY Standard | `+0x170C/+0x1710` | Kopfhöhe | Standbilder `+0x1A60[0..7]` |
|---|---|---|---|---|---|
| 1 | 108 / 318 | 15 / 5 | 12 / 12 | 100 | – |
| sonst | 55 / 162 | 9 / 3 | 7 / 3 | 50 | – |
| 10 | 55 / 150 | 9 / 3 | 5 / 5 | – | 22 34 46 52 70 82 94 4 |
| 11 | 78 / 198 | 13 / 4 | 8 / 8 | – | 13 25 37 49 61 73 85 1 |
| 12 | 42 / 124 | 9 / 3 | 5 / 5 | – | 13 25 37 49 61 73 85 1 (+ `Talk.a16`-Ebene) |

Typ 1 Bilder 36…47 X: 18 16 18 10 8 20 20 22 22 14 6 20 (84…95 gleich; „sonst“ jeweils
etwa halbiert: 9 8 9 5 4 10 10 11 11 7 3 10); Y-Überschreibungen: 0 0 1 1 0 0 0 0 1 2 0 0
bzw. 0 0 1 1 0 0 0 0 1 2 0 0.
UNVERIFIED: Typ 1 = Rodrigo (volle Größe), „sonst“ = halbe Größe – Zuordnung per Testlauf.
