# Sprachdateien `LocNN.GER` (Texte) und `*.ACS` (Lippensynchronisation)

## LocNN.GER – INI-Datei pro Schauplatz
Gelesen mit `GetPrivateProfileStringA` (Windows-INI-Semantik: Schlüssel ohne
Groß-/Kleinschreibung, erster Treffer, Kommentar `;`, Wert ohne Anführungszeichen-Logik).
Endung fest `GER` (`0x00476734`); eine ENG-Variante gibt es in dieser Version nicht.
Dateipfad über `FUN_00448b30(szene, "LocNN", "GER")` (Pfadauflösung wie SZN, offen).
33 Dateien, je zwei Sektionen:

| Sektion | Schlüssel | Wert | Leser |
|---|---|---|---|
| `[AREA_TEXT]` | Bereichsname (wie in SZN/`CSzene_AddArea`) | Anzeigetext, max. 99 Zeichen | `CSzene_AddArea` `0x0044ADD0` → `CArea_SetText` `0x004361F0` |
| `[SPEECH]` | Sprachdatei-Name, z. B. `010_LOU_01` | Untertitel, max. 1023 Zeichen | `CSpeech_Load` `0x004460F0` |

- `AREA_TEXT` wird nur gelesen, wenn `CSzene+0x5D0` (aktuelle Schauplatznummer) ≠ 0;
  Datei `Loc%02d.GER`. Fehlender Schlüssel (`"NULL"` als Vorgabe) → kein Text.
- `SPEECH` fehlt → Text wird `Sorry, but Speech [%s] not found...` (so auch im Port).

## DefInvent.ger (Common-Ordner) – Inventartexte
Wie LocNN.GER, nur Sektion `[SPEECH]`, Schlüssel `INVENT001…`. `CGame_DescribeItem(n)`
(`0x00440AD0`): Figur vorhanden und aktiv (`+0x1A4`) → Figur dreht sich (`Slot +0x24`),
spricht `INVENT%03d` (mit WAV/ACS aus Common); sonst nur Text `INVENT%03dH`.
Eine `DefInvent.eng` liegt ebenfalls bei, wird vom Code aber nicht verwendet (Endung fest `GER`).

## Sprachausgabe `CSpeech_Load(ini, name, stream)` (`0x004460F0`)
Objekt `CSpeech` (0x400 Bytes = Textpuffer, Ctor `0x00445FB0`), aufgerufen u. a. von
`CGame_Say` `0x004404C0` (`stream = 1`) und `0x004405D0`.
1. Untertitel aus `[SPEECH]`.
2. `<name>.WAV` vorhanden → Audioobjekt: `stream == 0` → Puffer-Klasse (0x188,
   `0x00436C30`), sonst Stream-Klasse (0x1A4, `0x004376C0`); `Open(path, ?, 3)` (Slot 1).
3. Nur bei `stream == 0` und vorhandenem Audio: `<name>.ACS` laden (unten).

`CSpeech_Play` (`0x004463B0`):
- Mit Audio (und Schalter `0x00476BD4`): Ende-Ereignis `0x00481EE8` an das Audio hängen,
  Stereo-Position aus der X-Mitte des Sprechers: `pan = map(x, 0…639 → −0.2…0.2)`,
  abspielen.
- Ohne Audio: Timer `0x00481EE8` auf `max(1000, (strlen(text) / 20) * 1000)` ms
  (Ganzzahl – Texte < 40 Zeichen → 1 s, 40–59 → 2 s …).
- Sprecher-Animation `0x00481EA0`: mit ACS → `CAnimLayer_SetLipSync` `0x00442EF0`,
  sonst normale Sprechanimation (`FUN_00442f90(…, 0x0D, …)`); Figur-Variante `0x00435E00`.
- Untertitel nur bei `g_SubTitles` (Ctrl+F11/F12) – Ausgabe in Rechteck `0x00481E90`.

## ACS – Lippensynchronisation
704 Dateien neben den WAVs (14–670 Bytes). Inhalt: Folge von **u16-Bildnummern**.

Laden (`0x004460F0`): Puffer = Dateigröße + 100 Bytes, vorab mit `0xFF` gefüllt.
- Erstes u16 = `0xFFFF` → Kopf: `u16[1]` = Datenoffset, `u16[2]` = Rate;
  Daten ab Offset. (388 Dateien, alle mit Offset 6, Rate 10.)
- sonst Rate = 20, Daten ab 0. (316 Dateien.)

Abspielen (`CAnimLayer` Felder `+0x484` Daten, `+0x488` Rate, `+0x490` Bildoffset,
`+0x494` Audioobjekt; Auswertung in `0x00442730`):
```
idx   = (audio->GetPos() * rate) / 1000      // vtable +0x18, unsigned; Einheit ms (UNVERIFIED)
v     = data[idx]
if v == 0xFFFF: Animation beenden (FUN_004429d0)
else: bild = bildoffset + v; image->Seek(bild); image->Next()
```
Rate = Einträge pro Sekunde. Das Datenende wird durch die `0xFF`-Füllung erkannt;
liegt `idx` mehr als 50 Einträge hinter dem Ende, liest das Original über den Puffer
hinaus – Port: außerhalb als `0xFFFF` behandeln.
