# Durchspiel-Test (Port-Werkzeug)

Ein Skript spielt das Spiel über benannte Klickbereiche durch und prüft Szenen, Gegenstände
und Flags. Kein Original-Code; die Eingaben gehen aber denselben Weg wie echte Nachrichten
(`GameContext::pumpMessage` → Mausbewegung/`CSzene_OnClick`/Taste), damit auch Wartestellen,
die Nachrichten zählen (SCRIPTING.md), wie beim Spielen reagieren.

Code: `src/game/walkthrough.*`, Schalter in `src/app/main.cpp`; Skripte `tests/walkthrough/`.

## Aufruf
```
tools/walkthrough.sh RentAHero "<…>/Rent-A-Hero/Common"          # alle Kapitel
./build/rentahero --cd RentAHero --common <Common> --play --skip-intro --no-auto-load \
  --save-dir <tmp> --hidden --virtual-clock 20 --walkthrough <skript>
```
Rückgabe 0 = Skript fertig, 3 = Fehler (Meldung mit Zeilennummer auf stderr).

## Vorgespulte Uhr `--virtual-clock <ms>`
`timeGetTime()` liefert eine Spielzeit, die je Game_Idle um `ms` weiterläuft; es wird nicht
gewartet. Der Mischer wird im Gleichschritt aufgerufen (22050 Hz, ohne Audiogerät), Sprache und
Musik enden also zur richtigen Spielzeit. Ein ganzes Kapitel läuft so in Sekundenbruchteilen.
Unterschied zum echten Spiel: feste Bildzeit statt ungebremster Schleife (TIMING.md) – die
Takt-Stufen sehen also immer dasselbe Raster.

## Befehle
| Befehl | Wirkung |
|---|---|
| `wait [ruhig-ms] [max-ms]` | bis das Spiel `ruhig-ms` lang ununterbrochen bereit ist (Vorgabe 1500, max. 300000) |
| `sleep <ms>` | Spielzeit vergehen lassen |
| `click <Bereich>` | Maus auf einen Punkt, an dem `CSzene_HitTest` genau diesen Bereich liefert, dann Klick |
| `move <Bereich>` | nur Mausbewegung (Text/Zeiger) |
| `clickxy <x> <y>` | Klick an Bildschirmkoordinaten |
| `item <ID>` | Gegenstand in der Leiste anklicken (blättert wie der Spieler am Rand) |
| `key <VK>` | Taste (`WM_KEYDOWN`) |
| `expect scene <Loc> <Szene>` / `item <ID>` / `noitem <ID>` / `flag <Offset> <Wert>` / `area <Bereich>` | Prüfung, sonst Abbruch |
| `shot <datei.ppm>` | Bildschirm speichern |
| `areas` | anklickbare Bereiche und Gegenstände ausgeben (zum Erkunden) |
| `note <Text>` | nur Ausgabe |

Bereichsnamen sind die Namen aus SZN bzw. Dateinamen der Ebenen (AREAS.md), wie in
`docs/locations/locNN.md`. Ein Bereich muss aktiv und an mindestens einem Punkt treffbar sein.

## „Bereit“
Eingaben werden erst abgeschickt, wenn alles gilt (sonst nennt die Zeitüberschreitung den Grund):
- keine Wartestelle aktiv (`g_IdleDepth == 1`), kein Spielmenü, kein Schauplatzwechsel;
- Klicks erlaubt (`CSzene+0x514`), kein laufendes Vollbildobjekt (`+0x5DC`);
- das globale Klick-Ereignis `0x00481EE8` ist **nicht** aktiv – sonst würde der Klick nur
  Sprache oder eine Sequenz beenden (`speechEnd`, EVENTS.md);
- der Held hat kein Laufziel, dreht sich nicht, spricht nicht, kein offener Laufauftrag;
- keine eigene Eingabe ausstehend.
`click`/`move`/`item` verlangen 200 ms Bereitschaft, `wait` die angegebene Ruhezeit.

## Stand
| Skript | Weg | geprüft |
|---|---|---|
| `01_buero.txt` | Büro 4/6 → Smashville 1/7 | 2026-11-22 |
| `02_loui.txt` | 1/1007 → 1/9 → Louis Büro 5/10, Gespräch bis Stufe 4 | 2026-11-22 |
| `03_ramil.txt` | Büro mit Ramil (9 Gesprächsstufen), Geldsack = Gegenstand 7 | 2026-11-22 |
| `04_kanal.txt` | Sack abgelegt, Kanalwesen 1/46 bis Stufe 5 | 2026-11-22 |
| `05_fremder.txt` | Kneipe 3/12 → 2/13 → 2/2013 → 2/15, Piratenangriff, Gegenstand 1 | 2026-11-22 |
| `06_sancho.txt` | Sancho 2/14 gibt Gegenstand 13, in 1/7 benutzt (Kanalwesen weg) | 2026-11-22 |

Alle Kapitel zusammen: rund 745 s Spielzeit, unter 5 s Laufzeit.
Beobachtung (nicht als Fehler gewertet, UNVERIFIED gegen das Original): Gegenstand 13
bleibt nach dem Benutzen in der Leiste (`0x004079E0` im Port entfernt ihn nicht).
