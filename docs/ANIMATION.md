# Animierte Ebenen (`CAnimLayer`)

`CAnimLayer : CLayer` (0x15C8 Bytes, Ctor `0x004422E0`, vtable `0x0046B8E0`). Angelegt von
`CSzene_CreateLayer` (`0x0044AB10`), wenn in der SZN `fps ≠ 0`. In den Daten: 246 A16- und
58 AVI-Ebenen, dazu 2 AVI-Hintergründe; Bildfolgen aus BMP/TCC (Art < 2, Dateimuster mit
`%0*d`) kommen nicht vor. Port: `src/engine/animlayer.*`, Tests `tests/test_anim.cpp`.
Bildformat und `Next`/`Seek`/`Rewind` des A16-Bilds: `formats/tcc.md`.

## vtable `0x0046B8E0`
| Slot | Adresse | Funktion |
|---|---|---|
| 2 | `0x00442500` | Load (Art bestimmen; Art < 2: Zifferanzahl des Musters zählen) |
| 3 | `0x004425B0` | CreateImage: Art < 2 alle Bilder nach `+0x2EC` (Anzahl `+0x2BC`); Art 2/3 → `frame = −1`. Art 1/2/3 → `alpha (+0x198) = 1` (auch ohne Alpha im Bild). `next = now + period` |
| 5 | `0x00446E70` | SetPos (von CLayer) |
| 6 | `0x00442AA0` | Update |
| 9 | `0x00443260` | Stop/Endmodus |

## Felder
| Offset | Bedeutung | Ctor |
|---|---|---|
| `+0x1A8` | läuft | 0 |
| `+0x1AC` | Schleife | 0 |
| `+0x1B0` | Endmodus: 1 normal, 2 = am Ende auf Bereichsanfang zurück | – |
| `+0x1B4` | am Ende ausblenden (Modus 0/10) | – |
| `+0x1B8` | beendet (Start setzt dann auf Bild 0 zurück) | – |
| `+0x1C4/+0x1CC/+0x1D0` | `CEvent*` Ende (Typ 1) / Bild (Typ 3) / Schrittzähler (Typ 4) | – |
| `+0x2B0` | Periode ms | 100 |
| `+0x2C0` | aktuelles Bild | – |
| `+0x2CC` | Bild für Ereignis `+0x1CC` | – |
| `+0x2D0/+0x2D4` | Schrittzähler / Ziel für `+0x1D0` | – |
| `+0x2D8/+0x2DC` | Bereich von/bis | −1/−1 |
| `+0x2E0` | rückwärts | 0 |
| `+0x2E4` | Ende ausstehend | 0 |
| `+0x2E8` | nächster Bildzeitpunkt (timeGetTime) | – |
| `+0x484`…`+0x494` | Lippensync: Daten, Rate, Index, Bildoffset, Audio (formats/ger.md) | – |
| `+0x498` | 100 Ton-Cues à 0x2C Bytes (AUDIO.md) | leer |

## Update (`0x00442AA0`, vtable +0x18)
1. `CLayer_Update` (Interpolation `+0x50`).
2. Ende ausstehend → löschen, `beendet = 1`, Ende-Ereignis mit Typ 1 **markieren**
   (Auslösen erst in `CSzene_DispatchEvents`, Sofort-Flag ausgenommen), fertig.
3. Nur wenn aktiv und läuft: `NextFrame`; `period = 1000 / fps` (fps ≠ 0);
   `next += period`; ist `next < now` → **Nachholschritt** (`0x00442950`) und
   `next = now + period`. Danach `dirty = 1`.

Wann `Update` aufgerufen wird (Fälligkeit `next ≤ now`, 200-ms-Fenster), steht in TIMING.md.

### Nachholschritt `0x00442950` – Eigenheit
Ruft Bild-Slot 5 auf; bei A16 ist das `0x004387F0` = `return 1` → **kein Bildwechsel**.
Trotzdem werden das Bild-Ereignis (falls `bild == +0x2CC`) und die Cues des aktuellen
Bilds **erneut** ausgelöst. Ein verspäteter Takt spielt Cue-Töne also doppelt. Nachgebildet.

## NextFrame (`0x00442730`)
- vorwärts: `bild + 1`, `Next` des Bilds; liefert es 0 (Dateiende) → `OnEnd`.
- rückwärts: `bild − 1`; bei 0 → `OnEnd` (Bild 0 wird rückwärts nie gezeigt), sonst
  `Seek(bild)` + `Next`.
- Schrittzähler + 1; erreicht er das Ziel (≠ 0) → Typ 4 markieren.
- `bild == +0x2CC` → Bild-Ereignis Typ 3; `RunCues(bild)` (alle 100 Plätze, freie
  übersprungen); `bild == bis` → `OnEnd`.

## OnEnd (`0x004429D0`)
- Schleife oder Endmodus 2: zurück auf den Bereichsanfang (`von == −1`: Rewind, Bild 0;
  sonst `Seek(von)` + `Next`, Bild = von).
- keine Schleife: Bereich = −1, läuft = 0, rückwärts = 0, Ende ausstehend = 1.

## Start (`0x004430F0`, Modus)
Setzt Schleife 0, aktiv 1, läuft 1, Endmodus 1, dirty 1, Ende ausstehend 0. Modus über
Sprungtabelle `0x00443214` = `[0,0,5,5,5,5,5,5,5,5,1,2,3,4]` (Modus 0…13):
| Modus | Wirkung |
|---|---|
| 0, 10 | am Ende ausblenden |
| 1, 11 | stehen bleiben (`+0x1B4 = 0`) |
| 12 | Endmodus 2 |
| 13 | Schleife |
| 2…9, > 13 | `+0x1B4` unverändert |
War die Ebene beendet: A16 `Seek(0)`, Bild −1 (Art < 2: Bild 0). `beendet` wird immer gelöscht.

## Interpolation (`CLayer`, gilt für alle Ebenen)
Belegt an `0x004475B0`, `0x004475F0`, `CLayer_Update` `0x00447890`, `CLayer_SetColor`
`0x004473E0`, `CLayer_UpdateZoomRect` `0x00447060` (FPU-Folgen mit `re/scripts/FpuOps.java`).

**Kein-Ziel-Wert ist 0.0** (Konstante `0x0046B568`), nicht −1: Ein Ziel von 0 lässt den
Wert unverändert – Bewegung nach x = 0 ist so nicht möglich.

Start `0x004475F0(x, y, z, zoom: double; r, g, b: int; schritte, handler)`
(`0x004475B0` = `CLayer_MoveTo` ruft es mit r = g = b = 0; 26 Aufrufer):
- `schritte == 0` → nichts. Sonst `+0x50 = schritte`, Ziele `+0x228` X, `+0x230` Y,
  `+0x238` Z, `+0x240` Zoom, `+0x248` R, `+0x250` B, `+0x258` G (Farben als double).
- `handler ≠ 0` → Ereignis `+0x1D4` anlegen/Handler setzen, aktiviert. Bei `handler == 0`
  bleibt ein vorhandenes Ereignis samt Handler unverändert.
- Schrittweiten `(Ziel − aktuell) / schritte` (double), bei Ziel 0 → 0:
  `+0x278` X, `+0x280` Y, `+0x288` Z, `+0x290` Zoom; Farbe: Startwerte `+0x260` R,
  `+0x268` G, `+0x270` B aus der Ebenenfarbe (`+0x44` R, `+0x4C` G, `+0x48` B),
  Schritte `+0x298` R, `+0x2A0` B, `+0x2A8` G.

`CLayer_Update` (Slot 6; `CAnimLayer_Update` ruft es zuerst):
- `+0x50 == 0` → Rückgabe 1, sonst nichts (auch kein dirty).
- `--(+0x50) == 0` (letzter Schritt): Ziele ≠ 0 direkt übernehmen (Z ohne `+0x38`;
  CSzene_Update erkennt die Z-Änderung über ftol). Ist **irgendein** Farbziel ≠ 0:
  `SetColor(ftol R-Ziel, ftol G-Ziel, ftol B-Ziel)` – **Eigenheit:** Kanäle mit Ziel 0
  werden dabei auf 0 gesetzt, obwohl sie während der Schritte unverändert blieben.
  Danach Ereignis `+0x1D4` mit Typ 2 markieren (wie Typ 1, Sofort-Flag → sofort).
- sonst: X/Y/Z/Zoom += Schrittweite; bei einem Farbziel ≠ 0 Laufwerte += Schritt und
  `SetColor(ftol …)`.
- Zoom-Ziel ≠ 0 → `0x00447060` (Zoom-Rechteck + Klickbereich), sonst nur Klickbereich
  (`CLayer_GetRect` der Maske, Raster markieren). Immer `dirty = 1`.

`CLayer_SetColor(r, g, b)`: `+0x44 = r`, `+0x4C = g`, `+0x48 = b`, dann Bild-Slot 11:
TCC/A16 `0x004384D0` → `CImageTcc_SetColor(r, g, b, 1)`: LUTs neu und **aktuelles Bild
erneut dekodieren** (`0x0043B740`, vom Lesezeiger; bei Größen 0 – nach Seek – wird der Kopf
dort gelesen und der Zeiger weitergesetzt, das folgende `Next` überspringt dann dieses Bild).
Bei A16-Deltabildern kumuliert die Tönung (formats/tcc.md). BMP/AVI `0x004388A0`: Werte
speichern, Slot 12 ist leer – keine Wirkung.

## Weitere Aufrufe
- `PlayEndEvent(modus, ev)` `0x00443030`: ev = 0 → Ende-Ereignis abschalten; sonst Handler
  setzen und aktivieren; dann Start.
- `PlayRange(modus, ev, von, bis)` `0x00442E70` / ohne Ereignis `0x00442F90`: Bereich
  setzen; `von ≠ −1` → `SeekFrame(von)` (AVI mit `von = 0` ausgenommen), rückwärts =
  `bis ≠ −1 && bis < von`.
- `SeekFrame(n)` `0x00443310`: `n = −1 → 0`; A16: `Seek(n)`, Bild = `n < 1 ? −1 : n − 1`;
  aktiv → dirty; `beendet = 0`.
- `ShowFrame(n)` `0x004433B0`: Bereich −1, `SeekFrame(n)`, `NextFrame`, dirty.
- `SetFrameEvent(ev, bild)` `0x00443400`: ev = 0 → nichts.
- Slot 9 `0x00443260(x)`: x ≠ 0 → Schleife 0, Endmodus = x; x = 0 → Bereich −1, Schleife 0,
  läuft 0, beendet 1, rückwärts 0, Ende-Ereignis Typ 1 markieren.

## Offen
AVI-Ebenen: siehe VIDEO.md (portiert; direkter Weg `+0x30` offen). Lippensync `+0x484`, Bildfolgen Art < 2 (in den Daten unbenutzt),
Bedeutung von `+0x2C4/+0x2C8`.
