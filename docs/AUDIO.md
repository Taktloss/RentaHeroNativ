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

Beispiel Loc40 (Intro): `S114_IntroPart1` mit **45** Ton-Cues (Meer mit Blenden über
1,5 s/16 s/8 s, Möwen, Vögel, Hacken, Kanonen, Explosionen), S115 52, S116 4 (je einer davon
über einen gemeinsamen Sprung `0x004329FC`; ermittelt mit `re/scripts/CallArgsResolved.java`).
Ein Cue mit `vol = 0` ändert die Lautstärke nicht.

## Audio-Manager `CAudio` (`g_Audio` `0x0047BF10`, `CSzene+0x5C4`)
Ctor `0x00436530`: 100 Slots leer, Blend-Takt (`CTimer`, 100 ms periodisch). Init `0x00436670`:
`DirectSoundCreate`, Kooperationsstufe 3 (`DSSCL_EXCLUSIVE`), Fähigkeiten (`DSCAPS`) →
`+0x74 = (dwFreeHw3DAllBuffers ≠ 0)`: **3D nur mit freien Hardware-3D-Puffern**; Primärpuffer
(Flags 0x14 bzw. mit 3D) im Format **22 050 Hz, 16 Bit, Stereo** (`0x00436750`).
Gesamtlautstärken `+0x68` Effekte, `+0x6C` Musik, `+0x70` Sprache (rah.ini `[SOUNDS]`,
Vorgabe 100; `0x004369D0`/`0x00436A20`/`0x00436A70` wenden sie sofort auf alle Puffer der
Kategorie an). Ohne DirectSound (`+0x04 = 0`) scheitert jedes `Open` still.

## Puffer `CAudioBuffer` (0x188 Bytes, vtable `0x0046B71C`)
| vtable | Funktion |
|---|---|
| `+0x04` `0x00436DC0` | `Open(pfad, mgr, kategorie)`: Slot eintragen, WAV ganz in einen **statischen** Puffer (Flags 0xE2 = Lautstärke/Pan/Frequenz/statisch, +8 bei `+0x78`, +0x10 = 3D bei `+0x74`), Grundfrequenz `+0x118` = Abtastrate, Dauer `+0x174`, Lautstärke/Frequenz 100 %, anwenden |
| `+0x08` `0x00437160` | `Play`: läuft (`+0x168`) → erst `Stop`; nicht vorbereitet → auf Anfang; mit Ende-Ereignis (`+0x130`) einmaliger Timer nach Dauer → `0x00437600` (markiert nur); abspielen, `+0x15C = now`, läuft = 1, vorbereitet = 0, Schleife `+0x178 = 0` |
| `+0x0C` `0x00436DB0` | → `+0x10` |
| `+0x10` `0x00437100` | Schleife (`DSBPLAY_LOOPING`), `+0x178 = 1`, kein Ende-Ereignis |
| `+0x14` `0x00437270` | `Stop`: anhalten (Position bleibt), läuft = 0, Ende-Ereignis markieren (Typ/Param 0x1F, Sofort-Flag → auslösen), Ende-Timer weg |
| `+0x18` `0x00437640` | Position in ms (Abspielzeiger; ohne Puffer `now − +0x15C`) |
| `+0x1C` `0x004370D0` | vorbereiten: auf Anfang, `+0x164 = 1` |

- **Lautstärke** `0x00437350`: `vol · 0.01f [· Kategorie · 0.01f] · 3333.0f − 3333.0f`, `_ftol`
  (abschneiden), auf −3333…0 (`0x004373B0`) → `SetVolume` in 1/100 dB: 0…100 % ist also
  **linear in dB** von −33,33 dB bis 0 dB. Kategorie nur für 2/3/4.
- **Frequenz** `0x004374C0`: `Grundfrequenz · % / 100`, auf 100…100 000 Hz.
- `0x00437FD0(vol, ms)` (Musik-Einblenden, `CMusic_FadeIn` = (80, 2000)): nur wenn still –
  Lautstärke 0, Blende, `+0x10`.
- `0x00438010(s)` / `0x00438000` (= 2 s): `+0x12C = 1`, Blende auf 0 über `s · 1000` ms;
  `CAudio_FadeTick` **löscht** den Puffer, sobald Lautstärke und Ziel 0 sind. Einziger Nutzer:
  `CGame_PlayMusic` für die alte Musik. Eigenheit: bei Lautstärke < 20 ist der Schritt 0 –
  die alte Musik blendet nie aus und spielt weiter.
- `0x004375E0(ev)`: `+0x130 = ev` und Ereignis aktivieren (`ev = 0`: nichts).
- `0x00437560`/`0x004375B0`: Eigenschaften über `+0x158` (`IKsPropertySet`, nur mit 3D
  vorhanden; vermutlich EAX-Hall) – ohne Hardware-3D wirkungslos.
- Dtor `0x00436D00`: `Stop` (markiert das Ende-Ereignis!), Timer weg, Slot austragen.

## Stream `CAudioStream` (0x1A4 Bytes, vtable `0x0046B750`; Musik, `CGame_Say`)
Ctor `0x004376C0`: Ring `+0x194 = 2000` ms, Nachladen `+0x198 = 250` ms.
- `Open` `0x004377F0`: Wiederholpause `+0x180 = 30` s; Ring = `nAvgBytesPerSec · 2 s`, höchstens
  Datengröße; Flags 0xE0 (kein statischer Puffer); WAV bleibt offen; Ring füllen (`0x00437DC0`).
- `Play` `0x00437E50`: wie oben, aber Ring in Schleife, `+0x19C = now`, Nachlade-Timer 250 ms
  periodisch → `0x00437B80`.
- Nachladen `0x00437B80`: Rest = Datengröße − Lesestand; `now − +0x19C ≥ Dauer` **und** Rest 0
  → `Stop` (Ende-Ereignis). Sonst Ring bis zum Abspielzeiger mit Daten bzw. Stille füllen.
  Danach: steht er und `+0x178` gilt → Pause 0: sofort `Play`; sonst einmaliger Timer
  `Pause · 1000` ms (`0x00437D70`: `Play`, falls still) und **Pause += 10 bis 300 s** –
  Musik wiederholt sich nach 30, 40, 50 … 300 s Stille.
- `+0x0C` `0x00437E20(s)`: `+0x178 = 1`, Pause = s, still → `Play`. `+0x10` `0x00437E10` = `+0x0C(30)`.
- `Stop` `0x00437F50`: anhalten, läuft = 0, Nachlade-Timer weg, Ende-Ereignis markieren.

## CGame
- `CGame_PlayMusic(name)` `0x004400C0`: läuft schon Musik (`CGame+0x08`) mit gleichem Namen
  (`CSzene+0x1AC`, `lstrcmpi`) → diese. Sonst alte Musik ausblenden/löschen (`0x00438000`),
  neue `CAudioStream` öffnen (Kategorie 4), 3D (0, 0, −0.1), `CSzene+0x1A8/+0x1AC` setzen.
  **Abspielen muss der Aufrufer.** Die Musik gehört der Szene und überlebt Schauplatzwechsel.
- `CGame_LoadSfx(name)` `0x004402F0`: `CAudioBuffer`, Kategorie 2 (ohne Audio-Manager 0);
  `CGame_LoadSfxVol(name, v)` `0x004403A0` zusätzlich `SetVolume(v)`.
- `CGame_PlaySfxRandom3D` `0x00440290`: 3D (±1.0 zufällig, 0, ±1.0), abspielen.

## Port
`audio/mixer`: Software-Mischer statt DirectSound, Ausgabe 22 050 Hz/16 Bit/Stereo wie der
Primärpuffer, Sättigung; Mono ohne Panorama auf beide Kanäle; Lautstärke `10^(dB/2000)`,
Frequenz über lineare Interpolation (UNVERIFIED gegenüber der DirectSound-Software-Mischung).
Streams spielen die Datei am Stück; Ring und Lesestand werden für die Ende-Erkennung
nachgebildet. Alle Audio-Timer (Blend-Takt, Ende, Nachladen, Wiederholen) prüft
`pollTimers` im Hauptthread. **3D und Eigenschaften:** Der Port verhält sich wie ein System
ohne Hardware-3D (heute der Normalfall, auch unter Wine) – Positionen werden nur gespeichert.
Offene Frage an den Nutzer: optionale 3D-Nachbildung (wie mit 3D-Soundkarte 1999)?
Programm: `--mute`, `--music-vol/--sfx-vol/--speech-vol`, `--audio-dump <wav>`.
