# Standardformate: BMP, WAV, AVI

## BMP (`CImageBmp_Load` `0x004382C0`)
`LoadImageA(NULL, pfad, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION)`;
Größe aus `GetObject`. Keine eigene Verarbeitung. Daten: 93 Dateien, alle
`BITMAPINFOHEADER` 24 Bit unkomprimiert (90 × 640×360 Hintergründe, 3 kleinere).
Port: einfacher 24-Bit-BMP-Lader genügt (bottom-up, Zeilen auf 4 Bytes ausgerichtet);
andere Varianten ablehnen.

## WAV (`CWave_Open` `0x0044DF00`, Objekt 0x5C, Ctor `0x0044DE80`)
RIFF/WAVE über `mmio*`: `fmt `-Chunk (16 Bytes + bei Nicht-PCM `cbSize` + Extra),
danach `data` (`FUN_0044e0f0`). Abgeleitet:
`+0x54` = Datengröße, `+0x48` = Dauer in ms = `size * 1000 / nAvgBytesPerSec`
(64-Bit-Rechnung, abgerundet). Fehlercodes `+0x08`: 0x103 Öffnen, 0x105 Lesen,
0x102 Speicher. Daten: PCM 22 050 Hz mono, 8/16 Bit (FILES.md).

## AVI als Szenenebene (`CImageAvi_Load` `0x0043C7C0`)
DirectShow-Multimedia-Streaming, nicht AVIFile:
`CoCreateInstance(CLSID_AMMultiMediaStream)` → `IAMMultiMediaStream::OpenFile` →
`GetMediaStream(MSPID_PrimaryVideo)` → `IDirectDrawMediaStream` → Format, Sample
(DirectDraw-Oberfläche). Dauer/Zeit in `+0x3C4…+0x3D0`
(`STREAM_TIME / 1 000 000`, also Zehntelsekunden – UNVERIFIED, mit Laufzeit prüfen).
- **Sonderfall 640×180**: Höhe wird als 360 gemeldet, `+0x3D8 = 1` (Zeilenverdopplung).
  Kein solches Video in den Daten.
- Start: `SetState(RUN)` (`+0x1C`, 1); Rücksprung an den Anfang `0x0043C9A0`
  (`Seek(0)`, dann nächstes Bild).

Daten: 124 AVI-Dateien, alle **10 fps**; Codecs Cinepak (`cvid`, 93), Indeo 5 (`IV50`,
29), Indeo 3 (`IV32`, 2). FFmpeg dekodiert alle drei (`cinepak`, `indeo5`, `indeo3`).

Es gibt **keinen separaten Vollbild-Videopfad**: alle 124 AVIs liegen in Schauplatz-
ordnern (`GAME/LOCnn`, Intro in `LOC40` – Startschauplatz eines neuen Spiels) und
werden als 640×360-Ebenen abgespielt. `0x004050B0` erzeugt `AMMultiMediaStream` nur
probeweise beim Start (Prüfung, ob DirectX Media installiert ist), `0x004050F0`
prüft analog Indeo 5 (`ICOpen`).

Die zusätzlich importierten `AVIStream*`- und `DrawDib*`-Funktionen gehören zur
Klasse mit vtable `0x0046B858` (Ctor `0x0043BEE0`, Dtor `0x0043BFC0`, Zeichnen
`0x0043C440`), die nirgends erzeugt wird (STRUCTURES.md) – vermutlich eine ältere
AVIFile-Lösung. Für den Port irrelevant.
