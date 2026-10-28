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
  UNVERIFIED: Polarität (1 = sichtbar?) – per Vergleich mit `tools/iv5ref` prüfen.

## Bitstrom der Maske (`0x1004F840`)
Liegt direkt hinter den Banddaten des Bildes. LE-Bitleser (LSB zuerst), Masken aus der Tabelle
`0x1008C550` (`(1 << n) − 1`).
1. Ist `ctx+0xBE & 1`: 24 Bit überspringen.
2. 3 Bit `n` = Anzahl Rechtecke; je Rechteck vier 16-Bit-Werte (a, b, c, d). Ergebnis nur als
   Begrenzungsrahmen in `ctx+0x30` (Bedeutung im Detail offen, für die Maske nicht nötig).
3. 1 Bit `delta` (an `0x1004F3C0` als „XOR mit Vorzeile“ weitergereicht).
4. 1 Bit `eigenes Codebuch`: dann 4 Bit Zeilenanzahl `k`, `k` × 4 Bit `xbits` → Deskriptor
   `ctx+0x1D110` (Anzahl, Werte). Sonst bleibt der vorige Deskriptor.
5. Auf Byte ausrichten; dann **Kacheln** (Kachelgröße/-anzahl aus `ctx+0xD0`/`+0xD8`) mit je
   einem Kachelkopf (Bits „leer“, „Größe vorhanden“; Größe 8 Bit, bei `0xFF` 24 Bit) und
   den Lauflängen (`0x1004F3C0`).
   UNVERIFIED: genaue Reihenfolge und Wirkung der Kachelkopf-Bits (Dekompilat unklar).

## Lauflängen einer Kachel (`0x1004F3C0`)
- Leere Kachel: ganz mit `DAT_100980F0[wert]` füllen (Wert = Bit aus dem Kopf).
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

## Offen
- Position des Maskenbeginns: `0x1003D5A0` liest je Ebene/Band Kopf (`0x1001E020`) und Daten
  (`0x100249D0`) und rückt um die jeweils verbrauchte Länge vor; die Maske folgt danach.
  Zu klären, ob Band-/Kachelgrößen im Kopf ein Überspringen ohne Koeffizienten-Dekodierung
  erlauben.
- Kachelkopf, Standard-Codebuch, Polarität, Füllfarbe aus dem Bitstrom.
