# Wegsuche (`CPathFinder`, 0xE8 Bytes)

Belege: Dekompilate und Disassembly `0x00444C60`–`0x00445C50`, Aufrufer
`CActor_SetPathMap` `0x00434680`, `CActor_WalkCore` `0x00434EA0`.
Für exaktes Verhalten (Reihenfolge bei gleichen Kosten!) muss der Port die
Datenstrukturen **wörtlich** nachbilden (sortierte Liste mit linearem Einfügen,
memmove-Verschiebungen), nicht durch eine Bibliotheks-Prioritätswarteschlange ersetzen.

## Raster
Ein **globales** Byte-Raster `0x0047E64C` mit 160 × 90 Zellen (Zelle = 4 × 4 Pixel der
Szene 640 × 360), Index `(x/4) * 90 + (y/4)` (spaltenweise; `/4` mit C-Rundung zu 0).
| Bit | Bedeutung |
|---|---|
| `0x80` | gesperrt durch die Wegkarte (ein Pixel des 4×4-Blocks = 0) oder Randzelle |
| `0x40` | dynamische Sperre aus dem Spielcode (`CActor_BlockRect`/`UnblockRect`) |
| `0x01` | besucht (nur während der Suche, danach gelöscht) |

- Aufbau `CPathFinder_ctor(zmap)` (`0x00444C60`) beim Setzen der Wegkarte
  (`CActor_SetPathMap` `0x00434680`, aus `CSzene_LoadPath` und weiteren): Raster
  löschen, Zelle frei nur, wenn **alle 16 Pixel ≠ 0** (`0x004457C0`); erste/letzte Zeile
  und Spalte gesperrt. Das Raster ist global: Die zuletzt aufgebaute Figur bestimmt es.
- `CActor_BlockRect(x1, y1, x2, y2)` (`0x00435D90` → `0x00445060`): Zellen
  `x1/4…x2/4 × y1/4…y2/4` (einschließlich) `|= 0x40`; `UnblockRect` (`0x00435DC0` →
  `0x004450D0`) löscht das Bit. Pixelkoordinaten der Szene, nur wenn die Figur einen
  Wegsucher hat. 31 bzw. 9 Aufrufe im Spielcode (Hindernisse wie Personen/Türen).
- Frei-Test `PathCellFree(x, y)` (`0x00445020`): ganzes Byte `== 0`.

## Laufbefehl → Zielpunkt
In `CActor_WalkCore` (SCRIPTING.md), wenn Wegkarte und Wegsucher vorhanden:
1. Ziel einrasten (Disassembly `0x00435057`ff.): solange nicht frei `y++` bis 355
   (sonst `y` zurück), dann solange nicht frei `y--` bis 5 (sonst zurück), dann `x++` bis
   635, dann `x--` bis 5. (Erst nach unten, dann nach oben, dann rechts, dann links.)
2. Pixel der **Wegkarte** am Ziel = 0 → sofort Ereignis 0x29, kein Laufen.
3. `CPathFinder_Find(start, ziel)` (`0x00444E20`); läuft die Figur schon
   (`+0x1AE0/+0x1AE4` ≠ 0) und schlägt die Suche fehl, zweiter Versuch vom bisherigen
   Zwischenziel aus. Kein Weg → `0x00434AE0(x, y, 1)` und Ereignis 0x29
   (UNVERIFIED: `0x00434AE0` noch nicht analysiert, vermutlich direkt setzen).
4. Wegpunkte mit `CPathFinder_Next` (`0x00444FE0`) abholen (der erste ist der Start
   und wird verworfen); Richtung zum nächsten Wegpunkt mit `0x00434850`.

## `CPathFinder_Find(sx, sy, tx, ty)` (`0x00444E20`)
- Wegpunkte löschen, Pixel speichern (`+0x10…+0x1C`), Zellen = Pixel/4, Besucht-Bits löschen.
- Start- bzw. Zielzelle gesperrt → `PathNearestFree` (`0x004456B0`): erste freie Zelle
  unter den 8 Nachbarn in der Reihenfolge `(0,−1) (1,−1) (1,0) (1,1) (0,1) (−1,1)
  (−2,0) (−1,−1)` – **der westliche Nachbar ist (−2, 0)** (Original-Fehler, am
  Disassembly `0x00445703` belegt); Nachbar muss innerhalb `1…Breite−2 / 1…Höhe−2`
  liegen. Keine → Fehlschlag.
- Start- und Zielzelle gleich → Wegpunkte `[Start, Ziel]`, Erfolg.
- sonst A* **vom Ziel zum Start** (`0x00445140(ziel, start)`), danach Wegpunkte bauen.

## A* (`0x00445140`)
Knoten (12 Bytes, Pool `+0xC4` mit 72 000 Plätzen, Zähler `+0xD4` ab **1**):
`u8 x, u8 y, u16 parent, i32 g, i32 h`. Indexliste `+0xC8` (72 000 × u16): offene Liste
aufsteigend sortiert in `[+0xD8, +0xDC)`, geschlossene Liste als Stapel von `+0xE0`
abwärts bis `+0xE4`.
- `h = __ftol(sqrt(dx² + dy²) * 1000.0)` in Zellen (`0x004454F0`, x87, Abschneiden).
- Nachbarn je Knoten in der Reihenfolge `(0,−1) (1,−1) (1,0) (1,1) (0,1) (−1,1) (−1,0)
  (−1,−1)`, Kosten 1000 gerade / 1414 diagonal; Nachbar nur wenn `Raster & 0xC0 == 0`
  (keine Prüfung auf Diagonal-Ecken).
- Einfügen in die offene Liste (`0x00445A80`): lineare Suche nach dem ersten Eintrag
  mit `f_neu ≤ f`, davor einfügen → **bei gleichem f kommt der neueste Knoten zuerst**.
- Schleife: ersten Knoten der offenen Liste entnehmen (`0x004459B0`); ist es die
  Startzelle → gefunden. Für jeden Nachbarn: `g' = g + Kosten`, Suche in offener
  (`0x00445B70`) und geschlossener Liste (`0x00445BE0`).
  - nicht besucht → neuer Knoten (`0x004458C0`), besucht setzen, einfügen.
  - in offener Liste und `g' < g(alt)` → Knoten **an Ort und Stelle** aktualisieren
    (`0x00445970`), **ohne Neusortierung** (Original-Verhalten).
  - nur in geschlossener Liste: Vergleich erfolgt fälschlich mit Knoten 0, der nie
    beschrieben wird. Die Debug-CRT füllt `malloc`-Speicher mit `0xCD` →
    `g(0) = 0xCDCDCDCD < 0` → Bedingung nie wahr, geschlossene Knoten werden nie
    wieder geöffnet. Port: diesen Zweig als „nie“ nachbilden (UNVERIFIED: Laufzeitprüfung
    des Speicherinhalts).
  - danach Knoten in die geschlossene Liste (`0x00445B40`).
- Ende, wenn die offene Liste leer ist. `+0x0C` = zuletzt entnommener Knoten.
  Pool voll (72 000) → `0x004458C0` liefert 0 (bei 14 400 Zellen praktisch unerreichbar).
- Besucht-Bits löschen; Rückgabe gefunden ja/nein.

## Wegpunkte (`0x00445540`)
Liste `+0x20` mit 20 Paaren `(x, y)` in Pixeln, Leseindex `+0xC0`, Ende = `(0, 0)`.
- Punkt 0 = Startpixel. Dann der Elternkette (Richtung Ziel) folgen: solange vom
  letzten Anker aus Sichtlinie zur Zellmitte (`Zelle*4+2`) besteht, weitergehen; bei
  Blockade den letzten sichtbaren Knoten als Wegpunkt eintragen und neu ankern; am Ende
  der Kette das genaue Zielpixel eintragen.
- Sichtlinie `0x00445820` (in Zellen): `n = max(|dx|, |dy|)`, Festkomma 10 Bit,
  Schrittweite `(d * 1024) / n` (Division mit Abschneiden), prüft die Zellen 0 … n−1
  (Endzelle nicht) auf Rasterbyte `≠ 0`.
- **Keine Obergrenze**: mehr als 19 Wegpunkte überschreiben `+0xC0` und folgende Felder
  (Original-Fehler; Port: gleiches Speicherlayout nachbilden oder optional begrenzen).
