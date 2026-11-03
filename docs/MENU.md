# Spielmenü

Belege: `GameMenu_Open` `0x00403370`, `GameMenu_Close` `0x00403480`, Aufbau `0x00403520`
(aus WinMain), Menüklasse `0x004436B0` (Ctor, 0x22C Bytes, vtable `0x0046B908` mit Dtor
`0x004439B0` und Start `0x00443A60`), Ereignisse `0x00444010`, Seite aufbauen `0x00443C40`,
Regler `0x004445D0`, Spielstände `0x004446C0`, Zeichnen `0x00444830`/`0x004447E0`,
Namensdialog `0x004435D0`, Tastatur `0x0044CE50`, Befehle `0x00402870`, `Save_Process`
`0x004045C0`. Daten: `GAME/MENU/MENU.SZN` (Szenenordner `menu_`).

## Aufbau beim Programmstart (`0x00403520`)
- Eigene Szene `g_MenuScene` (`0x0047D268`, `CSzene` 0xACC) auf demselben Gerät/derselben
  Oberfläche wie die Spielszene; Pfade `<Szenenpfad>menu_`. `+0x5BC` = Menüklasse,
  `+0xAC8 = 1` (wie Editor: kein Raster-Test beim Klick, keine globalen Ereignisse; in
  `CSzene_Compose` ruft `+0xAC8` am Ende das Menü-Zeichnen `0x00444830(+0x5BC)` – nur wenn
  kein Editor-Vollbild `+0x5C8` und kein Bereichstext sichtbar ist), Klickraster geleert.
- Menüklasse `g_MenuGame` (`0x0047D270`, auch `0x0047E648`): `CSzene_LoadAll(0x00476A08)`,
  Objekte per Name (Tabelle unten), Bereiche `Optionen`, `Spielstand`, `NeuesSpiel`,
  `Beenden`, `File1`…`File9` (`File%d`), Töne `Sfx_ButtonLarge` (+0x21C), `Sfx_ButtonSmall`
  (+0x220), `Sfx_Fire` (+0x224), `Menu_ROD_01` (+0x228, Kategorie 3 = Sprache, Lautstärke
  sofort angewendet `0x00437350`). `+0x80 = 1` (Einfahren beim nächsten Start).

| Off | Objekt | | Off | Objekt |
|---|---|---|---|---|
| +0x198 | Menu_MainMenu | | +0x1C4 | Menu_Regler_InfoText (Maus-Verzögerung) |
| +0x19C | Menu_RightMenu | | +0x1C8/+0x1CC | ReglerBack/Regler_Musik |
| +0x1A0 | Menu_OptHead | | +0x1D0/+0x1D4 | ReglerBack/Regler_Effekte |
| +0x1A4 | Menu_Untertitel | | +0x1D8/+0x1DC | ReglerBack/Regler_Sprache |
| +0x1A8 | Menu_InfoText | | +0x1E0 | Menu_SpielLaden |
| +0x1AC/+0x1B0/+0x1B4 | Menu_Musik/Effekte/Sprache | | +0x1E4 | Menu_SpielSpeichern |
| +0x1B8/+0x1BC | Menu_ButtonOn/Off (Untertitel) | | +0x1C0 | Menu_ReglerBack_InfoText |

## Öffnen und Schließen
- Tastatur (`0x0044CE50`, nur wenn Klicks erlaubt `+0x514` und kein laufendes
  Vollbildobjekt): **Esc** oder **F1…F12** (VK 0x70…0x7B) → `WM_COMMAND 40079` (öffnen,
  Modus 1) bzw. bei offenem Menü `40092` (schließen). Ctrl+F1/Ctrl+F2 öffnen Modus 1/2
  (CONTROLS.md). Offene Hilfe-Seite: Taste schließt nur sie (`0x004416C0`).
- `GameMenu_Open`: nur wenn nicht offen und Verschachtelungstiefe `0x0047D754 == 0`
  (nicht in einer Wartestelle). `0x0047D694` = „Speichern möglich“ = Held existiert und
  aktiv. Spielszene/-klasse sichern (`0x0047D768`/`0x0047D76C`), Menü wird aktuelle Szene,
  Ereignisprozedur der Menüklasse. Mit Spiel: **Spielbild einfrieren** (`0x0044A0C0`:
  Szenenoberfläche 640×360 ab (0,0) in die Hintergrund-Oberfläche des Menüs kopieren;
  nur bei statischem Hintergrund), Musik-Zeiger `+0x08` übernehmen. Ohne Spiel:
  Hintergrund schwarz füllen (DDBLT_COLORFILL). Dann Start (vtable +4).
- `GameMenu_Close` (Befehl 40092): Szene/Klasse zurück, Ereignisprozedur zurück
  (`0x0043F1E0`), Leiste und Szene neu zeichnen, ohne Spiel Renderer leeren, Hilfe
  `0x00441490`, offen = 0. Aufträge (`g_SaveRequest`) laufen erst danach in `Save_Process`.

## Start (`0x00443A60`) und Seitenaufbau (`0x00443C40`)
Start: `+0x80 = 1`, `+0x84 = +0x88 = +0x8C = 0`, alles deaktivieren, Ereignisse
setzen (Tabelle unten), Regler-Ebenen `fps = 20` (`+0x54 = 0x14`), Seite aufbauen.

Seite aufbauen:
1. Alles deaktivieren; `+0x60` (UNVERIFIED: Objekt aus GetViewObjects) aktivieren.
2. `+0x80 || !+0x84` → MainMenu und RightMenu aktivieren.
3. `+0x80` (erstes Mal nach Start): `+0x80 = 0`; MainMenu auf x = −250, RightMenu auf
   x = 620; beide in 5 Schritten nach x = 50 bzw. 300 fahren (MoveTo, zweite mit
   Warteplatz), warten, `Sfx_ButtonLarge`. Danach **Rahmen in den Hintergrund backen**
   (`0x0044A0C0` wie oben); gelingt es: `+0x84 = 1`, beide Ebenen deaktivieren.
4. Bereiche `Optionen`, `Spielstand`, `NeuesSpiel`, `Beenden` aktiv.
5. Modus ≠ 2 (**Optionen**): OptHead, Untertitel, InfoText, Musik, Effekte, Sprache an;
   Untertitel an → ButtonOn (klickbar), sonst ButtonOff; vier Regler: Hintergrund an,
   Knopf auf x = `490 + 52·p/100` (`0x004445A0`) mit p = Maus-Verzögerung
   `(d·5 − 500)·20/1900` bzw. Lautstärke Musik/Effekte/Sprache, Knopf klickbar.
6. Modus 2 (**Spielstand**): SpielLaden klickbar; SpielSpeichern: mit „Speichern möglich“
   Farbe (0,0,0) und klickbar, sonst Farbe (−128,−128,−128) (abgedunkelt) und nicht
   klickbar. Namen lesen (`0x004446C0`), `+0x88 = +0x8C = 1`, `File1…9` aktiv.

Eingaben werden zu Beginn beider Funktionen nicht gesperrt; am Ende `CSzene_EnableInput`.

## Ereignisse (`0x00444010`)
| ID | Auslöser | Wirkung |
|---:|---|---|
| 1 | Optionen | Modus 1, Seite aufbauen, Sfx_ButtonLarge |
| 2 | Spielstand | Modus 2, Seite aufbauen, Sfx_ButtonLarge |
| 3 | NeuesSpiel | Sfx_ButtonLarge, `MessageBox(Text 0x00476AE4, Titel 0x00470914, MB_YESNO)` (Frage nach neuem Spiel); Ja (IDYES 6) → Auftrag 3, Menü schließen |
| 4 | Beenden | Sfx_ButtonLarge, Text 0x00476AB8 (Frage nach Verlassen), ohne „Speichern möglich“ 0x00476A44 (Warnung, dass nicht automatisch gespeichert werden kann, Leerzeile, dieselbe Frage); Ja → Auftrag 4, schließen |
| 7 | ButtonOn | Untertitel aus (ButtonOff klickbar), Sfx_ButtonSmall |
| 8 | ButtonOff | Untertitel an, Sfx_ButtonSmall |
| 13 | Regler Maus | Regler ziehen, Verzögerung = `100 + 1900·p/100` (`SetMouseDelay` 100…2000) |
| 14 | Regler Musik | ziehen (Lautstärke live) |
| 15 | Regler Effekte | `Sfx_Fire` in Schleife (+0x10), ziehen, Schleifenende markieren (+0x14) |
| 16 | Regler Sprache | `Menu_ROD_01` abspielen, ziehen |
| 17 | SpielLaden | Name des Platzes; „Leer...“ → nichts; sonst Pfad `%sSaveGame%02d.rah`, Auftrag 1, schließen |
| 18 | SpielSpeichern | Namensdialog; OK → Name „Leer...“ (unverändert) → „Game %d“ (Platznummer 0…8); Pfad, Auftrag 2, `+0x88 = 1`, neu zeichnen, schließen |
| 31…39 | File1…File9 | Platz 0…8 (`0x0047D690`), Sfx_ButtonSmall, `+0x88 = +0x8C = 1`, neu zeichnen |

Titel der MessageBox: String `0x00470914` (Programmname). Port: alle Dialogtexte zur Laufzeit
aus der EXE lesen (keine Originaltexte im Quelltext). Der aktuelle Platz `0x0047D690` beginnt bei 0 und bleibt über
Menüaufrufe erhalten.

## Regler (`0x004445D0`)
Eingaben sperren; Schleife: x = Maus-x (`CSzene+0x500`) − 12, auf 490…542 begrenzen,
p = (x − 490)·100/52, Knopf auf x setzen; Musik/Effekte/Sprache-Lautstärke sofort auf p.
Ende, wenn die linke Maustaste nicht mehr gedrückt ist (`GetAsyncKeyState(1)`), sonst eine
Nachricht verteilen und `Game_Idle`. Eingaben frei, Rückgabe p.

## Spielstand-Liste (`0x004446C0`, `0x00444830`, `0x004447E0`)
- Für Platz i = 0…8: Bereichstext = Leer-Kennung (`0x00476A3C`, „Leer...“); existiert `%sSaveGame%02d.rah`, Version
  (4 Bytes) und Name (30 Bytes) lesen; Version 1 → Bereichstext = Name. Version und
  Name gehen in die globalen Puffer `0x0047080C`/`0x00470810` (beeinflusst die Reste
  im Namenspuffer beim nächsten Speichern, savegame.md).
- Zeichnen nur in Modus 2, aus `CSzene_Compose`: `+0x88` → Namen aller Plätze zentriert
  (`0x00436370` → `CArea_DrawText` in der Rechteckmitte) auf die Szenenoberfläche,
  `+0x88 = 0`, alles neu zeichnen. `+0x8C` (bleibt gesetzt) → Rahmen um den aktuellen
  Platz: Rechteck oben −5, unten −3, `FrameRect` 1 Pixel, Farbe `0x0596FA`
  (COLORREF → R 0xFA, G 0x96, B 0x05), Rechteck in beide Zeichenlisten.
- Namensdialog `IDD_FILENAME`: Position = Fenster-Client + (Platz.links − 2,
  Platz.oben + 0x36); Feld 0x3FF mit dem Bereichstext vorbelegt; OK schreibt höchstens
  29 Zeichen + NUL direkt in den Bereichstext. Abbrechen ändert nichts.

## Aufträge (`Save_Process`, Ende jedes `Game_Idle`, nicht bei offenem Menü)
Siehe formats/savegame.md. Beim Start setzt WinMain Auftrag 3 (Neues Spiel → Intro
40/114); mit `AutoLoadGame` und vorhandenem `SaveGameExit.rah` stattdessen Auftrag 1.
Beim Laden setzt `Game_SwitchLocation` die Figur auf X/Y/Richtung aus dem Spielstand
(`CActor_SetPos` `0x00434AB0`, Disassembly `0x0040415D`) und `CGame+0x04 = 1`.

## Port
- Ja/Nein-Abfragen: `SDL_ShowMessageBox` (System-Dialog wie MessageBox). Namenseingabe:
  Eingabefeld im Spielbild an der Dialogposition (SDL hat keinen Textdialog) –
  Entscheidung Nutzer 2026-11-03.
- Speicherort konfigurierbar (Entscheidung Nutzer): `--save-dir <ordner>`; Standard ist der
  Benutzer-Datenordner (`SDL_GetPrefPath`), der Common-Ordner bleibt unverändert. Wer die
  Original-Spielstände nutzen will, gibt den Common-Ordner als `--save-dir` an.
- Umsetzung: `src/game/menu.cpp` (Menüklasse, Öffnen/Schließen, Befehle `0x00402870`),
  `src/game/savegame.cpp` (`Save_Read`/`Save_Write`/`Save_Process`), Szene: `setMenuScene`,
  `onKeyDown`, `snapshotBackground`, `frameRect`; `Layer::setX`.
- `PostMessage(WM_COMMAND)` = Warteschlange vor den Plattform-Ereignissen; `CGame_Wait`
  bricht bei einem Befehl ab und verwirft ihn (Original: Esc während einer Wartestelle beendet
  sie, öffnet aber kein Menü). Ctrl-Kürzel (Accelerator) landen ebenfalls in der Schlange
  (UNVERIFIED: genaue Reihenfolge zu anderen Nachrichten).
- `SetEventVariant` = `Game::bindEventVariant`: die Wartestellen-Rückrufe gehen an die
  aktuelle Klasse (Menü bzw. Schauplatz).
- Folge des Originals: Die Menü-Szene wird nach der Spielszene gebaut und ist danach das
  Ziel des Bereichstext-Umschalters (`0x00481F30`, `CSzene_ctor`).
- Prüfoptionen: `--play`, `--auto-load`, `--save-dir`, `--key-at <bild> <vk|befehl>`; kopflos
  (`--hidden`) beantwortet Abfragen mit Ja und übernimmt den Namen unverändert.
- Probelauf 2026-11-03: Loc03 → Esc → Optionen (Einfahren, Regler), Spielstand-Seite (Namen,
  Rahmen), Speichern in Platz 3 („Game 2“, 17 122 Bytes), Laden des Original-Spielstands
  „Save2Draussen“ (Loc01/S7, Held auf 123/156), Beenden mit Autosave, `--auto-load`.
- Offen: Regler-Ziehen und Namenseingabe nur ohne Fenster geprüft; Bildschirm leeren beim
  Schließen ohne Spiel (Renderer-vtable +0x04); Tastatur `0x0044CF00` ohne Spiel.
