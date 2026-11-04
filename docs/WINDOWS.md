# Windows-Build (x86_64, Windows 10/11)

Entscheidung 2026-11-04 (CLAUDE.md): Cross-Build unter Linux mit **llvm-mingw** (Clang, UCRT),
Abhängigkeiten aus den Quellen, **FFmpeg als DLLs** (LGPL).

## Bauen

```sh
tools/windows/build.sh            # [--jobs N]; LLVM_MINGW=<pfad> optional
```

- Toolchain: `~/.local/opt/llvm-mingw-20261006-ucrt-ubuntu-22.04-x86_64` (Release 20261006,
  SHA-256 des Archivs `5f9c6ed95b2d4bdb…`), ohne root. `cmake/llvm-mingw.cmake` findet sie über
  `LLVM_MINGW` oder nimmt die neueste unter `~/.local/opt`.
- Quellen nach `build-win/src` (Prüfsummen im Skript fest; zlib entspricht der offiziellen
  Prüfsumme, die übrigen sind beim ersten Herunterladen festgelegt), Bibliotheken nach
  `build-win/deps`, Port nach `build-win/port`, Ergebnis in `build-win/dist` und als
  `build-win/rentahero-win64-<git>.zip`. `build-win/` ist nicht eingecheckt.
- Schritte werden über `deps/.done-*` übersprungen. Neu bauen: die Datei bzw. `build-win/` löschen.

| Bibliothek | Version | Art | Lizenz |
|---|---|---|---|
| SDL | 3.4.18 | statisch | zlib |
| FreeType | 2.14.3 (ohne zlib/png/bzip2/harfbuzz/brotli) | statisch | FTL |
| FFmpeg | 8.1.3: nur AVI-Demuxer, Decoder `cinepak`/`indeo3`/`indeo5`, swscale, `--enable-small`, ohne x86-Assembler (`nasm` fehlt) | **DLLs** | LGPL-2.1+ |
| unshield | 1.6.2 (eigenes MD5) | statisch | MIT |
| zlib | 1.3.1 | statisch | zlib |
| libc++/libunwind | aus llvm-mingw | DLLs | Apache-2.0 mit LLVM-Ausnahme |

Dieselben Versionen wie der Linux-Build (Fedora 44). Lizenztexte und die configure-Zeile von
FFmpeg legt das Skript nach `dist/licenses/` bzw. `dist/THIRD-PARTY.txt`.

## Anpassungen für Windows

- **GUI-Programm** (`WIN32_EXECUTABLE`), also kein Konsolenfenster. `src/app/platform.cpp`:
  - Ist die Ausgabe schon umgeleitet (Datei/Pipe, auch unter Wine), bleibt sie so.
  - Sonst hängt sich das Programm an die Konsole des Aufrufers (cmd/PowerShell), dann
    erscheinen `--help`/`--debug` dort.
  - Ohne Konsole (Doppelklick) geht die Ausgabe nach `%APPDATA%\RentaHeroNativ\rentahero-log.txt`.
    Endet das Programm mit Fehler, zeigt ein Meldungsfenster das Protokoll.
- **Manifest** (`src/app/rentahero.manifest`, per `rentahero.rc` eingebunden):
  - `activeCodePage = UTF-8` (Windows 10 1903+): `argv`, `std::filesystem::path` aus `char` und
    die `…A`-APIs arbeiten in UTF-8. Pfade mit Umlauten funktionieren also, z. B. ein Benutzername
    `Jörg`; unter Wine mit `--save-dir …\Spielstände_Jörg` geprüft.
  - `PerMonitorV2`-DPI.
  - Windows 10/11 als unterstütztes System.
  - Kein Programmsymbol, weil das des Originals Originaldaten wären.
- Schrift: `%WINDIR%\Fonts\arial*.ttf` (Projektentscheidung „System-Arial“), vorhanden ab Werk.
- Benutzer-Datenordner (`SDL_GetPrefPath`): `%APPDATA%\RentaHeroNativ\` mit `Install\` (aus
  `DATA1.CAB` entpackt), Spielständen und Protokoll.
- Behobene Portabilitätsfehler: fehlendes `<algorithm>` (`engine/area.hpp`, libc++ strenger),
  `class`/`struct` bei `Surface565`; ungenutztes `libswresample` aus CMake entfernt.

## Geprüft (unter Wine, 2026-11-04)

- `rah_tests.exe`: 150 Tests, 0 Fehler. Mit `RAH_TEST_DATA` (Z:-Pfad) auch die Datentests
  (102 TCC, 219 A16). Zwei Schrift-Tests werden übersprungen, weil der Wine-Prefix kein
  `windows\Fonts\arial.ttf` hat.
- `rentahero.exe` ohne `--common`: entpackt `DATA1.CAB` nach `%APPDATA%\RentaHeroNativ\Install`
  (129 Common-Dateien + EXE), lädt Schauplatz 1/Szene 7, das Bild entspricht dem Linux-Build
  (anderes Animationsbild wegen der Echtzeit).
- `--auto-load` aus einem Ordner mit Umlauten lädt den Spielstand.

**Nicht geprüft:** echtes Windows 10/11, also Fenster, Ton (WASAPI), Eingabe, DPI und die
Meldungsfenster. Das muss auf einem echten Rechner getestet werden.
