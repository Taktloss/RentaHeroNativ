# Spieldateien

Referenzversion für den Port: **Rent-A-Hero V4.0 (deutsch), 2 CDs**.

| Datei | SHA-256 | Größe |
|---|---|---:|
| `Rent-A-Hero.exe` (installiert, das Spiel) | `a1ccd60497111f0ac2e50d4eedae0dcd7973a49da8947b6a80d7f6fccc7b4927` | 716 873 |
| `RENT-A-HERO.EXE` (CD1, Autorun-Starter) | `149dcc4722ca06d9ae4c4e81bdde842a21099d65e7f2624cff7f530882203c17` | 110 592 |
| `SETUP/DATA1.CAB` (CD1, InstallShield 5) | `c5667ca1bcbd2e173e03303e7d67bf6ed173147ec213fb1dc2edc6d83507b773` | 162 397 234 |

Vollständige Manifeste (Prüfsumme, Größe, Pfad), erzeugt mit `tools/inventory.py`:
- `docs/manifests/images.sha256` – BIN/CUE-Images
- `docs/manifests/cd-merged.sha256` – zusammengeführter CD-Inhalt (2792 Dateien)
- `docs/manifests/install.sha256` – Installationsverzeichnis (131 Dateien)

## Aufteilung

- **CD1**: Starter, Setup (`SETUP/`, enthält in `DATA1.CAB` die EXE und `Common/`),
  Redistributables (`DIRECTX/` = DirectX 6, `INTELINDEO5/`, `MSDIRECTMEDIA/`),
  `GAME/COMMONCD`, `GAME/MENU`, Schauplätze `LOC35 LOC37 LOC40 LOC41`.
- **CD2**: `GAME/COMMONCD` (identisch zu CD1), Schauplätze `LOC01`–`LOC31`.
- Zuordnung Schauplatz → CD steht auch in `Common/CDMap.ini` (`[CDMAP] locNN=<CD>`)
  und deckt sich exakt mit dem Inhalt der CDs.
- **Installation** (`C:\Program Files (x86)\neoSoftware\Rent-A-Hero\`): `Rent-A-Hero.exe`,
  `Uninst.isu`, `Common/` (Musik, Inventar-Grafiken, Rodrigo-Animationen, Basis-SFX,
  einige Videos, `CDMap.ini`, `DefInvent.ger/.eng`).

## Dateitypen (Spieldaten in `GAME/` und `Common/`)

| Endung | Anzahl | MB | Inhalt | Status |
|---|---:|---:|---|---|
| AVI | 136 | 428,2 | Videos/Animationen, Cinepak/Indeo 5/Indeo 3, 24 bit, ohne Ton; Vollbild 640×360 | belegt (Header) |
| A16 | 252 | 222,1 | Animationen (Name deutet auf 16-Bit-Farbe), Header beginnt mit Bildanzahl? | UNVERIFIED |
| WAV | 1034 | 188,1 | PCM mono 22050 Hz, 8 bit (1005) bzw. 16 bit (29); Sprache, SFX, Musik | belegt (Header) |
| BMP | 93 | 59,4 | Hintergründe 640×360, 24 bit unkomprimiert; eine 512×386 8 bit | belegt (Header) |
| ZBF | 119 | 26,2 | Tiefenpuffer pro Szene (Namen `*_DEPTH.ZBF`) | UNVERIFIED |
| TCC | 106 | 2,8 | Einzelbilder/Sprites (z. B. Menüknöpfe, Hilfe-Icon) | UNVERIFIED |
| SZN | 77 | 0,1 | Szenenbeschreibung, INI-Text: `[GENERAL]` (Background/Layer/Area), Objektsektionen (Filename, X, Y, Z, ZoomVal, fps; Areas X1/X2/Y1/Y2/CursorType), `[COMPILER]` (Loc, Szn, VideoN) | belegt (Text) |
| ACS | 704 | 0,1 | klein, binär, Namensschema `<Satznr>_<Sprecher>_<nr>` wie die WAVs → vermutlich Lippensync-/Sprechanimationsdaten zu Sprachausgabe | UNVERIFIED |
| GER / ENG | 34 / 1 | 0,1 | Texte im INI-Format, z. B. `[SPEECH] INVENT001="…"`, Kodierung cp1252 | belegt (Text) |
| HLP | 17 | <0,1 | Rätseltipps, cp1252-Text mit XOR 0x80 | belegt (Dekodierung geprüft) |
| INI | 2 | <0,1 | `CDMap.ini`; `GAME/LOC21/RAH.INI` = liegengebliebene Entwickler-Konfiguration (`ShowMenu`, `ShowAreas`, `SHOWTIMING` …) | belegt (Text) |

Bildschirm: Szenen sind 640×360; das Fenster ist 640×480 → vermutlich 120 px für
UI/Untertitel/Inventar. UNVERIFIED.

## Registry und Systemänderungen

Durch die Installation (InstallShield 5):
- `HKLM\Software\neoSoftware\Rent-A-Hero\4.0` (leer)
- `HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\Rent-A-Hero.exe` (Pfad)
- `HKLM\…\Uninstall\Rent-A-Hero` (`IsUn0407.exe`)
- `C:\WINDOWS\rah.ini`: `[MODES] TrueColor, MouseDelay, SubTitles`,
  `[DATAPATH] CommonPath, CommonPathCD, ScenePath, ScenePathCD, ExecPath, UninstallPath`,
  `[SOUNDS] MusicVol, SfxVol, SpeechVol`

Durch das Spiel zur Laufzeit (aus dem Binary, Funktion `0x00448290`):
- liest/schreibt `…\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\NoDriveTypeAutoRun`
  (Hive noch nicht bestimmt; schaltet vermutlich CD-Autostart während des Spiels ab, UNVERIFIED) → im Port nicht nachbilden.
- Spielstände: `%sSaveGame%02d.rah`, `SaveGameExit.rah`, `AutoSaveGame`; Temp-Datei `RAH.TMP`.
