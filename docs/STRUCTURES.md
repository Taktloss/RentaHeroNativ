# Klassen und Strukturen

Quellen: Konstruktormuster (`re/scripts/FindVtables.java` → vtables), `operator new`-Größen
(`re/scripts/NewSizes.java`), Dekompilate, Codegenerator-Vorlage im Binary (BINARY.md).
Namen: `docs/symbols/names.tsv` (per `ApplyNames.java` ins Ghidra-Projekt übertragen).

Belegstufen: **V** = Vorlage + Code stimmen überein, **C** = aus Code abgeleitet,
**?** = Kandidat (UNVERIFIED).

## Übersicht

| Klasse | Größe | Konstruktor | vtable (Slots) | Stufe | Begründung |
|---|---:|---|---|---|---|
| `CGame` (Basis aller Schauplätze) | – | `0x0043F4D0` | `0x0046B8D8` (2) | V | von jedem `LocNN`-Ctor zuerst aufgerufen; Slots = Destruktor, `Start()` (leer) |
| `LocNN : CGame` (33 Klassen) | 0xA0–0x178 | siehe unten | `0x0046B414`–`0x0046B530` (je 2) | V | setzt `CurLocation`, `CurSzene`, `FromSzene`, `View` wie die Vorlage |
| `CSzene` (View) | 0xACC | `0x004483A0` | keine | C | in WinMain angelegt (`g_CurScene`); Felder passen zum SZN-Lader |
| `CActor : CAnimLayer` (Figur) | 0x1B00 | `0x004338A0` | `0x0046B538` (10) | C | teilt Slots 2, 3 mit `CAnimLayer`; Felder `+0x15CC` Direction, `+0x160C…` Farb-Arrays, `+0x16D8` Zpos, `+0x1AEC` Type wie im SZN-Lader |
| `CGameState` | 0x4110 | `0x0044DA60` | keine | C | Größe = Zustandsblock im Spielstand |
| `CLayer` (statische Ebene) | 0x2B0 | `0x004466A0` | `0x0046B918` (9) | C | `CSzene_CreateLayer` bei `fps == 0` |
| `CAnimLayer` (animierte Ebene) | 0x15C8 | `0x004422E0` | `0x0046B8E0` (10) | C | `CSzene_CreateLayer` bei `fps != 0`; teilt Slots 5, 7, 8 mit `CLayer` |
| `CImage` (abstrakt) | – | – | `0x0046B7A8` (14, mehrere `_purecall`) | C | Basis der Bildformat-Klassen |
| `CImageBmp` | 0x394 | `0x004381E0` | `0x0046B770` (14) | C | Typ 0 in `CLayer_CreateImage` |
| `CImageTcc` | 0x3A4 | `0x00438390` | `0x0046B7E0` (14) | C | Typ 1 |
| `CImageA16 : CImageTcc` | 0xABC | `0x0043BAA0` | `0x0046B820` (14) | C | Typ 2; Ctor ruft `CImageTcc`-Ctor |
| `CImageAvi` | 0x478 | `0x0043C5C0` | `0x0046B890` (14) | C | Typ 3 |
| (unbenutzt) | ? | `0x0043BEE0` | `0x0046B858` (14) | ? | erbt `CImage`; keine Aufrufer → vermutlich toter Code |
| `IRenderer` (abstrakt) | – | – | `0x0046B8CC` (3× `_purecall`) | C | Schnittstelle |
| Renderer Vollbild | global | `0x00401CF0` | `0x0046B3FC` (3) | C | Objekt `0x0047C870`, gewählt bei `FullScreen` |
| Renderer Fenster | global | `0x00401D60` | `0x0046B408` (3) | C | Objekt `0x0047C230` |
| Audio-Manager | global | `0x00436530` | `0x0046B718` (1) | ? | Objekt `0x0047BF10`; Lautstärken `+0x6C…`, 100 Puffer-Slots ab `+0x84` |
| Audiopuffer | 0x188 | `0x00436C30` | `0x0046B71C` (8) | ? | Kandidat `CAudioBuffer` |
| Musik/Stream | 0x1A4 | `0x004376C0` | `0x0046B750` (8) | ? | Kandidat Musikobjekt (`FadeIn` `0x00437FC0`) |
| … | 0x22C | `0x004436B0` | `0x0046B908` (2) | ? | |
| … | 0x140 | `0x0043D620` | `0x0046B8C8` (1) | ? | |
| `CEvent` | 0x24 | `0x0043F130` | – | C | 48× erzeugt; Felder und Auslösen siehe `docs/EVENTS.md` |
| Timer | 0x18 | `0x0044D790` | – | ? | neben `timeSetEvent`-Aufruf `0x0044D7D0` |
| WAV-Datei | 0x5C | `0x0044DE80` | – | ? | neben `mmio*`-Funktionen |
| weitere | 0xBC, 0x42C, 0xE8, 0x16C, 0x400 | `0x00435F80`, `0x00441750`, `0x00444C60`, `0x00445C50`, `0x00445FB0` | – | ? | |

`_purecall` = `0x0044F3E0` (`_amsg_exit(0x19)`, Laufzeitfehler R6025).

## CGame-Felder (aus `Loc05_ctor` `0x0040D530` und `Loc05_Start` `0x0040D890`)

| Offset | Name (Vorlage) | Beleg |
|---|---|---|
| `+0x00` | vtable | |
| `+0x04` | ? | `Start()` verzweigt bei `== 0` |
| `+0x08` | `Music` | `CMusic_FadeIn(this->+0x08)` am Anfang von `Start()` (V) |
| `+0x1C` | Figur (Hero?) | Methodenaufrufe mit Koordinaten, z. B. `FUN_00434ab0(+0x1C, 0x137, 0xDA)` (?) |
| `+0x28` | `CurLocation` | Konstante im Ctor (V) |
| `+0x2C` | `CurSzene` | Ctor-Parameter 2 (V) |
| `+0x30` | `FromSzene` | Ctor-Parameter 3 (V) |
| `+0x50` | `View` (`CSzene*`) | Ctor-Parameter 1 (V) |
| `+0x78` | `CGameState*` | Zugriffe `+0x3EC8`, `+0x3ED8` = Flag-Bereich des Spielstands (C) |
| ab Basisgröße | szenenspezifische Zeiger (`xxxWav`, `xxx`-Areas/Layer) | Vorlage, z. B. Loc05 `+0x7C`, `+0x80` = `GetObject("S10_DoorOpen")`, `("S10_LouiTalk")` |

Methoden: `CGame_SetEventProc` `0x0043FA30` (Argument = Zeiger auf `LocNN::OnGenericEvent`),
`CGame_LoadSfx` `0x004402F0(name)`, `CGame_LoadSfxVol` `0x004403A0(name, vol)`.
`CSzene_Load` `0x004492E0(view, "S%d", LoadMask)`, `CSzene_GetObject` `0x0044AF60(view, name)`,
`CSzene_DisableAll` `0x0044B0C0`, `CLayer_Enable` `0x00447470`.

## Schauplatz-Klassen

Erzeugt in `Game_SwitchLocation` (`0x004036E0`). `CurLocation` aus `mov [reg+0x28], imm`
im jeweiligen Konstruktor.

| Loc | Ctor | Größe | vtable | | Loc | Ctor | Größe | vtable |
|---:|---|---:|---|---|---:|---|---:|---|
| 1 | `0x00405530` | 0x15C | `0x0046B414` | | 20 | `0x004223E0` | 0xBC | `0x0046B4B8` |
| 2 | `0x00407E50` | 0x104 | `0x0046B41C` | | 21 | `0x00422D90` | 0x104 | `0x0046B4C0` |
| 3 | `0x0040A4E0` | 0xEC | `0x0046B424` | | 22 | `0x00425CF0` | 0x12C | `0x0046B4C8` |
| 4 | `0x0040B8C0` | 0x130 | `0x0046B42C` | | 23 | `0x00426A20` | 0xDC | `0x0046B4D0` |
| 5 | `0x0040D530` | 0xA0 | `0x0046B434` | | 24 | `0x00427C10` | 0x114 | `0x0046B4D8` |
| 6 | `0x0040E730` | 0xDC | `0x0046B43C` | | 25 | `0x00429020` | 0x118 | `0x0046B4E0` |
| 7 | `0x0040F7B0` | 0xB4 | `0x0046B444` | | 26 | `0x0042A070` | 0xDC | `0x0046B4E8` |
| 8 | `0x00411640` | 0x11C | `0x0046B44C` | | 27 | `0x0042AE20` | 0x110 | `0x0046B4F0` |
| 9 | `0x004134B0` | 0xBC | `0x0046B454` | | 28 | `0x0042C6C0` | 0x14C | `0x0046B4F8` |
| 10 | `0x00413FF0` | 0x178 | `0x0046B45C` | | 29 | `0x0042EE50` | 0xB0 | `0x0046B500` |
| 12 | `0x00418700` | 0xB0 | `0x0046B480` | | 30 | `0x0042F750` | 0xA8 | `0x0046B508` |
| 14 | `0x00419650` | 0xC8 | `0x0046B488` | | 31 | `0x00430360` | 0xB8 | `0x0046B510` |
| 15 | `0x0041A330` | 0xE4 | `0x0046B490` | | 35* | `0x00430A20` | 0xC0 | `0x0046B518` |
| 16 | `0x0041B290` | 0xF4 | `0x0046B498` | | 37 | `0x00431260` | 0xAC | `0x0046B520` |
| 17 | `0x0041CF00` | 0x13C | `0x0046B4A0` | | 40 | `0x00431920` | 0x100 | `0x0046B528` |
| 18 | `0x0041E4F0` | 0xE0 | `0x0046B4A8` | | 41 | `0x004332A0` | 0xA8 | `0x0046B530` |
| 19 | `0x0041F070` | 0x11C | `0x0046B4B0` | | | | | |

\* Loc35 (Gleiter-Szene, lädt `S1_Back`, `S1_RodChecksItems` … aus `GAME/LOC35`) setzt im
Konstruktor `CurLocation = 1` (`0x00430A64`, `eax = 1`) und `CurSzene` fest – Eigenheit des
Originals, im Port so übernehmen. Die Zuordnung „35“ beruht auf den geladenen Dateien.

Kein eigener Code für LOC11, 13, 32–34, 36, 38, 39 (diese Ordner existieren auch nicht).

## Ebenen und Bilder

`CSzene_CreateLayer` (`0x0044AB10`, aufgerufen vom SZN-Lader):
- freier Platz im Ebenen-Array `CSzene+0x04 + i*4` (max. 40, sonst Rückgabe 0)
- `fps == 0` → `new CLayer` (0x2B0), sonst `new CAnimLayer` (0x15C8)
- vtable `+0x08` `Load(hwnd, ?, filename, ?)` – bei Fehler: Ebene löschen, 0 zurück
- vtable `+0x0C` danach (bei `CLayer`: `CLayer_CreateImage`), dann `+0x14 SetPos(0, CSzene+0x2F0)`
- `CSzene+0x2B8` = Index, `CSzene+0x5D8` = zuletzt erzeugte Ebene
- legt automatisch einen Klickbereich mit dem Ebenennamen an (`CSzene_AddArea` ohne Rechteck)
  und verknüpft ihn mit der Ebene (`FUN_00436060`)

`CLayer_Load` (`0x00446940`, Slot 2): kopiert den Pfad nach `+0x5C`, prüft die Datei
(`FUN_0044e810`), bestimmt den Typ `+0x220` aus den **letzten drei Zeichen** per
`lstrcmpA` (case-sensitiv!): `BMP`/`bmp`→0, `TCC`/`tcc`→1, `A16`/`a16`→2, `AVI`/`avi`→3,
sonst 4. Gemischte Schreibweisen (`Bmp`) ergeben 4 und scheitern später – kommt in den
Daten nicht vor (nur `a16 A16 avi bmp BMP TCC`), im Port aber genauso behandeln.

`CLayer_CreateImage` (`0x00446AC0`, Slot 3): erzeugt `CImage*` nach Typ in `+0x0C`;
Typ 1 (TCC) setzt `+0x19C` (= `DirtyAlpha`-Flag) zwingend; ist es gesetzt, auch
`image+0x37C = 1`. Danach `image->Load(layer+0x5C)` (Slot 0); übernimmt
Breite `image+0x338` → `+0x1E8`, Höhe `image+0x33C` → `+0x1EC`, Rechtecke `+0x1F0`/`+0x200`
= (0, 0, w, h); `image+0x334 != 0` → `+0x198 = 1` (Bedeutung offen, evtl. Transparenz).
