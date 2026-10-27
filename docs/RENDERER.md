# Renderer: Bildschirmmodus, Pixelformat, Blitter, Schrift

Belege jeweils mit Adresse. Ausgabeziel ist eine gesperrte DirectDraw-Oberfläche
(`Lock` vtable `+0x64`, `Unlock` `+0x80`); die Bildklassen schreiben die Pixel selbst.

## Bildschirmmodus (`0x0043DCDC`–`0x0043DD62`)
Immer 640×480. Mit `TrueColor` (rah.ini `[MODES]`, Standard **0**): erst 24 Bit, dann
32 Bit, sonst 16 Bit (und `TrueColor` wird gelöscht). Ohne `TrueColor`: **16 Bit**.
Auf heutiger Hardware/Wine ist 16 Bit = **RGB565** (Maske `0xF800`); RGB555 ist im Code
ebenfalls vorhanden. **Referenz für den Port: RGB565**, danach verlustfrei auf 32 Bit
erweitert. Die Szene liegt bei y = 60…419 (EVENTS.md).

## Pixelformat-Auswahl (`0x0043ADC0` deckend, `0x0043AE90` mit Alpha)
Nach `dwRGBBitCount` (`DDSURFACEDESC+0x54`) und `dwRBitMask` (`+0x58`):
16 Bit 565 / 16 Bit 555 / 24 Bit / 32 Bit, jeweils mit R oben (`0xFF0000`) oder
vertauschten Ebenen. Die TCC-Ebenen werden bei `RMask = 0xFF0000` als (R, G, B)
übergeben → **Ebene 0 = Rot** ist damit am Code belegt (tcc.md).

Bei Alpha wird ein Code gebildet und per `switch` verteilt:
`code = bpp + 10000·(Maske 565 bzw. R oben) + 1000·(ZoomVal ≠ 100) + 100·(Tiefentest)`
(Tiefentest nur bei Tiefenkarte und Ebenen-Z ≥ 4).

| Variante | 565 | 555 | 24 | 32 |
|---|---|---|---|---|
| deckend (ohne Alpha) | `0x004399E0` | `0x0043A390` | `0x00439190` | `0x004388D0` |
| Alpha | `0x00439A80` | `0x0043A430` | `0x00439210` | `0x00438990` |
| Alpha + Zoom | `0x00439C30` | `0x0043A600` | `0x00439350` | `0x00438B10` |
| Alpha + Tiefe | `0x00439EC0` | `0x0043A8C0` | `0x004395A0` | `0x00438D50` |
| Alpha + Zoom + Tiefe | `0x0043A0B0` | `0x0043AAD0` | `0x00439740` | `0x00438F00` |

## Formeln (RGB565, Referenz)
Deckend (`0x004399E0`): `p = (R & 0xF8) << 8 | (G & 0xFC) << 3 | B >> 3` (abschneiden,
kein Runden/Dithering).

Alpha `a` (`0x00439A80`):
- `a = 255` → wie deckend; `a = 0` → Pixel bleibt.
- sonst Ziel entpacken: `dR = (p >> 8) & 0xF8`, `dG = (p & 0x7E0) >> 3`, `dB = (p & 0x1F) << 3`;
  je Kanal `c = (d + (((s − d) · a) >> 8)) & 0xFF` (Rechnung in 32 Bit, nur das untere Byte
  zählt – entspricht `floor`), dann wie deckend packen.
  Division durch 256 statt 255: auch `a = 254` lässt den Hintergrund leicht durch.

Tiefentest (`0x00439EC0`): Tiefenkarte (`CZMap+0x108`, Zeilenlänge 640) ab der
Zielposition der Ebene; Pixel nur gezeichnet, wenn **`depth[x, y] > Z`** (strikt,
Z = Ebenen-/Figurentiefe). Sonst wie Alpha.

Zoom (`0x00439C30`): Nächster Nachbar mit 7-Bit-Festkomma:
`stepX = (srcW << 7) / dstW`, `stepY = (srcH << 7) / dstH` (unsigned, abgerundet);
Quellspalte `(i · stepX) >> 7`, Quellzeile `(j · stepY) >> 7`; Start um die
Clipping-Ränder (`+0x34C` links, `+0x350` oben, je `· step >> 7`) verschoben. Danach
Alpha wie oben.

Genauer (`0x00439C30`, `0x0043A0B0`):
- Zielrechteck gesamt `+0x24…+0x30` (für die Schrittweite), sichtbarer Ausschnitt
  `*(RECT*)+0x20` (für Schleifengröße und Zielposition).
- Quellursprung `o = ((clipL · stepX) >> 7) + ((clipT · stepY) >> 7) · srcW`
  (`clipL/T` = `+0x34C/+0x350`). Spaltentabelle `col[i] = (i · stepX) >> 7`,
  Zeile `row[j] = (j · stepY) >> 7`, Quellpixel `o + row[j] · srcW + col[i]`.
  **Eigenheit:** Ursprung und Zeile werden getrennt abgerundet
  (`(clipT·s>>7) + (j·s>>7)` ≠ `((clipT+j)·s>>7)`) – bei geclippten, skalierten
  Ebenen kann das Bild um eine Quellzeile/-spalte versetzt sein. Nachbilden.
- Zoom + Tiefe (`0x0043A0B0`): wie Zoom, zusätzlich pro **Zielpixel** Test
  `depth[top + j][left + i] > Z` – Tiefenzeiger `CZMap+0x108 + rect.top·640 + rect.left`
  über das **sichtbare Zielrechteck** `*(RECT*)(this+0x20)`, genau wie bei Alpha + Tiefe
  (`0x00439EC0`). UNVERIFIED: ob dieses Rechteck in Bildschirm- (y ab 60) oder
  Szenenkoordinaten liegt (Ziel = eigene Szenenoberfläche?); Port: Parameter `sceneTop`.
- Spaltentabelle wird pro Aufruf angelegt und freigegeben (nur Speicher, kein Effekt).

Ausschnitt (ohne Zoom): Bild-Offsets `+0x34C…+0x358` (Clipping links/oben/rechts/unten)
bzw. bei `DirtyAlpha` das Alpha-Rechteck `+0x35C…+0x368`; Breite `+0x380`, Höhe `+0x384`.

## Zeichnen eines Bilds (`CImageTcc_Draw` `0x0043B900`)
1. Rechteck (Standard: ganzes Bild) clippen (`FUN_00438040`), nach `+0x24` merken.
2. Liegt das Rechteck nicht auf dem unteren Streifen (Bildzeilen 360–420) und ist es
   unsichtbar (`FUN_004481e0`), nichts tun.
3. `param_4 == 0` → Zielrechteck vorher mit Farbe 0 füllen (`Blt` `DDBLT_COLORFILL`).
4. Oberfläche sperren, Blitter nach Tabelle, entsperren.

## Schrift (GDI)
| Verwendung | Funktion | `CreateFontA` |
|---|---|---|
| Bereichsname unter dem Mauszeiger | `0x00436230` | Arial, Höhe 15, **fett (700), kursiv**, `DEFAULT_CHARSET`; schwarzer Rand durch 4 diagonale Versätze (±1, ±1), dann weiß |
| Hilfetexte | `0x00440F70` | Arial, Höhe 15, normal |
| Renderer (Untertitel/Debug) | `0x0043E4C0` | Arial, Höhe 12, normal |

Ersatz bei Fehler: `GetStockObject(SYSTEM_FONT)`. Positive Höhe = Zellenhöhe.

**Für den Port:** Pixelgenaue Schrift erfordert Arial und GDI-Rasterung (ohne Glättung
im 16-Bit-Modus). Arial darf nicht mitgeliefert werden. Vorschlag: Arial des Systems
nutzen, falls vorhanden, sonst metrisch kompatibles Liberation Sans (SIL OFL);
Rasterung ohne Antialiasing. Das ist eine bewusste Abweichung → mit dem Nutzer klären.
