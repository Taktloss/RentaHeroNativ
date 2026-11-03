# Spielstand `*.rah`

Belege: `Save_Write` `0x00404930`, `Save_Read` `0x00404780`, Ablaufsteuerung
`Save_Process` `0x004045C0` (aus `Game_Idle` `0x00403359`). Größe und Aufbau am echten
Spielstand `SaveGame00.rah` (17 122 Bytes) gegengeprüft.

## Aufbau (Little-Endian, keine Prüfsumme)

| Off | Größe | Inhalt | Quelle im Original |
|---:|---:|---|---|
| 0 | 4 | Version, muss **1** sein | `0x0047080C` (wird vor dem Schreiben auf 1 gesetzt) |
| 4 | 30 | Name (ANSI, NUL-terminiert) | Puffer `0x00470810` |
| 34 | 4 | vorheriger Schauplatz | `0x0047D748` |
| 38 | 4 | vorherige Szene | `0x0047D744` |
| 42 | 4 | Schauplatz | `0x0047D73C` |
| 46 | 4 | Szene | `0x0047D740` |
| 50 | 4 | Figur X | `CActor+0x15D0` |
| 54 | 4 | Figur Y | `CActor+0x15D4` |
| 58 | 4 | Figur Blickrichtung | `CActor+0x15CC` |
| 62 | 400 | 100 × u32: Gegenstand *i* im Inventar (0/1) | `FUN_00441bf0(inv, i)` |
| 462 | 4 | Größe des Zustandsblocks, beim Schreiben immer `0x4110` | |
| 466 | 16 656 | Zustandsblock (`CGameState`) | `CSzene+0x5B8` |

Summe 17 122 Bytes.

## Schreiben (`0x00404930`)
- Nur wenn ein Schauplatz läuft und die Figur (`CGame+0x1C`) existiert und aktiv ist
  (`+0x1A4`); sonst wird nichts geschrieben.
- Datei `0x0047C128` wird neu angelegt (`CREATE_ALWAYS`).
- Vor dem Zustandsblock: `state[CSzene+0x5D4] = CGame+0x10` (Hinweis-Countdown der aktuellen Szene,
  HINTS.md, gamestate.md) – der Zähler wird also im Zustandsblock mitgespeichert.
- Der Name wird mit festen 30 Bytes geschrieben. Da Namen per `strcpy` in den Puffer
  kopiert werden, stehen hinter dem NUL **Reste früherer Namen**
  (z. B. „AutoSaveGame“ über „SaveGame01.rah“ → `AutoSaveGame\0h\0…`; das echte
  `SaveGameExit.rah` im Common-Ordner hat keine Reste – Puffer war beim Schreiben noch leer). Für
  byte-identische Dateien muss der Port diesen Puffer genauso fortführen.

## Lesen (`0x00404780`)
- Version ≠ 1 → Abbruch (nichts übernommen).
- Kopf direkt in die globalen Variablen; Inventar zuerst geleert (`FUN_004418d0`), dann
  jeder Eintrag ≠ 0 hinzugefügt (`FUN_00441960`) – jeder Wert ≠ 0 zählt.
- Die Blockgröße aus der Datei wird **ungeprüft** als Lesegröße in den 0x4110-Puffer
  verwendet (Überlauf bei manipulierten Dateien). Port: auf 0x4110 begrenzen –
  Sicherheitsfix ohne Einfluss auf gültige Spielstände.
- Bei Lesefehler mitten im Inventar bleibt ein teilweise geladener Zustand zurück.
- Erfolg setzt `0x0047D764 = 1`, danach `Game_SwitchLocation()`.

## Ablauf (`Save_Process` `0x004045C0`, Auftrag in `0x0047D68C`, nicht bei offenem Menü)

| Auftrag | Wirkung |
|---:|---|
| 1 | Laden: aktuellen Schauplatz löschen, `Save_Read`, bei Erfolg Schauplatz wechseln |
| 2 | Speichern: `Save_Write` |
| 3 | Neues Spiel: Schauplatz löschen, Zustand zurücksetzen (`FUN_0044da90`), Inventar leeren, Start bei **Schauplatz 40, Szene 114**, vorherige Szene 0 |
| 4 | Beenden mit Autosave (wenn `0x0047D694`): Pfad = Spielstandordner `0x0047D278` + `SaveGameExit.rah`, Name „AutoSaveGame“, speichern, `PostQuitMessage` |
| 5 | Beenden ohne Speichern |

Dateinamen der Speicherplätze: `%sSaveGame%02d.rah` (Spielmenü, `0x004442D1`/`0x004443F9`,
noch nicht im Detail analysiert). Dateidialog-Filter: `Gamestatus (*.rah)`.

## Werkzeug
`tools/save_patch.py` wandelt einen Spielstand für Probeläufe ab (Ort, Herkunft, Position, Inventar `+ID`/`-ID`,
Flags). Mit `--play --auto-load --save-dir <ordner>` startet der Port dort; so lassen sich
Abläufe mitten im Spiel prüfen, ohne sie vorher durchzuspielen.
