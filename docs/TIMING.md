# Timing-Modell

Alle Zeiten in Millisekunden aus `timeGetTime()`. Belege: Dekompilate + Disassembly
(Adressen angegeben). Kein fester Frame-Takt – die Hauptschleife läuft ungebremst.

## Hauptschleife (`WinMain` `0x00401E00`)
```
loop:
  while (!PeekMessage(PM_REMOVE)):
      if (!g_AppActive) WaitMessage();      // Pause bei inaktivem Fenster
      else              Game_Idle();        // 0x004030C0, ein "Frame"
  if (msg == WM_QUIT) break;
  if (!TranslateAccelerator(...)) { TranslateMessage; DispatchMessage; }
```
Kein `Sleep` im Echtzeitmodus → 100 % CPU im Original. Der Port darf hier schlafen
(Vsync/kleines Delay), solange die Zeitlogik unten identisch bleibt.

## Ein Frame: `Game_Idle` (`0x004030C0`)
1. Abbruch, wenn `0x0047D738` oder `0x0047D154` gesetzt (Bedeutung offen).
2. Frame-Zähler `0x0047D754++`.
3. Falls Schauplatz aktiv (`g_CurGame`) oder `0x0047D144`: `CSzene_Update(scene, realtime)`
   (`0x0044B280`) mit `realtime = g_RealTime` außer wenn `0x0047D130` gesetzt.
   `g_RealTime` wird in `Config_ReadModes` fest auf 1 gesetzt (EDITOR.md).
4. Dirty-Rechtecke ermitteln (`FUN_0044b7d0`), Menü-/Leistenobjekt `CSzene+0x5B4`
   (`FUN_00441c30`, nicht bei offenem Spielmenü), dann Renderer-Ausgabe (vtable-Slot 0
   pro Rechteck bzw. Vollbild-Flip `FUN_0043e1d0`).
5. `CSzene_DispatchEvents` (`0x0044C690`) – alle ausstehenden Ereignisse (EVENTS.md).
6. Nicht bei offenem Spielmenü: `CGame_Tick` (`0x00440B80`).
7. Anstehender Schauplatzwechsel (`0x0047D750`): alten `CGame` löschen,
   `Game_SwitchLocation()`; schlägt er fehl, mit vertauschten Zielwerten erneut.

## Animations-Takt: `CSzene_Update` (`0x0044B280`)
Vier Takt-Stufen, gemeinsam getaktet in einem **200-ms-Fenster** ab `CSzene+0x2DC`;
Zähler pro Stufe `CSzene+0x2E0…+0x2EC`.

| Stufe | Ebenen-fps (`layer+0x54`) | Ticks im Fenster bei `dt = now − start` |
|---:|---|---|
| 0 | 1–5 | 0 |
| 1 | 6–10 | 0, 100 |
| 2 | 11–15 | 0, 67, 134 |
| 3 | ≥ 16 | 0, 50, 100, 150 |

Stufe = `min((fps − 1) / 5, 3)` (Ganzzahldivision).

Exakter Ablauf (Disassembly `0x0044B2F0`–`0x0044B385`, `c[i]` = Zähler):
```
dt = now - start
if dt >= 200:
    start += 200; if start < now: start = now      // kein Nachholen
    c[0..3] = 0; tick[0..3] = 1
elif dt >= 150: tick[3] = c[3] < 4
elif dt >= 134: tick[2] = c[2] < 3
elif dt >= 100: if c[1] < 2: tick[1] = tick[3] = 1   // Stufe 3 hängt an c[1]!
elif dt >=  67: tick[2] = c[2] < 2
elif dt >=  50: tick[3] = c[3] < 2
for i: if tick[i]: c[i]++
```
Eigenheiten, die der Port übernehmen muss:
- Pro Aufruf feuert höchstens **eine** Zwischenstufe; verpasste Fenster (langsamer
  Frame) werden nicht nachgeholt → effektive Bildrate sinkt.
- Der 100-ms-Tick von Stufe 3 wird über den Zähler von Stufe 1 gesteuert.
- `dt` ist vorzeichenlos. Beim Fensterwechsel gilt praktisch immer `start = now`
  (nur bei genau `dt == 200` bleibt `start + 200`).
- Die Zwischenstufen-Bedingungen (`c < 2` usw.) setzen voraus, dass der Fensterstart den
  Zähler schon auf 1 gebracht hat. Im **ersten Fenster nach `CSzene_ctor`** (`0x004483A0`,
  Start = `timeGetTime()`, Zähler 0) tickt daher jede Zwischenstufe in zwei Frames
  hintereinander (Stufe 3: 50 und 51 ms, 100 und 101 ms), Stufe 0 gar nicht.
  Belegt per Test `scene_update_stage_ticks`.
- `realtime == 0`: alle Stufen ticken in jedem Frame, dazu `Sleep(0)`.

Danach werden aktualisiert (vtable-Slot 6 `Update`, `+0x18`), jeweils nur bei Tick
ihrer Stufe oder wenn `layer+0x1A0` (dirty) gesetzt ist:
1. Objekt `CSzene+0x5DC`, sonst `CSzene+0x5C0` (wenn `+0x1BC`);
2. die beiden Figuren `CSzene+0x5E0`, `+0x5E4` (`+0x58 != 0` → immer);
3. alle Ebenen `CSzene+0x04[0..39]`, **Abbruch beim ersten leeren Platz** (`0x0044B544`):
   - `+0x38` (double) ≠ −1.0 ist das **alte Z** nach einer Z-Änderung: `count[alt]−−`,
     `count[Z]++`, Max/Min mit neuem Z erweitern, war alt = Max bzw. Min → Grenze
     schrumpfen, solange der Zähler 0 ist (Max nicht unter 1, Min nicht über 0xFE;
     Vergleiche vorzeichenlos). Danach `+0x38 = −1.0`.
   - `fps == 0`: Update nur bei Interpolation (`+0x50`) **und** Stufe-1-Tick.
   - `fps ≠ 0`: Update bei dirty (`+0x1A0`) **oder** Tick der Stufe. Weil `CAnimLayer_Update`
     nach jedem Bild dirty setzt und erst das Zeichnen es löscht, schaltet eine Ebene, deren
     Zeichnen scheitert, in jedem Frame weiter.
   - Ändert das Update Z (ftol vorher/nachher), dieselbe Zählerpflege.
   - dirty → `g_SceneChanged = 1`.
   Hintergrund (nur mit `+0x1BC`, also AVI): nach dem Update `CRectList_Add(A, NULL)` (ganze
   Szene). Vollbildobjekt `+0x5DC` vorhanden → nur dieses wird aktualisiert (Update liefert 0
   → löschen) und alles Übrige übersprungen. Am Ende Inventarleiste `+0x5B4`
   (`FUN_00441CE0` bei `+0x408` oder Stufe-1-Tick).

## Animierte Ebene: `CAnimLayer_Update` (`0x00442AA0`, vtable-Slot 6)
- Ende erreicht (`+0x2E4`): Ereignis `+0x1C4` mit **Typ 1** (Animation fertig), `+0x1B8 = 1`.
- Nur wenn aktiv (`+0x1A4`) und sichtbar/laufend (`+0x1A8`).
- Periode `+0x2B0 = 1000 / fps` (Ganzzahl). Eigene Uhr `+0x2E8`:
  `next += period; if next < now: Bild weiter (FUN_00442950); next = now + period`.
  Da das nur bei Stufen-Ticks aufgerufen wird, begrenzt in der Praxis die Stufe die
  Bildrate. (Zusammenspiel im Testlauf prüfen – UNVERIFIED.)

## Multimedia-Timer
`CTimer` (0x18 Bytes, Ctor `0x0044D790`), `CTimer_Start` (`0x0044D7D0`) =
`timeSetEvent(delay, resolution, CTimer_Callback 0x0044D820, this, flags)`.
Nutzer: `CEvent_SetTimer` (`0x0043F2A0`) und die Audio-Streams (`0x00437160`, `0x00437B80`).

`CEvent_SetTimer(ms, flags)`: alten Timer stoppen; erstmals in die Timerliste
`0x0047E614[10]` eintragen; `timeSetEvent(ms, ms, …, flags)` (flags 1 = periodisch).
`CEvent_Timer` (`0x0043F390`, **läuft im Timer-Thread**): `pending = 1`, Typ/Param = **0x15**;
bei Sofort-Flag `+0x0C` direkt `CEvent_Fire` im Timer-Thread, sonst Auslösung im nächsten
`CSzene_DispatchEvents` (über `FireTimers` `0x0043F4A0`).

Port (Entscheidung 2026-10-27): Fälligkeit millisekundengenau ab Startzeit wie
`timeSetEvent`, geprüft in **jedem** Hauptschleifendurchlauf im Hauptthread. Normale
Ereignisse werden wie im Original nur markiert und in `CSzene_DispatchEvents` ausgelöst;
Sofort-Flag-Ereignisse beim nächsten Schleifendurchlauf statt im Timer-Thread (entfernt
die Race). Schalter `timer_thread = original` (Standard aus): echte Threads wie 1999.
Offen: wie oft das Sofort-Flag bei Timern überhaupt vorkommt (Phase 4).

## Spiel-Tick: `CGame_Tick` (`0x00440B80`)
Läuft frühestens alle **5001 ms** (`dt < 0x1389` → nichts) und steuert das
Hinweissystem (Hilfe-Symbol nach Countdown) – siehe HINTS.md.
