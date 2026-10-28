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
| 12 | 42 / 124 | 9 / 3 | 5 / 5 | – | 13 25 37 49 61 73 85 1 |

Typ 1 Bilder 36…47 X: 18 16 18 10 8 20 20 22 22 14 6 20 (84…95 gleich; „sonst“ jeweils
etwa halbiert: 9 8 9 5 4 10 10 11 11 7 3 10); Y-Überschreibungen: 0 0 1 1 0 0 0 0 1 2 0 0
bzw. 0 0 1 1 0 0 0 0 1 2 0 0.
Typ 1 = `RodrigoLarge.a16` (volle Größe), „sonst“ (in den Daten 2) = `RodrigoSmall.a16`
– per Testlauf bestätigt (LOC04/006: `RodrigoLarge` Typ 1, `RamWalk` Typ 11).

**Sprechkopf (für alle Typen, nach dem `switch`):** Ist `+0x1AF4` leer, wird
`<Figurenpfad ohne die letzten 4 Zeichen>Talk.a16` als `CAnimLayer` geladen (Farbschlüssel 0,
`0x00446C90`, Name aus dem Dateinamen), z. B. `RodrigoLargeTalk.a16`. Schlägt das Laden fehl,
wird das Objekt gelöscht, **der Zeiger bleibt aber stehen** (Original-Fehler; Port: kein Kopf).
Im Konstruktor ist der Pfad noch leer; `+0x1AF4` und `+0x171C` werden dort erst **nach**
`SetType(0)` auf 0 gesetzt (Disassembly `0x00433AD8`/`0x00433AFF`).

## Konstruktor `0x004338A0` (Auszug)
`+0x1A8 = +0x1AC = 1` (läuft, Schleife), Z `+0x20 = 0.0`, Richtungstabelle `+0x1A90[Code]`
= `0 6 2 0 0 7 1 0 4 5 3` (Code 1 rechts, 2 links, 4 unten, 8 oben → Richtung 0 unten,
1 unten-links, 2 links, 3 oben-links, 4 oben, 5 oben-rechts, 6 rechts, 7 unten-rechts),
erste/letzte Laufbilder `+0x1A20` = 12 24 36 48 60 72 84 0, `+0x1A40` = 23 35 47 59 71 83 95 11,
Standbilder `+0x1A60` = 98 99 100 101 102 103 104 97, Schrittgeräusch-Bilder `+0x1A80` = 10
(links), `+0x1A84` = 4 (rechts), `+0x1A8C = −1`, `+0x1AD4 = −1`, `+0x16EC = 100`,
`+0x1AE8 = 1`, `srand(time(NULL))` (MSVC-LCG, `+0x14` der Thread-Daten).

## vtable `0x0046B538`
| Slot | Funktion | Bedeutung |
|---|---|---|
| 0 | `0x00433B50` | Destruktor (`0x00433C40`: Slot 1, Kopf löschen, `CAnimLayer`-Dtor) |
| 1 | `0x00433CB0` | Wegsucher, drei Schrittgeräusche, Ereignis `+0x16F0` freigeben |
| 2/3 | `0x00442500`/`0x004425B0` | Laden wie `CAnimLayer` |
| 4 | `0x00433D40` | Editor-Info `%c %s XY … Z … Zoom` |
| 5 | `0x00434A90` | **SetPos = `CActor_SetPosition(x, y, 0)`** (Fußpunkt!) |
| 6 | `0x00435340` | `CActor_Update` |
| 7/8 | `0x004349D0`/`0x00434A30` | Editor: X bzw. Y verschieben, neu setzen |
| 9 | `0x00435D10` | statt `Stop`: Laufziel `+0x1AE0/+0x1AE4` löschen |

`CActor_SetPos(x, y, richtung)` `0x00434AB0` hat **drei** Parameter (`RET 0xC`): Slot 5, dann
`0x00435B80(richtung)` (Richtung sofort, Standbild). `0x00435BA0(richtung)`: wie `Turn`
ohne Ereignis.

## Laufen
`CActor_WalkTo/WalkTo2(x, y, ereignis, endrichtung)` `0x00434D70/0x00434D90`: mit Ereignis
`+0x1AE8 = 0`; `CActor_SetWalk` `0x00434DC0`: Ereignis `+0x1C8` anlegen (Handler, aktiv),
Endrichtung `≠ −1` → `+0x1AD4`; dann `CActor_WalkCore(x, y, ereignis ≠ 0)` `0x00434EA0`:
1. `CLayer_Enable` (**erst das Laufen macht eine Figur sichtbar**; der SZN-Lader aktiviert
   sie nicht). Ohne Wegkarte → 0.
2. Fuß schon am Ziel: Endrichtung gesetzt und anders → `Turn` (mit dem Handler des
   Ereignisses), sonst Ereignis 0x29; `ClearRun`.
3. Ohne neues Ereignis und mit altem: Aktion lief noch (`+0x1AE8 == 0`) → altes Ereignis mit
   **0x2A** markieren und nach `+0x16F0` verschieben, sonst löschen.
4. `Zpos == 0` → Ziel direkt (ohne Wegsuche, **ohne `FaceTowards`**). Sonst Wegsuche
   (PATHFINDING.md), ersten Wegpunkt holen, `FaceTowards`; Richtungswechsel → Drehen.

Ereignis-Markierung überall: `+4 = 1`, Typ = Param = 0x29 (fertig) bzw. 0x2A (abgebrochen),
Sofort-Flag → `CEvent_Fire`; danach `+0x1AE8 = 1`. `CSzene_DispatchEvents` löst je Figur nur
`+0x1C8` aus, sonst `+0x16F0` – die Ebenen-Ereignisse der Figur nicht.

## `CActor_Update` `0x00435340` (vtable Slot 6)
1. Spricht (`+0x1ADC`): Kopf-Update, dirty, 1.
2. Dreht (`+0x1AD8`): angezeigte Richtung `+0x1A8C` einen Schritt in kürzerer Richtung
   (`<` : `dir − shown < 5` → +1; sonst `shown − dir > 3` → +1; sonst −1; Umlauf 0…7), Bild
   `first + frame` (frame auf 0…`last−first−1`). Fertig → bei leerem Ziel „angekommen“.
3. Laufen: Fuß == Ziel ≠ (0, 0) → nächster Wegpunkt (`FaceTowards`) bzw. „angekommen“.
   Schritt (Disassembly `0x004354DA`–`0x00435779`): Bildindex `n` wie oben, `sx, sy` aus
   `StepX/StepY` (szn.md), `ix = ftol(sx + 0.5)`, `iy = ftol(sy + 0.5)`. Modus 3 rechnet wie 2
   (beide Zweige ergeben dieselben Werte). **Modus 1** (Hauptachse x): `vx = ±ix` (Vorzeichen
   von dx), `|dx| < |ftol(vx)|` → `vx = dx`; `k = vx ≠ 0 ? dx / vx : dx`;
   `vy = ±(|dy| / k + 0.5)`, `|dy| < |ftol(vy)|` → `vy = dy`. **Sonst** gespiegelt mit y als
   Hauptachse. `CActor_SetPosition(Fuß + ftol(vx), Fuß + ftol(vy), 1)`. Neue Richtung →
   Laufbild `first[dir] + frame`.
   Angekommen: Endrichtung −1 oder erreicht → Standbild, `frame = shown = −1`, Ereignis 0x29,
   `ClearRun`; sonst Drehen zur Endrichtung.
4. Dreht, läuft oder angekommen: Schrittgeräusch-Position (`0x00447CB0`: Fuß-x 0…639 → −1…1,
   Z 0…255 → 0.1…0.7), `CAnimLayer_Update` (0 → Rückgabe 0); Bild == `+0x1A80` → links,
   == `+0x1A84` → rechts bzw. bei `rand() % 10 == 5` alternativ; `SetPos3D`, `Play`.
5. Nicht drehend und `frame ≥ last − first` → `frame = −1`, Bild `first[dir]`.
6. Begleiter `+0x1AF8` (`0x00435F70`): bei Ankunft oder neuem Wegpunkt zum Punkt 20 Pixel
   hinter der Figur (je Richtung, **vorzeichenlose** Grenzen 1…639/1…359), sonst zur Figur.

**Original-Verhalten:** Bei `Zpos == 0` bleibt der Laufmodus 0 (senkrecht). Ein rein
waagerechter Weg ergibt `vy = 0` → `k = dy = 0` → `|dx| / 0 = ∞` → `__ftol` = `0x80000000`:
die Figur springt ins Nirgendwo. Port bildet das nach (Test `actor_walk_without_zpos_…`).

## Sprechen (CSpeech, Sprecher = Held)
`0x00435E00(lippendaten, rate, audio)` (`0x00435DF0` = ohne): Drehen aus, `+0x1ADC = 1`,
Standbild; Kopf übernimmt Z, X, Y, fps, Zoom und die **Bildfarbe** des Körpers; mit Daten und
Audio `CAnimLayer_SetLipSync(…, Offset = first[dir])`, sonst Bereich `first…last[dir]` in
Schleife. `CActor_Draw`: löscht die obersten `+0x171C` Zeilen der **Alpha-Ebene** des
Körperbilds (`CImage_ClearRows` `0x004387B0` schreibt `Breite · n` Bytes ab `+0x3A0`),
zeichnet Körper und Kopf. Ende `0x00435F10`: Kopf `Stop(0)`, ausblenden, fps zurück, Bild 0,
dann Standbild, dirty.

## Szene
- Laden (`0x0044A770`): neu → `CActor` laden, sonst `0x00433B70` (Werte zurücksetzen, Bild
  `Stand[0]`, **Datei wird nicht neu geladen**); Wegkarte; Slot 5 `(0, Y-Versatz)`;
  `+0x5D8 = Figur`, `+0x2B8 = 40`; Bereich „Rodrigo“ (`+0x5E0`) bzw. „Girlfriend“ (`+0x5E4`).
  Eine vorhandene Figur wird nur bei anderem `Type` ersetzt; der Bereich der alten bleibt dann
  mit hängendem Masken-Zeiger stehen (Port: Maske gelöst).
- Update: wie Ebenen (Stufe nach fps), `+0x58` (Run) erzwingt dirty und Update.
- Klick: ohne Inventarleiste (`+0x5BC == 0`) zuerst `0x0044CB60` (zuletzt angelegte Figur
  `+0x5E4` bzw. Held); sonst Klick ins Leere oder auf den Heldenbereich → Held aktiv →
  `CActor_WalkToClick(x, y − 60)`.
- Hilfe: Held fehlt oder Fuß-x > 319 → Text links, sonst rechts.

## Port
`engine/actor` (`Actor : AnimLayer`), `engine/pathfinder`; Szene `actors_` (Laden, Update,
Ereignisse, Compositor Schritte 4/8/10/11, Klick), Sprache (Figur-Variante), `Game::hero_`.
Szenen-Betrachter: `--walk x y` lässt den Held im zweiten Bild laufen (Testhilfe).
