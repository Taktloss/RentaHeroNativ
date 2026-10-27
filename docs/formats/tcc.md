# TCC und A16 – Bildformat mit Alpha und Animation

Belege: `CImageTcc_Load` `0x0043B630`, `CImageTcc_Decode` `0x0043B740`, RLE `0x004380B0`,
4-Bit-Expansion `0x00438800`, Farbtabellen `0x004385D0`, `CImageA16_Load` `0x0043BB60`,
`_Next` `0x0043BD20`, `_Rewind` `0x0043BCE0`, `_Seek` `0x0043BDC0`, Alpha-Bounds `0x004386E0`,
`IsTransparent` `0x00438680`. Gegengeprüft mit `tools/rahimg.py` an allen 102 TCC- und
219 A16-Dateien (Größen stimmen exakt; Testbilder dekodieren korrekt).

## Bild (TCC = genau ein Bild, A16 = Folge von Bildern)

Little-Endian. Kopf 28 Bytes (7 × u32), direkt in `CImage+0x04…+0x1C` kopiert:

| Off | Feld | Bedeutung |
|---:|---|---|
| 0 | `flags` | Bit 0: Ebenen 4-Bit (expandieren); Bit 4 (0x10): Deltabild; A16: erstes Bild immer 0x20, Folgebilder 0x10 oder 0x20; TCC: 0 |
| 4 | `size0` | Bytes RLE-Daten Ebene 0 (Rot) |
| 8 | `size1` | Ebene 1 (Grün) |
| 12 | `size2` | Ebene 2 (Blau) |
| 16 | `size3` | Alpha-Ebene; 0 = kein Alpha |
| 20 | `width` | |
| 24 | `height` | |

Danach folgen die vier RLE-Blöcke direkt hintereinander. Jede Ebene ist `width × height`
Bytes, zeilenweise, oben links beginnend. Kanalreihenfolge R, G, B: am Bild geprüft
und am Ausgabecode belegt (RENDERER.md, `0x0043ADC0`).

`size0 == 0` (TCC): das Original liest dann den Kopf ein zweites Mal aus dem Datenpuffer
(`0x0043B740`). Kommt in den Daten nicht vor.

## RLE (`0x004380B0`)
Pakete, bis `size` Bytes verbraucht sind; nur das untere Bit-Paar des ersten Bytes zählt:

| `op & 1` | `op & 2` | Paket | Wirkung |
|---|---|---|---|
| 1 | – | `op n v` (3 B) | `n` × Wert `v` schreiben |
| 0 | 1 | `op n` (2 B) | `n` Zielbytes überspringen (Wert bleibt) |
| 0 | 0 | `op n d[n]` (2+n B) | `n` Bytes kopieren |

Abbruch nur bei **genau** 0 Restbytes (Vergleich `== 0`); fehlerhafte Daten laufen im
Original über. Port: Grenzen prüfen, Ergebnis für gültige Daten identisch.

## 4-Bit-Ebenen (`flags & 1`, `0x00438800`)
Nach dem RLE in-place rückwärts: für `i = n−2, n−4, … > 0`:
`dst[i] = src[i/2] & 0xF0; dst[i+1] = src[i/2] << 4`. **Das Paar (0, 1) wird nie
expandiert** (Schleife `> 0`) – Original-Bug, nachbilden. In den Daten derzeit unbenutzt.

## Nachbearbeitung
- Farbtabellen (`0x004385D0`, vtable-Slot 12): je Ebene eine 256-Byte-LUT
  (`+0x34` R, `+0x134` G, `+0x234` B), nur wenn der jeweilige Wert ≠ 0 (`+0x340` R,
  `+0x348` G, `+0x344` B). Aufbau `CImageTcc_SetColor(r, g, b, neu)` (`0x004384F0`):
  `lut[i] = clamp(i + wert, 0, 255)`; mit `neu` wird das aktuelle Bild sofort neu
  dekodiert. Quelle: SZN `RedAdd/GreenAdd/BlueAdd` über `CLayer_SetColor` `0x004473E0`
  (Felder `+0x44`, `+0x4C`, `+0x48`); in den Daten über 70× je Kanal ≠ 0.
- Die LUT wird **in-place** auf die Ebenen angewandt, die bei A16 zugleich der Zustand
  für Deltabilder sind. Am Code belegt: übersprungene Pixel eines Deltabilds (und beim
  Neu-Dekodieren) bekommen die LUT erneut → Tönung **kumuliert** über die Bilder bis zur
  Sättigung. Ob/wie stark das im Spiel sichtbar ist, bleibt im Testlauf zu prüfen; der
  Port bildet es exakt nach.
- Alpha-Bounds (`0x004386E0`, Slot 9, nur bei `+0x37C`): kleinstes Rechteck mit Alpha ≠ 0
  → `+0x35C`; ohne Alpha ganzes Bild.
- Transparenz für Treffer-Tests (`0x00438680`): mit Alpha `alpha < 0x80`; ohne Alpha
  Pixel (0, 0, 0).

## A16-Animation
- Datei = Folge von Bildern wie oben ohne Zwischenraum, max. **300** Bilder (Rest ignoriert).
- Ladefunktion liest die ganze Datei, legt Tabelle `+0x3AC[300]` (Bild-Offsets) und
  `+0x85C[300]` (u16, 1 = Schlüsselbild = Bit 4 *nicht* gesetzt) an, dekodiert Bild 0.
- Deltabilder überschreiben die persistenten Ebenen; übersprungene Pixel bleiben.
  Ein Alpha-Puffer wird nur angelegt, wenn das erste Bild Alpha hat.
- `Next` (`0x0043BD20`): nach dem Laden liefert der erste Aufruf nur `1` (Bild 0 bleibt);
  am Dateiende `0`. Zähler `+0xAB8`.
- `Seek(n)` (`0x0043BDC0`): `n < 1` → zurück auf Bild 0; `n > 299` → Fehler; sonst
  vom letzten Schlüsselbild ≤ n (oder vom aktuellen Bild, wenn näher) vorwärts dekodieren.
- Statistik: max. 141 Bilder je Datei, Breite/Höhe innerhalb einer Datei konstant.

## Daten-Besonderheit
`GAME/LOC06/S26_RING.TCC` ist nur 32 Bytes groß, ungültig und wird von keiner SZN
referenziert.
