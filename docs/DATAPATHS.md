# Datenpfade und Dateisuche

Belege: `Config_ReadPaths` `0x00404B60`, `WinMain` `0x00401E00`, `Game_SwitchLocation`
`0x004036E0`, `CSzene_FindFile` `0x00448B30`, `CSzene_FindPath` `0x00448A70`,
`MakeFileName` `0x00447F90`.

## Konfiguration (`rah.ini` im Windows-Ordner, Sektion `[DATAPATH]`)
| Schlüssel | Global | Installiert (Beispiel) |
|---|---|---|
| `CommonPath` | `0x0047D278` | `<Installationsordner>\common` (Festplatte) |
| `CommonPathCd` | `0x0047D37C` | `D:\SETUP\..\Game\CommonCD` |
| `ScenePath` | `0x0047D480` | `D:\SETUP\..\Game` |
| `ScenePathCd` | `0x0047D584` | `D:\SETUP\..\Game` |

Alle vier bekommen ein `\` angehängt. Fehlt `CommonPath` → Meldung „Die RAH.INI
Konfigurationsdatei k…“, Programmende. Der Spielstandordner ist `CommonPath`
(Autosave `SaveGameExit.rah`, savegame.md).

CD-Suche (nur zum Verständnis, **wird nicht nachgebildet**): existiert `CommonPathCd`
nicht, werden alle Laufwerke vom Typ CD-ROM durchprobiert (Laufwerksbuchstabe
ersetzt); bei Treffer wird der Buchstabe in allen Pfaden mit gleichem Laufwerk
ersetzt, sonst Dialog „CD einlegen“ (`LastCdNr` aus `[MODES]`) und erneut prüfen.

## Suchordner der Szene
`CSzene` hält vier Ordner (je 260 Bytes):

| Feld | Inhalt | gesetzt in |
|---|---|---|
| `+0x5E8` | `ScenePath` + `LocNN\` (bzw. `menu\` bei Schauplatz 0) | `Game_SwitchLocation` |
| `+0x6EC` | `ScenePathCd` + `LocNN\` / `menu\` | `Game_SwitchLocation` |
| `+0x7F0` | `CommonPath` | `WinMain` |
| `+0x8F4` | `CommonPathCd` | `WinMain` |

Das Menü (`g_MenuScene`) bekommt dieselben Common-Pfade und `menu\` als Szenenordner
(`0x00403520`).

## Suchreihenfolge (`CSzene_FindFile(name, ext)` und `CSzene_FindPath(name)`)
1. Szenenordner (`+0x5E8`)
2. `CommonPath` (`+0x7F0`)
3. `CommonPathCd` (`+0x8F4`)
4. Szenenordner CD (`+0x6EC`)

Erster Treffer per `_access(pfad, 0)` (`0x0044E810`) gewinnt; ohne Treffer liefert
`FindFile` NULL (`FindPath` ebenso).

`MakeFileName(dir, name, ext)`: hat `name` mehr als 4 Zeichen und steht an
Position `len−4` ein `.`, bleibt der Name, sonst wird `"." + ext` angehängt.
Namen mit kurzer Endung (z. B. `a.b`) erhalten also zusätzlich die Endung.

## Inhalt von `CommonPath` (Festplatte, aus `DATA1.CAB` installiert)
128 Spieldateien, die **nicht** auf den CDs als Einzeldateien liegen: 77+1 WAV, 33 A16,
12 AVI, 3 TCC (u. a. Inventar-Animationen), `DefInvent.ger`/`.eng` (Inventartexte,
formats/ger.md) und `CDMap.ini`; dazu die Spielstände.

`CDMap.ini` (`%sCDMAP.INI` mit `CommonPath`, gelesen in `0x00404360`): Sektion `[CDMAP]`,
Schlüssel `loc01…`, Wert = CD-Nummer (1 oder 2; 4 × CD1, 29 × CD2). Dient nur dem
CD-Wechsel und wird im Port nicht gebraucht.

## Port
- Der Nutzer gibt ein Datenverzeichnis an (zusammengeführte CD, siehe SETUP.md).
  Abbildung: `ScenePath` = `ScenePathCd` = `<daten>/GAME/`, `CommonPathCd` =
  `<daten>/GAME/COMMONCD/`, `CommonPath` = installierter Ordner `Common`. Die
  Common-Dateien gibt es nur in `SETUP/DATA1.CAB`: InstallShield-Kabinett, Signatur
  `ISc(`, Versionsfeld `0x01005100` → **InstallShield 5**, Kopf in der CAB selbst (keine
  `.HDR`). Möglichkeiten (Entscheidung Nutzer): (a) Nutzer gibt zusätzlich den
  installierten `Common`-Ordner an; (b) Entpacken beim ersten Start mit `libunshield`
  (MIT-Lizenz, GPL-kompatibel, unterstützt IS5). Prüfung über `docs/manifests/install.sha256`.
  Spielstände in einen eigenen Benutzerordner.
- Windows-Dateisystem ist case-insensitiv; die CD-Daten sind großgeschrieben, die
  Namen im Code gemischt (`Loc05`, `hlp`). Port: case-insensitive Suche
  (Verzeichnis-Index beim Start).
