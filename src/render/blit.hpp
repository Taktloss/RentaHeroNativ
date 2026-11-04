// SPDX-License-Identifier: GPL-3.0-or-later
// Blitter-Kerne für TCC-Bilder auf RGB565 (Formeln und Eigenheiten: docs/RENDERER.md,
// docs/COMPOSITOR.md). Ziel wird wie im Original linear adressiert (Startpixel + Zeile · Pitch);
// Pitch = Breite · 2 Bytes. Das Original rechnet mit lPitch aus Lock (0x004399E0: DDSURFACEDESC+0x10),
// Wine richtet auf 8 Bytes aus (WINEPROBE.md) – für den Port gleichwertig. Schreibzugriffe außerhalb des
// Puffers werden verworfen.
#pragma once

#include <cstdint>

#include "core/rect.hpp"
#include "formats/tcc.hpp"
#include "render/surface.hpp"

namespace rah {

// Tiefenkarte (640 × 360) ab dem Zielstartpixel, Zeilenlänge 640; gezeichnet nur bei depth > z.
// orig: 0x00439EC0 / 0x0043A0B0 (Zeiger map + top·640 + left; Szenenkoordinaten, COMPOSITOR.md)
struct DepthTest {
    const std::uint8_t* map = nullptr;
    int z = 0;
};

// orig: Rent-A-Hero.exe 0x004399E0 Blit565_Opaque – immer das ganze Bild (w × h) ab (dstX, dstY),
// ohne Clipping und ohne Alpha.
void blitOpaque565(Surface565& s, const TccPlanes& p, int dstX, int dstY);

// orig: Rent-A-Hero.exe 0x00439A80 Blit565_Alpha, 0x00439EC0 Blit565_AlphaDepth –
// Quelle ab Index srcOff, width × height, danach je Zeile srcSkip Pixel überspringen.
void blitAlpha565(Surface565& s, const TccPlanes& p, std::size_t srcOff, int width, int height, int srcSkip,
                  int dstX, int dstY, const DepthTest* depth);

// orig: Rent-A-Hero.exe 0x00439C30 (Alpha + Zoom), 0x0043A0B0 (Alpha + Zoom + Tiefe):
// `full` = gesamtes Zielrechteck (Schrittweite), `dst` = sichtbarer Ausschnitt,
// clipL/clipT = Ränder in Zielpixeln (+0x34C/+0x350).
void blitAlphaZoom565(Surface565& s, const TccPlanes& p, const Rect& full, const Rect& dst, int clipL,
                      int clipT, const DepthTest* depth);

}  // namespace rah
