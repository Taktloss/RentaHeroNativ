# ZBF – Tiefen- und Wegkarte

Belege: `CZMap_Load` `0x0043D710`, `CZMap_CalcScale` `0x0043D6C0`, Klasse `CZMap`
(0x140 Bytes, Ctor `0x0043D620`, vtable `0x0046B8C8`). Geladen von `CSzene_LoadDepth`
`0x0044A1B0` (SZN-Schlüssel `zBuf`, → `CSzene+0x2BC`) und `CSzene_LoadPath`
`0x0044A260` (`wBuf`, → `CSzene+0x2C0`). Gegengeprüft an allen 119 Dateien.

## Aufbau
Optionaler Kopf, danach **640 × 360 Bytes** (230 400 = `0x38400`), ein Byte pro Pixel,
zeilenweise ab oben links.

Ist die Datei genau 230 400 Bytes groß, gibt es keinen Kopf; die vier Kopfwerte
werden 0, `CalcScale` wird nicht aufgerufen. (Kommt in den Daten nicht vor.)

Sonst Kopf (Little-Endian):

| Off | Typ | Feld | Ziel |
|---:|---|---|---|
| 0 | u32 | Kopfgröße `hs`, muss ≥ 20 sein (sonst Fehler) | |
| 4 | f32 | Skalenwert bei `vMin` | `+0x110` |
| 8 | i32 | `vMin` | `+0x118` |
| 12 | f32 | Skalenwert bei `vMax` | `+0x114` |
| 16 | i32 | `vMax` | `+0x11C` |
| 20 | – | `hs − 20` weitere Bytes: werden gelesen und gleich danach von den Kartendaten überschrieben, also **ignoriert** | |

Daten: 50+50 Dateien mit `hs = 48` (Zusatzbytes ≠ 0, Bedeutung egal), 9+10 mit `hs = 20`.
Typische Werte: `10.0, 5, 24.0, 250`. Tiefenkarten enthalten Werte 5…255,
Wegkarten 0…~245.

Fehler beim Lesen der 230 400 Bytes (auch zu kurze Datei) → Karte verworfen, Szene
ohne Karte.

## Abgeleitete Werte (`0x0043D6C0`)
```
dv    = vMax - vMin                    (+0x120, int)
ds    = sMax - sMin                    (+0x124, float)
k     = (double)(ds / (float)dv)       (+0x128)  Skala pro Kartenwert
k_inv = (double)((float)dv / ds)       (+0x130)
```
Also lineare Abbildung Kartenwert → Skala. Belegte Verwendung: Der SZN-Lader berechnet
für Figuren `CActor+0x16E0 = (Zpos − vMin) · k + sMin` **mit den Werten der Wegkarte**
(`CSzene+0x2C0`, szn.md). Weitere Nutzung (Skala beim Laufen) mit der Figurenlogik.
Float-Genauigkeit: Division in `float`, Ergebnis als `double` gespeichert – im Port
genauso rechnen.

## Semantik der Werte
- Tiefenkarte: Vergleich mit Ebenen-/Figuren-Z zum Verdecken – noch zu belegen.
- Wegkarte: begehbare Fläche für die Figur (0 = nicht begehbar?) – noch zu belegen
  (`FUN_00434e80`, EVENTS.md).
