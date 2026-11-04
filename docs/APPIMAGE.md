# Linux-AppImage (Port)

Ein AppImage ist eine einzelne ausführbare Datei, die ohne Installation auf den meisten
x86_64-Linux-Systemen läuft. Die Originaldaten (CDs bzw. Abbilder) sind wie immer **nicht**
enthalten. Sie werden im Startmenü gewählt (LAUNCHER.md).

## Bauen

```sh
tools/appimage/build.sh            # im Container (podman oder docker)
tools/appimage/build.sh --jobs 8
tools/appimage/build.sh --no-container   # direkt auf dem Rechner (Ergebnis nur so portabel wie dessen glibc)
```

Ergebnis: `build-appimage/RentAHero-<git-describe>-x86_64.AppImage` und derselbe Inhalt als
einfaches Archiv `build-appimage/rentahero-linux-x86_64-<git-describe>.tar.gz` (`bin/rentahero`,
`lib/` mit FFmpeg, `share/`; RPATH `$ORIGIN/../lib`, ohne FUSE). Der Ordner ist per `.gitignore`
ausgeschlossen.

- **Container** `tools/appimage/Containerfile`: Ubuntu 22.04 (glibc 2.35), GCC 12, CMake 3.31.
  Er enthält nur Entwicklerpakete für X11, Wayland, PipeWire/Pulse/ALSA und GL/EGL. Das AppImage
  läuft deshalb auf allen Systemen mit glibc ≥ 2.35: Ubuntu 22.04+, Debian 12+, Fedora 36+,
  aktuelles Arch/openSUSE/SteamOS 3.
- **Abhängigkeiten** werden aus den Quellen gebaut. Versionen und Prüfsummen sind dieselben wie
  beim Windows-Build und stehen in `tools/deps/sources.sh`.
  - SDL 3.4.18, FreeType 2.14.3, unshield 1.6.2 und zlib 1.3.1: statisch. SDL lädt X11, Wayland,
    Audio und OpenGL zur Laufzeit vom System.
  - FFmpeg 8.1.3: Wegen der LGPL liegt es als gemeinsame Bibliotheken unter `usr/lib`
    (avcodec, avformat, avutil, swscale). Gebaut sind nur AVI, Cinepak, Indeo 3/5 und swscale.
  - libstdc++/libgcc: statisch.
  - Das Skript bricht ab, wenn das Programm außer glibc und FFmpeg noch etwas anderes braucht.
- **Werkzeuge** mit fester Version und Prüfsumme (wie auf GitHub veröffentlicht):
  appimagetool 1.9.1 und type2-runtime 20251108. Diese Runtime ist statisch und braucht kein
  `libfuse2` auf dem Zielsystem. Fehlt FUSE ganz, hilft `--appimage-extract-and-run`.
- **Inhalt**:
  - `usr/bin/rentahero` mit RPATH `$ORIGIN/../lib`.
  - Desktop-Datei und ein eigenes Symbol (`tools/appimage/rentahero.svg`, kein Original-Symbol).
  - `usr/share/doc/rentahero/`: LICENSE, THIRD-PARTY.txt und die Lizenztexte, wie im
    Windows-Paket.
- Beim Bau laufen die Tests ohne Originaldaten (`rah_tests`).

## Laufzeit

- Schrift: Wie überall wird die System-Arial verwendet, sonst Liberation Sans
  (`fonts-liberation` bzw. `liberation-sans-fonts`). Die Schrift ist nicht eingebettet.
- Einstellungen und Spielstände liegen in `~/.local/share/RentaHeroNativ/`, wie beim normalen Build.
- Dateidialoge laufen über xdg-desktop-portal bzw. zenity (LAUNCHER.md).

## Geprüft (2026-11-04)

- Bau im podman-Container, 157 Tests ohne Fehler (13 ohne Originaldaten übersprungen).
- Start auf Fedora 44 (Mesa, glibc 2.43): Startmenü, FSR mit Entbänderung über OpenGL, Intro-Video
  bildgleich mit dem normalen Build (`--shot` verglichen).

**Nicht geprüft:** Start auf einem anderen System (z. B. Ubuntu 22.04 mit Desktop, SteamOS).
