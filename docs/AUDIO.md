# Ton: Puffer-Eigenschaften, Blenden, Animations-Cues

Belege: Disassembly/Dekompilate `0x00436B20`, `0x004372E0`–`0x004374F0`,
`0x00442BB0`–`0x00442D30`, Registrierung `0x00436592`. Lade-/Abspielfunktionen und
Lippensync: formats/media.md, formats/ger.md.

## Puffer-Eigenschaften (`CAudioBuffer`)
| Funktion | Wirkung |
|---|---|
| `CAudioBuffer_SetVolume(v)` `0x004372E0` | `v` auf 0…100, `+0x10C = v`, Blende aus (`+0x128 = 0`), anwenden `0x00437350` |
| `CAudioBuffer_FadeVolume(v, ms)` `0x00437310` | Ziel `+0x124 = v`, Schritt `+0x128 = (v − aktuell) / (ms / 100)` (int); **`ms < 100` → Division durch 0** (Absturz im Original; Port: Prüfung mit Fehlermeldung, siehe unten) |
| `CAudioBuffer_SetFreq(p)` `0x00437430` | `p` auf 1…200 % , `+0x114 = p`, Frequenz `= Basis(+0x118) · p / 100` (`0x004374C0`) |
| `CAudioBuffer_FadeFreq(p, ms)` `0x00437480` | Ziel `+0x11C`, Schritt `+0x120` wie oben |
| `CAudioBuffer_SetPos3D(x, y, z)` `0x004374F0` | Werte × **7.0** (`0x0046B744`) → `0x00437530` (DirectSound-3D-Position) |

Alle Fälle im Spielcode mit Blendzeit verwenden Werte ≥ 100 ms (Loc40: 1500, 8000,
16000 …) – die Division durch 0 tritt praktisch nicht auf (UNVERIFIED: alle Aufrufer prüfen).

## Blend-Takt `CAudio_FadeTick` (`0x00436B20`)
Beim Anlegen des Audio-Managers: `CTimer` (TIMING.md) mit **100 ms**, Rückruf
`0x00436B20` – läuft im Multimedia-Timer-Thread, unabhängig von der Bildrate.
Für jeden der 100 registrierten Puffer (`CAudio+0x84[100]`, Eintragen `0x00436AC0`,
Austragen `0x00436AF0`):
1. Frequenz-Schritt `≠ 0`: `p += Schritt`; Ziel über-/unterschritten → `p = Ziel`,
   Schritt 0; `SetFreq(p)`.
2. Lautstärke-Schritt genauso, dann anwenden.
Port: gehört zur Timer-Entscheidung (CLAUDE.md) – im Hauptthread alle 100 ms fällig,
im Modus `timer_thread = original` im eigenen Thread.

## Animations-Cues (`CAnimLayer+0x498`, 100 × 0x2C Bytes)
| Offset | Feld |
|---|---|
| `+0x00` | Typ: 0 frei, 1 einfach, 2 erweitert |
| `+0x04` | Bildnummer |
| `+0x08` | Puffer (`CAudioBuffer*`) |
| `+0x0C` | Aktion: 1 abspielen (vtable `+0x08`), 2 vtable `+0x10`, 3 vtable `+0x14` (UNVERIFIED: Stop/Schleife) |
| `+0x10/+0x14` | Lautstärke, Blendzeit ms (0 = sofort) |
| `+0x18/+0x1C` | Frequenz %, Blendzeit ms |
| `+0x20/+0x24/+0x28` | 3D-Position (float, alle 0.0 = nicht setzen) |

- `CAnimLayer_AddCue(bild, puffer)` `0x00442BB0` (Typ 1, Aktion 1),
  `CAnimLayer_AddCueAction(bild, puffer, aktion)` `0x00442C10`,
  `CAnimLayer_AddCueEx(bild, puffer, aktion, vol, volMs, freq, freqMs, x, y, z)`
  `0x00442C70`. Erster freier Platz; **mehr als 100 Cues werden still verworfen**.
- `CAnimLayer_ClearCues` `0x00442D10`.
- `CAnimLayer_RunCues(bild)` `0x00442D30`, aufgerufen von `CAnimLayer_NextFrame`
  `0x00442730` und `0x00442950`: alle Cues mit dieser Bildnummer in Listenreihenfolge;
  Typ 2 setzt zuerst Lautstärke, Frequenz, 3D-Position, dann die Aktion.
  `NextFrame` zählt die Bildnummer `+0x2C0` immer um genau 1 weiter (bzw. zurück) –
  bei niedriger Bildrate läuft die Animation langsamer, Cues gehen nicht verloren.
  Ausnahme Lippensync-Modus (`+0x484`, ger.md): Bild = Tabelle[Audio-Position], dort
  können Bildnummern übersprungen werden.

Beispiel Loc40 (Intro, Szenen 114–116): `S114_IntroPart1` mit 78 Ton-Cues (Meer mit
Blenden über 1,5 s/16 s/8 s, Möwen, Vögel, Hacken, Kanonen, Explosionen).
