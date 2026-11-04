# Wine-Messungen (DirectDraw, GDI)

Messprogramme unter `tools/wineprobe/` (Win32, i686, ohne CRT, keine Originaldaten). Sie
bilden die Aufrufe des Originals nach und geben die Ergebnisse als Text aus.

```sh
tools/wineprobe/build.sh <ordner>
WINEPREFIX=$PWD/wineprefix WINEDEBUG=-all wine <ordner>/ddprobe.exe
```

**Grenze:** Gemessen wird Wines `ddraw`/`gdi32`, nicht Windows 98 mit einem Treiber von 1999.
Die Werte belegen also das Verhalten unter Wine. Für Windows sind sie ein Hinweis, kein Beweis.
Wo das Ergebnis ohnehin nicht sichtbar wird, steht das dabei.

## Oberflächen wie im Original

Ebenenoberflächen entstehen in `0x00446D10…0x00446D7E`: `DDSURFACEDESC` mit `dwFlags = 7`
(Caps, Höhe, Breite, **ohne** Pixelformat, also das des Bildschirmmodus, im Spiel 16 Bit).
Zuerst wird `OFFSCREENPLAIN | VIDEOMEMORY` (0x4040) versucht, dann
`OFFSCREENPLAIN | SYSTEMMEMORY` (0x840). Das Messprogramm gibt RGB565 explizit vor
(`DDSD_PIXELFORMAT`), damit der Desktop nicht umgeschaltet wird.

## Ergebnisse (2026-11-04, Wine des Systems, `wineprefix/`)

| Messung | Ergebnis | Folge für den Port |
|---|---|---|
| Pitch (System- und Videospeicher gleich) | auf 8 Bytes aufgerundet: Breite 1…4 → 8, 5 → 16, 9 → 24, 321 → 648, 640 → 1280 | Das Original rechnet mit `lPitch` aus `Lock` (`0x004399E0`: `DDSURFACEDESC+0x10`). Der Pitch bleibt also intern, Breite·2 im Port ist gleichwertig (`render/blit.hpp`). |
| Inhalt neuer Oberflächen | 4096/4096 Pixel 0, auch bei sofortiger Wiederverwendung nach dem Füllen | AVI-Oberflächen werden sofort ganz überschrieben → unsichtbar (`animlayer.cpp`) |
| `Lock(rect)` | außerhalb (auch um 1 Pixel) und umgedreht → `DDERR_INVALIDPARAMS` (0x80070057); leeres Rechteck (10,10,10,10) erlaubt | `tccsprite.cpp`: umgedrehte Rechtecke zusätzlich abgelehnt |
| `BltFast` ohne Clipper | Ziel teilweise außerhalb, Quelle außerhalb, leere Quelle → `DDERR_INVALIDRECT` (0x88760096), **nichts** gezeichnet; vollständig innen → 0 | `layer.cpp`: leere Quelle jetzt ebenfalls Fehlschlag (dirty bleibt gesetzt) |
| `Blt` gestreckt | nächster Nachbar, Quellindex `(x · ((s << 16) / d)) >> 16` (8→12: 0 0 1 1 2 3 3 4 5 5 6 7) | im Spiel unerreichbar: `ZoomVal ≠ 100` nur bei TCC mit Alpha (eigener Blitter, alle 15 geprüft) und A16; BMP/AVI immer 100 |
| GDI-`BitBlt` 24 → 16 Bit (`DDCopyBitmap`-Muster `0x0043D130`: `GetDC`, `CreateCompatibleDC`, `BitBlt SRCCOPY`) | 1024/1024 Rampenwerte abgeschnitten (`R & 0xF8`, `G & 0xFC`, `B >> 3`), einfarbige Fläche (0x84,0x42,0x21) überall 0x8204 → kein Dithering | wie `loadBmp565`/`pack565` (`formats/bmp.hpp`) |

## Schrift (`fontprobe.exe` + `rah_fontcmp`)

`fontprobe.exe` ruft `CreateFontA` wie das Original auf: „Arial“, `DEFAULT_CHARSET`, Höhe 15
fett-kursiv (`0x00436230`), 15 normal (`0x00440F70`), 12 normal (`0x0043E4C0`). Das Original
übergibt `DEFAULT_QUALITY`, die Messung `NONANTIALIASED_QUALITY` (Projektentscheidung: ohne
Kantenglättung). Gemessen werden 13 Texte in CP1252: Metriken, Breite per
`GetTextExtentPoint32A`, die Pixel bei `TA_LEFT|TA_BASELINE` (Datei `font.out`) und der
Versatz bei `TA_CENTER`. `rah_fontcmp font.out [ttf]` zeichnet dieselben Texte mit `rah_font`
und vergleicht sie Pixel für Pixel.

Wine nimmt für „Arial“ seine eigene `share/wine/fonts/arial.ttf`. Die gibt es nur als Regular,
Fett/Kursiv **erzeugt Wine künstlich**. Der Fall fett-kursiv ist daher nicht vergleichbar: Unter
Windows gäbe es `arialbi.ttf`, der Port nimmt die echte BoldItalic-Datei.

| Messung | Wine-GDI | Port |
|---|---|---|
| `tmHeight`/`tmAscent` (15 bzw. 12) | 15/12 bzw. 12/10 (`tmInternalLeading` 2 bzw. 1) | gleich (gerundet; abgerundet wäre 12/9) |
| Glyphen mit **derselben** TTF (`rah_fontcmp font.out …/wine/fonts/arial.ttf`) | – | 26 Regular-Texte **pixelgleich** (Pixel-em, Hinting, Grundlinie, Vorschub beim Zeichnen) |
| `TA_CENTER` | Ursprung `x − floor(Breite/2)` in allen 39 Fällen (ungerade Breiten: 51 → 25, 9 → 4) | gleich |
| Breite `GetTextExtentPoint32A` | weicht von Wines **eigenem** `TextOut`-Vorschub ab (z. B. „WWWW“: Ausdehnung 48, gezeichnet 4 × 13) | Summe der gezeichneten Vorschübe (52) |

Die letzte Zeile ist eine Eigenheit von Wine: Bei `TA_CENTER` rechnet Wine mit der kleineren
Ausdehnung, zentrierter Text liegt dort also bis zu 2 px weiter rechts als im Port. Unter
Windows wird erwartet, dass Ausdehnung und Zeichnen dieselben Glyphbreiten verwenden. Das ist
hier nicht messbar (UNVERIFIED). Der Port bleibt bei der gezeichneten Breite.
Mit Liberation Sans statt Wines `arial.ttf` unterscheiden sich die Glyphen leicht
(andere Datei, anderes Hinting). Das ist eine Frage der Schriftdatei, nicht der Darstellungsregeln.
