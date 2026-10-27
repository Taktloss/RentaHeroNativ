# Analyse von Rent-A-Hero.exe

SHA-256 `a1ccd604…4927`, 716 873 Bytes. Werkzeuge: `tools/peinfo.py`, Ghidra 12.2
(`re/scripts/ExportSummary.java`). Vollständige Funktions- und Importliste:
`docs/symbols/functions.tsv`, `docs/symbols/imports.txt`.

## Format und Toolchain
- PE32, i386, GUI (Subsystem 2, Version 4.0), ImageBase `0x00400000`,
  EntryPoint `0x004524F0` (CRT-Startup in `.text`).
- Linkzeitpunkt 1999-03-19 17:21:38 UTC, Linker 6.0.
- Rich-Header: Compiler-Builds 8168 (C und C++), MASM 7299, Linker 8034,
  cvtres 1720 → **Microsoft Visual C++ 6.0** (Build 8168 = VC6 ohne bzw. vor SP3).
- **C++** (`operator new`, `__CxxFrameHandler`, `_CallMemberFunction0`), **ohne RTTI**
  (keine `.?AV`-Typnamen), **ohne MFC**, reines Win32.
- CRT **statisch** gelinkt, und zwar die **Debug-CRT** (`dbgheap.c`, `_CrtDbgReport`,
  `dbgdel.cpp`), obwohl der PDB-Pfad `Release` lautet.
- CodeView-Eintrag NB10 im Overlay: `D:\Development\Projects\Games\Rah\ws\Release\RahGame.pdb`
  (Signatur `0x3687EE6B`, Age 20). Die PDB liegt nicht bei. Interner Projektname: **RahGame**.
- Kein Packer und kein Protektor: Standardsektionen, Entropie normal (`.text` 6,27),
  keine TLS-Callbacks, keine Relokationen (feste ImageBase), keine Exporte.

## Sektionen

| Name | VA | VSize | Raw | Entropie |
|---|---|---|---|---:|
| .text | 0x00401000 | 0x69668 | 0x6A000 | 6,27 |
| .rdata | 0x0046B000 | 0x04F62 | 0x05000 | 5,42 |
| .data | 0x00470000 | 0x13694 | 0x0B000 | 4,38 |
| .rsrc | 0x00484000 | 0x33050 | 0x34000 | 3,97 |

`.data` ist virtuell 0x8694 Bytes größer als auf der Platte (BSS, ~34 KB).

## Grobe Speicheraufteilung von `.text` (Ghidra, 1417 Funktionen)

| Bereich | Funktionen | Bytes | Inhalt (belegt durch API-Aufrufe/Strings) |
|---|---:|---:|---|
| 0x00401000–0x004051FF | 37 | 10 716 | WinMain/Fenster mit Hauptschleife (PeekMessage), `Game_Idle` `0x004030C0`, Ortswechsel `0x004036E0` ( „Change FAILED to Location %d Szene %d“), `rah.ini`/Pfade/CD-Erkennung `0x00404B60`, Prüfung DirectX Media (`CoCreateInstance`, `0x004050B0`) und Indeo 5 (`ICOpen`, `0x004050F0`) |
| 0x00405200–0x00434FFF | 227 | 125 775 | **Spiellogik pro Schauplatz/Szene** (Funktionen referenzieren `S<nn>_…`, `Loc<nn>`, Sprach-IDs wie `099_ROD_08`); größte: `0x00414A50` (4,7 KB, Szene 37), Intro `0x00431920` |
| 0x00435000–0x0044EA4F | 450 | 77 216 | **Engine**: Grafik (DirectDraw/GDI/DrawDib), Timer (`timeSetEvent` `0x0044D7D0`), Eingabe, SZN-Lader `0x00449380` (`Background1`, `Layer%-d`, `LoadCond`, `DirtyAlpha`), WAV über mmio `0x0044DED0`…, Registry `0x00448290` |
| 0x0044EA50–0x0046A667 | 703 | 104 120 | VC6-CRT (statisch, Debug-Variante); Function ID erkennt nur ~330 davon |

Die Grenzen sind aus Strings und API-Aufrufen abgeleitet und nur ungefähr; sie werden
in Phase 2 genauer. UNVERIFIED für einzelne Funktionen nahe den Grenzen.

vtables (Ghidra, ≥3 Zeiger mit Referenz): `0x0046B414` (20 Einträge),
`0x0046B480` (30 Einträge) und zwei CRT-Tabellen. Das sind weniger, als die Klassen
erwarten lassen; wahrscheinlich hat die Suche noch Lücken, Phase 2 klärt das.

## Hinweise auf die Architektur des Originals

Die EXE enthält einen **Codegenerator des Original-Editors** (Strings `0x00470064`–
`0x00470758`, Dateifilter „Skripts (*.cpp)“). Er erzeugt aus einer SZN-Datei
(`[COMPILER]`-Sektion) ein C++-Gerüst. Daraus folgt die Klassenstruktur des Originals:

```cpp
#include <game.h>
class LocNN;
typedef void (LocNN::*CGAME_CALLBACKnn)(CEvent*, int);
class LocNN : public CGame {
public:
    LocNN(CSzene *CurView, int NewSzene, int OldSzene); // setzt View, CurSzene, FromSzene, CurLocation
    ~LocNN();
    void Start(void);                 // Music->FadeIn(), View->DisableAll(), Background->Enable(), SetEventProc(...)
    void OnGenericEvent(CEvent *thisEvent, int EventType);  // switch (thisEvent->Param) { case CLICKED_xxx: … }
protected:
    EVENTCALLBACK MakeEventPtr(CGAME_CALLBACKnn);  // Member-Funktionszeiger -> EVENTCALLBACK per _asm push/pop
    CAudioBuffer *xxxWav;  CArea *xxx;  CLayer *xxx;  CLayer *xxxAvi;
};
// Ressourcen: LoadMusic("MUSIC"), LoadSfx(".."), LoadAnim(".."),
// View->Load("S%d", LoadMask), View->GetArea(".."), View->GetLayer(".."), GetViewObjects()
```

Folgerungen:
- **Es gibt keinen Skriptinterpreter.** Die Logik ist kompiliertes C++, eine Klasse pro
  Schauplatz (`LocNN : CGame`), ereignisgesteuert über Callbacks (`CEvent`, `Param`,
  `CLICKED_*`). Szenen (`S<n>`) werden aus SZN-Daten geladen, und darauf greift der Code zu.
  Tool `scriptdis` entfällt; stattdessen werden die Szenenklassen dekompiliert und
  pro Schauplatz in `docs/GAMELOGIC.md` beschrieben.
- Engine-Klassen (Namen aus dem Generator): `CGame`, `CSzene`, `CLayer`, `CArea`, `CEvent`,
  `CAudioBuffer`, Musikobjekt mit `FadeIn`. Der Editor ist im Release enthalten:
  Dialoge `IDD_SZENEWAHL`, `IDD_SETAREA`, `IDD_COLORADD`, `IDD_INVENTORY`, `IDD_FILENAME`,
  Menü `APPMENU`, Accelerator-Tabelle, Drag & Drop (`DragQueryFileA`), Datei-Dialoge.
  Wahrscheinlich über `ShowMenu=1` in `rah.ini` erreichbar (UNVERIFIED) – sehr nützlich für die Analyse.

## Importierte APIs (247 Funktionen, Auswahl nach Bereich)

- **Grafik**: `DirectDrawCreate` (Rest über COM-vtable), GDI (`BitBlt`, `StretchBlt`,
  `SetPixel`/`GetPixel`, `CreateFontA`, `TextOutA`, `DrawTextA`), DrawDib (`DrawDibDraw` u. a.).
- **Sound**: `DSOUND` Ordinal 1 und 2 (= `DirectSoundCreate`, `DirectSoundEnumerateA`); WAV-Laden über `mmio*`.
- **Video**: AVIFile (`AVIStreamOpenFromFileA`, `AVIStreamRead`, …), `ICOpen`/`ICClose`.
- **Timing**: `timeSetEvent`/`timeKillEvent` (Multimedia-Timer), `timeGetTime`, `Sleep`.
  Kein `QueryPerformanceCounter`, kein `GetTickCount`, kein `timeBeginPeriod`.
- **Eingabe**: Fensternachrichten, `GetAsyncKeyState`, `GetKeyState`; kein DirectInput.
- **Dateien**: `CreateFileA`/`ReadFile`/`WriteFile`, `_lopen`/`_lread`, CRT-`fopen`.
- **CD/Konfiguration**: `GetLogicalDrives`, `GetDriveTypeA`, `GetPrivateProfile*`,
  `WritePrivateProfileStringA`, Registry (`RegOpenKeyExA` …).
- **Sonstiges**: `CreateMutexA` (Einzelinstanz?), `CoInitialize`/`CoCreateInstance`.

## Kopierschutz
Kein Kopierschutz-Code gefunden (keine Signaturen von SafeDisc, SecuROM oder LaserLock).
Die CD-Erkennung (`GetLogicalDrives`/`GetDriveTypeA` mit „Bitte legen Sie CD n ein…“)
dient dem Datenzugriff mit CD-Wechsel (`CDMap.ini`, `LastCdNr`). Im Port wird sie
durch ein konfigurierbares Datenverzeichnis ersetzt. Anti-Debug-Tricks oder
selbstmodifizierender Code: keine Hinweise (keine entsprechenden Importe, `.text` nicht
beschreibbar).

## Autorun-Starter (CD1 `RENT-A-HERO.EXE`)
PE32, VC6, Linkzeitpunkt 1998-10-11. Importiert DSETUP und MSVFW32, startet Setup,
DirectX 6, Indeo 5 und DirectMedia, danach das Spiel. Für den Port nicht relevant.
