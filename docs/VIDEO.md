# Videoebenen (AVI, `CImageAvi`)

Ergänzt `formats/media.md` (Zeitmodell), ANIMATION.md (CAnimLayer), COMPOSITOR.md
(CLayer_Draw). Port: `src/engine/aviimage.*` (Zustandsmodell), `src/video/ffvideo.*`
(FFmpeg-Dekoder), `src/engine/layer.cpp` (Zeichenpfad), Tests `tests/test_avi.cpp`.
Werkzeug: `re/scripts/CallArgs.java` (Push-Argumente je Aufruf), `re/scripts/FieldWrites.java`.

## vtable `0x0046B890` (14 Slots)
| Slot | Adresse | Funktion |
|---|---|---|
| 0 | `0x0043C7C0` | Load |
| 1 | `0x0043CAF0` | Draw mit Ebenenargumenten → Slot 2 `(surface, rect, flag)` |
| 2 | `0x0043CB10` | Draw (Sample aktualisieren) |
| 3 | `0x0043CAB0` | in eigene Oberfläche: Slot 2 mit `(0, 0, +0x338, +0x33C)` |
| 4 | `0x0043CA80` | Next |
| 5 | `0x0043CA60` | `return 1` (Nachholschritt: kein Bildwechsel, wie A16) |
| 6 | `0x0043C9A0` | Rewind |
| 7 | `0x0043C9E0` | Seek |
| 13 | `0x0043C6A0` | Destruktor |

## Load `0x0043C7C0`
`CoCreateInstance(CLSID_AMMultiMediaStream)` → `Initialize(STREAMTYPE_READ, 1 =
AMMSF_NOGRAPHTHREAD, 0)` → `AddMediaStream(DirectDraw 0x0047E600, MSPID_PrimaryVideo, 0, 0)`
→ `OpenFile(name, 4 = AMMSF_NOCLOCK)` (Argumente per `CallArgs.java`, `0x0043C81D`–`0x0043C868`).
**Ohne Uhr**: jedes `Update` liefert das nächste Bild, unabhängig von der Zeit.
Dann `GetMediaStream` → `IDirectDrawMediaStream` → `GetFormat` (Breite `+0x404`, Höhe `+0x400`),
Start `+0x3C4 = +0x3CC = GetTime / 1e6`, Länge `+0x3C8 = GetDuration / 1e6` (Einheit 100 ms,
`__alldiv` schneidet ab), `+0x338/+0x33C` = Breite/Höhe (640×180 → 360, `+0x3D8 = 1`; kommt
nicht vor), geladen `+0x3DC = 1`, `SetState(RUN)`, Bild angefordert `+0x3E4 = 1`.

## Draw `0x0043CB10(surface, rect)`
1. `+0x3CC++` – **bei jedem Aufruf**, auch wenn danach nichts gezeichnet wird.
2. Nicht geladen → 0.
3. Zielrechteck = `rect`; bei `+0x38C` (Bild einer Ebene mit `+0x30`) stattdessen
   Primäroberfläche `0x0047E604`, Rechteck + Fensterursprung + (0, 60).
4. Kein Sample (`+0x3E0 == 0`): ohne Anforderung (`+0x3E4 == 0`) → 1 (nichts). Sonst
   `CreateSample(ziel, rect, 1)` → `+0x3F4`, `SetState(RUN)`, `+0x3E0 = 1`, Rechteck merken
   (`+0x464`).
5. Rechteck geändert: merken; ist das Ziel die Oberfläche des Samples → `SetRect`, sonst
   Stopp, Sample freigeben, neu anlegen, Lauf. **Gleiches Rechteck, andere Oberfläche → das
   Sample schreibt weiter in die alte.**
6. `Update(0)` (synchron; bei `+0x38C` asynchron, wiederholt bei `MS_E_BUSY`):
   `S_OK`/`MS_S_PENDING` → 1; `MS_S_ENDOFSTREAM` (`0x40003`) → `+0x3CC = Länge + 1`, 1
   (Ziel unverändert); sonst 0.

## Next `0x0043CA80`, Ende `0x0043CD60`, Seek, Rewind
- Next: `+0x3CC < Länge` → `+0x3E4 = 1`, 1. Sonst (Sample vorhanden) `0x0043CD60`, 0.
- `0x0043CD60`: mit Sample: `CompletionStatus(COMPSTAT_WAIT, INFINITE)`, `SetState(STOP)`,
  Sample freigeben, `+0x3E0 = 0`; immer `+0x3E4 = 0`.
- Seek(n) `0x0043C9E0`: `+0x3E4 = 1`; nur wenn `n ≠ +0x3CC`: `n ≤ Länge` → `+0x3CC = n`;
  `IMultiMediaStream::Seek(+0x3CC · 1 000 000)` (Produkt in 32 Bit, vorzeichenerweitert).
  Liefert immer 1.
- Rewind `0x0043C9A0`: `+0x3CC = Start`, `Seek(0)` am Stream, `+0x3E4 = 1`, dann Next.

Zähler `+0x3CC` = Anzahl der `Draw`-Aufrufe seit Start/Seek, Stream-Position = Anzahl der
erfolgreichen `Update`s. Beide laufen auseinander, wenn `Draw` ohne Sample und ohne Anforderung
aufgerufen wird (nach dem Ende) – dann zählt nur der Zähler.

## Ebenenseite
- `CAnimLayer_CreateImage` (`0x004425B0`): `image+0x38C = layer+0x30`; Alpha `+0x198 = 1`.
- `layer+0x30` ist bei SZN-Ebenen **0** (Ctor `0x004466A0`). 1 setzen nur `CGame_LoadAnim`
  (`0x00440016`) und das Vollbildobjekt `CSzene+0x5DC` (`0x0044A360`, `FieldWrites.java`) –
  diese Videos gehen direkt auf den Bildschirm und folgen mit den Schauplätzen.
- `CLayer_Draw` (`0x004471C0`), Typ 3: **direkt** = Zoom 100 und `flag == 0` und `+0x30`.
  Nicht direkt und Alpha gesetzt → Alpha 0, Oberfläche freigeben (`0x00446E20`). Direkt und
  Alpha 0 → Alpha 1, Oberfläche freigeben. Alpha 0 → Weg über die Ebenenoberfläche (`+0x08`,
  fehlt sie: `0x00446C90`), `BltFast` mit Farbschlüssel bei `flag ≠ 0` (wie BMP). Alpha 1 →
  Slot 1 → Draw mit dem **ungeclippten** Ebenenrechteck.
- `0x00446C90` („in Ebenenoberfläche zeichnen“): nicht geladen → 0; Alpha und Typ > 0 → 1
  (A16/TCC, auch AVI vor dem ersten Zeichnen). Aktiv → dirty. Fehlt die Oberfläche: anlegen
  (Breite/Höhe der Ebene, Systemspeicher, `SetColorKey(DDCKEY_SRCBLT, Ebenen-Farbschlüssel)`).
  Dann Bild-Slot 3 → für AVI ein `Draw` in die Ebenenoberfläche.
- Aufrufer von `0x00446C90` u. a.: Ende von `CAnimLayer_Update` (`0x00442B6A`, nach
  `NextFrame`), `CAnimLayer_ShowFrame`, `CSzene_CreateLayer`, `CSzene_CreateBackground`.
- `0x00446E20(1)`: Oberfläche `PageUnlock`, freigeben, `+0x08 = 0`.
- `SeekFrame`/`Start` rufen bei Typ ≥ 2 den Bild-Seek (Slot 7); bei beendeter Ebene `Seek(0)`.

### Ablauf einer SZN-Videoebene
1. Anlegen: Alpha 1 → `0x00446C90` tut nichts.
2. Update: `NextFrame` → Next (`+0x3CC < Länge`); `0x00446C90` tut noch nichts (Alpha 1).
3. Erstes `CLayer_Draw`: nicht direkt → Alpha 0, Oberfläche anlegen, `Draw` → Sample auf
   der Ebenenoberfläche, Bild 0 dekodiert, `BltFast` in die Szene.
4. Jedes weitere Update: `NextFrame`, dann `0x00446C90` → `Draw` → nächstes Bild.
5. Nach dem letzten Bild: Next liefert 0 → `0x0043CD60` → OnEnd (Schleife: Rewind = Stream auf
   0, nächster `Draw` legt ein neues Sample an und dekodiert Bild 0).

## AVI-Hintergrund (`CSzene_CreateBackground` `0x00449E70`)
Erst als `CLayer` geladen; setzt `CLayer_Load` `+0x1BC` (AVI), wird die Ebene verworfen und als
`CAnimLayer` neu geladen. Dann Z = 255, Slot 3, `0x00446C90`, `0x00446DE0`, Position, Rechteck in A.
Gezeichnet mit `flag = 0` → Weg über die Ebenenoberfläche ohne Farbschlüssel.

## Port
- Dekodieren mit FFmpeg (Cinepak, Indeo 3/5) bildweise, ohne Uhr, aus dem Speicher (VFS).
  Zeit in 100 ns: Dauer = Bildanzahl · 10⁷ · fps-Nenner / fps-Zähler, Seek auf
  `t` → erstes Bild mit Anzeigezeit ≥ `t`.
- Pixel: FFmpeg → RGB24 → `pack565` (abschneiden). UNVERIFIED: DirectShow wandelt über den
  Codec bzw. den Farbraumwandler in das 16-Bit-Format der Oberfläche; Rundung/Dithering dort
  nicht belegt.
- UNVERIFIED: `SetState(STOP)`/`RUN` behält die Stream-Position.
- Direkter Weg (`+0x30`, Primäroberfläche) und 640×180-Zeilenverdopplung: noch nicht portiert.

## Vollbildobjekt `CSzene+0x5DC` (Intro-Videos, `CGame_PlayAnimWait`)
Belegt an `CSzene_SetModal` `0x0044A330`, `CGame_LoadAnim` `0x0043FFB0`, `0x0044A360`,
`CSzene_Update` `0x0044B3C5`, `CSzene_Compose` `0x0044B7D0`, `CSzene_DispatchEvents`
`0x0044C690`, `CSzene_OnClick`, `CSzene_MouseMove`, `CSzene_Reset` `0x00448930`.
- `CGame_LoadAnim(name)`: `CSzene_FindFile(name, "AVI")`, neue `CAnimLayer` mit `+0x30 = 1`,
  Laden mit dem Farbschlüssel der Szene, Slot 3, Alpha `+0x198 = 1`, `SetPos(0, +0x2F0)`,
  Name = Dateiname. Gehört dem Schauplatz (Loc40-Dtor löscht es).
- `CSzene_SetModal(obj)`: `+0x5DC = obj`; `obj == 0` → altes Objekt Slot 9 (`Stop(0)`) und
  `CSzene_InvalidateAll`. `0x0044A360(pfad)` legt dagegen selbst eine `CAnimLayer` an und
  **löscht** das vorherige Objekt (auch eines, das einem Schauplatz gehört).
- Update: ist `+0x5DC` gesetzt, wird **nur** es aktualisiert (dirty oder Tick seiner Stufe),
  keine Figuren, kein Hintergrund, keine Ebenen; die Inventarleiste schon. Liefert das Update 0,
  wird gelöscht und danach `+0x5DC` gelesen (Nullzeiger) – kommt bei `CAnimLayer` nie vor.
- Compose Schritt 2 (noch vor „kein Hintergrund“): nicht (beendet und ausblenden) → bei dirty
  und aktiv `Draw(Szenenoberfläche, flag 0)` – AVI mit `+0x30` geht damit direkt auf die
  Primäroberfläche (Rechteck + (0, 60)). Beendet und ausblenden **ohne** Ende-Ereignis →
  `Stop(0)`, abmelden, `InvalidateAll`. Ist das Objekt danach aktiv → `g_SceneChanged = 0`,
  Rückgabe 0 (keine Ausgabeliste).
- DispatchEvents nach dem Szenen- und Umschalt-Ereignis: hat das Objekt ein Ende-Ereignis,
  bei „beendet und ausblenden“ erst `Stop(0)`, abmelden, `InvalidateAll`; dann Ende-Ereignis
  auslösen. Danach Bild-Ereignis `+0x1CC`, falls noch angemeldet.
- OnClick: `+0x518 == 0` und Objekt aktiv und läuft → Klick verworfen. MouseMove: Objekt
  aktiv und läuft → nichts (Zeiger 0).
- `CSzene_Reset` (`0x00448930`, aus `CGame_dtor`) setzt `+0x5DC = 0` ohne zu löschen.
- Eine fehlende SZN (Loc40 lädt `xxx`) ergibt eine leere, aber geladene Szene (`+0x00 = 1`).
Port: `Scene::setModal/loadAnim`, `AnimLayer::drawImageDirect` (`g.screen`).

## Indeo-5-Transparenz
Von 136 AVIs sind 95 Cinepak (`cvid`), 39 Indeo 5 (`IV50`), 2 Indeo 3 (`IV32`). **21** der
Indeo-5-Videos (u. a. alle Figuren-Videos von Loc35, Credits, Drache, Uhr-Szenen) haben eine
Transparenzmaske (GOP-Flag `0x08`), die FFmpeg überspringt → ohne Auswertung graue Flächen.
Alle 21 nutzen dieselbe Form: GOP-Flags `0x89`, eine Kachel, Füllfarbe Schwarz im Bitstrom.

**Messung mit dem Original-Codec** (`tools/iv5ref`, `ir50_32.dll` aus dem Wine-Prefix,
SHA-256 `e473843f364a447d…`): Das Messprogramm ruft den VfW-`DriverProc` direkt
(`DRV_OPEN`, `ICM_DECOMPRESS_BEGIN`/`ICM_DECOMPRESS`, RGB24, Puffer vorher 0xA5). Jedes Pixel
wird geschrieben; transparente Pixel werden mit **Schwarz** gefüllt = Farbschlüssel der Ebenen
(COMPOSITOR.md) → im Spiel durchsichtig.

Format und Herleitung: formats/indeo5-transparenz.md.

**Port** (`src/video/iv5mask`, `src/video/ffvideo.cpp`): Beim Senden jedes Pakets liest
`Indeo5Mask` die Maske (nur Kopfdaten und Bandlängen, kein Koeffizienten-Dekodieren) und
merkt sie unter dem Zeitstempel; nach dem Umwandeln nach RGB24 werden Pixel mit Maskenbit 0
schwarz gesetzt. Nicht nachgebildete Varianten (Kacheln, Schutz, fehlende Bandlängen) bleiben
deckend (kommen in den Spieldaten bei transparenten Videos nicht vor). Geprüft: alle 21
Videos (je bis 30 Bilder) pixelgenau gegen den Original-Codec (Prototyp
`tools/iv5ref/iv5mask.py`); C++-Leser gegen den Prototyp (`tests/test_iv5mask.cpp`,
`RAH_TEST_COMMON` = installierter `Common`-Ordner).

**Alle Bilder (2026-11-04):** `rah_iv5scan RentAHero <Common>` liest jedes Bild aller
39 Indeo-5-Videos: 824 deckend, **4149 mit Maske, 0 nicht nachgebildet**, Maskengröße immer gleich
der Videogröße. `tools/iv5ref/compare_all.sh <arbeitsordner> build/rah_iv5scan RentAHero
<Common>` (mit `WINEPREFIX`) dekodiert jedes der 21 Videos komplett mit dem Original-Codec
und vergleicht: **Maske 0 → immer schwarz** (0 Abweichungen bei rund 770 Mio. Pixeln). Von den
rund 64 Mio. Pixeln mit Maske 1 sind nur 9174 schwarz, das ist echter Bildinhalt (davon 9159 im
Abspann). Eine falsche Maske würde dort viel Schwarz zeigen, also sind beide Richtungen praktisch
belegt.

**Weitere Randfälle (Datenscan, 2026-11-04):**
- Keine der 136 AVIs (124 CD, 12 Common) hat leere Video-Chunks, alle laufen mit 10 fps.
  Das Verhalten von DirectShow bei Lücken spielt also keine Rolle.
- Länger als 2147 Bilder (32-Bit-Überlauf in `CImageAvi_Seek`) ist nur `S112_CREDITS.AVI`
  mit 2905 Bildern. Loc41 spielt ihn linear ab (`playEnd`), es gibt kein Seek.

Werkzeug: `tools/iv5ref/build.sh <ordner>` (clang/lld-link, i686, ohne CRT),
`python3 -I tools/iv5ref/avi2bin.py <avi> <ordner>/in.bin [bpp] [bilder]`, dann im Ordner
mit `ir50_32.dll` daneben `wine iv5ref.exe` → `out.bin`;
`python3 -I tools/iv5ref/iv5mask.py <avi> <out.bin> <bilder>` vergleicht.
