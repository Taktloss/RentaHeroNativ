# Indeo 5: Transparenzmaske

Ermittelt aus Intels Codec `ir50_32.dll` (733 696 Bytes, SHA-256 `e473843f364a447d…`, aus dem
Wine-Prefix; Ghidra-Projekt `re/private/iv5/ghidra`, Projekt `Indeo5`). Adressen sind VAs der
DLL (ImageBase `0x10000000`). Ausgangsbefund und Messung: VIDEO.md.

## Datenfluss im Codec
| Adresse | Rolle (Name aus der Fehlertabelle der DLL) |
|---|---|
| `0x10040B50` | `VFW_Decompress`: Dekodieren (`0x10034300`), dann Farbausgabe (`0x1003C4C0`) |
| `0x10034300` | Bild dekodieren (`0x1003D5A0`), Ausgabestruktur füllen |
| `0x1003D5A0` | Bänder aller drei Ebenen, danach **Maske** (`0x1004F840`) |
| `0x1004F840` | Maske: Kopf, Kacheln |
| `0x1004F3C0` | Maske einer Kachel (Lauflängen) |
| `0x1004F340` / `0x100016F0` | Huffman-Tabelle aus Deskriptor bauen |
| `0x1003C4C0` | `ColorOutFrame`: Umwandlung je Ausgabeformat |
| `0x10064BA0` | YUV 4:1:0 → RGB24 (Format 0xB/0xC) mit Maske |

Dekoderzustand (`ctx+0xD0`): Masken `+0x130`/`+0x138` (Puffer `+0x134`), Stride `+0x144` =
`((Breite + 31) >> 3) & ~3` Bytes, Merker „Maske vorhanden“ `+0x13C`, Füllfarbe aus dem
Bitstrom `+0x150`, Transparenz-Bit des Bildkopfs `+0x128 & 1`.

## Farbausgabe
- Die Umwandler bekommen beide Maskenzeiger, Stride, Füllfarbe (24 Bit) und Flags
  (`ctx+0x2E & 8` → Bit 0; Bit 1 = einmalig vorher löschen). Zwei Masken werden zeilenweise
  UND-verknüpft (`0x10064BA0`).
- Füllfarbe: Einstellung des Codecs (`ctx+0x80`); nur wenn sie `0xFF000000` („nicht
  gesetzt“) ist, gilt die Farbe aus dem Bitstrom (`0x10034300`). Gemessen: Schwarz.
  UNVERIFIED: Vorgabewert der Einstellung und Inhalt von `+0x150` in den Spieldaten.
- Maskenbits: 1 Bit pro Pixel, je 32-Bit-Wort MSB zuerst (Byte 0, Bit 7 = erstes Pixel).
  **0 = transparent, 1 = sichtbar** (Vergleich mit `tools/iv5ref`, pixelgenau).

## Lage im Bild (belegt, Prototyp stimmt pixelgenau)
LE-Bitleser über das ganze Bild:
1. 5 Bit Startcode `0x1F`, 3 Bit Bildtyp (0 = Schlüsselbild, ≤ 4), 8 Bit Nummer.
2. Nur Bildtyp 0: GOP-Kopf (`0x100232D0`). 8 Bit GOP-Flags (`ctx+0xBD`): `&1` → 16 Bit
   Kopflänge in Bytes (gezählt ab Bit 16; wird vom Codec überlesen, stimmt aber: der
   Bildkopf beginnt bei `16 + 8·Länge`), `&0x20` → 32 Bit Schlüssel („geschützt“), `&0x40` →
   2 Bit Kachelgröße `64 << v` (sonst eine Kachel = Bild), 3 Bit Ebenenaufteilung `c`
   (`c ∈ {0, 1, 2, 6}`; Luma-Bänder `(c & 3)·3 + 1`, je Chroma-Ebene `(c >> 2)·3 + 1`),
   4 Bit Bildgröße (15 → 26 Bit: Breite = obere 13, Höhe = untere 13; sonst Tabellen
   `0x10087C90` = 640, 320, 160, 704, 352, 352, 176, 240, 640, 704, 80, 88 und `0x10087CD0` =
   480, 240, 120, 480, 240, 288, 144, 180, 240, 240, 60, 72), dann je Luma-Band und je
   Chroma-Band (U und V gemeinsam) 6 Bit; sind Bit 3–5 nicht alle 0, folgen 2 Bit, die 0
   sein müssen (sonst `INT3`, `0x10023CE2`). Dann bei `&8` Transparenz: 4 Bit, untere 3 Bit
   müssen 0 sein, Bit 3 → 24 Bit Füllfarbe (R, G, B → `ctx+0x150…0x152`). Auf Byte
   ausrichten, 24 Bit Version (`ctx+0xBF = (v >> 19) & 0xF`; Bit 23 → 16-Bit-Worte, solange
   deren Bit 15 gesetzt ist), auf Byte ausrichten.
3. Bildkopf: 8 Bit Flags (`ctx+0xBE`): `&1` → 24 Bit Bildgröße in Bytes (= Chunk-Länge),
   `&0x10` → 16 Bit Prüfsumme, `&0x20` → Erweiterungen (8 Bit Länge + Daten, bis Länge 0),
   `&0x40` → 3 Bit Makroblock-Tabelle (7 → 4 Bit Anzahl + je 4 Bit), dann 3 Bit. Danach auf
   Byte ausrichten.
4. Bänder: Luma-Bänder, dann U, dann V. Je Band 8 Bit Flags; `&1` = leer (1 Byte), sonst
   bei Bild-Flags `&0x80` 24 Bit **Bandlänge in Bytes ab Bandbeginn** (`0x1001E020`).
   Geschützte Videos verschlüsseln Teile des ersten Bandkopfs (`0x1001E020`, nur zur
   Kenntnis); die Längen stehen davor und werden nur zum Überspringen gebraucht.
5. Dahinter die Maske.

## Bitstrom der Maske (`0x1004F840`)
Liegt direkt hinter den Banddaten des Bildes. LE-Bitleser (LSB zuerst), Masken aus der Tabelle
`0x1008C550` (`(1 << n) − 1`).
1. Bild-Flags `&1`: 24 Bit Maskenlänge (gemessen: Rest des Bildes − 5…8).
2. 3 Bit `n` = Anzahl Rechtecke; je Rechteck vier 16-Bit-Werte (x, y, Breite, Höhe des
   sichtbaren Bereichs, z. B. (2, 1, 235, 318) bei 240×328). Nur Begrenzungsrahmen
   (`ctx+0x30`), für die Maske nicht nötig.
3. 1 Bit `delta` (an `0x1004F3C0` als „XOR mit Vorzeile“ weitergereicht).
4. 1 Bit `eigenes Codebuch`: dann 4 Bit Zeilenanzahl `k`, `k` × 4 Bit `xbits` → Deskriptor
   `ctx+0x1D110` (Anzahl, Werte). Sonst bleibt der vorige Deskriptor.
5. Auf Byte ausrichten; dann **Kacheln** (Kachelgröße/-anzahl aus `ctx+0xD0`/`+0xD8`). Je
   Kachel: Bit „gefüllt“ → nächstes Bit ist der Füllwert; sonst Bit „Größe“ → 8 Bit Größe
   (`0xFF` → 24 Bit), Daten ab dem nächsten Byte; ohne Größe Daten ab Byte 1 der Kachel.
   Gefüllt: **Bit 1** ist der Füllwert, die nächste Kachel beginnt zwei Bit weiter.
   `ctx+0xEC[Kachel]` = Kachel liegt im angeforderten Ausschnitt (`0x100232D0`); bei voller
   Dekodierung immer 1 (der Zweig für 0 überspringt nur). Belegt für eine Kachel.

## Lauflängen einer Kachel (`0x1004F3C0`)
- Leere Kachel: ganz mit `DAT_100980F0[wert]` füllen (Wert = Bit aus dem Kopf).
- Breite je Zeile = Kachelbreite auf 32 aufgerundet; gezählt werden auch die Füllspalten.
- Sonst: Startwert aus dem ersten Bit; dann wiederholt ein Huffman-Symbol `s` lesen
  (16-Bit-Schritte, LSB zuerst; Nachschlagetabelle `tab[2]`, Codelänge `tab+0x2010+s`):
  `s` Pixel mit dem aktuellen Wert schreiben, danach Wert wechseln. `s == 0` bedeutet
  **255 Pixel ohne Wechsel**.
- Ab der zweiten Zeile mit `delta`: jedes Wort XOR mit dem Wort der Zeile darüber.
- Ergebnis wird in beide Maskenpuffer geschrieben.

## Huffman-Codes (`0x100016F0`)
Deskriptor `k, xbits[1..k]` (je ≤ 8). Zeile `i` (ab 1): Präfix aus `i − 1` Einsen und einer
Null (letzte Zeile ohne Null), dann `xbits[i]` Bits in umgekehrter Reihenfolge
(Tabellen `0x100903F0`). Symbole fortlaufend ab 0 über alle Zeilen (höchstens 256).
UNVERIFIED: Standard-Deskriptor, falls ein Bild keinen eigenen mitbringt.

## Prüfung
`tools/iv5ref/iv5mask.py` (Prototyp mit vollständigem GOP-Kopf; die Kopflänge aus `&1` wird
gegen die Parser-Position geprüft und stimmt) gegen den Original-Codec (`tools/iv5ref`):
**alle 21 transparenten Videos** der Spieldaten, je bis 30 Bilder – jedes Maskenbit 0 ist im
Original schwarz, Maske 1 deckt sich mit den nicht-schwarzen Pixeln. Alle 21: GOP `0x89`,
eine Kachel, Bitstrom-Füllfarbe (0, 0, 0), Kachelarten „gefüllt“ und „Lauflängen ohne Größe“.

## Offen
- Nicht in den Spieldaten und daher nur beschrieben, nicht geprüft: mehrere Kacheln
  (Folgeposition nach Kacheln ohne Größe im Dekompilat mehrdeutig), Schutz-Bit, Bilder ohne
  Bild-Flag `0x80`.
- Standard-Codebuch, falls das erste Bild keines mitbringt; Füllfarbe aus dem Bitstrom
  (`0xFF000000`-Regel) gegen Messung.
