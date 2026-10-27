# Schauplatz-Logik: Spiel-API und Ablaufmodell

Die `LocNN`-Klassen (STRUCTURES.md) sind normaler C++-Code. Ihre Ereignis-Handler
(`LocNN_OnGenericEvent`, EVENTS.md) laufen **blockierend**: Sie starten Animationen oder
Sprache und warten in einer **verschachtelten Nachrichtenschleife**, bis diese fertig
sind. Belege jeweils mit Adresse.

## Warten: `CGame_BeginWait` / `CGame_Wait`
`CGame+0x34[5]` = Warteplätze (0 frei, 1 wartet, 2 fertig, 3 abgebrochen),
`CGame+0x4C` = zuletzt belegter Platz.

`CGame_BeginWait()` (`0x0043F880`): ersten freien Platz `i` auf 1 setzen, `+0x4C = i`,
liefert den Rückruf `CGame_WaitDone_i` (`0x0043F910`, `0x0043F940`, `0x0043F970`,
`0x0043F9B0`, …) – er wird als Ende-Handler an Animation/Sprache/Timer übergeben.
Alle 5 belegt → `+0x4C = 5`, Rückgabe 0.

`CGame_WaitDone_i(event, typ)`: Handler des Ereignisses löschen; Platz `i` von 1 auf
**2** (fertig), bei Typ **0x2A** auf **3** (abgebrochen).

`CGame_Wait()` (`0x0043F780`) mit `i = +0x4C`:
- `i == 5`: Platz 0 von 1 auf 3, Rückgabe `false`.
- Schleife, solange Platz `i == 1`:
  - Gibt es einen wartenden Platz `j < i` (äußere Wartestelle), werden alle Plätze
    `0…j` mit Zustand 1 auf **3 (abgebrochen)** gesetzt und Platz `i` bleibt 1 →
    **eine neue Aktion bricht die laufenden (äußeren) Aktionen ab**.
  - sonst eine Nachricht: keine → `Game_Idle()` (ein Bild); `WM_QUIT` → an das Fenster
    weitersenden, Platz frei, `false`; **`WM_COMMAND` → verworfen** (nicht ausgeführt!),
    Platz frei, `false`; sonst normal übersetzen/verteilen.
- Ende: Platz freigeben, `0x0047D130 = 0`, Rückgabe `Zustand != 3`
  (`true` = regulär fertig).

Typisches Muster im Schauplatzcode (78 Aufrufpaare):
```
cb = CGame_BeginWait(this);
anim->Play(…, cb);            // oder Sprache, Delay, Laufen
if (!CGame_Wait(this)) return;   // abgebrochen → Handler endet
```

**Port:** Diese Verschachtelung 1:1 nachbilden: `wait()` ruft die Hauptschleifen-Funktion
(`pumpOnce`: ein SDL-Ereignis bzw. ein `Game_Idle`) re-entrant auf, bis der Platz ≠ 1
ist. Alternativen (Coroutinen) würden die Reihenfolge von Abbrüchen schwer exakt
treffen. Menübefehle während einer Wartestelle gehen wie im Original verloren.

## Weitere API (Auswahl, nach Häufigkeit im Spielcode)
| Adresse | Name | Wirkung |
|---|---|---|
| `0x004409F0` | `CGame_Delay(ms)` | temporäres `CEvent`, Timer (`CEvent_SetTimerHandler` `0x0043F360`, einmalig) mit Warte-Rückruf, `CGame_Wait` |
| `0x00440A80` | `CGame_ChangeScene(scene, loc)` | alle Warteplätze → 3, `RequestLocation` `0x00404B20` |
| `0x00404B20` | `RequestLocation(loc, scene, prevScene, abort)` | `g_PrevLocation = g_NextLocation`, Ziel setzen, Wechsel-Flag `0x0047D750`; Ausführung in `Game_Idle` (TIMING.md) |
| `0x00440690` | `CGame_Random(min, max)` | siehe unten |
| `0x0044B160` / `0x0044B170` | `CSzene_EnableInput` / `DisableInput` | `CSzene+0x514` (Klicks an/aus, EVENTS.md) |
| `0x0043FA80` | `EventValue(x)` | `x & 0xFFF` (Codegenerator: `MakeEventValue`) |
| `0x00436440` | `CArea_SetEvent(id)` | Handler-ID setzen, Ereignis und Bereich aktivieren |
| `0x0044AFB0` | `CSzene_GetArea(name)` | Bereich per Name (`lstrcmpi`, `CArea+0x10`) |
| `0x00447490` | `CLayer_Disable` | Gegenstück zu `CLayer_Enable`, deaktiviert auch den Bereich |
| `0x00442E70` / `0x00442F90` | `CAnimLayer_Play…` | Bildbereich `+0x2D8/+0x2DC`, Ende-Handler (Warte-Rückruf) |
| `0x004430F0` | `CAnimLayer_Start(rückwärts?)` | aktiv/sichtbar/dirty setzen |
| `0x004433B0` | `CAnimLayer_ShowFrame(n)` | Bild `n` anzeigen, Abspielbereich löschen |
| `0x00442BB0` | `CAnimLayer_AddCue(a, b)` | Eintrag in Liste `+0x498` (je 0x2C Bytes) |
| `0x0043FFB0` | `CGame_LoadAnim(name)` | `.AVI` suchen, `CAnimLayer` anlegen |
| `0x004400C0` | `CGame_PlayMusic(name)` | gleiche Musik läuft weiter, sonst wechseln |
| `0x00440430` | `CGame_Speak(ini, key, …, cb)` | Sprachausgabe (ger.md); Varianten `0x004403E0`, `0x00440400` |
| `0x0043F6C0` | `CGame_GetViewObjects` | Figuren der Szene → `CGame+0x1C` (Held), `+0x54` (sein Bereich) … |
| `0x0044A330` | `CSzene_SetModal(obj)` | `CSzene+0x5DC`; 0 → altes Objekt Slot `+0x24(0)` |
| `0x004492C0` | `CSzene_LoadAll(name)` | `CSzene_Load(name, 0xFFFF)` |
| `0x004375B0`, `0x00437560` | Audio-Eigenschaften | `IKsPropertySet`-artige Aufrufe am Puffer (`+0x158`), Details offen |

## Zufall: `CGame_Random(min, max)` (`0x00440690`)
```
seed = (seed * 0x7F05) / 0xB2        // int32: Multiplikation mit Überlauf, Division signed
r    = abs(seed) % (max - min + 1) + min;  if r > max: r = max
```
`seed` = `CGame+0x20`, im `CGame`-Konstruktor (`0x0043F4D0`) mit `timeGetTime()` belegt –
also pro Schauplatz neu und nicht reproduzierbar. Port: identische Formel; für Tests
optional fester Startwert. `abs(INT_MIN)` bleibt negativ (Original-Verhalten).

## Maus: Doppelklick = Rennen
`OnMouseButton` (EVENTS.md): Doppelklick → `CActor+0x58 = 1` (`0x00435320`), die Figur
wird dann in **jedem** Bild aktualisiert statt im Animationstakt (TIMING.md) → läuft
schneller. Jeder andere Klick → `0x0047D130 = 0`, `CActor+0x58 = 0` (`0x00435330`).

## Figur: Laufen und Drehen (`CActor`)
| Adresse | Name | Wirkung |
|---|---|---|
| `0x00434D70` | `CActor_WalkTo(x, y, cb, dir)` | ruft `0x00434D90` (297× im Spielcode) |
| `0x00434D90` | `CActor_WalkTo2(x, y, cb, dir)` | mit `cb`: `+0x1AE8 = 0` (Ziel-erreicht-Merker); dann `0x00434DC0` (149×) |
| `0x00434DC0` | `CActor_SetWalk` | `cb` → Ereignis `+0x1C8` (bei Bedarf neu, aktiv); `dir != -1` → End-Richtung `+0x1AD4`; dann Kern |
| `0x00434E80` | `CActor_WalkToClick(x, y)` | End-Richtung −1, Kern ohne Rückruf (Klick ins Leere, EVENTS.md) |
| `0x00434EA0` | `CActor_WalkCore(x, y, hatCb)` | siehe unten, Wegsuche PATHFINDING.md, Position/Richtung ACTOR.md |
| `0x00435BF0` | `CActor_Turn(dir, cb)` | schon in Richtung `dir` → Standbild `+0x1A60[dir]`, sofort **0x29**; sonst Drehung starten (`+0x1A8C` = alte, `+0x15CC` = neue Richtung, `+0x1AD8 = 1`) |

`CActor_WalkCore` (Anfang belegt, Wegsuche folgt):
- Figur nicht bereit (`+0x15C8 == 0`) → nichts.
- Ziel = aktuelle Position (`+0x15D0/+0x15D4`): End-Richtung gesetzt und abweichend →
  `CActor_Turn(dir, Handler des Rückrufs)`; sonst Ereignis **0x29** („angekommen“)
  auslösen, `+0x1AE8 = 1`. Jeweils `CActor_ClearRun`.
- Neuer Befehl **ohne** Rückruf, während ein alter Rückruf noch offen ist
  (`+0x1AE8 == 0`): altes Ereignis mit **0x2A** („abgebrochen“) auslösen.

Zusammen mit `CGame_WaitDone_i` (0x2A → Platz 3): **Klickt der Spieler während eines
Skript-Laufwegs ins Leere, wird der Weg ersetzt, das Skript bekommt „abgebrochen“ und
der Handler endet** (`if (!CGame_Wait()) return;`). Ereignistypen: 0x29 = Figur
angekommen/gedreht, 0x2A = Laufbefehl abgebrochen (EVENTS.md).
