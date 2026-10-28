# Sprachausgabe und Untertitel (`CSpeech`)

Belege: `CGame_Say` `0x004404C0`, `0x004405D0`, `CGame_Speak` `0x00440430` (Wrapper
`0x004403E0`, `0x00440400`), `CSpeech_ctor` `0x00445FB0`, `CSpeech_Load` `0x004460F0`,
`CSpeech_Play` `0x004463B0`, Stopp `0x00446030`/`0x004460C0`, Ende-Handler `0x00446560`.
Ergänzt formats/ger.md (Dateien) und AUDIO.md.

## Globale Zustände (eine Sprachausgabe gleichzeitig)
| Adresse | Bedeutung |
|---|---|
| `0x00481EE8` | `g_ClickEvent` – zugleich das statische Klick-Ereignis der Szene (EVENTS.md); Handler `0x00446560` |
| `0x00481EB8` | Ereignis „Sprache fertig“ (Handler = Ereignis-ID des Aufrufers, Typ 0x1F) |
| `0x00481EB0` | Audiopuffer der Sprache (0 = keiner) |
| `0x00481EA0`/`+A4`/`+AC`/`0x00481EE4` | sprechende Figur, Bildbereich, Modus (Figuren: ACTOR.md, folgt) |
| `0x00481EE0` | Sprache läuft |
| `0x00481F0C` | Lippensync aktiv |
| `0x00481E90` | Untertitel-Rechteck (40, 424, 600, 475) |
| `0x00481F18` / `0x00481F14` | Szene / Renderer |
| `g_SubTitles` `0x00481F1C` | `rah.ini [MODES] SubTitles`, Vorgabe **0** |
| `0x00476BD4` | konstant 1 (nie geschrieben) |

## Ablauf
- `CSpeech_ctor`: Puffer, Figur, Flags 0; `g_ClickEvent` deaktiviert, Handler `0x00446560`;
  `0x00481EB8` deaktiviert; Untertitel-Rechteck; Lippensync-Rate 20.
- `CSpeech_Load(ger, name, stream)`: `[SPEECH] name` aus der GER-Datei (max. 0x400 Bytes,
  Vorgabe `NULL`; fehlt der Schlüssel → `Sorry, but Speech [%s] not found...`).
  `<name>.WAV` per `CSzene_FindFile`; vorhanden → Audiopuffer (stream 0: Pufferklasse,
  1: Streamklasse), `Open(pfad, …, 3)`. Ohne Stream und mit Puffer zusätzlich die
  Lippensync-Datei (`DAT_00476BD8`). Liefert immer 1.
- `CGame_Say(ger, name)`: vorherige Sprache stoppen, GER per `CSzene_FindFile(…, "GER")`,
  laden mit stream 1, `CSpeech_Play(0)` (ohne Ende-Ereignis).
- `CGame_Speak(ger, name, figur, modus, von, bis, ereignis)`: stoppen, laden über
  `0x004405D0` (stream 0); Figurparameter `0x00446660`; `CSpeech_Play(ereignis)`.
- **`CSpeech_Play(ereignis)`**:
  1. Mit Audiopuffer: Puffer meldet sein Ende an `g_ClickEvent` (`0x004375E0`), 3D-Position
     aus der Figur, abspielen; läuft = 1.
  2. Figur-Animation (Lippensync oder Bereich in Schleife) – folgt mit den Figuren.
  3. **Ohne Audiopuffer** (WAV fehlt): `ms = (strlen(text) / 20) · 1000`, mindestens 1000;
     `CEvent_SetTimer(g_ClickEvent, ms, einmalig)`; läuft = 1.
  4. `0x00481EB8`: Handler = `ereignis`, aktiviert. `g_ClickEvent` aktiviert.
  5. `g_SubTitles` → Statustext löschen und Text im Untertitel-Rechteck (INVENTORY.md,
     Statustext des Renderers).
- Ende `0x00446560` (Handler von `g_ClickEvent`: Ton zu Ende, Timer abgelaufen **oder Klick**
  – `CSzene_OnClick` markiert `g_ClickEvent` mit 0x16, solange `+0x510` gilt und es aktiviert
  ist): `g_ClickEvent` deaktivieren; Statustext löschen; läuft = 0; Puffer abmelden und
  stoppen; Figur stoppen bzw. Bild setzen; `0x00481EB8` mit Typ **0x1F** markieren
  (Sofort-Flag → auslösen) und anschließend `CEvent_Fire` – es wird also sofort ausgelöst
  (Param = Ereignis-ID).
- Stopp `0x00446030` (vor jeder neuen Sprache, `CGame_dtor`): läuft → `0x00446560` (das
  Ende-Ereignis feuert also auch beim Abbrechen), Puffer stoppen; dann beide Ereignisse
  deaktivieren, Puffer löschen, Lippensync freigeben.

## Port
`engine/speech` mit echten Puffern (AUDIO.md): `CGame_Say` → Stream, `CGame_Speak` → Puffer;
Ende über `g_ClickEvent`. Ohne Tonausgabegerät legt der Port keinen Audio-Manager an; dann
nimmt jede Sprache den Timer-Weg (3) (Dauer nach Textlänge).
Sprecher `0x00446660(ebene, modus, von, bis)`; `CGame_Speak` setzt bei Sprecher == Held
(`CGame+0x1C`) Modus −1 = Figur-Variante (`0x00435DF0/0x00435E00/0x00435F10`, ACTOR.md
„Sprechen“). Ebenen-Sprecher: mit ACS und Puffer Lippensync (ger.md), sonst Bereich in
Schleife (Modus 0x0D); Ende: `Stop(0)`, dann Modus 10 ausblenden, 11 Bild `bis` (≠ −1),
12 Bild `von` (−1 → 0). 3D-Position aus der X-Mitte der Ebene (`0x00447CB0` = lineare
Abbildung 0…639 → −0.2…0.2, z = 0.1). `CGame_Say` hat keinen Sprecher (Ctor setzt 0).
