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
| `07_kanal_labyrinth.txt` | 1/46 → Kanal 7/47 → 7/49 → 7/52: Sack ins Wasser, altes Seil reißt | 2026-11-22 |
| `08_pilzwald.txt` | Karte 31/113 → Pilzwald 10/37: Teddy (12) aus 10/41 gegen Seil (10) in 10/38 | 2026-11-22 |
| `09_gitter.txt` | Seil an die Kurbel 7/52, Gitter hoch, 7/53 → Loc15 Szene 57 | 2026-11-22 |
| `10_drache.txt` | Schlucht 15/57 → Drachenhorst 16/58, Tagesgespräch (9 Stufen) → 17/59 | 2026-11-22 |
| `11_schaedel.txt` | Totenschädel 17/60: Busch weg (4 Klicks), durchs Ohr → 18/61 | 2026-11-22 |
| `12_blatt.txt` | Pilz (8) aus 10/39 an die Schnecke 10/44 → Blatt (6) | 2026-11-22 |
| `13_gehirn.txt` | Blatt aufs Gehirn 18/61, Kiefer auf, Aasfresser holen den Piraten | 2026-11-22 |
| `14_hut_hand.txt` | Hut (4) und Hand (3) im Drachenhorst | 2026-11-22 |
| `15_ring.txt` | Kanal zurück, Endavin 28/31 (Ring-Flag), Kloster 6/26 → 6/27: Gegenstand 14 | 2026-11-22 |
| `16_endavin.txt` | verkleidet durchs Tor 28/32, Gegenstand 25, Piratenkneipe 29/33 | 2026-11-22 |
| `17_kerker.txt` | Krug → Kerker 30/34, Fenster 28/35, Jasper, Gitter → Flucht nach 10/37 | 2026-11-22 |
| `18_esragoth.txt` | 10/38 → 19/68 → Zwergentor 19/69, Ramil, Tür → Bibliothek 20/70 | 2026-11-22 |
| `19_tharain.txt` | Tharain (8 Stufen), Gegenstand 21, Weg nach 71 offen | 2026-11-22 |
| `20_bergwerk.txt` | Jasmin an der Kette, Bergwerk 21/71…80: Schaufel, Spitzhacke, Leuchtstein | 2026-11-22 |
| `21_bohrer.txt` | Kessel, Bohrer nach 76 (Zunder 26), Weiche, Bohrer nach 79 → 22/81 | 2026-11-22 |
| `22_halskette.txt` | Pilzwald 10/40: Jasmin auf dem Baum, Stroh, Feuer, Halskette (16) | 2026-11-22 |
| `23_kristalle.txt` | Kristallrätsel 22/1081 → Loc09/S89 → Cynthia 14/90 | 2026-11-22 |
| `24_cynthia.txt` | Cynthia (Stufe 5) → Brunnen 12/91 (Nacht) | 2026-11-22 |
| `25_nacht.txt` | Nachtgespräch mit dem Drachen (Brunnen-Stufe 2), Brunnen ohne Geldsack | 2026-11-22 |
| `26_brunnen.txt` | Geldsack aus 15/57, Brunnen bezahlt, Rat (0x3FBC) | 2026-11-22 |
| `27_flug.txt` | Gespräch vor dem Abflug (5 Stufen) → Insel 26/93 | 2026-11-22 |

Alle Kapitel zusammen: rund 4100 s Spielzeit (gut eine Stunde), etwa 20 s Laufzeit. Bisher keine
Port-Fehler gefunden; alle Flag-/Gegenstandsänderungen wie im Port-Code erwartet.
Nächstes Kapitel: ab der Insel Loc26/S93 (Bereiche `S93_Huette S93_Meer S93_Wald S93_Dragon`).
Gegenstand 13 bleibt nach dem Benutzen in der Leiste – wie im Original: `0x004079E0` ruft
nur `Inventory_SetEvent(13, 0)` (`0x00442240`), keine Entfernen-Funktion (Callee-Liste).
