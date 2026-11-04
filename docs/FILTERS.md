# Bildfilter und CRT-Shader (Port)

Entscheidung 2026-11-04 (CLAUDE.md): Die Darstellung läuft über **OpenGL 3.3 + GLSL**. Gibt es
kein OpenGL, wird SDL_Renderer ohne Filter verwendet. Umgeschaltet wird mit Alt-Tasten und einem
ImGui-Overlay, die Auswahl landet in `launcher.ini`. **Standard: „Original (Pixel)“, ganzzahlig
skaliert.** Das entspricht der bisherigen Darstellung. Das Original selbst läuft im
DirectDraw-Vollbild mit 640×480 (COMPOSITOR.md). Die Filter verändern nur die Ausgabe ins
Fenster, nie das 640×480-Spielbild, Spiellogik, Takt oder Prüfbilder (`--dump`).

## Einordnung

Das Spielbild ist vorgerenderte 3D-Grafik in 16 Bit (RGB565), dazu Cinepak- und Indeo-Videos.
Pixelart ist es nicht. Daraus folgt:

- **Empfohlen:**
  - **Sharp-Bilinear**: scharfe Pixel ohne ungleiche Breiten, auch bei Faktoren wie 2,25.
  - **AMD FSR 1**: EASU hochskalieren, RCAS nachschärfen.
  - **Entbänderung** gegen die Farbstufen des 16-Bit-Bildes, besonders in Himmel und Schatten.
- **Pixelart-Filter** (xBR, xBRZ, HQx, ScaleFX) sind für harte Kanten mit wenigen Farben
  gebaut. Hier glätten sie die Ränder der Figuren, auf Flächen wirken sie aber stellenweise
  „ölig“. Sie sind zum Ausprobieren dabei.
- **CRT** (Lottes, Geom) bringt Retro-Flair. Ein CRT-Shader rechnet seine Scanlines aus dem
  640×480-Bild und skaliert selbst. Ist er an, wird der gewählte Filter deshalb nicht verwendet;
  die Entbänderung läuft davor weiter.

| id | Name | Preset | Durchgänge |
|---|---|---|---|
| `original` | Original (Pixel) | – (nächster Nachbar) | 0 |
| `smooth` | Weich (bilinear) | – | 0 |
| `sharp` | Sharp-Bilinear | `interpolation/sharp-bilinear-simple.glslp` | 1 |
| `fsr` | AMD FSR 1 | `fsr/fsr.glslp` | 2 |
| `xbr` | xBR (lv2) | `xbr/xbr-lv2.glslp` | 1 |
| `xbrz` | xBRZ (freescale) | `xbrz/xbrz-freescale.glslp` | 1 |
| `hq2x`/`hq4x` | HQ2x/HQ4x | `hqx/hq2x.glslp`, `hq4x.glslp` (mit LUT-PNG) | 2 |
| `scalefx` | ScaleFX | `scalefx/scalefx.glslp` | 5 |
| CRT `lottes`/`geom` | CRT-Lottes/-Geom | `crt/crt-lottes.glslp`, `crt-geom.glslp` | 1 |
| Entbänderung | – | `misc/deband.glslp` (vor allem anderen) | +1 |

## Bedienung

- **Startmenü**, Reiter Optionen → „Bild“.
- **Im Spiel:**

  | Taste | Wirkung |
  |---|---|
  | Alt+F | Nächster Filter; mit Umschalt der vorige. Schaltet CRT aus. |
  | Alt+C | CRT: aus → Lottes → Geom |
  | Alt+D | Entbänderung an/aus |
  | Alt+I | Ganzzahlig skalieren / Fenster füllen (4:3) |
  | Alt+Enter | Vollbild |
  | Alt+O | Overlay „Bild“ mit allen Einstellungen. Esc schließt es. |
  | Tab (halten) | Hotspots der Szene anzeigen (HOTSPOTS.md) |

  Jede Änderung wird oben links 2,5 s lang eingeblendet und in `launcher.ini` gespeichert
  (`Filter`, `Crt`, `Deband`, `IntegerScale`, `Fullscreen`). Das Spiel läuft dabei weiter.
  Ist das Overlay offen, bekommt es Maus und Tastatur, sobald ImGui sie beansprucht. Esc öffnet
  dann nicht das Spielmenü.
- **Kein Konflikt mit dem Original:** Das Original belegt Esc und F1–F12 (MENU.md) sowie
  Ctrl+F-Tasten (CONTROLS.md). Alt+Buchstabe käme dort als `WM_SYSKEYDOWN` an, und das wertet
  es nicht aus.
- **Schalter** (auch ohne Menü):
  - `--filter <id>`, `--crt <off|lottes|geom>`, `--deband`, `--no-integer-scale`: Sie haben
    Vorrang vor `launcher.ini`.
  - `--renderer sdl` erzwingt den Weg ohne OpenGL.
  - `--shot <datei.bmp>` speichert am Ende das Fensterbild mit Filter.
  - `--sdl-key-at <bild> <alt+f|click:x,y>` speist Tasten oder Klicks ein. Das dient der Prüfung.

## Umsetzung (`src/app/present/`, `src/app/ui.cpp`)

- **`Presenter`**: Das Spielbild kommt als 640×480 RGB565 herein. Ausgabe ins Fenster:
  - Ganzzahlig: größter Faktor k ≥ 1, sonst einpassen. Füllen: 4:3 einpassen. Jeweils zentriert,
    mit schwarzem Rand.
  - Gezeichnet wird nur, wenn sich Bild, Fenstergröße oder Einstellung geändert haben oder ImGui
    sichtbar ist. Die Hauptschleife läuft wie bisher ohne VSync (`SDL_GL_SetSwapInterval(0)`).
    Unveränderte Bilder kosten so keine GPU-Zeit.
  - Mausereignisse werden über das Ausgaberechteck in Spielbild-Koordinaten umgerechnet und
    ersetzen `SDL_ConvertEventToRenderCoordinates`. Geprüft: Fenstermitte → (320,240), Ecke bei
    k = 2 → (−32,−24), in beiden Wegen.
  - Das Fenster hat `SDL_WINDOW_HIGH_PIXEL_DENSITY`, auf HiDPI-Bildschirmen werden also volle Pixel
    gerechnet.
- **OpenGL**:
  - 3.3 im Kompatibilitätsprofil, weil die libretro-Shader `#version 130` verlangen.
  - Funktionen lädt `SDL_GL_GetProcAddress`, gelinkt wird keine GL-Bibliothek. Sampler-Objekte
    regeln Abtastung und Rand je Durchgang.
- **libretro-GLSL** (`preset.cpp`, `presenter.cpp` `Chain`). Unterstützt wird, was die gewählten
  Presets nutzen:
  - `.glslp` mit `shaders`, `shaderN`, `filter_linearN`, `scale_type[_x|_y]N`
    (source/viewport/absolute), `scale[_x|_y]N`, `float_framebufferN` (RGBA16F) und `wrap_modeN`
    (`clamp_to_border`), dazu `textures` mit `<name>`/`<name>_linear` (PNG über stb_image) und
    `parameters`.
  - Ohne `scale_type` ist der letzte Durchgang `viewport`, alle anderen sind `source` 1,0.
  - Quelltext: Die `#version`-Zeile wandert nach vorn, dann `VERTEX`/`FRAGMENT` und
    `PARAMETER_UNIFORM`. Die `#pragma parameter`-Standards (bzw. die Werte aus dem Preset) werden
    als Uniforms gesetzt.
  - Uniforms: `MVPMatrix`, `FrameDirection`, `FrameCount`, `OutputSize`, `TextureSize`,
    `InputSize`, `Texture`, `Orig*`, `PassPrevN*` (Ausgabe von Durchgang i−N, i−N < 0 →
    Original), `PassN*` und LUTs. Attribute: `VertexCoord`, `TexCoord`, `COLOR`.
  - Alle Durchgänge rendern in eigene Framebuffer, Bildzeile 0 liegt oben. Ein eigener
    Ausgabedurchgang bringt das Ergebnis gespiegelt ins Ausgaberechteck: mit nächstem Nachbarn,
    wenn die Größe stimmt, sonst bilinear („Original“ immer nächster Nachbar).
  - Übersetzte Ketten werden zwischengespeichert. Das erste Umschalten auf ScaleFX dauert kurz,
    danach ist es sofort da. Scheitert ein Shader, kommt eine Meldung, und es gilt der direkte Weg.
- **Eingebettet**: `cmake/embed.cmake` legt alle Dateien aus `third_party/libretro-shaders` beim
  Konfigurieren als Bytefelder in `shaders_embedded.cpp` (≈ 0,2 MB). Ein Shader-Ordner neben der
  EXE ist nicht nötig.
- **SDL_Renderer**:
  - Bietet nur Original/Weich und die Skalierungsart. Das Overlay sagt dazu „Ohne OpenGL 3.3 nur
    …“.
  - Das ImGui-Backend skaliert bei gebrochener Pixeldichte (z. B. 1,1) die Beschneidung doppelt.
    Deshalb setzt der Port den Maßstab dort selbst über `SDL_SetRenderScale`.
- **Overlay/Startmenü**: Beide teilen einen ImGui-Kontext und `displayControls`. Das Startmenü
  nutzt jetzt dieselbe Darstellung, also das ImGui-OpenGL3-Backend.

## Lizenzen (`third_party/libretro-shaders/VERSION.txt`)

Die Shader sind unverändert übernommen, aus libretro/glsl-shaders, Commit `1aaabbeb…` (2026-11-02):

| Shader | Lizenz |
|---|---|
| sharp-bilinear-simple, crt-lottes | Public Domain |
| fsr, xbr-lv2, scalefx | MIT |
| xbrz-freescale | MIT, enthält xBRZ von Zenju (GPL-3.0) |
| hqx | LGPL-2.1+ |
| crt-geom, deband | GPL-2.0+ |

Alle sind mit GPL-3.0-or-later vereinbar. Das Programm als Ganzes steht unter GPL-3.0. Dazu kommt
`stb_image.h` 2.30 (MIT/Public Domain, `third_party/stb`). Das Windows-Paket enthält die
Lizenzköpfe aller Shader in `licenses/libretro-shaders.txt`.

## Geprüft (2026-11-04)

- **Linux** (Mesa 26.2, radeonsi): alle 9 Filter, beide CRT-Shader und FSR + Entbänderung übersetzt.
  `--shot` von Loc 1/S7: 12 verschiedene Bilder, Ausrichtung stimmt. Dazu Alt-Tasten, Overlay und
  `launcher.ini` über den echten Ereignisweg (`--sdl-key-at`), der SDL_Renderer-Ersatzweg mit
  Overlay, die Mausumrechnung und das Startmenü mit OpenGL.
- **Windows-Build unter Wine**: FSR, ScaleFX, CRT-Geom und das Overlay.
- Tests: `filters_*`, also Preset-Leser, Shader-Aufbereitung, alle Ketten eingebettet und
  vollständig, Umschalten und Ausgaberechteck.

**Nicht geprüft:** Treiber unter echtem Windows (AMD/NVIDIA/Intel). Wie ruckelfrei schwere Ketten
(ScaleFX, xBRZ) bei 4K laufen, wurde nicht gemessen.
